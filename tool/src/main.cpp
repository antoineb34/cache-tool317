#include <iostream>
#include <string>

#include "CacheReader.h"
#include "DefinitionsLoader.h"
#include "VersionList.h"
#include "MapRegion.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: tool <path to cache folder> [region id]" << std::endl;
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

        MapRegion region = MapRegion::load(reader, vList, regionId);

        std::cout << "Region " << region.regionId() << " map decode\n";
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

        std::cout << "\nObject placement sample\n";
        for (int i = 0; i < 20; i++) {
            if (i >= (int)region.objects().getObjects().size()) break;
            const auto& obj = region.objects().getObjects()[i];
            std::cout << "  locId=" << obj.id
                      << " name=\"" << loader.getLoc(obj.id).name << "\""
                      << " plane=" << obj.z
                      << " x=" << obj.x
                      << " y=" << obj.y
                      << " type=" << obj.type
                      << " rotation=" << obj.rotation
                      << "\n";
        }

        std::cout << "\nNamed object sample\n";
        int namedCount = 0;
        for (const auto& obj : region.objects().getObjects()) {
            const LocDef& loc = loader.getLoc(obj.id);
            if (loc.name == "null")
                continue;

            std::cout << "  locId=" << obj.id
                      << " name=\"" << loc.name << "\""
                      << " plane=" << obj.z
                      << " x=" << obj.x
                      << " y=" << obj.y
                      << " type=" << obj.type
                      << " rotation=" << obj.rotation
                      << "\n";

            namedCount++;
            if (namedCount >= 30)
                break;
        }
        std::cout << "  named sample count=" << namedCount << "\n";

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
