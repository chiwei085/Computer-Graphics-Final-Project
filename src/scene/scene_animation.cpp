#include "future_gaze/scene/scene_animation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numbers>
#include <string>
#include <utility>

#include "future_gaze/anim/easing.hpp"
#include "future_gaze/scene/node_names.hpp"
#include "future_gaze/scene/scene_graph.hpp"

namespace future_gaze
{

namespace
{
constexpr float kPi = std::numbers::pi_v<float>;
constexpr float kRadToDeg = 180.0f / kPi;
constexpr float kDanceBeat = 2.65f;

constexpr std::uint32_t kPhysicsObstacle = 1u << 0u;
constexpr std::uint32_t kPhysicsCharacter = 1u << 1u;
constexpr std::uint32_t kPhysicsAerial = 1u << 2u;
constexpr std::uint32_t kPhysicsFragment = 1u << 3u;
constexpr std::uint32_t kPhysicsEyeSafety = 1u << 4u;

constexpr physics::CollisionMask kObstacleMask{
    kPhysicsObstacle, kPhysicsCharacter | kPhysicsFragment};
constexpr physics::CollisionMask kCharacterMask{kPhysicsCharacter,
                                                kPhysicsObstacle};
constexpr physics::CollisionMask kAerialMask{kPhysicsAerial, kPhysicsEyeSafety};
constexpr physics::CollisionMask kFragmentMask{kPhysicsFragment,
                                               kPhysicsObstacle};
constexpr physics::CollisionMask kEyeSafetyMask{kPhysicsEyeSafety,
                                                kPhysicsAerial};

// Binding is name-based, so a missing target is silent until something visibly
// fails to move. These helpers turn that into a loud one-line warning at
// startup (captured on stderr) instead of a no-op channel.
TfHandle FindOrWarn(SceneGraph& graph, const char* name) {
    const TfHandle handle = graph.Find(name);
    if (!handle.IsValid()) {
        std::fprintf(stderr,
                     "[SceneAnimation] node '%s' not found — its animation is "
                     "disabled.\n",
                     name);
    }
    return handle;
}

std::vector<TfHandle> CollectOrWarn(SceneGraph& graph, const char* prefix) {
    std::vector<TfHandle> out;
    graph.Collect(prefix, out);
    if (out.empty()) {
        std::fprintf(stderr,
                     "[SceneAnimation] no nodes matching '%s' — its animation "
                     "is disabled.\n",
                     prefix);
    }
    return out;
}

int ParseTargetIndex(const std::string& name, const char* prefix) {
    const std::string p(prefix);
    if (name.rfind(p, 0) != 0 || name.size() <= p.size()) {
        return 0;
    }
    const char value = name[p.size()];
    if (value < '0' || value > '9') {
        return 0;
    }
    return value - '0';
}

int ParseSlotIndex(const std::string& name) {
    const std::size_t pos = name.rfind('_');
    if (pos == std::string::npos || pos + 1 >= name.size()) {
        return 0;
    }
    const char value = name[pos + 1];
    if (value < '0' || value > '9') {
        return 0;
    }
    return value - '0';
}

bool Contains(const std::string& text, const char* token) {
    return text.find(token) != std::string::npos;
}

float Wave(float t, float phase = 0.0f) {
    return std::sin(t * kDanceBeat + phase);
}

Vec3 RotateDinnerLocal(Vec3 local, float yaw_deg) {
    const float yaw = yaw_deg * (kPi / 180.0f);
    const float s = std::sin(yaw);
    const float c = std::cos(yaw);
    return {local.x * c + local.z * s, local.y, -local.x * s + local.z * c};
}

constexpr std::array<Vec3, 3> kFutureAnchors = {
    Vec3{-1.02f, 1.10f, -0.14f},
    Vec3{0.18f, 1.10f, -0.14f},
    Vec3{1.38f, 1.10f, -0.14f},
};

constexpr std::array<Vec3, 3> kMemoryAnchors = {
    Vec3{-1.20f, 0.78f, 1.35f},
    Vec3{0.00f, 0.78f, 1.22f},
    Vec3{1.20f, 0.78f, 1.38f},
};

}  // namespace

void SceneAnimation::Bind(SceneGraph& graph, const GazeController& gaze) {
    tf_ = &graph.tf();
    gaze_ = &gaze;

    // ── Resolve animation targets ────────────────────────────────────────────
    table_root_ = FindOrWarn(graph, "dinner_scene");
    circuit_ring_ = FindOrWarn(graph, names::kCircuitRing);
    ingenuity_ = FindOrWarn(graph, names::kIngenuity);
    robonaut_ = FindOrWarn(graph, names::kRobonaut);
    if (tf_->Contains(robonaut_)) {
        const Transform rest = tf_->Local(robonaut_);
        robonaut_rest_position_ = rest.position;
        robonaut_rest_euler_ = rest.euler_deg;
        robonaut_phys_pos_ = rest.position;  // seed physics at scene start pos
    }

    eye_center_ = gaze.Origin();
    BindRobonautDance(graph);

    // Gather authored story fragments and snapshot their fully-open rest pose.
    for (TfHandle n : CollectOrWarn(graph, names::kFuturePrefix)) {
        const std::string& name = tf_->Name(n);
        FutureRest r;
        r.handle = n;
        r.local = tf_->Local(n);
        r.target = ParseTargetIndex(name, names::kFuturePrefix);
        r.slot = ParseSlotIndex(name);
        if (Contains(name, "_branch_")) {
            r.kind = FutureKind::BranchProp;
        }
        else if (Contains(name, "_trajectory_")) {
            r.kind = FutureKind::Trajectory;
        }
        else if (Contains(name, "_stain_")) {
            r.kind = FutureKind::Stain;
        }
        else if (Contains(name, "_shard_")) {
            r.kind = FutureKind::Shard;
        }
        else {
            r.kind = FutureKind::Slice;
        }
        future_fragments_.push_back(r);
    }
    for (TfHandle n : CollectOrWarn(graph, names::kMemoryPrefix)) {
        const std::string& name = tf_->Name(n);
        MemoryRest r;
        r.handle = n;
        r.local = tf_->Local(n);
        r.target = ParseTargetIndex(name, names::kMemoryPrefix);
        r.slot = ParseSlotIndex(name);
        if (Contains(name, "_relic_")) {
            r.kind = MemoryKind::Relic;
        }
        else if (Contains(name, "_figure_")) {
            r.kind = MemoryKind::Figure;
        }
        else if (Contains(name, "_fort_")) {
            r.kind = MemoryKind::Fort;
        }
        else if (Contains(name, "_argument_")) {
            r.kind = MemoryKind::Argument;
        }
        else {
            r.kind = MemoryKind::Panel;
        }
        memory_fragments_.push_back(r);
    }

    // Shared keyframed unfold curve: closed → spring open → hold → close.
    unfold_.Add(0.0f, 0.0f, ease::SmootherStep)
        .Add(1.4f, 1.0f, ease::OutBackEase)
        .Add(4.6f, 1.0f, ease::SmoothStep)
        .Add(6.0f, 0.0f, ease::InOutCubic);

    BuildPhysicsWorld();
    BuildChannels();
}

void SceneAnimation::BuildPhysicsWorld() {
    physics_.Clear();
    static_physics_.clear();

    auto add_table_static = [&](physics::Collider2D collider,
                                Vec3 local_position, float local_yaw_deg,
                                physics::HeightRange height) {
        const physics::BodyId body = physics_.AddStatic(
            collider, physics::Transform2D{local_position, local_yaw_deg},
            height, kObstacleMask);
        static_physics_.push_back({body, local_position, local_yaw_deg});
    };

    // Dinner table blockers, authored in dinner_scene local space.
    add_table_static(physics::Collider2D::Box(1.05f, 0.475f), {}, 0.0f,
                     {0.70f, 0.86f});
    for (const Vec3 p : {Vec3{-1.98f, 0.0f, -0.88f}, Vec3{1.98f, 0.0f, -0.88f},
                         Vec3{-1.98f, 0.0f, 0.88f}, Vec3{1.98f, 0.0f, 0.88f}}) {
        add_table_static(physics::Collider2D::Circle(0.055f), p, 0.0f,
                         {0.0f, 0.76f});
    }

    struct ChairSpec
    {
        Vec3 base;
        float yaw_deg = 0.0f;
    };
    constexpr std::array<ChairSpec, 6> kChairs = {{
        {{-1.20f, 0.0f, -1.38f}, -5.0f},
        {{0.00f, 0.0f, -1.50f}, 0.0f},
        {{1.20f, 0.0f, -1.35f}, 7.0f},
        {{-1.20f, 0.0f, 1.35f}, 185.0f},
        {{0.00f, 0.0f, 1.22f}, 180.0f},
        {{1.20f, 0.0f, 1.38f}, 174.0f},
    }};
    for (const ChairSpec& chair : kChairs) {
        add_table_static(physics::Collider2D::Circle(0.34f), chair.base,
                         chair.yaw_deg, {0.0f, 0.78f});
        const Vec3 back_local =
            RotateDinnerLocal({0.0f, 0.0f, -0.20f}, chair.yaw_deg);
        add_table_static(physics::Collider2D::Box(0.24f, 0.045f),
                         chair.base + back_local, chair.yaw_deg,
                         {0.45f, 1.02f});
    }

    (void)physics_.AddStatic(physics::Collider2D::Circle(2.12f),
                             physics::Transform2D{eye_center_, 0.0f},
                             {1.60f, 4.80f}, kEyeSafetyMask);

    if (tf_ != nullptr && tf_->Contains(robonaut_)) {
        robonaut_body_ = physics_.AddKinematic(
            physics::Collider2D::Capsule(0.38f, 0.17f), robonaut_phys_pos_,
            {0.0f, 1.80f}, kCharacterMask);
    }
    if (tf_ != nullptr && tf_->Contains(ingenuity_)) {
        const Vec3 pos = tf_->WorldPosition(ingenuity_);
        ingenuity_body_ = physics_.AddSensor(physics::Collider2D::Circle(0.46f),
                                             pos, {2.30f, 4.50f}, kAerialMask);
    }
    for (FutureRest& fragment : future_fragments_) {
        const Vec3 pos = tf_->WorldPosition(fragment.handle);
        fragment.physics_body =
            physics_.AddSensor(physics::Collider2D::Circle(0.10f), pos,
                               {pos.y + 0.05f, pos.y + 0.32f}, kFragmentMask);
    }
    for (MemoryRest& fragment : memory_fragments_) {
        const Vec3 pos = tf_->WorldPosition(fragment.handle);
        fragment.physics_body =
            physics_.AddSensor(physics::Collider2D::Circle(0.14f), pos,
                               {pos.y + 0.03f, pos.y + 0.36f}, kFragmentMask);
    }

    SyncPhysicsStaticObstacles();
}

void SceneAnimation::SyncPhysicsStaticObstacles() {
    Vec3 root_pos{};
    float root_yaw = 0.0f;
    if (tf_ != nullptr && tf_->Contains(table_root_)) {
        const Transform table = tf_->Local(table_root_);
        root_pos = table.position;
        root_yaw = table.euler_deg.y;
    }

    if (root_yaw == last_table_yaw_ && root_pos.x == last_table_pos_.x &&
        root_pos.z == last_table_pos_.z) {
        return;
    }
    last_table_yaw_ = root_yaw;
    last_table_pos_ = root_pos;

    for (const StaticPhysicsBinding& binding : static_physics_) {
        const Vec3 world_position =
            root_pos + RotateDinnerLocal(binding.local_position, root_yaw);
        physics_.SetBodyTransform(binding.body, world_position,
                                  root_yaw + binding.local_yaw_deg);
    }
}

Vec3 SceneAnimation::ResolveSensorPosition(physics::BodyId body, Vec3 desired,
                                           float yaw_deg) {
    if (!body.IsValid()) {
        return desired;
    }
    Vec3 resolved = desired;
    for (int i = 0; i < 3; ++i) {
        physics_.SetBodyTransform(body, resolved, yaw_deg);
        const std::vector<physics::Contact> contacts =
            physics_.QueryOverlaps(body);
        if (contacts.empty()) {
            break;
        }
        Vec3 correction{};
        for (const physics::Contact& contact : contacts) {
            const float push = std::min(contact.penetration + 0.002f, 0.08f);
            correction = correction + contact.normal * push;
        }
        resolved = resolved + Vec3{correction.x, 0.0f, correction.z};
    }
    physics_.SetBodyTransform(body, resolved, yaw_deg);
    return resolved;
}

void SceneAnimation::SyncFragmentSensor(physics::BodyId body, TfHandle handle) {
    if (tf_ == nullptr || !body.IsValid() || !tf_->Contains(handle)) {
        return;
    }
    const Vec3 pos = tf_->WorldPosition(handle);
    Transform local = tf_->Local(handle);
    physics_.SetBodyHeight(body, {pos.y + 0.04f, pos.y + 0.32f});
    physics_.SetBodyTransform(body, pos, local.euler_deg.y);

    Vec3 correction{};
    for (const physics::Contact& contact : physics_.QueryOverlaps(body)) {
        const float push = std::min(contact.penetration + 0.002f, 0.035f);
        correction = correction + contact.normal * push;
    }
    if (std::abs(correction.x) <= 0.0001f &&
        std::abs(correction.z) <= 0.0001f) {
        return;
    }

    local.position = local.position + Vec3{correction.x, 0.0f, correction.z};
    tf_->SetLocal(handle, local);
    physics_.SetBodyTransform(body, tf_->WorldPosition(handle),
                              local.euler_deg.y);
}

void SceneAnimation::BuildChannels() {
    // ── Self-rotation
    // ───────────────────────────────────────────────────────── Only the green
    // circuit ring spins, in its own plane (rot_y about its normal — see
    // prediction_core's circuit_tilt/circuit_ring split), so it never tumbles
    // into the sclera. The dark iris frame stays put.
    animator_.Add("self_rotation", [this](float t, float) {
        if (tf_ != nullptr && tf_->Contains(circuit_ring_)) {
            Transform local = tf_->Local(circuit_ring_);
            local.euler_deg.y = std::fmod(t * 42.0f, 360.0f);
            tf_->SetLocal(circuit_ring_, local);
        }
    });

    // ── Orbit motion
    // ────────────────────────────────────────────────────────── Ingenuity
    // rides a cosine/sine orbit around the eye with a vertical bob and banks
    // into the turn. Also publishes its world position for the tracker.
    animator_.Add("orbit_drone", [this](float t, float) {
        // Must clear the eye's full extent in EVERY yaw/pitch pose, not just
        // the 1.2 sclera: the cable bundle reaches ~2.10 and the cyan glow ~2.0
        // from the centre, and they sweep around as the user rotates the gaze.
        // With the drone's ~0.4 half-extent, 2.35 left only a ~0.25 gap → the
        // drone clipped through the cables. 3.0 clears cables+drone with
        // margin.
        constexpr float kRadius = 3.0f;
        constexpr float kOmega = 0.55f;  // rad/s
        const float a = t * kOmega;
        const float bob = 0.40f * std::sin(a * 2.0f);
        ingenuity_world_ = {eye_center_.x + kRadius * std::cos(a),
                            eye_center_.y + bob,
                            eye_center_.z + kRadius * std::sin(a)};
        ingenuity_world_ = ResolveSensorPosition(
            ingenuity_body_, ingenuity_world_, -a * kRadToDeg + 90.0f);
        if (tf_ != nullptr && tf_->Contains(ingenuity_)) {
            Transform local = tf_->Local(ingenuity_);
            local.position = ingenuity_world_;
            // Face along the tangent (heading) and bank with the bob.
            local.euler_deg.y = -a * kRadToDeg + 90.0f;
            local.euler_deg.z = 14.0f * std::cos(a * 2.0f);
            local.euler_deg.x = 6.0f * std::sin(a * 2.0f);
            tf_->SetLocal(ingenuity_, local);
        }
    });

    // ── Future-state fragments
    // ───────────────────────────────────────────── Results invade causes only
    // where the Prediction Core looks. Slices, stains and shards all share the
    // same gaze weight but reveal with different geometry grammar.
    animator_.Add("gaze_future", [this](float t, float) {
        for (const FutureRest& r : future_fragments_) {
            const float w = FutureWeight();
            const float slot = static_cast<float>(r.slot);
            const float tremor = 0.94f + 0.06f * std::sin(t * 3.0f + slot);
            Transform local = r.local;
            switch (r.kind) {
                case FutureKind::Slice: {
                    const float lead =
                        ease::Clamp01(w * (1.16f - slot * 0.055f));
                    local.position = r.local.position +
                                     Vec3{0.0f, 0.065f * slot * lead, 0.0f};
                    local.scale = {
                        r.local.scale.x * (0.06f + 0.94f * lead),
                        r.local.scale.y * (0.20f + 1.40f * lead) * tremor,
                        r.local.scale.z * (0.06f + 0.94f * lead)};
                    local.euler_deg =
                        r.local.euler_deg +
                        Vec3{0.0f, t * 20.0f + slot * 18.0f * lead, 0.0f};
                    break;
                }
                case FutureKind::Trajectory: {
                    const float lead =
                        ease::Clamp01(w * (1.05f - slot * 0.06f));
                    local.scale = r.local.scale * (0.03f + 0.97f * lead);
                    local.euler_deg =
                        r.local.euler_deg +
                        Vec3{0.0f, 0.0f,
                             7.0f * std::sin(t * 2.1f + slot) * lead};
                    break;
                }
                case FutureKind::Stain: {
                    const float spread =
                        ease::Clamp01(w * (1.0f - slot * 0.14f));
                    local.scale = {r.local.scale.x * spread, r.local.scale.y,
                                   r.local.scale.z * spread};
                    local.euler_deg.y = r.local.euler_deg.y + 9.0f * spread;
                    break;
                }
                case FutureKind::Shard: {
                    const float scatter =
                        ease::Clamp01(w * (1.08f - slot * 0.04f));
                    local.scale = r.local.scale * scatter;
                    local.position =
                        r.local.position + Vec3{0.025f * slot * scatter, 0.0f,
                                                -0.016f * slot * scatter};
                    local.euler_deg =
                        r.local.euler_deg +
                        Vec3{0.0f, t * 10.0f * scatter, 16.0f * scatter};
                    break;
                }
                case FutureKind::BranchProp: {
                    const float lead =
                        ease::Clamp01(w * (1.18f - slot * 0.075f));
                    const Vec3 origin =
                        kFutureAnchors[static_cast<std::size_t>(r.target)];
                    const Vec3 tucked =
                        origin + Vec3{0.06f * slot, -0.27f, 0.08f};
                    local.position =
                        tucked + (r.local.position - tucked) * lead;
                    local.scale = r.local.scale * (0.08f + 0.92f * lead);
                    local.euler_deg =
                        r.local.euler_deg +
                        Vec3{6.0f * std::sin(t * 1.6f + slot) * lead,
                             10.0f * std::sin(t * 0.9f + slot) * lead,
                             7.0f * std::sin(t * 1.2f + slot) * lead};
                    break;
                }
            }
            tf_->SetLocal(r.handle, local);
            SyncFragmentSensor(r.physics_body, r.handle);
        }
    });

    // ── Longing/private fronts
    // ───────────────────────────────────────────── Memory fragments open only
    // toward the viewer relationship selected by the eye. From the front they
    // read as figures, forts and argument slashes; from the side they remain
    // thin confusing plates.
    animator_.Add("gaze_memory", [this](float t, float) {
        for (const MemoryRest& r : memory_fragments_) {
            const float w = MemoryWeight();
            const float breathe =
                (0.96f + 0.04f * std::sin(t * 1.8f + r.target + r.slot)) * w;
            Transform local = r.local;
            switch (r.kind) {
                case MemoryKind::Panel:
                    local.scale = {r.local.scale.x, r.local.scale.y * breathe,
                                   r.local.scale.z * (0.25f + 0.75f * w)};
                    local.position =
                        r.local.position + Vec3{0.0f, 0.20f * w, 0.0f};
                    local.euler_deg =
                        r.local.euler_deg +
                        Vec3{0.0f, 10.0f * std::sin(t * 0.55f + r.target) * w,
                             0.0f};
                    break;
                case MemoryKind::Figure: {
                    const Vec3 tucked_f{0.0f, 0.58f, 0.05f};
                    local.scale = r.local.scale * breathe;
                    local.position =
                        tucked_f + (r.local.position - tucked_f) * w;
                    break;
                }
                case MemoryKind::Fort: {
                    const Vec3 tucked_ft{0.0f, 0.56f, 0.04f};
                    local.scale = r.local.scale * breathe;
                    local.position =
                        tucked_ft + (r.local.position - tucked_ft) * w;
                    local.euler_deg.z = r.local.euler_deg.z +
                                        3.0f * std::sin(t * 1.2f + r.slot) * w;
                    break;
                }
                case MemoryKind::Argument: {
                    const Vec3 tucked_a{0.0f, 0.56f, 0.05f};
                    local.scale = r.local.scale * breathe;
                    local.position =
                        tucked_a + (r.local.position - tucked_a) * w;
                    local.euler_deg.y = r.local.euler_deg.y +
                                        9.0f * std::sin(t * 1.1f + r.slot) * w;
                    break;
                }
                case MemoryKind::Relic: {
                    const Vec3 chair =
                        kMemoryAnchors[static_cast<std::size_t>(r.target)] +
                        Vec3{0.0f, -0.22f, -0.18f};
                    const float open =
                        ease::Clamp01(w * (1.10f - r.slot * 0.055f));
                    const float pulse =
                        0.985f + 0.015f * std::sin(t * 1.3f + r.slot);
                    local.scale =
                        r.local.scale * ((0.10f + 0.90f * open) * pulse);
                    local.position = chair + (r.local.position - chair) * open;
                    local.euler_deg =
                        r.local.euler_deg +
                        Vec3{0.0f, 5.0f * std::sin(t * 0.7f + r.slot) * open,
                             3.0f * std::sin(t * 1.1f + r.target) * open};
                    break;
                }
            }
            tf_->SetLocal(r.handle, local);
            SyncFragmentSensor(r.physics_body, r.handle);
        }
    });

    // ── Patrol + collision physics
    // ──────────────────────────────────────── Spring-mass-damper (ζ ≈ 0.71)
    // follows an elliptical perimeter; the new physics world owns the actual
    // swept movement so frame spikes cannot jump through table/chair blockers.
    animator_.Add("robonaut_patrol", [this](float t, float dt) {
        constexpr float kRx = 2.4f;
        constexpr float kRz = 1.8f;
        constexpr float kCz = 0.5f;
        constexpr float kOmega = 0.08f;
        constexpr float kSpring = 2.0f;
        constexpr float kDamp = 2.0f;
        constexpr float kMaxSpeed = 1.5f;

        if (!robonaut_body_.IsValid()) {
            return;
        }
        const float safe_dt = std::max(dt, 0.0001f);

        // ── Spring toward world-space patrol target
        // ───────────────────────────
        const float ta = t * kOmega;
        const float tx = kRx * std::cos(ta);
        const float tz = kCz + kRz * std::sin(ta);
        const float ax = (tx - robonaut_phys_pos_.x) * kSpring -
                         robonaut_phys_vel_.x * kDamp;
        const float az = (tz - robonaut_phys_pos_.z) * kSpring -
                         robonaut_phys_vel_.z * kDamp;
        robonaut_phys_vel_.x += ax * safe_dt;
        robonaut_phys_vel_.z += az * safe_dt;
        const float spd =
            std::sqrt(robonaut_phys_vel_.x * robonaut_phys_vel_.x +
                      robonaut_phys_vel_.z * robonaut_phys_vel_.z);
        if (spd > kMaxSpeed) {
            const float inv = kMaxSpeed / spd;
            robonaut_phys_vel_.x *= inv;
            robonaut_phys_vel_.z *= inv;
        }

        const Vec3 desired_delta{robonaut_phys_vel_.x * safe_dt, 0.0f,
                                 robonaut_phys_vel_.z * safe_dt};
        physics::MoveOptions options;
        options.max_iterations = 5;
        options.skin = 0.006f;
        const physics::MoveResult move = physics_.MoveKinematic(
            robonaut_body_, desired_delta, safe_dt, options);

        robonaut_phys_pos_ = move.final_position;
        robonaut_phys_vel_.x = move.applied_delta.x / safe_dt;
        robonaut_phys_vel_.z = move.applied_delta.z / safe_dt;

        robonaut_rest_position_.x = robonaut_phys_pos_.x;
        robonaut_rest_position_.z = robonaut_phys_pos_.z;
    });

    // ── Gaze tracking
    // ───────────────────────────────────────────────────────── Robonaut
    // smoothly (critically damped) turns to face the orbiting drone. Runs after
    // orbit_drone so it reads the drone's freshly-updated position.
    animator_.Add("robonaut_track", [this](float, float dt) {
        if (tf_ == nullptr || !tf_->Contains(robonaut_)) {
            return;
        }
        Transform local = tf_->Local(robonaut_);
        const Vec3 to = ingenuity_world_ - local.position;
        // Model forward is offset by its rest yaw; aim that forward at the
        // drone.
        const float target_yaw = std::atan2(to.x, to.z) * kRadToDeg + 180.0f;
        local.euler_deg.y = ease::SmoothDampAngleDeg(
            local.euler_deg.y, target_yaw, robonaut_yaw_vel_, 0.7f,
            std::max(dt, 0.0001f));
        tf_->SetLocal(robonaut_, local);
    });

    animator_.Add("robonaut_procedural_dance", [this](float t, float) {
        // All geometry lives on the Center node (OBJ has no joint rigging).
        // Only Centre and Head pivots carry meshes; all other pivot nodes are
        // empty. Animating them would corrupt the head's compound rotation.
        // Motion design: sentinel-stance weight shift + head scan.
        const float beat = t * kDanceBeat;
        const float bounce = 0.5f + 0.5f * std::sin(beat * 2.0f);
        if (tf_ != nullptr && tf_->Contains(robonaut_)) {
            Transform local = tf_->Local(robonaut_);
            local.position =
                robonaut_rest_position_ + Vec3{0.010f * std::sin(beat * 0.4f),
                                               0.030f * bounce,
                                               0.015f * std::sin(beat + 0.65f)};
            local.euler_deg.x =
                robonaut_rest_euler_.x + 2.0f * std::sin(beat + 0.6f);
            local.euler_deg.z =
                robonaut_rest_euler_.z + 3.0f * std::sin(beat * 0.5f);
            tf_->SetLocal(robonaut_, local);
        }
        for (const DanceRest& joint : robonaut_dance_joints_) {
            Vec3 euler{};
            Vec3 offset{};
            if (joint.role == "center") {
                // Gentle sentinel sway — amplitude kept small so the rigid
                // mesh reads as balanced, not flailing.
                offset = {0.0f, 0.018f * std::abs(Wave(t)), 0.0f};
                euler = {2.5f * Wave(t, 0.8f), 0.0f, 3.5f * Wave(t)};
            }
            else if (joint.role == "head") {
                // Neck ring is a single-DOF pitch joint: nod only.
                // Yaw is handled by the whole-body robonaut_track channel;
                // roll is not mechanically possible on this design.
                euler = {10.0f * Wave(t, 1.5f), 0.0f, 0.0f};
            }
            // All other pivot nodes (upper_body, lower_body, arms, legs, …)
            // hold their rest pose; their euler stays {0,0,0}.
            Transform local = joint.local;
            local.position = joint.local.position + offset;
            local.euler_deg = joint.local.euler_deg + euler;
            tf_->SetLocal(joint.handle, local);
        }
    });
}

void SceneAnimation::Update(float delta_seconds) {
    SyncPhysicsStaticObstacles();
    animator_.Update(delta_seconds);
}

void SceneAnimation::BindRobonautDance(SceneGraph& graph) {
    if (!graph.Find(names::kRobonautDanceRoot).IsValid()) {
        return;
    }

    auto add = [&](const char* node_name, std::string role) {
        const TfHandle node = graph.Find(node_name);
        if (!node.IsValid()) {
            std::fprintf(stderr,
                         "[SceneAnimation] dance proxy node '%s' not found.\n",
                         node_name);
            return;
        }
        robonaut_dance_joints_.push_back(
            DanceRest{node, std::move(role), tf_->Local(node)});
    };

    add(names::kRobonautDanceCenter, "center");
    add(names::kRobonautDanceLowerBody, "lower_body");
    add(names::kRobonautDanceUpperBody, "upper_body");
    add(names::kRobonautDanceHead, "head");

    add(names::kRobonautDanceLeftShoulder, "left_shoulder");
    add(names::kRobonautDanceLeftArm, "left_arm");
    add(names::kRobonautDanceLeftElbow, "left_elbow");
    add(names::kRobonautDanceLeftWrist, "left_wrist");

    add(names::kRobonautDanceRightShoulder, "right_shoulder");
    add(names::kRobonautDanceRightArm, "right_arm");
    add(names::kRobonautDanceRightElbow, "right_elbow");
    add(names::kRobonautDanceRightWrist, "right_wrist");

    add(names::kRobonautDanceLeftLeg, "left_leg");
    add(names::kRobonautDanceLeftKnee, "left_knee");
    add(names::kRobonautDanceLeftAnkle, "left_ankle");
    add(names::kRobonautDanceRightLeg, "right_leg");
    add(names::kRobonautDanceRightKnee, "right_knee");
    add(names::kRobonautDanceRightAnkle, "right_ankle");
}

// Gating is now per gaze ZONE, not per spatial target: rotating the eye into
// the Foresight (預見) sector blooms *every* future fragment across the whole
// table at once, and likewise Longing (眷戀) for memory fragments. The `target`
// only still seeds per-anchor phase variety in the channels above; it no longer
// decides whether a fragment is active. In the Blindspot (盲點) sector both
// weights fall to ~0, so the table collapses back to plain objects + blind kit.
float SceneAnimation::FutureWeight() const {
    if (gaze_ == nullptr) {
        return 0.0f;
    }
    return gaze_->ZoneWeight(GazeZone::kForesight);
}

float SceneAnimation::MemoryWeight() const {
    if (gaze_ == nullptr) {
        return 0.0f;
    }
    return gaze_->ZoneWeight(GazeZone::kLonging);
}

}  // namespace future_gaze
