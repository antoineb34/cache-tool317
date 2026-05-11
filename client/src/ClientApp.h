#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include "AppMode.h"
#include "DefinitionsLoader.h"
#include "MapRegion.h"
#include "Model.h"
#include "RegionRenderer2D.h"

struct LoadedRegion {
    MapRegion region;
    int baseX = 0;
    int baseY = 0;
};

class ClientApp {
public:
    explicit ClientApp(std::string cachePath, int regionId = 12850, bool smokeTest = false);
    ~ClientApp();

    ClientApp(const ClientApp&) = delete;
    ClientApp& operator=(const ClientApp&) = delete;

    int run();

private:
    bool initialize();
    bool loadWorld();
    void shutdown();

    void handleEvent(const SDL_Event& event);
    void updateCamera(float deltaSeconds);
    void render();
    void renderStart();
    void renderPlay();
    void setMode(AppMode mode);
    void drawRect(float x, float y, float w, float h, float r, float g, float b);
    void setup2D(int width, int height);
    void setup3D(int width, int height);
    void renderWorld();
    void applyFollowCamera();
    void renderTerrain();
    void renderRegionTerrain(const LoadedRegion& loaded);
    void renderTerrainGrid();
    void renderRegionTerrainGrid(const LoadedRegion& loaded);
    void renderPlayerMarker();
    void renderObjectMarkers();
    void renderRegionObjectMarkers(const LoadedRegion& loaded);
    void renderObjectMarker(const LoadedRegion& loaded, const MapObject& object);
    void renderObjectModels();
    void renderRegionObjectModels(const LoadedRegion& loaded);
    void renderObjectModel(const LoadedRegion& loaded, const MapObject& object);
    void renderModelWireframe(const Model& model);
    void renderModelSolid(const Model& model);
    Rgb modelFaceColor(int baseHsl, int renderType, float nx, float ny, float nz) const;
    int selectModelId(const LocDef& loc, int objectType) const;
    void drawObjectBox(float x, float y, float z, float width, float length, float height);
    void drawObjectLine(float x1, float y1, float z1, float x2, float y2, float z2);
    float terrainHeightAt(float x, float y) const;
    float cacheYToWorldZ(float cacheY) const;
    float worldZToCacheY(float worldZ) const;
    int cacheRotationToWorld(int rotation) const;
    Rgb floorColor(const Tile& tile) const;

    std::string cachePath_;
    AppMode mode_ = AppMode::Start;
    bool shouldQuit_ = false;
    bool smokeTest_ = false;
    bool wireframe_ = false;
    bool showGrid_ = false;
    bool showObjects_ = true;
    bool showModels_ = true;
    bool mouseLook_ = false;

    SDL_Window* window_ = nullptr;
    SDL_GLContext glContext_ = nullptr;

    DefinitionsLoader defs_;
    std::vector<LoadedRegion> regions_;
    std::unordered_map<int, Model> models_;
    std::vector<uint32_t> modelPalette_;
    int regionId_ = 12850;
    int regionBaseX_ = 0;
    int regionBaseY_ = 0;
    int objectCount_ = 0;
    int plane_ = 0;
    float playerX_ = 32.0f;
    float playerY_ = 34.0f;
    float cameraDistance_ = 95.0f;
    float cameraYaw_ = 45.0f;
    float cameraPitch_ = 42.0f;
    uint64_t lastFrameTicks_ = 0;
};
