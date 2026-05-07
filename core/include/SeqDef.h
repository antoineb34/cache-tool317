#pragma once

#include <vector>

#include "Buffer.h"

struct SeqDef {
    int id = -1;

    std::vector<int> frameIds;
    std::vector<int> frameIdHigh;
    std::vector<int> frameLengths;

    int replayOffset = -1;
    std::vector<int> interleaveOrder;
    bool stretches = false;
    int priority = 5;
    int mainHandItem = -1;
    int offHandItem = -1;
    int maxLoops = 99;
    int animatingPrecedence = -1;
    int walkingPrecedence = -1;
    int replayMode = 2;

    static SeqDef parse(Buffer& buf);
};
