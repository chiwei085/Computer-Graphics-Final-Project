#include "future_gaze/camera/orbit_camera.hpp"

#include <algorithm>
#include <cmath>

namespace future_gaze
{

namespace
{

constexpr float kDragSensitivity = 0.008f;
constexpr float kMinPitch = -1.25f;
constexpr float kMaxPitch = 1.25f;
constexpr float kMinDistance = 2.0f;
constexpr float kMaxDistance = 18.0f;

}  // namespace

Mat4 OrbitCamera::ViewMatrix() const {
    return Mat4::LookAt(Eye(), Target(), Vec3{0.0f, 1.0f, 0.0f});
}

void OrbitCamera::BeginDrag(int x, int y) {
    dragging_ = true;
    last_x_ = x;
    last_y_ = y;
}

void OrbitCamera::DragTo(int x, int y) {
    if (!dragging_) {
        return;
    }

    const int dx = x - last_x_;
    const int dy = y - last_y_;
    // Negate so drag direction matches scene motion (drag right → scene goes
    // right).
    yaw_radians_ -= static_cast<float>(dx) * kDragSensitivity;
    pitch_radians_ -= static_cast<float>(dy) * kDragSensitivity;
    pitch_radians_ = std::clamp(pitch_radians_, kMinPitch, kMaxPitch);
    last_x_ = x;
    last_y_ = y;
}

void OrbitCamera::EndDrag() {
    dragging_ = false;
}

void OrbitCamera::Zoom(float wheel_steps) {
    distance_ =
        std::clamp(distance_ - wheel_steps * 0.35f, kMinDistance, kMaxDistance);
}

Vec3 OrbitCamera::Eye() const {
    const float cos_pitch = std::cos(pitch_radians_);
    const Vec3 t = Target();
    return Vec3{t.x + distance_ * std::sin(yaw_radians_) * cos_pitch,
                t.y + distance_ * std::sin(pitch_radians_),
                t.z + distance_ * std::cos(yaw_radians_) * cos_pitch};
}

}  // namespace future_gaze
