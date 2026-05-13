#include "App.h"
#include "DebugModelViewer.h"

#include <iostream>
#include <filesystem>

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
    delete viewer_;
    viewer_ = nullptr;
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

void App::handleDebugKeys(SDL_Keycode key) {
    switch (key) {
    case SDLK_LEFT:
        // Previous model
        if (reader_) {
            int prev = currentModelId_ - 1;
            while (prev >= 0) {
                if (reader_->hasFile(1, prev)) {
                    currentModelId_ = prev;
                    if (viewer_->reloadModel(*reader_, currentModelId_)) {
                        std::cout << "--- Loaded model " << currentModelId_ << " ---" << std::endl;
                    }
                    break;
                }
                --prev;
            }
        }
        break;
    case SDLK_RIGHT:
        // Next model
        if (reader_) {
            int next = currentModelId_ + 1;
            int maxModels = 10000; // safety limit
            while (next < maxModels) {
                if (reader_->hasFile(1, next)) {
                    currentModelId_ = next;
                    if (viewer_->reloadModel(*reader_, currentModelId_)) {
                        std::cout << "--- Loaded model " << currentModelId_ << " ---" << std::endl;
                    }
                    break;
                }
                ++next;
            }
        }
        break;
    case SDLK_R:
        viewer_->resetView();
        break;
    case SDLK_W:
        viewer_->toggleWireframe();
        break;
    case SDLK_C:
        viewer_->toggleCulling();
        break;
    default:
        break;
    }
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
                } else {
                    handleDebugKeys(e.key.key);
                }
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

int App::run(CacheReader& reader, int initialModelId) {
    reader_ = &reader;
    currentModelId_ = initialModelId;

    // Create viewer and load model
    viewer_ = new DebugModelViewer();

    if (!reader.hasFile(1, currentModelId_)) {
        std::cerr << "Model " << currentModelId_ << " not found in archive 1" << std::endl;
        return 1;
    }

    if (!viewer_->load(reader, currentModelId_)) {
        std::cerr << "Failed to load model " << currentModelId_ << std::endl;
        return 1;
    }

    // Log initial model info
    std::cout << "--- Model Info ---" << std::endl;
    std::cout << "  Model ID:          " << viewer_->loadedModelId() << std::endl;
    std::cout << "  Vertices:          " << viewer_->vertexCount() << std::endl;
    std::cout << "  Triangles:         " << viewer_->triangleCount() << std::endl;
    std::cout << "  Textured triangles: " << viewer_->texturedFaceCount() << std::endl;
    std::cout << "-------------------" << std::endl;

    // Main loop
    while (running_) {
        handleEvents();

        int w = width_, h = height_;
        glViewport(0, 0, w, h);

        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        viewer_->update();
        viewer_->render(w, h);

        SDL_GL_SwapWindow(window_);
    }

    return 0;
}