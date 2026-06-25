#pragma once

#include <cmath>
#include <numbers>

namespace future_gaze
{

struct Vec3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    [[nodiscard]] constexpr Vec3 operator+(const Vec3& rhs) const noexcept {
        return {x + rhs.x, y + rhs.y, z + rhs.z};
    }

    [[nodiscard]] constexpr Vec3 operator-(const Vec3& rhs) const noexcept {
        return {x - rhs.x, y - rhs.y, z - rhs.z};
    }

    [[nodiscard]] constexpr Vec3 operator*(float scalar) const noexcept {
        return {x * scalar, y * scalar, z * scalar};
    }
};

[[nodiscard]] constexpr float Dot(const Vec3& lhs, const Vec3& rhs) noexcept {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

[[nodiscard]] constexpr Vec3 Cross(const Vec3& lhs, const Vec3& rhs) noexcept {
    return {lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z,
            lhs.x * rhs.y - lhs.y * rhs.x};
}

[[nodiscard]] inline float Length(const Vec3& value) noexcept {
    return std::sqrt(Dot(value, value));
}

[[nodiscard]] inline Vec3 Normalize(const Vec3& value) noexcept {
    const float length = Length(value);
    if (length <= 0.00001f) {
        return {};
    }
    return value * (1.0f / length);
}

[[nodiscard]] constexpr float DegToRad(float degrees) noexcept {
    return degrees * std::numbers::pi_v<float> / 180.0f;
}

}  // namespace future_gaze
