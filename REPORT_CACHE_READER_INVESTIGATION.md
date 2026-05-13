# CacheReader Investigation Report

## Branch / Commit

- **Branch:** `feature/wireframe-debug-viewer`
- **Commit:** `d09ef98` — "Fix sector archiveId validation: DAT sector headers use 1-based idx IDs"
- **Parent:** `f5a70ae` (wireframe debug viewer restructure)

## Exact Command Run

```bash
./build/bin/client ./cache
```

## Exact Error Observed

```
CacheReader: sector archiveId mismatch (expected=1, got=2, sector=1954)
terminate called after throwing an instance of 'std::runtime_error'
```

## Files Inspected

| File | Purpose |
|---|---|
| `core/src/CacheReader.cpp` (lines 209-285) | `readFile()` sector-chain traversal and header validation |
| `core/include/CacheReader.h` | API interface — `archiveId` parameter is 0-based (0=defs, 1=models, etc.) |
| `cache/main_file_cache.idx0` through `idx4` | Index files — 6-byte entries (3-byte size + 3-byte firstSector) |
| `cache/main_file_cache.dat` | Raw cache data — 520-byte sectors with 8-byte headers |

## Files Changed

- **`core/src/CacheReader.cpp`** — 1 line changed (the archiveId comparison), plus comment

## Debug Output Summary

### Sector Header Layout Verification

Raw sector headers at model 100's first sector (1954):

```
Sector 1954 | offset=1016080 | fileId=100, chunk=0, next=0, archId=2
```

The sector header archiveId is `2`, but the code expected `1` (the 0-based idx index passed by the caller).

### Full Mismatch Pattern

A comprehensive scan of every index entry vs. its sector header revealed a **systematic +1 offset** across the entire DAT file:

| idx file | API archiveId | Sector header archiveId | Delta |
|---|---|---|---|
| idx0 (definitions) | 0 | 1 | +1 |
| idx1 (models) | 1 | 2 | +1 |
| idx2 (animations) | 2 | 3 | +1 |
| idx3 (midis) | 3 | 4 | +1 |
| idx4 (maps) | 4 | 5 | +1 |

All 13,317 non-empty index entries were verified — every single sector header stores `archiveId = idxIndex + 1`.

### Archive ID Ranges in the DAT File

```
Sectors     0-    0: archiveId=0  (unused/empty)
Sectors     1- 1778: archiveId=1  (idx0 — definitions)
Sectors  1779-12271: archiveId=2  (idx1 — models)
Sectors 12272-13324: archiveId=3  (idx2 — animations)
Sectors 13325-18825: archiveId=4  (idx3 — midis)
Sectors 18826-24129: archiveId=5  (idx4 — maps)
```

## What Was Ruled Out

| Hypothesis | Status | Evidence |
|---|---|---|
| Wrong model ID (100 doesn't exist) | **Ruled out** | `hasFile(1, 100)` returns true; idx1[100] has size=266, firstSector=1954 |
| Wrong index table (reading from wrong idx) | **Ruled out** | idx1[100] points to sector 1954; sector 1954 fileId header = 100 ✓ |
| Wrong sector-chain traversal | **Ruled out** | Single-sector files (chunks=0, next=0) — no chain to follow, fails on first sector |
| DAT file corruption | **Ruled out** | Archive IDs are *consistently* +1 across the entire file — not random corruption |
| Wrong cache layout assumption | **Ruled out** | Contiguous sector layout verified; all sector headers have correct fileId and chunk values |
| idx0 returns definitions (not models) | **Ruled out** | idx0 entries confirmed to start at sector 1 with archiveId=1 in header |

## Suspected Cause

**The DAT sector header `archiveId` field is 1-based, not 0-based.**

The cache format uses `archiveId + 1` in the 8-byte sector header. The CacheReader's `readFile()` method compared the raw sector byte directly against the 0-based API `archiveId` argument, causing a mismatch on every single file read across all five archive types.

## Fix Applied

Single-line change in `core/src/CacheReader.cpp` line 262:

```cpp
// BEFORE (broken):
if (sectorArchiveId != static_cast<uint8_t>(archiveId))

// AFTER (fixed):
// NOTE: sector header archive id is 1-based (idx0=1, idx1=2, etc.)
if (sectorArchiveId != static_cast<uint8_t>(archiveId + 1))
```

The error message was also updated to report `archiveId + 1` as the expected value for clarity.

## Build / Run Status

- **Build:** All 5 targets pass (`core`, `buffer_benchmark`, `cache_benchmark`, `tool`, `client`)
- **Client run (model 100):** Works — 28 vertices, 62 triangles loaded and rendered
- **Client run (model 200):** Works — 28 vertices, 44 triangles
- **Benchmarks:** All pass with normal performance numbers

## Recommended Next Smallest Step

1. **Verify model `hasFaceRenderTypes` parsing** — model 100 reports `tex=0` (no textured faces); confirm whether this is a cache limitation or a parsing bug by checking a known textured model ID.
2. **Remove stale `client/DebugModelViewer.h`** — there are two copies: one at `client/DebugModelViewer.h` (stale, committed accidentally) and one at `client/include/DebugModelViewer.h` (the active one). The stale copy should be removed.
3. **Test `--model` flag end-to-end** to confirm arbitrary model IDs load correctly now that the archiveId validation is fixed.