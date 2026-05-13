#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <SDL3/SDL.h>
#include <iostream>

#include "CacheReader.h"
#include "ModelDef.h"
#include "TextureLoader.h"
#include "Mat4.h"
#include "ModelRenderer.h"

int main(int argc, char* argv[]) {
    if (argc < 2) { std::cerr << "Usage: client <cache path> [model id]\n"; return 1; }

    CacheReader reader;
    if (!reader.open(argv[1])) { std::cerr << "Failed to open cache\n"; return 1; }

    // Load textures from archive 0 file 6
    auto texArchivePtr = reader.readArchive(0, 6);
    if (!texArchivePtr) { std::cerr << "Failed to read textures archive\n"; return 1; }
    Archive& texArchive = *texArchivePtr;
    TextureLoader texLoader;
    texLoader.load(texArchive);
    std::vector<TextureDef> textures;
    for (int i = 0; i < texLoader.textureCount(); i++) {
        textures.push_back(texLoader.getTexture(i));
    }
    std::cout << "Loaded " << textures.size() << " textures\n";

    int modelId = argc >= 3 ? std::stoi(argv[2]) : 0;
    Buffer modelBuf = reader.readGzippedFile(1, modelId);
    if (modelBuf.empty()) {
        std::cerr << "Model " << modelId << " not found\n";
        return 1;
    }
    ModelDef model = ModelDef::parse(modelId, modelBuf);
    std::cout << "Model " << modelId << ": "
              << model.vertexX.size() << " verts, "
              << model.triA.size()    << " tris\n";

    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window*   window = SDL_CreateWindow("cache-tool317", 800, 600, SDL_WINDOW_OPENGL);
    SDL_GLContext ctx    = SDL_GL_CreateContext(window);
    if (!ctx) { std::cerr << "GL context failed: " << SDL_GetError() << "\n"; return 1; }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";

    ModelRenderer renderer;
    renderer.load(model, textures);

    float  camDist = renderer.boundingRadius() * 2.5f;
    float  cx      = renderer.centerX();
    float  cy      = renderer.centerY();
    float  cz      = renderer.centerZ();
    float  angle   = 0.0f;
    Uint64 last    = SDL_GetTicks();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e))
            if (e.type == SDL_EVENT_QUIT) running = false;

        float dt = (SDL_GetTicks() - last) / 1000.0f;
        last = SDL_GetTicks();
        angle += dt;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Mat4 transform = mat4Multiply(
            mat4Perspective(0.785f, 800.0f/600.0f, 1.0f, camDist * 4.0f),
            mat4Multiply(
                mat4Translate(0, 0, -camDist),
                mat4Multiply(mat4RotateY(angle), mat4Translate(-cx, -cy, -cz))
            )
        );

        renderer.draw(transform);
        SDL_GL_SwapWindow(window);
    }

    renderer.cleanup();
    SDL_GL_DestroyContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
