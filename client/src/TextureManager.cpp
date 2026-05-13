#include "TextureManager.h"

#include <SDL3/SDL_opengl.h>
#include "TextureDef.h"
#include "Archive.h"
#include "Buffer.h"

#include <iostream>
#include <cstring>

TextureManager::~TextureManager() {
    for (auto& [id, tex] : glTextures_) {
        glDeleteTextures(1, &tex);
    }
    glTextures_.clear();
    pixelData_.clear();
}

bool TextureManager::load(const Archive& texArchive) {
    // Load the palette from index.dat
    const Buffer& indexData = texArchive.getFile("index.dat");
    if (indexData.empty()) {
        std::cerr << "TextureManager: index.dat not found in texture archive" << std::endl;
        return false;
    }

    // Parse palette: index.dat contains 256 entries of 3 bytes each (R, G, B) = 768 bytes
    // But index.dat also contains other data; the palette is the first 768 bytes
    Buffer paletteBuf(indexData.slice(0, std::min(static_cast<int>(indexData.size()), 768)));

    // We need a palette buffer for TextureDef parsing
    Buffer paletteCopy(paletteBuf.slice(0, paletteBuf.size()));

    // Load up to 50 textures (IDs 0-49)
    int loaded = 0;
    for (int i = 0; i < 50; i++) {
        std::string name = std::to_string(i) + ".dat";
        if (!texArchive.hasFile(name)) continue;

        const Buffer& texBuf = texArchive.getFile(name);
        if (texBuf.empty()) continue;

        // Create copies since TextureDef::parse advances the buffer position
        Buffer dataCopy(texBuf.slice(0, texBuf.size()));
        Buffer paletteCopy2(paletteBuf.slice(0, paletteBuf.size()));

        try {
            TextureDef texDef = TextureDef::parse(i, dataCopy, paletteCopy2);

            // Convert palette-indexed pixels to RGBA
            std::vector<uint8_t> rgba(128 * 128 * 4);
            for (int py = 0; py < 128; py++) {
                for (int px = 0; px < 128; px++) {
                    uint8_t idx = texDef.pixels[py * 128 + px];
                    uint32_t rgb = texDef.getPixelRGB(px, py);
                    int offset = (py * 128 + px) * 4;
                    rgba[offset + 0] = (rgb >> 16) & 0xFF;
                    rgba[offset + 1] = (rgb >> 8) & 0xFF;
                    rgba[offset + 2] = rgb & 0xFF;
                    rgba[offset + 3] = (idx == 0) ? 0 : 255; // Transparency for color 0
                }
            }

            // Create OpenGL texture
            GLuint glTex;
            glGenTextures(1, &glTex);
            glBindTexture(GL_TEXTURE_2D, glTex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

            glTextures_[i] = glTex;
            pixelData_[i] = std::move(rgba);
            loaded++;
        } catch (const std::exception& e) {
            std::cerr << "TextureManager: failed to load texture " << i << ": " << e.what() << std::endl;
        }
    }

    std::cout << "TextureManager: loaded " << loaded << " textures" << std::endl;
    return loaded > 0;
}

bool TextureManager::bind(int texId) const {
    auto it = glTextures_.find(texId);
    if (it == glTextures_.end()) return false;
    glBindTexture(GL_TEXTURE_2D, it->second);
    return true;
}

const std::vector<uint8_t>* TextureManager::getPixels(int texId) const {
    auto it = pixelData_.find(texId);
    return (it != pixelData_.end()) ? &it->second : nullptr;
}