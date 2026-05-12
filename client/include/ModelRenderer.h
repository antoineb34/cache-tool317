#pragma once
#include <GL/gl.h>
#include "Mat4.h"
#include "ModelDef.h"

struct ModelRenderer {
    void load(const ModelDef& model);
    void draw(const Mat4& transform) const;
    void cleanup();

    float boundingRadius() const { return radius_; }
    float centerX() const { return cx_; }
    float centerY() const { return cy_; }
    float centerZ() const { return cz_; }

private:
    GLuint vao_    = 0;
    GLuint vbo_    = 0;
    GLuint prog_   = 0;
    int    count_  = 0;
    float  cx_ = 0, cy_ = 0, cz_ = 0;
    float  radius_ = 0;
};
