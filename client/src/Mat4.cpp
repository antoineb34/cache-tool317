#include "Mat4.h"

Mat4 mat4Identity() {
    Mat4 r;
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

Mat4 mat4Multiply(const Mat4& a, const Mat4& b) {
    Mat4 r;
    for (int col = 0; col < 4; col++)
        for (int row = 0; row < 4; row++)
            for (int k   = 0; k   < 4; k++)
                r.m[col*4+row] += a.m[k*4+row] * b.m[col*4+k];
    return r;
}

Mat4 mat4Translate(float x, float y, float z) {
    Mat4 r = mat4Identity();
    r.m[12] = x; r.m[13] = y; r.m[14] = z;
    return r;
}

Mat4 mat4RotateY(float a) {
    Mat4 r = mat4Identity();
    r.m[0]  =  std::cos(a); r.m[8]  = std::sin(a);
    r.m[2]  = -std::sin(a); r.m[10] = std::cos(a);
    return r;
}

Mat4 mat4Perspective(float fovY, float aspect, float near, float far) {
    Mat4 r;
    float f = 1.0f / std::tan(fovY * 0.5f);
    r.m[0]  = f / aspect;
    r.m[5]  = f;
    r.m[10] = (far + near)        / (near - far);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * far * near) / (near - far);
    return r;
}
