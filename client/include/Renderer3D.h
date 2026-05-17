#pragma once

#include <vector>
#include <SDL3/SDL.h>

#include "Renderable.h"
#include "Point2D.h"
#include "Vertex.h"
#include "Mesh.h"
#include "Mat4.h"

class Renderer3D {
public:
    Renderer3D(SDL_Renderer* sdlRenderer, int screenWidth, int screenHeight);

    void BeginFrame();
    void Submit(const Mesh& mesh, const Mat4& transform);
    void Render();
    void EndFrame();

private:
    SDL_Renderer* sdlRenderer;
    int screenWidth;
    int screenHeight;

    std::vector<Renderable> queue;

    Point2D Project(const Vertex& v) const;
    void DrawWireMesh(const Mesh& mesh, const Mat4& transform) const;
};
