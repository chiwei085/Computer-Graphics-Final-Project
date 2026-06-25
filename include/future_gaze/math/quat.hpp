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
    [[nodiscard]] Mat4 ToMat4() const;
};

}  // namespace future_gaze
