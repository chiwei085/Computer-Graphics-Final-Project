#pragma once

#include "future_gaze/math/mat4.hpp"
#include "future_gaze/math/quat.hpp"
#include "future_gaze/math/vec3.hpp"

namespace future_gaze
{

class OrbitCamera
{
public:
    [[nodiscard]] Mat4 ViewMatrix() const;

    // Orbit: left mouse drag — yaw around world Y, pitch around camera local X
    void BeginDrag(int x, int y);
    void DragTo(int x, int y);
    void EndDrag();

    // Pan: middle / right mouse drag — translate target in camera XY plane
    void BeginPan(int x, int y);
    void PanTo(int x, int y);
    void EndPan();

    void Zoom(float wheel_steps);
    void Reset();

private:
    [[nodiscard]] Vec3 Eye() const;

    // Orbit
    bool  dragging_          = false;
    int   last_x_            = 0;
    int   last_y_            = 0;
    Quat  orientation_       = Quat::FromAxisAngle({1.0f, 0.0f, 0.0f}, -0.25f);
    float pitch_accumulated_ = 0.25f;  // tracks total pitch for clamping
    float distance_          = 9.0f;

    // Pan
    bool panning_    = false;
    int  pan_last_x_ = 0;
    int  pan_last_y_ = 0;

    Vec3 target_{0.0f, 1.5f, -0.5f};
};

}  // namespace future_gaze
