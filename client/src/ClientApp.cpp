#include "ClientApp.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#include "CacheReader.h"
#include "VersionList.h"

namespace {
constexpr int WindowWidth = 1024;
constexpr int WindowHeight = 768;
constexpr float DegreesToRadians = 3.14159265358979323846f / 180.0f;

Rgb fromRgbInt(int rgb) {
    if (rgb < 0)
        return {70, 70, 70};

    return {
        static_cast<uint8_t>((rgb >> 16) & 0xff),
        static_cast<uint8_t>((rgb >> 8) & 0xff),
        static_cast<uint8_t>(rgb & 0xff)
    };
}

Rgb rsHslToRgb(int hsl, int shade = 0) {
    int hue = (hsl >> 10) & 0x3F;
    int saturation = (hsl >> 7) & 0x7;
    int lightness = std::clamp((hsl & 0x7F) + shade, 0, 127);

    double h = hue / 64.0 + 0.0078125;
    double s = saturation / 8.0 + 0.0625;
    double l = lightness / 128.0;

    double r = l;
    double g = l;
    double b = l;

    if (s != 0.0) {
        double a;
        if (l < 0.5) {
            a = l * (1.0 + s);
        } else {
            a = (l + s) - (l * s);
        }
        double b2 = (2.0 * l) - a;
        double fRed = h + (1.0 / 3.0);
        double fBlue = h - (1.0 / 3.0);
        if (fRed > 1.0) fRed -= 1.0;
        if (fBlue < 0.0) fBlue += 1.0;

        auto getValue = [](double hue, double a, double b) -> double {
            if (6.0 * hue < 1.0) return b + (a - b) * 6.0 * hue;
            if (2.0 * hue < 1.0) return a;
            if (3.0 * hue < 2.0) return b + (a - b) * ((2.0 / 3.0) - hue) * 6.0;
            return b;
        };

        r = getValue(fRed, a, b2);
        g = getValue(h, a, b2);
        b = getValue(fBlue, a, b2);
    }

    return {
        static_cast<uint8_t>(std::clamp(r * 256.0, 0.0, 255.0)),
        static_cast<uint8_t>(std::clamp(g * 256.0, 0.0, 255.0)),
        static_cast<uint8_t>(std::clamp(b * 256.0, 0.0, 255.0))
    };
}

float terrainHeight(const Tile& tile) {
    return static_cast<float>(-tile.height) / 32.0f;
}

int regionIdFromCoords(int regionX, int regionY) {
    return (regionX << 8) | regionY;
}

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

Vec3 normalize(Vec3 value) {
    float length = std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
    if (length <= 0.0001f)
        return {};
    return {value.x / length, value.y / length, value.z / length};
}

Vec3 cross(Vec3 a, Vec3 b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

void lookAt(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = normalize({center.x - eye.x, center.y - eye.y, center.z - eye.z});
    Vec3 s = normalize(cross(f, up));
    Vec3 u = cross(s, f);

    float matrix[16] = {
        s.x, u.x, -f.x, 0.0f,
        s.y, u.y, -f.y, 0.0f,
        s.z, u.z, -f.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    glMultMatrixf(matrix);
    glTranslatef(-eye.x, -eye.y, -eye.z);
}
}

ClientApp::ClientApp(std::string cachePath, int regionId, bool smokeTest)
    : cachePath_(std::move(cachePath)), smokeTest_(smokeTest), regionId_(regionId) {}

ClientApp::~ClientApp() {
    shutdown();
}

int ClientApp::run() {
    if (!initialize())
        return 1;

    lastFrameTicks_ = SDL_GetTicks();

    if (smokeTest_) {
        setMode(AppMode::Play);
        render();
        return 0;
    }

    while (!shouldQuit_) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                shouldQuit_ = true;
                continue;
            }
            handleEvent(event);
        }

        uint64_t now = SDL_GetTicks();
        float deltaSeconds = static_cast<float>(now - lastFrameTicks_) / 1000.0f;
        lastFrameTicks_ = now;
        updateCamera(std::min(deltaSeconds, 0.05f));
        render();
        SDL_Delay(16);
    }

    return 0;
}

bool ClientApp::initialize() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    window_ = SDL_CreateWindow("cache-tool317 client", WindowWidth, WindowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window_) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return false;
    }

    glContext_ = SDL_GL_CreateContext(window_);
    if (!glContext_) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << "\n";
        return false;
    }

    if (!SDL_GL_MakeCurrent(window_, glContext_)) {
        std::cerr << "SDL_GL_MakeCurrent failed: " << SDL_GetError() << "\n";
        return false;
    }

    SDL_GL_SetSwapInterval(1);
    glClearColor(0.07f, 0.08f, 0.09f, 1.0f);

    if (!loadWorld())
        return false;

    std::cout << "Client started.\n"
              << "  cache: " << cachePath_ << "\n"
              << "  region: " << regionId_ << " base=(" << regionBaseX_ << "," << regionBaseY_ << ")\n"
              << "  objects: " << objectCount_ << "\n"
              << "  models loaded: " << models_.size() << "\n"
              << "  keys: P=play, WASD/arrows=move player, Q/E=rotate camera, +/-=zoom, 1-4=plane, G=grid, F=wireframe, U=objects, M=models, mouse drag=look, Escape=start/quit\n";
    return true;
}

bool ClientApp::loadWorld() {
    try {
        CacheReader reader;
        if (!reader.open(cachePath_)) {
            std::cerr << "Failed to open cache at: " << cachePath_ << "\n";
            return false;
        }

        Archive defsArchive = reader.readArchive(0, 2);
        defs_.loadLocs(defsArchive);
        defs_.loadFlos(defsArchive);

        Archive versionArchive = reader.readArchive(0, 5);
        VersionList versionList = VersionList::parse(versionArchive);

        int centerRegionX = regionId_ >> 8;
        int centerRegionY = regionId_ & 0xff;
        regionBaseX_ = centerRegionX * 64;
        regionBaseY_ = centerRegionY * 64;

        regions_.clear();
        objectCount_ = 0;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                int neighborRegionId = regionIdFromCoords(centerRegionX + dx, centerRegionY + dy);
                try {
                    MapRegion region = MapRegion::load(reader, versionList, neighborRegionId);
                    LoadedRegion loaded;
                    loaded.baseX = ((region.regionId() >> 8) * 64) - regionBaseX_;
                    loaded.baseY = ((region.regionId() & 0xff) * 64) - regionBaseY_;
                    loaded.region = std::move(region);
                    objectCount_ += static_cast<int>(loaded.region.objects().getObjects().size());
                    regions_.push_back(std::move(loaded));
                } catch (const std::exception& ex) {
                    std::cerr << "Skipping map region " << neighborRegionId << ": " << ex.what() << "\n";
                }
            }
        }

        if (regions_.empty())
            throw std::runtime_error("No map regions could be loaded around " + std::to_string(regionId_));

        std::unordered_set<int> modelIds;
        for (const LoadedRegion& loaded : regions_) {
            for (const MapObject& object : loaded.region.objects().getObjects()) {
                const LocDef& loc = defs_.getLoc(object.id);
                int modelId = selectModelId(loc, object.type);
                if (modelId >= 0)
                    modelIds.insert(modelId);
            }
        }

        models_.clear();
        for (int modelId : modelIds) {
            try {
                std::vector<uint8_t> modelData = reader.readGzippedFile(1, modelId);
                if (modelData.empty())
                    continue;
                models_.emplace(modelId, Model::parse(modelData));
            } catch (const std::exception& ex) {
                std::cerr << "Skipping model " << modelId << ": " << ex.what() << "\n";
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Failed to load client world: " << ex.what() << "\n";
        return false;
    }

    return true;
}

void ClientApp::shutdown() {
    if (glContext_) {
        SDL_GL_DestroyContext(glContext_);
        glContext_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

void ClientApp::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (mode_ == AppMode::Start) {
            setMode(AppMode::Play);
            return;
        }
        mouseLook_ = true;
        return;
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
        mouseLook_ = false;
        return;
    }

    if (event.type == SDL_EVENT_MOUSE_MOTION && mouseLook_ && mode_ == AppMode::Play) {
        cameraYaw_ += event.motion.xrel * 0.35f;
        cameraPitch_ = std::clamp(cameraPitch_ + event.motion.yrel * 0.25f, 25.0f, 82.0f);
        return;
    }

    if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat)
        return;

    switch (event.key.scancode) {
        case SDL_SCANCODE_ESCAPE:
            if (mode_ == AppMode::Start)
                shouldQuit_ = true;
            else
                setMode(AppMode::Start);
            break;
        case SDL_SCANCODE_P:
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_SPACE:
            setMode(AppMode::Play);
            break;
        case SDL_SCANCODE_1:
        case SDL_SCANCODE_2:
        case SDL_SCANCODE_3:
        case SDL_SCANCODE_4:
            plane_ = static_cast<int>(event.key.scancode - SDL_SCANCODE_1);
            break;
        case SDL_SCANCODE_F:
            wireframe_ = !wireframe_;
            break;
        case SDL_SCANCODE_G:
            showGrid_ = !showGrid_;
            break;
        case SDL_SCANCODE_U:
            showObjects_ = !showObjects_;
            break;
        case SDL_SCANCODE_M:
            showModels_ = !showModels_;
            break;
        default:
            break;
    }
}

void ClientApp::updateCamera(float deltaSeconds) {
    if (mode_ != AppMode::Play)
        return;

    const bool* keys = SDL_GetKeyboardState(nullptr);
    float move = 24.0f * deltaSeconds;
    float rotate = 95.0f * deltaSeconds;
    float zoom = 85.0f * deltaSeconds;

    float yaw = cameraYaw_ * DegreesToRadians;
    float forwardX = std::sin(yaw);
    float forwardY = std::cos(yaw);
    float rightX = std::cos(yaw);
    float rightY = -std::sin(yaw);

    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) {
        playerX_ += forwardX * move;
        playerY_ += forwardY * move;
    }
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) {
        playerX_ -= forwardX * move;
        playerY_ -= forwardY * move;
    }
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
        playerX_ -= rightX * move;
        playerY_ -= rightY * move;
    }
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {
        playerX_ += rightX * move;
        playerY_ += rightY * move;
    }
    if (keys[SDL_SCANCODE_Q])
        cameraYaw_ -= rotate;
    if (keys[SDL_SCANCODE_E])
        cameraYaw_ += rotate;
    if (keys[SDL_SCANCODE_EQUALS] || keys[SDL_SCANCODE_KP_PLUS])
        cameraDistance_ -= zoom;
    if (keys[SDL_SCANCODE_MINUS] || keys[SDL_SCANCODE_KP_MINUS])
        cameraDistance_ += zoom;

    playerX_ = std::clamp(playerX_, -80.0f, 144.0f);
    playerY_ = std::clamp(playerY_, -80.0f, 144.0f);
    cameraDistance_ = std::clamp(cameraDistance_, 35.0f, 180.0f);
}

void ClientApp::render() {
    int width = WindowWidth;
    int height = WindowHeight;
    SDL_GetWindowSize(window_, &width, &height);

    switch (mode_) {
        case AppMode::Start:
            setup2D(width, height);
            renderStart();
            break;
        case AppMode::Play:
            setup3D(width, height);
            renderPlay();
            break;
    }
    SDL_GL_SwapWindow(window_);
}

void ClientApp::renderStart() {
    glClearColor(0.09f, 0.10f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    drawRect(280.0f, 260.0f, 464.0f, 160.0f, 0.16f, 0.29f, 0.24f);
    drawRect(304.0f, 284.0f, 416.0f, 112.0f, 0.24f, 0.42f, 0.34f);
}

void ClientApp::renderPlay() {
    renderWorld();
}

void ClientApp::setMode(AppMode mode) {
    mode_ = mode;
    switch (mode_) {
        case AppMode::Start:
            SDL_SetWindowTitle(window_, "cache-tool317 client - start");
            break;
        case AppMode::Play:
            SDL_SetWindowTitle(window_, "cache-tool317 client - play");
            break;
    }
}

void ClientApp::drawRect(float x, float y, float w, float h, float r, float g, float b) {
    glDisable(GL_TEXTURE_2D);
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void ClientApp::setup2D(int width, int height) {
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, width, height, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void ClientApp::setup3D(int width, int height) {
    float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    float fov = 55.0f * DegreesToRadians;
    float nearPlane = 1.0f;
    float farPlane = 500.0f;
    float top = std::tan(fov * 0.5f) * nearPlane;
    float right = top * aspect;

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-right, right, -top, top, nearPlane, farPlane);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void ClientApp::renderWorld() {
    glClearColor(0.47f, 0.58f, 0.70f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    applyFollowCamera();

    renderTerrain();
    if (showModels_)
        renderObjectModels();
    if (showObjects_)
        renderObjectMarkers();
    renderPlayerMarker();
}

void ClientApp::applyFollowCamera() {
    float playerHeight = terrainHeightAt(playerX_, playerY_);
    float yaw = cameraYaw_ * DegreesToRadians;
    float pitch = cameraPitch_ * DegreesToRadians;
    float horizontalDistance = std::cos(pitch) * cameraDistance_;
    float verticalDistance = std::sin(pitch) * cameraDistance_;

    Vec3 target{playerX_, playerHeight + 2.0f, playerY_};
    Vec3 eye{
        playerX_ - std::sin(yaw) * horizontalDistance,
        playerHeight + verticalDistance,
        playerY_ - std::cos(yaw) * horizontalDistance
    };

    lookAt(eye, target, {0.0f, 1.0f, 0.0f});
}

void ClientApp::renderTerrain() {
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_ ? GL_LINE : GL_FILL);

    for (const LoadedRegion& loaded : regions_)
        renderRegionTerrain(loaded);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (showGrid_)
        renderTerrainGrid();
}

void ClientApp::renderRegionTerrain(const LoadedRegion& loaded) {
    for (int x = 0; x < 63; x++) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int y = 0; y < 64; y++) {
            const Tile& left = loaded.region.terrain().getTile(plane_, x, y);
            const Tile& right = loaded.region.terrain().getTile(plane_, x + 1, y);

            Rgb leftColor = floorColor(left);
            glColor3ub(leftColor.r, leftColor.g, leftColor.b);
            glVertex3f(static_cast<float>(loaded.baseX + x), terrainHeight(left), cacheYToWorldZ(static_cast<float>(loaded.baseY + y)));

            Rgb rightColor = floorColor(right);
            glColor3ub(rightColor.r, rightColor.g, rightColor.b);
            glVertex3f(static_cast<float>(loaded.baseX + x + 1), terrainHeight(right), cacheYToWorldZ(static_cast<float>(loaded.baseY + y)));
        }
        glEnd();
    }
}

void ClientApp::renderTerrainGrid() {
    glColor3f(0.04f, 0.04f, 0.04f);

    for (const LoadedRegion& loaded : regions_)
        renderRegionTerrainGrid(loaded);
}

void ClientApp::renderRegionTerrainGrid(const LoadedRegion& loaded) {
    glBegin(GL_LINES);

    for (int x = 0; x < 64; x++) {
        for (int y = 0; y < 63; y++) {
            const Tile& a = loaded.region.terrain().getTile(plane_, x, y);
            const Tile& b = loaded.region.terrain().getTile(plane_, x, y + 1);
            glVertex3f(static_cast<float>(loaded.baseX + x), terrainHeight(a) + 0.04f, cacheYToWorldZ(static_cast<float>(loaded.baseY + y)));
            glVertex3f(static_cast<float>(loaded.baseX + x), terrainHeight(b) + 0.04f, cacheYToWorldZ(static_cast<float>(loaded.baseY + y + 1)));
        }
    }

    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 63; x++) {
            const Tile& a = loaded.region.terrain().getTile(plane_, x, y);
            const Tile& b = loaded.region.terrain().getTile(plane_, x + 1, y);
            glVertex3f(static_cast<float>(loaded.baseX + x), terrainHeight(a) + 0.04f, cacheYToWorldZ(static_cast<float>(loaded.baseY + y)));
            glVertex3f(static_cast<float>(loaded.baseX + x + 1), terrainHeight(b) + 0.04f, cacheYToWorldZ(static_cast<float>(loaded.baseY + y)));
        }
    }

    glEnd();
}

void ClientApp::renderPlayerMarker() {
    float h = terrainHeightAt(playerX_, playerY_);
    float radius = 0.7f;
    float height = 2.8f;

    glDisable(GL_TEXTURE_2D);
    glColor3f(0.95f, 0.92f, 0.25f);

    glBegin(GL_QUADS);
    glVertex3f(playerX_ - radius, h, playerY_ - radius);
    glVertex3f(playerX_ + radius, h, playerY_ - radius);
    glVertex3f(playerX_ + radius, h + height, playerY_ - radius);
    glVertex3f(playerX_ - radius, h + height, playerY_ - radius);

    glVertex3f(playerX_ + radius, h, playerY_ - radius);
    glVertex3f(playerX_ + radius, h, playerY_ + radius);
    glVertex3f(playerX_ + radius, h + height, playerY_ + radius);
    glVertex3f(playerX_ + radius, h + height, playerY_ - radius);

    glVertex3f(playerX_ + radius, h, playerY_ + radius);
    glVertex3f(playerX_ - radius, h, playerY_ + radius);
    glVertex3f(playerX_ - radius, h + height, playerY_ + radius);
    glVertex3f(playerX_ + radius, h + height, playerY_ + radius);

    glVertex3f(playerX_ - radius, h, playerY_ + radius);
    glVertex3f(playerX_ - radius, h, playerY_ - radius);
    glVertex3f(playerX_ - radius, h + height, playerY_ - radius);
    glVertex3f(playerX_ - radius, h + height, playerY_ + radius);
    glEnd();

    glColor3f(1.0f, 0.45f, 0.20f);
    glBegin(GL_TRIANGLES);
    glVertex3f(playerX_ - radius, h + height, playerY_ - radius);
    glVertex3f(playerX_ + radius, h + height, playerY_ - radius);
    glVertex3f(playerX_, h + height + 1.2f, playerY_);

    glVertex3f(playerX_ + radius, h + height, playerY_ - radius);
    glVertex3f(playerX_ + radius, h + height, playerY_ + radius);
    glVertex3f(playerX_, h + height + 1.2f, playerY_);

    glVertex3f(playerX_ + radius, h + height, playerY_ + radius);
    glVertex3f(playerX_ - radius, h + height, playerY_ + radius);
    glVertex3f(playerX_, h + height + 1.2f, playerY_);

    glVertex3f(playerX_ - radius, h + height, playerY_ + radius);
    glVertex3f(playerX_ - radius, h + height, playerY_ - radius);
    glVertex3f(playerX_, h + height + 1.2f, playerY_);
    glEnd();
}

void ClientApp::renderObjectMarkers() {
    glDisable(GL_TEXTURE_2D);
    glLineWidth(2.0f);

    for (const LoadedRegion& loaded : regions_)
        renderRegionObjectMarkers(loaded);

    glLineWidth(1.0f);
}

void ClientApp::renderObjectModels() {
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);

    for (const LoadedRegion& loaded : regions_)
        renderRegionObjectModels(loaded);
}

void ClientApp::renderRegionObjectModels(const LoadedRegion& loaded) {
    for (const MapObject& object : loaded.region.objects().getObjects()) {
        if (object.z != plane_)
            continue;
        renderObjectModel(loaded, object);
    }
}

void ClientApp::renderObjectModel(const LoadedRegion& loaded, const MapObject& object) {
    const LocDef& loc = defs_.getLoc(object.id);
    if (loc.modelIDs.empty())
        return;

    float x = static_cast<float>(loaded.baseX + object.x);
    int width = std::max(1, loc.width);
    int length = std::max(1, loc.length);
    if ((object.rotation & 1) == 1)
        std::swap(width, length);
    float z = cacheYToWorldZ(static_cast<float>(loaded.baseY + object.y)) - static_cast<float>(length);
    if (std::abs(x - playerX_) > 48.0f || std::abs(z - playerY_) > 48.0f)
        return;

    int modelId = selectModelId(loc, object.type);
    if (modelId < 0)
        return;

    auto modelIt = models_.find(modelId);
    if (modelIt == models_.end())
        return;

    float y = terrainHeightAt(x, z);

    glPushMatrix();
    glTranslatef(x + width * 0.5f, y, z + length * 0.5f);
    glRotatef(static_cast<float>(cacheRotationToWorld(object.rotation) & 3) * 90.0f, 0.0f, 1.0f, 0.0f);
    glTranslatef(
        static_cast<float>(loc.offsetX) / 128.0f,
        -static_cast<float>(loc.offsetY) / 128.0f,
        static_cast<float>(loc.offsetZ) / 128.0f);
    glScalef(
        static_cast<float>(loc.scaleX) / (128.0f * 128.0f),
        -static_cast<float>(loc.scaleY) / (128.0f * 128.0f),
        static_cast<float>(loc.scaleZ) / (128.0f * 128.0f));
    renderModelSolid(modelIt->second);
    glPopMatrix();
}

void ClientApp::renderModelWireframe(const Model& model) {
    glBegin(GL_LINES);
    for (const ModelTriangle& triangle : model.triangles()) {
        const ModelVertex& a = model.vertices()[triangle.a];
        const ModelVertex& b = model.vertices()[triangle.b];
        const ModelVertex& c = model.vertices()[triangle.c];

        glVertex3f(static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z));
        glVertex3f(static_cast<float>(b.x), static_cast<float>(b.y), static_cast<float>(b.z));

        glVertex3f(static_cast<float>(b.x), static_cast<float>(b.y), static_cast<float>(b.z));
        glVertex3f(static_cast<float>(c.x), static_cast<float>(c.y), static_cast<float>(c.z));

        glVertex3f(static_cast<float>(c.x), static_cast<float>(c.y), static_cast<float>(c.z));
        glVertex3f(static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z));
    }
    glEnd();
}

void ClientApp::renderModelSolid(const Model& model) {
    Vec3 lightDir = normalize({0.3f, -0.8f, 0.5f});
    float ambient = 25.0f;
    float diffuse = 70.0f;

    glBegin(GL_TRIANGLES);
    for (const ModelTriangle& triangle : model.triangles()) {
        const ModelVertex& a = model.vertices()[triangle.a];
        const ModelVertex& b = model.vertices()[triangle.b];
        const ModelVertex& c = model.vertices()[triangle.c];

        Vec3 ab = {static_cast<float>(b.x - a.x), static_cast<float>(b.y - a.y), static_cast<float>(b.z - a.z)};
        Vec3 ac = {static_cast<float>(c.x - a.x), static_cast<float>(c.y - a.y), static_cast<float>(c.z - a.z)};
        Vec3 normal = normalize(cross(ab, ac));

        float intensity = std::max(0.0f, normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z);
        int shade = static_cast<int>(ambient + intensity * diffuse);

        Rgb color = rsHslToRgb(triangle.color, shade);
        glColor3ub(color.r, color.g, color.b);

        glVertex3f(static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z));
        glVertex3f(static_cast<float>(b.x), static_cast<float>(b.y), static_cast<float>(b.z));
        glVertex3f(static_cast<float>(c.x), static_cast<float>(c.y), static_cast<float>(c.z));
    }
    glEnd();
}

int ClientApp::selectModelId(const LocDef& loc, int objectType) const {
    if (loc.modelIDs.empty())
        return -1;

    if (!loc.modelTypes.empty()) {
        for (std::size_t i = 0; i < loc.modelTypes.size() && i < loc.modelIDs.size(); i++) {
            if (loc.modelTypes[i] == objectType)
                return loc.modelIDs[i];
        }
        return -1;
    }

    return loc.modelIDs.front();
}

void ClientApp::renderRegionObjectMarkers(const LoadedRegion& loaded) {
    for (const MapObject& object : loaded.region.objects().getObjects()) {
        if (object.z != plane_)
            continue;
        renderObjectMarker(loaded, object);
    }
}

void ClientApp::renderObjectMarker(const LoadedRegion& loaded, const MapObject& object) {
    const LocDef& loc = defs_.getLoc(object.id);
    float x = static_cast<float>(loaded.baseX + object.x);
    int rotation = cacheRotationToWorld(object.rotation);
    int width = object.type >= 10 ? std::max(1, loc.width) : 1;
    int length = object.type >= 10 ? std::max(1, loc.length) : 1;
    if (object.type >= 10 && (rotation & 1) == 1)
        std::swap(width, length);
    float z = cacheYToWorldZ(static_cast<float>(loaded.baseY + object.y)) - static_cast<float>(length);
    float y = terrainHeightAt(x, z) + 0.08f;

    if (object.type == 0) {
        glColor3f(0.05f, 0.05f, 0.05f);
        switch (rotation & 3) {
            case 0:
                drawObjectLine(x, y, z, x, y + 2.2f, z + 1.0f);
                break;
            case 1:
                drawObjectLine(x, y, z + 1.0f, x + 1.0f, y + 2.2f, z + 1.0f);
                break;
            case 2:
                drawObjectLine(x + 1.0f, y, z, x + 1.0f, y + 2.2f, z + 1.0f);
                break;
            case 3:
                drawObjectLine(x, y, z, x + 1.0f, y + 2.2f, z);
                break;
        }
        return;
    }

    if (object.type == 2) {
        glColor3f(0.02f, 0.02f, 0.02f);
        switch (rotation & 3) {
            case 0:
                drawObjectLine(x, y, z, x, y + 2.2f, z + 1.0f);
                drawObjectLine(x, y, z + 1.0f, x + 1.0f, y + 2.2f, z + 1.0f);
                break;
            case 1:
                drawObjectLine(x, y, z + 1.0f, x + 1.0f, y + 2.2f, z + 1.0f);
                drawObjectLine(x + 1.0f, y, z, x + 1.0f, y + 2.2f, z + 1.0f);
                break;
            case 2:
                drawObjectLine(x + 1.0f, y, z, x + 1.0f, y + 2.2f, z + 1.0f);
                drawObjectLine(x, y, z, x + 1.0f, y + 2.2f, z);
                break;
            case 3:
                drawObjectLine(x, y, z, x + 1.0f, y + 2.2f, z);
                drawObjectLine(x, y, z, x, y + 2.2f, z + 1.0f);
                break;
        }
        return;
    }

    if (object.type == 9) {
        glColor3f(0.08f, 0.08f, 0.08f);
        if ((rotation & 1) == 0)
            drawObjectLine(x, y, z, x + 1.0f, y + 1.2f, z + 1.0f);
        else
            drawObjectLine(x, y, z + 1.0f, x + 1.0f, y + 1.2f, z);
        return;
    }

    if (object.type >= 10) {
        if (loc.name == "null")
            glColor3f(0.18f, 0.18f, 0.18f);
        else if (loc.blocksWalk)
            glColor3f(0.42f, 0.20f, 0.16f);
        else
            glColor3f(0.75f, 0.55f, 0.16f);

        drawObjectBox(x, y, z, static_cast<float>(width), static_cast<float>(length), loc.blocksWalk ? 2.4f : 1.3f);
        return;
    }

    glColor3f(0.75f, 0.15f, 0.55f);
    drawObjectBox(x + 0.3f, y, z + 0.3f, 0.4f, 0.4f, 1.0f);
}

void ClientApp::drawObjectBox(float x, float y, float z, float width, float length, float height) {
    float x2 = x + width;
    float y2 = y + height;
    float z2 = z + length;

    glBegin(GL_LINES);
    glVertex3f(x, y, z);
    glVertex3f(x2, y, z);
    glVertex3f(x2, y, z);
    glVertex3f(x2, y, z2);
    glVertex3f(x2, y, z2);
    glVertex3f(x, y, z2);
    glVertex3f(x, y, z2);
    glVertex3f(x, y, z);

    glVertex3f(x, y2, z);
    glVertex3f(x2, y2, z);
    glVertex3f(x2, y2, z);
    glVertex3f(x2, y2, z2);
    glVertex3f(x2, y2, z2);
    glVertex3f(x, y2, z2);
    glVertex3f(x, y2, z2);
    glVertex3f(x, y2, z);

    glVertex3f(x, y, z);
    glVertex3f(x, y2, z);
    glVertex3f(x2, y, z);
    glVertex3f(x2, y2, z);
    glVertex3f(x2, y, z2);
    glVertex3f(x2, y2, z2);
    glVertex3f(x, y, z2);
    glVertex3f(x, y2, z2);
    glEnd();
}

void ClientApp::drawObjectLine(float x1, float y1, float z1, float x2, float y2, float z2) {
    glBegin(GL_LINES);
    glVertex3f(x1, y1, z1);
    glVertex3f(x2, y2, z2);
    glEnd();
}

float ClientApp::terrainHeightAt(float x, float y) const {
    int tileX = static_cast<int>(std::floor(x));
    int tileY = static_cast<int>(std::floor(worldZToCacheY(y)));

    for (const LoadedRegion& loaded : regions_) {
        int localX = tileX - loaded.baseX;
        int localY = tileY - loaded.baseY;
        if (localX < 0 || localX >= 64 || localY < 0 || localY >= 64)
            continue;

        return terrainHeight(loaded.region.terrain().getTile(plane_, localX, localY));
    }

    return 0.0f;
}

float ClientApp::cacheYToWorldZ(float cacheY) const {
    return 128.0f - cacheY;
}

float ClientApp::worldZToCacheY(float worldZ) const {
    return 128.0f - worldZ;
}

int ClientApp::cacheRotationToWorld(int rotation) const {
    rotation &= 3;
    return (4 - rotation) & 3;
}

Rgb ClientApp::floorColor(const Tile& tile) const {
    if (tile.overlayId > 0) {
        try {
            return fromRgbInt(defs_.getFlo(tile.overlayId - 1).rgb);
        } catch (...) {
        }
    }

    if (tile.underlayId > 0) {
        try {
            return fromRgbInt(defs_.getFlo(tile.underlayId - 1).rgb);
        } catch (...) {
        }
    }

    return {60, 70, 60};
}
