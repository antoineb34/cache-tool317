#pragma once

#include <cstdint>
#include "Buffer.h"

struct Tile {
    int height = 0;
    uint8_t overlayId = 0;
    uint8_t overlayPath = 0;
    uint8_t overlayRotation = 0;
    uint8_t underlayId = 0;
    uint8_t settings = 0;
};

class MapTerrain {
public:
    static MapTerrain parse(Buffer& buf);
    static MapTerrain parse(Buffer& buf, int regionId);

    const Tile& getTile(int plane, int x, int y) const;

private:
    static int generateHeight(int worldX, int worldY);

    Tile tiles[4][64][64];
};
