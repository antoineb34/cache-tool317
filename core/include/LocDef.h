#pragma once

#include <array>
#include <string>
#include <vector>

#include "Buffer.h"

struct LocDef {
    // identity
    int         id          = -1;
    std::string name        = "null";
    std::string description = "";

    // models
    std::vector<int> modelIDs;
    std::vector<int> modelTypes;

    // map placement
    int width  = 1;
    int length = 1;

    // behavior/collision
    bool blocksWalk       = true;
    bool blocksProjectiles = true;
    bool hasActions       = false;
    bool contouredGround  = false;
    bool nonFlatShading   = false;
    bool modelClipped     = false;
    bool castsShadow      = true;
    bool obstructsGround  = false;
    bool hollow           = false;
    int  supportItems     = -1;

    // animation/rendering
    int animationID       = -1;
    int decorDisplacement = 16;
    int ambient           = 0;
    int contrast          = 0;
    bool rotated          = false;
    int scaleX            = 128;
    int scaleY            = 128;
    int scaleZ            = 128;
    int offsetX           = 0;
    int offsetY           = 0;
    int offsetZ           = 0;

    // minimap
    int mapSceneID    = -1;
    int mapFunctionID = -1;
    int surroundings  = 0;

    // right-click options
    std::array<std::string, 5> options = {"", "", "", "", ""};

    // colour recoloring
    std::vector<int> colorSrc;
    std::vector<int> colorDst;

    // object transform/override system
    int              varbitID = -1;
    int              varpID   = -1;
    std::vector<int> overrides;

    // parses one LocDef from a buffer positioned at the start of a loc entry
    static LocDef parse(Buffer& buf);
};
