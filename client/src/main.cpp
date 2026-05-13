// Entry point — creates App and DebugModelViewer, wires them together.
// This file is intentionally tiny.

#include "App.h"
#include "DebugModelViewer.h"

#include <iostream>
#include <cstring>

// Defined in DebugModelViewer.cpp
extern DebugModelViewer* g_viewer;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: client <cache_path> [--model <id>]" << std::endl;
        return 1;
    }

    std::string cachePath = argv[1];
    int modelId = 100; // default tree

    for (int i = 2; i < argc; i++) {
        if (std::strcmp(argv[i], "--model") == 0 && i + 1 < argc) {
            modelId = std::atoi(argv[++i]);
        }
    }

    // --- Load model from cache ---
    CacheReader reader;
    if (!reader.open(cachePath)) {
        std::cerr << "Failed to open cache: " << cachePath << std::endl;
        return 1;
    }
    std::cout << "Cache opened: " << cachePath << std::endl;

    if (!reader.hasFile(1, modelId)) {
        std::cerr << "Model " << modelId << " not found in archive 1" << std::endl;
        return 1;
    }

    DebugModelViewer viewer;
    if (!viewer.load(reader, modelId)) {
        std::cerr << "Failed to load model " << modelId << std::endl;
        return 1;
    }

    std::cout << "--- Model Info ---" << std::endl;
    std::cout << "  Vertices:       " << viewer.vertexCount() << std::endl;
    std::cout << "  Triangles:      " << viewer.triangleCount() << std::endl;
    std::cout << "  Textured faces: " << viewer.texturedFaceCount() << std::endl;
    std::cout << "------------------" << std::endl;

    // Wire global viewer pointer for App
    g_viewer = &viewer;

    // --- Run the window ---
    App app;
    if (!app.init("RS317 Wireframe Debug Viewer", 1280, 960)) {
        std::cerr << "App init failed" << std::endl;
        return 1;
    }

    return app.run();
}