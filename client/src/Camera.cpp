#include "Camera.h"

#include <algorithm>

Camera::Camera() = default;

void Camera::rotate(float deltaYaw, float deltaPitch) {
    yaw_ += deltaYaw;
    pitch_ += deltaPitch;

    // Clamp pitch to avoid flipping
    pitch_ = std::max(1.0f, std::min(89.0f, pitch_));

    // Normalize yaw
    while (yaw_ < 0.0f) yaw_ += 360.0f;
    while (yaw_ >= 360.0f) yaw_ -= 360.0f;

    viewDirty_ = true;
}

void Camera::zoom(float delta) {
    distance_ -= delta;
    distance_ = std::max(10.0f, std::min(500.0f, distance_));
    viewDirty_ = true;
}

void Camera::updateView() {
    if (!viewDirty_) return;
    viewDirty_ = false;

    float radYaw = yaw_ * 3.14159265f / 180.0f;
    float radPitch = pitch_ * 3.14159265f / 180.0f;

    float cosPitch = std::cos(radPitch);
    float sinPitch = std::sin(radPitch);
    float cosYaw = std::cos(radYaw);
    float sinYaw = std::sin(radYaw);

    // Camera position on the sphere
    Vec3f offset;
    offset.x = distance_ * cosPitch * sinYaw;
    offset.y = distance_ * sinPitch;
    offset.z = distance_ * cosPitch * cosYaw;

    Vec3f eye = target_ + offset;

    // Up vector: mostly up, but tilts with pitch
    Vec3f up;
    if (std::abs(pitch_) > 89.0f) {
        // Near zenith/nadir, use a different up reference
        up = Vec3f(0, (pitch_ > 45.0f) ? 1 : -1, 0);
    } else {
        up = Vec3f(0, 1, 0);
    }

    viewMatrix_ = Mat4f::lookAt(eye, target_, up);
}

Vec3f Camera::getPosition() const {
    float radYaw = yaw_ * 3.14159265f / 180.0f;
    float radPitch = pitch_ * 3.14159265f / 180.0f;

    float cosPitch = std::cos(radPitch);
    Vec3f offset;
    offset.x = distance_ * cosPitch * std::sin(radYaw);
    offset.y = distance_ * std::sin(radPitch);
    offset.z = distance_ * cosPitch * std::cos(radYaw);

    return target_ + offset;
}

Mat4f Camera::getView() const {
    const_cast<Camera*>(this)->updateView();
    return viewMatrix_;
}

Mat4f Camera::getProjection(float aspect) const {
    return Mat4f::perspective(60.0f, aspect, 0.1f, 1000.0f);
}

Mat4f Camera::getViewProjection(int windowWidth, int windowHeight) const {
    float aspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    return getProjection(aspect) * getView();
}