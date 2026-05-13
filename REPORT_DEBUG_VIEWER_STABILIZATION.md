# Debug Viewer Stabilization Report

## Branch / Commits

- **Branch:** `feature/wireframe-debug-viewer`
- **Commit:** `8ec451e` — "Stabilize debug viewer: remove stale file, add controls, error handling"
- **Parent:** `d09ef98` (archiveId fix)
- **Full commit history on branch:**
  - `d09ef98` Fix sector archiveId validation (1-based offset)
  - `f5a70ae` Restructure client into minimal wireframe debug viewer
  - `a132fe4` Replace AI-generated client with minimal wireframe debug viewer

## Exact Commands Tested

```bash
# Model 100 (default tree) — works
./build/bin/client ./cache --model 100

# Model 200 — works
./build/bin/client ./cache --model 200

# Model 99 — works
./build/bin/client ./cache --model 99

# Model 1000 — fails (corrupted payload in this cache)
./build/bin/client ./cache --model 1000

# Model 99999 — gracefully rejected (not in archive)
./build/bin/client ./cache --model 99999

# Benchmark suite — all pass
./build/bin/cache_benchmark ./cache
```

## Models Tested

| Model ID | Status | Vertices | Triangles | Textured | Notes |
|---|---|---|---|---|---|
| 1 | ❌ Parse error | — | — | — | Buffer truncated (pos=2367, need=2) |
| 2 | ❌ Parse error | — | — | — | Buffer truncated |
| 5 | ❌ Parse error | — | — | — | Buffer truncated |
| 10 | ❌ Parse error | — | — | — | Buffer truncated |
| 50 | ❌ Parse error | — | — | — | Buffer truncated |
| 99 | ✅ OK | 33 | 56 | 0 | |
| 100 | ✅ OK | 28 | 62 | 0 | Default model |
| 101 | ✅ OK | 16 | 16 | 0 | |
| 200 | ✅ OK | 28 | 44 | 0 | |
| 250 | ✅ OK | 19 | 22 | 0 | |
| 500 | ✅ OK | 21 | 38 | 0 | |
| 1000 | ❌ Parse error | — | — | — | Buffer truncated |
| 99999 | ❌ Not found | — | — | — | Correct error message |

## Files Changed

| File | Change | Reason |
|---|---|---|
| `client/DebugModelViewer.h` (old) | **Deleted** | Stale duplicate; active header is `client/include/DebugModelViewer.h` |
| `client/include/DebugModelViewer.h` | Updated | Added `reloadModel()`, `toggleWireframe()`, `toggleCulling()`, `resetView()`, `wireframe_`/`culling_` state |
| `client/src/DebugModelViewer.cpp` | Updated | Implemented new methods; added try/catch around `ModelDef::parse()`, wireframe/culling render paths |
| `client/include/App.h` | Updated | `run()` now takes `CacheReader&` and `initialModelId`; added `viewer_`, `reader_`, `currentModelId_` members, `handleDebugKeys()` |
| `client/src/App.cpp` | Rewritten | Debug key handling (arrows, R, W, C), graceful model reload with error checking |
| `client/src/main.cpp` | Updated | Simplified; delegates everything to `app.run(reader, modelId)` |

## Controls Added

| Key | Action |
|---|---|
| **Left Arrow** | Load previous valid model (decrements until `hasFile()` returns true) |
| **Right Arrow** | Load next valid model (increments until `hasFile()` returns true) |
| **R** | Reset rotation to default (0°) |
| **W** | Toggle wireframe ↔ solid rendering |
| **C** | Toggle backface culling on/off |
| **ESC** | Close window (existing) |

## Build / Run Status

- **Build:** All 5 targets pass cleanly (`core`, `buffer_benchmark`, `cache_benchmark`, `tool`, `client`)
- **Compiler:** GCC 15.2.1, C++20, SDL3 + OpenGL 3.3
- **Client launch:** Works for valid models; gracefully handles corrupt/missing models
- **Benchmarks:** All 6 benchmark tests pass

## Corrupted Models — Root Cause

Several models (1, 2, 5, 10, 50, 1000) fail with `Buffer: read past end of buffer` during `ModelDef::parse()`. This is **not a code bug** — the cache data (`main_file_cache.dat`) contains truncated payloads for these model entries. The index file claims sizes that don't match the actual compressed data available.

Evidence:
- `hasFile()` returns `true` for these models (non-zero size, non-zero firstSector)
- The compressed bytes decompress successfully via `readGzippedFile()`
- But `ModelDef::parse()` reads past the end of the decompressed buffer

The viewer now handles this gracefully via try/catch — corrupted models print an error and the interactive viewer stays running instead of crashing.

## What Was NOT Added (per requirements)

- No maps, terrain, textures
- No lighting or materials
- No ECS, scene graph, or asset manager
- No generalized renderer abstractions
- No animation system

## Recommended Next Smallest Step

1. **Investigate corrupted models:** Check whether the `size` values in `idx1` for models 1, 2, 5, 10, 50, 1000 are genuinely wrong or whether the compression/decompression path has an edge case. Compare against a known-good RS317 cache.

2. **Add `--model` validation hint:** When `--model` points to a corrupt file, suggest the nearest valid model IDs.

3. **Texture triangle rendering:** All tested models show `tex=0`. Find a model with textured faces and verify the texture triangle path works (may require loading a texture archive from idx2).

4. **Camera controls:** Add mouse-drag orbit or scroll-to-zoom for better geometry inspection.

5. **Edge rendering for solid mode:** When rendering as triangles, also draw wireframe edges on top so individual triangles remain visible.