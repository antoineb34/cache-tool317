#include "MapTerrain.h"

#include <cmath>
#include <stdexcept>

MapTerrain MapTerrain::parse(Buffer& buf) {
    return parse(buf, 0);
}

static int interpolatedNoise(int x, int y, int scale) {
    int scaledX = x / scale;
    int localX = x & (scale - 1);
    int scaledY = y / scale;
    int localY = y & (scale - 1);

    auto noise = [](int nx, int ny) {
        int n = nx + ny * 57;
        n ^= n << 13;
        return (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
    };

    auto smoothNoise = [&](int nx, int ny) {
        int corners = noise(nx - 1, ny - 1) + noise(nx + 1, ny - 1) +
                      noise(nx - 1, ny + 1) + noise(nx + 1, ny + 1);
        int sides = noise(nx - 1, ny) + noise(nx + 1, ny) +
                    noise(nx, ny - 1) + noise(nx, ny + 1);
        int center = noise(nx, ny);
        return corners / 16 + sides / 8 + center / 4;
    };

    auto interpolate = [](int a, int b, int fraction, int scale) {
        int cosine = 65536 - static_cast<int>(std::cos(fraction * 3.14159265358979323846 / scale) * 65536.0);
        cosine >>= 1;
        return ((a * (65536 - cosine)) >> 16) + ((b * cosine) >> 16);
    };

    int v1 = smoothNoise(scaledX, scaledY);
    int v2 = smoothNoise(scaledX + 1, scaledY);
    int v3 = smoothNoise(scaledX, scaledY + 1);
    int v4 = smoothNoise(scaledX + 1, scaledY + 1);
    int i1 = interpolate(v1, v2, localX, scale);
    int i2 = interpolate(v3, v4, localX, scale);
    return interpolate(i1, i2, localY, scale);
}

int MapTerrain::generateHeight(int worldX, int worldY) {
    int height = interpolatedNoise(worldX + 45365, worldY + 91923, 4) - 128;
    height += (interpolatedNoise(worldX + 10294, worldY + 37821, 2) - 128) >> 1;
    height += (interpolatedNoise(worldX, worldY, 1) - 128) >> 2;
    height = static_cast<int>(height * 0.3) + 35;
    if (height < 10) height = 10;
    if (height > 60) height = 60;
    return height;
}

MapTerrain MapTerrain::parse(Buffer& buf, int regionId) {
    MapTerrain terrain;
    int baseX = (regionId >> 8) * 64;
    int baseY = (regionId & 0xff) * 64;

    for (int plane = 0; plane < 4; plane++) {
        for (int x = 0; x < 64; x++) {
            for (int y = 0; y < 64; y++) {
                while (true) {
                    if (buf.isEmpty())
                        throw std::runtime_error("Terrain data ended while parsing region tile stream");

                    uint8_t opcode = buf.readByte();
                    if (opcode == 0) {
                        if (plane == 0) {
                            terrain.tiles[plane][x][y].height = -generateHeight(baseX + x, baseY + y) * 8;
                        } else {
                            terrain.tiles[plane][x][y].height = terrain.tiles[plane - 1][x][y].height - 240;
                        }
                        break; // End of tile
                    } else if (opcode == 1) {
                        int height = buf.readByte();
                        if (height == 1) height = 0;
                        if (plane == 0) {
                            terrain.tiles[plane][x][y].height = -height * 8;
                        } else {
                            terrain.tiles[plane][x][y].height = terrain.tiles[plane - 1][x][y].height - height * 8;
                        }
                        break; 
                    } else if (opcode <= 49) {
                        terrain.tiles[plane][x][y].overlayId = buf.readByte();
                        terrain.tiles[plane][x][y].overlayPath = (opcode - 2) / 4;
                        terrain.tiles[plane][x][y].overlayRotation = (opcode - 2) & 3;
                    } else if (opcode <= 81) {
                        terrain.tiles[plane][x][y].settings = opcode - 49;
                    } else {
                        terrain.tiles[plane][x][y].underlayId = opcode - 81;
                    }
                }
            }
        }
    }
    return terrain;
}

const Tile& MapTerrain::getTile(int plane, int x, int y) const {
    return tiles[plane][x][y];
}
