#include "Buffer.h"
#include <stdexcept>

Buffer::Buffer(std::vector<uint8_t>&& data)
    : data_(std::move(data)), pos(0) {}

Buffer::Buffer(const std::vector<uint8_t>& data)
    : data_(data), pos(0) {}

Buffer::Buffer(const uint8_t* data, std::size_t size)
    : data_(data, data + size), pos(0) {}

void Buffer::checkRemaining(int bytesNeeded) const {
    if (pos + bytesNeeded > (int)data_.size()) {
        throw std::runtime_error("Buffer: read past end of buffer (pos=" + std::to_string(pos) +
                               ", need=" + std::to_string(bytesNeeded) +
                               ", size=" + std::to_string(data_.size()) + ")");
    }
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
    checkRemaining(1);
    uint8_t value = data_[pos];
    if (value < 128) return readByte();
    return readUShort() - 32768;
}

int Buffer::readSignedSmart() {
    checkRemaining(1);
    uint8_t value = data_[pos];
    if (value < 128) return static_cast<int>(readByte()) - 64;
    return static_cast<int>(readUShort()) - 49152;
}

std::vector<uint8_t> Buffer::readBytes(int n) {
    checkRemaining(n);
    std::vector<uint8_t> result;
    result.reserve(n);
    result.assign(data_.begin() + pos, data_.begin() + pos + n);
    pos += n;
    return result;
}

// --- Peek methods (read WITHOUT advancing position) ---


// --- Position management ---

void Buffer::skip(int n) {
    checkRemaining(n);
    pos += n;
}

void Buffer::seek(int newPos) {
    if (newPos < 0 || newPos > (int)data_.size()) {
        throw std::runtime_error("Buffer: seek position out of range (pos=" + std::to_string(newPos) +
                               ", size=" + std::to_string(data_.size()) + ")");
    }
    pos = newPos;
}

// --- Data access ---

std::vector<uint8_t> Buffer::slice(int start, int end) const {
    if (start < 0 || end > (int)data_.size() || start > end) {
        throw std::runtime_error("Buffer: slice range invalid (start=" + std::to_string(start) +
                               ", end=" + std::to_string(end) +
                               ", size=" + std::to_string(data_.size()) + ")");
    }
    return std::vector<uint8_t>(data_.begin() + start, data_.begin() + end);
}

std::span<const uint8_t> Buffer::span(int start, int end) const {
    if (start < 0 || end > (int)data_.size() || start > end) {
        throw std::runtime_error("Buffer: span range invalid (start=" + std::to_string(start) +
                               ", end=" + std::to_string(end) +
                               ", size=" + std::to_string(data_.size()) + ")");
    }
    return std::span<const uint8_t>(data_.data() + start, end - start);
}
