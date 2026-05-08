#include "MapObjects.h"

#include <stdexcept>

MapObjects MapObjects::parse(Buffer& buf) {
    MapObjects objects;
    int locId = -1;
    
    while (true) {
        if (buf.isEmpty())
            throw std::runtime_error("Object map data ended before final object id terminator");

        int idDeltaStart = buf.position();
        uint32_t idOffset = buf.readSmart();
        int idDeltaEnd = buf.position();
        if (idOffset == 0) break;
        locId += idOffset;

        int location = 0;
        bool firstPlacementForId = true;
        while (true) {
            if (buf.isEmpty())
                throw std::runtime_error("Object map data ended before placement terminator");

            int locationDeltaStart = buf.position();
            uint32_t locOffset = buf.readSmart();
            if (locOffset == 0) break;
            location += locOffset - 1;

            int z = (location >> 12) & 3;
            int x = (location >> 6) & 63;
            int y = location & 63;

            if (buf.isEmpty())
                throw std::runtime_error("Object map data ended before placement attributes");

            uint8_t attributes = buf.readByte();
            int type = attributes >> 2;
            int rotation = attributes & 3;

            int byteStart = firstPlacementForId ? idDeltaStart : locationDeltaStart;
            int byteEnd = buf.position();
            objects.objects.push_back({
                locId,
                x, y, z,
                type,
                rotation,
                idOffset,
                locOffset,
                location,
                byteStart,
                byteEnd,
                buf.slice(byteStart, byteEnd)
            });
            firstPlacementForId = false;
        }
    }
    return objects;
}

const std::vector<MapObject>& MapObjects::getObjects() const {
    return objects;
}
