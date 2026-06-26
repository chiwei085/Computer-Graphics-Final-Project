#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "future_gaze/math/quat.hpp"
#include "future_gaze/math/vec3.hpp"

namespace future_gaze
{

struct Aabb3
{
    Vec3 min{};
    Vec3 max{};

    [[nodiscard]] constexpr Vec3 Size() const noexcept { return max - min; }
    [[nodiscard]] constexpr Vec3 Center() const noexcept {
        return (min + max) * 0.5f;
    }
    [[nodiscard]] constexpr Aabb3 Inset(float amount) const noexcept {
        return Inset({amount, amount, amount});
    }
    [[nodiscard]] constexpr Aabb3 Inset(Vec3 amount) const noexcept {
        return {min + amount, max - amount};
    }
    [[nodiscard]] constexpr bool Contains(Vec3 point) const noexcept {
        return point.x >= min.x && point.x <= max.x && point.y >= min.y &&
               point.y <= max.y && point.z >= min.z && point.z <= max.z;
    }
    [[nodiscard]] constexpr Vec3 ClampPoint(Vec3 point) const noexcept {
        return Clamp(point, min, max);
    }
    [[nodiscard]] constexpr Vec3 ClosestPoint(Vec3 point) const noexcept {
        return ClampPoint(point);
    }
    [[nodiscard]] constexpr Aabb3 Expanded(float amount) const noexcept {
        return Expanded({amount, amount, amount});
    }
    [[nodiscard]] constexpr Aabb3 Expanded(Vec3 amount) const noexcept {
        return {min - amount, max + amount};
    }
    [[nodiscard]] constexpr Aabb3 Include(Vec3 point) const noexcept {
        return {Min(min, point), Max(max, point)};
    }
    [[nodiscard]] constexpr Aabb3 Union(const Aabb3& other) const noexcept {
        return {Min(min, other.min), Max(max, other.max)};
    }
};

struct Plane
{
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float d = 0.0f;

    [[nodiscard]] static Plane FromPointNormal(Vec3 point,
                                               Vec3 normal) noexcept {
        const Vec3 n = Normalize(normal);
        return {n, -Dot(n, point)};
    }
};

struct Ray
{
    Vec3 origin{};
    Vec3 dir{0.0f, 0.0f, -1.0f};
};

struct Sphere
{
    Vec3 center{};
    float radius = 1.0f;
};

struct RayHit
{
    bool hit = false;
    float t_min = 0.0f;
    float t_max = 0.0f;
};

struct CameraFraming
{
    Sphere subject{{0.0f, 1.75f, -0.70f}, 2.45f};
    float screen_margin = 1.18f;
    float min_subject_distance = 4.0f;
};

struct CameraBasis
{
    Vec3 right{1.0f, 0.0f, 0.0f};
    Vec3 up{0.0f, 1.0f, 0.0f};
    Vec3 forward{0.0f, 0.0f, -1.0f};
};

struct CameraVolume
{
    Sphere eye{};
    std::array<Vec3, 4> near_corners{};
};

[[nodiscard]] inline float DistanceToPlane(const Plane& plane,
                                           Vec3 point) noexcept {
    return Dot(plane.normal, point) + plane.d;
}

[[nodiscard]] inline float SignedDistanceToAabbInterior(const Aabb3& box,
                                                        Vec3 point) noexcept {
    const float dx = std::min(point.x - box.min.x, box.max.x - point.x);
    const float dy = std::min(point.y - box.min.y, box.max.y - point.y);
    const float dz = std::min(point.z - box.min.z, box.max.z - point.z);
    if (box.Contains(point)) {
        return std::min(dx, std::min(dy, dz));
    }
    return -Distance(point, box.ClosestPoint(point));
}

[[nodiscard]] inline CameraBasis BuildCameraBasis(
    const Quat& orientation) noexcept {
    return {Normalize(orientation.Rotate({1.0f, 0.0f, 0.0f})),
            Normalize(orientation.Rotate({0.0f, 1.0f, 0.0f})),
            Normalize(orientation.Rotate({0.0f, 0.0f, -1.0f}))};
}

[[nodiscard]] inline std::array<Vec3, 4> ComputeNearPlaneCorners(
    Vec3 eye, Vec3 forward, Vec3 up, float fov_y_radians, float aspect,
    float near_plane) noexcept {
    const Vec3 f = Normalize(forward);
    const Vec3 r = Normalize(Cross(f, up));
    const Vec3 u = Normalize(Cross(r, f));
    const float half_h = std::tan(fov_y_radians * 0.5f) * near_plane;
    const float half_w = half_h * aspect;
    const Vec3 center = eye + f * near_plane;
    return {center - r * half_w + u * half_h, center + r * half_w + u * half_h,
            center + r * half_w - u * half_h, center - r * half_w - u * half_h};
}

[[nodiscard]] inline CameraVolume BuildCameraVolume(
    Vec3 eye, const Quat& orientation, float eye_radius, float fov_y_radians,
    float aspect, float near_plane) noexcept {
    const CameraBasis basis = BuildCameraBasis(orientation);
    return {{eye, eye_radius},
            ComputeNearPlaneCorners(eye, basis.forward, basis.up, fov_y_radians,
                                    aspect, near_plane)};
}

[[nodiscard]] inline bool CameraVolumeInsideRoom(const CameraVolume& volume,
                                                 const Aabb3& room) noexcept {
    if (!room.Inset(volume.eye.radius).Contains(volume.eye.center)) {
        return false;
    }
    for (const Vec3& corner : volume.near_corners) {
        if (!room.Contains(corner)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] inline RayHit IntersectRayAabb(const Ray& ray,
                                             const Aabb3& box) noexcept {
    float t_min = 0.0f;
    float t_max = std::numeric_limits<float>::infinity();
    const auto axis = [&](float origin, float dir, float min_v, float max_v) {
        if (std::abs(dir) < 1.0e-6f) {
            return origin >= min_v && origin <= max_v;
        }
        const float inv = 1.0f / dir;
        float a = (min_v - origin) * inv;
        float b = (max_v - origin) * inv;
        if (a > b) {
            std::swap(a, b);
        }
        t_min = std::max(t_min, a);
        t_max = std::min(t_max, b);
        return t_min <= t_max;
    };
    const bool hit = axis(ray.origin.x, ray.dir.x, box.min.x, box.max.x) &&
                     axis(ray.origin.y, ray.dir.y, box.min.y, box.max.y) &&
                     axis(ray.origin.z, ray.dir.z, box.min.z, box.max.z);
    return {hit, t_min, t_max};
}

[[nodiscard]] inline float FitDistanceForSphere(float radius,
                                                float fov_y_radians,
                                                float aspect,
                                                float margin) noexcept {
    const float half_y = std::max(0.001f, fov_y_radians * 0.5f);
    const float half_x = std::atan(std::tan(half_y) * std::max(0.001f, aspect));
    const float limiting_half_fov = std::max(0.001f, std::min(half_x, half_y));
    return std::max(
        0.0f, radius * std::max(1.0f, margin) / std::sin(limiting_half_fov));
}

}  // namespace future_gaze
