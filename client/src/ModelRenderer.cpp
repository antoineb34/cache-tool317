#define GL_GLEXT_PROTOTYPES
#include "ModelRenderer.h"
#include "Shader.h"
#include <GL/glext.h>
#include <cmath>
#include <algorithm>
#include <vector>

static const char* VERT_SRC = R"(
#version 330 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
uniform mat4 transform;
out vec3 vColor;
void main() {
    gl_Position = transform * vec4(pos, 1.0);
    vColor = color;
}
)";

static const char* FRAG_SRC = R"(
#version 330 core
in  vec3 vColor;
out vec4 fragColor;
void main() {
    fragColor = vec4(vColor, 1.0);
}
)";

static void hslToRgb(uint16_t packed, float& r, float& g, float& b) {
    float H = ((packed >> 10) & 0x3F) / 64.0f;
    float S = ((packed >>  7) & 0x07) /  7.0f;
    float L =  (packed        & 0x7F) / 127.0f;

    if (S == 0.0f) { r = g = b = L; return; }

    float C  = (1.0f - std::abs(2.0f * L - 1.0f)) * S;
    float X  = C * (1.0f - std::abs(std::fmod(H * 6.0f, 2.0f) - 1.0f));
    float m  = L - C * 0.5f;
    float h6 = H * 6.0f;

    if      (h6 < 1) { r=C+m; g=X+m; b=  m; }
    else if (h6 < 2) { r=X+m; g=C+m; b=  m; }
    else if (h6 < 3) { r=  m; g=C+m; b=X+m; }
    else if (h6 < 4) { r=  m; g=X+m; b=C+m; }
    else if (h6 < 5) { r=X+m; g=  m; b=C+m; }
    else             { r=C+m; g=  m; b=X+m; }
}

void ModelRenderer::load(const ModelDef& model) {
    prog_ = buildProgram(VERT_SRC, FRAG_SRC);

    // build vertex buffer: [x, y, z, r, g, b] per corner, 3 corners per triangle
    std::vector<float> verts;
    verts.reserve(model.triA.size() * 3 * 6);

    for (int i = 0; i < (int)model.triA.size(); i++) {
        float r, g, b;
        if (model.isFaceTextured(i)) {
            r = g = b = 0.5f;  // placeholder until texture rendering is implemented
        } else {
            hslToRgb(model.triColor[i], r, g, b);
        }

        uint16_t corners[3] = { model.triA[i], model.triB[i], model.triC[i] };
        for (uint16_t vi : corners) {
            verts.push_back((float) model.vertexX[vi]);
            verts.push_back((float)-model.vertexY[vi]);  // RS317 Y points down
            verts.push_back((float) model.vertexZ[vi]);
            verts.push_back(r); verts.push_back(g); verts.push_back(b);
        }
    }

    count_ = (int)model.triA.size() * 3;

    // compute bounding sphere for camera placement
    int nv = (int)model.vertexX.size();
    cx_ = cy_ = cz_ = 0;
    for (int i = 0; i < nv; i++) {
        cx_ += (float) model.vertexX[i];
        cy_ += (float)-model.vertexY[i];
        cz_ += (float) model.vertexZ[i];
    }
    cx_ /= nv; cy_ /= nv; cz_ /= nv;

    radius_ = 0;
    for (int i = 0; i < nv; i++) {
        float dx = (float)model.vertexX[i] - cx_;
        float dy = (float)-model.vertexY[i] - cy_;
        float dz = (float)model.vertexZ[i] - cz_;
        radius_ = std::max(radius_, std::sqrt(dx*dx + dy*dy + dz*dz));
    }

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
      glBindBuffer(GL_ARRAY_BUFFER, vbo_);
      glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
      glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void ModelRenderer::draw(const Mat4& transform) const {
    glUseProgram(prog_);
    GLint loc = glGetUniformLocation(prog_, "transform");
    glUniformMatrix4fv(loc, 1, GL_FALSE, transform.m);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, count_);
}

void ModelRenderer::cleanup() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
    glDeleteProgram(prog_);
}
