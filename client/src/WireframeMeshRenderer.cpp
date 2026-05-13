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

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

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

bool WireframeMeshRenderer::uploadLines(const float* vertices, std::size_t floatCount) {
    if (!initialized_) return false;
    if (floatCount % 3 != 0) {
        std::cerr << "uploadLines: floatCount (" << floatCount << ") not divisible by 3" << std::endl;
        return false;
    }

    vertexCount_ = floatCount / 3;

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, floatCount * sizeof(float), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "GL error after uploadLines: 0x" << std::hex << err << std::endl;
        return false;
    }

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
    if (!initialized_ || vertexCount_ == 0) return;

    glUseProgram(shaderProgram_);
    glBindVertexArray(vao_);
    glDrawArrays(GL_LINES, 0, (GLsizei)vertexCount_);
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
    vertexCount_ = 0;
    initialized_ = false;
}