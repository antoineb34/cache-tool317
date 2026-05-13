#include "TextureLoader.h"
#include <stdexcept>
#include <string>

void TextureLoader::load(const Archive& texArchive) {
    // Get the palette from index.dat
    const Buffer& indexData = texArchive.getFile("index.dat");
    if (indexData.empty()) {
        throw std::runtime_error("TextureLoader: index.dat not found in textures archive");
    }

    // Textures are stored as "0.dat", "1.dat", etc.
    // We need to find all texture files and load them
    // RS317 has a fixed number of textures (typically 1260 based on palette size / 3)
    int maxTextures = 1260;  // Based on 3780 byte palette / 3 = 1260 colors

    textures_.clear();

    for (int id = 0; id < maxTextures; id++) {
        std::string name = std::to_string(id) + ".dat";
        const Buffer& texData = texArchive.getFile(name);

        if (texData.empty()) {
            // Texture doesn't exist, add empty placeholder
            TextureDef empty;
            empty.id = id;
            textures_.push_back(empty);
            continue;
        }

        try {
            // Create copies for parsing (need to advance positions)
            Buffer dataCopy(texData.slice(0, texData.size()));
            Buffer paletteCopy(indexData.slice(0, indexData.size()));
            TextureDef tex = TextureDef::parse(id, dataCopy, paletteCopy);
            textures_.push_back(tex);
        } catch (const std::exception& e) {
            // If parsing fails, add empty placeholder
            TextureDef empty;
            empty.id = id;
            textures_.push_back(empty);
        }
    }
}

const TextureDef& TextureLoader::getTexture(int id) const {
    static TextureDef nullTex;
    if (id < 0 || id >= (int)textures_.size()) return nullTex;
    return textures_[id];
}
