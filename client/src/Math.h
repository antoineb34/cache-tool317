#pragma once

#include <cmath>
#include <cstdint>

// Simple 3D vector used for vertices, normals, light direction, etc.
struct Vec3f {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    Vec3f() = default;
    Vec3f(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3f operator+(const Vec3f& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vec3f operator-(const Vec3f& v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vec3f operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3f operator/(float s) const { return {x / s, y / s, z / s}; }

    Vec3f& operator+=(const Vec3f& v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vec3f& operator-=(const Vec3f& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    Vec3f& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    float dot(const Vec3f& v) const { return x * v.x + y * v.y + z * v.z; }
    Vec3f cross(const Vec3f& v) const {
        return {y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x};
    }
    float length() const { return std::sqrt(x * x + y * y + z * z); }
    Vec3f normalized() const { float l = length(); return l > 0 ? (*this / l) : Vec3f{}; }

    // RS317 uses Y-up; we flip Z so +Z = north (OpenGL convention)
    static Vec3f fromRS317(int x, int y, int z) {
        return {static_cast<float>(x), static_cast<float>(-y), static_cast<float>(-z)};
    }
};

// 4x4 column-major matrix for OpenGL
struct Mat4f {
    float m[16] = {0};

    Mat4f() = default;

    static Mat4f identity() {
        Mat4f mat;
        mat.m[0] = mat.m[5] = mat.m[10] = mat.m[15] = 1.0f;
        return mat;
    }

    static Mat4f perspective(float fovY, float aspect, float nearZ, float farZ) {
        Mat4f mat;
        float f = 1.0f / std::tan(fovY * 0.5f * 3.14159265f / 180.0f);
        float nf = 1.0f / (nearZ - farZ);
        mat.m[0]  = f / aspect;
        mat.m[5]  = f;
        mat.m[10] = (farZ + nearZ) * nf;
        mat.m[11] = -1.0f;
        mat.m[14] = 2.0f * farZ * nearZ * nf;
        return mat;
    }

    static Mat4f lookAt(const Vec3f& eye, const Vec3f& center, const Vec3f& up) {
        Mat4f mat;
        Vec3f f = (center - eye).normalized();
        Vec3f s = f.cross(up).normalized();
        Vec3f u = s.cross(f);

        mat.m[0] = s.x;  mat.m[4] = s.y;  mat.m[8]  = s.z;  mat.m[12] = -s.dot(eye);
        mat.m[1] = u.x;  mat.m[5] = u.y;  mat.m[9]  = u.z;  mat.m[13] = -u.dot(eye);
        mat.m[2] = -f.x; mat.m[6] = -f.y; mat.m[10] = -f.z; mat.m[14] = f.dot(eye);
        mat.m[15] = 1.0f;
        return mat;
    }

    static Mat4f rotateY(float angleDeg) {
        Mat4f mat = identity();
        float rad = angleDeg * 3.14159265f / 180.0f;
        float c = std::cos(rad), s = std::sin(rad);
        mat.m[0] = c;  mat.m[2] = s;
        mat.m[8] = -s; mat.m[10] = c;
        return mat;
    }

    static Mat4f translate(float x, float y, float z) {
        Mat4f mat = identity();
        mat.m[12] = x; mat.m[13] = y; mat.m[14] = z;
        return mat;
    }

    Mat4f operator*(const Mat4f& other) const {
        Mat4f result;
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                float sum = 0;
                for (int k = 0; k < 4; k++)
                    sum += m[k * 4 + r] * other.m[c * 4 + k];
                result.m[c * 4 + r] = sum;
            }
        }
        return result;
    }
};

// HSL-to-RGB conversion (RS317 packed HSL format)
// Packed: bits 15-10 = hue (0-63), bits 9-7 = sat (0-7), bits 6-0 = light (0-127)
inline uint32_t hslToRgb(uint16_t hsl) {
    int hue = (hsl >> 10) & 0x3F;
    int sat = (hsl >> 7) & 0x07;
    int light = hsl & 0x7F;

    float h = (hue / 64.0f) + 0.0078125f;
    float s = (sat / 8.0f) + 0.0625f;
    float l = light / 128.0f;

    float r, g, b;
    if (s == 0.0f) {
        r = g = b = l;
    } else {
        float a, p;
        if (l < 0.5f) {
            a = l * (1.0f + s);
        } else {
            a = l + s - l * s;
        }
        p = 2.0f * l - a;

        auto hueToRgb = [](float h, float a, float p) -> float {
            if (h < 0.0f) h += 1.0f;
            if (h > 1.0f) h -= 1.0f;
            if (6.0f * h < 1.0f) return p + (a - p) * 6.0f * h;
            if (2.0f * h < 1.0f) return a;
            if (3.0f * h < 2.0f) return p + (a - p) * ((2.0f / 3.0f) - h) * 6.0f;
            return p;
        };

        r = hueToRgb(h + 1.0f / 3.0f, a, p);
        g = hueToRgb(h, a, p);
        b = hueToRgb(h - 1.0f / 3.0f, a, p);
    }

    int ri = static_cast<int>(r * 256.0f);
    int gi = static_cast<int>(g * 256.0f);
    int bi = static_cast<int>(b * 256.0f);
    ri = std::max(0, std::min(255, ri));
    gi = std::max(0, std::min(255, gi));
    bi = std::max(0, std::min(255, bi));

    return (ri << 16) | (gi << 8) | bi;
}

// Apply lightness modifier to an HSL color (used for face shading)
inline uint16_t applyLightness(uint16_t hsl, int lightness) {
    int hue = (hsl >> 10) & 0x3F;
    int sat = (hsl >> 7) & 0x07;
    int lit = hsl & 0x7F;

    int newLit = (lightness * lit) >> 7;
    if (newLit < 2) newLit = 2;
    if (newLit > 126) newLit = 126;

    return static_cast<uint16_t>((hue << 10) | (sat << 7) | newLit);
}