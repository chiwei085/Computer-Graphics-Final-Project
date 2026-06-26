#pragma once

#include <algorithm>
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

    [[nodiscard]] constexpr Vec3 operator/(float scalar) const noexcept {
        return {x / scalar, y / scalar, z / scalar};
    }

    [[nodiscard]] constexpr Vec3 operator-() const noexcept {
        return {-x, -y, -z};
    }

    constexpr Vec3& operator+=(const Vec3& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    constexpr Vec3& operator-=(const Vec3& rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }
};

[[nodiscard]] constexpr Vec3 operator*(float scalar,
                                       const Vec3& value) noexcept {
    return value * scalar;
}

[[nodiscard]] constexpr float Dot(const Vec3& lhs, const Vec3& rhs) noexcept {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

[[nodiscard]] constexpr Vec3 Cross(const Vec3& lhs, const Vec3& rhs) noexcept {
    return {lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z,
            lhs.x * rhs.y - lhs.y * rhs.x};
}

[[nodiscard]] constexpr float LengthSquared(const Vec3& value) noexcept {
    return Dot(value, value);
}

[[nodiscard]] inline float Length(const Vec3& value) noexcept {
    return std::sqrt(LengthSquared(value));
}

[[nodiscard]] inline float DistanceSquared(const Vec3& lhs,
                                           const Vec3& rhs) noexcept {
    return LengthSquared(lhs - rhs);
}

[[nodiscard]] inline float Distance(const Vec3& lhs, const Vec3& rhs) noexcept {
    return std::sqrt(DistanceSquared(lhs, rhs));
}

[[nodiscard]] inline Vec3 Normalize(const Vec3& value) noexcept {
    const float length = Length(value);
    if (length <= 0.00001f) {
        return {};
    }
    return value * (1.0f / length);
}

[[nodiscard]] constexpr Vec3 Min(const Vec3& lhs, const Vec3& rhs) noexcept {
    return {lhs.x < rhs.x ? lhs.x : rhs.x, lhs.y < rhs.y ? lhs.y : rhs.y,
            lhs.z < rhs.z ? lhs.z : rhs.z};
}

[[nodiscard]] constexpr Vec3 Max(const Vec3& lhs, const Vec3& rhs) noexcept {
    return {lhs.x > rhs.x ? lhs.x : rhs.x, lhs.y > rhs.y ? lhs.y : rhs.y,
            lhs.z > rhs.z ? lhs.z : rhs.z};
}

[[nodiscard]] inline Vec3 Abs(const Vec3& value) noexcept {
    return {std::abs(value.x), std::abs(value.y), std::abs(value.z)};
}

[[nodiscard]] constexpr Vec3 Clamp(const Vec3& value, const Vec3& min_value,
                                   const Vec3& max_value) noexcept {
    return {std::clamp(value.x, min_value.x, max_value.x),
            std::clamp(value.y, min_value.y, max_value.y),
            std::clamp(value.z, min_value.z, max_value.z)};
}

[[nodiscard]] constexpr float Lerp(float a, float b, float t) noexcept {
    return a + (b - a) * t;
}

[[nodiscard]] constexpr Vec3 Lerp(const Vec3& a, const Vec3& b,
                                  float t) noexcept {
    return a + (b - a) * t;
}

[[nodiscard]] inline Vec3 ProjectOnPlane(const Vec3& value,
                                         const Vec3& normal) noexcept {
    const Vec3 n = Normalize(normal);
    return value - n * Dot(value, n);
}

[[nodiscard]] inline bool NearlyEqual(float lhs, float rhs,
                                      float tolerance = 0.0001f) noexcept {
    return std::abs(lhs - rhs) <= tolerance;
}

[[nodiscard]] inline bool NearlyEqual(const Vec3& lhs, const Vec3& rhs,
                                      float tolerance = 0.0001f) noexcept {
    return NearlyEqual(lhs.x, rhs.x, tolerance) &&
           NearlyEqual(lhs.y, rhs.y, tolerance) &&
           NearlyEqual(lhs.z, rhs.z, tolerance);
}

[[nodiscard]] constexpr float DegToRad(float degrees) noexcept {
    return degrees * std::numbers::pi_v<float> / 180.0f;
}

}  // namespace future_gaze
