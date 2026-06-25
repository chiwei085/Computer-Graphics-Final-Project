#pragma once

#include <memory>

#include "future_gaze/camera/orbit_camera.hpp"
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
    void ZoomCamera(float wheel_steps);

private:
    void ApplyLighting() const;
    void ApplyProjection() const;

    int width_ = 1280;
    int height_ = 720;
    float elapsed_seconds_ = 0.0f;
    OrbitCamera camera_;
    std::unique_ptr<SceneNode> scene_root_;
};

}  // namespace future_gaze
