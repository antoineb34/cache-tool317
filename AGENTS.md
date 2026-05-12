# cache-tool317 — Agent Instructions

## What This Is

A **RuneScape 317 cache tool** written in **C++20** with **CMake**.

The end goal is a fully customizable cache tool (read, write, create assets) plus a game client. Both share the `core` library.

**GitHub:** https://github.com/antoineb34/cache-tool317

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

Binaries:
- `build/bin/tool` — CLI cache inspection tool
- `build/bin/client` — windowed game client (currently a stub)

## Directory Map

```
cache-tool/
├── core/               ← shared static library (used by tool and client)
│   ├── include/        ← headers (.h)
│   └── src/            ← implementation (.cpp)
├── tool/               ← CLI cache inspection tool
│   └── src/main.cpp
├── client/             ← windowed game client
│   └── src/main.cpp
├── docs/               ← reference documentation (cache formats, algorithms, etc.)
├── third_party/        ← external libraries (empty, use system packages)
├── cache/              ← the actual 317 cache files (gitignored)
└── build/              ← cmake build output (gitignored)
```

## Collaboration Rules

- **The user is the main brain.** They decide what to build, what to name things, and the direction.
- **The agent executes.** Write code based on the user's direction. Don't design independently.
- **Think together before writing.** Present a plan, let the user confirm or redirect, then write.
- **Do one thing at a time.** Don't write multiple systems at once.
- **Always build and verify after every change.**

## Coding Conventions

- **C++20**, **GCC**, **CMake 3.20+**
- Each major type gets its own `.h` + `.cpp` pair
- Use `Buffer` for ALL sequential byte reading — never raw `data[offset]` arithmetic inline
- Unknown opcodes in parsers throw `std::runtime_error` (strict parsing)
- Public API is clean and minimal — implementation details go `private`
- Include guards: `#pragma once`
- No namespaces — flat global structs and functions (matches existing style)
- Error handling: throw `std::runtime_error` with descriptive messages

## Sacred Areas

- **`core/` cache parsing code** — user fully understands it. Ask before modifying.
- **`Buffer.h/cpp`** — the universal byte reader. Used everywhere.
- **`CacheReader.h/cpp`** — the cache I/O layer.

## External Dependencies

- `BZip2` — Jagex BZIP2 decompression
- `ZLIB` — GZIP decompression
- `SDL3` — window, input, events
- `OpenGL` — rendering (client only)

## Where to Find Things

| Need | Look In |
|------|---------|
| Cache format specs | `docs/cache-format.md` |
| Build / architecture | `docs/architecture.md` |
| Coding conventions (detailed) | `docs/conventions.md` |
| Algorithm reference (graphics, lighting, palette) | `docs/algorithms.md` |
| Project history / learning journal | `docs/history.md` |
| How to add a definition parser | `docs/adding-parsers.md` |
