#pragma once

#include <string>

#include "Buffer.h"

struct ParamDef {
    int id = -1;

    char type = 0;
    int defaultInt = 0;
    std::string defaultString;
    bool autoDisable = true;

    static ParamDef parse(Buffer& buf);
};
