#include "Model.h"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace {
class ModelReader {
public:
    explicit ModelReader(const std::vector<uint8_t>& data) : data_(data) {}

    void seek(int position) {
        if (position < 0 || position > static_cast<int>(data_.size()))
            throw std::runtime_error("Model read seek out of bounds");
        position_ = position;
    }

    uint8_t readByte() {
        require(1);
        return data_[position_++];
    }

    uint16_t readUShort() {
        require(2);
        uint16_t value = (static_cast<uint16_t>(data_[position_]) << 8) |
                         static_cast<uint16_t>(data_[position_ + 1]);
        position_ += 2;
        return value;
    }

    int readSignedSmart() {
        require(1);
        if (data_[position_] < 128)
            return static_cast<int>(readByte()) - 64;
        return static_cast<int>(readUShort()) - 49152;
    }

private:
    void require(int count) const {
        if (position_ + count > static_cast<int>(data_.size()))
            throw std::runtime_error("Model read out of bounds");
    }

    const std::vector<uint8_t>& data_;
    int position_ = 0;
};

void validateOffset(int offset, int size, const char* name) {
    if (offset < 0 || offset > size)
        throw std::runtime_error(std::string("Model offset out of bounds: ") + name);
}
}

Model Model::parse(const std::vector<uint8_t>& data) {
    if (data.size() < 18)
        throw std::runtime_error("Model data is too small");

    ModelReader footer(data);
    footer.seek(static_cast<int>(data.size()) - 18);

    int vertexCount = footer.readUShort();
    int triangleCount = footer.readUShort();
    int textureTriangleCount = footer.readByte();
    int hasFaceRenderTypes = footer.readByte();
    int priorityFlag = footer.readByte();
    int hasFaceAlpha = footer.readByte();
    int hasFaceSkins = footer.readByte();
    int hasVertexSkins = footer.readByte();
    int vertexXDataLength = footer.readUShort();
    int vertexYDataLength = footer.readUShort();
    int vertexZDataLength = footer.readUShort();
    int triangleIndexDataLength = footer.readUShort();

    int offset = 0;
    int vertexFlagsOffset = offset;
    offset += vertexCount;

    int triangleOpcodeOffset = offset;
    offset += triangleCount;

    int facePriorityOffset = -1;
    if (priorityFlag == 255) {
        facePriorityOffset = offset;
        offset += triangleCount;
    }

    int faceSkinOffset = -1;
    if (hasFaceSkins == 1) {
        faceSkinOffset = offset;
        offset += triangleCount;
    }

    int faceRenderTypeOffset = -1;
    if (hasFaceRenderTypes == 1) {
        faceRenderTypeOffset = offset;
        offset += triangleCount;
    }

    int vertexSkinOffset = -1;
    if (hasVertexSkins == 1) {
        vertexSkinOffset = offset;
        offset += vertexCount;
    }

    int faceAlphaOffset = -1;
    if (hasFaceAlpha == 1) {
        faceAlphaOffset = offset;
        offset += triangleCount;
    }

    int triangleIndexDataOffset = offset;
    offset += triangleIndexDataLength;

    int faceColorOffset = offset;
    offset += triangleCount * 2;

    int textureTriangleOffset = offset;
    offset += textureTriangleCount * 6;

    int vertexXDataOffset = offset;
    offset += vertexXDataLength;

    int vertexYDataOffset = offset;
    offset += vertexYDataLength;

    int vertexZDataOffset = offset;
    offset += vertexZDataLength;

    int dataSize = static_cast<int>(data.size());
    validateOffset(vertexFlagsOffset, dataSize, "vertexFlags");
    validateOffset(triangleOpcodeOffset, dataSize, "triangleOpcodes");
    validateOffset(triangleIndexDataOffset, dataSize, "triangleIndexData");
    validateOffset(faceColorOffset, dataSize, "faceColor");
    validateOffset(textureTriangleOffset, dataSize, "textureTriangle");
    validateOffset(vertexXDataOffset, dataSize, "vertexXData");
    validateOffset(vertexYDataOffset, dataSize, "vertexYData");
    validateOffset(vertexZDataOffset, dataSize, "vertexZData");
    validateOffset(offset, dataSize, "end");

    Model model;
    model.vertices_.resize(vertexCount);
    model.triangles_.resize(triangleCount);
    model.textureTriangles_.resize(textureTriangleCount);
    if (hasVertexSkins == 1)
        model.vertexSkins_.resize(vertexCount);

    ModelReader vertexFlags(data);
    ModelReader vertexXData(data);
    ModelReader vertexYData(data);
    ModelReader vertexZData(data);
    ModelReader vertexSkins(data);
    vertexFlags.seek(vertexFlagsOffset);
    vertexXData.seek(vertexXDataOffset);
    vertexYData.seek(vertexYDataOffset);
    vertexZData.seek(vertexZDataOffset);
    if (vertexSkinOffset != -1)
        vertexSkins.seek(vertexSkinOffset);

    int x = 0;
    int y = 0;
    int z = 0;
    for (int i = 0; i < vertexCount; i++) {
        int flags = vertexFlags.readByte();
        int dx = (flags & 1) ? vertexXData.readSignedSmart() : 0;
        int dy = (flags & 2) ? vertexYData.readSignedSmart() : 0;
        int dz = (flags & 4) ? vertexZData.readSignedSmart() : 0;

        x += dx;
        y += dy;
        z += dz;
        model.vertices_[i] = {x, y, z};

        if (vertexSkinOffset != -1)
            model.vertexSkins_[i] = vertexSkins.readByte();
    }

    ModelReader faceColors(data);
    ModelReader faceRenderTypes(data);
    ModelReader facePriorities(data);
    ModelReader faceAlphas(data);
    ModelReader faceSkins(data);
    faceColors.seek(faceColorOffset);
    if (faceRenderTypeOffset != -1)
        faceRenderTypes.seek(faceRenderTypeOffset);
    if (facePriorityOffset != -1)
        facePriorities.seek(facePriorityOffset);
    if (faceAlphaOffset != -1)
        faceAlphas.seek(faceAlphaOffset);
    if (faceSkinOffset != -1)
        faceSkins.seek(faceSkinOffset);

    for (int i = 0; i < triangleCount; i++) {
        ModelTriangle& triangle = model.triangles_[i];
        triangle.color = faceColors.readUShort();
        if (faceRenderTypeOffset != -1)
            triangle.renderType = faceRenderTypes.readByte();
        if (facePriorityOffset != -1)
            triangle.priority = facePriorities.readByte();
        else
            triangle.priority = priorityFlag;
        if (faceAlphaOffset != -1)
            triangle.alpha = faceAlphas.readByte();
        if (faceSkinOffset != -1)
            triangle.skin = faceSkins.readByte();
    }

    ModelReader triangleOpcodes(data);
    ModelReader triangleIndexData(data);
    triangleOpcodes.seek(triangleOpcodeOffset);
    triangleIndexData.seek(triangleIndexDataOffset);

    int a = 0;
    int b = 0;
    int c = 0;
    int last = 0;
    for (int i = 0; i < triangleCount; i++) {
        int opcode = triangleOpcodes.readByte();

        if (opcode == 1) {
            a = triangleIndexData.readSignedSmart() + last;
            last = a;
            b = triangleIndexData.readSignedSmart() + last;
            last = b;
            c = triangleIndexData.readSignedSmart() + last;
            last = c;
        } else if (opcode == 2) {
            b = c;
            c = triangleIndexData.readSignedSmart() + last;
            last = c;
        } else if (opcode == 3) {
            a = c;
            c = triangleIndexData.readSignedSmart() + last;
            last = c;
        } else if (opcode == 4) {
            std::swap(a, b);
            c = triangleIndexData.readSignedSmart() + last;
            last = c;
        } else {
            throw std::runtime_error("Unknown model triangle index opcode: " + std::to_string(opcode));
        }

        if (a < 0 || a >= vertexCount || b < 0 || b >= vertexCount || c < 0 || c >= vertexCount) {
            throw std::runtime_error(
                "Model triangle references out-of-range vertex at face " + std::to_string(i) +
                ": opcode=" + std::to_string(opcode) +
                " vertices=(" + std::to_string(a) + "," + std::to_string(b) + "," + std::to_string(c) + ")" +
                " vertexCount=" + std::to_string(vertexCount));
        }

        model.triangles_[i].a = a;
        model.triangles_[i].b = b;
        model.triangles_[i].c = c;
    }

    ModelReader textureTriangles(data);
    textureTriangles.seek(textureTriangleOffset);
    for (int i = 0; i < textureTriangleCount; i++) {
        model.textureTriangles_[i].a = textureTriangles.readUShort();
        model.textureTriangles_[i].b = textureTriangles.readUShort();
        model.textureTriangles_[i].c = textureTriangles.readUShort();
    }

    return model;
}

const std::vector<ModelVertex>& Model::vertices() const {
    return vertices_;
}

const std::vector<ModelTriangle>& Model::triangles() const {
    return triangles_;
}

const std::vector<ModelTextureTriangle>& Model::textureTriangles() const {
    return textureTriangles_;
}

const std::vector<int>& Model::vertexSkins() const {
    return vertexSkins_;
}

ModelBounds Model::bounds() const {
    ModelBounds result;
    if (vertices_.empty())
        return result;

    result.minX = result.maxX = vertices_[0].x;
    result.minY = result.maxY = vertices_[0].y;
    result.minZ = result.maxZ = vertices_[0].z;

    for (const ModelVertex& vertex : vertices_) {
        result.minX = std::min(result.minX, vertex.x);
        result.maxX = std::max(result.maxX, vertex.x);
        result.minY = std::min(result.minY, vertex.y);
        result.maxY = std::max(result.maxY, vertex.y);
        result.minZ = std::min(result.minZ, vertex.z);
        result.maxZ = std::max(result.maxZ, vertex.z);
    }

    return result;
}
