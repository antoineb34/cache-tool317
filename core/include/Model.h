#pragma once

#include <cstdint>
#include <vector>

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

class Model {
public:
    static Model parse(const std::vector<uint8_t>& data);

    const std::vector<ModelVertex>& vertices() const;
    const std::vector<ModelTriangle>& triangles() const;
    const std::vector<ModelTextureTriangle>& textureTriangles() const;
    const std::vector<int>& vertexSkins() const;
    ModelBounds bounds() const;

private:
    std::vector<ModelVertex> vertices_;
    std::vector<ModelTriangle> triangles_;
    std::vector<ModelTextureTriangle> textureTriangles_;
    std::vector<int> vertexSkins_;
};
