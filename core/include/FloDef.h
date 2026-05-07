#pragma once

#include <string>

#include "Buffer.h"

struct FloDef {
    int id = -1;

    int rgb = -1;
    int textureId = -1;
    bool occlude = true;
    std::string name;
    int secondaryRgb = -1;

    static FloDef parse(Buffer& buf);
};
