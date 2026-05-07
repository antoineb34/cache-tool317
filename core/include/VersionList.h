#pragma once

#include <vector>

#include "Archive.h"

struct MapIndexEntry {
    int regionId = -1;
    int terrainFileId = -1;
    int objectFileId = -1;
    int flag = 0;
};

class VersionList {
public:
    static VersionList parse(const Archive& archive);

    const std::vector<MapIndexEntry>& mapIndex() const;
    const MapIndexEntry* findMapRegion(int regionId) const;

private:
    std::vector<MapIndexEntry> mapIndex_;
};
