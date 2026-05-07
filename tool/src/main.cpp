#include <iostream>
#include "CacheReader.h"
#include "DefinitionsLoader.h"
#include "VersionList.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: tool <path to cache folder>" << std::endl;
        return 1;
    }

    CacheReader reader;
    if (!reader.open(argv[1])) {
        std::cerr << "Failed to open cache at: " << argv[1] << std::endl;
        return 1;
    }

    std::cout << "Cache opened successfully." << std::endl;

    try {
        Archive defs = reader.readArchive(0, 2);
        if (defs.size() == 0) {
            std::cerr << "Failed to read definitions archive." << std::endl;
            return 1;
        }

        DefinitionsLoader loader;
        loader.loadItems(defs);
        loader.loadNpcs(defs);
        loader.loadLocs(defs);
        loader.loadFlos(defs);
        loader.loadIdks(defs);
        loader.loadMesAnims(defs);
        loader.loadMes(defs);
        loader.loadParams(defs);
        loader.loadSeqs(defs);
        loader.loadSpotAnims(defs);
        loader.loadVarbits(defs);
        loader.loadVarps(defs);

        std::cout << "Definitions loaded successfully." << std::endl;
        std::cout << "Items: " << loader.itemCount() << std::endl;
        std::cout << "NPCs:  " << loader.npcCount() << std::endl;
        std::cout << "Locs:  " << loader.locCount() << std::endl;
        std::cout << "Flos:  " << loader.floCount() << std::endl;
        std::cout << "Idks:  " << loader.idkCount() << std::endl;
        std::cout << "MesAnims: " << loader.mesAnimCount() << std::endl;
        std::cout << "Mes:   " << loader.mesCount() << std::endl;
        std::cout << "Params: " << loader.paramCount() << std::endl;
        std::cout << "Seqs:  " << loader.seqCount() << std::endl;
        std::cout << "SpotAnims: " << loader.spotAnimCount() << std::endl;
        std::cout << "Varbits: " << loader.varbitCount() << std::endl;
        std::cout << "Varps: " << loader.varpCount() << std::endl;

        Archive versionlistArchive = reader.readArchive(0, 5);
        if (versionlistArchive.size() == 0) {
            std::cerr << "Failed to read versionlist archive." << std::endl;
            return 1;
        }

        VersionList versionList = VersionList::parse(versionlistArchive);
        std::cout << "Versionlist loaded successfully." << std::endl;
        std::cout << "Map regions: " << versionList.mapIndex().size() << std::endl;
        if (!versionList.mapIndex().empty()) {
            const auto& first = versionList.mapIndex().front();
            std::cout << "First map region: regionId=" << first.regionId
                      << " terrainFileId=" << first.terrainFileId
                      << " objectFileId=" << first.objectFileId
                      << " flag=" << first.flag << std::endl;
        }
    } catch (const std::exception& ex) {
        std::cerr << "Cache verification failed: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
