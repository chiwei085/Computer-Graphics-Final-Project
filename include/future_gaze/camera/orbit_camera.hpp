#pragma once

#include "future_gaze/math/geometry.hpp"
#include "future_gaze/math/mat4.hpp"
#include "future_gaze/math/quat.hpp"
#include "future_gaze/math/vec3.hpp"

namespace future_gaze
{

class OrbitCamera
{
public:
    // A full snapshot of the camera's framing, used to record a vantage and to
    // drive scripted transitions between two vantages.
    struct CameraPose
    {
        Quat orientation;
        Vec3 target;
        float distance = 9.0f;
        float pitch = 0.0f;  // mirrors pitch_accumulated_ for clamp continuity
    };

    [[nodiscard]] Mat4 ViewMatrix() const;

    // Advances an in-flight scripted transition (no-op when idle). Call once
    // per frame before reading ViewMatrix().
    void Update(float delta_seconds);
    [[nodiscard]] bool transitioning() const noexcept { return transitioning_; }

    [[nodiscard]] CameraPose CurrentPose() const noexcept;
    [[nodiscard]] CameraPose HomePose() const noexcept;
    [[nodiscard]] static CameraPose InitialPose() noexcept;
    [[nodiscard]] Vec3 Eye() const;
    [[nodiscard]] Vec3 EyeForPose(const CameraPose& pose) const;
    [[nodiscard]] bool CurrentVolumeInsideRoom() const noexcept;
    [[nodiscard]] bool PoseVolumeInsideRoom(
        const CameraPose& pose) const noexcept;
    [[nodiscard]] bool CurrentSubjectFramed() const noexcept;
    // Smoothly sweep (SmootherStep) from the current pose to `goal` over
    // `duration` seconds. User input is ignored until it completes.
    void StartTransitionTo(const CameraPose& goal, float duration);
    // The eye's own viewpoint: the camera steps just in front of the Prediction
    // Core (at `eye_origin`, along `gaze_dir`) and looks at `look_at` — used to
    // watch the dinner table after it slides into the gaze and turns its front
    // to face the eye, so we see that presented front from the eye's vantage.
    [[nodiscard]] static CameraPose GazePose(Vec3 eye_origin, Vec3 gaze_dir,
                                             Vec3 look_at);

    // Orbit: left mouse drag — yaw around world Y, pitch around camera local X
    void BeginDrag(int x, int y);
    void DragTo(int x, int y);
    void EndDrag();

    // Pan: middle / right mouse drag — translate target in camera XY plane
    void BeginPan(int x, int y);
    void PanTo(int x, int y);
    void EndPan();

    void Zoom(float wheel_steps);
    void Reset();
    void SetProjection(float fov_y_radians, float aspect, float near_plane);
    void SetRoomBounds(RoomBounds bounds);
    void SetFraming(CameraFraming framing);

private:
    struct SolveResult
    {
        CameraPose pose{};
        bool inside = false;
        bool subject_framed = false;
    };

    void ApplyPose(const CameraPose& pose) noexcept;
    [[nodiscard]] CameraPose ConstrainPose(CameraPose pose) const noexcept;
    [[nodiscard]] SolveResult SolvePose(CameraPose pose,
                                        bool preserve_view) const noexcept;
    [[nodiscard]] CameraPose FramedPose(CameraPose seed) const noexcept;
    [[nodiscard]] CameraPose PoseFromEyeTarget(Vec3 eye,
                                               Vec3 target) const noexcept;
    [[nodiscard]] bool SubjectInsideFrustum(
        const CameraPose& pose) const noexcept;
    [[nodiscard]] Vec3 ViewDirection(const CameraPose& pose) const noexcept;

    // Orbit
    bool dragging_ = false;
    int last_x_ = 0;
    int last_y_ = 0;
    Quat orientation_ = Quat::FromAxisAngle({1.0f, 0.0f, 0.0f}, -0.25f);
    float pitch_accumulated_ = 0.25f;  // tracks total pitch for clamping
    float distance_ = 9.0f;

    // Pan
    bool panning_ = false;
    int pan_last_x_ = 0;
    int pan_last_y_ = 0;

    // Scripted transition (G-key gaze-POV sweep). While active, user input is
    // ignored and the pose is eased from `from` to `to`.
    bool transitioning_ = false;
    float transition_t_ = 0.0f;
    float transition_dur_ = 1.0f;
    CameraPose transition_from_;
    CameraPose transition_to_;

    Vec3 target_{0.0f, 1.5f, -0.5f};
    RoomBounds room_bounds_{DefaultRoomBounds()};
    CameraFraming framing_{DefaultRoomBounds().DefaultFraming()};
    float fov_y_radians_ = DegToRad(55.0f);
    float aspect_ = 16.0f / 9.0f;
    float near_plane_ = 0.1f;
};

}  // namespace future_gaze
