#pragma once
#include <cmath>

struct Mat4 { float m[16] = {}; };

Mat4 mat4Identity();
Mat4 mat4Multiply(const Mat4& a, const Mat4& b);
Mat4 mat4Translate(float x, float y, float z);
Mat4 mat4RotateY(float angle);
Mat4 mat4Perspective(float fovY, float aspect, float near, float far);
