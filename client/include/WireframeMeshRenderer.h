#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <cstddef>

class WireframeMeshRenderer {
public:
    WireframeMeshRenderer();
    ~WireframeMeshRenderer();

    bool init();
    void shutdown();

    // Upload tightly packed XYZ float vertices. floatCount must be divisible by 3.
    bool uploadLines(const float* vertices, std::size_t floatCount);
    void render();

private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint shaderProgram_ = 0;

    std::size_t vertexCount_ = 0;
    bool initialized_ = false;

    GLuint createShader(GLenum type, const char* source);
};