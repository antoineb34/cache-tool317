#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

class WireframeMeshRenderer {
public:
    WireframeMeshRenderer();
    ~WireframeMeshRenderer();

    bool init();
    void shutdown();
    void render();

private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint shaderProgram_ = 0;

    bool initialized_ = false;

    GLuint createShader(GLenum type, const char* source);
};