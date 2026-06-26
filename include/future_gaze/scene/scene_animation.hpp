#pragma once

#include <string>
#include <vector>

#include "future_gaze/anim/animator.hpp"
#include "future_gaze/math/vec3.hpp"
#include "future_gaze/physics/physics_world.hpp"
#include "future_gaze/scene/gaze_controller.hpp"
#include "future_gaze/scene/tf_tree.hpp"

namespace future_gaze
{

class SceneGraph;

// Drives per-frame animation channels: iris spin, Ingenuity orbit, gaze-driven
// fragment bloom/collapse, Robonaut patrol and gaze tracking.
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
        physics::BodyId physics_body{};
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

    struct BlindRest
    {
        TfHandle handle{};
        Transform local;
        int slot = 0;
    };

    struct DanceRest
    {
        TfHandle handle{};
        std::string role;
        Transform local;
    };

    struct StaticPhysicsBinding
    {
        physics::BodyId body{};
        TfHandle root{};
        Vec3 local_position{};
        float local_yaw_deg = 0.0f;
    };

    Animator animator_;
    TfTree* tf_ = nullptr;
    const GazeController* gaze_ = nullptr;
    physics::PhysicsWorld physics_;

    TfHandle table_root_{};
    TfHandle props_root_{};
    Vec3 last_table_pos_{1.0e30f, 1.0e30f, 1.0e30f};
    Vec3 last_props_pos_{1.0e30f, 1.0e30f, 1.0e30f};
    float last_table_yaw_ = 1.0e30f;
    float last_props_yaw_ = 1.0e30f;

    TfHandle circuit_ring_{};

    TfHandle ingenuity_{};
    TfHandle robonaut_{};
    Vec3 eye_center_;
    float robonaut_yaw_vel_ = 0.0f;  // SmoothDamp state for yaw tracking
    Vec3 ingenuity_world_;
    Vec3 robonaut_rest_position_;
    Vec3 robonaut_rest_euler_;
    Vec3 robonaut_phys_pos_{};
    Vec3 robonaut_phys_vel_{};
    physics::BodyId robonaut_body_{};
    physics::BodyId ingenuity_body_{};

    std::vector<FutureRest> future_fragments_;
    std::vector<MemoryRest> memory_fragments_;
    std::vector<BlindRest> blind_backdrop_;
    std::vector<BlindRest> future_backdrop_;
    std::vector<BlindRest> memory_backdrop_;
    std::vector<DanceRest> robonaut_dance_joints_;
    std::vector<StaticPhysicsBinding> static_physics_;
    KeyTrack unfold_;

    void BuildChannels();
    void BindRobonautDance(SceneGraph& graph);
    void BuildPhysicsWorld();
    void SyncPhysicsStaticObstacles();
    void SyncFragmentSensor(physics::BodyId body, TfHandle handle);
    [[nodiscard]] Vec3 ResolveSensorPosition(physics::BodyId body, Vec3 desired,
                                             float yaw_deg);
    [[nodiscard]] float FutureWeight() const;
    [[nodiscard]] float MemoryWeight() const;
    [[nodiscard]] float BlindWeight() const;
};

}  // namespace future_gaze
