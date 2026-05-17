#include "Mat4.h"

#include <cmath>

Mat4::Mat4() {
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            m[row][col] = 0.0f;
        }
    }
}

Mat4 Mat4::Identity() {
    Mat4 mat;

    mat.m[0][0] = 1.0f;
    mat.m[1][1] = 1.0f;
    mat.m[2][2] = 1.0f;
    mat.m[3][3] = 1.0f;

    return mat;
}

Mat4 Mat4::Translation(float x, float y, float z) {
    Mat4 mat = Identity();

    mat.m[0][3] = x;
    mat.m[1][3] = y;
    mat.m[2][3] = z;

    return mat;
}

Mat4 Mat4::RotationY(float angle) {
    Mat4 mat = Identity();

    float c = std::cos(angle);
    float s = std::sin(angle);

    mat.m[0][0] = c;
    mat.m[0][2] = s;

    mat.m[2][0] = -s;
    mat.m[2][2] = c;

    return mat;
}

Mat4 Mat4::operator*(const Mat4& other) const {
    Mat4 result;

    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {

            result.m[row][col] = 0.0f;

            for (int k = 0; k < 4; k++) {
                result.m[row][col] +=
                    m[row][k] * other.m[k][col];
            }
        }
    }

    return result;
}

Vertex Mat4::operator*(const Vertex& v) const {
    Vertex result;

    result.x =
        m[0][0] * v.x +
        m[0][1] * v.y +
        m[0][2] * v.z +
        m[0][3];

    result.y =
        m[1][0] * v.x +
        m[1][1] * v.y +
        m[1][2] * v.z +
        m[1][3];

    result.z =
        m[2][0] * v.x +
        m[2][1] * v.y +
        m[2][2] * v.z +
        m[2][3];

    return result;
}
