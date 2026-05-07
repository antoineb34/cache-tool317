#pragma once

#include "Buffer.h"

struct VarpDef {
    int id = -1;

    // Opcode 5 value used by the client to identify config behavior.
    int configType = 0;

    static VarpDef parse(Buffer& buf);
};
