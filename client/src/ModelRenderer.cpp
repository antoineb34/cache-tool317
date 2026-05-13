#define GL_GLEXT_PROTOTYPES
#include "ModelRenderer.h"
#include "Shader.h"
#include <GL/glext.h>
#include <cmath>
#include <algorithm>
#include <vector>

// --- Shaders for colored faces ---
static const char* VERT_COLOR = R"(
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

static const char* FRAG_COLOR = R"(
#version 330 core
in  vec3 vColor;
out vec4 fragColor;
void main() {
    fragColor = vec4(vColor, 1.0);
}
)";

// --- Shaders for textured faces ---
static const char* VERT_TEX = R"(
#version 330 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 texCoord;
uniform mat4 transform;
out vec2 vTexCoord;
void main() {
    gl_Position = transform * vec4(pos, 1.0);
    vTexCoord = texCoord;
}
)";

static const char* FRAG_TEX = R"(
#version 330 core
in  vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D tex;
void main() {
    fragColor = texture(tex, vTexCoord);
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

GLuint ModelRenderer::createGLTexture(const TextureDef& tex) const {
    if (tex.pixels.empty() || tex.palette.size() < 3) return 0;

    // Convert palette indices to RGBA
    std::vector<uint8_t> rgba(tex.width * tex.height * 4);
    for (int i = 0; i < tex.width * tex.height; i++) {
        uint8_t idx = tex.pixels[i];
        if (idx * 3 + 2 < tex.palette.size()) {
            rgba[i*4+0] = tex.palette[idx*3+0]; // R
            rgba[i*4+1] = tex.palette[idx*3+1]; // G
            rgba[i*4+2] = tex.palette[idx*3+2]; // B
            rgba[i*4+3] = 255;                   // A
        } else {
            rgba[i*4+0] = 255;
            rgba[i*4+1] = 0;
            rgba[i*4+2] = 255; // magenta for invalid
            rgba[i*4+3] = 255;
        }
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

void ModelRenderer::loadColoredFaces(const ModelDef& model) {
    progColor_ = buildProgram(VERT_COLOR, FRAG_COLOR);

    std::vector<float> verts;
    int coloredTriCount = 0;
    for (int i = 0; i < (int)model.triA.size(); i++) {
        if (!model.isFaceTextured(i)) coloredTriCount++;
    }
    verts.reserve(coloredTriCount * 3 * 6);

    for (int i = 0; i < (int)model.triA.size(); i++) {
        if (model.isFaceTextured(i)) continue;

        float r, g, b;
        hslToRgb(model.triColor[i], r, g, b);

        uint16_t corners[3] = { model.triA[i], model.triB[i], model.triC[i] };
        for (uint16_t vi : corners) {
            verts.push_back((float) model.vertexX[vi]);
            verts.push_back((float)-model.vertexY[vi]);
            verts.push_back((float) model.vertexZ[vi]);
            verts.push_back(r); verts.push_back(g); verts.push_back(b);
        }
    }

    countColor_ = coloredTriCount * 3;

    glGenVertexArrays(1, &vaoColor_);
    glGenBuffers(1, &vboColor_);

    glBindVertexArray(vaoColor_);
      glBindBuffer(GL_ARRAY_BUFFER, vboColor_);
      glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
      glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void ModelRenderer::loadTexturedFaces(const ModelDef& model, const std::vector<TextureDef>& textures) {
    progTex_ = buildProgram(VERT_TEX, FRAG_TEX);

    // Collect textured faces and their texture IDs
    std::vector<int> texFaceIndices;
    faceTextureIds_.clear();

    for (int i = 0; i < (int)model.triA.size(); i++) {
        if (model.isFaceTextured(i)) {
            texFaceIndices.push_back(i);
            int texIdx = model.faceTexTriIndex(i);
            faceTextureIds_.push_back(texIdx);
        }
    }

    if (texFaceIndices.empty()) {
        countTex_ = 0;
        return;
    }

    // Build vertices with texture coordinates
    // For each textured face, we need to compute UV coords from texP/Q/R
    // This is a simplified version - in RS317, UV mapping uses the texture triangle indices
    std::vector<float> verts;
    verts.reserve(texFaceIndices.size() * 3 * 5); // x,y,z + u,v per vertex

    for (size_t j = 0; j < texFaceIndices.size(); j++) {
        int i = texFaceIndices[j];

        // Get the texture triangle index
        int texTriIdx = model.faceTexTriIndex(i);
        if (texTriIdx * 3 + 2 >= (int)model.texP.size()) continue;

        // Texture coordinates are implicit in RS317: the texture triangle defines
        // how the 3D face maps to the 2D texture. For now, use simple mapping:
        // Map each vertex to a corner of the texture (0,0), (1,0), (0,1)
        // TODO: Proper UV mapping using texP/Q/R vertices

        uint16_t corners[3] = { model.triA[i], model.triB[i], model.triC[i] };
        float uvs[3][2] = {{0,0}, {1,0}, {0,1}}; // Simplified

        for (int k = 0; k < 3; k++) {
            uint16_t vi = corners[k];
            verts.push_back((float) model.vertexX[vi]);
            verts.push_back((float)-model.vertexY[vi]);
            verts.push_back((float) model.vertexZ[vi]);
            verts.push_back(uvs[k][0]);
            verts.push_back(uvs[k][1]);
        }
    }

    countTex_ = (int)texFaceIndices.size() * 3;

    glGenVertexArrays(1, &vaoTex_);
    glGenBuffers(1, &vboTex_);

    glBindVertexArray(vaoTex_);
      glBindBuffer(GL_ARRAY_BUFFER, vboTex_);
      glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
      glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Create OpenGL textures for used texture IDs
    std::vector<int> uniqueTexIds = faceTextureIds_;
    std::sort(uniqueTexIds.begin(), uniqueTexIds.end());
    uniqueTexIds.erase(std::unique(uniqueTexIds.begin(), uniqueTexIds.end()), uniqueTexIds.end());

    glTextures_.resize(textures.size(), 0);
    for (int texId : uniqueTexIds) {
        if (texId >= 0 && texId < (int)textures.size() && !textures[texId].pixels.empty()) {
            glTextures_[texId] = createGLTexture(textures[texId]);
        }
    }
}

void ModelRenderer::load(const ModelDef& model, const std::vector<TextureDef>& textures) {
    // Compute bounding sphere
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

    // Load colored and textured faces
    loadColoredFaces(model);
    loadTexturedFaces(model, textures);
}

void ModelRenderer::draw(const Mat4& transform) const {
    // Draw colored faces
    if (countColor_ > 0) {
        glUseProgram(progColor_);
        GLint loc = glGetUniformLocation(progColor_, "transform");
        glUniformMatrix4fv(loc, 1, GL_FALSE, transform.m);
        glBindVertexArray(vaoColor_);
        glDrawArrays(GL_TRIANGLES, 0, countColor_);
    }

    // Draw textured faces
    if (countTex_ > 0) {
        glUseProgram(progTex_);
        GLint loc = glGetUniformLocation(progTex_, "transform");
        glUniformMatrix4fv(loc, 1, GL_FALSE, transform.m);

        // For simplicity, bind texture 0 for all textured faces
        // TODO: Proper per-face texture binding
        GLint texLoc = glGetUniformLocation(progTex_, "tex");
        glActiveTexture(GL_TEXTURE0);
        if (!glTextures_.empty() && glTextures_[0] != 0) {
            glBindTexture(GL_TEXTURE_2D, glTextures_[0]);
        }
        glUniform1i(texLoc, 0);

        glBindVertexArray(vaoTex_);
        glDrawArrays(GL_TRIANGLES, 0, countTex_);
    }
}

void ModelRenderer::cleanup() {
    if (vaoColor_) glDeleteVertexArrays(1, &vaoColor_);
    if (vboColor_) glDeleteBuffers(1, &vboColor_);
    if (vaoTex_)    glDeleteVertexArrays(1, &vaoTex_);
    if (vboTex_)    glDeleteBuffers(1, &vboTex_);
    for (GLuint tex : glTextures_) {
        if (tex) glDeleteTextures(1, &tex);
    }
    if (progColor_) glDeleteProgram(progColor_);
    if (progTex_)   glDeleteProgram(progTex_);
}
