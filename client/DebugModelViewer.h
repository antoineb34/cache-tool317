#pragma once

// Minimal math helpers for the debug viewer — no external dependencies.
struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vec3 operator+(const Vec3& v) const { return {x+v.x, y+v.y, z+v.z}; }
    Vec3 operator-(const Vec3& v) const { return {x-v.x, y-v.y, z-v.z}; }
    Vec3 operator*(float s) const { return {x*s, y*s, z*s}; }
    Vec3 operator/(float s) const { return {x/s, y/s, z/s}; }
    float dot(const Vec3& v) const { return x*v.x + y*v.y + z*v.z; }
    Vec3 cross(const Vec3& v) const { return {y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x}; }
    float length() const { return std::sqrt(x*x + y*y + z*z); }
    Vec3 normalized() const { float l = length(); return l > 0 ? (*this / l) : Vec3{}; }
};

struct Mat4 {
    float m[16] = {0};
    static Mat4 identity();
    static Mat4 perspective(float fovY, float aspect, float nearZ, float farZ);
    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up);
    static Mat4 rotateY(float deg);
    Mat4 operator*(const Mat4& o) const;
};

// HSL→RGB (RS317 packed HSL format)
uint32_t hslToRgb(uint16_t hsl);