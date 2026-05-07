#pragma once

#include <array>
#include <string>
#include <vector>

#include "Buffer.h"

struct NpcDef {
    // identity
    int         id      = -1;
    std::string name    = "null";
    std::string examine = "";

    // models
    std::vector<int> modelIDs;
    std::vector<int> headModelIDs;

    // animations
    int seqStandID      = -1;
    int seqWalkID       = -1;
    int seqTurnAroundID = -1;
    int seqTurnLeftID   = -1;
    int seqTurnRightID  = -1;

    // properties
    int  size           = 1;     // tiles occupied (1 = 1x1, 2 = 2x2, etc.)
    int  level          = -1;    // combat level (-1 = non-combat)
    int  turnSpeed      = 32;
    int  headicon       = -1;
    bool showOnMinimap  = true;
    bool interactable   = true;
    bool important      = false; // renders on top of other npcs

    // rendering
    int  scaleXY         = 128;
    int  scaleZ          = 128;
    int  lightAmbient    = 0;
    int  lightAttenuation = 0;

    // right-click options (up to 5)
    std::array<std::string, 5> options = {"", "", "", "", ""};

    // colour recoloring
    std::vector<int> colorSrc;
    std::vector<int> colorDst;

    // npc transform/override (morphing npcs e.g. shapeshifters)
    int              varbitID  = -1;
    int              varpID    = -1;
    std::vector<int> overrides;

    // parses one NpcDef from a buffer positioned at the start of an npc entry
    static NpcDef parse(Buffer& buf);
};
