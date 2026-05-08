#pragma once

#include <vector>
#include <cstdint>
#include "Buffer.h"

struct MapObject {
    int id;
    int x, y, z;
    int type;
    int rotation;
    uint32_t idDelta = 0;
    uint32_t locationDelta = 0;
    int packedLocation = 0;
    int byteStart = 0;
    int byteEnd = 0;
    std::vector<uint8_t> bytes;
};

class MapObjects {
public:
    static MapObjects parse(Buffer& buf);
    const std::vector<MapObject>& getObjects() const;

private:
    std::vector<MapObject> objects;
};
