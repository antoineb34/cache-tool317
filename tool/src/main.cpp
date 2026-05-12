#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>

#include "CacheReader.h"
#include "DefinitionsLoader.h"
#include "ModelDef.h"
#include "VersionList.h"
#include "MapRegion.h"
#include "RegionRenderer2D.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: tool <path to cache folder> [region id] [object filter]\n"
                  << "       tool <path to cache folder> <region id> render2d [output.ppm] [plane] [scale] [all|terrain|objects]\n"
                  << "       tool <path to cache folder> <region id> tile <worldX> <worldY> [plane]\n"
                  << "       tool <path to cache folder> dump_model <model id>\n";
        return 1;
    }

    CacheReader reader;
    if (!reader.open(argv[1])) {
        std::cerr << "Failed to open cache at: " << argv[1] << std::endl;
        return 1;
    }

    try {
        // dump_model: extract and inspect raw bytes of a single model entry
        if (argc >= 3 && std::string(argv[2]) == "dump_model") {
            int modelId = argc >= 4 ? std::stoi(argv[3]) : 0;

            if (!reader.hasFile(1, modelId)) {
                std::cerr << "Model " << modelId << " does not exist in archive 1\n";
                return 1;
            }

            auto data = reader.readGzippedFile(1, modelId);
            int size = (int)data.size();

            std::cout << "Model " << modelId << " — decompressed size: " << size << " bytes\n\n";

            // hex dump of first 64 bytes
            int dumpLen = std::min(64, size);
            std::cout << "First " << dumpLen << " bytes:\n";
            for (int i = 0; i < dumpLen; i++) {
                if (i % 16 == 0)
                    std::cout << std::hex << std::setw(4) << std::setfill('0') << i << ":  ";
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i] << " ";
                if (i % 16 == 15) std::cout << "\n";
            }
            std::cout << std::dec << "\n";

            if (size < 18) {
                std::cerr << "Model too small to have a valid footer (need 18 bytes)\n";
                return 1;
            }

            // parse footer: last 18 bytes
            const uint8_t* f = data.data() + size - 18;
            auto ru16 = [](const uint8_t* p, int off) -> uint16_t {
                return (uint16_t)((p[off] << 8) | p[off + 1]);
            };

            uint16_t vertexCount          = ru16(f,  0);
            uint16_t triangleCount        = ru16(f,  2);
            uint8_t  texTriangleCount     = f[4];
            uint8_t  hasFaceRenderTypes   = f[5];
            uint8_t  priorityFlag         = f[6];
            uint8_t  hasFaceAlpha         = f[7];
            uint8_t  hasFaceSkins         = f[8];
            uint8_t  hasVertexSkins       = f[9];
            uint16_t vertexXLen           = ru16(f, 10);
            uint16_t vertexYLen           = ru16(f, 12);
            uint16_t vertexZLen           = ru16(f, 14);
            uint16_t triIndexLen          = ru16(f, 16);

            std::cout << "--- Footer (last 18 bytes) ---\n";
            std::cout << "  vertexCount:             " << vertexCount          << "\n";
            std::cout << "  triangleCount:           " << triangleCount        << "\n";
            std::cout << "  texTriangleCount:        " << (int)texTriangleCount    << "\n";
            std::cout << "  hasFaceRenderTypes:      " << (int)hasFaceRenderTypes  << "\n";
            std::cout << "  priorityFlag:            " << (int)priorityFlag
                      << (priorityFlag == 255 ? "  (per-face)" : "  (shared)") << "\n";
            std::cout << "  hasFaceAlpha:            " << (int)hasFaceAlpha    << "\n";
            std::cout << "  hasFaceSkins:            " << (int)hasFaceSkins    << "\n";
            std::cout << "  hasVertexSkins:          " << (int)hasVertexSkins  << "\n";
            std::cout << "  vertexXDataLength:       " << vertexXLen           << "\n";
            std::cout << "  vertexYDataLength:       " << vertexYLen           << "\n";
            std::cout << "  vertexZDataLength:       " << vertexZLen           << "\n";
            std::cout << "  triIndexDataLength:      " << triIndexLen          << "\n";

            // verify: compute expected size from footer and compare to actual
            int expected = vertexCount                                          // vertexFlags
                         + triangleCount                                        // triangleOpcodes
                         + (priorityFlag == 255  ? triangleCount : 0)          // facePriority
                         + (hasFaceSkins         ? triangleCount : 0)          // faceSkin
                         + (hasFaceRenderTypes   ? triangleCount : 0)          // faceRenderType
                         + (hasVertexSkins       ? vertexCount   : 0)          // vertexSkin
                         + (hasFaceAlpha         ? triangleCount : 0)          // faceAlpha
                         + triIndexLen                                          // triangleIndexData
                         + triangleCount * 2                                    // faceColor (ushort each)
                         + texTriangleCount * 6                                 // textureTriangle (3 ushorts)
                         + vertexXLen + vertexYLen + vertexZLen                 // vertex coord data
                         + 18;                                                  // footer itself

            std::cout << "\nExpected size from footer: " << expected << " bytes\n";
            std::cout << "Actual size:               " << size      << " bytes\n";
            std::cout << (expected == size ? "[OK] sizes match — format confirmed\n"
                                          : "[MISMATCH] format doc may be wrong or model is unusual\n");

            // parse and print decoded summary
            std::cout << "\n--- Parsed ModelDef ---\n";
            ModelDef def = ModelDef::parse(modelId, data);
            std::cout << "  vertices:  " << def.vertexX.size() << "\n";
            std::cout << "  triangles: " << def.triA.size()    << "\n";
            std::cout << "  texTris:   " << def.texP.size()    << "\n";
            if (!def.vertexX.empty()) {
                std::cout << "  vertex[0]: ("
                    << def.vertexX[0] << ", "
                    << def.vertexY[0] << ", "
                    << def.vertexZ[0] << ")\n";
            }
            if (!def.triA.empty()) {
                std::cout << "  tri[0]:    ("
                    << def.triA[0] << ", "
                    << def.triB[0] << ", "
                    << def.triC[0] << ")  color=0x"
                    << std::hex << def.triColor[0] << std::dec << "\n";
            }
            return 0;
        }

        Archive defsArchive = reader.readArchive(0, 2);
        DefinitionsLoader loader;
        loader.loadLocs(defsArchive);
        loader.loadFlos(defsArchive);

        Archive vArchive = reader.readArchive(0, 5);
        VersionList vList = VersionList::parse(vArchive);
        int regionId = 12850;
        if (argc >= 3)
            regionId = std::stoi(argv[2]);
        std::string objectFilter;
        if (argc >= 4)
            objectFilter = argv[3];

        MapRegion region = MapRegion::load(reader, vList, regionId);
        int baseX = (region.regionId() >> 8) * 64;
        int baseY = (region.regionId() & 0xff) * 64;

        if (objectFilter == "render2d") {
            std::string outputPath = "region_" + std::to_string(regionId) + ".ppm";
            if (argc >= 5)
                outputPath = argv[4];
            int plane = argc >= 6 ? std::stoi(argv[5]) : 0;
            int scale = argc >= 7 ? std::stoi(argv[6]) : 8;
            RenderLayer layer = argc >= 8 ? RegionRenderer2D::parseLayer(argv[7]) : RenderLayer::All;

            RegionImage image = RegionRenderer2D::render(region, loader, plane, scale, layer);
            image.savePpm(outputPath);

            std::cout << "Rendered region " << regionId << " plane " << plane << "\n";
            std::cout << "  output: " << outputPath << "\n";
            return 0;
        }

        std::cout << "Region " << region.regionId() << " map decode\n";
        std::cout << "  base world coord: (" << baseX << "," << baseY << ")\n";
        std::cout << "  terrain tiles: 16384 (64x64x4)\n";
        std::cout << "  object placements: " << region.objects().getObjects().size() << "\n";

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
