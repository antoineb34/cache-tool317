#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <filesystem>

// Forward declarations
class DebugModelViewer;

class CacheReader;

// App owns the window, main loop, input forwarding, and viewer lifecycle.
class App {
public:
    App();
    ~App();

    bool init(const char* title, int w, int h);
    int run(CacheReader& reader, int initialModelId);
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
    void handleDebugKeys(SDL_Keycode key);

    DebugModelViewer* viewer_ = nullptr;
    CacheReader* reader_ = nullptr;
    int currentModelId_ = 100;
};