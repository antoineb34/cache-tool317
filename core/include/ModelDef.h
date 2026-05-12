#pragma once

#include <cstdint>
#include <vector>

struct ModelDef {
    int id = -1;

    // vertices (parallel arrays, index-aligned)
    std::vector<int> vertexX;
    std::vector<int> vertexY;
    std::vector<int> vertexZ;
    std::vector<int> vertexSkin;   // empty if not present

    // triangles (parallel arrays, index-aligned)
    std::vector<uint16_t> triA;
    std::vector<uint16_t> triB;
    std::vector<uint16_t> triC;
    std::vector<uint16_t> triColor;      // packed HSL: bits 15-10=hue, 9-7=sat, 6-0=light
    std::vector<uint8_t>  triRenderType; // empty if not present
    std::vector<uint8_t>  triPriority;   // empty means all faces use sharedPriority
    std::vector<uint8_t>  triAlpha;      // empty means all faces are fully opaque
    std::vector<int>      triSkin;       // empty if not present
    uint8_t sharedPriority = 0;

    // texture triangles
    std::vector<uint16_t> texP;
    std::vector<uint16_t> texQ;
    std::vector<uint16_t> texR;

    // helpers for decoding the packed faceRenderType byte:
    //   bit 0 = semi-transparent, bit 1 = textured, bits 7-2 = texture triangle index
    bool isFaceTextured(int i)     const { return !triRenderType.empty() && (triRenderType[i] & 2); }
    bool isFaceTransparent(int i)  const { return !triRenderType.empty() && (triRenderType[i] & 1); }
    int  faceTexTriIndex(int i)    const { return !triRenderType.empty() ? (triRenderType[i] >> 2) : 0; }

    static ModelDef parse(int id, const std::vector<uint8_t>& data);
};
