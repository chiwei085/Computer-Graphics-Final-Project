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

Mat4 Mat4::Translation(const Vec3& value) {
    return Mat4{{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                 1.0f, 0.0f, value.x, value.y, value.z, 1.0f}};
}

Mat4 Mat4::Scale(const Vec3& value) {
    return Mat4{{value.x, 0.0f, 0.0f, 0.0f, 0.0f, value.y, 0.0f, 0.0f, 0.0f,
                 0.0f, value.z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}};
}

Mat4 Mat4::RotationX(float radians) {
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return Mat4{{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, c, s, 0.0f, 0.0f, -s, c, 0.0f,
                 0.0f, 0.0f, 0.0f, 1.0f}};
}

Mat4 Mat4::RotationY(float radians) {
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return Mat4{{c, 0.0f, -s, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, s, 0.0f, c, 0.0f,
                 0.0f, 0.0f, 0.0f, 1.0f}};
}

Mat4 Mat4::RotationZ(float radians) {
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return Mat4{{c, s, 0.0f, 0.0f, -s, c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                 0.0f, 0.0f, 0.0f, 1.0f}};
}

Vec3 Mat4::TransformPoint(const Vec3& value) const noexcept {
    return {values_[0] * value.x + values_[4] * value.y + values_[8] * value.z +
                values_[12],
            values_[1] * value.x + values_[5] * value.y + values_[9] * value.z +
                values_[13],
            values_[2] * value.x + values_[6] * value.y +
                values_[10] * value.z + values_[14]};
}

Vec3 Mat4::TransformVector(const Vec3& value) const noexcept {
    return {
        values_[0] * value.x + values_[4] * value.y + values_[8] * value.z,
        values_[1] * value.x + values_[5] * value.y + values_[9] * value.z,
        values_[2] * value.x + values_[6] * value.y + values_[10] * value.z};
}

Mat4 Mat4::operator*(const Mat4& rhs) const noexcept {
    std::array<float, 16> out{};
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += values_[static_cast<std::size_t>(k * 4 + row)] *
                       rhs.values_[static_cast<std::size_t>(col * 4 + k)];
            }
            out[static_cast<std::size_t>(col * 4 + row)] = sum;
        }
    }
    return Mat4{out};
}

}  // namespace future_gaze
