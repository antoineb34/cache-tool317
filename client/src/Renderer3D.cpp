#include "Renderer3D.h"
#include <iostream>

Renderer3D::Renderer3D(SDL_Renderer* sdlRenderer, int screenWidth, int screenHeight)
    : sdlRenderer(sdlRenderer),
      screenWidth(screenWidth),
      screenHeight(screenHeight) {}

void Renderer3D::BeginFrame() {
    queue.clear();

    SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
    SDL_RenderClear(sdlRenderer);
}

void Renderer3D::Submit(const Mesh& mesh, const Mat4& transform) {
    queue.push_back(Renderable{ &mesh, transform });
}

void Renderer3D::Render() {
    SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 255, 255);

    for (const Renderable& r : queue) {
        DrawWireMesh(*r.mesh, r.transform);
    }
}

void Renderer3D::DrawWireMesh(const Mesh& mesh, const Mat4& transform) const {
    std::cout << "Drawing mesh\n";
    for (int i = 0; i < mesh.indices.size(); i += 3) {

        std::cout << "Triangle\n";
        Vertex a = transform * mesh.vertices[mesh.indices[i]];
        Vertex b = transform * mesh.vertices[mesh.indices[i + 1]];
        Vertex c = transform * mesh.vertices[mesh.indices[i + 2]];

        Point2D pa = Project(a);
        Point2D pb = Project(b);
        Point2D pc = Project(c);

        SDL_RenderLine(sdlRenderer, pa.x, pa.y, pb.x, pb.y);
        SDL_RenderLine(sdlRenderer, pb.x, pb.y, pc.x, pc.y);
        SDL_RenderLine(sdlRenderer, pc.x, pc.y, pa.x, pa.y);
    }
}

void Renderer3D::EndFrame() {
    SDL_RenderPresent(sdlRenderer);
}

Point2D Renderer3D::Project(const Vertex& v) const {

    float fov = 256.0f;

    Point2D p;

    p.x = (v.x * fov / v.z) + (screenWidth / 2.0f);
    p.y = (-v.y * fov / v.z) + (screenHeight / 2.0f);

    return p;
}
