#include "future_gaze/camera/orbit_camera.hpp"

#include <algorithm>
#include <cmath>

#include "future_gaze/anim/easing.hpp"
#include "future_gaze/math/geometry.hpp"

namespace future_gaze
{

namespace
{

constexpr float kOrbitSensitivity = 0.008f;
constexpr float kPanSensitivity = 0.0012f;
constexpr float kMaxPitch = 1.25f;  // ≈ 72°
constexpr float kMinDistance = 2.0f;
constexpr float kMaxDistance = 18.0f;

const Quat kInitOrientation = Quat::FromAxisAngle({1.0f, 0.0f, 0.0f}, -0.25f);
const Vec3 kInitTarget{0.0f, 1.5f, -0.5f};
constexpr float kInitPitch = 0.25f;
constexpr float kInitDistance = 9.0f;

// Gaze-follow offsets: behind the eye, laterally offset, and raised.
constexpr float kGazeBackDist = 4.8f;
constexpr float kGazeSideDist = 2.8f;
constexpr float kGazeRise = 1.65f;

float AxisCorrection(float min_bound, float max_bound, float current_min,
                     float current_max) {
    if (current_min >= min_bound && current_max <= max_bound) {
        return 0.0f;
    }
    const float move_up = min_bound - current_min;
    const float move_down = max_bound - current_max;
    if (current_min < min_bound && current_max > max_bound) {
        return std::abs(move_up) < std::abs(move_down) ? move_up : move_down;
    }
    if (current_min < min_bound) {
        return move_up;
    }
    return move_down;
}

bool IsFinite(Vec3 v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

bool IsFinite(const Quat& q) {
    return std::isfinite(q.w) && std::isfinite(q.x) && std::isfinite(q.y) &&
           std::isfinite(q.z);
}

}  // namespace

Mat4 OrbitCamera::ViewMatrix() const {
    return Mat4::LookAt(Eye(), target_,
                        orientation_.Rotate({0.0f, 1.0f, 0.0f}));
}

void OrbitCamera::Update(float delta_seconds) {
    if (!transitioning_) {
        return;
    }
    transition_t_ += delta_seconds;
    const float u = std::clamp(transition_t_ / transition_dur_, 0.0f, 1.0f);
    const float e = ease::SmootherStep(u);

    CameraPose pose;
    pose.orientation =
        Quat::Slerp(transition_from_.orientation, transition_to_.orientation, e)
            .Normalized();
    pose.target = transition_from_.target +
                  (transition_to_.target - transition_from_.target) * e;
    pose.distance =
        ease::Lerp(transition_from_.distance, transition_to_.distance, e);
    pose.pitch = ease::Lerp(transition_from_.pitch, transition_to_.pitch, e);
    ApplyPose(SolvePose(pose, true).pose);

    if (u >= 1.0f) {
        transitioning_ = false;
    }
}

OrbitCamera::CameraPose OrbitCamera::CurrentPose() const noexcept {
    return CameraPose{orientation_, target_, distance_, pitch_accumulated_};
}

OrbitCamera::CameraPose OrbitCamera::HomePose() const noexcept {
    return SolvePose(InitialPose(), false).pose;
}

OrbitCamera::CameraPose OrbitCamera::InitialPose() noexcept {
    return CameraPose{kInitOrientation, kInitTarget, kInitDistance, kInitPitch};
}

void OrbitCamera::StartTransitionTo(const CameraPose& goal, float duration) {
    transition_from_ = ConstrainPose(CurrentPose());
    transition_to_ = SolvePose(goal, true).pose;
    transition_t_ = 0.0f;
    transition_dur_ = std::max(0.0001f, duration);
    transitioning_ = true;
    dragging_ = false;
    panning_ = false;
}

OrbitCamera::CameraPose OrbitCamera::GazePose(Vec3 eye_origin, Vec3 gaze_dir,
                                              Vec3 look_at) {
    const Vec3 g = Normalize(gaze_dir);
    Vec3 side = Normalize(Cross(g, {0.0f, 1.0f, 0.0f}));
    if (LengthSquared(side) < 0.0001f) {
        side = {1.0f, 0.0f, 0.0f};
    }
    // Shoulder camera avoids jamming the lens into the tabletop below the eye.
    const Vec3 cam_eye = eye_origin - g * kGazeBackDist + side * kGazeSideDist +
                         Vec3{0.0f, kGazeRise, 0.0f};
    const Vec3 forward = Normalize(look_at - cam_eye);

    CameraPose pose;
    pose.target = look_at;
    pose.distance = Length(look_at - cam_eye);
    // Match the world-Y-yaw / local-X-pitch convention used by drag controls.
    pose.pitch = std::asin(std::clamp(-forward.y, -1.0f, 1.0f));
    const float yaw = std::atan2(-forward.x, -forward.z);
    pose.orientation = (Quat::FromAxisAngle({0.0f, 1.0f, 0.0f}, yaw) *
                        Quat::FromAxisAngle({1.0f, 0.0f, 0.0f}, -pose.pitch))
                           .Normalized();
    return pose;
}

Vec3 OrbitCamera::Eye() const {
    return EyeForPose(CurrentPose());
}

void OrbitCamera::ApplyPose(const CameraPose& pose) noexcept {
    orientation_ = pose.orientation;
    target_ = pose.target;
    distance_ = pose.distance;
    pitch_accumulated_ = pose.pitch;
}

Vec3 OrbitCamera::EyeForPose(const CameraPose& pose) const {
    return pose.target + pose.orientation.Rotate({0.0f, 0.0f, pose.distance});
}

bool OrbitCamera::CurrentVolumeInsideRoom() const noexcept {
    return PoseVolumeInsideRoom(CurrentPose());
}

bool OrbitCamera::PoseVolumeInsideRoom(const CameraPose& pose) const noexcept {
    const CameraVolume volume = BuildCameraVolume(
        EyeForPose(pose), pose.orientation, room_bounds_.camera_radius,
        fov_y_radians_, aspect_, near_plane_);
    return CameraVolumeInsideRoom(volume, room_bounds_.CameraInterior());
}

bool OrbitCamera::CurrentSubjectFramed() const noexcept {
    return SubjectInsideFrustum(CurrentPose());
}

OrbitCamera::CameraPose OrbitCamera::PoseFromEyeTarget(
    Vec3 eye, Vec3 target) const noexcept {
    const Vec3 forward = Normalize(target - eye);
    CameraPose pose;
    pose.target = target;
    pose.distance =
        std::clamp(Length(target - eye), kMinDistance, kMaxDistance);
    pose.pitch = std::asin(std::clamp(-forward.y, -1.0f, 1.0f));
    const float yaw = std::atan2(-forward.x, -forward.z);
    pose.orientation = (Quat::FromAxisAngle({0.0f, 1.0f, 0.0f}, yaw) *
                        Quat::FromAxisAngle({1.0f, 0.0f, 0.0f}, -pose.pitch))
                           .Normalized();
    return pose;
}

OrbitCamera::CameraPose OrbitCamera::ConstrainPose(
    CameraPose pose) const noexcept {
    return SolvePose(pose, true).pose;
}

Vec3 OrbitCamera::ViewDirection(const CameraPose& pose) const noexcept {
    return Normalize(pose.orientation.Rotate({0.0f, 0.0f, -1.0f}));
}

bool OrbitCamera::SubjectInsideFrustum(const CameraPose& pose) const noexcept {
    const Vec3 eye = EyeForPose(pose);
    const CameraBasis basis = BuildCameraBasis(pose.orientation);
    const Vec3 to_subject = framing_.subject.center - eye;
    const float depth = Dot(to_subject, basis.forward);
    const float radius = framing_.subject.radius;
    if (depth <= std::max(near_plane_ + radius * 0.35f,
                          framing_.min_subject_distance * 0.35f)) {
        return false;
    }
    const float half_h = std::tan(fov_y_radians_ * 0.5f) * depth;
    const float half_w = half_h * aspect_;
    const float margin = std::max(1.0f, framing_.screen_margin);
    const float x = std::abs(Dot(to_subject, basis.right));
    const float y = std::abs(Dot(to_subject, basis.up));
    return x + radius * margin <= half_w && y + radius * margin <= half_h;
}

OrbitCamera::CameraPose OrbitCamera::FramedPose(
    CameraPose seed) const noexcept {
    if (!IsFinite(seed.target) || !IsFinite(seed.orientation)) {
        seed.orientation = kInitOrientation;
        seed.pitch = kInitPitch;
    }
    const float fit_distance =
        FitDistanceForSphere(framing_.subject.radius, fov_y_radians_, aspect_,
                             framing_.screen_margin);
    seed.target = framing_.subject.center;
    seed.distance = std::clamp(
        std::max({seed.distance, fit_distance, framing_.min_subject_distance}),
        kMinDistance, kMaxDistance);
    seed.pitch = std::clamp(seed.pitch, -kMaxPitch, kMaxPitch);
    return seed;
}

OrbitCamera::SolveResult OrbitCamera::SolvePose(
    CameraPose pose, bool preserve_view) const noexcept {
    if (!IsFinite(pose.target) || !IsFinite(pose.orientation)) {
        pose = CameraPose{kInitOrientation, kInitTarget, kInitDistance,
                          kInitPitch};
    }
    pose.orientation = pose.orientation.Normalized();
    pose.distance = std::clamp(pose.distance, kMinDistance, kMaxDistance);
    pose.pitch = std::clamp(pose.pitch, -kMaxPitch, kMaxPitch);
    if (!preserve_view) {
        pose = FramedPose(pose);
    }

    const Aabb3 camera_box = room_bounds_.CameraInterior();
    const Aabb3 eye_box = camera_box.Inset(room_bounds_.camera_radius);

    // Hoist basis outside the loop; orientation is fixed here.
    const CameraBasis loop_basis = BuildCameraBasis(pose.orientation);
    auto volume_bounds = [&](const CameraPose& p) {
        const Vec3 eye = EyeForPose(p);
        const auto corners =
            ComputeNearPlaneCorners(eye, loop_basis.forward, loop_basis.up,
                                    fov_y_radians_, aspect_, near_plane_);
        Aabb3 bounds{
            eye - Vec3{room_bounds_.camera_radius, room_bounds_.camera_radius,
                       room_bounds_.camera_radius},
            eye + Vec3{room_bounds_.camera_radius, room_bounds_.camera_radius,
                       room_bounds_.camera_radius}};
        for (const Vec3& corner : corners) {
            bounds = bounds.Include(corner);
        }
        return bounds;
    };

    for (int i = 0; i < 6; ++i) {
        const Aabb3 vb = volume_bounds(pose);
        const Vec3 delta{AxisCorrection(camera_box.min.x, camera_box.max.x,
                                        vb.min.x, vb.max.x),
                         AxisCorrection(camera_box.min.y, camera_box.max.y,
                                        vb.min.y, vb.max.y),
                         AxisCorrection(camera_box.min.z, camera_box.max.z,
                                        vb.min.z, vb.max.z)};
        if (LengthSquared(delta) <= 0.000001f) {
            break;
        }
        pose.target += delta;
    }

    const Vec3 eye = EyeForPose(pose);
    const Vec3 clamped_eye = eye_box.ClampPoint(eye);
    if (!NearlyEqual(eye, clamped_eye, 0.0001f)) {
        pose.target += clamped_eye - eye;
    }

    if (!PoseVolumeInsideRoom(pose)) {
        CameraPose shortened = pose;
        float lo = kMinDistance;
        float hi = pose.distance;
        for (int i = 0; i < 18; ++i) {
            CameraPose candidate = pose;
            candidate.distance = (lo + hi) * 0.5f;
            if (PoseVolumeInsideRoom(candidate)) {
                lo = candidate.distance;
                shortened = candidate;
            }
            else {
                hi = candidate.distance;
            }
        }
        pose = shortened;
    }

    if (!PoseVolumeInsideRoom(pose)) {
        const Vec3 clamped_target = camera_box.ClampPoint(pose.target);
        pose = PoseFromEyeTarget(eye_box.ClampPoint(EyeForPose(pose)),
                                 clamped_target);
    }

    const bool inside = PoseVolumeInsideRoom(pose);
    return {pose, inside, SubjectInsideFrustum(pose)};
}

void OrbitCamera::BeginDrag(int x, int y) {
    if (transitioning_) {
        return;
    }
    dragging_ = true;
    last_x_ = x;
    last_y_ = y;
}

void OrbitCamera::DragTo(int x, int y) {
    if (!dragging_) {
        return;
    }
    const float dx = static_cast<float>(x - last_x_);
    const float dy = static_cast<float>(y - last_y_);
    last_x_ = x;
    last_y_ = y;

    // Left-multiply yaw keeps world-up stable; negate dx so drag-right orbits right.
    const Quat yaw =
        Quat::FromAxisAngle({0.0f, 1.0f, 0.0f}, -dx * kOrbitSensitivity);
    orientation_ = (yaw * orientation_).Normalized();

    // Right-multiply pitch stays local; separate accumulator prevents flip past ±90°.
    const float raw_delta = dy * kOrbitSensitivity;
    const float new_pitch =
        std::clamp(pitch_accumulated_ + raw_delta, -kMaxPitch, kMaxPitch);
    const float clamped = new_pitch - pitch_accumulated_;
    pitch_accumulated_ = new_pitch;

    const Quat pitch = Quat::FromAxisAngle({1.0f, 0.0f, 0.0f}, -clamped);
    orientation_ = (orientation_ * pitch).Normalized();

    ApplyPose(SolvePose(CurrentPose(), true).pose);
}

void OrbitCamera::EndDrag() {
    dragging_ = false;
}

void OrbitCamera::BeginPan(int x, int y) {
    if (transitioning_) {
        return;
    }
    panning_ = true;
    pan_last_x_ = x;
    pan_last_y_ = y;
}

void OrbitCamera::PanTo(int x, int y) {
    if (!panning_) {
        return;
    }
    const float dx = static_cast<float>(x - pan_last_x_);
    const float dy = static_cast<float>(y - pan_last_y_);
    pan_last_x_ = x;
    pan_last_y_ = y;

    const Vec3 right = orientation_.Rotate({1.0f, 0.0f, 0.0f});
    const Vec3 up = orientation_.Rotate({0.0f, 1.0f, 0.0f});
    const float scale = distance_ * kPanSensitivity;

    target_ = target_ - right * (dx * scale) + up * (dy * scale);
    ApplyPose(SolvePose(CurrentPose(), true).pose);
}

void OrbitCamera::EndPan() {
    panning_ = false;
}

void OrbitCamera::Zoom(float wheel_steps) {
    if (transitioning_) {
        return;
    }
    distance_ =
        std::clamp(distance_ - wheel_steps * 0.35f, kMinDistance, kMaxDistance);
    ApplyPose(SolvePose(CurrentPose(), true).pose);
}

void OrbitCamera::Reset() {
    dragging_ = false;
    panning_ = false;
    transitioning_ = false;
    ApplyPose(HomePose());
}

void OrbitCamera::SetProjection(float fov_y_radians, float aspect,
                                float near_plane) {
    fov_y_radians_ = fov_y_radians;
    aspect_ = std::max(0.0001f, aspect);
    near_plane_ = std::max(0.001f, near_plane);
    ApplyPose(SolvePose(CurrentPose(), false).pose);
}

void OrbitCamera::SetRoomBounds(RoomBounds bounds) {
    room_bounds_ = bounds;
    framing_ = room_bounds_.DefaultFraming();
    ApplyPose(SolvePose(CurrentPose(), false).pose);
}

void OrbitCamera::SetFraming(CameraFraming framing) {
    framing_ = framing;
    framing_.subject.radius = std::max(0.01f, framing_.subject.radius);
    framing_.screen_margin = std::max(1.0f, framing_.screen_margin);
    framing_.min_subject_distance =
        std::max(kMinDistance, framing_.min_subject_distance);
    ApplyPose(SolvePose(CurrentPose(), false).pose);
}

}  // namespace future_gaze
