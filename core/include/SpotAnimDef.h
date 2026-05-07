#pragma once

#include <array>

#include "Buffer.h"

struct SpotAnimDef {
    int id = -1;

    int modelId = -1;
    int seqId = -1;
    int scaleXY = 128;
    int scaleZ = 128;
    int rotation = 0;
    int ambient = 0;
    int contrast = 0;

    std::array<int, 6> recolorSrc = {-1, -1, -1, -1, -1, -1};
    std::array<int, 6> recolorDst = {-1, -1, -1, -1, -1, -1};

    static SpotAnimDef parse(Buffer& buf);
};
