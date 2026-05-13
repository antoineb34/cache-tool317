#include "WorldRenderer.h"

#include <SDL3/SDL_opengl.h>

#include "MapRegion.h"
#include "MapTerrain.h"
#include "MapObjects.h"
#include "LocDef.h"
#include "DefinitionsLoader.h"
#include "TextureManager.h"
#include "Math.h"

#include <iostream>
#include <cstring>
#include <algorithm>

// --- TerrainChunk ---

void TerrainChunk::init(const std::vector<TerrainVertex>& verts, const std::vector<uint32_t>& indices) {
    indexCount = static_cast<int>(indices.size());

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(TerrainVertex), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // Position: 3 floats
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, x));

    // Color: 3 unsigned bytes, normalized
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(TerrainVertex), (void*)offsetof(TerrainVertex, r));

    glBindVertexArray(0);
}

void TerrainChunk::draw() const {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void TerrainChunk::cleanup() {
    if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
    if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (ebo) { glDeleteBuffers(1, &ebo); ebo = 0; }
    indexCount = 0;
}

// --- ObjectBatch ---

void ObjectBatch::init(const std::vector<ObjectVertex>& verts, const std::vector<uint32_t>& indices) {
    indexCount = static_cast<int>(indices.size());

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(ObjectVertex), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // Position: 3 floats
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ObjectVertex), (void*)offsetof(ObjectVertex, x));

    // Color: 3 unsigned bytes, normalized
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ObjectVertex), (void*)offsetof(ObjectVertex, r));

    // TexCoord: 2 floats
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ObjectVertex), (void*)offsetof(ObjectVertex, u));

    glBindVertexArray(0);
}

void ObjectBatch::draw() const {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void ObjectBatch::cleanup() {
    if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
    if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (ebo) { glDeleteBuffers(1, &ebo); ebo = 0; }
    indexCount = 0;
}

// --- WorldRenderer ---

WorldRenderer::~WorldRenderer() {
    cleanup();
}

void WorldRenderer::cleanup() {
    for (auto& chunk : terrainChunks_) chunk.cleanup();
    for (auto& batch : objectBatches_) batch.cleanup();
    terrainChunks_.clear();
    objectBatches_.clear();
}

// Compute face normal for terrain shading
void WorldRenderer::computeNormal(const TerrainVertex& a, const TerrainVertex& b, const TerrainVertex& c,
                                   float& nx, float& ny, float& nz) {
    float abx = b.x - a.x, aby = b.y - a.y, abz = b.z - a.z;
    float acx = c.x - a.x, acy = c.y - a.y, acz = c.z - a.z;
    nx = aby * acz - abz * acy;
    ny = abz * acx - abx * acz;
    nz = abx * acy - aby * acx;
    float len = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (len > 0.001f) {
        nx /= len; ny /= len; nz /= len;
    }
}

uint32_t WorldRenderer::hslToRgb(uint16_t hsl) {
    return ::hslToRgb(hsl);
}

uint32_t WorldRenderer::applyLightness(uint16_t hsl, int lightness) {
    return ::applyLightness(hsl, lightness);
}

bool WorldRenderer::loadRegion(const MapRegion& region, const DefinitionsLoader& defs, TextureManager* texMgr) {
    cleanup();
    texMgr_ = texMgr;

    const MapTerrain& terrain = region.terrain();

    // --- Build terrain mesh ---
    // We tile the 64x64 grid with two triangles per tile
    // Lighting: use the RS317 light vector
    const float lightDirX = -30.0f / 65.0f;
    const float lightDirY = -50.0f / 65.0f;
    const float lightDirZ = -30.0f / 65.0f;
    const int lightMod = 64;

    for (int plane = 0; plane < 1; plane++) { // Just render plane 0 for now
        for (int x = 0; x < 63; x++) {
            for (int y = 0; y < 63; y++) {
                const Tile& t00 = terrain.getTile(plane, x, y);
                const Tile& t10 = terrain.getTile(plane, x + 1, y);
                const Tile& t01 = terrain.getTile(plane, x, y + 1);
                const Tile& t11 = terrain.getTile(plane, x + 1, y + 1);

                // World coordinates (RS317: +X east, +Y up, +Z south -> we flip Z)
                float x0 = static_cast<float>(x * 128);
                float x1 = static_cast<float>((x + 1) * 128);
                float z0 = static_cast<float>(-y * 128);
                float z1 = static_cast<float>(-(y + 1) * 128);

                // Heights (RS317 stores as negative, we flip)
                float h00 = static_cast<float>(-t00.height);
                float h10 = static_cast<float>(-t10.height);
                float h01 = static_cast<float>(-t01.height);
                float h11 = static_cast<float>(-t11.height);

                // Floor color from FloDef
                uint16_t baseHsl = 0;
                try {
                    const FloDef& flo = defs.getFlo(t00.underlayId > 0 ? t00.underlayId - 1 : 0);
                    baseHsl = static_cast<uint16_t>(flo.rgb);
                } catch (...) {
                    baseHsl = 0x1B52; // Default green
                }

                // Compute lighting using tile normal
                TerrainVertex v00, v10, v01, v11;

                auto setVert = [&](TerrainVertex& v, float px, float py, float pz, float h) {
                    v.x = px; v.y = h; v.z = pz;
                    // Compute normal by cross product of tile edges
                    float nx = (h00 - h10) * 0.01f;
                    float ny = 1.0f;
                    float nz = (h00 - h01) * 0.01f;
                    float len = std::sqrt(nx * nx + ny * ny + nz * nz);
                    if (len > 0.001f) { nx /= len; ny /= len; nz /= len; }

                    float dotNL = nx * lightDirX + ny * lightDirY + nz * lightDirZ;
                    int lightness = static_cast<int>(lightMod + dotNL * 256.0f);
                    lightness = std::max(32, std::min(255, lightness));

                    uint16_t litHsl = applyLightness(baseHsl, lightness);
                    uint32_t rgb = hslToRgb(litHsl);
                    v.r = static_cast<uint8_t>((rgb >> 16) & 0xFF);
                    v.g = static_cast<uint8_t>((rgb >> 8) & 0xFF);
                    v.b = static_cast<uint8_t>(rgb & 0xFF);
                };

                setVert(v00, x0, h00, z0, h00);
                setVert(v10, x1, h10, z0, h10);
                setVert(v01, x0, h01, z1, h01);
                setVert(v11, x1, h11, z1, h11);

                // Two triangles: (0,0)-(1,0)-(0,1) and (1,0)-(1,1)-(0,1)
                TerrainChunk chunk;
                std::vector<TerrainVertex> verts = {v00, v10, v01, v11};
                std::vector<uint32_t> indices = {0, 1, 2, 1, 3, 2};
                chunk.init(verts, indices);
                terrainChunks_.push_back(std::move(chunk));
            }
        }
    }

    // --- Build object meshes ---
    for (const MapObject& obj : region.objects().getObjects()) {
        try {
            const LocDef& loc = defs.getLoc(obj.id);

            // Skip if no models
            if (loc.modelIDs.empty()) continue;

            // Use the first model for the object
            int modelId = loc.modelIDs[0];

            // Get object color from loc def
            uint32_t objColor = 0x888888;
            if (!loc.colorSrc.empty() && !loc.colorDst.empty()) {
                objColor = 0xAAAAAA;
            }

            // Create a simple box mesh for the object (placeholder)
            // In a full implementation, we'd load the actual model
            float w = static_cast<float>(loc.width * 128);
            float l = static_cast<float>(loc.length * 128);
            float h = 64.0f;

            float px = static_cast<float>(obj.x * 128);
            float py = static_cast<float>(obj.y * 128);
            float pz = static_cast<float>(-obj.z * 240); // approximate height

            // Simple colored box
            ObjectBatch batch;
            std::vector<ObjectVertex> verts;
            std::vector<uint32_t> indices;

            // Bottom face
            verts.push_back({px, pz, py, 128, 128, 128, 0, 0});
            verts.push_back({px + w, pz, py, 128, 128, 128, 1, 0});
            verts.push_back({px + w, pz, py + l, 128, 128, 128, 1, 1});
            verts.push_back({px, pz, py + l, 128, 128, 128, 0, 1});

            // Top face
            verts.push_back({px, pz + h, py, 200, 200, 200, 0, 0});
            verts.push_back({px + w, pz + h, py, 200, 200, 200, 1, 0});
            verts.push_back({px + w, pz + h, py + l, 200, 200, 200, 1, 1});
            verts.push_back({px, pz + h, py + l, 200, 200, 200, 0, 1});

            // Indices for 6 faces (2 triangles each)
            uint32_t base = 0;
            // Bottom
            indices.insert(indices.end(), {base + 0, base + 2, base + 1, base + 0, base + 3, base + 2});
            // Top
            indices.insert(indices.end(), {base + 4, base + 5, base + 6, base + 4, base + 6, base + 7});
            // Front
            indices.insert(indices.end(), {base + 0, base + 1, base + 5, base + 0, base + 5, base + 4});
            // Back
            indices.insert(indices.end(), {base + 2, base + 3, base + 7, base + 2, base + 7, base + 6});
            // Left
            indices.insert(indices.end(), {base + 0, base + 4, base + 7, base + 0, base + 7, base + 3});
            // Right
            indices.insert(indices.end(), {base + 1, base + 2, base + 6, base + 1, base + 6, base + 5});

            batch.init(verts, indices);
            objectBatches_.push_back(std::move(batch));

        } catch (const std::exception& e) {
            // Skip objects that fail to load
        }
    }

    std::cout << "WorldRenderer: " << terrainChunks_.size() << " terrain chunks, "
              << objectBatches_.size() << " object batches" << std::endl;

    return !terrainChunks_.empty();
}

void WorldRenderer::draw(const Mat4f& mvp) const {
    // Draw terrain
    for (const auto& chunk : terrainChunks_) {
        chunk.draw();
    }

    // Draw objects
    for (const auto& batch : objectBatches_) {
        if (batch.textureId >= 0 && texMgr_) {
            texMgr_->bind(batch.textureId);
        }
        batch.draw();
    }
}