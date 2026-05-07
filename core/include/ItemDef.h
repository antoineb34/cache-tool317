#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "Buffer.h"

struct ItemDef {
    // identity
    int         id            = -1;
    std::string name          = "null";
    std::string description   = "";

    // 2D icon rendering
    int         modelId       = 0;
    int         zoom2d        = 2000;
    int         xan2d         = 0;     // x rotation
    int         yan2d         = 0;     // y rotation
    int         xof2d         = 0;     // x offset
    int         yof2d         = 0;     // y offset

    // properties
    bool        stackable     = false;
    int         value         = 1;
    bool        members       = false;

    // options shown on ground and in inventory (5 slots each)
    std::array<std::string, 5> groundOptions    = {"", "", "Take", "", ""};
    std::array<std::string, 5> inventoryOptions = {"", "", "", "", "Drop"};

    // noted item links (set if this is a noted version of another item)
    int         notedId       = -1;   // the real item this is noted from
    int         notedTemplate = -1;   // the note paper template item

    // colour recoloring (original colour → replacement colour pairs)
    std::vector<uint16_t> originalColors;
    std::vector<uint16_t> replacementColors;

    // parses one ItemDef from a buffer positioned at the start of an item entry
    static ItemDef parse(Buffer& buf);
};
