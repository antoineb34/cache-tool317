#pragma once

#include "Vertex.h"

class Mat4 {
public:

    float m[4][4];

    Mat4();

    static Mat4 Identity();

    static Mat4 Translation(float x, float y, float z);

    static Mat4 RotationY(float angle);

    Mat4 operator*(const Mat4& other) const;

    Vertex operator*(const Vertex& v) const;
};
