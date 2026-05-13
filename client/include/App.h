#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

class App {
public:
    App();
    ~App();

    int run();

private:
    bool init();
    void handleEvents();
    void render();
    void shutdown();

    // Triangle
    bool initTriangle();
    void destroyTriangle();
    GLuint createShader(GLenum type, const char* source);

    SDL_Window* window_;
    SDL_GLContext glContext_;
    bool running_;

    GLuint vao_;
    GLuint vbo_;
    GLuint shaderProgram_;
};