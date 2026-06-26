#include "future_gaze/scene/scene_animation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
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

float Snap(float t, float phase = 0.0f) {
    const float s = Wave(t, phase);
    return std::copysign(std::sqrt(std::abs(s)), s);
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
    circuit_ring_ = FindOrWarn(graph, names::kCircuitRing);
    ingenuity_ = FindOrWarn(graph, names::kIngenuity);
    robonaut_ = FindOrWarn(graph, names::kRobonaut);
    if (tf_->Contains(robonaut_)) {
        const Transform rest = tf_->Local(robonaut_);
        robonaut_rest_position_ = rest.position;
        robonaut_rest_euler_ = rest.euler_deg;
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

    BuildChannels();
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
                    const float lead = ease::Clamp01(w * (1.16f - slot * 0.055f));
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
                    const float lead = ease::Clamp01(w * (1.05f - slot * 0.06f));
                    local.scale = r.local.scale * (0.03f + 0.97f * lead);
                    local.euler_deg =
                        r.local.euler_deg +
                        Vec3{0.0f, 0.0f,
                             7.0f * std::sin(t * 2.1f + slot) * lead};
                    break;
                }
                case FutureKind::Stain: {
                    const float spread = ease::Clamp01(w * (1.0f - slot * 0.14f));
                    local.scale = {r.local.scale.x * spread, r.local.scale.y,
                                   r.local.scale.z * spread};
                    local.euler_deg.y = r.local.euler_deg.y + 9.0f * spread;
                    break;
                }
                case FutureKind::Shard: {
                    const float scatter = ease::Clamp01(w * (1.08f - slot * 0.04f));
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
                    const float lead = ease::Clamp01(w * (1.18f - slot * 0.075f));
                    const Vec3 origin =
                        kFutureAnchors[static_cast<std::size_t>(r.target)];
                    const Vec3 tucked =
                        origin + Vec3{0.06f * slot, -0.27f, 0.08f};
                    local.position = tucked + (r.local.position - tucked) * lead;
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
                    const float open = ease::Clamp01(w * (1.10f - r.slot * 0.055f));
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
        }
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
        const float beat = t * kDanceBeat;
        const float bounce = 0.5f + 0.5f * std::sin(beat * 2.0f);
        if (tf_ != nullptr && tf_->Contains(robonaut_)) {
            Transform local = tf_->Local(robonaut_);
            local.position =
                robonaut_rest_position_ + Vec3{0.020f * std::sin(beat * 0.5f),
                                               0.055f * bounce,
                                               0.030f * std::sin(beat + 0.65f)};
            local.euler_deg.x =
                robonaut_rest_euler_.x + 5.0f * std::sin(beat + 0.6f);
            local.euler_deg.z =
                robonaut_rest_euler_.z + 7.0f * std::sin(beat * 0.5f);
            tf_->SetLocal(robonaut_, local);
        }
        for (const DanceRest& joint : robonaut_dance_joints_) {
            Vec3 euler{};
            Vec3 offset{};
            if (joint.role == "center") {
                offset = {0.0f, 0.035f * std::abs(Wave(t)), 0.0f};
                euler = {4.0f * Wave(t, 1.2f), 0.0f, 10.0f * Snap(t)};
            }
            else if (joint.role == "lower_body") {
                euler = {9.0f * Wave(t, 1.1f), 12.0f * Wave(t, 0.2f),
                         15.0f * Wave(t, 2.0f)};
            }
            else if (joint.role == "upper_body") {
                euler = {20.0f * Wave(t, 2.2f), 12.0f * Wave(t, 0.9f),
                         26.0f * Snap(t, 0.0f)};
            }
            else if (joint.role == "head") {
                euler = {13.0f * Wave(t, 2.0f), 30.0f * Snap(t, 0.4f),
                         16.0f * Wave(t, 1.5f)};
            }
            else if (joint.role == "left_shoulder") {
                euler = {12.0f * Wave(t, 0.7f), 0.0f, 20.0f * Wave(t, 1.8f)};
            }
            else if (joint.role == "right_shoulder") {
                euler = {12.0f * Wave(t, 3.85f), 0.0f, -20.0f * Wave(t, 1.8f)};
            }
            else if (joint.role == "left_arm") {
                euler = {55.0f * Wave(t, 0.0f) - 18.0f, 30.0f * Wave(t, 1.1f),
                         92.0f * Snap(t, 0.35f)};
            }
            else if (joint.role == "right_arm") {
                euler = {55.0f * Wave(t, kPi) - 18.0f, -30.0f * Wave(t, 1.1f),
                         -92.0f * Snap(t, 0.35f)};
            }
            else if (joint.role == "left_elbow") {
                euler = {68.0f + 32.0f * Wave(t, 1.2f), 10.0f * Wave(t, 2.0f),
                         28.0f * Snap(t, 2.7f)};
            }
            else if (joint.role == "right_elbow") {
                euler = {68.0f + 32.0f * Wave(t, 4.35f), -10.0f * Wave(t, 2.0f),
                         -28.0f * Snap(t, 2.7f)};
            }
            else if (joint.role == "left_wrist") {
                euler = {22.0f * Wave(t, 2.5f), 36.0f * Snap(t, 0.8f),
                         45.0f * Wave(t, 1.7f)};
            }
            else if (joint.role == "right_wrist") {
                euler = {22.0f * Wave(t, 5.65f), -36.0f * Snap(t, 0.8f),
                         -45.0f * Wave(t, 1.7f)};
            }
            else if (joint.role == "left_leg") {
                euler = {28.0f * Wave(t, 3.15f), 5.0f * Wave(t, 0.4f),
                         16.0f * Wave(t, 1.1f)};
            }
            else if (joint.role == "right_leg") {
                euler = {28.0f * Wave(t, 0.0f), -5.0f * Wave(t, 0.4f),
                         -16.0f * Wave(t, 1.1f)};
            }
            else if (joint.role == "left_knee") {
                euler = {44.0f + 26.0f * Wave(t, 0.4f), 0.0f,
                         9.0f * Wave(t, 2.0f)};
            }
            else if (joint.role == "right_knee") {
                euler = {44.0f + 26.0f * Wave(t, 3.55f), 0.0f,
                         -9.0f * Wave(t, 2.0f)};
            }
            else if (joint.role == "left_ankle") {
                euler = {-18.0f * Wave(t, 0.4f), 10.0f * Wave(t, 1.6f),
                         14.0f * Snap(t, 2.2f)};
            }
            else if (joint.role == "right_ankle") {
                euler = {-18.0f * Wave(t, 3.55f), -10.0f * Wave(t, 1.6f),
                         -14.0f * Snap(t, 2.2f)};
            }
            Transform local = joint.local;
            local.position = joint.local.position + offset;
            local.euler_deg = joint.local.euler_deg + euler;
            tf_->SetLocal(joint.handle, local);
        }
    });
}

void SceneAnimation::Update(float delta_seconds) {
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

    std::fprintf(stderr,
                 "[SceneAnimation] Robonaut procedural mesh dance enabled "
                 "(%zu mesh pivots).\n",
                 robonaut_dance_joints_.size());
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
