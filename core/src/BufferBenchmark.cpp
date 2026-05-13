#include "Buffer.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <cstring>

// Benchmark helper
template<typename F>
double benchmark(const char* name, F func, int iterations = 100) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        func();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> duration = end - start;
    double usPerOp = duration.count() / iterations;
    std::cout << name << ": " << usPerOp << " µs/op (" << iterations << " iterations)\n";
    return usPerOp;
}

// Test 1: Basic read operations
void testBasicReads(const Buffer& data) {
    std::cout << "--- Test 1: Basic Read Operations ---\n";
    
    benchmark("readByte()", [&]() {
        Buffer buf(data.slice(0, data.size()));
        volatile uint8_t sum = 0;  // volatile to prevent optimization
        for (int i = 0; i < 1000; i++) {
            sum += buf.readByte();
        }
    }, 1000);
    
    benchmark("readUShort()", [&]() {
        Buffer buf(data.slice(0, data.size()));
        volatile uint16_t sum = 0;
        for (int i = 0; i < 500; i++) {
            sum += buf.readUShort();
        }
    }, 1000);
    
    benchmark("readInt()", [&]() {
        Buffer buf(data.slice(0, data.size()));
        volatile int32_t sum = 0;
        for (int i = 0; i < 250; i++) {
            sum += buf.readInt();
        }
    }, 1000);
}

// Test 2: readString() optimization
void testReadString(const Buffer& data) {
    std::cout << "\n--- Test 2: readString() ---\n";
    
    // Create a buffer with a string for testing
    std::vector<uint8_t> strData = {
        'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', 10,  // "Hello World\n"
        'T', 'e', 's', 't', 10,  // "Test\n"
        0  // empty string
    };
    Buffer strBuf(strData);
    
    benchmark("readString() (optimized)", [&]() {
        Buffer buf(strData);
        std::string s = buf.readString();
    }, 10000);
}

// Test 3: span() vs slice()
void testSpanVsSlice(const Buffer& data) {
    std::cout << "\n--- Test 3: span() vs slice() ---\n";
    
    benchmark("slice(100, 500) - copy", [&]() {
        std::vector<uint8_t> v = data.slice(100, 500);
    }, 10000);
    
    benchmark("span(100, 500) - zero-copy", [&]() {
        std::span<const uint8_t> s = data.span(100, 500);
    }, 10000);
}

// Test 4: peek vs read
void testPeekVsRead(const Buffer& data) {
    std::cout << "\n--- Test 4: peek() vs read() ---\n";
    
    if (data.size() > 100) {
        benchmark("readByte() - advance", [&]() {
            Buffer buf(data.slice(0, data.size()));
            volatile uint8_t x;
            for (int i = 0; i < 100; i++) {
                x = buf.readByte();
            }
        }, 1000);
        
        benchmark("peekByte() - no advance", [&]() {
            Buffer buf(data.slice(0, data.size()));
            volatile uint8_t x;
            for (int i = 0; i < 100; i++) {
                x = buf.peekByte();  // Always reads position 0
            }
        }, 1000);
    }
}

// Test 5: Smart integer reads
void testSmartReads(const Buffer& data) {
    std::cout << "\n--- Test 5: Smart Integer Reads ---\n";
    
    if (data.size() > 200) {
        benchmark("readSmart()", [&]() {
            Buffer buf(data.slice(0, data.size()));
            volatile uint32_t sum = 0;
            for (int i = 0; i < 100; i++) {
                sum += buf.readSmart();
            }
        }, 1000);
        
        benchmark("readSignedSmart()", [&]() {
            Buffer buf(data.slice(0, data.size()));
            volatile int sum = 0;
            for (int i = 0; i < 100; i++) {
                sum += buf.readSignedSmart();
            }
        }, 1000);
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=== Buffer Optimization Benchmark ===\n";
    std::cout << "Compiler: " << __VERSION__ << "\n";
#ifdef DEBUG_BUFFER
    std::cout << "Mode: DEBUG (bounds checking enabled)\n\n";
#else
    std::cout << "Mode: RELEASE (bounds checking disabled)\n\n";
#endif
    
    // Create test data from a real model
    // For now, just create dummy data
    std::vector<uint8_t> dummyData(5000);
    for (int i = 0; i < 5000; i++) dummyData[i] = i % 256;
    Buffer data(dummyData);
    
    testBasicReads(data);
    testReadString(data);
    testSpanVsSlice(data);
    testPeekVsRead(data);
    testSmartReads(data);
    
    std::cout << "\n=== Benchmark Complete ===\n";
    return 0;
}
