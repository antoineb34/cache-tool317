#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

class WireframeMeshRenderer;

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

    SDL_Window* window_;
    SDL_GLContext glContext_;
    bool running_;

    WireframeMeshRenderer* renderer_;
};