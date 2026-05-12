#pragma once

#include <cstdint>
#include <vector>

#include "Buffer.h"

struct ModelVertex {
    int x = 0;
    int y = 0;
    int z = 0;
};

struct ModelTriangle {
    int a = 0;
    int b = 0;
    int c = 0;
    int color = 0;
    int renderType = 0;
    int priority = -1;
    int alpha = -1;
    int skin = -1;
};

struct ModelTextureTriangle {
    int a = 0;
    int b = 0;
    int c = 0;
};

struct ModelBounds {
    int minX = 0;
    int maxX = 0;
    int minY = 0;
    int maxY = 0;
    int minZ = 0;
    int maxZ = 0;
};

// 317 model geometry decoded from a raw idx1 file.
// Parsed using Buffer, matching the convention of other cache parsers.
struct Model {
    std::vector<ModelVertex> vertices;
    std::vector<ModelTriangle> triangles;
    std::vector<ModelTextureTriangle> textureTriangles;
    std::vector<int> vertexSkins;

    ModelBounds bounds() const;

    // Parse one model from decompressed idx1 bytes.
    // Throws std::runtime_error on malformed data.
    static Model parse(Buffer& buf);
};
