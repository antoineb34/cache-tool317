// Minimal wireframe debug viewer for RS317 ModelDef validation.
// Loads a single hardcoded model from the cache and renders it as GL_LINES.
// No textures, no lighting, no terrain, no definitions, no batching.
//
// Usage: client <cache_path> [--model <id>]

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include "CacheReader.h"
#include "ModelDef.h"

#include <iostream>
#include <vector>
#include <unordered_set>
#include <cstdint>
#include <cmath>
#include <cstring>

// ============================================
// Simple vector / matrix helpers (no external dependency)
// ============================================

struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vec3 operator+(const Vec3& v) const { return {x+v.x, y+v.y, z+v.z}; }
    Vec3 operator-(const Vec3& v) const { return {x-v.x, y-v.y, z-v.z}; }
    Vec3 operator*(float s) const { return {x*s, y*s, z*s}; }
    float dot(const Vec3& v) const { return x*v.x + y*v.y + z*v.z; }
    Vec3 cross(const Vec3& v) const {
        return {y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x};
    }
    float length() const { return std::sqrt(x*x + y*y + z*z); }
    Vec3 normalized() const { float l = length(); return l > 0 ? (*this / l) : Vec3{}; }
    Vec3 operator/(float s) const { return {x/s, y/s, z/s}; }
};

struct Mat4 {
    float m[16] = {0};

    static Mat4 identity() {
        Mat4 r;
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1;
        return r;
    }

    static Mat4 perspective(float fovY, float aspect, float nearZ, float farZ) {
        Mat4 r;
        float f = 1.0f / std::tan(fovY * 0.5f * 3.14159265f / 180.0f);
        float nf = 1.0f / (nearZ - farZ);
        r.m[0]  = f / aspect;
        r.m[5]  = f;
        r.m[10] = (farZ + nearZ) * nf;
        r.m[11] = -1.0f;
        r.m[14] = 2.0f * farZ * nearZ * nf;
        return r;
    }

    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Mat4 r;
        Vec3 f = (center - eye).normalized();
        Vec3 s = f.cross(up).normalized();
        Vec3 u = s.cross(f);
        r.m[0]=s.x; r.m[4]=s.y; r.m[8]=s.z;  r.m[12]=-s.dot(eye);
        r.m[1]=u.x; r.m[5]=u.y; r.m[9]=u.z;  r.m[13]=-u.dot(eye);
        r.m[2]=-f.x;r.m[6]=-f.y;r.m[10]=-f.z; r.m[14]=f.dot(eye);
        r.m[15]=1;
        return r;
    }

    static Mat4 rotateY(float deg) {
        Mat4 r = identity();
        float rad = deg * 3.14159265f / 180.0f;
        float c = std::cos(rad), s = std::sin(rad);
        r.m[0] = c;  r.m[2] = s;
        r.m[8] = -s; r.m[10] = c;
        return r;
    }

    static Mat4 translate(float x, float y, float z) {
        Mat4 r = identity();
        r.m[12] = x; r.m[13] = y; r.m[14] = z;
        return r;
    }

    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int row = 0; row < 4; row++)
            for (int col = 0; col < 4; col++) {
                float sum = 0;
                for (int k = 0; k < 4; k++)
                    sum += m[k*4+row] * o.m[col*4+k];
                r.m[col*4+row] = sum;
            }
        return r;
    }
};

// ============================================
// WireframeVertex — position + colour
// ============================================

struct WireframeVertex {
    float x, y, z;
    uint8_t r, g, b;
};

// ============================================
// Simple shader (hardcoded strings, no file I/O)
// ============================================

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::cerr << "Shader compile error: " << log << std::endl;
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static GLuint createShaderProgram(const char* vertSrc, const char* fragSrc) {
    GLuint v = compileShader(GL_VERTEX_SHADER, vertSrc);
    GLuint f = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    if (!v || !f) return 0;
    GLuint prog = glCreateProgram();
    glAttachShader(prog, v);
    glAttachShader(prog, f);
    glLinkProgram(prog);
    glDeleteShader(v);
    glDeleteShader(f);
    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::cerr << "Shader link error: " << log << std::endl;
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

// ============================================
// WireframeRenderer
// ============================================

class WireframeRenderer {
public:
    WireframeRenderer() = default;
    ~WireframeRenderer() { cleanup(); }

    // Non-copyable
    WireframeRenderer(const WireframeRenderer&) = delete;
    WireframeRenderer& operator=(const WireframeRenderer&) = delete;

    bool loadModel(CacheReader& reader, int modelId) {
        if (!reader.hasFile(1, modelId)) {
            std::cerr << "Model " << modelId << " not found in cache archive 1" << std::endl;
            return false;
        }
        Buffer buf = reader.readGzippedFile(1, modelId);
        if (buf.empty()) {
            std::cerr << "Empty buffer for model " << modelId << std::endl;
            return false;
        }

        ModelDef def = ModelDef::parse(modelId, buf);
        return buildFromDef(def);
    }

    bool initGL() {
        const char* vertSrc = R"GLSL(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
uniform mat4 uMVP;
out vec3 vColor;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}
)GLSL";

        const char* fragSrc = R"GLSL(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor / 255.0, 1.0);
}
)GLSL";

        shader_ = createShaderProgram(vertSrc, fragSrc);
        if (!shader_) return false;

        // Build GL buffers
        if (vertices_.empty() || edges_.empty()) return false;

        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);

        glBindVertexArray(vao_);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER,
                     vertices_.size() * sizeof(WireframeVertex),
                     vertices_.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     edges_.size() * 2 * sizeof(uint32_t),
                     edges_.data(), GL_STATIC_DRAW);

        // Attribute 0: position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(WireframeVertex), (void*)0);
        // Attribute 1: colour
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(WireframeVertex), (void*)offsetof(WireframeVertex, r));

        glBindVertexArray(0);

        indexCount_ = (int)(edges_.size() * 2);
        mvpLoc_ = glGetUniformLocation(shader_, "uMVP");

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "GL error during wireframe init: 0x" << std::hex << err << std::endl;
            return false;
        }

        return true;
    }

    void draw(const Mat4& mvp, float rotateY) {
        if (!shader_ || !vao_) return;
        Mat4 modelMat = Mat4::rotateY(rotateY);
        Mat4 finalMVP = mvp * modelMat;

        glUseProgram(shader_);
        if (mvpLoc_ >= 0) glUniformMatrix4fv(mvpLoc_, 1, GL_FALSE, finalMVP.m);

        glBindVertexArray(vao_);
        glDrawElements(GL_LINES, indexCount_, GL_UNSIGNED_INT, 0);
    }

    void cleanup() {
        if (ebo_) { glDeleteBuffers(1, &ebo_); ebo_ = 0; }
        if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
        if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
        if (shader_) { glDeleteProgram(shader_); shader_ = 0; }
    }

    // Metadata
    int modelId() const { return modelId_; }
    int vertexCount() const { return (int)vertices_.size(); }
    int triangleCount() const { return (int)triIndices_.size() / 3; }
    int texturedFaceCount() const { return texturedFaceCount_; }

private:
    int modelId_ = -1;
    int texturedFaceCount_ = 0;

    std::vector<WireframeVertex> vertices_;
    std::vector<uint32_t> triIndices_;     // flat [a,b,c, ...]
    std::vector<std::pair<uint32_t,uint32_t>> edges_;

    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    GLuint shader_ = 0;
    int indexCount_ = 0;
    int mvpLoc_ = -1;

    bool buildFromDef(const ModelDef& def) {
        modelId_ = def.id;
        texturedFaceCount_ = 0;

        // --- Validate indices first (before any GPU upload) ---
        for (size_t i = 0; i < def.triA.size(); i++) {
            int a = def.triA[i], b = def.triB[i], c = def.triC[i];
            if (a < 0 || a >= (int)def.vertexX.size() ||
                b < 0 || b >= (int)def.vertexX.size() ||
                c < 0 || c >= (int)def.vertexX.size()) {
                std::cerr << "ERROR: Triangle " << i
                          << " index out of range (vertexCount=" << def.vertexX.size()
                          << ") a=" << a << " b=" << b << " c=" << c
                          << std::endl;
                return false;
            }
        }

        // --- Build flat index array ---
        triIndices_.clear();
        triIndices_.reserve(def.triA.size() * 3);
        for (size_t i = 0; i < def.triA.size(); i++) {
            triIndices_.push_back((uint32_t)def.triA[i]);
            triIndices_.push_back((uint32_t)def.triB[i]);
            triIndices_.push_back((uint32_t)def.triC[i]);
        }

        // --- Build vertices with vertex colours from first adjacent face ---
        vertices_.clear();
        vertices_.reserve(def.vertexX.size());
        std::vector<bool> coloured(def.vertexX.size(), false);

        for (size_t i = 0; i < def.vertexX.size(); i++) {
            WireframeVertex v;
            // RS317 Y-up → OpenGL Y-up: negate Y and Z
            v.x = (float)def.vertexX[i];
            v.y = -(float)def.vertexY[i];
            v.z = -(float)def.vertexZ[i];
            v.r = v.g = v.b = 180; // default grey
            vertices_.push_back(v);
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
                    vertices_[idx[j]].r = cr;
                    vertices_[idx[j]].g = cg;
                    vertices_[idx[j]].b = cb;
                    coloured[idx[j]] = true;
                }
            }

            // Count textured faces
            if (!def.triRenderType.empty() && (def.triRenderType[i] & 2))
                texturedFaceCount_++;
        }

        // --- Extract unique edges from triangles ---
        struct PairHash {
            size_t operator()(const std::pair<uint32_t,uint32_t>& p) const {
                return ((size_t)p.first << 16) | p.second;
            }
        };
        std::unordered_set<std::pair<uint32_t,uint32_t>, PairHash> edgeSet;
        for (size_t i = 0; i < def.triA.size(); i++) {
            uint32_t a = def.triA[i], b = def.triB[i], c = def.triC[i];
            auto me = [](uint32_t x, uint32_t y) { return x<y ? std::make_pair(x,y) : std::make_pair(y,x); };
            edgeSet.insert(me(a,b));
            edgeSet.insert(me(b,c));
            edgeSet.insert(me(c,a));
        }
        edges_.assign(edgeSet.begin(), edgeSet.end());

        std::cout << "WireframeRenderer loaded model " << modelId_
                  << " | vertices=" << vertices_.size()
                  << " | triangles=" << def.triA.size()
                  << " | textured=" << texturedFaceCount_
                  << " | edges=" << edges_.size()
                  << std::endl;
        return true;
    }

    // HSL→RGB from Math.h (reimplemented here to avoid extra includes)
    static uint32_t hslToRgb(uint16_t hsl) {
        int hue = (hsl >> 10) & 0x3F;
        int sat = (hsl >>  7) & 0x07;
        int lit =  hsl        & 0x7F;
        float h = (hue / 64.0f) + 0.0078125f;
        float s = (sat /  8.0f) + 0.0625f;
        float l =  lit / 128.0f;
        float r,g,b;
        if (s == 0) { r = g = b = l; }
        else {
            float a,p;
            if (l < 0.5f) a = l*(1+s); else a = l+s-l*s;
            p = 2*l - a;
            auto hue2 = [&](float h) -> float {
                if (h<0) h+=1; if (h>1) h-=1;
                if (6*h<1) return p+(a-p)*6*h;
                if (2*h<1) return a;
                if (3*h<2) return p+(a-p)*((2.0f/3)-h)*6;
                return p;
            };
            r = hue2(h + 1.0f/3);
            g = hue2(h);
            b = hue2(h - 1.0f/3);
        }
        int ri = std::max(0,std::min(255,(int)(r*256)));
        int gi = std::max(0,std::min(255,(int)(g*256)));
        int bi = std::max(0,std::min(255,(int)(b*256)));
        return (ri<<16)|(gi<<8)|bi;
    }
};

// ============================================
// Globals
// ============================================

static bool g_running = true;
static float g_rotationY = 0.0f;

// ============================================
// Input
// ============================================

static void handleEvents(SDL_Window* w) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_EVENT_QUIT:
                g_running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (e.key.key == SDLK_ESCAPE) g_running = false;
                break;
            default: break;
        }
    }
}

// ============================================
// Entry
// ============================================

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: client <cache_path> [--model <id>]" << std::endl;
        return 1;
    }

    std::string cachePath = argv[1];
    int modelId = 100; // default: common tree

    for (int i = 2; i < argc; i++) {
        if (std::strcmp(argv[i], "--model") == 0 && i+1 < argc) {
            modelId = std::atoi(argv[++i]);
        }
    }

    // --- SDL3 init ---
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "RS317 Wireframe Debug Viewer",
        1280, 960,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window) { std::cerr << "Window failed: " << SDL_GetError() << std::endl; SDL_Quit(); return 1; }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    if (!SDL_GL_CreateContext(window)) {
        std::cerr << "GL context failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window); SDL_Quit(); return 1;
    }

    // Check GL 3.3+
    if (!SDL_GL_GetProcAddress("glGenVertexArrays") ||
        !SDL_GL_GetProcAddress("glBindBuffer") ||
        !SDL_GL_GetProcAddress("glUseProgram")) {
        std::cerr << "Need OpenGL 3.3+" << std::endl;
        SDL_Quit(); return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // --- Load cache ---
    CacheReader reader;
    if (!reader.open(cachePath)) {
        std::cerr << "Failed to open cache: " << cachePath << std::endl;
        SDL_Quit(); return 1;
    }
    std::cout << "Cache opened: " << cachePath << std::endl;

    if (!reader.hasFile(1, modelId)) {
        std::cerr << "Model " << modelId << " not found in archive 1" << std::endl;
        SDL_Quit(); return 1;
    }

    WireframeRenderer renderer;
    if (!renderer.loadModel(reader, modelId)) {
        std::cerr << "Failed to load model " << modelId << std::endl;
        SDL_Quit(); return 1;
    }
    if (!renderer.initGL()) {
        std::cerr << "Failed to init wireframe GL" << std::endl;
        SDL_Quit(); return 1;
    }

    // Camera setup
    Vec3 eye(0, 50, 100);
    Vec3 target(0, 0, 0);
    Vec3 up(0, 1, 0);
    Mat4 proj = Mat4::perspective(60.0f, 1280.0f/960.0f, 0.1f, 1000.0f);
    Mat4 view = Mat4::lookAt(eye, target, up);
    Mat4 mvp = proj * view;

    std::cout << "=== Wireframe Debug Viewer Running ===" << std::endl;
    std::cout << "  Model: " << renderer.modelId() << std::endl;
    std::cout << "  Vertices: " << renderer.vertexCount() << std::endl;
    std::cout << "  Triangles: " << renderer.triangleCount() << std::endl;
    std::cout << "  Textured faces: " << renderer.texturedFaceCount() << std::endl;
    std::cout << "  Controls: ESC=quit, mouse=drag to rotate" << std::endl;

    Uint64 lastTime = SDL_GetTicks();
    bool mouseDown = false;
    float lastMX = 0, lastMY = 0;

    while (g_running) {
        Uint64 now = SDL_GetTicks();
        float dt = (now - lastTime) / 1000.0f;
        lastTime = now;

        handleEvents(window);

        // Simple click-drag rotation
        float mx = 0, my = 0;
        Uint32 buttons = SDL_GetMouseState(&mx, &my);
        if (buttons & SDL_BUTTON_LMASK) {
            if (!mouseDown) { lastMX = mx; lastMY = my; mouseDown = true; }
            float dx = mx - lastMX;
            g_rotationY += dx * 0.3f;
            lastMX = mx; lastMY = my;
        } else {
            mouseDown = false;
        }

        // Auto-rotate slowly
        g_rotationY += dt * 5.0f;

        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        glViewport(0, 0, w, h);

        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer.draw(mvp, g_rotationY);

        SDL_GL_SwapWindow(window);
    }

    std::cout << "Shutting down..." << std::endl;
    SDL_Quit();
    return 0;
}