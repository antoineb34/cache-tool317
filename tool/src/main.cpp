#include <iostream>
#include "CacheReader.h"
#include "DefinitionsLoader.h"

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

        std::cout << "Definitions loaded successfully." << std::endl;
        std::cout << "Items: " << loader.itemCount() << std::endl;
        std::cout << "NPCs:  " << loader.npcCount() << std::endl;
        std::cout << "Locs:  " << loader.locCount() << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Definition loading failed: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
