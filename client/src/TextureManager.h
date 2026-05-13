#pragma once

#include <unordered_map>
#include <vector>
#include <cstdint>
#include <SDL3/SDL_opengl.h>

struct TextureDef;
struct Archive;
class Buffer;

class TextureManager {
public:
    TextureManager() = default;
    ~TextureManager();

    bool load(const Archive& texArchive);
    bool bind(int texId) const;
    const std::vector<uint8_t>* getPixels(int texId) const;

    int count() const { return static_cast<int>(glTextures_.size()); }

private:
    bool loadTexture(int texId, const Archive& archive);

    std::unordered_map<int, GLuint> glTextures_;
    std::unordered_map<int, std::vector<uint8_t>> pixelData_;
};