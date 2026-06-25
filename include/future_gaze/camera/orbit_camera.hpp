#pragma once

#include "future_gaze/math/mat4.hpp"
#include "future_gaze/math/vec3.hpp"

namespace future_gaze
{

class OrbitCamera
{
public:
    [[nodiscard]] Mat4 ViewMatrix() const;

    void BeginDrag(int x, int y);
    void DragTo(int x, int y);
    void EndDrag();
    void Zoom(float wheel_steps);

private:
    [[nodiscard]] constexpr Vec3 Target() const noexcept { return {}; }
    [[nodiscard]] Vec3 Eye() const;

    bool dragging_ = false;
    int last_x_ = 0;
    int last_y_ = 0;
    float yaw_radians_ = 0.75f;
    float pitch_radians_ = 0.42f;
    float distance_ = 5.0f;
};

}  // namespace future_gaze
