#include "future_gaze/camera/orbit_camera.hpp"

#include <algorithm>
#include <cmath>

#include "future_gaze/anim/easing.hpp"

namespace future_gaze
{

namespace
{

constexpr float kOrbitSensitivity = 0.008f;
constexpr float kPanSensitivity = 0.0012f;
constexpr float kMaxPitch = 1.25f;  // ≈ 72°
constexpr float kMinDistance = 2.0f;
constexpr float kMaxDistance = 18.0f;

// Initial camera state (duplicated in Reset())
const Quat kInitOrientation = Quat::FromAxisAngle({1.0f, 0.0f, 0.0f}, -0.25f);
const Vec3 kInitTarget{0.0f, 1.5f, -0.5f};
constexpr float kInitPitch = 0.25f;
constexpr float kInitDistance = 9.0f;

// Where the gaze-follow camera parks relative to the eye: pulled back behind
// the eye (opposite the gaze) and raised, so it frames the eye in the
// foreground with the re-staged table beyond, instead of jamming into the
// tabletop (the eye sits almost directly over the table).
constexpr float kGazeBackDist = 4.8f;
constexpr float kGazeRise = 1.3f;

}  // namespace

// ── View ─────────────────────────────────────────────────────────────────────

Mat4 OrbitCamera::ViewMatrix() const {
    return Mat4::LookAt(Eye(), target_,
                        orientation_.Rotate({0.0f, 1.0f, 0.0f}));
}

// ── Scripted transition
// ───────────────────────────────────────────────────────

void OrbitCamera::Update(float delta_seconds) {
    if (!transitioning_) {
        return;
    }
    transition_t_ += delta_seconds;
    const float u = std::clamp(transition_t_ / transition_dur_, 0.0f, 1.0f);
    const float e = ease::SmootherStep(u);

    orientation_ =
        Quat::Slerp(transition_from_.orientation, transition_to_.orientation, e)
            .Normalized();
    target_ = transition_from_.target +
              (transition_to_.target - transition_from_.target) * e;
    distance_ =
        ease::Lerp(transition_from_.distance, transition_to_.distance, e);
    pitch_accumulated_ =
        ease::Lerp(transition_from_.pitch, transition_to_.pitch, e);

    if (u >= 1.0f) {
        transitioning_ = false;
    }
}

OrbitCamera::CameraPose OrbitCamera::CurrentPose() const noexcept {
    return CameraPose{orientation_, target_, distance_, pitch_accumulated_};
}

void OrbitCamera::StartTransitionTo(const CameraPose& goal, float duration) {
    transition_from_ = CurrentPose();
    transition_to_ = goal;
    transition_t_ = 0.0f;
    transition_dur_ = std::max(0.0001f, duration);
    transitioning_ = true;
    dragging_ = false;
    panning_ = false;
}

OrbitCamera::CameraPose OrbitCamera::GazePose(Vec3 eye_origin, Vec3 gaze_dir,
                                              Vec3 look_at) {
    const Vec3 g = Normalize(gaze_dir);
    // The eye sits almost on top of the table, so a literal POV (stepping the
    // lens forward past the eye) jams it into the tabletop. Instead pull back
    // BEHIND and ABOVE the eye and look along the gaze: the eye reads in the
    // foreground with the re-staged table beyond it, at a comfortable distance.
    const Vec3 cam_eye =
        eye_origin - g * kGazeBackDist + Vec3{0.0f, kGazeRise, 0.0f};
    const Vec3 forward = Normalize(look_at - cam_eye);

    CameraPose pose;
    pose.target = look_at;
    pose.distance = Length(look_at - cam_eye);
    // Decompose `forward` into the same world-Y-yaw / local-X-pitch convention
    // the drag controls build, so dragging after the sweep stays continuous.
    pose.pitch = std::asin(std::clamp(-forward.y, -1.0f, 1.0f));
    const float yaw = std::atan2(-forward.x, -forward.z);
    pose.orientation = (Quat::FromAxisAngle({0.0f, 1.0f, 0.0f}, yaw) *
                        Quat::FromAxisAngle({1.0f, 0.0f, 0.0f}, -pose.pitch))
                           .Normalized();
    return pose;
}

Vec3 OrbitCamera::Eye() const {
    return target_ + orientation_.Rotate(Vec3{0.0f, 0.0f, distance_});
}

// ── Orbit
// ─────────────────────────────────────────────────────────────────────

void OrbitCamera::BeginDrag(int x, int y) {
    if (transitioning_) {
        return;
    }
    dragging_ = true;
    last_x_ = x;
    last_y_ = y;
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
    const Quat yaw =
        Quat::FromAxisAngle({0.0f, 1.0f, 0.0f}, -dx * kOrbitSensitivity);
    orientation_ = (yaw * orientation_).Normalized();

    // Pitch: camera-local X axis (right-multiply stays local).
    // Negate dy so drag-down = bird's eye (camera rises).
    // Clamp via separate accumulator to prevent flip past ±90°.
    const float raw_delta = dy * kOrbitSensitivity;
    const float new_pitch =
        std::clamp(pitch_accumulated_ + raw_delta, -kMaxPitch, kMaxPitch);
    const float clamped = new_pitch - pitch_accumulated_;
    pitch_accumulated_ = new_pitch;

    const Quat pitch = Quat::FromAxisAngle({1.0f, 0.0f, 0.0f}, -clamped);
    orientation_ = (orientation_ * pitch).Normalized();
}

void OrbitCamera::EndDrag() {
    dragging_ = false;
}

// ── Pan
// ───────────────────────────────────────────────────────────────────────

void OrbitCamera::BeginPan(int x, int y) {
    if (transitioning_) {
        return;
    }
    panning_ = true;
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

    const Vec3 right = orientation_.Rotate({1.0f, 0.0f, 0.0f});
    const Vec3 up = orientation_.Rotate({0.0f, 1.0f, 0.0f});
    const float scale = distance_ * kPanSensitivity;

    // Drag right → target shifts in -right so scene appears to slide right.
    // Drag down (dy>0) → target shifts in +up so scene slides down.
    target_ = target_ - right * (dx * scale) + up * (dy * scale);
}

void OrbitCamera::EndPan() {
    panning_ = false;
}

// ── Zoom / Reset
// ──────────────────────────────────────────────────────────────

void OrbitCamera::Zoom(float wheel_steps) {
    if (transitioning_) {
        return;
    }
    distance_ =
        std::clamp(distance_ - wheel_steps * 0.35f, kMinDistance, kMaxDistance);
}

void OrbitCamera::Reset() {
    orientation_ = kInitOrientation;
    pitch_accumulated_ = kInitPitch;
    target_ = kInitTarget;
    distance_ = kInitDistance;
    dragging_ = false;
    panning_ = false;
    transitioning_ = false;
}

}  // namespace future_gaze
