#pragma once

#include <cstddef>

#include "future_gaze/math/vec3.hpp"
#include "future_gaze/scene/tf_tree.hpp"

namespace future_gaze
{

class SceneGraph;

// Gaze zone selected by yaw split into 120-degree sectors.
enum class GazeZone
{
    kForesight = 0,  // 預見 — futures bloom (cool/cyan)
    kLonging = 1,    // 眷戀 — memories unfold (warm/gold)
    kBlindspot = 2,  // 盲點 — deformation stops
};
inline constexpr int kGazeZoneCount = 3;

class GazeController
{
public:
    void Bind(SceneGraph& graph);
    void Update(float elapsed_seconds, float delta_seconds);

    void ToggleControlMode();
    void SetControlMode(bool enabled);
    void ToggleDebug();
    void ResetAim();
    void SetZoneOverride(float foresight, float longing, float blindspot);
    void ClearZoneOverride();

    [[nodiscard]] bool control_mode() const noexcept { return control_mode_; }
    [[nodiscard]] bool debug_enabled() const noexcept { return debug_enabled_; }

    void BeginDrag(int x, int y);
    void DragTo(int x, int y);
    void EndDrag();

    [[nodiscard]] Vec3 Origin() const noexcept { return origin_; }
    [[nodiscard]] Vec3 Direction() const noexcept { return direction_; }
    // User yaw offset in degrees, excludes base facing and idle sway.
    [[nodiscard]] float YawOffsetDeg() const noexcept {
        return yaw_offset_deg_;
    }
    [[nodiscard]] float PitchOffsetDeg() const noexcept {
        return pitch_offset_deg_;
    }
    [[nodiscard]] Vec3 BlindDirection() const noexcept {
        return direction_ * -1.0f;
    }

    [[nodiscard]] GazeZone ActiveZone() const noexcept { return active_zone_; }
    // Eased weight [0,1] for the given zone; active zone approaches 1 in ~0.6s.
    [[nodiscard]] float ZoneWeight(GazeZone zone) const noexcept {
        return zone_weight_[static_cast<std::size_t>(zone)];
    }
    [[nodiscard]] float ZoneWeight(int zone) const noexcept {
        return zone_weight_[static_cast<std::size_t>(zone)];
    }
    [[nodiscard]] std::size_t ZoneWeightsRevision() const noexcept {
        return zone_weights_revision_;
    }

    [[nodiscard]] float FrontWeight(Vec3 world_pos, float inner_deg = 11.0f,
                                    float outer_deg = 34.0f) const;
    [[nodiscard]] float BlindWeight(Vec3 world_pos, float inner_deg = 24.0f,
                                    float outer_deg = 82.0f) const;

private:
    [[nodiscard]] Vec3 ComputeDirection(float yaw_deg, float pitch_deg) const;
    [[nodiscard]] static float ConeWeight(float dot, float inner_deg,
                                          float outer_deg);
    void UpdateZone(float yaw_deg, float delta_seconds);
    void MarkZoneWeightsDirty();

    TfTree* tf_ = nullptr;
    TfHandle eye_root_{};
    Vec3 base_euler_{};
    Vec3 origin_{};
    Vec3 direction_{0.0f, -0.25f, 1.0f};

    float yaw_offset_deg_ = 0.0f;
    float pitch_offset_deg_ = 0.0f;
    bool zone_override_active_ = false;

    // Initialised to Foresight so the resting eye starts fully in zone 0.
    GazeZone active_zone_ = GazeZone::kForesight;
    float zone_weight_[kGazeZoneCount] = {1.0f, 0.0f, 0.0f};
    std::size_t zone_weights_revision_ = 0;

    bool control_mode_ = false;
    bool debug_enabled_ = false;
    bool dragging_ = false;
    int last_x_ = 0;
    int last_y_ = 0;
};

}  // namespace future_gaze
