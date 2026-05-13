#include "App.h"
#include "WireframeMeshRenderer.h"

#include <iostream>

App::App()
    : window_(nullptr)
    , glContext_(nullptr)
    , running_(false)
    , renderer_(nullptr)
{
}

App::~App() {
    shutdown();
}

bool App::init() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    window_ = SDL_CreateWindow("RS317 Cache Tool", 1280, 720, SDL_WINDOW_OPENGL);
    if (!window_) {
        std::cerr << "Window failed: " << SDL_GetError() << std::endl;
        return false;
    }

    glContext_ = SDL_GL_CreateContext(window_);
    if (!glContext_) {
        std::cerr << "GL context failed: " << SDL_GetError() << std::endl;
        return false;
    }

    if (!SDL_GL_GetProcAddress("glGenVertexArrays") ||
        !SDL_GL_GetProcAddress("glBindBuffer") ||
        !SDL_GL_GetProcAddress("glUseProgram")) {
        std::cerr << "Need OpenGL 3.3+" << std::endl;
        return false;
    }

    glViewport(0, 0, 1280, 720);

    renderer_ = new WireframeMeshRenderer();
    if (!renderer_->init()) {
        std::cerr << "Failed to init wireframe renderer" << std::endl;
        return false;
    }

    running_ = true;
    return true;
}

void App::handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_EVENT_QUIT:
                running_ = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (e.key.key == SDLK_ESCAPE) {
                    running_ = false;
                }
                break;
            default:
                break;
        }
    }
}

void App::render() {
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    renderer_->render();

    SDL_GL_SwapWindow(window_);
}

void App::shutdown() {
    delete renderer_;
    renderer_ = nullptr;
    if (glContext_) {
        SDL_GL_DestroyContext(glContext_);
        glContext_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

int App::run() {
    if (!init()) {
        return 1;
    }

    while (running_) {
        handleEvents();
        render();
    }

    shutdown();
    return 0;
}