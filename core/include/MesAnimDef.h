#pragma once

#include "Buffer.h"

struct MesAnimDef {
    int id = -1;

    static MesAnimDef parse(Buffer& buf);
};
