# CLIENT RESET — Minimal SDL3/OpenGL Base

## Files Deleted

- `client/src/DebugModelViewer.cpp` — model loading, GL buffer upload, shader compilation, wireframe rendering
- `client/include/DebugModelViewer.h` — Vec3f/Mat4f math, WireVert struct, hslToRgb, full viewer class
- `client/include/App.h` (old) — depended on DebugModelViewer, CacheReader, model ID tracking
- `client/src/App.cpp` (old) — depended on DebugModelViewer, CacheReader, ran model-viewer loop
- `client/src/main.cpp` (old) — parsed CLI for cache path + model ID, opened CacheReader

## Files Created

- `client/include/App.h` — minimal class: init/handleEvents/render/shutdown/run, SDL_Window* + SDL_GLContext only
- `client/src/App.cpp` — SDL3 init, OpenGL context creation, gray clear, Escape/quit handling
- `client/src/main.cpp` — create App, call run()

## CMake Changes

- `client/CMakeLists.txt` — removed `core` dependency, removed `DebugModelViewer.cpp`, removed `GL_GLEXT_PROTOTYPES` define (no extension loading needed), removed `client` subdirectory link to `core`

## Exact Commands Run

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
timeout 5 ./build/bin/client ./cache
```

## Build Status

**SUCCESS** — all targets built cleanly (core, buffer_benchmark, cache_benchmark, tool, client).

## Runtime Behavior

- Gray window (1280x720) appeared: **YES**
- Window title: "RS317 Cache Tool"
- Escape key closes window: **YES** (verified — window closed cleanly, exit code 0)
- No crash, no errors on stdout/stderr
- No cache loading (not needed for base test)

## Known Issues

- None. Clean SDL3 + OpenGL 3.3 core context, single-file each for App header/source/entry.