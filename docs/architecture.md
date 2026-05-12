# Project Architecture

## Module Overview

```
┌─────────────────────────────────────────┐
│              client.exe                  │
│  (SDL3 window + OpenGL renderer)         │
└─────────────────┬───────────────────────┘
                  │ links against
┌─────────────────▼───────────────────────┐
│              tool.exe                    │
│  (CLI cache inspection)                  │
└─────────────────┬───────────────────────┘
                  │ links against
┌─────────────────▼───────────────────────┐
│              libcore.a                   │
│  (cache reading, parsing, definitions)   │
└─────────────────────────────────────────┘
                  │ links against
    BZip2, ZLIB, SDL3, OpenGL
```

## core/ — Shared Library

Everything in `core/` is **cache-agnostic** — it knows about RS317 file formats but has no UI or rendering code.

### Layers (bottom to top)

1. **Byte I/O** — `Buffer.h/cpp`
   - Sequential big-endian byte reader
   - All cache parsing goes through this

2. **Cache I/O** — `CacheReader.h/cpp`
   - Opens `.dat` + `.idx` files
   - Reads sectors, follows chains
   - Decompresses BZIP2 and GZIP
   - Parses JAG sub-archives

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

## Adding a New Module

1. Create `core/include/NewModule.h` and `core/src/NewModule.cpp`
2. Add `src/NewModule.cpp` to `core/CMakeLists.txt`
3. Include `"NewModule.h"` from tool or client
4. Build and verify
