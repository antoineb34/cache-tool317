#pragma once

#include "Buffer.h"

struct VarbitDef {
    int id = -1;

    // Varbits read a bit range from one backing varp value.
    int varpId = -1;
    int leastSignificantBit = 0;
    int mostSignificantBit = 0;

    static VarbitDef parse(Buffer& buf);
};
