#pragma once

#include <map>
#include <memory>
#include <string>

#include "future_gaze/camera/orbit_camera.hpp"
#include "future_gaze/render/material.hpp"
#include "future_gaze/render/mesh.hpp"
#include "future_gaze/render/texture.hpp"
#include "future_gaze/scene/scene_animation.hpp"
#include "future_gaze/scene/scene_node.hpp"

namespace future_gaze
{

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

private:
    void ApplyLighting() const;
    void ApplyProjection() const;
    void DrawGroundReceiver() const;
    void DrawPlanarShadowOnGround();
    void DrawPlanarShadowOnTable();
    void DrawReceiverBox(Vec3 center, Vec3 scale) const;
    void DrawShadowPass(const std::array<float, 16>& shadow_matrix,
                        Vec3 receiver_center, Vec3 receiver_scale,
                        SceneNode& caster_root, float min_caster_y,
                        float max_caster_y) const;
    static std::array<float, 16> MakeShadowMatrix(
        const std::array<float, 4>& light_pos,
        const std::array<float, 4>& plane);
    const Texture* GetTexture(const std::string& name) const;

    int width_ = 1280;
    int height_ = 720;
    float elapsed_seconds_ = 0.0f;
    OrbitCamera camera_;
    Mesh receiver_mesh_{Mesh::Cube()};
    Material ground_material_{{0.080f, 0.090f, 0.090f, 1.0f},
                              {0.34f, 0.38f, 0.36f, 1.0f},
                              {0.05f, 0.060f, 0.060f, 1.0f},
                              3.0f};
    std::map<std::string, Texture> textures_;
    std::unique_ptr<SceneNode> scene_root_;
    SceneAnimation animation_;
};

}  // namespace future_gaze
