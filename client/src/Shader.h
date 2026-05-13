#pragma once

#include <string>
#include <SDL3/SDL_opengl.h>

class Shader {
public:
    Shader() : id_(0) {}
    ~Shader() { if (id_) glDeleteProgram(id_); }

    bool compile(const std::string& vertSrc, const std::string& fragSrc);
    void use() const { glUseProgram(id_); }
    void setUniform(const std::string& name, int v) const;
    void setUniform(const std::string& name, float v) const;
    void setUniform(const std::string& name, float v0, float v1, float v2) const;
    void setUniformMatrix4fv(const std::string& name, const float* m) const;
    GLuint id() const { return id_; }

private:
    GLuint id_ = 0;
    static GLuint compileShader(GLenum type, const std::string& source);
};