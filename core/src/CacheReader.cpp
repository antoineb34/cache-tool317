#include "CacheReader.h"

#include <iostream>

// reads N bytes from a stream and assembles them into a uint32_t using big-endian order
static uint32_t readBigEndian(std::ifstream& stream, int numBytes) {
    uint32_t result = 0;
    for (int i = 0; i < numBytes; i++) {
        result = (result << 8) | static_cast<uint8_t>(stream.get());
    }
    return result;
}

bool CacheReader::open(const std::filesystem::path& cachePath) {
    dat.open(cachePath / "main_file_cache.dat", std::ios::binary);
    if (!dat.is_open()) {
        std::cerr << "Failed to open main_file_cache.dat" << std::endl;
        return false;
    }

    for (int i = 0; i < 5; i++) {
        std::string idxName = "main_file_cache.idx" + std::to_string(i);
        idx[i].open(cachePath / idxName, std::ios::binary);
        if (!idx[i].is_open()) {
            std::cerr << "Failed to open " << idxName << std::endl;
            return false;
        }
    }

    return true;
}

IndexEntry CacheReader::readIndex(int archiveId, int fileId) {
    idx[archiveId].seekg(fileId * 6);

    IndexEntry entry;
    entry.size        = readBigEndian(idx[archiveId], 3);
    entry.firstSector = readBigEndian(idx[archiveId], 3);
    return entry;
}

Sector CacheReader::readSector(uint32_t sectorNum) {
    dat.seekg(sectorNum * 520);

    Sector sector;
    sector.header.fileId     = readBigEndian(dat, 2);
    sector.header.chunk      = readBigEndian(dat, 2);
    sector.header.nextSector = readBigEndian(dat, 3);
    sector.header.archiveId  = readBigEndian(dat, 1);
    dat.read(reinterpret_cast<char*>(sector.data), 512);
    return sector;
}

std::vector<uint8_t> CacheReader::readFile(int archiveId, int fileId) {
    IndexEntry entry = readIndex(archiveId, fileId);

    if (entry.size == 0 || entry.firstSector == 0) return {};
    if (entry.size == 0xFFFFFF || entry.firstSector == 0xFFFFFF) return {};

    std::vector<uint8_t> buffer;
    buffer.reserve(entry.size);

    uint32_t sectorNum = entry.firstSector;

    while (buffer.size() < entry.size && sectorNum != 0) {
        Sector sector = readSector(sectorNum);

        uint32_t remaining  = entry.size - buffer.size();
        uint32_t bytesToCopy = remaining < 512 ? remaining : 512;

        buffer.insert(buffer.end(), sector.data, sector.data + bytesToCopy);
        sectorNum = sector.header.nextSector;
    }

    return buffer;
}
