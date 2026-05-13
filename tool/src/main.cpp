#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>

#include "CacheReader.h"
#include "DefinitionsLoader.h"
#include "ModelDef.h"
#include "TextureDef.h"
#include "VersionList.h"
#include "MapRegion.h"
#include "RegionRenderer2D.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: tool <path to cache folder> [region id] [object filter]\n"
                  << "       tool <path to cache folder> <region id> render2d [output.ppm] [plane] [scale] [all|terrain|objects]\n"
                  << "       tool <path to cache folder> <region id> tile <worldX> <worldY> [plane]\n"
                  << "       tool <path to cache folder> dump_model <model id>\n";
        return 1;
    }

    CacheReader reader;
    if (!reader.open(argv[1])) {
        std::cerr << "Failed to open cache at: " << argv[1] << std::endl;
        return 1;
    }

    try {
        // dump_textures: list all files in the textures JAG archive and probe naming patterns
        if (argc >= 3 && std::string(argv[2]) == "dump_textures") {
            auto texArchivePtr = reader.readArchive(0, 6);
            if (!texArchivePtr) { std::cerr << "Failed to read textures archive\n"; return 1; }
            Archive& texArchive = *texArchivePtr;
            std::cout << "Textures archive: " << texArchive.size() << " files\n\n";

            // list every file by hash and size
            std::cout << "All files (hash -> size):\n";
            std::map<uint32_t,int> sorted;
            for (auto& [hash, data] : texArchive.files)
                sorted[hash] = (int)data.size();
            for (auto& [hash, size] : sorted)
                std::cout << "  0x" << std::hex << std::setw(8) << std::setfill('0') << hash
                          << "  " << std::dec << size << " bytes\n";

            // probe common naming patterns to identify how textures are named
            std::cout << "\nProbing naming patterns (id 0..4):\n";
            for (int i = 0; i < 5; i++) {
                for (const std::string& name : {
                    std::to_string(i) + ".dat",
                    std::to_string(i),
                    "texture" + std::to_string(i) + ".dat"
                }) {
                    if (texArchive.hasFile(name)) {
                        const Buffer& d = texArchive.getFile(name);
                        std::cout << "  FOUND \"" << name << "\" -> " << d.size() << " bytes\n";
                    }
                }
            }
            return 0;
        }

        // dump_texture <id>: use TextureDef to decode and render as PPM
        if (argc >= 3 && std::string(argv[2]) == "dump_texture") {
            int texId = argc >= 4 ? std::stoi(argv[3]) : 0;
            auto texArchivePtr = reader.readArchive(0, 6);
            if (!texArchivePtr) { std::cerr << "Failed to read textures archive\n"; return 1; }
            Archive& texArchive = *texArchivePtr;

            // Get palette from index.dat
            const Buffer& indexData = texArchive.getFile("index.dat");
            if (indexData.empty()) {
                std::cerr << "index.dat not found in textures archive\n";
                return 1;
            }

            // Get texture file
            std::string name = std::to_string(texId) + ".dat";
            const Buffer& texData = texArchive.getFile(name);
            if (texData.empty()) {
                std::cerr << "Texture " << texId << " not found\n";
                return 1;
            }

            // Parse using TextureDef
            Buffer dataCopy(texData.slice(0, texData.size()));
            Buffer paletteCopy(indexData.slice(0, indexData.size()));
            TextureDef tex = TextureDef::parse(texId, dataCopy, paletteCopy);

            std::cout << "Texture " << texId << ": " << tex.width << "x" << tex.height
                      << ", " << tex.pixels.size() << " pixels, "
                      << tex.palette.size() / 3 << " palette colors\n";

            // Render as PPM
            std::string ppmPath = "texture_" + std::to_string(texId) + ".ppm";
            std::ofstream ppm(ppmPath, std::ios::binary);
            ppm << "P6\n" << tex.width << " " << tex.height << "\n255\n";
            for (int y = 0; y < tex.height; y++) {
                for (int x = 0; x < tex.width; x++) {
                    uint32_t rgb = tex.getPixelRGB(x, y);
                    ppm.put((rgb >> 16) & 0xFF);
                    ppm.put((rgb >> 8) & 0xFF);
                    ppm.put(rgb & 0xFF);
                }
            }
            std::cout << "Wrote " << ppmPath << "\n";
            return 0;
        }

        // dump_model: extract and inspect raw bytes of a single model entry
        if (argc >= 3 && std::string(argv[2]) == "dump_model") {
            int modelId = argc >= 4 ? std::stoi(argv[3]) : 0;

            if (!reader.hasFile(1, modelId)) {
                std::cerr << "Model " << modelId << " does not exist in archive 1\n";
                return 1;
            }

            Buffer data = reader.readGzippedFile(1, modelId);
            if (data.empty()) {
                std::cerr << "Failed to read/decompress model " << modelId << "\n";
                return 1;
            }
            int size = (int)data.size();

            std::cout << "Model " << modelId << " — decompressed size: " << size << " bytes\n\n";

            // hex dump of first 64 bytes
            int dumpLen = std::min(64, size);
            std::cout << "First " << dumpLen << " bytes:\n";
            for (int i = 0; i < dumpLen; i++) {
                if (i % 16 == 0)
                    std::cout << std::hex << std::setw(4) << std::setfill('0') << i << ":  ";
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data.peekByte() << " ";
                data.readByte(); // advance
                if (i % 16 == 15) std::cout << "\n";
            }
            std::cout << std::dec << "\n";

            if (size < 18) {
                std::cerr << "Model too small to have a valid footer (need 18 bytes)\n";
                return 1;
            }

            // parse footer: last 18 bytes
            Buffer footerBuf(data.slice(size - 18, size));
            
            auto readU16 = [&]() -> uint16_t {
                return footerBuf.readUShort();
            };

            uint16_t vertexCount          = readU16();
            uint16_t triangleCount        = readU16();
            uint8_t  texTriangleCount     = footerBuf.readByte();
            uint8_t  hasFaceRenderTypes   = footerBuf.readByte();
            uint8_t  priorityFlag         = footerBuf.readByte();
            uint8_t  hasFaceAlpha         = footerBuf.readByte();
            uint8_t  hasFaceSkins         = footerBuf.readByte();
            uint8_t  hasVertexSkins       = footerBuf.readByte();
            uint16_t vertexXLen           = readU16();
            uint16_t vertexYLen           = readU16();
            uint16_t vertexZLen           = readU16();
            uint16_t triIndexLen          = readU16();

            std::cout << "--- Footer (last 18 bytes) ---\n";
            std::cout << "  vertexCount:             " << vertexCount          << "\n";
            std::cout << "  triangleCount:           " << triangleCount        << "\n";
            std::cout << "  texTriangleCount:        " << (int)texTriangleCount    << "\n";
            std::cout << "  hasFaceRenderTypes:      " << (int)hasFaceRenderTypes  << "\n";
            std::cout << "  priorityFlag:            " << (int)priorityFlag
                      << (priorityFlag == 255 ? "  (per-face)" : "  (shared)") << "\n";
            std::cout << "  hasFaceAlpha:            " << (int)hasFaceAlpha    << "\n";
            std::cout << "  hasFaceSkins:            " << (int)hasFaceSkins    << "\n";
            std::cout << "  hasVertexSkins:          " << (int)hasVertexSkins  << "\n";
            std::cout << "  vertexXDataLength:       " << vertexXLen           << "\n";
            std::cout << "  vertexYDataLength:       " << vertexYLen           << "\n";
            std::cout << "  vertexZDataLength:       " << vertexZLen           << "\n";
            std::cout << "  triIndexDataLength:      " << triIndexLen          << "\n";

            // verify: compute expected size from footer and compare to actual
            int expected = vertexCount                                          // vertexFlags
                         + triangleCount                                        // triangleOpcodes
                         + (priorityFlag == 255  ? triangleCount : 0)          // facePriority
                         + (hasFaceSkins         ? triangleCount : 0)          // faceSkin
                         + (hasFaceRenderTypes   ? triangleCount : 0)          // faceRenderType
                         + (hasVertexSkins       ? vertexCount   : 0)          // vertexSkin
                         + (hasFaceAlpha         ? triangleCount : 0)          // faceAlpha
                         + triIndexLen                                          // triangleIndexData
                         + triangleCount * 2                                    // faceColor (ushort each)
                         + texTriangleCount * 6                                 // textureTriangle (3 ushorts)
                         + vertexXLen + vertexYLen + vertexZLen                 // vertex coord data
                         + 18;                                                  // footer itself

            std::cout << "\nExpected size from footer: " << expected << " bytes\n";
            std::cout << "Actual size:               " << size      << " bytes\n";
            std::cout << (expected == size ? "[OK] sizes match — format confirmed\n"
                                          : "[MISMATCH] format doc may be wrong or model is unusual\n");

            // parse and print decoded summary
            std::cout << "\n--- Parsed ModelDef ---\n";
            Buffer modelBuf(data.slice(0, data.size()));
            ModelDef def = ModelDef::parse(modelId, modelBuf);
            std::cout << "  vertices:  " << def.vertexX.size() << "\n";
            std::cout << "  triangles: " << def.triA.size()    << "\n";
            std::cout << "  texTris:   " << def.texP.size()    << "\n";
            if (!def.vertexX.empty()) {
                std::cout << "  vertex[0]: ("
                    << def.vertexX[0] << ", "
                    << def.vertexY[0] << ", "
                    << def.vertexZ[0] << ")\n";
            }
            if (!def.triA.empty()) {
                std::cout << "  tri[0]:    ("
                    << def.triA[0] << ", "
                    << def.triB[0] << ", "
                    << def.triC[0] << ")  color=0x"
                    << std::hex << def.triColor[0] << std::dec << "\n";
            }

            // render type breakdown
            if (!def.triRenderType.empty()) {
                std::map<int,int> typeCounts;
                for (uint8_t t : def.triRenderType) typeCounts[t]++;
                std::cout << "\n  Render type distribution:\n";
                for (auto& [type, count] : typeCounts)
                    std::cout << "    type " << type << ": " << count << " faces\n";

                std::cout << "\n  First 10 triangles (type | color | indices):\n";
                for (int i = 0; i < std::min(10,(int)def.triA.size()); i++) {
                    std::cout << "    [" << i << "] type=" << (int)def.triRenderType[i]
                              << "  color=0x" << std::hex << def.triColor[i] << std::dec
                              << "  (" << def.triA[i] << "," << def.triB[i] << "," << def.triC[i] << ")\n";
                }
            }
            return 0;
        }

        auto defsArchivePtr = reader.readArchive(0, 2);
        if (!defsArchivePtr) { std::cerr << "Failed to read definitions archive\n"; return 1; }
        Archive& defsArchive = *defsArchivePtr;
        DefinitionsLoader loader;
        loader.loadLocs(defsArchive);
        loader.loadFlos(defsArchive);

        auto vArchivePtr = reader.readArchive(0, 5);
        if (!vArchivePtr) { std::cerr << "Failed to read version archive\n"; return 1; }
        Archive& vArchive = *vArchivePtr;
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
            std::cout << "  output: " << outputPath << "\n";
            return 0;
        }

        std::cout << "Region " << region.regionId() << " map decode\n";
        std::cout << "  base world coord: (" << baseX << "," << baseY << ")\n";
        std::cout << "  terrain tiles: 16384 (64x64x4)\n";
        std::cout << "  object placements: " << region.objects().getObjects().size() << "\n";

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
