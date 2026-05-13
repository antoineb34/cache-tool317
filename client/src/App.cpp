#include "App.h"
#include "DebugModelViewer.h"

#include <iostream>

App::App() = default;
App::~App() { shutdown(); }

bool App::init(const char* title, int w, int h) {
    width_ = w;
    height_ = h;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    window_ = SDL_CreateWindow(title, w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window_) {
        std::cerr << "Window failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    if (!SDL_GL_CreateContext(window_)) {
        std::cerr << "GL context failed: " << SDL_GetError() << std::endl;
        return false;
    }

    if (!SDL_GL_GetProcAddress("glGenVertexArrays") ||
        !SDL_GL_GetProcAddress("glBindBuffer") ||
        !SDL_GL_GetProcAddress("glUseProgram")) {
        std::cerr << "Need OpenGL 3.3+" << std::endl;
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    return true;
}

void App::shutdown() {
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

// Forward-declared; defined in main.cpp
extern DebugModelViewer* g_viewer;

void App::handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_EVENT_QUIT:
                running_ = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (e.key.key == SDLK_ESCAPE) running_ = false;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                width_ = e.window.data1;
                height_ = e.window.data2;
                break;
            default:
                break;
        }
    }
}

int App::run() {
    if (!g_viewer) {
        std::cerr << "No viewer attached" << std::endl;
        return 1;
    }

    while (running_) {
        handleEvents();

        int w = width_, h = height_;
        glViewport(0, 0, w, h);

        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        g_viewer->update();
        g_viewer->render(w, h);

        SDL_GL_SwapWindow(window_);
    }

    return 0;
}