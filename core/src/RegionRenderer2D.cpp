#include "RegionRenderer2D.h"

#include <algorithm>
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

RegionImage RegionRenderer2D::render(const MapRegion& region, const DefinitionsLoader& defs, int plane, int scale) {
    if (plane < 0 || plane >= 4)
        throw std::runtime_error("Render plane must be between 0 and 3");
    if (scale < 1)
        throw std::runtime_error("Render scale must be at least 1");

    RegionImage image(64 * scale, 64 * scale);

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

    for (const MapObject& obj : region.objects().getObjects()) {
        if (obj.z != plane)
            continue;

        const LocDef& loc = defs.getLoc(obj.id);
        Rgb color = loc.name == "null" ? Rgb{35, 35, 35} : Rgb{210, 40, 40};
        if (obj.type >= 10 && loc.blocksWalk)
            color = {30, 30, 30};
        else if (obj.type >= 10)
            color = {230, 180, 40};

        int width = obj.type >= 10 ? std::max(1, loc.width) : 1;
        int length = obj.type >= 10 ? std::max(1, loc.length) : 1;
        if ((obj.rotation & 1) == 1)
            std::swap(width, length);

        for (int dx = 0; dx < width; dx++) {
            for (int dy = 0; dy < length; dy++) {
                int tx = obj.x + dx;
                int ty = obj.y + dy;
                int imageY = 63 - ty;
                for (int px = 1; px < scale - 1; px++) {
                    for (int py = 1; py < scale - 1; py++) {
                        image.setPixel(tx * scale + px, imageY * scale + py, color);
                    }
                }
                if (scale == 1)
                    image.setPixel(tx, imageY, color);
            }
        }
    }

    return image;
}
