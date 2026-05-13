#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include "CacheReader.h"
#include "ModelDef.h"

#include <vector>
#include <unordered_set>
#include <cstdint>
#include <cmath>
struct Vec3f {
    float x = 0, y = 0, z = 0;
    Vec3f() = default;
    Vec3f(float x, float y, float z) : x(x), y(y), z(z) {}
    Vec3f operator+(const Vec3f& v) const { return {x+v.x, y+v.y, z+v.z}; }
    Vec3f operator-(const Vec3f& v) const { return {x-v.x, y-v.y, z-v.z}; }
    Vec3f operator*(float s) const { return {x*s, y*s, z*s}; }
    Vec3f operator/(float s) const { return {x/s, y/s, z/s}; }
    float dot(const Vec3f& v) const { return x*v.x + y*v.y + z*v.z; }
    Vec3f cross(const Vec3f& v) const { return {y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x}; }
    float length() const { return std::sqrt(x*x + y*y + z*z); }
    Vec3f normalized() const { float l = length(); return l > 0 ? (*this / l) : Vec3f{}; }
};

struct Mat4f {
    float m[16] = {0};
    static Mat4f identity();
    static Mat4f perspective(float fovY, float aspect, float nearZ, float farZ);
    static Mat4f lookAt(const Vec3f& eye, const Vec3f& center, const Vec3f& up);
    static Mat4f rotateY(float deg);
    Mat4f operator*(const Mat4f& o) const;
};

// Per-vertex data for the wireframe GPU buffer.
struct WireVert {
    float x, y, z;
    uint8_t r, g, b;
};

// DebugModelViewer — loads one model, validates it, renders wireframe.
class DebugModelViewer {
public:
    DebugModelViewer();
    ~DebugModelViewer();

    // Non-copyable
    DebugModelViewer(const DebugModelViewer&) = delete;
    DebugModelViewer& operator=(const DebugModelViewer&) = delete;

    // Load a model from the cache via existing pipeline.
    // Returns false if model not found, parse/validation fails.
    bool load(CacheReader& reader, int modelId);

    // Reload a different model using the same CacheReader.
    bool reloadModel(CacheReader& reader, int modelId);

    // Toggle wireframe drawing mode.
    void toggleWireframe();

    // Toggle backface culling.
    void toggleCulling();

    // Reset rotation to default.
    void resetView();

    // Upload current verts/edges to GPU. Call after GL context is live.
    bool initGL();

    // Advance rotation and render wireframe.
    void update();
    void render(int windowWidth, int windowHeight);

    // Metadata logged at load time
    int loadedModelId() const { return modelId_; }
    int vertexCount() const { return (int)verts_.size(); }
    int triangleCount() const { return (int)triIndices_.size() / 3; }
    int texturedFaceCount() const { return texFaceCount_; }

private:
    int modelId_ = -1;
    int texFaceCount_ = 0;

    std::vector<WireVert> verts_;
    std::vector<uint32_t> triIndices_;   // flat [a,b,c, ...]
    std::vector<std::pair<uint32_t,uint32_t>> edges_;  // deduplicated wireframe edges

    bool wireframe_ = true;
    bool culling_ = true;

    // GL state
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    GLuint prog_ = 0;
    int mvpLoc_ = -1;
    int idxCount_ = 0;
    float rotationY_ = 0.0f;

    // Internal helpers
    bool validateIndices(const ModelDef& def);
    bool buildFromDef(const ModelDef& def);
    static uint32_t hslToRgb(uint16_t hsl);
    void compileShaders();
};

// Global pointer used by App to reach the viewer (avoids complex dependency wiring).
extern DebugModelViewer* g_viewer;