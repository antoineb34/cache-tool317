#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <iostream>

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("cache-tool317 - client stub", 800, 600, SDL_WINDOW_OPENGL);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "GL context failed: " << SDL_GetError() << "\n";
        return 1;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
        }
        glClear(GL_COLOR_BUFFER_BIT);
        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
