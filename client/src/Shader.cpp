#include "Shader.h"

#include <SDL3/SDL_opengl.h>
#include <iostream>

GLuint Shader::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool Shader::compile(const std::string& vertSrc, const std::string& fragSrc) {
    GLuint vert = compileShader(GL_VERTEX_SHADER, vertSrc);
    if (!vert) return false;

    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    if (!frag) {
        glDeleteShader(vert);
        return false;
    }

    id_ = glCreateProgram();
    glAttachShader(id_, vert);
    glAttachShader(id_, frag);
    glLinkProgram(id_);

    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint success;
    glGetProgramiv(id_, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(id_, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Shader link failed: " << infoLog << std::endl;
        glDeleteProgram(id_);
        id_ = 0;
        return false;
    }
    return true;
}

void Shader::setUniform(const std::string& name, int v) const {
    glUniform1i(glGetUniformLocation(id_, name.c_str()), v);
}

void Shader::setUniform(const std::string& name, float v) const {
    glUniform1f(glGetUniformLocation(id_, name.c_str()), v);
}

void Shader::setUniform(const std::string& name, float v0, float v1, float v2) const {
    glUniform3f(glGetUniformLocation(id_, name.c_str()), v0, v1, v2);
}

void Shader::setUniformMatrix4fv(const std::string& name, const float* m) const {
    glUniformMatrix4fv(glGetUniformLocation(id_, name.c_str()), 1, GL_FALSE, m);
}