# cache-tool317 — Agent Workflow Document

## Collaboration Model
- The **user is the main brain**. They decide what to build, what to name things, and the direction of the project.
- The **agent executes**. Write code based on the user's direction. Don't go off and design things independently.
- Think together before writing. Present a plan, let the user confirm or redirect, then write.
- Do **one thing at a time**. Don't write multiple systems at once.
- Always commit and push after a logical piece of work is done.

---

## Project Overview
A **RuneScape 317 cache tool** written in **C++20** with **CMake**.  
The end goal is a fully customizable cache tool (read, write, create assets) + a game client.  
Both will share the `core` library.

**GitHub:** https://github.com/antoineb34/cache-tool317

---

## Project Structure
```
cache-tool/
├── core/               ← shared library (used by both tool and client)
│   ├── include/        ← all header files (.h)
│   └── src/            ← all source files (.cpp)
├── tool/               ← the cache editor (runnable)
│   └── src/main.cpp
├── client/             ← game client (not started yet)
├── third_party/        ← external libraries
├── cache/              ← the actual 317 cache files (gitignored)
└── build/              ← cmake build output (gitignored)
```

---

## Build System
- **CMake 3.20+**, **C++20**, **GCC**
- To build: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build build`
- Binary output: `build/bin/tool`
- Run: `./build/bin/tool ./cache`

**External libs linked:**
- `BZip2` (system, found via `find_package(BZip2 REQUIRED)`)
- GLFW + OpenGL + Dear ImGui — **not yet added**, planned for the tool GUI

---

## Coding Conventions
- Structs in `core/include/CacheDefinitions.h` for raw binary format types
- Each major class gets its own `.h` + `.cpp`
- **`Buffer`** is used for ALL sequential byte reading — never do raw `data[offset]` arithmetic inline
- Public API should be clean and minimal — implementation details go `private`
- Temporary/debug code is clearly marked with `// TODO: remove` and cleaned up before committing
- Commit messages are descriptive and tight

---

## What's Been Built

### `core/include/CacheDefinitions.h`
Raw binary format structs — these map directly to bytes in the cache:
- `IndexEntry` — 6-byte record in `.idx` files (size + firstSector)
- `ArchiveEntry` — 10-byte record in JAG sub-archives (nameHash + decompressedSize + compressedSize)
- `Sector` — 520-byte block in `.dat` file (8-byte `Header` + 512-byte data)

### `core/include/Buffer.h` + `Buffer.cpp`
Sequential byte reader wrapping a `const uint8_t*`. Big-endian reads.  
Methods: `readByte()`, `readSByte()`, `readUShort()`, `readShort()`, `readTribyte()`, `readInt()`, `readString()`, `skip()`, `position()`, `remaining()`, `isEmpty()`

### `core/include/CacheReader.h` + `CacheReader.cpp`
Opens and reads the cache files.  
**Public API:**
- `open(path)` — opens `.dat` + all 5 `.idx` files
- `readFile(archiveId, fileId)` — reads raw bytes by following the sector chain
- `readArchive(archiveId, fileId)` — reads + decompresses a packed JAG sub-archive, returns `Archive`

**Private internals:**
- `readIndex()` — reads one `IndexEntry` from an `.idx` file
- `readSector()` — reads one `Sector` from the `.dat` file

### `core/include/Archive.h` + `Archive.cpp`
Holds a parsed JAG sub-archive.  
```cpp
struct Archive {
    std::unordered_map<uint32_t, std::vector<uint8_t>> files; // hash → bytes
    std::vector<uint8_t> getFile(uint32_t nameHash) const;
    std::vector<uint8_t> getFile(const std::string& name) const;
    bool hasFile(uint32_t nameHash) const;
    bool hasFile(const std::string& name) const;
    std::size_t size() const;
    static uint32_t hashName(const std::string& name); // (hash * 61 + toupper(c)) - 32
};
```

### `core/include/ArchiveNames.h`
Namespace of `constexpr uint32_t` hash constants for all known sub-file names.  
Keep this updated whenever new hashes are identified.

---

## 317 Cache Format — Key Knowledge

### File Layout
```
main_file_cache.dat    — all raw data, divided into 520-byte sectors
main_file_cache.idx0   — index for archive 0 (definitions)
main_file_cache.idx1   — index for models
main_file_cache.idx2   — index for animations
main_file_cache.idx3   — index for midis/sounds
main_file_cache.idx4   — index for maps
```

### Sector Format (520 bytes)
`fileId (2) | chunk (2) | nextSector (3) | archiveId (1) | data (512)`

### Index Entry Format (6 bytes)
`size (3) | firstSector (3)`

### Archive 0 — Sub-Archive Map
Each file in archive 0 is itself a packed JAG archive:
| File ID | Name | Contents |
|---------|------|----------|
| 1 | title | fonts (p11_full.dat, p12_full.dat, b12_full.dat, q8_full.dat), logo, background, buttons, runes, index.dat |
| 2 | definitions | obj.dat/idx, npc.dat/idx, loc.dat/idx, flo.dat, idk.dat/idx, seq.dat/idx, spotanim.dat/idx, varp.dat/idx, varbit.dat/idx |
| 3 | interface | UI definitions |
| 4 | media | 2D graphics/sprites |
| 5 | versionlist | update list |
| 6 | textures | textures |
| 7 | wordenc | chat filter |
| 8 | sounds | sound effects |

### JAG Sub-Archive Format
```
[3 bytes: decompressed size]
[3 bytes: compressed size]
  if sizes equal → not whole-archive compressed (sub-files may be individually compressed)
  if sizes differ → BZIP2 whole-archive compressed (sub-files are raw inside)
[2 bytes: number of entries]
for each entry (10 bytes):
    [4 bytes: name hash]
    [3 bytes: decompressed size]
    [3 bytes: compressed size]
[raw data for each entry, sequentially]
```

### Name Hash Algorithm
```cpp
uint32_t hash = 0;
for (char c : name)
    hash = (hash * 61 + toupper(c)) - 32;
```

### Compression
- Jagex BZIP2 is standard BZIP2 **without** the `BZh1` header — prepend it before decompressing
- Models (idx1) and animations (idx2) use **GZIP** — not yet implemented

---

## What's Next
The immediate next step is **parsing item definitions** from `obj.dat`.

To get `obj.dat`:
```cpp
Archive defs = reader.readArchive(0, 2);
auto objDat = defs.getFile(ArchiveNames::OBJ_DAT);
Buffer buf(objDat);
```

The `obj.dat` format starts with a 2-byte count of items, then for each item:
- A series of opcode-driven fields (read opcode, then read the fields for that opcode, loop until opcode 0)

The item definition struct should go in `core/include/` and the parser in `core/src/`.

After items: NPCs (`npc.dat`), then objects/locations (`loc.dat`), then maps.

---

## Planned but Not Started
- GUI (GLFW + OpenGL + Dear ImGui) for the tool
- Model viewer (renders idx1 model files)
- Map viewer (renders idx4 map files)
- Game client (`client/` folder exists but is empty)
- GZIP decompression (needed for models and animations)
- Writing/modifying cache files
