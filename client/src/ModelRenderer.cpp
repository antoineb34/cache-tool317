#include "ModelRenderer.h"

#include <SDL3/SDL_opengl.h>

#include "ModelDef.h"
#include "TextureDef.h"
#include "TextureManager.h"
#include "CacheReader.h"
#include "Math.h"

#include <iostream>
#include <algorithm>
#include <cstring>

// --- RenderBatch ---

void RenderBatch::init(const std::vector<ModelVertex>& verts, const std::vector<uint32_t>& indices) {
    indexCount = static_cast<int>(indices.size());

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(ModelVertex), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // Position: 3 floats
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, x));

    // Color: 3 unsigned bytes, normalized
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, r));

    // TexCoord: 2 floats
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, u));

    // HasTexture: 1 unsigned byte
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, hasTexture));

    glBindVertexArray(0);
}

void RenderBatch::draw() const {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void RenderBatch::cleanup() {
    if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
    if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (ebo) { glDeleteBuffers(1, &ebo); ebo = 0; }
    indexCount = 0;
}

// --- ModelRenderer ---

ModelRenderer::~ModelRenderer() {
    cleanup();
}

void ModelRenderer::cleanup() {
    for (auto& batch : opaqueBatches_) batch.cleanup();
    for (auto& batch : transparentBatches_) batch.cleanup();
    opaqueBatches_.clear();
    transparentBatches_.clear();
}

bool ModelRenderer::loadModel(CacheReader& reader, int modelId, const TextureManager* texMgr) {
    if (!reader.hasFile(1, modelId)) {
        std::cerr << "Model " << modelId << " not found in cache" << std::endl;
        return false;
    }
    Buffer buf = reader.readGzippedFile(1, modelId);
    if (buf.empty()) return false;
    ModelDef model = ModelDef::parse(modelId, buf);
    return loadModel(model, texMgr);
}

bool ModelRenderer::loadModel(const ModelDef& model, const TextureManager* texMgr) {
    cleanup();
    modelId_ = model.id;
    texMgr_ = texMgr;
    buildBatches(model);
    return !opaqueBatches_.empty() || !transparentBatches_.empty();
}

void ModelRenderer::buildBatches(const ModelDef& model) {
    // Group faces by render characteristics:
    // - textured vs untextured
    // - transparent vs opaque
    // - same texture ID (for textured faces)

    struct FaceGroup {
        bool textured;
        bool transparent;
        int texId;
        uint16_t hslColor;
        std::vector<uint32_t> indices;  // into the global vertex array
    };

    // Build a global vertex list and face groups
    std::vector<ModelVertex> globalVerts;
    std::vector<FaceGroup> groups;

    // Pre-compute vertex normals for smooth shading
    // (Simplified: we use flat shading per-face for now)
    auto computeNormal = [&](int triIdx) -> Vec3f {
        int a = model.triA[triIdx];
        int b = model.triB[triIdx];
        int c = model.triC[triIdx];
        Vec3f va(model.vertexX[a], -model.vertexY[a], -model.vertexZ[a]);
        Vec3f vb(model.vertexX[b], -model.vertexY[b], -model.vertexZ[b]);
        Vec3f vc(model.vertexX[c], -model.vertexY[c], -model.vertexZ[c]);
        Vec3f ab = vb - va;
        Vec3f ac = vc - va;
        return ab.cross(ac).normalized();
    };

    // Light direction (RS317 uses light vector (-30, -50, -30), normalized)
    const Vec3f lightDir = Vec3f(-30, -50, -30).normalized();
    const int lightMod = 64;
    const float lightMagInv = 1.0f / 65.0f;

    for (int triIdx = 0; triIdx < static_cast<int>(model.triA.size()); triIdx++) {
        int a = model.triA[triIdx];
        int b = model.triB[triIdx];
        int c = model.triC[triIdx];

        // Compute face normal for lighting
        Vec3f normal = computeNormal(triIdx);
        float dotNL = normal.dot(lightDir);
        int lightness = static_cast<int>((lightMod + dotNL * 256.0f));
        lightness = std::max(0, std::min(255, lightness));

        bool isTextured = model.isFaceTextured(triIdx);
        bool isTransparent = model.isFaceTransparent(triIdx);
        int texTriIdx = model.faceTexTriIndex(triIdx);
        int texId = -1;

        if (isTextured && texTriIdx < static_cast<int>(model.texP.size())) {
            // The texture triangle indices point to vertex indices
            // For now, we use the texture ID from the texture triangle
            // In the real game, texP/Q/R are vertex indices into the model's vertex arrays
            // that define the texture coordinate mapping
            texId = texTriIdx; // Simplified: use tex tri index as texture ID
        }

        // Determine HSL color and apply lighting
        uint16_t baseColor = model.triColor[triIdx];
        int litColor = lightness;

        // Apply lighting to HSL color
        int finalHSL = applyLightness(baseColor, lightness);
        uint32_t rgb = hslToRgb(static_cast<uint16_t>(finalHSL));

        // Find or create a face group
        int groupIdx = -1;
        for (size_t i = 0; i < groups.size(); i++) {
            if (groups[i].textured == isTextured &&
                groups[i].transparent == isTransparent &&
                groups[i].texId == texId &&
                groups[i].hslColor == static_cast<uint16_t>(finalHSL)) {
                groupIdx = static_cast<int>(i);
                break;
            }
        }

        if (groupIdx == -1) {
            FaceGroup fg;
            fg.textured = isTextured;
            fg.transparent = isTransparent;
            fg.texId = texId;
            fg.hslColor = static_cast<uint16_t>(finalHSL);
            groupIdx = static_cast<int>(groups.size());
            groups.push_back(fg);
        }

        // Add vertices
        int vertBase = static_cast<int>(globalVerts.size());
        for (int vi = 0; vi < 3; vi++) {
            int vIdx = (vi == 0) ? a : (vi == 1) ? b : c;
            ModelVertex vert;
            vert.x = static_cast<float>(model.vertexX[vIdx]);
            vert.y = static_cast<float>(-model.vertexY[vIdx]); // flip Y
            vert.z = static_cast<float>(-model.vertexZ[vIdx]); // flip Z

            // Color from lit HSL
            vert.r = static_cast<uint8_t>((rgb >> 16) & 0xFF);
            vert.g = static_cast<uint8_t>((rgb >> 8) & 0xFF);
            vert.b = static_cast<uint8_t>(rgb & 0xFF);

            // Texture coordinates (simplified UV mapping)
            if (isTextured && texTriIdx < static_cast<int>(model.texP.size())) {
                // Use texP/Q/R vertex indices to derive UVs
                // The tex vertices define a triangle in model space that maps to the texture
                int tp = model.texP[texTriIdx];
                int tq = model.texQ[texTriIdx];
                int tr = model.texR[texTriIdx];

                // Simple planar UV based on vertex position relative to texture triangle
                Vec3f vp(model.vertexX[tp], -model.vertexY[tp], -model.vertexZ[tp]);
                Vec3f vq(model.vertexX[tq], -model.vertexY[tq], -model.vertexZ[tq]);
                Vec3f vr(model.vertexX[tr], -model.vertexY[tr], -model.vertexZ[tr]);

                Vec3f pv = Vec3f(vert.x, vert.y, vert.z) - vp;
                Vec3f edge1 = vq - vp;
                Vec3f edge2 = vr - vp;

                // Project onto the two longest edges of the texture triangle
                float e1Len = edge1.length();
                float e2Len = edge2.length();

                float u = 0.5f, v = 0.5f;
                if (e1Len > 0.001f && e2Len > 0.001f) {
                    u = pv.dot(edge1.normalized()) / e1Len;
                    v = pv.dot(edge2.normalized()) / e2Len;
                    u = std::max(0.0f, std::min(1.0f, u));
                    v = std::max(0.0f, std::min(1.0f, v));
                }

                vert.u = u;
                vert.v = v;
                vert.hasTexture = 1;
            } else {
                vert.u = 0.0f;
                vert.v = 0.0f;
                vert.hasTexture = 0;
            }

            globalVerts.push_back(vert);
        }

        // Add indices (triangle fan from vertBase)
        groups[groupIdx].indices.push_back(vertBase);
        groups[groupIdx].indices.push_back(vertBase + 1);
        groups[groupIdx].indices.push_back(vertBase + 2);
    }

    // Create RenderBatches from face groups
    for (auto& fg : groups) {
        RenderBatch batch;
        batch.textureId = fg.texId;
        batch.transparent = fg.transparent;
        batch.hslColor = fg.hslColor;
        batch.init(globalVerts, fg.indices);

        if (fg.transparent) {
            transparentBatches_.push_back(std::move(batch));
        } else {
            opaqueBatches_.push_back(std::move(batch));
        }
    }
}

void ModelRenderer::draw(const Mat4f& mvp, bool drawTransparent) const {
    for (const auto& batch : opaqueBatches_) {
        if (drawTransparent) continue; // Skip opaque when drawing transparent pass

        if (batch.textureId >= 0 && texMgr_) {
            texMgr_->bind(batch.textureId);
        }
        batch.draw();
    }
}

void ModelRenderer::drawTransparent(const Mat4f& mvp) const {
    // Draw transparent batches back-to-front (simplified: just draw them)
    for (const auto& batch : transparentBatches_) {
        if (batch.textureId >= 0 && texMgr_) {
            texMgr_->bind(batch.textureId);
        }
        batch.draw();
    }
}