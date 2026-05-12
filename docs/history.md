# Project History

This is a learning journal. It records what was built, when, and why.

## 2024-05 — Foundation

- Cache reading: `.dat` + `.idx` sector chain following
- JAG sub-archive parsing with BZIP2 decompression
- GZIP decompression for map/model files
- `Buffer` universal byte reader

## 2024-05 — Definition Parsers

Implemented strict opcode-driven parsers for:
- `ItemDef`, `NpcDef`, `LocDef`, `FloDef`, `IdkDef`
- `MesAnimDef`, `MesDef`, `ParamDef`
- `SeqDef`, `SpotAnimDef`, `VarbitDef`, `VarpDef`

All throw on unknown opcodes.

## 2024-05 — Map Decoding

- `MapTerrain`: opcode-per-tile terrain parsing
- `MapObjects`: unsigned-smart delta-encoded object placements
- `MapRegion`: connects versionlist → terrain + object files
- `RegionRenderer2D`: PPM output with terrain colors and object footprints

## 2024-05 — Model Decoding (v1)

- `Model::parse()` with 18-byte footer layout
- Vertex deltas via signed-smart
- Triangle index opcodes (1-4) for strip compression
- HSL face colors

## 2024-05 — Client Renderer (v1, deleted)

- OpenGL 2.1 immediate mode
- RS 317 HSL→RGB palette (65,536 entries)
- Per-vertex normal accumulation
- `mixLightness` lighting
- Wireframe + filled triangle rendering
- Camera: orbit (mouse drag), zoom (wheel), pan (arrows)

Deleted because color scheme was wrong and code had too many workarounds.

## 2024-05 — Fresh Start

- Deleted all model/render code
- Moved rendering code intention from `client/` to `core/` (shared)
- Rewriting model decoder and renderer from scratch, step by step
- Split `WORKFLOW.md` into `AGENTS.md` + `docs/` reference files
