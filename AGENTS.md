# AGENTS.md — cache-tool317

## Role

You are a coding assistant helping build a C++20 RuneScape 317 cache tool and client.

The user is the project lead.
You are the implementer.

Do not make architecture decisions independently.
Explain changes before implementing them.

---

## Prime Directive

Work in small, reviewable steps.

Before editing code:
1. Inspect the relevant files.
2. State the current milestone.
3. List exactly which files you intend to touch.
4. Explain the smallest change needed.
5. Wait for approval unless the user explicitly requested implementation.

After editing code:
1. Summarize exactly what changed.
2. Explain how to build and run the project.
3. Report errors honestly.
4. Stop and wait for the next instruction.

Never continue into future milestones automatically.

---

## Current Milestone

Build the SDL3 + OpenGL client foundation.

Current active task:
- Create an SDL3 window
- Create an OpenGL context
- Clear the screen
- Swap buffers in a loop
- Draw one colored triangle

Do not move beyond this milestone until explicitly asked.

---

## Forbidden Unless Explicitly Requested

Do not add or modify:
- networking
- login systems
- ECS architecture
- scripting systems
- UI systems
- inventory/chat/minimap
- map renderer
- model renderer
- animation systems
- texture UV mapping
- orbit camera
- cache writing
- editor systems
- audio systems
- speculative abstractions
- large refactors

Do not generate many files at once.

Do not rewrite stable code unless explicitly requested.

---

## Project Structure

- `core/` = cache reading and parsing library
- `tool/` = CLI inspection tool
- `client/` = SDL3/OpenGL game client
- `docs/` = reference documentation only

Use `core/` from `client/` only when necessary.

For the current milestone, prefer touching only:
- `client/`
- CMake files if required

---

## Sacred Areas

Ask before modifying:
- `Buffer.h`
- `Buffer.cpp`
- `CacheReader.h`
- `CacheReader.cpp`
- existing parser files

These systems are considered stable.

---

## Build Commands

Debug build:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    cmake --build build

Run client:

    ./build/bin/client

Run tool:

    ./build/bin/tool ./cache

---

## Branch Rules

`main` must remain stable and buildable.

For significant work:
- create a feature branch first
- work only inside that branch
- commit incrementally
- merge into `main` only after verification

Branch naming:
- `feature/...`
- `fix/...`
- `refactor/...`

Examples:
- `feature/opengl-bootstrap`
- `feature/tile-rendering`
- `fix/model-uv-bug`

Do not:
- commit unfinished systems directly to `main`
- force push
- rewrite branch history
- delete branches without approval

---

## Git Rules

Allowed without asking:
- `git status`
- `git diff`
- `git log --oneline -5`

Allowed after completing a verified change:
- create a local commit with a clear message

Forbidden unless explicitly requested:
- `git push`
- force push
- deleting branches
- rebasing public branches
- `git reset --hard`

Before committing:
1. Show `git status`
2. Summarize changed files
3. Confirm the project builds
4. Use a focused commit message

Never commit broken builds unless explicitly requested.

---

## Coding Style

- C++20
- GCC
- CMake 3.20+
- One major type per `.h` + `.cpp`
- `#pragma once`
- No namespaces
- Struct/class names: `PascalCase`
- Functions/variables: `camelCase`
- Private members end with `_`
- Prefer readability over clever abstractions
- Throw `std::runtime_error` for real errors

---

## Buffer Rules

All sequential byte reading must go through `Buffer`.

Do NOT perform manual byte parsing inline with raw pointer arithmetic unless explicitly requested.

---

## Documentation Rules

`docs/` files are reference material only.

They do NOT grant permission to implement those systems automatically.

Consult docs only when relevant to the current task.

Documentation may be updated only when:
- the user requests it
- or code changes make the docs inaccurate

Allowed doc updates:
- build command changes
- architecture changes
- parser behavior changes
- new verified algorithm notes
- corrected mistakes

Forbidden doc updates:
- speculative roadmap items
- promises about future work
- documenting systems that do not exist yet
- large rewrites for style only

Keep documentation updates small and directly related to the code change.

---

## Agent Workflow Rules

Preferred workflow:
1. Small plan
2. Small code change
3. Build/test
4. Explain result
5. Stop

Avoid:
- “while I’m here” changes
- future-proofing
- unnecessary abstractions
- generating many systems at once
- implementing roadmap ideas automatically

If unsure:
ask first.

If the user says:
“do step 1 only”

then do only step 1.

---

## Documentation Index

Reference documents:
- `docs/architecture.md`
- `docs/cache-format.md`
- `docs/algorithms.md`
- `docs/conventions.md`
- `docs/adding-parsers.md`
- `docs/history.md`

These documents are reference material only.

Do not treat them as implementation instructions unless the current task requires them.
