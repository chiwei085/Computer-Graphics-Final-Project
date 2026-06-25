#include "future_gaze/math/quat.hpp"

#include <cmath>

namespace future_gaze
{

Quat Quat::FromAxisAngle(const Vec3& axis, float radians) {
    const Vec3 unit_axis = Normalize(axis);
    const float half_angle = radians * 0.5f;
    const float s = std::sin(half_angle);
    return Quat{std::cos(half_angle), unit_axis.x * s, unit_axis.y * s,
                unit_axis.z * s};
}

Mat4 Quat::ToMat4() const {
    const float xx = x * x;
    const float yy = y * y;
    const float zz = z * z;
    const float xy = x * y;
    const float xz = x * z;
    const float yz = y * z;
    const float wx = w * x;
    const float wy = w * y;
    const float wz = w * z;

    return Mat4{{1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy),
                 0.0f, 2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz),
                 2.0f * (yz + wx), 0.0f, 2.0f * (xz + wy), 2.0f * (yz - wx),
                 1.0f - 2.0f * (xx + yy), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}};
}

}  // namespace future_gaze
