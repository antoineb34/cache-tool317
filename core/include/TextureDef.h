#pragma once

#include <cstdint>
#include <vector>
#include "Buffer.h"

struct TextureDef {
    int id = -1;

    int width = 0;
    int height = 0;
    std::vector<uint8_t> pixels;      // palette indices (0..255), row-major
    std::vector<uint8_t> palette;     // 768 bytes: R,G,B for each of 256 colors

    // Get RGB value for a pixel (applies palette)
    uint32_t getPixelRGB(int x, int y) const;

    // Parse from buffers (reads and advances the Buffer positions)
    static TextureDef parse(int id, Buffer& data, Buffer& paletteData);
};
