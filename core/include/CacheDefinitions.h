#pragma once

#include <cstdint>

struct IndexEntry {
    uint32_t size;        // total size of the file in bytes (3-byte value in cache)
    uint32_t firstSector; // first sector in the .dat file where this file's data begins (3-byte value in cache)
};

struct ArchiveHeader {
    uint32_t decompressedSize; // total size of the archive when decompressed (3-byte value)
    uint32_t compressedSize;   // size of the archive as stored (3-byte value)
                               // if equal to decompressedSize, data is not compressed
};

struct ArchiveEntry {
    uint32_t nameHash;         // hash of the sub-file name (4-byte value)
    uint32_t decompressedSize; // size of the sub-file when decompressed (3-byte value)
    uint32_t compressedSize;   // size of the sub-file as stored (3-byte value)
                               // if equal to decompressedSize, sub-file is not compressed
};

struct Sector {
    struct Header {
        uint16_t fileId;     // which file this sector belongs to
        uint16_t chunk;      // which chunk of the file this is (files can span multiple sectors)
        uint32_t nextSector; // next sector in the chain (3-byte value in cache)
        uint8_t  archiveId;  // which archive (idx file) this sector belongs to
    };

    Header   header;
    uint8_t  data[512];
};
