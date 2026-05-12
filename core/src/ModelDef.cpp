#include "ModelDef.h"
#include "Buffer.h"
#include <stdexcept>
#include <string>

ModelDef ModelDef::parse(int id, const std::vector<uint8_t>& data) {
    if ((int)data.size() < 18)
        throw std::runtime_error("ModelDef " + std::to_string(id) + ": data too small");

    ModelDef def;
    def.id = id;

    // --- read footer (last 18 bytes) ---
    const uint8_t* f = data.data() + data.size() - 18;
    auto ru16 = [](const uint8_t* p, int off) -> int {
        return (p[off] << 8) | p[off + 1];
    };

    int     vertexCount       = ru16(f,  0);
    int     triangleCount     = ru16(f,  2);
    int     texTriCount       = f[4];
    bool    hasFaceRenderTypes = f[5] == 1;
    uint8_t priorityFlag      = f[6];
    bool    hasFaceAlpha      = f[7] == 1;
    bool    hasFaceSkins      = f[8] == 1;
    bool    hasVertexSkins    = f[9] == 1;
    int     vertexXLen        = ru16(f, 10);
    int     vertexYLen        = ru16(f, 12);
    int     vertexZLen        = ru16(f, 14);
    int     triIndexLen       = ru16(f, 16);

    // --- compute block offsets (sequential from byte 0) ---
    int offset = 0;
    int vertexFlagsOff    = offset; offset += vertexCount;
    int triOpcodesOff     = offset; offset += triangleCount;
    int facePriorityOff   = offset; if (priorityFlag == 255) offset += triangleCount;
    int faceSkinOff       = offset; if (hasFaceSkins)        offset += triangleCount;
    int faceRenderTypeOff = offset; if (hasFaceRenderTypes)  offset += triangleCount;
    int vertexSkinOff     = offset; if (hasVertexSkins)      offset += vertexCount;
    int faceAlphaOff      = offset; if (hasFaceAlpha)        offset += triangleCount;
    int triIndexOff       = offset; offset += triIndexLen;
    int faceColorOff      = offset; offset += triangleCount * 2;
    int texTriOff         = offset; offset += texTriCount * 6;
    int vertexXOff        = offset; offset += vertexXLen;
    int vertexYOff        = offset; offset += vertexYLen;
    int vertexZOff        = offset;

    const uint8_t* raw = data.data();

    // --- decode vertices ---
    Buffer vertexFlags(raw + vertexFlagsOff, vertexCount);
    Buffer xBuf(raw + vertexXOff, vertexXLen);
    Buffer yBuf(raw + vertexYOff, vertexYLen);
    Buffer zBuf(raw + vertexZOff, vertexZLen);

    def.vertexX.resize(vertexCount);
    def.vertexY.resize(vertexCount);
    def.vertexZ.resize(vertexCount);

    int x = 0, y = 0, z = 0;
    for (int i = 0; i < vertexCount; i++) {
        int flags = vertexFlags.readByte();
        if (flags & 1) x += xBuf.readSignedSmart();
        if (flags & 2) y += yBuf.readSignedSmart();
        if (flags & 4) z += zBuf.readSignedSmart();
        def.vertexX[i] = x;
        def.vertexY[i] = y;
        def.vertexZ[i] = z;
    }

    if (hasVertexSkins) {
        Buffer vSkinBuf(raw + vertexSkinOff, vertexCount);
        def.vertexSkin.resize(vertexCount);
        for (int i = 0; i < vertexCount; i++)
            def.vertexSkin[i] = vSkinBuf.readByte();
    }

    // --- decode triangle indices ---
    Buffer opcodes(raw + triOpcodesOff, triangleCount);
    Buffer triIdx(raw + triIndexOff, triIndexLen);

    def.triA.resize(triangleCount);
    def.triB.resize(triangleCount);
    def.triC.resize(triangleCount);

    int a = 0, b = 0, c = 0, last = 0;
    for (int i = 0; i < triangleCount; i++) {
        int op = opcodes.readByte();
        if (op == 1) {
            a = triIdx.readSignedSmart() + last; last = a;
            b = triIdx.readSignedSmart() + last; last = b;
            c = triIdx.readSignedSmart() + last; last = c;
        } else if (op == 2) {
            b = c;
            c = triIdx.readSignedSmart() + last; last = c;
        } else if (op == 3) {
            a = c;
            c = triIdx.readSignedSmart() + last; last = c;
        } else if (op == 4) {
            std::swap(a, b);
            c = triIdx.readSignedSmart() + last; last = c;
        } else {
            throw std::runtime_error("ModelDef " + std::to_string(id)
                + ": unknown triangle opcode " + std::to_string(op));
        }
        def.triA[i] = (uint16_t)a;
        def.triB[i] = (uint16_t)b;
        def.triC[i] = (uint16_t)c;
    }

    // --- face colors ---
    Buffer colorBuf(raw + faceColorOff, triangleCount * 2);
    def.triColor.resize(triangleCount);
    for (int i = 0; i < triangleCount; i++)
        def.triColor[i] = colorBuf.readUShort();

    // --- optional per-face data ---
    if (hasFaceRenderTypes) {
        Buffer renderBuf(raw + faceRenderTypeOff, triangleCount);
        def.triRenderType.resize(triangleCount);
        for (int i = 0; i < triangleCount; i++)
            def.triRenderType[i] = renderBuf.readByte();
    }

    if (priorityFlag == 255) {
        Buffer priBuf(raw + facePriorityOff, triangleCount);
        def.triPriority.resize(triangleCount);
        for (int i = 0; i < triangleCount; i++)
            def.triPriority[i] = priBuf.readByte();
    } else {
        def.sharedPriority = priorityFlag;
    }

    if (hasFaceAlpha) {
        Buffer alphaBuf(raw + faceAlphaOff, triangleCount);
        def.triAlpha.resize(triangleCount);
        for (int i = 0; i < triangleCount; i++)
            def.triAlpha[i] = alphaBuf.readByte();
    }

    if (hasFaceSkins) {
        Buffer fSkinBuf(raw + faceSkinOff, triangleCount);
        def.triSkin.resize(triangleCount);
        for (int i = 0; i < triangleCount; i++)
            def.triSkin[i] = fSkinBuf.readByte();
    }

    // --- texture triangles ---
    if (texTriCount > 0) {
        Buffer texBuf(raw + texTriOff, texTriCount * 6);
        def.texP.resize(texTriCount);
        def.texQ.resize(texTriCount);
        def.texR.resize(texTriCount);
        for (int i = 0; i < texTriCount; i++) {
            def.texP[i] = texBuf.readUShort();
            def.texQ[i] = texBuf.readUShort();
            def.texR[i] = texBuf.readUShort();
        }
    }

    return def;
}
