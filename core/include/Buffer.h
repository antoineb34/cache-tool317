#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Buffer wraps a byte array and provides sequential read methods
// all multi-byte reads are big-endian (as used throughout the 317 cache)
class Buffer {
public:
    Buffer(const std::vector<uint8_t>& data);
    Buffer(const uint8_t* data, std::size_t size);

    uint8_t     readByte();       // 1 byte unsigned
    int8_t      readSByte();      // 1 byte signed
    uint16_t    readUShort();     // 2 bytes unsigned
    int16_t     readShort();      // 2 bytes signed
    uint32_t    readTribyte();    // 3 bytes unsigned
    int32_t     readInt();        // 4 bytes signed
    std::string readString();     // null-terminated string
    uint32_t    readSmart();

    void skip(int n);                                           // advance position by n bytes
    std::vector<uint8_t> slice(int start, int end) const;       // copy bytes from [start, end)
    int  position() const { return pos; }                      // current read position
    int  remaining() const { return (int)size - pos; }         // bytes left to read
    bool isEmpty()   const { return pos >= (int)size; }        // nothing left to read

private:
    const uint8_t* data;
    std::size_t    size;
    int            pos;
};
