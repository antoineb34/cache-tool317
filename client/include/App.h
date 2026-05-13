#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

class DebugModelViewer;

// App owns the window, main loop, and input forwarding.
class App {
public:
    App();
    ~App();

    bool init(const char* title, int w, int h);
    int run();
    void shutdown();

    SDL_Window* window() const { return window_; }
    int width() const { return width_; }
    int height() const { return height_; }

private:
    SDL_Window* window_ = nullptr;
    int width_ = 1280;
    int height_ = 960;
    bool running_ = true;

    void handleEvents();
};