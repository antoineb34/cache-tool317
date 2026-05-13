#include "TextureDef.h"
#include "Buffer.h"
#include <stdexcept>
#include <string>

uint32_t TextureDef::getPixelRGB(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height || pixels.empty() || palette.size() < 768) {
        return 0xFF00FF; // magenta for invalid
    }

    uint8_t idx = pixels[y * width + x];
    uint8_t r = palette[idx * 3];
    uint8_t g = palette[idx * 3 + 1];
    uint8_t b = palette[idx * 3 + 2];

    return (r << 16) | (g << 8) | b;
}

TextureDef TextureDef::parse(int id, Buffer& data, Buffer& paletteData) {
    TextureDef def;
    def.id = id;

    // RS317 textures are 128x128
    def.width = 128;
    def.height = 128;

    // Texture data format:
    //   bytes 0-1: header (unknown, ignore)
    //   bytes 2-16385: 128*128 pixel indices (0..N)
    if (data.size() < 2 + 128 * 128) {
        throw std::runtime_error("TextureDef " + std::to_string(id) + ": data too small (need at least " + std::to_string(2 + 128 * 128) + " bytes, got " + std::to_string(data.size()) + ")");
    }

    data.skip(2); // Skip header
    
    // Read 128*128 pixel indices
    def.pixels.resize(128 * 128);
    for (int i = 0; i < 128 * 128; i++) {
        def.pixels[i] = data.readByte();
    }

    // Palette comes from index.dat in the textures archive
    // Format: 3 bytes per color (R, G, B) for 256 colors = 768 bytes
    if (paletteData.size() >= 768) {
        def.palette.resize(768);
        for (int i = 0; i < 768; i++) {
            def.palette[i] = paletteData.readByte();
        }
    }

    return def;
}
