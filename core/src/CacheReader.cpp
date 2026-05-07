#include "CacheReader.h"
#include "Buffer.h"

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

Archive CacheReader::readArchive(int archiveId, int fileId) {
    std::vector<uint8_t> raw = readFile(archiveId, fileId);
    if (raw.size() < 6) return {};

    // parse the outer 6-byte archive header
    Buffer rawBuf(raw);
    ArchiveHeader header;
    header.decompressedSize = rawBuf.readTribyte();
    header.compressedSize   = rawBuf.readTribyte();

    // get the data block, decompressing the whole archive if needed
    std::vector<uint8_t> data;
    if (header.decompressedSize != header.compressedSize) {
        // whole-archive compressed — sub-files inside will NOT be individually compressed
        data = decompressBzip2(raw.data() + 6, header.compressedSize, header.decompressedSize);
    } else {
        // not whole-archive compressed — sub-files may be individually compressed
        data.assign(raw.begin() + 6, raw.end());
    }

    if (data.size() < 2) return {};

    bool wholeCompressed = (header.decompressedSize != header.compressedSize);

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
