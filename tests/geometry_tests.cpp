#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

#include "future_gaze/math/geometry.hpp"
#include "future_gaze/math/mat4.hpp"

namespace
{

void FailGeometry(std::string_view message) {
    std::cerr << "geometry_tests: " << message << '\n';
    std::exit(1);
}

void RequireGeometry(bool value, std::string_view message) {
    if (!value) {
        FailGeometry(message);
    }
}

void RequireNearGeometry(float actual, float expected,
                         std::string_view message) {
    if (std::abs(actual - expected) > 0.0001f) {
        std::cerr << "geometry_tests: " << message << " (actual " << actual
                  << " expected " << expected << ")\n";
        std::exit(1);
    }
}

void RequireNearGeometry(future_gaze::Vec3 actual, future_gaze::Vec3 expected,
                         std::string_view message) {
    RequireNearGeometry(actual.x, expected.x, message);
    RequireNearGeometry(actual.y, expected.y, message);
    RequireNearGeometry(actual.z, expected.z, message);
}

void TestAabbInsetContainsClosestPoint() {
    const future_gaze::Aabb3 box{{-2.0f, 0.0f, -3.0f}, {2.0f, 4.0f, 3.0f}};
    const future_gaze::Aabb3 inset = box.Inset({0.5f, 1.0f, 0.25f});

    RequireGeometry(inset.Contains({0.0f, 2.0f, 0.0f}),
                    "inset contains interior point");
    RequireGeometry(!inset.Contains({-1.75f, 2.0f, 0.0f}),
                    "inset rejects outside point");
    RequireNearGeometry(inset.ClosestPoint({5.0f, -1.0f, 0.0f}),
                        {1.5f, 1.0f, 0.0f}, "closest point clamps axes");
}

void TestPlaneDistance() {
    const future_gaze::Plane plane = future_gaze::Plane::FromPointNormal(
        {0.0f, 2.0f, 0.0f}, {0.0f, 2.0f, 0.0f});
    RequireNearGeometry(future_gaze::DistanceToPlane(plane, {0.0f, 5.0f, 0.0f}),
                        3.0f, "plane distance above");
    RequireNearGeometry(future_gaze::DistanceToPlane(plane, {0.0f, 1.0f, 0.0f}),
                        -1.0f, "plane distance below");
}

void TestNearPlaneCorners() {
    const auto corners = future_gaze::ComputeNearPlaneCorners(
        {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f},
        future_gaze::DegToRad(90.0f), 1.0f, 1.0f);
    RequireNearGeometry(corners[0], {-1.0f, 1.0f, -1.0f},
                        "near top-left corner");
    RequireNearGeometry(corners[2], {1.0f, -1.0f, -1.0f},
                        "near bottom-right corner");
}

void TestRigidInverseRoundTrip() {
    const future_gaze::Mat4 m =
        future_gaze::Mat4::Translation({1.0f, 2.0f, 3.0f}) *
        future_gaze::Mat4::RotationY(future_gaze::DegToRad(90.0f));
    const future_gaze::Mat4 inv = m.InverseRigid();
    const future_gaze::Vec3 p{2.0f, 2.0f, 3.0f};
    RequireNearGeometry(inv.TransformPoint(m.TransformPoint(p)), p,
                        "rigid inverse roundtrip");
}

void TestRayAabbAndFitDistance() {
    const future_gaze::Aabb3 box{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
    const future_gaze::RayHit hit = future_gaze::IntersectRayAabb(
        {{0.0f, 0.0f, 4.0f}, {0.0f, 0.0f, -1.0f}}, box);
    RequireGeometry(hit.hit, "ray hits aabb");
    RequireNearGeometry(hit.t_min, 3.0f, "ray aabb entry distance");

    const float distance = future_gaze::FitDistanceForSphere(
        1.0f, future_gaze::DegToRad(60.0f), 1.0f, 1.0f);
    RequireNearGeometry(distance, 2.0f, "fit sphere in 60-degree square fov");
}

}  // namespace

void RunGeometryTests() {
    TestAabbInsetContainsClosestPoint();
    TestPlaneDistance();
    TestNearPlaneCorners();
    TestRigidInverseRoundTrip();
    TestRayAabbAndFitDistance();
}
