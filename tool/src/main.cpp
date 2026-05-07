#include <iostream>
#include "CacheReader.h"

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

    // scan archives 0 and 1 for the first 10 non-empty entries each
    for (int archive = 0; archive < 2; archive++) {
        std::cout << "\nScanning archive " << archive << " for non-empty entries..." << std::endl;
        int found = 0;
        for (int i = 0; i < 1000 && found < 10; i++) {
            IndexEntry entry = reader.readIndex(archive, i);
            if (entry.size > 0 && entry.firstSector > 0
                && entry.size != 0xFFFFFF && entry.firstSector != 0xFFFFFF) {
                std::cout << "  File " << i << " -> size: " << entry.size
                          << " firstSector: " << entry.firstSector << std::endl;
                found++;
            }
        }
    }

    return 0;
}
