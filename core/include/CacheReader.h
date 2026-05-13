#pragma once

#include <filesystem>
#include <fstream>
#include <vector>
#include <map>
#include <memory>
#include <utility>
#include <cstdint>

#ifdef __linux__
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "Archive.h"
#include "Buffer.h"

struct IndexEntry {
    uint32_t size;        // total size of the file in bytes (3-byte value in cache)
    uint32_t firstSector; // first sector in the .dat file where this file's data begins (3-byte value in cache)
};

// CacheReader reads the RS317 cache format and provides clean access to data.
// It hides the ugly details (sectors, indices, BZIP2) and returns:
//   - Buffer (owns data) for raw files
//   - Archive (contains Buffers) for JAG sub-archives
//
// Usage:
//   CacheReader reader;
//   reader.open("cache/");
//   Buffer modelBuf = reader.readFile(1, 12345);       // model from archive 1
//   Archive texArchive = reader.readArchive(0, 6);     // textures from archive 0 file 6
class CacheReader {
public:
    // Opens the cache: main_file_cache.dat + idx0 through idx4
    bool open(const std::filesystem::path& cachePath);
    
    // Destructor: clean up mmap resources
    ~CacheReader() {
        // Clean up .dat mmap
        if (useMmap && datMmap != nullptr && datMmap != MAP_FAILED) {
            munmap(datMmap, datSize);
            datMmap = nullptr;
        }
        if (datFd >= 0) {
            close(datFd);
            datFd = -1;
        }
        
        // Clean up .idx mmaps
        for (int i = 0; i < 5; i++) {
            if (idxMmap[i].data != nullptr && idxMmap[i].data != MAP_FAILED) {
                munmap(idxMmap[i].data, idxMmap[i].size);
                idxMmap[i].data = nullptr;
            }
            if (idxMmap[i].fd >= 0) {
                close(idxMmap[i].fd);
                idxMmap[i].fd = -1;
            }
        }
    }

    // Reads a raw file from the cache as a Buffer (owns its data).
    // archiveId: 0=definitions, 1=models, 2=animations, 3=midis, 4=maps
    // Returns empty Buffer if file doesn't exist.
    Buffer readFile(int archiveId, int fileId);

    // Reads a file and decompresses it if gzipped (used for models, map data).
    // Returns decompressed data as Buffer.
    Buffer readGzippedFile(int archiveId, int fileId);

    // Reads a file and parses it as a JAG sub-archive.
    // Returns a shared_ptr to an Archive with sub-files stored as Buffer, indexed by name hash.
    // Returns nullptr if parsing fails.
    std::shared_ptr<Archive> readArchive(int archiveId, int fileId);

    // Queries without reading file data
    int  getFileCount(int archiveId) const;
    bool hasFile(int archiveId, int fileId) const;

private:
    IndexEntry readIndex(int archiveId, int fileId) const;

    // Memory-mapped .dat file (instead of std::ifstream)
    int               datFd = -1;
    uint8_t*          datMmap = nullptr;
    size_t            datSize = 0;
    
    // Memory-mapped .idx files (instead of std::ifstream)
    struct MmapIdx {
        int      fd = -1;
        uint8_t* data = nullptr;
        size_t   size = 0;
    };
    mutable std::array<MmapIdx, 5> idxMmap;
    bool              useIdxMmap = false;
    
    // Fallback std::ifstream if mmap fails
    std::ifstream     datFallback;
    bool              useMmap = false;
    
    mutable std::array<std::ifstream, 5> idx;
    
    // Cache for parsed archives to avoid repeated decompression
    std::map<std::pair<int, int>, std::shared_ptr<Archive>> archiveCache;
};