#include "Buffer.h"

Buffer::Buffer(const std::vector<uint8_t>& data)
    : data(data.data()), size(data.size()), pos(0) {}

Buffer::Buffer(const uint8_t* data, std::size_t size)
    : data(data), size(size), pos(0) {}

uint8_t Buffer::readByte() {
    return data[pos++];
}

int8_t Buffer::readSByte() {
    return static_cast<int8_t>(data[pos++]);
}

uint16_t Buffer::readUShort() {
    uint16_t value = (static_cast<uint16_t>(data[pos]) << 8) | data[pos + 1];
    pos += 2;
    return value;
}

int16_t Buffer::readShort() {
    return static_cast<int16_t>(readUShort());
}

uint32_t Buffer::readTribyte() {
    uint32_t value = (static_cast<uint32_t>(data[pos])     << 16)
                   | (static_cast<uint32_t>(data[pos + 1]) <<  8)
                   |  static_cast<uint32_t>(data[pos + 2]);
    pos += 3;
    return value;
}

int32_t Buffer::readInt() {
    int32_t value = (static_cast<int32_t>(data[pos])     << 24)
                  | (static_cast<int32_t>(data[pos + 1]) << 16)
                  | (static_cast<int32_t>(data[pos + 2]) <<  8)
                  |  static_cast<int32_t>(data[pos + 3]);
    pos += 4;
    return value;
}

std::string Buffer::readString() {
    std::string result;
    while (pos < (int)size && data[pos] != 0) {
        result += static_cast<char>(data[pos++]);
    }
    pos++; // skip null terminator
    return result;
}

void Buffer::skip(int n) {
    pos += n;
}
