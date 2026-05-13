#include "WireframeMeshRenderer.h"

#include <iostream>

WireframeMeshRenderer::WireframeMeshRenderer() = default;

WireframeMeshRenderer::~WireframeMeshRenderer() {
    shutdown();
}

bool WireframeMeshRenderer::init() {
    const char* vsSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
void main() {
    gl_Position = vec4(aPos, 1.0);
}
)";

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

    // Hardcoded wireframe square (4 line segments, 8 vertices)
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,

         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,

         0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f,

        -0.5f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f
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
        std::cerr << "GL error after WireframeMeshRenderer init: 0x" << std::hex << err << std::endl;
        return false;
    }

    initialized_ = true;
    return true;
}

GLuint WireframeMeshRenderer::createShader(GLenum type, const char* source) {
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

void WireframeMeshRenderer::render() {
    if (!initialized_) return;

    glUseProgram(shaderProgram_);
    glBindVertexArray(vao_);
    glDrawArrays(GL_LINES, 0, 8);
    glBindVertexArray(0);
    glUseProgram(0);
}

void WireframeMeshRenderer::shutdown() {
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
    initialized_ = false;
}