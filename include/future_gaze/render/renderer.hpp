#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

#include "future_gaze/camera/orbit_camera.hpp"
#include "future_gaze/render/material.hpp"
#include "future_gaze/render/mesh.hpp"
#include "future_gaze/render/texture.hpp"
#include "future_gaze/scene/gaze_controller.hpp"
#include "future_gaze/scene/scene_animation.hpp"
#include "future_gaze/scene/scene_graph.hpp"

namespace future_gaze
{

namespace render_detail
{

struct ZonePalette
{
    std::array<float, 4> key_diffuse{};
    std::array<float, 4> fill_diffuse{};
    std::array<float, 4> rim_diffuse{};
    std::array<float, 4> ambient{};
    std::array<float, 3> bg{};
    float fog_start = 0.0f;
    float fog_end = 0.0f;
    float shadow_alpha = 0.0f;
};

}  // namespace render_detail

class Renderer
{
public:
    void Initialize();
    void Resize(int width, int height);
    void Update(float delta_seconds);
    void Render();

    void BeginCameraDrag(int x, int y);
    void DragCameraTo(int x, int y);
    void EndCameraDrag();

    void BeginCameraPan(int x, int y);
    void PanCameraTo(int x, int y);
    void EndCameraPan();

    void ZoomCamera(float wheel_steps);
    void ResetCamera();

    void ToggleGazeControlMode();
    void SetGazeControlMode(bool enabled);
    void ToggleGazeDebug();
    void ResetGazeAim();
    [[nodiscard]] bool GazeControlMode() const;
    void BeginGazeDrag(int x, int y);
    void DragGazeTo(int x, int y);
    void EndGazeDrag();
    void LogObservationState(const char* label = "") const;

private:
    void UpdateTableTransition(float delta_seconds);
    void StartTableRestage(float target_yaw_deg);
    void SnapTableStage(float yaw_deg);
    [[nodiscard]] OrbitCamera::CameraPose CameraFollowPoseForRestage() const;
    void UpdateCameraRestage(float delta_seconds);
    void RefreshZonePaletteCache();
    void ApplyLighting() const;
    void ApplyProjection() const;
    void DrawGroundReceiver() const;
    void DrawPlanarShadowOnGround(float shadow_alpha);
    void DrawPlanarShadowOnTable(float shadow_alpha);
    void DrawGazeDebug() const;
    void DrawReceiverBox(Vec3 center, Vec3 scale) const;
    void DrawShadowPass(const std::array<float, 16>& shadow_matrix,
                        Vec3 receiver_center, Vec3 receiver_scale,
                        TfHandle caster_root, float min_caster_y,
                        float max_caster_y, float shadow_alpha) const;
    static std::array<float, 16> MakeShadowMatrix(
        const std::array<float, 4>& light_pos,
        const std::array<float, 4>& plane);
    const Texture* GetTexture(const std::string& name) const;

    int width_ = 1280;
    int height_ = 720;
    float elapsed_seconds_ = 0.0f;
    OrbitCamera camera_;

    // G-key re-stage: swings the dinner group to park in front of the new gaze.
    TfHandle table_root_{};
    TfHandle props_root_{};
    bool table_transition_active_ = false;
    float table_transition_t_ = 0.0f;
    Vec3 table_from_pos_{};
    Vec3 table_to_pos_{};
    float table_from_yaw_ = 0.0f;
    float table_to_yaw_ = 0.0f;

    bool camera_restage_pending_ = false;
    bool camera_home_pending_ = false;
    float camera_restage_delay_ = 0.0f;
    Mesh receiver_mesh_{Mesh::Cube()};
    Material ground_material_{{0.030f, 0.036f, 0.040f, 1.0f},
                              {0.150f, 0.170f, 0.165f, 1.0f},
                              {0.040f, 0.050f, 0.052f, 1.0f},
                              6.0f};
    std::unordered_map<std::string, Texture> textures_;
    std::unique_ptr<SceneGraph> scene_root_;
    GazeController gaze_;
    SceneAnimation animation_;
    render_detail::ZonePalette zone_palette_cache_{};
    std::size_t zone_palette_cache_revision_ = static_cast<std::size_t>(-1);
};

}  // namespace future_gaze
