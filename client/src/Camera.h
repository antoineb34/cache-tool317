#pragma once

#include "Math.h"

// Simple orbit camera that rotates around a target point
class Camera {
public:
    Camera();

    // Get the view-projection matrix
    Mat4f getViewProjection(int windowWidth, int windowHeight) const;
    Mat4f getView() const;
    Mat4f getProjection(float aspect) const;

    // Mouse input to rotate
    void rotate(float deltaYaw, float deltaPitch);

    // Zoom (distance from target)
    void zoom(float delta);

    // Set target point to orbit around
    void setTarget(const Vec3f& target) { target_ = target; }

    // Get camera position
    Vec3f getPosition() const;

    // Update the view matrix cache
    void updateView();

    float getYaw() const { return yaw_; }
    float getPitch() const { return pitch_; }
    float getDistance() const { return distance_; }

private:
    Vec3f target_ = {0, 0, 0};
    float yaw_ = 180.0f;    // Horizontal rotation (degrees)
    float pitch_ = 45.0f;   // Vertical rotation (degrees, clamped)
    float distance_ = 100.0f; // Orbit distance

    Mat4f viewMatrix_;
    bool viewDirty_ = true;
};