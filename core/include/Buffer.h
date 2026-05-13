#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <span>  // For zero-copy slice

// Buffer owns a byte array and provides sequential read methods
// All multi-byte reads are big-endian (as used throughout the 317 cache)
// Buffer owns its data to prevent dangling pointers from the original source.
class Buffer {
public:
    // Default constructor — creates an empty buffer
    Buffer() : pos(0) {}

    // Construct from existing vector (moves the data to avoid copy)
    explicit Buffer(std::vector<uint8_t>&& data);
    
    // Construct from existing vector (copies the data)
    explicit Buffer(const std::vector<uint8_t>& data);
    
    // Construct from raw pointer + size (copies the data)
    Buffer(const uint8_t* data, std::size_t size);

    // No default copy/move needed, we'll define them explicitly if required
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&&) = default;
    Buffer& operator=(Buffer&&) = default;

    // --- Basic read methods (advance position) ---
    
    // Inline for speed - these are called frequently
    inline uint8_t readByte() {
        checkRemaining(1);
        return data_[pos++];
    }
    
    inline int8_t readSByte() {
        checkRemaining(1);
        return static_cast<int8_t>(data_[pos++]);
    }
    
    inline uint16_t readUShort() {
        checkRemaining(2);
        uint16_t value = (static_cast<uint16_t>(data_[pos]) << 8) | data_[pos + 1];
        pos += 2;
        return value;
    }
    
    int16_t readShort() { return static_cast<int16_t>(readUShort()); }
    
    inline uint32_t readTribyte() {
        checkRemaining(3);
        uint32_t value = (static_cast<uint32_t>(data_[pos])     << 16)
                       | (static_cast<uint32_t>(data_[pos + 1]) <<  8)
                       |  static_cast<uint32_t>(data_[pos + 2]);
        pos += 3;
        return value;
    }
    
    inline int32_t readInt() {
        checkRemaining(4);
        int32_t value = (static_cast<int32_t>(data_[pos])     << 24)
                      | (static_cast<int32_t>(data_[pos + 1]) << 16)
                      | (static_cast<int32_t>(data_[pos + 2]) <<  8)
                      |  static_cast<int32_t>(data_[pos + 3]);
        pos += 4;
        return value;
    }
    std::string readString();     // null-terminated string (terminator is 0x0A)
    uint32_t    readSmart();      // unsigned smart integer (1 or 2 bytes)
    int         readSignedSmart();// signed smart integer (model format, offset by 64/49152)

    // --- Peek methods (read WITHOUT advancing position) ---
    inline uint8_t peekByte() const {
        checkRemaining(1);
        return data_[pos];
    }
    
    inline int8_t peekSByte() const {
        checkRemaining(1);
        return static_cast<int8_t>(data_[pos]);
    }
    
    inline uint16_t peekUShort() const {
        checkRemaining(2);
        return (static_cast<uint16_t>(data_[pos]) << 8) | data_[pos + 1];
    }
    
    int16_t peekShort() const { return static_cast<int16_t>(peekUShort()); }
    
    inline uint32_t peekTribyte() const {
        checkRemaining(3);
        return (static_cast<uint32_t>(data_[pos])     << 16)
             | (static_cast<uint32_t>(data_[pos + 1]) <<  8)
             |  static_cast<uint32_t>(data_[pos + 2]);
    }
    
    inline int32_t peekInt() const {
        checkRemaining(4);
        return (static_cast<int32_t>(data_[pos])     << 24)
             | (static_cast<int32_t>(data_[pos + 1]) << 16)
             | (static_cast<int32_t>(data_[pos + 2]) <<  8)
             |  static_cast<int32_t>(data_[pos + 3]);
    }

    // --- Bulk read ---
    std::vector<uint8_t> readBytes(int n);  // Read n bytes into a new vector

    // --- Position management ---
    void skip(int n);                                           // advance position by n bytes
    void seek(int pos);                                          // jump to absolute position
    int  position() const { return pos; }                      // current read position
    int  remaining() const { return (int)data_.size() - pos; } // bytes left to read
    bool isEmpty()   const { return pos >= (int)data_.size(); } // nothing left to read
    bool empty()     const { return data_.empty(); }            // buffer has no data

    // --- Safety checks ---
    bool canRead(int n) const { return pos + n <= (int)data_.size(); }  // Check without throwing

    // --- Data access ---
    // Get pointer to raw data (valid for Buffer's lifetime)
    const uint8_t* data() const { return data_.data(); }
    std::size_t size() const { return data_.size(); }

    // Slice methods
    std::vector<uint8_t> slice(int start, int end) const;       // Copy bytes [start, end)
    std::span<const uint8_t> span(int start, int end) const;    // Zero-copy view [start, end)

private:
    std::vector<uint8_t> data_;  // Owns the data - no dangling pointers!
    int pos = 0;

    // Throws if we'd read past the end of the buffer
    void checkRemaining(int bytesNeeded) const;
};
