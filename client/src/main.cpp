#include <SDL3/SDL.h>
#include <iostream>

#include "CacheReader.h"
#include "ModelDef.h"
#include "Mesh.h"
#include "Renderer3D.h"
#include "Mat4.h"

constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 600;

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "RS Model Renderer",
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Renderer* sdlRenderer = SDL_CreateRenderer(window, nullptr);

    if (!sdlRenderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        return 1;
    }

    CacheReader reader;

    if (!reader.open("/home/clizard34/zedprojects/cache-tool/cache")) {
        std::cerr << "Failed to open cache\n";
        return 1;
    }

    int currentModelId = 2;

    ModelDef modelDef;
    Mesh mesh;

    auto loadModel = [&](int modelId) {

        if (!reader.hasFile(1, modelId)) {
            std::cout << "Model " << modelId << " does not exist\n";
            return;
        }

        Buffer modelBuffer = reader.readGzippedFile(1, modelId);

        if (modelBuffer.empty()) {
            std::cout << "Model buffer is empty\n";
            return;
        }

        try {

            modelDef = ModelDef::parse(modelId, modelBuffer);

            mesh = Mesh::FromModelDef(modelDef);

            currentModelId = modelId;

            std::cout << "Loaded model: " << currentModelId << "\n";

            std::cout << "Vertices: " << mesh.vertices.size() << "\n";
            std::cout << "Indices: " << mesh.indices.size() << "\n";

        } catch (const std::exception& e) {

            std::cout << "Failed loading model "
                      << modelId
                      << ": "
                      << e.what()
                      << "\n";
        }
    };

    loadModel(currentModelId);

    Renderer3D renderer3D(sdlRenderer, SCREEN_WIDTH, SCREEN_HEIGHT);

    bool running = true;
    SDL_Event event;
    float angle = 0.0f;

    uint64_t lastTicks = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }

            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_M) {
                    loadModel(currentModelId + 1);
                }

                if (event.key.key == SDLK_N) {
                    loadModel(currentModelId - 1);
                }
            }
        }

        uint64_t currentTicks = SDL_GetTicks();
        float dt = (currentTicks - lastTicks) / 1000.0f;
        lastTicks = currentTicks;

        angle += dt * 0.5f;

        Mat4 rotation = Mat4::RotationY(angle);
        Mat4 translation = Mat4::Translation(0.0f, 0.0f, 500.0f);
        Mat4 transform = translation * rotation;

        renderer3D.BeginFrame();
        renderer3D.Submit(mesh, transform);
        renderer3D.Render();
        renderer3D.EndFrame();
    }

    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
