#pragma once

#include <array>

#include "future_gaze/math/vec3.hpp"

namespace future_gaze
{

class Mat4
{
public:
    constexpr Mat4() = default;
    explicit constexpr Mat4(std::array<float, 16> values) noexcept
        : values_(values) {}

    static constexpr Mat4 Identity() noexcept {
        return Mat4{{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                     1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}};
    }
    static Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up);
    static Mat4 Perspective(float fov_y_radians, float aspect, float near_plane,
                            float far_plane);
    static Mat4 RotationY(float radians);

    const float* Data() const { return values_.data(); }

private:
    std::array<float, 16> values_{};
};

}  // namespace future_gaze
