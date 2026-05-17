#include "Mesh.h"

Mesh Mesh::FromModelDef(const ModelDef& def) {

    Mesh mesh;

    // vertices
    for (size_t i = 0; i < def.vertexX.size(); i++) {

        Vertex v;

        v.x = static_cast<float>(def.vertexX[i]);
        v.y = static_cast<float>(def.vertexY[i]);
        v.z = static_cast<float>(def.vertexZ[i]);

        mesh.vertices.push_back(v);
    }

    // triangle indices
    for (size_t i = 0; i < def.triA.size(); i++) {

        mesh.indices.push_back(def.triA[i]);
        mesh.indices.push_back(def.triB[i]);
        mesh.indices.push_back(def.triC[i]);
    }

    return mesh;
}
