# Project Architecture

## Module Overview

```
┌──────────────────────┐   ┌──────────────────────────────┐
│      tool.exe        │   │         client.exe            │
│  (CLI cache tool)    │   │  (SDL3 window + OpenGL)       │
└──────────┬───────────┘   └──────────────┬───────────────┘
           │                              │
           │       links against          │ links against
           └──────────────┬───────────────┘
              ┌───────────▼───────────────┐
              │        libcore.a          │
              │  (cache reading,          │
              │   parsing, definitions)   │
              └───────────┬───────────────┘
                          │ links against
                     BZip2, ZLIB

client.exe also links: SDL3, OpenGL
```

## core/ — Shared Library

Everything in `core/` is **cache-agnostic** — it knows about RS317 file formats but has no UI or rendering code.

### Layers (bottom to top)

1. **Byte I/O** — `Buffer.h/cpp`
   - Sequential big-endian byte reader
   - All cache parsing goes through this

2. **Cache I/O** — `CacheReader.h/cpp`
   - Opens `.dat` + `.idx` files
   - **Memory-mapped I/O (mmap)** for zero-copy access (Linux)
   - **Archive caching** to avoid repeated decompression
   - Reads sectors, follows chains
   - Decompresses BZIP2 and GZIP
   - Parses JAG sub-archives
   - Fallback to std::ifstream if mmap unavailable

3. **Archive** — `Archive.h/cpp`
   - Holds parsed JAG sub-archive
   - Maps name hashes to decompressed file bytes

4. **Definitions** — `DefinitionsLoader.h/cpp` + `*Def.h/cpp`
   - Parses game definition files (items, NPCs, locations, floors, etc.)
   - Each definition has its own `.h` + `.cpp`
   - Strict: unknown opcodes throw

5. **Map Data** — `MapRegion.h/cpp`, `MapTerrain.h/cpp`, `MapObjects.h/cpp`
   - Parses terrain and object placement files
   - Connects to `LocDef` for object properties

6. **2D Rendering** — `RegionRenderer2D.h/cpp`
   - Renders map regions to PPM images
   - Uses `FloDef` for terrain colors

## tool/ — CLI Tool

Single-file `main.cpp` that:
- Opens the cache
- Loads definitions
- Decodes and prints region data
- Renders 2D map images

## client/ — Game Client

Currently a stub. Will eventually:
- Open SDL3 window with OpenGL context
- Render 3D models with lighting
- Display the game world

## CMake Structure

```cmake
# Root CMakeLists.txt
find_package(BZip2 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(SDL3 REQUIRED CONFIG)
find_package(OpenGL REQUIRED)

add_subdirectory(core)   # STATIC library
add_subdirectory(tool)   # executable, links core
add_subdirectory(client) # executable, links core + SDL3 + OpenGL
```

`core` uses `PUBLIC` includes so tool and client inherit the include path.

## Performance Optimizations

### CacheReader Optimizations
- **Memory-mapped I/O**: Uses `mmap()` for `.dat` and `.idx` files on Linux, eliminating `read()`/`seekg()` syscalls
- **Archive caching**: Parsed archives are cached with `std::shared_ptr<Archive>`, avoiding repeated BZip2 decompression (5-9x faster repeated reads)
- **Sector batch reading**: Reads all sectors in one operation instead of per-sector
- **Sub-microsecond file checks**: `hasFile()` and `getFileCount()` are 16-59x faster with mmap

### Benchmark Suite
- `BufferBenchmark.cpp` — tests Buffer read operations, span() vs slice(), etc.
- `CacheBenchmark.cpp` — tests CacheReader performance (file reads, archive reads, model parsing)
- Build with `-DBUILD_BENCHMARKS=ON` to enable benchmarks
