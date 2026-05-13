#pragma once

#include <vector>
#include <SDL3/SDL_opengl.h>

#include "Buffer.h"

struct Archive;
struct CacheReader;
struct Mat4f;
struct ModelDef;
struct TextureManager;

struct ModelVertex {
    float x, y, z;
    uint8_t r, g, b;
    float u, v;
    uint8_t hasTexture;
};

struct RenderBatch {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int indexCount = 0;
    int textureId = -1;
    bool transparent = false;
    uint16_t hslColor = 0;

    void init(const std::vector<ModelVertex>& verts, const std::vector<uint32_t>& indices);
    void draw() const;
    void cleanup();
};

class ModelRenderer {
public:
    ModelRenderer() = default;
    ~ModelRenderer();

    bool loadModel(const ModelDef& model, const TextureManager* texMgr);
    bool loadModel(CacheReader& reader, int modelId, const TextureManager* texMgr);

    void draw(const Mat4f& mvp, bool drawTransparent = false) const;
    void drawTransparent(const Mat4f& mvp) const;
    void cleanup();

    int getModelId() const { return modelId_; }
    int getBatchCount() const { return static_cast<int>(opaqueBatches_.size()) + static_cast<int>(transparentBatches_.size()); }

private:
    int modelId_ = -1;
    std::vector<RenderBatch> opaqueBatches_;
    std::vector<RenderBatch> transparentBatches_;
    const TextureManager* texMgr_ = nullptr;

    void buildBatches(const ModelDef& model);
    static uint32_t packColor(uint8_t r, uint8_t g, uint8_t b);
};