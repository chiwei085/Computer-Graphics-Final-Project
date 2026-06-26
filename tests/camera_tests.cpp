#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string_view>

#include "future_gaze/anim/easing.hpp"
#include "future_gaze/camera/orbit_camera.hpp"
#include "future_gaze/math/geometry.hpp"

namespace
{

void FailCamera(std::string_view message) {
    std::fprintf(stderr, "camera_tests: %.*s\n",
                 static_cast<int>(message.size()), message.data());
    std::exit(1);
}

void RequireCamera(bool value, std::string_view message) {
    if (!value) {
        FailCamera(message);
    }
}

void RequireNearCamera(float actual, float expected, std::string_view message) {
    if (std::abs(actual - expected) > 0.0001f) {
        std::fprintf(stderr, "camera_tests: %.*s (actual %.6f expected %.6f)\n",
                     static_cast<int>(message.size()), message.data(), actual,
                     expected);
        std::exit(1);
    }
}

void RequireNearCamera(future_gaze::Vec3 actual, future_gaze::Vec3 expected,
                       std::string_view message) {
    if (!future_gaze::NearlyEqual(actual, expected, 0.0001f)) {
        std::fprintf(stderr,
                     "camera_tests: %.*s "
                     "(actual %.6f %.6f %.6f expected %.6f %.6f %.6f)\n",
                     static_cast<int>(message.size()), message.data(), actual.x,
                     actual.y, actual.z, expected.x, expected.y, expected.z);
        std::exit(1);
    }
}

void TestResetVolumeInsideRoom() {
    future_gaze::OrbitCamera camera;
    camera.SetRoomBounds(future_gaze::DefaultRoomBounds());
    camera.SetProjection(future_gaze::DegToRad(55.0f), 16.0f / 9.0f, 0.1f);
    camera.Reset();

    RequireCamera(camera.CurrentVolumeInsideRoom(),
                  "reset camera volume is inside room");
    RequireCamera(camera.CurrentSubjectFramed(),
                  "reset camera frames the default subject volume");
}

void TestPanZoomOrbitStayInsideRoom() {
    future_gaze::OrbitCamera camera;
    camera.SetRoomBounds(future_gaze::DefaultRoomBounds());
    camera.SetProjection(future_gaze::DegToRad(55.0f), 16.0f / 9.0f, 0.1f);
    camera.Reset();

    camera.BeginPan(0, 0);
    camera.PanTo(100000, -100000);
    camera.EndPan();
    RequireCamera(camera.CurrentVolumeInsideRoom(),
                  "extreme pan remains inside room");

    for (int i = 0; i < 100; ++i) {
        camera.Zoom(-1.0f);
    }
    RequireCamera(camera.CurrentVolumeInsideRoom(),
                  "max zoom-out remains inside room");

    for (int i = 0; i < 100; ++i) {
        camera.Zoom(1.0f);
    }
    RequireCamera(camera.CurrentVolumeInsideRoom(),
                  "max zoom-in remains inside room");

    camera.BeginDrag(0, 0);
    camera.DragTo(120000, 120000);
    camera.EndDrag();
    RequireCamera(camera.CurrentVolumeInsideRoom(),
                  "extreme orbit remains inside room");
    const auto pose = camera.CurrentPose();
    RequireCamera(std::isfinite(pose.target.x) &&
                      std::isfinite(pose.target.y) &&
                      std::isfinite(pose.target.z) &&
                      std::isfinite(pose.distance) && pose.distance >= 2.0f,
                  "extreme camera pose remains finite");
}

void TestTransitionSamplesStayInsideRoom() {
    future_gaze::OrbitCamera camera;
    camera.SetRoomBounds(future_gaze::DefaultRoomBounds());
    camera.SetProjection(future_gaze::DegToRad(55.0f), 16.0f / 9.0f, 0.1f);
    camera.Reset();

    future_gaze::OrbitCamera::CameraPose goal = camera.CurrentPose();
    goal.target = {0.0f, 20.0f, 60.0f};
    goal.distance = 18.0f;
    camera.StartTransitionTo(goal, 1.0f);

    for (int i = 0; i < 32; ++i) {
        camera.Update(1.0f / 32.0f);
        RequireCamera(camera.CurrentVolumeInsideRoom(),
                      "transition sample remains inside room");
    }
}

void TestInitialPoseMatchesResetAndTransitionHome() {
    future_gaze::OrbitCamera camera;
    camera.SetRoomBounds(future_gaze::DefaultRoomBounds());
    camera.SetProjection(future_gaze::DegToRad(55.0f), 16.0f / 9.0f, 0.1f);
    camera.Reset();

    const future_gaze::OrbitCamera::CameraPose home = camera.HomePose();
    RequireNearCamera(camera.CurrentPose().target, home.target,
                      "reset target matches home pose");
    RequireNearCamera(camera.CurrentPose().distance, home.distance,
                      "reset distance matches home pose");

    camera.BeginDrag(0, 0);
    camera.DragTo(260, -180);
    camera.EndDrag();
    camera.StartTransitionTo(home, 1.0f);
    for (int i = 0; i < 32; ++i) {
        camera.Update(1.0f / 32.0f);
        RequireCamera(camera.CurrentVolumeInsideRoom(),
                      "home transition sample remains inside room");
    }
    RequireNearCamera(camera.CurrentPose().target, home.target,
                      "home transition target reaches home pose");
    RequireNearCamera(camera.CurrentPose().distance, home.distance,
                      "home transition distance reaches home pose");
}

}  // namespace

void RunCameraTests() {
    TestResetVolumeInsideRoom();
    TestPanZoomOrbitStayInsideRoom();
    TestTransitionSamplesStayInsideRoom();
    TestInitialPoseMatchesResetAndTransitionHome();
}
