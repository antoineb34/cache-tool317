#pragma once

#include <array>
#include <vector>

#include "Buffer.h"

struct IdkDef {
    int id = -1;

    int bodyPartId = -1;
    bool selectable = true;

    std::vector<int> modelIds;
    std::array<int, 10> recolorSrc = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    std::array<int, 10> recolorDst = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    std::array<int, 5> headModelIds = {-1, -1, -1, -1, -1};

    static IdkDef parse(Buffer& buf);
};
