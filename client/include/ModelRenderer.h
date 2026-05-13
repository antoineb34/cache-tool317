#pragma once
#include <GL/gl.h>
#include "Mat4.h"
#include "ModelDef.h"
#include "TextureDef.h"
#include <vector>

struct ModelRenderer {
    void load(const ModelDef& model, const std::vector<TextureDef>& textures);
    void draw(const Mat4& transform) const;
    void cleanup();

    float boundingRadius() const { return radius_; }
    float centerX() const { return cx_; }
    float centerY() const { return cy_; }
    float centerZ() const { return cz_; }

private:
    void loadColoredFaces(const ModelDef& model);
    void loadTexturedFaces(const ModelDef& model, const std::vector<TextureDef>& textures);
    GLuint createGLTexture(const TextureDef& tex) const;

    // Colored faces (non-textured)
    GLuint vaoColor_ = 0;
    GLuint vboColor_ = 0;
    int    countColor_ = 0;

    // Textured faces
    GLuint vaoTex_ = 0;
    GLuint vboTex_ = 0;
    int    countTex_ = 0;
    std::vector<GLuint> glTextures_;
    std::vector<int> faceTextureIds_;  // texture ID per textured face

    GLuint progColor_ = 0;
    GLuint progTex_   = 0;

    float  cx_ = 0, cy_ = 0, cz_ = 0;
    float  radius_ = 0;
};
