#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "CacheReader.h"
#include "DefinitionsLoader.h"
#include "VersionList.h"
#include "MapRegion.h"
#include "Model.h"
#include "RegionRenderer2D.h"

static std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

static std::string hexBytes(const std::vector<uint8_t>& bytes) {
    std::ostringstream out;
    for (std::size_t i = 0; i < bytes.size(); i++) {
        if (i > 0) out << ' ';
        out << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(bytes[i]);
    }
    return out.str();
}

static std::string listInts(const std::vector<int>& values) {
    if (values.empty())
        return "none";

    std::ostringstream out;
    for (std::size_t i = 0; i < values.size(); i++) {
        if (i > 0) out << ',';
        out << values[i];
    }
    return out.str();
}

static std::string listActions(const LocDef& loc) {
    std::ostringstream out;
    bool first = true;
    for (const std::string& option : loc.options) {
        if (option.empty())
            continue;
        if (!first) out << ", ";
        out << option;
        first = false;
    }
    return first ? "none" : out.str();
}

static bool matchesFilter(const MapObject& obj, const LocDef& loc, const std::string& filter) {
    if (filter.empty())
        return true;

    std::string f = lower(filter);
    if (f.rfind("type=", 0) == 0)
        return obj.type == std::stoi(f.substr(5));

    bool numeric = !f.empty() && std::all_of(f.begin(), f.end(), [](unsigned char c) { return std::isdigit(c); });
    if (numeric)
        return obj.id == std::stoi(f);

    if (lower(loc.name).find(f) != std::string::npos)
        return true;

    for (const std::string& option : loc.options) {
        if (lower(option).find(f) != std::string::npos)
            return true;
    }

    return false;
}

static void printObjectEntry(const MapObject& obj, const LocDef& loc, int baseX, int baseY) {
    std::cout << "  locId=" << obj.id
              << " name=\"" << loc.name << "\""
              << " local=(" << obj.x << "," << obj.y << "," << obj.z << ")"
              << " world=(" << baseX + obj.x << "," << baseY + obj.y << "," << obj.z << ")"
              << " type=" << obj.type
              << " rotation=" << obj.rotation
              << "\n";

    std::cout << "    bytes[" << obj.byteStart << ".." << obj.byteEnd << "): "
              << hexBytes(obj.bytes)
              << " | idDelta=" << obj.idDelta
              << " locDelta=" << obj.locationDelta
              << " packed=" << obj.packedLocation
              << "\n";

    std::cout << "    loc.def: size=" << loc.width << "x" << loc.length
              << " models=" << listInts(loc.modelIDs)
              << " actions=" << listActions(loc)
              << " blocksWalk=" << (loc.blocksWalk ? "yes" : "no")
              << " animation=" << loc.animationID
              << "\n";
}

static std::string floorName(const DefinitionsLoader& loader, uint8_t underlayId) {
    if (underlayId == 0)
        return "none";

    try {
        return loader.getFlo(underlayId - 1).name;
    } catch (...) {
        return "unknown";
    }
}

static void printTerrainByteDecode(const Tile& tile) {
    std::size_t i = 0;
    while (i < tile.bytes.size()) {
        uint8_t opcode = tile.bytes[i++];
        std::cout << "      opcode " << static_cast<int>(opcode) << ": ";

        if (opcode == 0) {
            std::cout << "end tile, generated/default height\n";
        } else if (opcode == 1) {
            if (i >= tile.bytes.size()) {
                std::cout << "explicit height but missing height byte\n";
                return;
            }
            uint8_t heightByte = tile.bytes[i++];
            int height = heightByte == 1 ? 0 : heightByte;
            std::cout << "explicit height byte " << static_cast<int>(heightByte)
                      << " -> height step " << height << "\n";
        } else if (opcode <= 49) {
            if (i >= tile.bytes.size()) {
                std::cout << "overlay opcode but missing overlay id byte\n";
                return;
            }
            uint8_t overlayId = tile.bytes[i++];
            std::cout << "overlay id " << static_cast<int>(overlayId)
                      << ", path=" << ((opcode - 2) / 4)
                      << ", rotation=" << ((opcode - 2) & 3)
                      << "\n";
        } else if (opcode <= 81) {
            std::cout << "settings=" << (opcode - 49) << "\n";
        } else {
            std::cout << "underlayId=" << (opcode - 81) << "\n";
        }
    }
}

static void printTerrainEntry(const Tile& tile, const DefinitionsLoader& loader, int baseX, int baseY, int plane, int x, int y) {
    std::cout << "  tile local=(" << x << "," << y << "," << plane << ")"
              << " world=(" << baseX + x << "," << baseY + y << "," << plane << ")"
              << " height=" << tile.height
              << " underlayId=" << static_cast<int>(tile.underlayId)
              << " floor=" << floorName(loader, tile.underlayId)
              << " overlayId=" << static_cast<int>(tile.overlayId)
              << " overlayPath=" << static_cast<int>(tile.overlayPath)
              << " overlayRotation=" << static_cast<int>(tile.overlayRotation)
              << " settings=" << static_cast<int>(tile.settings)
              << "\n";

    std::cout << "    bytes[" << tile.byteStart << ".." << tile.byteEnd << "): "
              << hexBytes(tile.bytes)
              << "\n";
    printTerrainByteDecode(tile);
}

static bool objectTouchesTile(const MapObject& obj, const LocDef& loc, int localX, int localY, int plane) {
    if (obj.z != plane)
        return false;

    int width = obj.type >= 10 ? std::max(1, loc.width) : 1;
    int length = obj.type >= 10 ? std::max(1, loc.length) : 1;
    if (obj.type >= 10 && (obj.rotation & 1) == 1)
        std::swap(width, length);

    return localX >= obj.x && localX < obj.x + width &&
           localY >= obj.y && localY < obj.y + length;
}

static int inspectTile(const MapRegion& region, const DefinitionsLoader& loader, int baseX, int baseY, int worldX, int worldY, int plane) {
    int localX = worldX - baseX;
    int localY = worldY - baseY;
    if (plane < 0 || plane >= 4) {
        std::cerr << "Tile inspect plane must be between 0 and 3\n";
        return 1;
    }
    if (localX < 0 || localX >= 64 || localY < 0 || localY >= 64) {
        std::cerr << "World tile (" << worldX << "," << worldY << ") is outside region "
                  << region.regionId() << " base=(" << baseX << "," << baseY << ")\n";
        return 1;
    }

    std::cout << "Tile inspect\n";
    const Tile& tile = region.terrain().getTile(plane, localX, localY);
    printTerrainEntry(tile, loader, baseX, baseY, plane, localX, localY);

    std::cout << "\nObjects touching this tile\n";
    int count = 0;
    for (const MapObject& obj : region.objects().getObjects()) {
        const LocDef& loc = loader.getLoc(obj.id);
        if (!objectTouchesTile(obj, loc, localX, localY, plane))
            continue;

        printObjectEntry(obj, loc, baseX, baseY);
        count++;
    }

    std::cout << "  object count=" << count << "\n";
    return 0;
}

static int inspectModel(CacheReader& reader, int modelId) {
    std::vector<uint8_t> modelData = reader.readGzippedFile(1, modelId);
    if (modelData.empty()) {
        std::cerr << "Failed to read/decompress model " << modelId << "\n";
        return 1;
    }

    Buffer modelBuf(modelData);
    Model model = Model::parse(modelBuf);
    ModelBounds bounds = model.bounds();

    std::cout << "Model " << modelId << "\n";
    std::cout << "  raw decompressed bytes: " << modelData.size() << "\n";
    std::cout << "  vertices: " << model.vertices.size() << "\n";
    std::cout << "  triangles: " << model.triangles.size() << "\n";
    std::cout << "  textured triangles: " << model.textureTriangles.size() << "\n";
    std::cout << "  vertex skins: " << model.vertexSkins.size() << "\n";
    std::cout << "  bounds x=[" << bounds.minX << "," << bounds.maxX << "]"
              << " y=[" << bounds.minY << "," << bounds.maxY << "]"
              << " z=[" << bounds.minZ << "," << bounds.maxZ << "]\n";

    std::cout << "\n  ALL vertices (" << model.vertices.size() << "):\n";
    for (std::size_t i = 0; i < model.vertices.size(); i++) {
        const ModelVertex& v = model.vertices[i];
        std::cout << "    v[" << i << "] = (" << v.x << ", " << v.y << ", " << v.z << ")\n";
    }

    std::cout << "\n  ALL triangles (" << model.triangles.size() << "):\n";
    std::cout << "    idx   a    b    c   color   H  S   L   type  prio  alpha  skin\n";
    for (std::size_t i = 0; i < model.triangles.size(); i++) {
        const ModelTriangle& t = model.triangles[i];
        int hue = (t.color >> 10) & 0x3F;
        int sat = (t.color >> 7)  & 0x07;
        int lit =  t.color        & 0x7F;
        std::cout << "    " << std::setw(3) << i
                  << "  " << std::setw(3) << t.a
                  << "  " << std::setw(3) << t.b
                  << "  " << std::setw(3) << t.c
                  << "  " << std::setw(5) << t.color
                  << "  " << std::setw(2) << hue
                  << "  " << sat
                  << "  " << std::setw(3) << lit
                  << "  " << std::setw(4) << t.renderType
                  << "  " << std::setw(4) << t.priority
                  << "  " << std::setw(5) << t.alpha
                  << "  " << std::setw(4) << t.skin
                  << "\n";
    }

    if (!model.textureTriangles.empty()) {
        std::cout << "\n  ALL texture triangles (" << model.textureTriangles.size() << "):\n";
        for (std::size_t i = 0; i < model.textureTriangles.size(); i++) {
            const ModelTextureTriangle& tt = model.textureTriangles[i];
            std::cout << "    tt[" << i << "] = (" << tt.a << ", " << tt.b << ", " << tt.c << ")\n";
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: tool <path to cache folder> [region id] [object filter]\n"
                  << "       tool <path to cache folder> model <model id>\n"
                  << "       tool <path to cache folder> <region id> render2d [output.ppm] [plane] [scale] [all|terrain|objects]\n"
                  << "       tool <path to cache folder> <region id> tile <worldX> <worldY> [plane]\n";
        return 1;
    }

    CacheReader reader;
    if (!reader.open(argv[1])) {
        std::cerr << "Failed to open cache at: " << argv[1] << std::endl;
        return 1;
    }

    try {
        if (argc >= 4 && std::string(argv[2]) == "model")
            return inspectModel(reader, std::stoi(argv[3]));

        Archive defsArchive = reader.readArchive(0, 2);
        DefinitionsLoader loader;
        loader.loadLocs(defsArchive);
        loader.loadFlos(defsArchive);

        Archive vArchive = reader.readArchive(0, 5);
        VersionList vList = VersionList::parse(vArchive);
        int regionId = 12850;
        if (argc >= 3)
            regionId = std::stoi(argv[2]);
        std::string objectFilter;
        if (argc >= 4)
            objectFilter = argv[3];

        MapRegion region = MapRegion::load(reader, vList, regionId);
        int baseX = (region.regionId() >> 8) * 64;
        int baseY = (region.regionId() & 0xff) * 64;

        if (objectFilter == "render2d") {
            std::string outputPath = "region_" + std::to_string(regionId) + ".ppm";
            if (argc >= 5)
                outputPath = argv[4];
            int plane = argc >= 6 ? std::stoi(argv[5]) : 0;
            int scale = argc >= 7 ? std::stoi(argv[6]) : 8;
            RenderLayer layer = argc >= 8 ? RegionRenderer2D::parseLayer(argv[7]) : RenderLayer::All;

            RegionImage image = RegionRenderer2D::render(region, loader, plane, scale, layer);
            image.savePpm(outputPath);

            std::cout << "Rendered region " << regionId << " plane " << plane << "\n";
            std::cout << "  base world coord: (" << baseX << "," << baseY << ")\n";
            std::cout << "  layer: " << RegionRenderer2D::layerName(layer) << "\n";
            std::cout << "  output: " << outputPath << "\n";
            std::cout << "  size: " << image.width() << "x" << image.height() << "\n";
            std::cout << "  terrain tiles: 16384 (64x64x4)\n";
            std::cout << "  object placements: " << region.objects().getObjects().size() << "\n";
            return 0;
        }

        if (objectFilter == "tile") {
            if (argc < 6)
                throw std::runtime_error("tile mode requires: <worldX> <worldY> [plane]");
            int worldX = std::stoi(argv[4]);
            int worldY = std::stoi(argv[5]);
            int plane = argc >= 7 ? std::stoi(argv[6]) : 0;
            return inspectTile(region, loader, baseX, baseY, worldX, worldY, plane);
        }

        std::cout << "Region " << region.regionId() << " map decode\n";
        std::cout << "  base world coord: (" << baseX << "," << baseY << ")\n";
        std::cout << "  terrain idx4 file: " << region.terrainFileId() << " (" << region.terrainDataSize() << " bytes decompressed)\n";
        std::cout << "  objects idx4 file: " << region.objectFileId() << " (" << region.objectDataSize() << " bytes decompressed)\n";
        std::cout << "  terrain tiles: 16384 (64x64x4)\n";
        std::cout << "  object placements: " << region.objects().getObjects().size() << "\n\n";

        std::cout << "Terrain sample with bytes, plane 0, y=32\n";
        for (int i = 30; i < 40; i++) {
            const Tile& t = region.terrain().getTile(0, i, 32);
            printTerrainEntry(t, loader, baseX, baseY, 0, i, 32);
        }

        std::cout << "\nObject placement sample with bytes\n";
        for (int i = 0; i < 20; i++) {
            if (i >= (int)region.objects().getObjects().size()) break;
            const auto& obj = region.objects().getObjects()[i];
            printObjectEntry(obj, loader.getLoc(obj.id), baseX, baseY);
        }

        if (objectFilter.empty())
            std::cout << "\nNamed object sample\n";
        else
            std::cout << "\nFiltered object sample matching \"" << objectFilter << "\"\n";

        int sampleCount = 0;
        for (const auto& obj : region.objects().getObjects()) {
            const LocDef& loc = loader.getLoc(obj.id);
            if (objectFilter.empty() && loc.name == "null")
                continue;
            if (!matchesFilter(obj, loc, objectFilter))
                continue;

            printObjectEntry(obj, loc, baseX, baseY);

            sampleCount++;
            if (sampleCount >= 30)
                break;
        }
        std::cout << "  sample count=" << sampleCount << "\n";

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
