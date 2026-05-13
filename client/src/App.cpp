#include "App.h"

#include <iostream>

App::App()
    : window_(nullptr)
    , glContext_(nullptr)
    , running_(false)
    , vao_(0)
    , vbo_(0)
    , shaderProgram_(0)
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

    if (!initTriangle()) {
        std::cerr << "Failed to init triangle" << std::endl;
        return false;
    }

    running_ = true;
    return true;
}

bool App::initTriangle() {
    // Vertex shader — pass-through
    const char* vsSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
void main() {
    gl_Position = vec4(aPos, 1.0);
}
)";

    // Fragment shader — solid white
    const char* fsSource = R"(
#version 330 core
out vec4 fragColor;
void main() {
    fragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
)";

    GLuint vs = createShader(GL_VERTEX_SHADER, vsSource);
    if (!vs) return false;

    GLuint fs = createShader(GL_FRAGMENT_SHADER, fsSource);
    if (!fs) {
        glDeleteShader(vs);
        return false;
    }

    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vs);
    glAttachShader(shaderProgram_, fs);
    glLinkProgram(shaderProgram_);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok;
    glGetProgramiv(shaderProgram_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(shaderProgram_, sizeof(log), nullptr, log);
        std::cerr << "Shader link error: " << log << std::endl;
        glDeleteProgram(shaderProgram_);
        shaderProgram_ = 0;
        return false;
    }

    // Triangle vertices (NDC)
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "GL error after triangle init: 0x" << std::hex << err << std::endl;
        return false;
    }

    return true;
}

GLuint App::createShader(GLenum type, const char* source) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &source, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::cerr << "Shader compile error: " << log << std::endl;
        glDeleteShader(s);
        return 0;
    }
    return s;
}

void App::destroyTriangle() {
    if (shaderProgram_) {
        glDeleteProgram(shaderProgram_);
        shaderProgram_ = 0;
    }
    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
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

    glUseProgram(shaderProgram_);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    SDL_GL_SwapWindow(window_);
}

void App::shutdown() {
    destroyTriangle();
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