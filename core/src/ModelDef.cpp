#include "ModelDef.h"
#include "Buffer.h"
#include <stdexcept>
#include <string>

ModelDef ModelDef::parse(int id, Buffer& buf) {
    int dataSize = buf.size();
    if (dataSize < 18)
        throw std::runtime_error("ModelDef " + std::to_string(id) + ": data too small");

    ModelDef def;
    def.id = id;

    // --- read footer (last 18 bytes) ---
    buf.seek(dataSize - 18);

    auto readU16 = [&]() -> uint16_t {
        return buf.readUShort();
    };

    int     vertexCount       = readU16();
    int     triangleCount     = readU16();
    int     texTriCount       = buf.readByte();
    bool    hasFaceRenderTypes = buf.readByte() == 1;
    uint8_t priorityFlag      = buf.readByte();
    bool    hasFaceAlpha      = buf.readByte() == 1;
    bool    hasFaceSkins      = buf.readByte() == 1;
    bool    hasVertexSkins    = buf.readByte() == 1;
    int     vertexXLen        = readU16();
    int     vertexYLen        = readU16();
    int     vertexZLen        = readU16();
    int     triIndexLen       = readU16();

    // Validate total size
    int expectedSize = vertexCount + triangleCount
        + (priorityFlag == 255 ? triangleCount : 0)
        + (hasFaceSkins ? triangleCount : 0)
        + (hasFaceRenderTypes ? triangleCount : 0)
        + (hasVertexSkins ? vertexCount : 0)
        + (hasFaceAlpha ? triangleCount : 0)
        + triIndexLen
        + triangleCount * 2
        + texTriCount * 6
        + vertexXLen + vertexYLen + vertexZLen
        + 18;  // footer

    if (dataSize < expectedSize) {
        throw std::runtime_error("ModelDef " + std::to_string(id) + ": data too small for declared sizes");
    }

    // --- compute block offsets (sequential from byte 0) ---
    int offset = 0;
    auto nextBlock = [&](int size) {
        int blockStart = offset;
        offset += size;
        return blockStart;
    };

    int vertexFlagsOff    = nextBlock(vertexCount);
    int triOpcodesOff    = nextBlock(triangleCount);
    int facePriorityOff  = nextBlock(priorityFlag == 255 ? triangleCount : 0);
    int faceSkinOff      = nextBlock(hasFaceSkins ? triangleCount : 0);
    int faceRenderTypeOff= nextBlock(hasFaceRenderTypes ? triangleCount : 0);
    int vertexSkinOff    = nextBlock(hasVertexSkins ? vertexCount : 0);
    int faceAlphaOff     = nextBlock(hasFaceAlpha ? triangleCount : 0);
    int triIndexOff      = nextBlock(triIndexLen);
    int faceColorOff     = nextBlock(triangleCount * 2);
    int texTriOff        = nextBlock(texTriCount * 6);
    int vertexXOff       = nextBlock(vertexXLen);
    int vertexYOff       = nextBlock(vertexYLen);
    int vertexZOff       = nextBlock(vertexZLen);

    // --- Helper to seek to a block ---
    auto seekToBlock = [&](int start, int size) {
        buf.seek(start);
    };

    // --- decode vertices ---
    seekToBlock(vertexFlagsOff, vertexCount);
    seekToBlock(vertexXOff, vertexXLen);
    seekToBlock(vertexYOff, vertexYLen);
    seekToBlock(vertexZOff, vertexZLen);

    def.vertexX.resize(vertexCount);
    def.vertexY.resize(vertexCount);
    def.vertexZ.resize(vertexCount);

    // Re-seek to read vertex flags from the start
    buf.seek(vertexFlagsOff);
    std::vector<uint8_t> vertexFlags(vertexCount);
    for (int i = 0; i < vertexCount; i++) vertexFlags[i] = buf.readByte();

    Buffer xBuf(buf.slice(vertexXOff, vertexXOff + vertexXLen));
    Buffer yBuf(buf.slice(vertexYOff, vertexYOff + vertexYLen));
    Buffer zBuf(buf.slice(vertexZOff, vertexZOff + vertexZLen));

    int x = 0;
    int y = 0;
    int z = 0;

    for (int i = 0; i < vertexCount; i++) {

        int flags = vertexFlags[i];

        if (flags & 1)
            x += xBuf.readSignedSmart();

        if (flags & 2)
            y += yBuf.readSignedSmart();

        if (flags & 4)
            z += zBuf.readSignedSmart();

        def.vertexX[i] = x;
        def.vertexY[i] = y;
        def.vertexZ[i] = z;
    }

    if (hasVertexSkins) {
        buf.seek(vertexSkinOff);
        def.vertexSkin.resize(vertexCount);
        for (int i = 0; i < vertexCount; i++)
            def.vertexSkin[i] = buf.readByte();
    }

    // --- decode triangle indices ---
    buf.seek(triOpcodesOff);
    std::vector<uint8_t> opcodes(triangleCount);
    for (int i = 0; i < triangleCount; i++) opcodes[i] = buf.readByte();

    buf.seek(triIndexOff);

    def.triA.resize(triangleCount);
    def.triB.resize(triangleCount);
    def.triC.resize(triangleCount);

    int a = 0, b = 0, c = 0, last = 0;
    for (int i = 0; i < triangleCount; i++) {
        int op = opcodes[i];
        if (op == 1) {
            a = buf.readSignedSmart() + last; last = a;
            b = buf.readSignedSmart() + last; last = b;
            c = buf.readSignedSmart() + last; last = c;
        } else if (op == 2) {
            b = c;
            c = buf.readSignedSmart() + last; last = c;
        } else if (op == 3) {
            a = c;
            c = buf.readSignedSmart() + last; last = c;
        } else if (op == 4) {
            std::swap(a, b);
            c = buf.readSignedSmart() + last; last = c;
        } else {
            throw std::runtime_error("ModelDef " + std::to_string(id)
                + ": unknown triangle opcode " + std::to_string(op));
        }
        def.triA[i] = (uint16_t)a;
        def.triB[i] = (uint16_t)b;
        def.triC[i] = (uint16_t)c;
    }

    // --- face colors ---
    buf.seek(faceColorOff);
    def.triColor.resize(triangleCount);
    for (int i = 0; i < triangleCount; i++)
        def.triColor[i] = buf.readUShort();

    // --- optional per-face data ---
    if (hasFaceRenderTypes) {
        buf.seek(faceRenderTypeOff);
        def.triRenderType.resize(triangleCount);
        for (int i = 0; i < triangleCount; i++)
            def.triRenderType[i] = buf.readByte();
    }

    if (priorityFlag == 255) {
        buf.seek(facePriorityOff);
        def.triPriority.resize(triangleCount);
        for (int i = 0; i < triangleCount; i++)
            def.triPriority[i] = buf.readByte();
    } else {
        def.sharedPriority = priorityFlag;
    }

    if (hasFaceAlpha) {
        buf.seek(faceAlphaOff);
        def.triAlpha.resize(triangleCount);
        for (int i = 0; i < triangleCount; i++)
            def.triAlpha[i] = buf.readByte();
    }

    if (hasFaceSkins) {
        buf.seek(faceSkinOff);
        def.triSkin.resize(triangleCount);
        for (int i = 0; i < triangleCount; i++)
            def.triSkin[i] = buf.readByte();
    }

    // --- texture triangles ---
    if (texTriCount > 0) {
        buf.seek(texTriOff);
        def.texP.resize(texTriCount);
        def.texQ.resize(texTriCount);
        def.texR.resize(texTriCount);
        for (int i = 0; i < texTriCount; i++) {
            def.texP[i] = buf.readUShort();
            def.texQ[i] = buf.readUShort();
            def.texR[i] = buf.readUShort();
        }
    }

    return def;
}
