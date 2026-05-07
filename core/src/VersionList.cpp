#include "VersionList.h"

#include <stdexcept>
#include <string>

#include "Buffer.h"

VersionList VersionList::parse(const Archive& archive) {
    VersionList versionList;

    auto mapIndexData = archive.getFile("map_index");
    if (mapIndexData.empty())
        throw std::runtime_error("Missing or empty versionlist file: map_index");
    if (mapIndexData.size() % 7 != 0) {
        throw std::runtime_error(
            "Malformed map_index: size " + std::to_string(mapIndexData.size()) +
            " is not divisible by 7"
        );
    }

    Buffer buf(mapIndexData);
    int count = static_cast<int>(mapIndexData.size() / 7);
    versionList.mapIndex_.reserve(count);

    for (int i = 0; i < count; i++) {
        MapIndexEntry entry;
        entry.regionId = buf.readUShort();
        entry.terrainFileId = buf.readUShort();
        entry.objectFileId = buf.readUShort();
        entry.flag = buf.readByte();
        versionList.mapIndex_.push_back(entry);
    }

    return versionList;
}

const std::vector<MapIndexEntry>& VersionList::mapIndex() const {
    return mapIndex_;
}

const MapIndexEntry* VersionList::findMapRegion(int regionId) const {
    for (const auto& entry : mapIndex_) {
        if (entry.regionId == regionId)
            return &entry;
    }
    return nullptr;
}
