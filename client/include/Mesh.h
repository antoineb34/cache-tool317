#pragma once

#include <vector>
#include "Vertex.h"
#include "ModelDef.h"

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

    static Mesh FromModelDef(const ModelDef& def);
};
