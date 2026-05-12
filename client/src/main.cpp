// GL_GLEXT_PROTOTYPES tells the headers to declare all modern GL functions
// directly — on Linux/Mesa, libGL exports them all so no loader is needed
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <SDL3/SDL.h>
#include <iostream>

// ---------------------------------------------------------------------------
// Shaders
//
// The vertex shader runs once per vertex. It receives our (x,y,z) position
// and outputs gl_Position — where on screen that vertex lands.
// Clip space means (0,0) is center, (-1,-1) is bottom-left, (1,1) top-right.
//
// The fragment shader runs once per pixel inside the triangle.
// It just outputs a constant white color for now.
// ---------------------------------------------------------------------------

static const char* vertSrc = R"(
#version 330 core
layout(location = 0) in vec3 pos;
void main() {
    gl_Position = vec4(pos, 1.0);
}
)";

static const char* fragSrc = R"(
#version 330 core
out vec4 color;
void main() {
    color = vec4(1.0, 1.0, 1.0, 1.0);
}
)";

static GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader error: " << log << "\n";
    }
    return shader;
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    // tell SDL we want OpenGL 3.3 core — must be set before creating the window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow("cache-tool317", 800, 600, SDL_WINDOW_OPENGL);
    if (!window) { std::cerr << "Window failed: " << SDL_GetError() << "\n"; return 1; }

    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    if (!ctx) { std::cerr << "GL context failed: " << SDL_GetError() << "\n"; return 1; }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";

    // ----- shaders -----
    GLuint vert    = compileShader(GL_VERTEX_SHADER,   vertSrc);
    GLuint frag    = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    glDeleteShader(vert);  // GPU has them now, we don't need these handles
    glDeleteShader(frag);

    // ----- geometry -----
    // 3 vertices in clip space. No matrix math yet — positions go straight to screen.
    float verts[] = {
         0.0f,  0.5f, 0.0f,   // top center
        -0.5f, -0.5f, 0.0f,   // bottom left
         0.5f, -0.5f, 0.0f,   // bottom right
    };

    // VAO remembers the vertex layout so we don't have to re-describe it every frame
    // VBO is the actual vertex data living on the GPU
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    // attribute 0 = the "pos" in our vertex shader
    // 3 floats, no padding, starting at byte 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);  // unbind so nothing accidentally modifies it

    // ----- render loop -----
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e))
            if (e.type == SDL_EVENT_QUIT) running = false;

        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);  // draw 3 vertices = 1 triangle

        SDL_GL_SwapWindow(window);
    }

    // cleanup
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(program);
    SDL_GL_DestroyContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
