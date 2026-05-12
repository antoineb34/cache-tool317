# RuneScape 317 Cache Format Reference

## Overview

The cache consists of a single `.dat` data file and five `.idx` index files.

```
main_file_cache.dat    — all raw data, divided into 520-byte sectors
main_file_cache.idx0   — index for archive 0 (definitions / JAG sub-archives)
main_file_cache.idx1   — index for archive 1 (models)
main_file_cache.idx2   — index for archive 2 (animations)
main_file_cache.idx3   — index for archive 3 (midis / sounds)
main_file_cache.idx4   — index for archive 4 (maps)
```

## Index Entry (6 bytes)

Each file in an archive has a 6-byte entry in its `.idx` file at offset `fileId * 6`.

```
0-2: size         (3 bytes, big-endian) — total file size in bytes
3-5: firstSector  (3 bytes, big-endian) — first sector number in .dat
```

Special values:
- `size == 0` or `firstSector == 0` → file does not exist
- `size == 0xFFFFFF` or `firstSector == 0xFFFFFF` → file does not exist

## Sector (520 bytes)

Files span one or more sectors chained together.

```
0-1:   fileId     (2 bytes, big-endian) — which file this sector belongs to
2-3:   chunk      (2 bytes, big-endian) — chunk index within the file
4-6:   nextSector (3 bytes, big-endian) — next sector in chain, 0 = end
7:     archiveId  (1 byte) — which .idx this sector belongs to
8-519: data       (512 bytes) — actual file data
```

The last sector may be partially filled. Only copy `min(512, remaining)` bytes.

## Reading a File

```
1. Read index entry from .idx
2. Follow sector chain starting at firstSector
3. Copy data bytes from each sector
4. Stop when we have 'size' bytes or nextSector is 0
```

## Archive 0 — JAG Sub-Archives

Files in archive 0 are themselves packed JAG archives. Each has this structure:

```
0-2:   decompressedSize (3 bytes)
3-5:   compressedSize   (3 bytes)
         if equal → not whole-archive compressed
         if different → BZIP2 compressed (strip BZh1 header before decompressing)
6-7:   numEntries       (2 bytes)

For each entry (10 bytes):
    0-3:   nameHash         (4 bytes) — hash of sub-file name
    4-6:   decompressedSize (3 bytes)
    7-9:   compressedSize   (3 bytes)
         if equal → raw data
         if different → BZIP2 compressed individually

Then: raw data for each entry, sequentially
```

### Name Hash Algorithm

```cpp
uint32_t hash = 0;
for (char c : name)
    hash = (hash * 61 + std::toupper(c)) - 32;
```

### Archive 0 File Map

| File ID | Name | Contents |
|---------|------|----------|
| 1 | title | fonts, logo, background, buttons, runes, index.dat |
| 2 | definitions | obj, npc, loc, flo, idk, seq, spotanim, varp, varbit, mesanim, mes, param |
| 3 | interface | UI definitions |
| 4 | media | 2D graphics / sprites |
| 5 | versionlist | update list (map_index) |
| 6 | textures | textures |
| 7 | wordenc | chat filter |
| 8 | sounds | sound effects |

## Compression

### Jagex BZIP2

Standard BZIP2 **without** the `BZh1` header. Prepend `BZh1` before decompressing with libbzip2.

### GZIP

Some files (maps, models in some caches) are raw GZIP. Decompress with zlib (`inflateInit2` with `15 + 16` window bits for auto-detection).

## Archive 4 — Map Files

Maps are stored in archive 4 (`.idx4`). Each region has two files:

```
terrainFileId  = map_index[regionId].terrainFileId
objectFileId   = map_index[regionId].objectFileId
```

Both are GZIP-compressed.

### Terrain File

Decompressed terrain data is a sequence of opcodes per tile, plane by plane (0-3), row by row (y=0..63), column by column (x=0..63).

Opcodes:
```
0      end tile, generated/default height
1      explicit height, followed by 1 height byte, then end tile
       height = (heightByte == 1) ? 0 : heightByte
2..49  overlay shape/rotation opcode, followed by 1 overlay id byte
       path = (opcode - 2) / 4, rotation = (opcode - 2) & 3
50..81 tile settings = opcode - 49
82+    underlay id = opcode - 81
```

### Object File

Decompressed object data is a sequence of object placements.

```
For each object type:
    idDelta = readUnsignedSmart()
    objectId += idDelta
    if idDelta == 0: break (end of this type's placements)
    if idDelta == 0 and objectId == 0: break (end of file)

    For each placement of this object:
        positionDelta = readUnsignedSmart()
        position += positionDelta
        if positionDelta == 0: break (end of this object's placements)

        attributes = readByte()
        type = attributes >> 2
        rotation = attributes & 3
        plane = position >> 12
        x = (position >> 6) & 0x3F
        y = position & 0x3F
```

## Model Format (Archive 1)

Models in archive 1 are typically GZIP-compressed. The decompressed data has this structure:

### Footer (last 18 bytes)

```
0-1:   vertexCount            (ushort)
2-3:   triangleCount          (ushort)
4:     textureTriangleCount   (byte)
5:     hasFaceRenderTypes     (byte) — 1 if each face has a render type
6:     priorityFlag           (byte) — 255 if each face has its own priority, else shared value
7:     hasFaceAlpha           (byte) — 1 if each face has alpha
8:     hasFaceSkins           (byte) — 1 if each face has a skin group
9:     hasVertexSkins         (byte) — 1 if each vertex has a skin group
10-11: vertexXDataLength      (ushort)
12-13: vertexYDataLength      (ushort)
14-15: vertexZDataLength      (ushort)
16-17: triangleIndexDataLength (ushort)
```

### Data Blocks (sequential from byte 0)

```
vertexFlags:       vertexCount bytes
                   bit 0: has X delta, bit 1: has Y delta, bit 2: has Z delta

triangleOpcodes:   triangleCount bytes
                   1 = read 3 new indices
                   2 = reuse b as a, read new c
                   3 = reuse a as b, read new c
                   4 = swap a,b, read new c

facePriority:      triangleCount bytes (if priorityFlag == 255)

faceSkin:          triangleCount bytes (if hasFaceSkins == 1)

faceRenderType:    triangleCount bytes (if hasFaceRenderTypes == 1)
                   bit 0: transparency flag (1 = semi-transparent face)
                   bit 1: texture flag      (1 = textured face, uses a texture triangle)
                   bits 7-2: texture triangle index (renderType >> 2), only valid if bit 1 set
                   examples: 0=flat opaque, 1=flat transparent,
                             2=textured opaque (texIdx=0), 6=textured opaque (texIdx=1)

vertexSkin:        vertexCount bytes (if hasVertexSkins == 1)

faceAlpha:         triangleCount bytes (if hasFaceAlpha == 1)

triangleIndexData: triangleIndexDataLength bytes
                   signed smart integers, interpreted per opcode

faceColor:         triangleCount * 2 bytes (ushort per face)
                   packed HSL: bits 15-10 = hue(0-63), 9-7 = sat(0-7), 6-0 = light(0-127)

textureTriangle:   textureTriangleCount * 6 bytes (3 ushorts each)

vertexXData:       vertexXDataLength bytes (signed smart deltas)
vertexYData:       vertexYDataLength bytes (signed smart deltas)
vertexZData:       vertexZDataLength bytes (signed smart deltas)
```

### Vertex Decoding

```
x = 0, y = 0, z = 0
for each vertex i:
    flags = vertexFlags[i]
    if flags & 1: x += readSignedSmart(vertexXData)
    if flags & 2: y += readSignedSmart(vertexYData)
    if flags & 4: z += readSignedSmart(vertexZData)
    vertex[i] = (x, y, z)
```

### Triangle Index Decoding

```
a = 0, b = 0, c = 0, last = 0
for each triangle i:
    opcode = triangleOpcodes[i]
    if opcode == 1:
        a = readSignedSmart() + last; last = a
        b = readSignedSmart() + last; last = b
        c = readSignedSmart() + last; last = c
    else if opcode == 2:
        b = c
        c = readSignedSmart() + last; last = c
    else if opcode == 3:
        a = c
        c = readSignedSmart() + last; last = c
    else if opcode == 4:
        swap(a, b)
        c = readSignedSmart() + last; last = c
    triangle[i] = (a, b, c)
```

### Signed Smart

```
byte = readByte()
if byte < 128:
    return byte - 64                    // 1 byte, range -64..+63
else:
    next = readByte()
    return ((byte - 128) << 8 | next) - 16384   // 2 bytes, range -16384..+16383
```
