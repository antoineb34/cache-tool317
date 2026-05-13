#pragma once

#include "TextureDef.h"
#include "Archive.h"
#include <vector>

struct TextureLoader {
    void load(const Archive& texArchive);

    const TextureDef& getTexture(int id) const;
    int textureCount() const { return (int)textures_.size(); }

private:
    std::vector<TextureDef> textures_;
};
