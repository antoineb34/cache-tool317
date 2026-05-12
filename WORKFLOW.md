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
├── tool/               ← CLI/debug cache inspection tool
│   └── src/main.cpp
├── client/             ← windowed game-client shell
├── third_party/        ← external libraries
├── cache/              ← the actual 317 cache files (gitignored)
└── build/              ← cmake build output (gitignored)
```

---

## Build System
- **CMake 3.20+**, **C++20**, **GCC**
- To build: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build build`
- Binary outputs:
  - `build/bin/tool` — CLI/debug cache inspection tool
  - `build/bin/client` — windowed game-client shell
- Run CLI tool: `./build/bin/tool ./cache`
- Run client shell: `./build/bin/client ./cache`
- Run a specific client region: `./build/bin/client ./cache --region 12345`
- Headless client smoke test: `env SDL_VIDEODRIVER=offscreen ./build/bin/client ./cache --smoke-test`

**External libs linked:**
- `BZip2` (system, found via `find_package(BZip2 REQUIRED)`)
- `ZLIB` (system, found via `find_package(ZLIB REQUIRED)`)
- `SDL3` (system, found via `find_package(SDL3 REQUIRED CONFIG)`)
- OpenGL — used by the client shell for the current render path
- Dear ImGui — **not yet added**, deferred until after the game renderer exists

Long-term client stack:
- `SDL3` owns the window, input, events, fullscreen/window mode, and later audio/controller support.
- `OpenGL` owns real rendering for maps, models, sprites, and UI composition.
- `Dear ImGui` is reserved for later editor-only panels after the playable/rendered client path is established.

Current priority is game rendering first. Editor/tool UI remains a future module, not the next focus.

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

Known definitions archive hashes include:
- `obj.dat` / `obj.idx`
- `npc.dat` / `npc.idx`
- `loc.dat` / `loc.idx`
- `flo.dat` / `flo.idx`
- `idk.dat` / `idk.idx`
- `seq.dat` / `seq.idx`
- `spotanim.dat` / `spotanim.idx`
- `varp.dat` / `varp.idx`
- `varbit.dat` / `varbit.idx`
- cracked small definition files: `mesanim.dat` / `mesanim.idx`, `mes.dat` / `mes.idx`, `param.dat` / `param.idx`

### `core/include/VersionList.h` + `VersionList.cpp`
Parses archive `0`, file `5` (`versionlist`) metadata.

Currently implemented:
- `VersionList::parse(archive)` reads `map_index`
- `mapIndex()` returns all parsed map region records
- `findMapRegion(regionId)` finds one map region record

`map_index` records are 7 bytes:
```text
regionId (2) | terrainFileId (2) | objectFileId (2) | flag (1)
```

Verified against the bundled cache:
```text
Versionlist loaded successfully.
Map regions: 587
First map region: regionId=7499 terrainFileId=216 objectFileId=217 flag=0
```

This is the bridge from a map region id to the actual `idx4` terrain/object file ids.

### Map Decoding
Initial map decoding is implemented in reusable core classes:
- `MapTerrain` parses one decompressed idx4 terrain file into 4 planes of 64x64 `Tile`s.
- `MapObjects` parses one decompressed idx4 object-placement file into `MapObject` placements.
- `MapRegion::load(reader, versionList, regionId)` connects `map_index` to the terrain/object idx4 files, decompresses them, and returns the decoded terrain + object placements.

Terrain files are GZIP-compressed idx4 files. Terrain decoding is opcode-per-tile:
```text
0      end tile, generated/default height
1      explicit height, followed by 1 height byte, then end tile
2..49  overlay shape/rotation opcode, followed by 1 overlay id byte
50..81 tile settings
82+    underlay id, stored as opcode - 81
```

Object files are GZIP-compressed idx4 files. Object placement decoding uses unsigned smart integers and deltas:
```text
object id delta
  position delta
  attributes byte: type = attributes >> 2, rotation = attributes & 3
  ...
0 ends placements for that object id
0 ends the object file
```

The tool can decode a region directly:
```bash
./build/bin/tool ./cache 12850
```

It can also inspect object placements with raw placement bytes, world coordinates, and `loc.def` details:
```bash
./build/bin/tool ./cache 12850 range
./build/bin/tool ./cache 12850 type=9
```

To inspect one exact world tile from a render:
```bash
./build/bin/tool ./cache 12850 tile 3212 3215 0
```

This prints the terrain bytes for that tile and every object placement whose footprint touches it.

Terrain sample output now includes the raw bytes for each tile and decodes them back into opcodes:
```text
bytes[9792..9798): 18 06 32 90 01 01
opcode 24: overlay id 6, path=5, rotation=2
opcode 50: settings=1
opcode 144: underlayId=63
opcode 1: explicit height byte 1 -> height step 0
```

First-pass rendering is implemented as a modular 2D region renderer:
```bash
./build/bin/tool ./cache 12850 render2d /tmp/region_12850.ppm 0 8
```

This writes a 512x512 PPM for one 64x64 plane at scale 8. Terrain colors come from `flo.dat`; object placements are drawn over the floor. This is intentionally a simple visual verification step before full 3D/model rendering.

Render layers can be selected with the final argument:
```bash
./build/bin/tool ./cache 12850 render2d region_12850_all.ppm 0 8 all
./build/bin/tool ./cache 12850 render2d region_12850_terrain.ppm 0 8 terrain
./build/bin/tool ./cache 12850 render2d region_12850_objects.ppm 0 8 objects
```

The 2D renderer draws object placement types differently:
```text
type 0    straight wall edge, using rotation
type 2    corner wall, using rotation
type 9    diagonal wall/object line
type >=10 filled object footprint using loc.def width/length
other     small center marker
```

Verified examples:
```text
Region 12850: terrain file 382, object file 383, 4505 object placements
Region 12345: terrain file 632, object file 633, 1815 object placements
```

### Definition Parsers
Opcode-driven parsers are implemented for:
- `ItemDef` from `obj.dat` / `obj.idx`
- `NpcDef` from `npc.dat` / `npc.idx`
- `LocDef` from `loc.dat` / `loc.idx`
- `FloDef` from `flo.dat` / `flo.idx`
- `IdkDef` from `idk.dat` / `idk.idx`
- `MesAnimDef` from `mesanim.dat` / `mesanim.idx`
- `MesDef` from `mes.dat` / `mes.idx`
- `ParamDef` from `param.dat` / `param.idx`
- `SeqDef` from `seq.dat` / `seq.idx`
- `SpotAnimDef` from `spotanim.dat` / `spotanim.idx`
- `VarbitDef` from `varbit.dat` / `varbit.idx`
- `VarpDef` from `varp.dat` / `varp.idx`

Each parser lives in `core/include/<Name>.h` and `core/src/<Name>.cpp`.
All definition parsers are strict: unknown opcodes throw a `std::runtime_error` instead of returning a partial definition.

### `core/include/DefinitionsLoader.h` + `DefinitionsLoader.cpp`
Loads parsed definition arrays from the definitions archive.

**Public API:**
- `loadItems(archive)`, `getItem(id)`, `itemCount()`
- `loadNpcs(archive)`, `getNpc(id)`, `npcCount()`
- `loadLocs(archive)`, `getLoc(id)`, `locCount()`
- `loadFlos(archive)`, `getFlo(id)`, `floCount()`
- `loadIdks(archive)`, `getIdk(id)`, `idkCount()`
- `loadMesAnims(archive)`, `getMesAnim(id)`, `mesAnimCount()`
- `loadMes(archive)`, `getMes(id)`, `mesCount()`
- `loadParams(archive)`, `getParam(id)`, `paramCount()`
- `loadSeqs(archive)`, `getSeq(id)`, `seqCount()`
- `loadSpotAnims(archive)`, `getSpotAnim(id)`, `spotAnimCount()`
- `loadVarbits(archive)`, `getVarbit(id)`, `varbitCount()`
- `loadVarps(archive)`, `getVarp(id)`, `varpCount()`

The shared loader validates that requested `.dat` / `.idx` files exist, that indexed entry sizes stay inside the data file, and that each parser consumes the full indexed entry. Missing, malformed, partially parsed, or unknown-opcode definition files should throw a clear `std::runtime_error` instead of crashing through `Buffer`.

### Definition Verification
The runnable tool now verifies all implemented definition parsers by loading archive `0,2` and printing counts:
```text
Cache opened successfully.
Definitions loaded successfully.
Items: 5553
NPCs:  2266
Locs:  7199
Flos:  122
Idks:  84
MesAnims: 20
Mes:   44
Params: 106
Seqs:  2195
SpotAnims: 408
Varbits: 627
Varps: 493
```

`loadLocs()` has been tested against the bundled cache:
- `loc.dat` size: 294,680 bytes
- `loc.idx` size: 14,400 bytes
- loaded loc definitions: 7,199
- `getLoc()` out-of-range guard works

Observed loc opcodes in this cache:
```text
0 1 2 3 5 14 15 17 18 19 21 22 23 24 28 29
30 31 32 33 34 39 40 60 62 64 65 66 67 68 69
70 71 72 73 74 75 77
```

All observed loc opcodes are currently handled by `LocDef::parse`.

Observed floor opcodes in this cache:
```text
0 1 2 3 5 6 7
```

Observed identity kit opcodes in this cache:
```text
0 1 2 3 40 50 60
```

Observed message animation opcodes in this cache:
```text
0
```

Observed message opcodes in this cache:
```text
0
```

Observed parameter opcodes in this cache:
```text
0
```

Observed animation sequence opcodes in this cache:
```text
0 1 2 3 4 5 6 7 8 9 10 11
```

Observed spot animation opcodes in this cache:
```text
0 1 2 4 5 6 7 8 40 41 42 43 44 45 50 51 52 53 54 55
```

Observed varbit opcodes in this cache:
```text
0 1
```

Observed varp opcodes in this cache:
```text
0 5
```

Useful transform relationship:
- Some `LocDef` and `NpcDef` entries use `varbitID` or `varpID` plus an `overrides` list.
- `VarbitDef` maps a varbit id to a backing `varpId` plus a bit range (`leastSignificantBit`..`mostSignificantBit`).
- Example from the bundled cache: loc `2452` (`airtemple_ruined`) uses `varbitID=607` with overrides `[7103, 7104]`; varbit `607` reads bit `0` from varp `491`.

### Client Shell
The client has been **completely rewritten from scratch** to be a minimal, educational renderer. The previous complex client was deleted. The goal is step-by-step learning of the graphics pipeline.

Current client (`client/src/main.cpp`):
- **SDL3** window + **OpenGL 2.1 immediate mode** context
- Single-file implementation (no `ClientApp` class, no headers)
- Loads **one model** from the cache (model 1219 from idx1) and renders it as a **wireframe**
- Uses **perspective projection** (custom `perspective()` matrix, not `gluPerspective`)
- Frame-rate independent camera controls

Current controls:
- **Mouse drag (left button)** = rotate the view (orbit)
- **Mouse wheel** = zoom in / out
- **Arrow keys** = pan / move around the scene

Graphics concepts learned so far:
1. Opening an SDL window and clearing the screen
2. `glBegin(GL_QUADS)` / `glEnd()` immediate mode
3. Normalized Device Coordinates (`-1` to `+1`) vs custom world coordinates
4. `glOrtho` for 2D projections (used for terrain earlier, currently not active)
5. Aspect ratio correction (`width/height`)
6. Perspective projection matrices (`glFrustum`-style manual matrix)
7. `glTranslatef` / `glRotatef` for camera positioning
8. `GL_LINES` for wireframe rendering
9. Double buffering (`SDL_GL_SwapWindow`)
10. Frame-rate independent movement (`deltaTime` from `SDL_GetTicks`)

The user understands `core/` (cache reading, definitions, map decode) but is learning graphics from zero. The agent must teach OpenGL concepts step-by-step before writing code.

The CLI `tool` remains available for detailed cache/debug inspection.

### Model Decoding
Initial `idx1` model decoding is implemented in `core/include/Model.h` + `core/src/Model.cpp`.

Currently parsed:
- vertices
- triangle indices
- face colors
- optional face render types, priorities, alpha, skins
- optional vertex skins
- textured triangle index triples
- model bounds

The decoder handles the classic 317 model footer/offset layout and uses model-local signed smart reads for vertex and triangle deltas.

The CLI can inspect one model:
```bash
./build/bin/tool ./cache model 1219
```

Verified examples:
```text
Model 1219: vertices=162 triangles=193 textured=5 bounds x=[-64,151] y=[-224,0] z=[-112,104]
Model 5404: vertices=75 triangles=106 textured=0 bounds x=[-126,126] y=[-66,0] z=[-47,49]
Model 1835: vertices=76 triangles=122 textured=0 bounds x=[-26,57] y=[-85,1] z=[-39,37]
Model 2141: vertices=68 triangles=120 textured=5 bounds x=[-48,48] y=[-80,0] z=[-48,48]
```

Client play mode now connects object placements to `LocDef.modelIDs`, loads unique selected models from `idx1`, and draws nearby placements as flat-shaded colored triangles. Model face colors are converted from the RS 16-bit HSL format to RGB using the original client's palette algorithm. If a loc has `modelTypes`, the placed `MapObject::type` selects the matching model id; untyped locs fall back to their first model id. The renderer applies placement rotation plus `LocDef` scale and offset fields. Lighting, animation, and exact origin/alignment behavior are still pending.

Future client/editor split:
```text
core
  cache parsing / cache writing / definitions / map decode

client_runtime
  SDL3 window/input
  OpenGL renderer
  asset manager
  world state

client
  normal deployable client build

editor_client
  same runtime with Dear ImGui editor layer enabled
```

For now there is one `client` executable while the game rendering path is being established. Editor work is deferred; when it returns, editor-only code should be isolated so the normal client can be built without editor access compiled in.

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
- Raw compressed cache files such as maps, models, and animations use **GZIP**; GZIP decompression is implemented in `CacheReader` via zlib.

---

## What's Next
The project is in a **graphics education phase**. The user is learning OpenGL from scratch by building the client renderer step-by-step. `core/` (cache reading, definitions, map decode, model decode) is complete and well-understood.

Current learning path (graphics):
1. ✅ Open window + clear screen
2. ✅ Draw a single square in 2D
3. ✅ Set up `glOrtho` for tile coordinates
4. ✅ Aspect ratio correction
5. ✅ Draw a grid of squares (64×64)
6. ✅ Load real terrain colors from `FloDef` and draw them
7. ✅ Switch to 3D: perspective projection matrix
8. ✅ Load and render a single model wireframe
9. ✅ Camera controls: rotate (mouse drag), zoom (wheel), pan (arrows)
10. ⏳ Frame-rate independent timing

Immediate next topics (user decides order):
- Fill triangles with solid colors (wireframe → filled faces)
- Use real face colors from the model (HSL → RGB palette lookup)
- Render the model at actual world positions on the map terrain
- Add more models / object placements
- Textures (after solid-color rendering is solid)

The user decides direction. The agent explains concepts, then writes minimal code, then builds and verifies. No dumping entire files.

## Agent Collaboration Notes
The user is the main brain. The agent executes. **Critical rules for future agents:**

1. **Explain before writing.** The user wants to understand the graphics pipeline and OpenGL concepts. Do not write code the user doesn't understand.
2. **One step at a time.** Each step should add one concept and ~10-20 lines of code maximum.
3. **Build and verify after every change.** The user must see it working before moving on.
4. **Do not dump entire files.** Make small surgical edits. The user cannot follow massive code dumps.
5. **The user knows C++.** Do not explain basic programming. Explain graphics concepts: projection matrices, coordinate systems, rasterization, the GPU pipeline, etc.
6. **Ask before refactoring.** If the code is working, ask if the user wants to clean it up or add the next feature.
7. **The `core/` library is sacred.** The user fully understands it. Do not modify `core/` without explicit permission.

To get the parsed definitions archive:
```cpp
Archive defs = reader.readArchive(0, 2);
DefinitionsLoader loader;
loader.loadItems(defs);
loader.loadNpcs(defs);
loader.loadLocs(defs);
loader.loadFlos(defs);
loader.loadIdks(defs);
loader.loadMesAnims(defs);
loader.loadMes(defs);
loader.loadParams(defs);
loader.loadSeqs(defs);
loader.loadSpotAnims(defs);
loader.loadVarbits(defs);
loader.loadVarps(defs);
```

GZIP decompression and first-pass terrain/object placement parsing are implemented. `FloDef`, `LocDef`, `VarbitDef`, and `VarpDef` are available to support terrain colors/textures and object transform resolution.

Use `VersionList::mapIndex()` to map a region id to the `idx4` file ids:
```cpp
Archive versionlistArchive = reader.readArchive(0, 5);
VersionList versionList = VersionList::parse(versionlistArchive);
const MapIndexEntry* entry = versionList.findMapRegion(regionId);
// entry->terrainFileId and entry->objectFileId are files in archive idx4
```

---

## Planned but Not Started
- Model viewer with solid colors and real face colors
- Model placement on map terrain at correct world positions
- Textured triangle rendering
- Playable game-client runtime
- Dear ImGui editor panels
- Writing/modifying cache files
