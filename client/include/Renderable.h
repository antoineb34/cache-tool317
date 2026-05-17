#pragma once

#include "Mesh.h"
#include "Mat4.h"

struct Renderable {
    const Mesh* mesh;
    Mat4 transform;
};
