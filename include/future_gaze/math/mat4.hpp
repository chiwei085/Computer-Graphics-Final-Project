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
    static Mat4 Translation(const Vec3& value);
    static Mat4 Scale(const Vec3& value);
    static Mat4 RotationX(float radians);
    static Mat4 RotationY(float radians);
    static Mat4 RotationZ(float radians);

    [[nodiscard]] const float* Data() const { return values_.data(); }
    [[nodiscard]] const std::array<float, 16>& Values() const noexcept {
        return values_;
    }
    [[nodiscard]] Vec3 TransformPoint(const Vec3& value) const noexcept;
    [[nodiscard]] Vec3 TransformVector(const Vec3& value) const noexcept;
    [[nodiscard]] Mat4 operator*(const Mat4& rhs) const noexcept;

private:
    std::array<float, 16> values_{};
};

}  // namespace future_gaze
