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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: tool <path to cache folder> [region id] [object filter]" << std::endl;
        return 1;
    }

    CacheReader reader;
    if (!reader.open(argv[1])) {
        std::cerr << "Failed to open cache at: " << argv[1] << std::endl;
        return 1;
    }

    try {
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

        std::cout << "Region " << region.regionId() << " map decode\n";
        std::cout << "  base world coord: (" << baseX << "," << baseY << ")\n";
        std::cout << "  terrain idx4 file: " << region.terrainFileId() << " (" << region.terrainDataSize() << " bytes decompressed)\n";
        std::cout << "  objects idx4 file: " << region.objectFileId() << " (" << region.objectDataSize() << " bytes decompressed)\n";
        std::cout << "  terrain tiles: 16384 (64x64x4)\n";
        std::cout << "  object placements: " << region.objects().getObjects().size() << "\n\n";

        std::cout << "Terrain sample, plane 0, y=32\n";
        for (int i = 30; i < 40; i++) {
            const Tile& t = region.terrain().getTile(0, i, 32);
            std::string name = "none";
            if (t.underlayId > 0) {
                try { name = loader.getFlo(t.underlayId - 1).name; } catch(...) {}
            }
            std::cout << "  tile x=" << i
                      << " y=32"
                      << " height=" << t.height
                      << " underlayId=" << static_cast<int>(t.underlayId)
                      << " floor=" << name
                      << " overlayId=" << static_cast<int>(t.overlayId)
                      << " settings=" << static_cast<int>(t.settings)
                      << "\n";
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
