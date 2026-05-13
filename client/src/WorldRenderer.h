#pragma once

#include <vector>
#include <SDL3/SDL_opengl.h>

#include "Buffer.h"

struct MapRegion;
struct MapTerrain;
struct MapObject;
struct Tile;
struct DefinitionsLoader;
struct TextureManager;
struct Mat4f;

struct TerrainVertex {
    float x, y, z;
    uint8_t r, g, b;
};

struct ObjectVertex {
    float x, y, z;
    uint8_t r, g, b;
    float u, v;
};

struct TerrainChunk {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int indexCount = 0;

    void init(const std::vector<TerrainVertex>& verts, const std::vector<uint32_t>& indices);
    void draw() const;
    void cleanup();
};

struct ObjectBatch {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int indexCount = 0;
    int textureId = -1;

    void init(const std::vector<ObjectVertex>& verts, const std::vector<uint32_t>& indices);
    void draw() const;
    void cleanup();
};

class WorldRenderer {
public:
    WorldRenderer() = default;
    ~WorldRenderer();

    bool loadRegion(const MapRegion& region, const DefinitionsLoader& defs, TextureManager* texMgr);
    void draw(const Mat4f& mvp) const;
    void cleanup();

    int getTerrainChunks() const { return static_cast<int>(terrainChunks_.size()); }
    int getObjectBatches() const { return static_cast<int>(objectBatches_.size()); }

private:
    std::vector<TerrainChunk> terrainChunks_;
    std::vector<ObjectBatch> objectBatches_;
    TextureManager* texMgr_ = nullptr;

    static uint32_t hslToRgb(uint16_t hsl);
    static uint32_t applyLightness(uint16_t hsl, int lightness);
    static void computeNormal(const TerrainVertex& a, const TerrainVertex& b, const TerrainVertex& c,
                              float& nx, float& ny, float& nz);
};