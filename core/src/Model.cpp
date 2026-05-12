#include "Model.h"

#include <algorithm>
#include <stdexcept>
#include <string>

static void require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error("Model parse error: " + message);
}

Model Model::parse(Buffer& buf) {
    require(buf.remaining() >= 18, "data too small for footer");

    int dataSize = buf.remaining();

    // ------------------------------------------------------------------
    // Read the 18-byte footer from the end of the file.
    // ------------------------------------------------------------------
    std::vector<uint8_t> footerData = buf.slice(dataSize - 18, dataSize);
    Buffer footer(footerData);

    int vertexCount            = footer.readUShort();
    int triangleCount          = footer.readUShort();
    int textureTriangleCount   = footer.readByte();
    int hasFaceRenderTypes     = footer.readByte();
    int priorityFlag           = footer.readByte();
    int hasFaceAlpha           = footer.readByte();
    int hasFaceSkins           = footer.readByte();
    int hasVertexSkins         = footer.readByte();
    int vertexXDataLength      = footer.readUShort();
    int vertexYDataLength      = footer.readUShort();
    int vertexZDataLength      = footer.readUShort();
    int triangleIndexDataLength = footer.readUShort();

    require(footer.remaining() == 0, "footer not fully consumed");

    // ------------------------------------------------------------------
    // Compute the start offset of every data block.
    // Blocks are packed sequentially from byte 0 of the file.
    // ------------------------------------------------------------------
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

    require(offset <= dataSize - 18,
            "computed data blocks (" + std::to_string(offset) +
            " bytes) exceed file size minus footer (" +
            std::to_string(dataSize - 18) + " bytes)");

    // ------------------------------------------------------------------
    // Extract each data section into its own Buffer.
    // We must keep the underlying vectors alive while we read.
    // ------------------------------------------------------------------
    std::vector<uint8_t> vertexFlagsData      = buf.slice(vertexFlagsOffset,       vertexFlagsOffset       + vertexCount);
    std::vector<uint8_t> vertexXDataData      = buf.slice(vertexXDataOffset,       vertexXDataOffset       + vertexXDataLength);
    std::vector<uint8_t> vertexYDataData      = buf.slice(vertexYDataOffset,       vertexYDataOffset       + vertexYDataLength);
    std::vector<uint8_t> vertexZDataData      = buf.slice(vertexZDataOffset,       vertexZDataOffset       + vertexZDataLength);
    std::vector<uint8_t> vertexSkinsData      = (hasVertexSkins == 1)
                                                  ? buf.slice(vertexSkinOffset,     vertexSkinOffset     + vertexCount)
                                                  : std::vector<uint8_t>{};
    std::vector<uint8_t> faceColorsData       = buf.slice(faceColorOffset,         faceColorOffset         + triangleCount * 2);
    std::vector<uint8_t> faceRenderTypesData  = (hasFaceRenderTypes == 1)
                                                  ? buf.slice(faceRenderTypeOffset, faceRenderTypeOffset + triangleCount)
                                                  : std::vector<uint8_t>{};
    std::vector<uint8_t> facePrioritiesData   = (priorityFlag == 255)
                                                  ? buf.slice(facePriorityOffset,   facePriorityOffset   + triangleCount)
                                                  : std::vector<uint8_t>{};
    std::vector<uint8_t> faceAlphasData       = (hasFaceAlpha == 1)
                                                  ? buf.slice(faceAlphaOffset,      faceAlphaOffset      + triangleCount)
                                                  : std::vector<uint8_t>{};
    std::vector<uint8_t> faceSkinsData        = (hasFaceSkins == 1)
                                                  ? buf.slice(faceSkinOffset,       faceSkinOffset       + triangleCount)
                                                  : std::vector<uint8_t>{};
    std::vector<uint8_t> triangleOpcodesData  = buf.slice(triangleOpcodeOffset,    triangleOpcodeOffset    + triangleCount);
    std::vector<uint8_t> triangleIndexDataData = buf.slice(triangleIndexDataOffset, triangleIndexDataOffset + triangleIndexDataLength);
    std::vector<uint8_t> textureTrianglesData = buf.slice(textureTriangleOffset,   textureTriangleOffset   + textureTriangleCount * 6);

    Buffer vertexFlags      (vertexFlagsData);
    Buffer vertexXData      (vertexXDataData);
    Buffer vertexYData      (vertexYDataData);
    Buffer vertexZData      (vertexZDataData);
    Buffer vertexSkins      (vertexSkinsData);
    Buffer faceColors       (faceColorsData);
    Buffer faceRenderTypes  (faceRenderTypesData);
    Buffer facePriorities   (facePrioritiesData);
    Buffer faceAlphas       (faceAlphasData);
    Buffer faceSkins        (faceSkinsData);
    Buffer triangleOpcodes  (triangleOpcodesData);
    Buffer triangleIndexData(triangleIndexDataData);
    Buffer textureTriangles (textureTrianglesData);

    // ------------------------------------------------------------------
    // Parse vertices.
    // ------------------------------------------------------------------
    Model model;
    model.vertices.resize(vertexCount);
    if (hasVertexSkins == 1)
        model.vertexSkins.resize(vertexCount);

    int x = 0, y = 0, z = 0;
    for (int i = 0; i < vertexCount; i++) {
        int flags = vertexFlags.readByte();
        int dx = (flags & 1) ? vertexXData.readSignedSmart() : 0;
        int dy = (flags & 2) ? vertexYData.readSignedSmart() : 0;
        int dz = (flags & 4) ? vertexZData.readSignedSmart() : 0;

        x += dx;
        y += dy;
        z += dz;
        model.vertices[i] = {x, y, z};

        if (hasVertexSkins == 1)
            model.vertexSkins[i] = vertexSkins.readByte();
    }

    // ------------------------------------------------------------------
    // Parse triangle face data (colors, render types, priorities, alphas, skins).
    // ------------------------------------------------------------------
    model.triangles.resize(triangleCount);
    for (int i = 0; i < triangleCount; i++) {
        ModelTriangle& tri = model.triangles[i];
        tri.color = faceColors.readUShort();
        if (hasFaceRenderTypes == 1)
            tri.renderType = faceRenderTypes.readByte();
        if (priorityFlag == 255)
            tri.priority = facePriorities.readByte();
        else
            tri.priority = priorityFlag;
        if (hasFaceAlpha == 1)
            tri.alpha = faceAlphas.readByte();
        if (hasFaceSkins == 1)
            tri.skin = faceSkins.readByte();
    }

    // ------------------------------------------------------------------
    // Parse triangle vertex indices.
    // ------------------------------------------------------------------
    int a = 0, b = 0, c = 0, last = 0;
    for (int i = 0; i < triangleCount; i++) {
        int opcode = triangleOpcodes.readByte();

        if (opcode == 1) {
            a = triangleIndexData.readSignedSmart() + last; last = a;
            b = triangleIndexData.readSignedSmart() + last; last = b;
            c = triangleIndexData.readSignedSmart() + last; last = c;
        } else if (opcode == 2) {
            b = c;
            c = triangleIndexData.readSignedSmart() + last; last = c;
        } else if (opcode == 3) {
            a = c;
            c = triangleIndexData.readSignedSmart() + last; last = c;
        } else if (opcode == 4) {
            std::swap(a, b);
            c = triangleIndexData.readSignedSmart() + last; last = c;
        } else {
            throw std::runtime_error(
                "Model: unknown triangle index opcode " + std::to_string(opcode) +
                " at face " + std::to_string(i));
        }

        require(a >= 0 && a < vertexCount &&
                b >= 0 && b < vertexCount &&
                c >= 0 && c < vertexCount,
                "triangle " + std::to_string(i) + " references out-of-range vertex (" +
                std::to_string(a) + "," + std::to_string(b) + "," + std::to_string(c) + ")");

        model.triangles[i].a = a;
        model.triangles[i].b = b;
        model.triangles[i].c = c;
    }

    // ------------------------------------------------------------------
    // Parse texture triangles.
    // ------------------------------------------------------------------
    model.textureTriangles.resize(textureTriangleCount);
    for (int i = 0; i < textureTriangleCount; i++) {
        model.textureTriangles[i].a = textureTriangles.readUShort();
        model.textureTriangles[i].b = textureTriangles.readUShort();
        model.textureTriangles[i].c = textureTriangles.readUShort();
    }

    return model;
}

ModelBounds Model::bounds() const {
    ModelBounds result;
    if (vertices.empty())
        return result;

    result.minX = result.maxX = vertices[0].x;
    result.minY = result.maxY = vertices[0].y;
    result.minZ = result.maxZ = vertices[0].z;

    for (const ModelVertex& v : vertices) {
        result.minX = std::min(result.minX, v.x);
        result.maxX = std::max(result.maxX, v.x);
        result.minY = std::min(result.minY, v.y);
        result.maxY = std::max(result.maxY, v.y);
        result.minZ = std::min(result.minZ, v.z);
        result.maxZ = std::max(result.maxZ, v.z);
    }

    return result;
}
