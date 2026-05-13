#include "DebugModelViewer.h"

#include <iostream>
#include <algorithm>

// ---- Math helpers (could be moved to a shared math header later) ----

Mat4f Mat4f::identity() {
    Mat4f r;
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

Mat4f Mat4f::perspective(float fovY, float aspect, float nearZ, float farZ) {
    Mat4f r;
    float f = 1.0f / std::tan(fovY * 0.5f * 3.14159265f / 180.0f);
    float nf = 1.0f / (nearZ - farZ);
    r.m[0]  = f / aspect;
    r.m[5]  = f;
    r.m[10] = (farZ + nearZ) * nf;
    r.m[11] = -1.0f;
    r.m[14] = 2.0f * farZ * nearZ * nf;
    return r;
}

Mat4f Mat4f::lookAt(const Vec3f& eye, const Vec3f& center, const Vec3f& up) {
    Mat4f r;
    Vec3f f = (center - eye).normalized();
    Vec3f s = f.cross(up).normalized();
    Vec3f u = s.cross(f);
    r.m[0]=s.x; r.m[4]=s.y; r.m[8]=s.z;  r.m[12]=-s.dot(eye);
    r.m[1]=u.x; r.m[5]=u.y; r.m[9]=u.z;  r.m[13]=-u.dot(eye);
    r.m[2]=-f.x;r.m[6]=-f.y;r.m[10]=-f.z; r.m[14]=f.dot(eye);
    r.m[15]=1.0f;
    return r;
}

Mat4f Mat4f::rotateY(float deg) {
    Mat4f r = identity();
    float rad = deg * 3.14159265f / 180.0f;
    float c = std::cos(rad), s = std::sin(rad);
    r.m[0] = c;  r.m[2] = s;
    r.m[8] = -s; r.m[10] = c;
    return r;
}

Mat4f Mat4f::operator*(const Mat4f& o) const {
    Mat4f r;
    for (int row = 0; row < 4; row++)
        for (int col = 0; col < 4; col++) {
            float sum = 0;
            for (int k = 0; k < 4; k++)
                sum += m[k*4+row] * o.m[col*4+k];
            r.m[col*4+row] = sum;
        }
    return r;
}

// ---- HSL→RGB (RS317 packed format: H6 S3 L7) ----

uint32_t DebugModelViewer::hslToRgb(uint16_t hsl) {
    int hue = (hsl >> 10) & 0x3F;
    int sat = (hsl >>  7) & 0x07;
    int lit =  hsl        & 0x7F;

    float h = (hue / 64.0f) + 0.0078125f;
    float s = (sat /  8.0f) + 0.0625f;
    float l =  lit / 128.0f;

    float r, g, b;
    if (s == 0.0f) {
        r = g = b = l;
    } else {
        auto hueToRgb = [](float h, float a, float p) -> float {
            if (h < 0.0f) h += 1.0f;
            if (h > 1.0f) h -= 1.0f;
            if (6.0f * h < 1.0f) return p + (a - p) * 6.0f * h;
            if (2.0f * h < 1.0f) return a;
            if (3.0f * h < 2.0f) return p + (a - p) * ((2.0f/3.0f) - h) * 6.0f;
            return p;
        };
        float a, p;
        if (l < 0.5f) {
            a = l * (1.0f + s);
        } else {
            a = l + s - l * s;
        }
        p = 2.0f * l - a;
        r = hueToRgb(h + 1.0f/3.0f, a, p);
        g = hueToRgb(h, a, p);
        b = hueToRgb(h - 1.0f/3.0f, a, p);
    }

    int ri = std::max(0, std::min(255, (int)(r * 256.0f)));
    int gi = std::max(0, std::min(255, (int)(g * 256.0f)));
    int bi = std::max(0, std::min(255, (int)(b * 256.0f)));

    return (ri << 16) | (gi << 8) | bi;
}

// ---- Validation ----

bool DebugModelViewer::validateIndices(const ModelDef& def) {
    int vcount = (int)def.vertexX.size();
    for (size_t i = 0; i < def.triA.size(); i++) {
        if (def.triA[i] < 0 || def.triA[i] >= vcount ||
            def.triB[i] < 0 || def.triB[i] >= vcount ||
            def.triC[i] < 0 || def.triC[i] >= vcount)
        {
            std::cerr << "ERROR: triangle " << i
                      << " index out of bounds (vcount=" << vcount
                      << ") a=" << def.triA[i]
                      << " b=" << def.triB[i]
                      << " c=" << def.triC[i] << std::endl;
            return false;
        }
    }
    return true;
}

// ---- Build from parsed ModelDef ----

bool DebugModelViewer::buildFromDef(const ModelDef& def) {
    modelId_ = def.id;
    texFaceCount_ = 0;

    if (!validateIndices(def)) return false;

    // Flatten triangle indices
    triIndices_.clear();
    triIndices_.reserve(def.triA.size() * 3);
    for (size_t i = 0; i < def.triA.size(); i++) {
        triIndices_.push_back((uint32_t)def.triA[i]);
        triIndices_.push_back((uint32_t)def.triB[i]);
        triIndices_.push_back((uint32_t)def.triC[i]);
    }

    // Build vertices, colouring from first adjacent face
    verts_.clear();
    verts_.reserve(def.vertexX.size());
    std::vector<bool> coloured(def.vertexX.size(), false);

    for (size_t i = 0; i < def.vertexX.size(); i++) {
        WireVert v;
        v.x = (float)def.vertexX[i];
        v.y = -(float)def.vertexY[i];  // flip Y (RS317 → OpenGL)
        v.z = -(float)def.vertexZ[i];  // flip Z
        v.r = v.g = v.b = 180;         // default grey
        verts_.push_back(v);
    }

    for (size_t i = 0; i < def.triA.size(); i++) {
        uint16_t hsl = def.triColor[i];
        uint32_t rgb = hslToRgb(hsl);
        uint8_t cr = (rgb >> 16) & 0xFF;
        uint8_t cg = (rgb >>  8) & 0xFF;
        uint8_t cb =  rgb        & 0xFF;

        int idx[3] = { def.triA[i], def.triB[i], def.triC[i] };
        for (int j = 0; j < 3; j++) {
            if (idx[j] >= 0 && idx[j] < (int)def.vertexX.size() && !coloured[idx[j]]) {
                verts_[idx[j]].r = cr;
                verts_[idx[j]].g = cg;
                verts_[idx[j]].b = cb;
                coloured[idx[j]] = true;
            }
        }

        if (!def.triRenderType.empty() && (def.triRenderType[i] & 2))
            texFaceCount_++;
    }

    // Extract unique edges
    struct PairHash {
        size_t operator()(const std::pair<uint32_t,uint32_t>& p) const {
            return ((size_t)p.first << 16) | p.second;
        }
    };
    std::unordered_set<std::pair<uint32_t,uint32_t>, PairHash> edgeSet;
    for (size_t i = 0; i < def.triA.size(); i++) {
        uint32_t a = def.triA[i], b = def.triB[i], c = def.triC[i];
        auto me = [](uint32_t x, uint32_t y) {
            return x < y ? std::make_pair(x, y) : std::make_pair(y, x);
        };
        edgeSet.insert(me(a, b));
        edgeSet.insert(me(b, c));
        edgeSet.insert(me(c, a));
    }
    edges_.assign(edgeSet.begin(), edgeSet.end());

    std::cout << "DebugModelViewer: model=" << modelId_
              << " verts=" << verts_.size()
              << " tris=" << def.triA.size()
              << " tex=" << texFaceCount_
              << " edges=" << edges_.size()
              << std::endl;

    // Compute and log bounding box
    if (!verts_.empty()) {
        float minX = verts_[0].x, maxX = verts_[0].x;
        float minY = verts_[0].y, maxY = verts_[0].y;
        float minZ = verts_[0].z, maxZ = verts_[0].z;
        for (const auto& v : verts_) {
            if (v.x < minX) minX = v.x; if (v.x > maxX) maxX = v.x;
            if (v.y < minY) minY = v.y; if (v.y > maxY) maxY = v.y;
            if (v.z < minZ) minZ = v.z; if (v.z > maxZ) maxZ = v.z;
        }
        std::cout << "  Bounding box: X[" << minX << "," << maxX
                  << "] Y[" << minY << "," << maxY
                  << "] Z[" << minZ << "," << maxZ << "]" << std::endl;
    }

    return true;
}

// ---- Public API ----

DebugModelViewer::DebugModelViewer() = default;
DebugModelViewer::~DebugModelViewer() {
    if (ebo_) glDeleteBuffers(1, &ebo_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (prog_) glDeleteProgram(prog_);
}

bool DebugModelViewer::load(CacheReader& reader, int modelId) {
    if (!reader.hasFile(1, modelId)) {
        std::cerr << "Model " << modelId << " not in archive 1" << std::endl;
        return false;
    }
    Buffer buf = reader.readGzippedFile(1, modelId);
    if (buf.empty()) {
        std::cerr << "Empty buffer for model " << modelId << std::endl;
        return false;
    }
    try {
        ModelDef def = ModelDef::parse(modelId, buf);
        bool ok = buildFromDef(def);
        if (!ok) {
            std::cerr << "Model " << modelId << ": validation failed" << std::endl;
        }
        return ok;
    } catch (const std::exception& e) {
        std::cerr << "Model " << modelId << ": parse error: " << e.what() << std::endl;
        return false;
    }
}

bool DebugModelViewer::reloadModel(CacheReader& reader, int modelId) {
    // Destroy old GL objects so we can rebuild with new geometry
    if (ebo_) { glDeleteBuffers(1, &ebo_); ebo_ = 0; }
    if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
    if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
    idxCount_ = 0;

    bool ok = load(reader, modelId);
    if (ok && prog_) initGL();  // re-upload to GPU
    return ok;
}

void DebugModelViewer::toggleWireframe() {
    wireframe_ = !wireframe_;
    std::cout << "Wireframe " << (wireframe_ ? "ON" : "OFF") << std::endl;
}

void DebugModelViewer::toggleCulling() {
    culling_ = !culling_;
    std::cout << "Backface culling " << (culling_ ? "ON" : "OFF") << std::endl;
}

void DebugModelViewer::resetView() {
    rotationY_ = 0.0f;
    std::cout << "View reset" << std::endl;
}

// ---- Shader compilation ----

static GLuint sCompileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::cerr << "Shader error: " << log << std::endl;
        glDeleteShader(s);
        return 0;
    }
    return s;
}

void DebugModelViewer::compileShaders() {
    const char* vs = R"GLSL(
#version 330 core
layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aCol;
uniform mat4 uMVP;
out vec3 vCol;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vCol = aCol;
}
)GLSL";

    const char* fs = R"GLSL(
#version 330 core
in vec3 vCol;
out vec4 outColor;
void main() {
    outColor = vec4(vCol, 1.0);
}
)GLSL";

    GLuint vs_ = sCompileShader(GL_VERTEX_SHADER, vs);
    GLuint fs_ = sCompileShader(GL_FRAGMENT_SHADER, fs);
    if (!vs_ || !fs_) { prog_ = 0; return; }

    prog_ = glCreateProgram();
    glAttachShader(prog_, vs_);
    glAttachShader(prog_, fs_);
    glLinkProgram(prog_);
    glDeleteShader(vs_);
    glDeleteShader(fs_);

    GLint ok;
    glGetProgramiv(prog_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog_, sizeof(log), nullptr, log);
        std::cerr << "Link error: " << log << std::endl;
        glDeleteProgram(prog_);
        prog_ = 0;
        return;
    }
    mvpLoc_ = glGetUniformLocation(prog_, "uMVP");
}

bool DebugModelViewer::initGL() {
    compileShaders();
    if (!prog_) return false;

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 verts_.size() * sizeof(WireVert),
                 verts_.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 edges_.size() * 2 * sizeof(uint32_t),
                 edges_.data(),  GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(WireVert), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(WireVert), (void*)offsetof(WireVert, r));

    glBindVertexArray(0);

    idxCount_ = (int)(edges_.size() * 2);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "GL init error: 0x" << std::hex << err << std::endl;
        return false;
    }

    std::cout << "DebugModelViewer: GL buffers uploaded ("
              << verts_.size() << " verts, " << edges_.size() << " edges, "
              << "vao=" << vao_ << " vbo=" << vbo_ << " ebo=" << ebo_
              << " prog=" << prog_ << " idxCount=" << idxCount_ << ")"
              << std::endl;
    return true;
}

// ---- Update / Render ----

void DebugModelViewer::update() {
    // Slow auto-rotation: 15 degrees per second
    rotationY_ += 15.0f * (1.0f / 60.0f);
    if (rotationY_ >= 360.0f) rotationY_ -= 360.0f;
}

void DebugModelViewer::render(int w, int h) {
    if (!prog_ || !vao_ || !idxCount_) {
        std::cerr << "Render skip: prog=" << prog_ << " vao=" << vao_ << " idxCount=" << idxCount_ << std::endl;
        return;
    }

    // Camera: positioned to see the model comfortably
    Mat4f proj  = Mat4f::perspective(45.0f, (float)w / (float)h, 0.1f, 2000.0f);
    Mat4f view  = Mat4f::lookAt(
        Vec3f(0.0f, 60.0f, 600.0f),  // eye — pulled back to see Z range up to ~400
        Vec3f(0.0f, 50.0f, 0.0f),    // center — roughly model center Y
        Vec3f(0.0f, 1.0f, 0.0f));    // up
    Mat4f mvp   = proj * view;
    Mat4f model = Mat4f::rotateY(rotationY_);
    mvp = mvp * model;

    glUseProgram(prog_);
    if (mvpLoc_ >= 0)
        glUniformMatrix4fv(mvpLoc_, 1, GL_FALSE, mvp.m);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "GL error after uniform setup: 0x" << std::hex << err << std::endl;
    }

    // Cull faces toggle
    if (culling_) glEnable(GL_CULL_FACE);
    else          glDisable(GL_CULL_FACE);

    glBindVertexArray(vao_);
    if (wireframe_) {
        glDrawElements(GL_LINES, idxCount_, GL_UNSIGNED_INT, 0);
    } else {
        glDrawElements(GL_TRIANGLES, idxCount_, GL_UNSIGNED_INT, 0);
    }

    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "GL error after draw: 0x" << std::hex << err << std::endl;
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

// Global pointer used by App::run()
DebugModelViewer* g_viewer = nullptr;