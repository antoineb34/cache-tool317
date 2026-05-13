// Full 3D game client for RS317 cache tool
// Uses SDL3 for windowing/input, OpenGL for rendering
// (GL_GLEXT_PROTOTYPES is set via CMake)

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <iostream>
#include <string>
#include <cstdint>
#include <cmath>

// Core cache libraries
#include "CacheReader.h"
#include "ModelDef.h"
#include "TextureDef.h"
#include "DefinitionsLoader.h"
#include "VersionList.h"
#include "MapRegion.h"
#include "Archive.h"
#include "FloDef.h"

// Our client modules
#include "Math.h"
#include "Shader.h"
#include "Camera.h"
#include "TextureManager.h"
#include "ModelRenderer.h"
#include "WorldRenderer.h"

// ============================================
// Globals
// ============================================
static bool g_running = true;
static Camera g_camera;
static bool g_firstMouse = true;
static float g_lastMouseX = 0.0f;
static float g_lastMouseY = 0.0f;
static bool g_mouseCaptured = false;
static float g_moveSpeed = 200.0f;

// Cache data
static CacheReader g_cacheReader;
static DefinitionsLoader g_defs;
static TextureManager g_texMgr;
static WorldRenderer g_worldRenderer;
static ModelRenderer g_modelRenderer;

// Current region
static int g_currentRegionId = 12850;
static bool g_worldLoaded = false;

// OpenGL shader program
static Shader g_shader;
static const char* g_uniformMVP  = "uMVP";
static const char* g_uniformTex  = "uTexEnabled";

// OpenGL VAO/VBO for a unit cube (used for object placeholders)
static GLuint g_cubeVAO = 0;
static GLuint g_cubeVBO = 0;
static GLuint g_cubeEBO = 0;

// ============================================
// Shader source code
// ============================================

static const char* g_vertShaderSrc = R"GLSL(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in float aHasTex;

uniform mat4 uMVP;

out vec3 vColor;
out vec2 vTexCoord;
out float vHasTex;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
    vTexCoord = aTexCoord;
    vHasTex = aHasTex;
}
)GLSL";

static const char* g_fragShaderSrc = R"GLSL(
#version 330 core
in vec3 vColor;
in vec2 vTexCoord;
in float vHasTex;

uniform sampler2D uTexture;
uniform int uTexEnabled;

out vec4 FragColor;

void main() {
    if (uTexEnabled == 1 && vHasTex > 0.5) {
        vec4 texColor = texture(uTexture, vTexCoord);
        if (texColor.a < 0.1) discard;
        FragColor = vec4(texColor.rgb * vColor, 1.0);
    } else {
        FragColor = vec4(vColor / 255.0, 1.0);
    }
}
)GLSL";

// ============================================
// OpenGL helpers
// ============================================

static bool initOpenGL(SDL_Window* window) {
    // Create OpenGL context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        return false;
    }

    // Verify that the GL function pointers are available
    // (SDL_GL_GetProcAddress returns non-null for core functions in GL 3.3+)
    if (!SDL_GL_GetProcAddress("glGenVertexArrays") ||
        !SDL_GL_GetProcAddress("glBindBuffer") ||
        !SDL_GL_GetProcAddress("glUseProgram")) {
        std::cerr << "Required OpenGL functions not available (need GL 3.3+)" << std::endl;
        return false;
    }

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Enable backface culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Compile shader
    if (!g_shader.compile(g_vertShaderSrc, g_fragShaderSrc)) {
        std::cerr << "Failed to compile shader" << std::endl;
        return false;
    }
    g_shader.use();
    g_shader.setUniform(g_uniformMVP, 0);
    g_shader.setUniform(g_uniformTex, 0);

    return true;
}

static void buildUnitCube() {
    // Unit cube centered at origin, size 1.0
    float s = 0.5f;
    float verts[] = {
        // positions          // colors       // texcoords
        -s, -s, -s,  1,1,1,  0,0,
         s, -s, -s,  1,1,1,  1,0,
         s,  s, -s,  1,1,1,  1,1,
        -s,  s, -s,  1,1,1,  0,1,
        -s, -s,  s,  1,1,1,  0,0,
         s, -s,  s,  1,1,1,  1,0,
         s,  s,  s,  1,1,1,  1,1,
        -s,  s,  s,  1,1,1,  0,1
    };
    GLuint indices[] = {
        0,1,2, 0,2,3,  // back
        4,6,5, 4,7,6,  // front
        0,4,5, 0,5,1,  // bottom
        2,6,7, 2,7,3,  // top
        0,3,7, 0,7,4,  // left
        1,5,6, 1,6,2   // right
    };

    glGenVertexArrays(1, &g_cubeVAO);
    glGenBuffers(1, &g_cubeVBO);
    glGenBuffers(1, &g_cubeEBO);

    glBindVertexArray(g_cubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, g_cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    // Color attribute (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    // TexCoord attribute (location = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    // HasTexture attribute (location = 3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(8 * sizeof(float)));

    glBindVertexArray(0);
}

// ============================================
// Cache loading
// ============================================

static bool loadCache(const std::string& cachePath) {
    std::cout << "Opening cache: " << cachePath << std::endl;

    if (!g_cacheReader.open(cachePath)) {
        std::cerr << "Failed to open cache" << std::endl;
        return false;
    }

    // Load definitions archive (archive 0, file 2)
    auto defsArchive = g_cacheReader.readArchive(0, 2);
    if (!defsArchive) {
        std::cerr << "Failed to read definitions archive" << std::endl;
        return false;
    }

    // Load all definition types
    g_defs.loadLocs(*defsArchive);
    g_defs.loadFlos(*defsArchive);
    g_defs.loadItems(*defsArchive);
    g_defs.loadNpcs(*defsArchive);
    g_defs.loadSeqs(*defsArchive);
    g_defs.loadSpotAnims(*defsArchive);
    g_defs.loadIdks(*defsArchive);
    g_defs.loadParams(*defsArchive);
    g_defs.loadVarbits(*defsArchive);
    g_defs.loadVarps(*defsArchive);

    std::cout << "Loaded definitions:"
              << " locs=" << g_defs.locCount()
              << " flos=" << g_defs.floCount()
              << " items=" << g_defs.itemCount()
              << " npcs=" << g_defs.npcCount()
              << " seqs=" << g_defs.seqCount()
              << std::endl;

    // Load textures (archive 0, file 6)
    auto texArchive = g_cacheReader.readArchive(0, 6);
    if (texArchive) {
        g_texMgr.load(*texArchive);
        std::cout << "Loaded " << g_texMgr.count() << " textures" << std::endl;
    } else {
        std::cerr << "Warning: Could not load texture archive" << std::endl;
    }

    // Load version list for map
    auto verArchive = g_cacheReader.readArchive(0, 5);
    if (!verArchive) {
        std::cerr << "Failed to read version archive" << std::endl;
        return false;
    }
    VersionList vList = VersionList::parse(*verArchive);

    // Load a map region
    try {
        MapRegion region = MapRegion::load(g_cacheReader, vList, g_currentRegionId);
        g_worldRenderer.loadRegion(region, g_defs, &g_texMgr);
        g_worldLoaded = true;
        std::cout << "Loaded region " << g_currentRegionId << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load region: " << e.what() << std::endl;
    }

    // Load a sample model (id 100 is a common tree in RS317)
    if (g_cacheReader.hasFile(1, 100)) {
        try {
            g_modelRenderer.loadModel(g_cacheReader, 100, &g_texMgr);
            std::cout << "Loaded model 100" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Failed to load model 100: " << e.what() << std::endl;
        }
    }

    return true;
}

// ============================================
// Input handling
// ============================================

static void handleEvents(SDL_Window* window) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                g_running = false;
                break;

            case SDL_EVENT_KEY_DOWN:
                // SDL3: event.key.key is the SDL_Keycode (not keysym.sym)
                if (event.key.key == SDLK_ESCAPE) {
                    if (g_mouseCaptured) {
                        SDL_SetWindowRelativeMouseMode(window, 0);
                        g_mouseCaptured = false;
                        SDL_ShowCursor();
                    } else {
                        g_running = false;
                    }
                }
                if (event.key.key == SDLK_TAB) {
                    g_mouseCaptured = !g_mouseCaptured;
                    SDL_SetWindowRelativeMouseMode(window, g_mouseCaptured ? 1 : 0);
                    if (g_mouseCaptured) SDL_HideCursor();
                    else SDL_ShowCursor();
                    g_firstMouse = true;
                }
                break;

            case SDL_EVENT_MOUSE_MOTION:
                if (g_mouseCaptured) {
                    float xpos = static_cast<float>(event.motion.xrel);
                    float ypos = static_cast<float>(event.motion.yrel);

                    if (g_firstMouse) {
                        g_lastMouseX = xpos;
                        g_lastMouseY = ypos;
                        g_firstMouse = false;
                    }

                    float xoffset = xpos - g_lastMouseX;
                    float yoffset = g_lastMouseY - ypos;
                    g_lastMouseX = xpos;
                    g_lastMouseY = ypos;

                    static const float sensitivity = 0.15f;
                    xoffset *= sensitivity;
                    yoffset *= sensitivity;

                    g_camera.rotate(xoffset, yoffset);
                }
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                g_camera.zoom(static_cast<float>(event.wheel.y) * 10.0f);
                break;

            default:
                break;
        }
    }
}

static void handleMovement(float dt) {
    // SDL3 returns const bool* — cast to const Uint8* for array indexing
    const bool* stateBool = SDL_GetKeyboardState(nullptr);
    const Uint8* state = reinterpret_cast<const Uint8*>(stateBool);

    Vec3f forward, right;

    // Simple WASD movement relative to camera yaw
    float radYaw = g_camera.getYaw() * 3.14159265f / 180.0f;
    forward = Vec3f(-std::sin(radYaw), 0, -std::cos(radYaw));
    right = Vec3f(std::cos(radYaw), 0, -std::sin(radYaw));

    Vec3f moveDir(0, 0, 0);
    if (state[SDL_SCANCODE_W]) moveDir += forward;
    if (state[SDL_SCANCODE_S]) moveDir -= forward;
    if (state[SDL_SCANCODE_D]) moveDir += right;
    if (state[SDL_SCANCODE_A]) moveDir -= right;
    if (state[SDL_SCANCODE_SPACE]) moveDir.y += 1.0f;
    if (state[SDL_SCANCODE_LSHIFT]) moveDir.y -= 1.0f;

    if (moveDir.length() > 0.001f) {
        moveDir = moveDir.normalized() * g_moveSpeed * dt;
        g_camera.setTarget(g_camera.getPosition() + moveDir);
    }
}

// ============================================
// Rendering
// ============================================

static void render(int windowWidth, int windowHeight) {
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    g_shader.use();

    Mat4f mvp = g_camera.getViewProjection(windowWidth, windowHeight);
    g_shader.setUniformMatrix4fv(g_uniformMVP, mvp.m);

    // Draw world terrain and objects
    if (g_worldLoaded) {
        g_shader.setUniform(g_uniformTex, 0);
        g_worldRenderer.draw(mvp);
    }

    // Draw model (if loaded)
    if (g_modelRenderer.getBatchCount() > 0) {
        // Position the model in front of the camera
        Mat4f modelMat = Mat4f::translate(0, 0, -50);
        Mat4f modelMVP = mvp * modelMat;
        g_shader.setUniformMatrix4fv(g_uniformMVP, modelMVP.m);

        g_shader.setUniform(g_uniformTex, 1);
        g_modelRenderer.draw(modelMVP, false);
        g_modelRenderer.drawTransparent(modelMVP);

        // Reset MVP for world drawing
        g_shader.setUniformMatrix4fv(g_uniformMVP, mvp.m);
    }
}

// ============================================
// Main
// ============================================

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: client <cache path> [region_id]" << std::endl;
        return 1;
    }

    std::string cachePath = argv[1];
    if (argc >= 3) {
        g_currentRegionId = std::stoi(argv[2]);
    }

    // Initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create OpenGL window
    SDL_Window* window = SDL_CreateWindow(
        "cache-tool317 - RS317 Client",
        1280, 960,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Initialize OpenGL (SDL3 provides GL function prototypes via SDL_opengl.h)
    if (!initOpenGL(window)) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Build unit cube for object rendering
    buildUnitCube();

    // Set initial camera position
    g_camera.setTarget(Vec3f(0, 0, 0));
    g_camera.zoom(200.0f);

    // Load cache data
    if (!loadCache(cachePath)) {
        std::cerr << "Failed to load cache" << std::endl;
        // Continue anyway - might work with partial data
    }

    std::cout << "=== RS317 Client Running ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  TAB = capture/release mouse" << std::endl;
    std::cout << "  WASD = move, Space/Shift = up/down" << std::endl;
    std::cout << "  Mouse = look around, Scroll = zoom" << std::endl;
    std::cout << "  ESC = exit (or release mouse first)" << std::endl;

    // ============================================
    // Main loop
    // ============================================
    Uint64 lastTime = SDL_GetTicks();
    int frameCount = 0;
    double fpsUpdateTime = 0.0;

    while (g_running) {
        Uint64 currentTime = SDL_GetTicks();
        float dt = static_cast<float>(currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Cap delta time to prevent huge jumps
        if (dt > 0.1f) dt = 0.1f;

        // Handle input events
        handleEvents(window);

        // Handle continuous movement
        handleMovement(dt);

        // Render
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        glViewport(0, 0, w, h);
        render(w, h);

        // Swap buffers
        SDL_GL_SwapWindow(window);

        // FPS counter
        frameCount++;
        fpsUpdateTime += dt;
        if (fpsUpdateTime >= 1.0) {
            std::cout << "\rFPS: " << frameCount
                      << " | Region: " << g_currentRegionId
                      << " | Chunks: " << g_worldRenderer.getTerrainChunks()
                      << " | Models: " << g_modelRenderer.getBatchCount()
                      << "     " << std::flush;
            frameCount = 0;
            fpsUpdateTime = 0.0;
        }
    }

    // Cleanup
    std::cout << "\nShutting down..." << std::endl;

    if (g_cubeVAO) glDeleteVertexArrays(1, &g_cubeVAO);
    if (g_cubeVBO) glDeleteBuffers(1, &g_cubeVBO);
    if (g_cubeEBO) glDeleteBuffers(1, &g_cubeEBO);

    SDL_Quit();
    std::cout << "Client shut down cleanly." << std::endl;
    return 0;
}