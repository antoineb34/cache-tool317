#include <iostream>
#include <string>

#include "CacheReader.h"
#include "DefinitionsLoader.h"
#include "VersionList.h"
#include "MapRegion.h"
#include "RegionRenderer2D.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: tool <path to cache folder> [region id] [object filter]\n"
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
