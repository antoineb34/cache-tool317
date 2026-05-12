#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include "CacheReader.h"
#include "Model.h"

// Helper: set up a perspective projection matrix.
// 'fov' is field of view in degrees, 'aspect' is width/height.
static void perspective(float fov, float aspect, float nearPlane, float farPlane) {
    float f = 1.0f / std::tan(fov * 3.14159265f / 360.0f);
    float nf = 1.0f / (nearPlane - farPlane);

    float m[16] = {
        f / aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, (farPlane + nearPlane) * nf, -1,
        0, 0, (2.0f * farPlane * nearPlane) * nf, 0
    };
    glLoadMatrixf(m);
}

int main() {
    // 1. Initialize SDL (the library that creates windows and handles input)
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    // 2. Create a window with an OpenGL context
    SDL_Window* window = SDL_CreateWindow(
        "cache-tool317",      // title
        800, 600,              // width, height
        SDL_WINDOW_OPENGL      // we want OpenGL, not just a 2D surface
    );

    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << "\n";
        return 1;
    }

    // 3. Create the OpenGL context (the thing that lets us talk to the GPU)
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "GL context failed: " << SDL_GetError() << "\n";
        return 1;
    }

    // 4. Tell OpenGL: when clearing the screen, use this color (dark blue)
    glClearColor(0.1f, 0.2f, 0.4f, 1.0f);

    // ------------------------------------------------------------------
    // Load one 3D model from the cache (archive 1 = models).
    // ------------------------------------------------------------------
    CacheReader reader;
    if (!reader.open("./cache")) {
        std::cerr << "Failed to open cache\n";
        return 1;
    }

    // Model 1219 — a known test model from the cache.
    std::vector<uint8_t> modelData = reader.readGzippedFile(1, 1219);
    if (modelData.empty()) {
        std::cerr << "Model 1219 not found\n";
        return 1;
    }
    Model model = Model::parse(modelData);
    std::cout << "Loaded model 1219: " << model.vertices().size() << " vertices, "
              << model.triangles().size() << " triangles\n";

    // 5. Main loop
    bool running = true;
    float cameraZ = -800.0f;   // distance from the model (negative = back away)
    float rotX = 25.0f;        // tilt up/down (degrees)
    float rotY = 45.0f;        // spin left/right (degrees)
    float panX = 0.0f;         // move camera left/right in world space
    float panY = 0.0f;         // move camera up/down in world space
    bool mouseDragging = false;
    float lastMouseX = 0.0f;
    float lastMouseY = 0.0f;
    uint64_t lastTicks = SDL_GetTicks();
    while (running) {
        // Handle events (window close, mouse drag, mouse wheel).
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                if (event.wheel.y > 0)
                    cameraZ *= 0.9f;
                if (event.wheel.y < 0)
                    cameraZ *= 1.1f;
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
                mouseDragging = true;
                lastMouseX = event.button.x;
                lastMouseY = event.button.y;
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
                mouseDragging = false;
            }
            if (event.type == SDL_EVENT_MOUSE_MOTION && mouseDragging) {
                float dx = event.motion.x - lastMouseX;
                float dy = event.motion.y - lastMouseY;
                rotY += dx * 0.5f;
                rotX += dy * 0.5f;
                rotX = std::clamp(rotX, -90.0f, 90.0f);
                lastMouseX = event.motion.x;
                lastMouseY = event.motion.y;
            }
        }

        // Frame-rate independent timing.
        uint64_t now = SDL_GetTicks();
        float delta = static_cast<float>(now - lastTicks) / 1000.0f;
        lastTicks = now;
        delta = std::min(delta, 0.05f);  // cap to avoid huge jumps if lag spikes

        // Continuous keyboard controls (smooth while holding keys).
        const bool* keys = SDL_GetKeyboardState(nullptr);
        float moveSpeed = -cameraZ * 0.5f * delta;  // gentle, scales with zoom
        if (keys[SDL_SCANCODE_LEFT])  panX += moveSpeed;
        if (keys[SDL_SCANCODE_RIGHT]) panX -= moveSpeed;
        if (keys[SDL_SCANCODE_UP])    panY -= moveSpeed;
        if (keys[SDL_SCANCODE_DOWN])  panY += moveSpeed;
        // Note: zoom is mouse-wheel only. Page Up/Down are free for future use.

        // Clear the screen to the color we set above
        glClear(GL_COLOR_BUFFER_BIT);

        // Set up a 3D perspective projection.
        // Things farther away look smaller — this is what makes it look 3D.
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        float aspect = static_cast<float>(w) / static_cast<float>(h);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        perspective(60.0f, aspect, 1.0f, 1000.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Move the camera to the current zoom distance and pan position.
        glTranslatef(panX, panY - 100.0f, cameraZ);

        // Rotate based on mouse drag (or default starting angle).
        glRotatef(rotX, 1.0f, 0.0f, 0.0f);   // tilt up/down
        glRotatef(rotY, 0.0f, 1.0f, 0.0f);   // spin left/right

        // Draw the model as a wireframe — just the edges of every triangle.
        // No lighting, no colors, no filling. Just white lines showing the shape.
        glBegin(GL_LINES);
        glColor3f(1.0f, 1.0f, 1.0f);  // white
        for (const ModelTriangle& tri : model.triangles()) {
            const ModelVertex& a = model.vertices()[tri.a];
            const ModelVertex& b = model.vertices()[tri.b];
            const ModelVertex& c = model.vertices()[tri.c];

            // Edge a -> b
            glVertex3f(static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z));
            glVertex3f(static_cast<float>(b.x), static_cast<float>(b.y), static_cast<float>(b.z));

            // Edge b -> c
            glVertex3f(static_cast<float>(b.x), static_cast<float>(b.y), static_cast<float>(b.z));
            glVertex3f(static_cast<float>(c.x), static_cast<float>(c.y), static_cast<float>(c.z));

            // Edge c -> a
            glVertex3f(static_cast<float>(c.x), static_cast<float>(c.y), static_cast<float>(c.z));
            glVertex3f(static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z));
        }
        glEnd();

        // Swap the hidden buffer we just drew into to the screen
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
