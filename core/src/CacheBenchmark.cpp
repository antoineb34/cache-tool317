#include "Buffer.h"
#include "CacheReader.h"
#include "ModelDef.h"
#include "TextureDef.h"
#include "DefinitionsLoader.h"
#include "ArchiveNames.h"
#include "ItemDef.h"
#include "NpcDef.h"
#include "LocDef.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <filesystem>

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

// Test 1: CacheReader open
void benchmarkOpen(const std::string& cachePath) {
    std::cout << "--- Test 1: CacheReader Open ---\n";
    benchmark("Open cache", [&]() {
        CacheReader r;
        r.open(cachePath);
    }, 10);
}

// Test 2: Read model file
void benchmarkModelRead(CacheReader& reader) {
    std::cout << "\n--- Test 2: Model File Reading ---\n";
    
    benchmark("Read model 1000 (gzip)", [&]() {
        Buffer buf = reader.readGzippedFile(1, 1000);
        (void)buf;
    }, 100);
    
    benchmark("Read model 5000 (gzip)", [&]() {
        Buffer buf = reader.readGzippedFile(1, 5000);
        (void)buf;
    }, 100);
}

// Test 3: Read texture archive
void benchmarkTextureArchive(CacheReader& reader) {
    std::cout << "\n--- Test 3: Texture Archive Reading ---\n";
    
    benchmark("Read texture archive (0,6)", [&]() {
        auto archive = reader.readArchive(0, 6);
        (void)archive;
    }, 10);
}

// Test 4: Read definitions archive
void benchmarkDefsArchive(CacheReader& reader) {
    std::cout << "\n--- Test 4: Definitions Archive Reading ---\n";
    
    benchmark("Read def archive (0,2)", [&]() {
        auto archive = reader.readArchive(0, 2);
        (void)archive;
    }, 10);
}

// Test 5: Full model parsing
void benchmarkModelParsing(CacheReader& reader) {
    std::cout << "\n--- Test 5: Full Model Parsing ---\n";
    
    // First check if model 1000 exists
    if (!reader.hasFile(1, 1000)) {
        std::cout << "Model 1000 does not exist, skipping benchmark\n";
        return;
    }
    
    benchmark("Parse model 1000", [&]() {
        try {
            Buffer buf = reader.readGzippedFile(1, 1000);
            if (!buf.empty()) {
                ModelDef def = ModelDef::parse(1000, buf);
                (void)def;
            }
        } catch (const std::exception& e) {
            // Ignore parse errors in benchmark
        }
    }, 100);
}

// Test 6: File existence checks
void benchmarkFileChecks(CacheReader& reader) {
    std::cout << "\n--- Test 6: File Existence Checks ---\n";
    
    benchmark("hasFile() - exists", [&]() {
        bool exists = reader.hasFile(1, 1000);
        (void)exists;
    }, 1000);
    
    benchmark("hasFile() - not exists", [&]() {
        bool exists = reader.hasFile(1, 999999);
        (void)exists;
    }, 1000);
    
    benchmark("getFileCount()", [&]() {
        int count = reader.getFileCount(1);
        (void)count;
    }, 1000);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: cache_benchmark <cache_path>\n";
        return 1;
    }
    
    std::cout << "=== CacheReader Performance Benchmark ===\n";
    std::cout << "Compiler: " << __VERSION__ << "\n";
#ifdef DEBUG_BUFFER
    std::cout << "Mode: DEBUG (bounds checking enabled)\n\n";
#else
    std::cout << "Mode: RELEASE (bounds checking disabled)\n\n";
#endif
    
    CacheReader reader;
    if (!reader.open(argv[1])) {
        std::cerr << "Failed to open cache: " << argv[1] << "\n";
        return 1;
    }
    
    try {
        benchmarkOpen(argv[1]);
        benchmarkModelRead(reader);
        benchmarkTextureArchive(reader);
        benchmarkDefsArchive(reader);
        benchmarkModelParsing(reader);
        benchmarkFileChecks(reader);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "\n=== Benchmark Complete ===\n";
    return 0;
}
