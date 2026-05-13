#include "Buffer.h"
#include <stdexcept>

Buffer::Buffer(std::vector<uint8_t>&& data)
    : data_(std::move(data)), pos(0) {}

Buffer::Buffer(const std::vector<uint8_t>& data)
    : data_(data), pos(0) {}

Buffer::Buffer(const uint8_t* data, std::size_t size)
    : data_(data, data + size), pos(0) {}

void Buffer::checkRemaining(int bytesNeeded) const {
#ifdef DEBUG_BUFFER
    if (pos + bytesNeeded > (int)data_.size()) {
        throw std::runtime_error("Buffer: read past end of buffer (pos=" + std::to_string(pos) +
                               ", need=" + std::to_string(bytesNeeded) +
                               ", size=" + std::to_string(data_.size()) + ")");
    }
#endif
}

// --- Basic read methods (advance position) ---

std::string Buffer::readString() {
    // 317 cache format uses byte value 10 (0x0a) as the string terminator
    // First, find the length to avoid reallocations
    int start = pos;
    while (pos < (int)data_.size() && data_[pos] != 10) {
        pos++;
    }
    int len = pos - start;
    
    std::string result(reinterpret_cast<const char*>(&data_[start]), len);
    
    if (pos < (int)data_.size()) pos++; // skip the terminator byte
    return result;
}

uint32_t Buffer::readSmart() {
#ifdef DEBUG_BUFFER
    checkRemaining(1);
#endif
    uint8_t value = data_[pos];
    if (value < 128) return readByte();
    return readUShort() - 32768;
}

int Buffer::readSignedSmart() {
#ifdef DEBUG_BUFFER
    checkRemaining(1);
#endif
    uint8_t value = data_[pos];
    if (value < 128) return static_cast<int>(readByte()) - 64;
    return static_cast<int>(readUShort()) - 49152;
}

std::vector<uint8_t> Buffer::readBytes(int n) {
#ifdef DEBUG_BUFFER
    checkRemaining(n);
#endif
    std::vector<uint8_t> result;
    result.reserve(n);
    result.assign(data_.begin() + pos, data_.begin() + pos + n);
    pos += n;
    return result;
}

// --- Peek methods (read WITHOUT advancing position) ---


// --- Position management ---

void Buffer::skip(int n) {
#ifdef DEBUG_BUFFER
    checkRemaining(n);
#endif
    pos += n;
}

void Buffer::seek(int newPos) {
    if (newPos < 0) newPos = 0;
    if (newPos > (int)data_.size()) newPos = data_.size();
    pos = newPos;
}

// --- Data access ---

std::vector<uint8_t> Buffer::slice(int start, int end) const {
    if (start < 0) start = 0;
    if (end > (int)data_.size()) end = data_.size();
    if (start > end) start = end;
    return std::vector<uint8_t>(data_.begin() + start, data_.begin() + end);
}

std::span<const uint8_t> Buffer::span(int start, int end) const {
    if (start < 0) start = 0;
    if (end > (int)data_.size()) end = data_.size();
    if (start > end) start = end;
    return std::span<const uint8_t>(data_.data() + start, end - start);
}
