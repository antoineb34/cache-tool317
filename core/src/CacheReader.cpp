#include "CacheReader.h"
#include "Buffer.h"

#include <zlib.h>
#include <array>
#include <bzlib.h>
#include <iostream>

// decompresses a Jagex BZIP2 block
// Jagex strips the standard BZh1 header so we prepend it before decompressing
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
// decompresses a GZIP block using zlib
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
    std::array<uint8_t, 6> bytes;
    idx[archiveId].seekg(fileId * 6);
    idx[archiveId].read(reinterpret_cast<char*>(bytes.data()), 6);

    Buffer buf(bytes.data(), 6);
    IndexEntry entry;
    entry.size        = buf.readTribyte();
    entry.firstSector = buf.readTribyte();
    return entry;
}

Sector CacheReader::readSector(uint32_t sectorNum) {
    std::array<uint8_t, 8> headerBytes;
    dat.seekg(sectorNum * 520);
    dat.read(reinterpret_cast<char*>(headerBytes.data()), 8);

    Buffer headerBuf(headerBytes.data(), 8);
    Sector sector;
    sector.header.fileId     = headerBuf.readUShort();
    sector.header.chunk      = headerBuf.readUShort();
    sector.header.nextSector = headerBuf.readTribyte();
    sector.header.archiveId  = headerBuf.readByte();
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

Archive CacheReader::readArchive(int archiveId, int fileId) {
    std::vector<uint8_t> raw = readFile(archiveId, fileId);
    if (raw.size() < 6) return {};

    // parse the outer 6-byte header: decompressed size + compressed size
    Buffer rawBuf(raw);
    uint32_t decompressedSize = rawBuf.readTribyte();
    uint32_t compressedSize   = rawBuf.readTribyte();

    // get the data block, decompressing the whole archive if needed
    std::vector<uint8_t> data;
    if (decompressedSize != compressedSize) {
        // whole-archive compressed — sub-files inside will NOT be individually compressed
        data = decompressBzip2(raw.data() + 6, compressedSize, decompressedSize);
    } else {
        // not whole-archive compressed — sub-files may be individually compressed
        data.assign(raw.begin() + 6, raw.end());
    }

    if (data.size() < 2) return {};

    bool wholeCompressed = (decompressedSize != compressedSize);

    // parse the entry table
    Buffer dataBuf(data);
    uint16_t numEntries = dataBuf.readUShort();

    // read all entries first, data follows after the full table
    std::vector<ArchiveEntry> entries(numEntries);
    for (auto& entry : entries) {
        entry.nameHash         = dataBuf.readInt();
        entry.decompressedSize = dataBuf.readTribyte();
        entry.compressedSize   = dataBuf.readTribyte();
    }

    // now read each sub-file's data
    Archive archive;
    for (const auto& entry : entries) {
        std::vector<uint8_t> fileData;
        if (!wholeCompressed && entry.decompressedSize != entry.compressedSize) {
            // sub-file is individually compressed
            fileData = decompressBzip2(data.data() + dataBuf.position(), entry.compressedSize, entry.decompressedSize);
        } else {
            // sub-file is raw
            fileData.assign(data.begin() + dataBuf.position(),
                            data.begin() + dataBuf.position() + entry.decompressedSize);
        }
        dataBuf.skip(entry.compressedSize);
        archive.files[entry.nameHash] = std::move(fileData);
    }

    return archive;
}

std::vector<uint8_t> CacheReader::readGzippedFile(int archiveId, int fileId) {
    std::vector<uint8_t> raw = readFile(archiveId, fileId);
    if (raw.empty()) return {};
    return decompressGzip(raw.data(), static_cast<uint32_t>(raw.size()));
}

int CacheReader::getFileCount(int archiveId) const {
    if (archiveId < 0 || archiveId >= 5) return 0;
    idx[archiveId].seekg(0, std::ios::end);
    auto size = idx[archiveId].tellg();
    return static_cast<int>(size / 6);
}

bool CacheReader::hasFile(int archiveId, int fileId) const {
    if (archiveId < 0 || archiveId >= 5) return false;
    idx[archiveId].seekg(fileId * 6);
    std::array<uint8_t, 6> bytes;
    idx[archiveId].read(reinterpret_cast<char*>(bytes.data()), 6);
    if (!idx[archiveId].good()) return false;
    Buffer buf(bytes.data(), 6);
    uint32_t size        = buf.readTribyte();
    uint32_t firstSector = buf.readTribyte();
    return size != 0 && firstSector != 0 && size != 0xFFFFFF && firstSector != 0xFFFFFF;
}
