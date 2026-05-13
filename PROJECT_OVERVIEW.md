# cache-tool317 — Project Overview

## Purpose

This project is a modern C++20 toolkit and experimental client for understanding, parsing, visualizing, and eventually rendering RuneScape 317 cache data.

The project is intentionally being built step-by-step to:
- understand the RS317 cache format deeply
- learn graphics/rendering fundamentals
- build a clean and maintainable architecture
- avoid overengineered RSPS spaghetti code

This is not intended to become a copy-paste private server client.
The focus is understanding the systems and building a strong technical foundation.

---

# High-Level Architecture

The project is layered intentionally.

```txt
main_file_cache.dat / idx files
            ↓
       CacheReader
            ↓
          Buffer
            ↓
     Parsers / Loaders
            ↓
  Models / Maps / Definitions
            ↓
        Renderer
            ↓
           Client
```

Each layer has a specific responsibility.

---

# Core Systems

## Buffer

`Buffer` is the low-level binary reading primitive.

Responsibilities:
- sequential byte reading
- bounds checking
- endian-aware reads
- smart decoding
- string decoding
- safe cursor management

Parsers should read binary data through `Buffer` instead of manual pointer arithmetic.

---

## CacheReader

`CacheReader` handles:
- `.dat` and `.idx` access
- mmap-backed file access
- sector-chain traversal
- raw cache file extraction
- gzip/bzip2 decompression helpers
- JAG archive loading/caching

`CacheReader` should NOT contain gameplay parsing logic.

It is a cache access layer, not a definitions layer.

---

## Archive

`Archive` represents a JAG archive container.

Responsibilities:
- store decompressed sub-files
- lookup by hash/name
- expose archive contents cleanly

---

## Parsers

Parsers convert raw binary data into structured data.

Examples:
- ItemDef
- NpcDef
- LocDef
- FloDef
- ModelDef
- SeqDef

Parsers should:
- consume `Buffer`
- avoid filesystem logic
- avoid rendering logic

---

# Rendering Direction

The client uses:
- SDL3
- OpenGL

The renderer is being built gradually:
1. SDL/OpenGL bootstrap
2. simple geometry
3. tile rendering
4. camera system
5. terrain rendering
6. model rendering
7. cache-driven world rendering

The goal is to understand the rendering pipeline, not to immediately build a full MMORPG client.

---

# Current Development Philosophy

The project intentionally prioritizes:

```txt
correctness
clarity
understanding
small incremental changes
```

over:

```txt
massive abstractions
premature optimization
overengineering
AI-generated complexity
```

---

# AI Agent Workflow

AI agents are used as implementation helpers, not autonomous architects.

Expected workflow:
1. inspect
2. explain
3. make small changes
4. build/test
5. commit
6. stop

Agents should not:
- rewrite large systems
- create speculative abstractions
- continue into future milestones automatically

`AGENTS.md` contains strict workflow rules.

---

# Current Status

The project currently has:
- mmap-backed cache access
- safe Buffer implementation
- sector-chain cache reading
- gzip/bzip2 decompression
- JAG archive parsing
- definition parsers
- experimental SDL3/OpenGL client
- model loading groundwork

The architecture is still evolving and should remain flexible.

---

# Near-Term Goals

Current priorities:
1. stabilize cache systems
2. stabilize archive/parsing systems
3. improve model loading
4. improve map loading
5. clean renderer bootstrap
6. render cache-driven geometry
7. build debugging/visualization tools

---

# Long-Term Goals

Potential future goals:
- full map renderer
- textured terrain
- animation system
- scene graph
- software renderer experiments
- cache editing tools
- modernized RS317 renderer
- custom asset pipeline

These are future possibilities, not immediate tasks.

---

# Important Philosophy

The project should remain understandable.

If a system becomes:
- too magical
- too abstract
- difficult to debug
- difficult to reason about

then the design should be reconsidered.

Understanding the system is more important than adding features quickly.