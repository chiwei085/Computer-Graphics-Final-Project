#pragma once

#include <string>
#include <vector>

#include "future_gaze/anim/animator.hpp"
#include "future_gaze/math/vec3.hpp"
#include "future_gaze/scene/gaze_controller.hpp"
#include "future_gaze/scene/tf_tree.hpp"

namespace future_gaze
{

class SceneGraph;

// Wires the procedural scene graph to the Animator:
//   Self-rotation  — green iris circuit ring spins in its own plane
//   Orbit          — Ingenuity rides a sin/cos orbit around the eye
//   Future bloom   — wine-glass "future-state" slices bloom open
//   Memory unfold  — chair memory panels unfold (keyframed) toward a diner
//   Gaze tracking  — Robonaut smoothly turns to track the orbiting drone
//
// Bind() is called once after the scene graph is built; Update() runs per
// frame.
class SceneAnimation
{
public:
    void Bind(SceneGraph& graph, const GazeController& gaze);
    void Update(float delta_seconds);

private:
    // Per-node rest pose captured at Bind time so channels animate as offsets.
    struct Rest
    {
        TfHandle handle{};
        Transform local;
        int target = 0;
        int slot = 0;
    };

    enum class FutureKind
    {
        Slice,
        Trajectory,
        Stain,
        Shard,
        BranchProp
    };
    enum class MemoryKind
    {
        Panel,
        Figure,
        Fort,
        Argument,
        Relic
    };

    struct FutureRest : Rest
    {
        FutureKind kind = FutureKind::Slice;
    };

    struct MemoryRest : Rest
    {
        MemoryKind kind = MemoryKind::Panel;
    };

    struct DanceRest
    {
        TfHandle handle{};
        std::string role;
        Transform local;
    };

    Animator animator_;
    TfTree* tf_ = nullptr;
    const GazeController* gaze_ = nullptr;

    // Self-rotating iris ring + the eye it lives on.
    TfHandle circuit_ring_{};

    // Orbiting drone + the character that tracks it.
    TfHandle ingenuity_{};
    TfHandle robonaut_{};
    Vec3 eye_center_;                // world-space orbit centre
    float robonaut_yaw_vel_ = 0.0f;  // SmoothDamp state
    Vec3 ingenuity_world_;           // updated each frame for the tracker
    Vec3 robonaut_rest_position_;
    Vec3 robonaut_rest_euler_;

    // Gaze-driven representative fragments.
    std::vector<FutureRest> future_fragments_;
    std::vector<MemoryRest> memory_fragments_;
    std::vector<DanceRest> robonaut_dance_joints_;
    KeyTrack unfold_;  // shared keyframed open/close curve for the panels

    void BuildChannels();
    void BindRobonautDance(SceneGraph& graph);
    [[nodiscard]] float FutureWeight() const;
    [[nodiscard]] float MemoryWeight() const;
};

}  // namespace future_gaze
