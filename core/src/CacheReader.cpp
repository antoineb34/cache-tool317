#include "CacheReader.h"
#include "Buffer.h"

#include <zlib.h>
#include <bzlib.h>
#include <array>
#include <iostream>
#include <cstring>
#include <sys/stat.h>

// Decompresses a Jagex BZIP2 block.
// Jagex strips the standard BZh header, so we prepend it before decompressing.
static std::vector<uint8_t> decompressBzip2(const uint8_t* src, uint32_t srcLen, uint32_t destLen) {
    std::vector<uint8_t> input(srcLen + 4);
    input[0] = 'B'; input[1] = 'Z'; input[2] = 'h'; input[3] = '1';
    std::copy(src, src + srcLen, input.begin() + 4);

    std::vector<uint8_t> output(destLen);
    unsigned int outLen = destLen;

    int result = BZ2_bzBuffToBuffDecompress(
        reinterpret_cast<char*>(output.data()), &outLen,
        reinterpret_cast<char*>(input.data()), static_cast<unsigned int>(input.size()),
        0, 0
    );

    if (result != BZ_OK) {
        std::cerr << "BZip2 decompression failed: " << result << std::endl;
        return {};
    }
    return output;
}

// Decompresses a GZIP block using zlib
static std::vector<uint8_t> decompressGzip(const uint8_t* src, uint32_t srcLen) {
    z_stream strm = {};
    strm.next_in = const_cast<uint8_t*>(src);
    strm.avail_in = srcLen;

    // windowBits 15 + 16 enables gzip header detection
    if (inflateInit2(&strm, 15 + 16) != Z_OK) return {};

    std::vector<uint8_t> out;
    uint8_t buf[4096];
    int ret;
    do {
        strm.next_out = buf;
        strm.avail_out = sizeof(buf);
        ret = inflate(&strm, Z_NO_FLUSH);
        
        size_t have = sizeof(buf) - strm.avail_out;
        if (have > 0) {
            out.insert(out.end(), buf, buf + have);
        }
    } while (ret == Z_OK);

    inflateEnd(&strm);
    return (ret == Z_STREAM_END) ? out : std::vector<uint8_t>{};
}

bool CacheReader::open(const std::filesystem::path& cachePath) {
    auto datPath = cachePath / "main_file_cache.dat";
    
    // Try to memory-map the .dat file
    useMmap = false;
    datFd = -1;
    datMmap = nullptr;
    datSize = 0;
    
#ifdef __linux__
    datFd = ::open(datPath.c_str(), O_RDONLY);
    if (datFd >= 0) {
        // Get file size
        struct stat st;
        if (fstat(datFd, &st) == 0) {
            datSize = st.st_size;
            if (datSize > 0) {
                datMmap = static_cast<uint8_t*>(mmap(nullptr, datSize, PROT_READ, MAP_PRIVATE, datFd, 0));
                if (datMmap != MAP_FAILED) {
                    useMmap = true;
                } else {
                    close(datFd);
                    datFd = -1;
                }
            }
        }
    }
#endif
    
    // Fallback to std::ifstream if mmap failed
    if (!useMmap) {
        datFallback.open(datPath, std::ios::binary);
        if (!datFallback.is_open()) {
            std::cerr << "Failed to open main_file_cache.dat: " << datPath << std::endl;
            return false;
        }
    }

    // Try to memory-map the index files
    useIdxMmap = false;
    for (int i = 0; i < 5; i++) {
        std::string idxName = "main_file_cache.idx" + std::to_string(i);
        auto idxPath = cachePath / idxName;
        
        int fd = ::open(idxPath.c_str(), O_RDONLY);
        if (fd >= 0) {
            struct stat st;
            if (fstat(fd, &st) == 0 && st.st_size > 0) {
                uint8_t* data = static_cast<uint8_t*>(mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
                if (data != MAP_FAILED) {
                    idxMmap[i].fd = fd;
                    idxMmap[i].data = data;
                    idxMmap[i].size = st.st_size;
                    useIdxMmap = true;
                    continue;  // Success, move to next index
                }
            }
            close(fd);
        }
        
        // Fallback: use std::ifstream
        idx[i].open(idxPath, std::ios::binary);
        if (!idx[i].is_open()) {
            std::cerr << "Failed to open " << idxName << std::endl;
            return false;
        }
    }

    return true;
}

IndexEntry CacheReader::readIndex(int archiveId, int fileId) const {
    if (useIdxMmap && idxMmap[archiveId].data != nullptr) {
        // Memory-mapped: read directly from mmap'd buffer
        size_t offset = fileId * 6;
        if (offset + 6 > idxMmap[archiveId].size) {
            return {};  // Out of bounds
        }
        const uint8_t* bytes = idxMmap[archiveId].data + offset;
        Buffer buf(bytes, 6);
        IndexEntry entry;
        entry.size        = buf.readTribyte();
        entry.firstSector = buf.readTribyte();
        return entry;
    } else {
        // Fallback: use std::ifstream
        std::array<uint8_t, 6> bytes;
        idx[archiveId].seekg(fileId * 6);
        idx[archiveId].read(reinterpret_cast<char*>(bytes.data()), 6);

        Buffer buf(bytes.data(), 6);
        IndexEntry entry;
        entry.size        = buf.readTribyte();
        entry.firstSector = buf.readTribyte();
        return entry;
    }
}

Buffer CacheReader::readFile(int archiveId, int fileId) {
    IndexEntry entry = readIndex(archiveId, fileId);

    if (entry.size == 0 || entry.firstSector == 0) return Buffer(std::vector<uint8_t>{});
    if (entry.size == 0xFFFFFF || entry.firstSector == 0xFFFFFF) return Buffer(std::vector<uint8_t>{});

    // Calculate total sectors needed
    uint32_t numSectors = (entry.size + 511) / 512;  // round up
    uint32_t totalBytes = numSectors * 520;  // 8 header + 512 data per sector
    
    // Read all sectors in one operation
    std::vector<uint8_t> rawSectors(totalBytes);
    
    if (useMmap && datMmap != nullptr) {
        // Memory-mapped: copy directly from mmap'd buffer
        if (entry.firstSector * 520 + totalBytes > datSize) {
            return Buffer(std::vector<uint8_t>{});  // Out of bounds
        }
        std::memcpy(rawSectors.data(), datMmap + (entry.firstSector * 520), totalBytes);
    } else {
        // Fallback: use std::ifstream
        datFallback.seekg(entry.firstSector * 520);
        datFallback.read(reinterpret_cast<char*>(rawSectors.data()), totalBytes);
        
        if (!datFallback) {
            return Buffer(std::vector<uint8_t>{});
        }
    }
    
    // Extract data, skipping 8-byte header per sector
    std::vector<uint8_t> buffer;
    buffer.reserve(entry.size);
    buffer.resize(entry.size);
    
    uint8_t* dst = buffer.data();
    uint32_t bytesCopied = 0;
    
    for (uint32_t i = 0; i < numSectors && bytesCopied < entry.size; i++) {
        uint8_t* sectorData = rawSectors.data() + (i * 520) + 8;  // skip header
        uint32_t toCopy = std::min(512u, entry.size - bytesCopied);
        std::memcpy(dst + bytesCopied, sectorData, toCopy);
        bytesCopied += toCopy;
    }
    
    buffer.resize(bytesCopied);
    return Buffer(std::move(buffer));
}

Buffer CacheReader::readGzippedFile(int archiveId, int fileId) {
    Buffer rawBuf = readFile(archiveId, fileId);
    if (rawBuf.empty()) return Buffer(std::vector<uint8_t>{});
    
    std::vector<uint8_t> decompressed = decompressGzip(rawBuf.data(), static_cast<uint32_t>(rawBuf.size()));
    if (decompressed.empty()) return Buffer(std::vector<uint8_t>{});
    
    return Buffer(std::move(decompressed));
}

std::shared_ptr<Archive> CacheReader::readArchive(int archiveId, int fileId) {
    // Check cache first
    std::pair<int, int> key = {archiveId, fileId};
    auto it = archiveCache.find(key);
    if (it != archiveCache.end()) {
        return it->second;  // Return cached archive
    }
    
    Buffer rawBuf = readFile(archiveId, fileId);
    if (rawBuf.size() < 6) return nullptr;

    // Parse the outer 6-byte header: decompressed size + compressed size
    uint32_t decompressedSize = rawBuf.readTribyte();
    uint32_t compressedSize   = rawBuf.readTribyte();

    // Get the data block, decompressing the whole archive if needed
    std::vector<uint8_t> data;
    if (decompressedSize != compressedSize) {
        // Whole-archive compressed — sub-files inside are NOT individually compressed
        data = decompressBzip2(rawBuf.data() + 6, compressedSize, decompressedSize);
    } else {
        // Not whole-archive compressed — sub-files may be individually compressed
        data.assign(rawBuf.data() + 6, rawBuf.data() + rawBuf.size());
    }

    if (data.size() < 2) return {};

    bool wholeCompressed = (decompressedSize != compressedSize);

    // Parse the entry table
    Buffer dataBuf(data);
    uint16_t numEntries = dataBuf.readUShort();

    // Read all entries first, data follows after the full table
    std::vector<ArchiveEntry> entries(numEntries);
    for (auto& entry : entries) {
        entry.nameHash         = dataBuf.readInt();
        entry.decompressedSize = dataBuf.readTribyte();
        entry.compressedSize   = dataBuf.readTribyte();
    }

    // Now read each sub-file's data
    Archive archive;
    for (const auto& entry : entries) {
        std::vector<uint8_t> fileData;
        if (!wholeCompressed && entry.decompressedSize != entry.compressedSize) {
            // Sub-file is individually compressed
            fileData = decompressBzip2(data.data() + dataBuf.position(), entry.compressedSize, entry.decompressedSize);
        } else {
            // Sub-file is raw
            fileData.assign(data.begin() + dataBuf.position(),
                            data.begin() + dataBuf.position() + entry.compressedSize);
        }
        dataBuf.skip(entry.compressedSize);
        archive.files[entry.nameHash] = Buffer(std::move(fileData));
    }

    // Store in cache before returning
    auto archivePtr = std::make_shared<Archive>(std::move(archive));
    archiveCache[key] = archivePtr;
    
    return archivePtr;
}

int CacheReader::getFileCount(int archiveId) const {
    if (archiveId < 0 || archiveId >= 5) return 0;
    
    if (useIdxMmap && idxMmap[archiveId].data != nullptr) {
        // Memory-mapped: calculate from mmap'd size
        return static_cast<int>(idxMmap[archiveId].size / 6);
    } else {
        // Fallback: use std::ifstream
        idx[archiveId].seekg(0, std::ios::end);
        auto size = idx[archiveId].tellg();
        return static_cast<int>(size / 6);
    }
}

bool CacheReader::hasFile(int archiveId, int fileId) const {
    if (archiveId < 0 || archiveId >= 5) return false;
    
    if (useIdxMmap && idxMmap[archiveId].data != nullptr) {
        // Memory-mapped: read directly from mmap'd buffer
        size_t offset = fileId * 6;
        if (offset + 6 > idxMmap[archiveId].size) {
            return false;  // Out of bounds
        }
        const uint8_t* bytes = idxMmap[archiveId].data + offset;
        Buffer buf(bytes, 6);
        uint32_t size        = buf.readTribyte();
        uint32_t firstSector = buf.readTribyte();
        return size != 0 && firstSector != 0 && size != 0xFFFFFF && firstSector != 0xFFFFFF;
    } else {
        // Fallback: use std::ifstream
        idx[archiveId].seekg(fileId * 6);
        std::array<uint8_t, 6> bytes;
        idx[archiveId].read(reinterpret_cast<char*>(bytes.data()), 6);
        if (!idx[archiveId].good()) return false;
        Buffer buf(bytes.data(), 6);
        uint32_t size        = buf.readTribyte();
        uint32_t firstSector = buf.readTribyte();
        return size != 0 && firstSector != 0 && size != 0xFFFFFF && firstSector != 0xFFFFFF;
    }
}
