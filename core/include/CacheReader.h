#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

#include "Archive.h"
#include "CacheDefinitions.h"

class CacheReader {
public:
    // opens the .dat and all .idx files from the given cache folder
    bool open(const std::filesystem::path& cachePath);

    // reads a single index entry from an idx file
    // archiveId: 0=defs+configs, 1=models, 2=animations, 3=midis, 4=maps
    // fileId = which entry inside the archive
    IndexEntry readIndex(int archiveId, int fileId);

    // reads a single 520-byte sector from the .dat file
    Sector readSector(uint32_t sectorNum);

    // assembles the full data of a file by following the sector chain
    // returns the raw bytes of the file
    std::vector<uint8_t> readFile(int archiveId, int fileId);

    // reads and parses a packed JAG sub-archive (archive 0 files)
    // returns an Archive with all sub-files decompressed and indexed by name hash
    Archive readArchive(int archiveId, int fileId);

private:
    std::ifstream              dat;     // handle for main_file_cache.dat
    std::array<std::ifstream, 5> idx;  // handles for idx0 through idx4
};
