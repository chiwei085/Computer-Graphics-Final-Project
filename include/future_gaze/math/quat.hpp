#pragma once

#include "future_gaze/math/mat4.hpp"
#include "future_gaze/math/vec3.hpp"

namespace future_gaze
{

struct Quat
{
    float w = 1.0f;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    [[nodiscard]] static Quat FromAxisAngle(const Vec3& axis, float radians);
    [[nodiscard]] Mat4 ToMat4()               const noexcept;
    [[nodiscard]] Quat operator*(const Quat&) const noexcept;
    [[nodiscard]] Vec3 Rotate(const Vec3&)    const noexcept;
    [[nodiscard]] Quat Normalized()           const noexcept;
};

}  // namespace future_gaze
