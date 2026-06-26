#include "future_gaze/scene/gaze_controller.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "future_gaze/math/quat.hpp"
#include "future_gaze/scene/scene_graph.hpp"

namespace future_gaze
{

namespace
{
// Bumped so a single ~400px drag crosses one full 120-degree zone (at 0.12 it
// took ~1000px, making the zone switch feel unreachably heavy).
constexpr float kDragYawSensitivity = 0.30f;
constexpr float kDragPitchSensitivity = 0.095f;
constexpr float kMinPitchOffset = -46.0f;
constexpr float kMaxPitchOffset = 38.0f;

constexpr float kZoneSpanDeg = 120.0f;
// Exponential-smoothing time constant for the zone crossfade. tau ~= 0.26s
// reaches ~90% in ~0.6s, matching the requested transition feel.
constexpr float kZoneBlendTau = 0.26f;
constexpr float kZoneWeightSnapEpsilon = 0.0005f;
constexpr float kZoneWeightDirtyEpsilon = 0.00001f;

float NormalizeDeg360(float deg) {
    deg = std::fmod(deg, 360.0f);
    return (deg < 0.0f) ? deg + 360.0f : deg;
}
}  // namespace

void GazeController::Bind(SceneGraph& graph) {
    tf_ = &graph.tf();
    eye_root_ = graph.Find(names::kPredictionCore);
    if (!eye_root_.IsValid()) {
        std::cerr << "[GazeController] node '" << names::kPredictionCore
                  << "' not found; gaze disabled.\n";
        return;
    }
    const Transform local = tf_->Local(eye_root_);
    base_euler_ = local.euler_deg;
    origin_ = tf_->WorldPosition(eye_root_);
    direction_ = Normalize(tf_->WorldVector(eye_root_, {0.0f, 0.0f, -1.0f}));
}

void GazeController::Update(float elapsed_seconds, float delta_seconds) {
    if (tf_ == nullptr || !tf_->Contains(eye_root_)) {
        return;
    }

    const float idle_yaw =
        control_mode_ ? 0.0f : 3.5f * std::sin(elapsed_seconds * 0.23f);
    const float idle_pitch =
        control_mode_ ? 0.0f : 1.8f * std::sin(elapsed_seconds * 0.17f + 1.1f);
    const float yaw = base_euler_.y + yaw_offset_deg_ + idle_yaw;
    const float pitch = base_euler_.x + pitch_offset_deg_ + idle_pitch;

    Transform local = tf_->Local(eye_root_);
    local.euler_deg.y = yaw;
    local.euler_deg.x = pitch;
    tf_->SetLocal(eye_root_, local);
    origin_ = tf_->WorldPosition(eye_root_);
    direction_ = Normalize(tf_->WorldVector(eye_root_, {0.0f, 0.0f, -1.0f}));

    // Zone selection is measured from the user's yaw offset (not absolute yaw),
    // and excludes the idle sway, so the resting eye sits in the centre of the
    // Foresight zone and never flickers at a boundary while at rest.
    UpdateZone(yaw_offset_deg_, delta_seconds);
}

void GazeController::UpdateZone(float zone_yaw_deg, float delta_seconds) {
    if (zone_override_active_) {
        return;
    }
    // +half-span centres zone 0 (Foresight) on the resting eye, so a boundary
    // is a full 60 degrees of rotation away in either direction.
    const int sector =
        static_cast<int>(NormalizeDeg360(zone_yaw_deg + kZoneSpanDeg * 0.5f) /
                         kZoneSpanDeg) %
        kGazeZoneCount;
    active_zone_ = static_cast<GazeZone>(sector);

    // Exponential approach toward (1 for active, 0 otherwise). Frame-rate
    // independent; clamped dt guards against a huge first-frame step.
    const float dt = std::clamp(delta_seconds, 0.0f, 0.1f);
    const float alpha = 1.0f - std::exp(-dt / kZoneBlendTau);
    bool weights_changed = false;
    for (int i = 0; i < kGazeZoneCount; ++i) {
        const float target = (i == sector) ? 1.0f : 0.0f;
        float next = zone_weight_[i] + (target - zone_weight_[i]) * alpha;
        if (std::abs(next - target) < kZoneWeightSnapEpsilon) {
            next = target;
        }
        if (std::abs(next - zone_weight_[i]) > kZoneWeightDirtyEpsilon) {
            zone_weight_[i] = next;
            weights_changed = true;
        }
    }
    if (weights_changed) {
        MarkZoneWeightsDirty();
    }
}

void GazeController::ToggleControlMode() {
    SetControlMode(!control_mode_);
}

void GazeController::SetControlMode(bool enabled) {
    control_mode_ = enabled;
    dragging_ = false;
}

void GazeController::ToggleDebug() {
    debug_enabled_ = !debug_enabled_;
}

void GazeController::ResetAim() {
    yaw_offset_deg_ = 0.0f;
    pitch_offset_deg_ = 0.0f;
    dragging_ = false;
}

void GazeController::SetZoneOverride(float foresight, float longing,
                                     float blindspot) {
    zone_override_active_ = true;
    const float next[kGazeZoneCount] = {
        std::clamp(foresight, 0.0f, 1.0f),
        std::clamp(longing, 0.0f, 1.0f),
        std::clamp(blindspot, 0.0f, 1.0f),
    };
    bool weights_changed = false;
    for (int i = 0; i < kGazeZoneCount; ++i) {
        if (std::abs(next[i] - zone_weight_[i]) > kZoneWeightDirtyEpsilon) {
            zone_weight_[i] = next[i];
            weights_changed = true;
        }
    }
    if (weights_changed) {
        MarkZoneWeightsDirty();
    }
}

void GazeController::ClearZoneOverride() {
    zone_override_active_ = false;
}

void GazeController::BeginDrag(int x, int y) {
    dragging_ = true;
    last_x_ = x;
    last_y_ = y;
}

void GazeController::DragTo(int x, int y) {
    if (!dragging_) {
        return;
    }

    const float dx = static_cast<float>(x - last_x_);
    const float dy = static_cast<float>(y - last_y_);
    last_x_ = x;
    last_y_ = y;

    yaw_offset_deg_ += dx * kDragYawSensitivity;
    pitch_offset_deg_ =
        std::clamp(pitch_offset_deg_ - dy * kDragPitchSensitivity,
                   kMinPitchOffset, kMaxPitchOffset);
}

void GazeController::EndDrag() {
    dragging_ = false;
}

void GazeController::MarkZoneWeightsDirty() {
    ++zone_weights_revision_;
}

float GazeController::FrontWeight(Vec3 world_pos, float inner_deg,
                                  float outer_deg) const {
    const Vec3 to_target = Normalize(world_pos - origin_);
    return ConeWeight(Dot(direction_, to_target), inner_deg, outer_deg);
}

float GazeController::BlindWeight(Vec3 world_pos, float inner_deg,
                                  float outer_deg) const {
    const Vec3 to_target = Normalize(world_pos - origin_);
    return ConeWeight(Dot(BlindDirection(), to_target), inner_deg, outer_deg);
}

Vec3 GazeController::ComputeDirection(float yaw_deg, float pitch_deg) const {
    const Quat yaw = Quat::FromAxisAngle({0.0f, 1.0f, 0.0f}, DegToRad(yaw_deg));
    const Quat pitch =
        Quat::FromAxisAngle({1.0f, 0.0f, 0.0f}, DegToRad(pitch_deg));
    return Normalize((yaw * pitch).Rotate({0.0f, 0.0f, -1.0f}));
}

float GazeController::ConeWeight(float dot, float inner_deg, float outer_deg) {
    const float inner = std::cos(DegToRad(inner_deg));
    const float outer = std::cos(DegToRad(outer_deg));
    if (dot >= inner) {
        return 1.0f;
    }
    if (dot <= outer) {
        return 0.0f;
    }
    const float t = (dot - outer) / (inner - outer);
    return t * t * (3.0f - 2.0f * t);
}

}  // namespace future_gaze
