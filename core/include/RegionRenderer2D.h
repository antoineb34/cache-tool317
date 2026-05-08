#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "DefinitionsLoader.h"
#include "MapRegion.h"

struct Rgb {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
};

enum class RenderLayer {
    All,
    Terrain,
    Objects
};

class RegionImage {
public:
    RegionImage(int width, int height);

    int width() const;
    int height() const;
    void setPixel(int x, int y, Rgb color);
    Rgb getPixel(int x, int y) const;
    void savePpm(const std::string& path) const;

private:
    int width_;
    int height_;
    std::vector<Rgb> pixels_;
};

class RegionRenderer2D {
public:
    static RegionImage render(const MapRegion& region, const DefinitionsLoader& defs, int plane = 0, int scale = 8, RenderLayer layer = RenderLayer::All);
    static RenderLayer parseLayer(const std::string& name);
    static std::string layerName(RenderLayer layer);

private:
    static Rgb floorColor(const DefinitionsLoader& defs, const Tile& tile);
    static Rgb shadeByHeight(Rgb color, int height);
    static void drawObject(RegionImage& image, const MapObject& obj, const LocDef& loc, int scale);
};
