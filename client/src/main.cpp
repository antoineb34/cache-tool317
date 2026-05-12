#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>

#include "CacheReader.h"
#include "ModelDef.h"

// ---------------------------------------------------------------------------
// Shaders
//
// Now the vertex shader has two inputs: pos (location 0) and color (location 1).
// It passes the color through to the fragment shader via "vColor".
// The fragment shader just outputs whatever color it received.
// ---------------------------------------------------------------------------
static const char* vertSrc = R"(
#version 330 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
uniform mat4 transform;
out vec3 vColor;
void main() {
    gl_Position = transform * vec4(pos, 1.0);
    vColor = color;
}
)";

static const char* fragSrc = R"(
#version 330 core
in  vec3 vColor;
out vec4 fragColor;
void main() {
    fragColor = vec4(vColor, 1.0);
}
)";

// ---------------------------------------------------------------------------
// HSL to RGB
//
// RS317 packs hue/sat/light into a uint16:
//   bits 15-10: hue   (0-63)
//   bits  9- 7: sat   (0-7)
//   bits  6- 0: light (0-127)
// ---------------------------------------------------------------------------
static void hslToRgb(uint16_t packed, float& r, float& g, float& b) {
    float H = ((packed >> 10) & 0x3F) / 64.0f;
    float S = ((packed >>  7) & 0x07) /  7.0f;
    float L =  (packed        & 0x7F) / 127.0f;

    if (S == 0.0f) { r = g = b = L; return; }

    float C  = (1.0f - std::abs(2.0f * L - 1.0f)) * S;
    float X  = C * (1.0f - std::abs(std::fmod(H * 6.0f, 2.0f) - 1.0f));
    float m  = L - C * 0.5f;
    float h6 = H * 6.0f;

    if      (h6 < 1) { r=C+m; g=X+m; b=  m; }
    else if (h6 < 2) { r=X+m; g=C+m; b=  m; }
    else if (h6 < 3) { r=  m; g=C+m; b=X+m; }
    else if (h6 < 4) { r=  m; g=X+m; b=C+m; }
    else if (h6 < 5) { r=X+m; g=  m; b=C+m; }
    else             { r=C+m; g=  m; b=X+m; }
}

// ---------------------------------------------------------------------------
// Matrix math (same as before)
// ---------------------------------------------------------------------------
struct Mat4 { float m[16] = {}; };

static Mat4 identity() {
    Mat4 r; r.m[0]=r.m[5]=r.m[10]=r.m[15]=1.0f; return r;
}
static Mat4 multiply(const Mat4& a, const Mat4& b) {
    Mat4 r;
    for (int col=0; col<4; col++)
        for (int row=0; row<4; row++)
            for (int k=0; k<4; k++)
                r.m[col*4+row] += a.m[k*4+row] * b.m[col*4+k];
    return r;
}
static Mat4 translate(float x, float y, float z) {
    Mat4 r=identity(); r.m[12]=x; r.m[13]=y; r.m[14]=z; return r;
}
static Mat4 rotateY(float a) {
    Mat4 r=identity();
    r.m[0]= std::cos(a); r.m[8] = std::sin(a);
    r.m[2]=-std::sin(a); r.m[10]= std::cos(a);
    return r;
}
static Mat4 perspective(float fovY, float aspect, float near, float far) {
    Mat4 r;
    float f=1.0f/std::tan(fovY*0.5f);
    r.m[0]=f/aspect; r.m[5]=f;
    r.m[10]=(far+near)/(near-far); r.m[11]=-1.0f;
    r.m[14]=(2.0f*far*near)/(near-far);
    return r;
}

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s=glCreateShader(type);
    glShaderSource(s,1,&src,nullptr); glCompileShader(s);
    GLint ok; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(s,512,nullptr,log);
        throw std::runtime_error(std::string("Shader error: ")+log);
    }
    return s;
}

int main(int argc, char* argv[]) {
    if (argc < 2) { std::cerr << "Usage: client <cache path> [model id]\n"; return 1; }

    CacheReader reader;
    if (!reader.open(argv[1])) { std::cerr << "Failed to open cache\n"; return 1; }

    int modelId = argc >= 3 ? std::stoi(argv[2]) : 0;
    ModelDef model = ModelDef::parse(modelId, reader.readGzippedFile(1, modelId));

    std::cout << "Model " << modelId << ": "
              << model.vertexX.size() << " vertices, "
              << model.triA.size() << " triangles\n";

    // -----------------------------------------------------------------------
    // Build vertex buffer — no index buffer this time.
    //
    // For each triangle we emit 3 vertices, all with the same face color.
    // Layout per vertex: [x, y, z, r, g, b]  (6 floats)
    //
    // This is the standard way to do per-face color in OpenGL — you can't
    // attach a color to a triangle directly, only to vertices. So we just
    // give the same color to all 3 corners of each triangle.
    // -----------------------------------------------------------------------
    std::vector<float> verts;
    verts.reserve(model.triA.size() * 3 * 6);

    for (int i = 0; i < (int)model.triA.size(); i++) {
        float r, g, b;
        if (model.isFaceTextured(i)) {
            // textured face: triColor is a dark tint meant to modulate a texture,
            // not a flat color. Use mid-grey placeholder until textures are implemented.
            r = g = b = 0.5f;
        } else {
            hslToRgb(model.triColor[i], r, g, b);
        }

        uint16_t corners[3] = { model.triA[i], model.triB[i], model.triC[i] };
        for (uint16_t vi : corners) {
            verts.push_back((float) model.vertexX[vi]);
            verts.push_back((float)-model.vertexY[vi]);  // RS317 Y points down
            verts.push_back((float) model.vertexZ[vi]);
            verts.push_back(r);
            verts.push_back(g);
            verts.push_back(b);
        }
    }

    // bounding sphere for camera placement
    int triCount = (int)model.triA.size();
    float cx=0, cy=0, cz=0;
    for (int i=0; i<(int)model.vertexX.size(); i++) {
        cx += (float) model.vertexX[i];
        cy += (float)-model.vertexY[i];
        cz += (float) model.vertexZ[i];
    }
    int nv = (int)model.vertexX.size();
    cx/=nv; cy/=nv; cz/=nv;

    float radius=0;
    for (int i=0; i<nv; i++) {
        float dx=(float)model.vertexX[i]-cx;
        float dy=(float)-model.vertexY[i]-cy;
        float dz=(float)model.vertexZ[i]-cz;
        radius=std::max(radius, std::sqrt(dx*dx+dy*dy+dz*dz));
    }
    float camDist = radius * 2.5f;

    // --- SDL + OpenGL ---
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window*   window = SDL_CreateWindow("cache-tool317", 800, 600, SDL_WINDOW_OPENGL);
    SDL_GLContext ctx    = SDL_GL_CreateContext(window);
    if (!ctx) { std::cerr << "GL context failed: " << SDL_GetError() << "\n"; return 1; }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";

    GLuint vert = compileShader(GL_VERTEX_SHADER,   vertSrc);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert); glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert); glDeleteShader(frag);

    GLint tfmLoc = glGetUniformLocation(prog, "transform");

    // --- upload to GPU ---
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(float), verts.data(), GL_STATIC_DRAW);

      // attribute 0: position (first 3 floats of each 6-float vertex)
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
      glEnableVertexAttribArray(0);

      // attribute 1: color (last 3 floats of each 6-float vertex)
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
      glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    float  angle = 0.0f;
    Uint64 last  = SDL_GetTicks();

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e))
            if (e.type == SDL_EVENT_QUIT) running = false;

        float dt = (SDL_GetTicks()-last)/1000.0f;
        last = SDL_GetTicks();
        angle += dt;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Mat4 tfm = multiply(
            perspective(0.785f, 800.0f/600.0f, 1.0f, camDist*4.0f),
            multiply(
                translate(0, 0, -camDist),
                multiply(rotateY(angle), translate(-cx, -cy, -cz))
            )
        );

        glUseProgram(prog);
        glUniformMatrix4fv(tfmLoc, 1, GL_FALSE, tfm.m);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, triCount * 3);

        SDL_GL_SwapWindow(window);
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(prog);
    SDL_GL_DestroyContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
