#include "future_gaze/math/mat4.hpp"

#include <cmath>

namespace future_gaze
{

Mat4 Mat4::LookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
    const Vec3 forward = Normalize(target - eye);
    const Vec3 side = Normalize(Cross(forward, up));
    const Vec3 camera_up = Cross(side, forward);

    return Mat4{{side.x, camera_up.x, -forward.x, 0.0f, side.y, camera_up.y,
                 -forward.y, 0.0f, side.z, camera_up.z, -forward.z, 0.0f,
                 -Dot(side, eye), -Dot(camera_up, eye), Dot(forward, eye),
                 1.0f}};
}

Mat4 Mat4::Perspective(float fov_y_radians, float aspect, float near_plane,
                       float far_plane) {
    const float tan_half_fov = std::tan(fov_y_radians * 0.5f);
    return Mat4{{1.0f / (aspect * tan_half_fov), 0.0f, 0.0f, 0.0f, 0.0f,
                 1.0f / tan_half_fov, 0.0f, 0.0f, 0.0f, 0.0f,
                 -(far_plane + near_plane) / (far_plane - near_plane), -1.0f,
                 0.0f, 0.0f,
                 -(2.0f * far_plane * near_plane) / (far_plane - near_plane),
                 0.0f}};
}

Mat4 Mat4::RotationY(float radians) {
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return Mat4{{c, 0.0f, -s, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, s, 0.0f, c, 0.0f,
                 0.0f, 0.0f, 0.0f, 1.0f}};
}

}  // namespace future_gaze
