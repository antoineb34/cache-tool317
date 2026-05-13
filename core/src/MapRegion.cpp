#include "MapRegion.h"

#include <stdexcept>
#include <string>

#include "Buffer.h"

MapRegion MapRegion::load(CacheReader& reader, const VersionList& versionList, int regionId) {
    const MapIndexEntry* entry = versionList.findMapRegion(regionId);
    if (entry == nullptr)
        throw std::runtime_error("Map region not found in map_index: " + std::to_string(regionId));

    Buffer terrainData = reader.readGzippedFile(4, entry->terrainFileId);
    if (terrainData.empty())
        throw std::runtime_error("Failed to read/decompress terrain map file: " + std::to_string(entry->terrainFileId));

    Buffer objectData = reader.readGzippedFile(4, entry->objectFileId);
    if (objectData.empty())
        throw std::runtime_error("Failed to read/decompress object map file: " + std::to_string(entry->objectFileId));

    MapRegion region;
    region.regionId_ = regionId;
    region.terrainFileId_ = entry->terrainFileId;
    region.objectFileId_ = entry->objectFileId;
    region.terrainDataSize_ = terrainData.size();
    region.objectDataSize_ = objectData.size();
    region.terrain_ = MapTerrain::parse(terrainData, regionId);
    region.objects_ = MapObjects::parse(objectData);
    return region;
}

int MapRegion::regionId() const {
    return regionId_;
}

int MapRegion::terrainFileId() const {
    return terrainFileId_;
}

int MapRegion::objectFileId() const {
    return objectFileId_;
}

std::size_t MapRegion::terrainDataSize() const {
    return terrainDataSize_;
}

std::size_t MapRegion::objectDataSize() const {
    return objectDataSize_;
}

const MapTerrain& MapRegion::terrain() const {
    return terrain_;
}

const MapObjects& MapRegion::objects() const {
    return objects_;
}
