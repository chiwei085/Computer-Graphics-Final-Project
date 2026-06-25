#include "future_gaze/camera/orbit_camera.hpp"

#include <algorithm>
#include <cmath>

namespace future_gaze
{

namespace
{

constexpr float kOrbitSensitivity = 0.008f;
constexpr float kPanSensitivity   = 0.0012f;
constexpr float kMaxPitch         = 1.25f;   // ≈ 72°
constexpr float kMinDistance      = 2.0f;
constexpr float kMaxDistance      = 18.0f;

// Initial camera state (duplicated in Reset())
const Quat  kInitOrientation = Quat::FromAxisAngle({1.0f, 0.0f, 0.0f}, -0.25f);
const Vec3  kInitTarget{0.0f, 1.5f, -0.5f};
constexpr float kInitPitch    = 0.25f;
constexpr float kInitDistance = 9.0f;

}  // namespace

// ── View ─────────────────────────────────────────────────────────────────────

Mat4 OrbitCamera::ViewMatrix() const {
    return Mat4::LookAt(Eye(), target_, orientation_.Rotate({0.0f, 1.0f, 0.0f}));
}

Vec3 OrbitCamera::Eye() const {
    return target_ + orientation_.Rotate(Vec3{0.0f, 0.0f, distance_});
}

// ── Orbit ─────────────────────────────────────────────────────────────────────

void OrbitCamera::BeginDrag(int x, int y) {
    dragging_ = true;
    last_x_   = x;
    last_y_   = y;
}

void OrbitCamera::DragTo(int x, int y) {
    if (!dragging_) {
        return;
    }
    const float dx = static_cast<float>(x - last_x_);
    const float dy = static_cast<float>(y - last_y_);
    last_x_ = x;
    last_y_ = y;

    // Yaw: world-space Y axis (left-multiply keeps world-up stable).
    // Negate dx so drag-right makes scene orbit right.
    const Quat yaw = Quat::FromAxisAngle({0.0f, 1.0f, 0.0f}, -dx * kOrbitSensitivity);
    orientation_   = (yaw * orientation_).Normalized();

    // Pitch: camera-local X axis (right-multiply stays local).
    // Negate dy so drag-down = bird's eye (camera rises).
    // Clamp via separate accumulator to prevent flip past ±90°.
    const float raw_delta  = dy * kOrbitSensitivity;
    const float new_pitch  = std::clamp(pitch_accumulated_ + raw_delta, -kMaxPitch, kMaxPitch);
    const float clamped    = new_pitch - pitch_accumulated_;
    pitch_accumulated_     = new_pitch;

    const Quat pitch = Quat::FromAxisAngle({1.0f, 0.0f, 0.0f}, -clamped);
    orientation_     = (orientation_ * pitch).Normalized();
}

void OrbitCamera::EndDrag() {
    dragging_ = false;
}

// ── Pan ───────────────────────────────────────────────────────────────────────

void OrbitCamera::BeginPan(int x, int y) {
    panning_    = true;
    pan_last_x_ = x;
    pan_last_y_ = y;
}

void OrbitCamera::PanTo(int x, int y) {
    if (!panning_) {
        return;
    }
    const float dx = static_cast<float>(x - pan_last_x_);
    const float dy = static_cast<float>(y - pan_last_y_);
    pan_last_x_ = x;
    pan_last_y_ = y;

    const Vec3  right = orientation_.Rotate({1.0f, 0.0f, 0.0f});
    const Vec3  up    = orientation_.Rotate({0.0f, 1.0f, 0.0f});
    const float scale = distance_ * kPanSensitivity;

    // Drag right → target shifts in -right so scene appears to slide right.
    // Drag down (dy>0) → target shifts in +up so scene slides down.
    target_ = target_ - right * (dx * scale) + up * (dy * scale);
}

void OrbitCamera::EndPan() {
    panning_ = false;
}

// ── Zoom / Reset ──────────────────────────────────────────────────────────────

void OrbitCamera::Zoom(float wheel_steps) {
    distance_ = std::clamp(distance_ - wheel_steps * 0.35f, kMinDistance, kMaxDistance);
}

void OrbitCamera::Reset() {
    orientation_       = kInitOrientation;
    pitch_accumulated_ = kInitPitch;
    target_            = kInitTarget;
    distance_          = kInitDistance;
    dragging_          = false;
    panning_           = false;
}

}  // namespace future_gaze
