#include "RegionRenderer2D.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>

RegionImage::RegionImage(int width, int height)
    : width_(width), height_(height), pixels_(width * height) {}

int RegionImage::width() const {
    return width_;
}

int RegionImage::height() const {
    return height_;
}

void RegionImage::setPixel(int x, int y, Rgb color) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_)
        return;
    pixels_[y * width_ + x] = color;
}

Rgb RegionImage::getPixel(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_)
        return {};
    return pixels_[y * width_ + x];
}

void RegionImage::savePpm(const std::string& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out)
        throw std::runtime_error("Failed to open render output: " + path);

    out << "P6\n" << width_ << " " << height_ << "\n255\n";
    for (Rgb pixel : pixels_) {
        out.put(static_cast<char>(pixel.r));
        out.put(static_cast<char>(pixel.g));
        out.put(static_cast<char>(pixel.b));
    }
}

static Rgb fromRgbInt(int rgb) {
    if (rgb < 0)
        return {70, 70, 70};

    return {
        static_cast<uint8_t>((rgb >> 16) & 0xff),
        static_cast<uint8_t>((rgb >> 8) & 0xff),
        static_cast<uint8_t>(rgb & 0xff)
    };
}

Rgb RegionRenderer2D::floorColor(const DefinitionsLoader& defs, const Tile& tile) {
    if (tile.overlayId > 0) {
        try {
            return fromRgbInt(defs.getFlo(tile.overlayId - 1).rgb);
        } catch (...) {
        }
    }

    if (tile.underlayId > 0) {
        try {
            return fromRgbInt(defs.getFlo(tile.underlayId - 1).rgb);
        } catch (...) {
        }
    }

    return {60, 70, 60};
}

Rgb RegionRenderer2D::shadeByHeight(Rgb color, int height) {
    int shade = std::clamp(96 + (-height / 16), 70, 135);
    auto apply = [shade](uint8_t value) {
        return static_cast<uint8_t>(std::clamp((static_cast<int>(value) * shade) / 100, 0, 255));
    };
    return {apply(color.r), apply(color.g), apply(color.b)};
}

static int tilePixelX(int tileX, int scale) {
    return tileX * scale;
}

static int tilePixelY(int tileY, int scale) {
    return (63 - tileY) * scale;
}

static void fillRect(RegionImage& image, int x, int y, int width, int height, Rgb color) {
    for (int px = x; px < x + width; px++) {
        for (int py = y; py < y + height; py++) {
            image.setPixel(px, py, color);
        }
    }
}

static void drawTileFill(RegionImage& image, int tileX, int tileY, int width, int length, int scale, Rgb color) {
    int margin = scale >= 4 ? 1 : 0;
    for (int dx = 0; dx < width; dx++) {
        for (int dy = 0; dy < length; dy++) {
            int x = tilePixelX(tileX + dx, scale) + margin;
            int y = tilePixelY(tileY + dy, scale) + margin;
            fillRect(image, x, y, std::max(1, scale - margin * 2), std::max(1, scale - margin * 2), color);
        }
    }
}

static void drawEdge(RegionImage& image, int tileX, int tileY, int rotation, int scale, Rgb color) {
    int x = tilePixelX(tileX, scale);
    int y = tilePixelY(tileY, scale);
    int thickness = std::max(1, scale / 4);

    switch (rotation & 3) {
        case 0: // west
            fillRect(image, x, y, thickness, scale, color);
            break;
        case 1: // north
            fillRect(image, x, y, scale, thickness, color);
            break;
        case 2: // east
            fillRect(image, x + scale - thickness, y, thickness, scale, color);
            break;
        case 3: // south
            fillRect(image, x, y + scale - thickness, scale, thickness, color);
            break;
    }
}

static void drawCorner(RegionImage& image, int tileX, int tileY, int rotation, int scale, Rgb color) {
    drawEdge(image, tileX, tileY, rotation, scale, color);
    drawEdge(image, tileX, tileY, rotation + 1, scale, color);
}

static void drawDiagonal(RegionImage& image, int tileX, int tileY, int rotation, int scale, Rgb color) {
    int x = tilePixelX(tileX, scale);
    int y = tilePixelY(tileY, scale);
    int thickness = std::max(1, scale / 5);

    for (int i = 0; i < scale; i++) {
        int px = x + i;
        int py = ((rotation & 1) == 0) ? y + i : y + scale - 1 - i;
        fillRect(image, px, py, thickness, thickness, color);
    }
}

static void drawMarker(RegionImage& image, int tileX, int tileY, int scale, Rgb color) {
    int size = std::max(1, scale / 2);
    int x = tilePixelX(tileX, scale) + (scale - size) / 2;
    int y = tilePixelY(tileY, scale) + (scale - size) / 2;
    fillRect(image, x, y, size, size, color);
}

void RegionRenderer2D::drawObject(RegionImage& image, const MapObject& obj, const LocDef& loc, int scale) {
    Rgb wallColor = loc.name == "null" ? Rgb{15, 15, 15} : Rgb{180, 20, 20};
    Rgb objectColor = loc.blocksWalk ? Rgb{35, 35, 35} : Rgb{220, 170, 35};
    Rgb decorColor = loc.name == "null" ? Rgb{70, 70, 70} : Rgb{220, 50, 50};

    if (obj.type == 0) {
        drawEdge(image, obj.x, obj.y, obj.rotation, scale, wallColor);
        return;
    }

    if (obj.type == 2) {
        drawCorner(image, obj.x, obj.y, obj.rotation, scale, wallColor);
        return;
    }

    if (obj.type == 9) {
        drawDiagonal(image, obj.x, obj.y, obj.rotation, scale, wallColor);
        return;
    }

    if (obj.type >= 10) {
        int width = std::max(1, loc.width);
        int length = std::max(1, loc.length);
        if ((obj.rotation & 1) == 1)
            std::swap(width, length);
        drawTileFill(image, obj.x, obj.y, width, length, scale, objectColor);
        return;
    }

    drawMarker(image, obj.x, obj.y, scale, decorColor);
}

static std::string normalizeLayerName(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return name;
}

RenderLayer RegionRenderer2D::parseLayer(const std::string& name) {
    std::string normalized = normalizeLayerName(name);
    if (normalized == "all")
        return RenderLayer::All;
    if (normalized == "terrain")
        return RenderLayer::Terrain;
    if (normalized == "objects")
        return RenderLayer::Objects;
    throw std::runtime_error("Unknown render2d layer: " + name);
}

std::string RegionRenderer2D::layerName(RenderLayer layer) {
    switch (layer) {
        case RenderLayer::All:
            return "all";
        case RenderLayer::Terrain:
            return "terrain";
        case RenderLayer::Objects:
            return "objects";
    }
    return "unknown";
}

RegionImage RegionRenderer2D::render(const MapRegion& region, const DefinitionsLoader& defs, int plane, int scale, RenderLayer layer) {
    if (plane < 0 || plane >= 4)
        throw std::runtime_error("Render plane must be between 0 and 3");
    if (scale < 1)
        throw std::runtime_error("Render scale must be at least 1");

    RegionImage image(64 * scale, 64 * scale);

    if (layer == RenderLayer::Objects) {
        fillRect(image, 0, 0, image.width(), image.height(), {190, 190, 180});
    } else {
        for (int x = 0; x < 64; x++) {
            for (int y = 0; y < 64; y++) {
                const Tile& tile = region.terrain().getTile(plane, x, y);
                Rgb color = shadeByHeight(floorColor(defs, tile), tile.height);
                int imageY = 63 - y;
                for (int px = 0; px < scale; px++) {
                    for (int py = 0; py < scale; py++) {
                        image.setPixel(x * scale + px, imageY * scale + py, color);
                    }
                }
            }
        }
    }

    if (layer == RenderLayer::Terrain)
        return image;

    for (const MapObject& obj : region.objects().getObjects()) {
        if (obj.z != plane)
            continue;

        const LocDef& loc = defs.getLoc(obj.id);
        drawObject(image, obj, loc, scale);
    }

    return image;
}
