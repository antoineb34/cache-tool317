#pragma once

#include <vector>
#include <cstdint>
#include "Buffer.h"

struct MapObject {
    int id;
    int x, y, z;
    int type;
    int rotation;
};

class MapObjects {
public:
    static MapObjects parse(Buffer& buf);
    const std::vector<MapObject>& getObjects() const;

private:
    std::vector<MapObject> objects;
};
