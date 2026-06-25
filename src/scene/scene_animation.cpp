#include "future_gaze/scene/scene_animation.hpp"

#include <cmath>
#include <cstdio>
#include <numbers>

#include "future_gaze/anim/easing.hpp"
#include "future_gaze/scene/node_names.hpp"

namespace future_gaze
{

namespace
{
constexpr float kPi = std::numbers::pi_v<float>;
constexpr float kRadToDeg = 180.0f / kPi;

// Binding is name-based, so a missing target is silent until something visibly
// fails to move. These helpers turn that into a loud one-line warning at
// startup (captured on stderr) instead of a no-op channel.
SceneNode* FindOrWarn(SceneNode& root, const char* name) {
    SceneNode* node = root.Find(name);
    if (node == nullptr) {
        std::fprintf(stderr,
                     "[SceneAnimation] node '%s' not found — its animation is "
                     "disabled.\n",
                     name);
    }
    return node;
}

std::vector<SceneNode*> CollectOrWarn(SceneNode& root, const char* prefix) {
    std::vector<SceneNode*> out;
    root.Collect(prefix, out);
    if (out.empty()) {
        std::fprintf(stderr,
                     "[SceneAnimation] no nodes matching '%s' — its animation "
                     "is disabled.\n",
                     prefix);
    }
    return out;
}
}  // namespace

void SceneAnimation::Bind(SceneNode& root) {
    // ── Resolve animation targets ────────────────────────────────────────────
    eye_root_ = FindOrWarn(root, names::kPredictionCore);
    circuit_ring_ = FindOrWarn(root, names::kCircuitRing);
    ingenuity_ = FindOrWarn(root, names::kIngenuity);
    robonaut_ = FindOrWarn(root, names::kRobonaut);

    if (eye_root_ != nullptr) {
        eye_base_euler_ = eye_root_->euler_deg;
        eye_center_ = eye_root_->position;
    }

    // Gather the bloom slices and memory panels, snapshotting their rest pose.
    for (SceneNode* n : CollectOrWarn(root, names::kFutureSlice)) {
        future_slices_.push_back({n, n->position, n->euler_deg, n->scale});
    }
    for (SceneNode* n : CollectOrWarn(root, names::kMemoryPanel)) {
        memory_panels_.push_back({n, n->position, n->euler_deg, n->scale});
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
        if (circuit_ring_ != nullptr) {
            circuit_ring_->euler_deg.y = std::fmod(t * 42.0f, 360.0f);
        }
    });

    // ── Ambient gaze idle
    // ───────────────────────────────────────────────────── A slow, eased
    // scanning drift so the eye feels alive, not frozen.
    animator_.Add("eye_idle", [this](float t, float) {
        if (eye_root_ == nullptr) {
            return;
        }
        eye_root_->euler_deg.y = eye_base_euler_.y + 6.0f * std::sin(t * 0.28f);
        eye_root_->euler_deg.x =
            eye_base_euler_.x + 3.0f * std::sin(t * 0.19f + 1.3f);
    });

    // ── Orbit motion
    // ────────────────────────────────────────────────────────── Ingenuity
    // rides a cosine/sine orbit around the eye with a vertical bob and banks
    // into the turn. Also publishes its world position for the tracker.
    animator_.Add("orbit_drone", [this](float t, float) {
        constexpr float kRadius = 2.35f;
        constexpr float kOmega = 0.55f;  // rad/s
        const float a = t * kOmega;
        const float bob = 0.40f * std::sin(a * 2.0f);
        ingenuity_world_ = {eye_center_.x + kRadius * std::cos(a),
                            eye_center_.y + bob,
                            eye_center_.z + kRadius * std::sin(a)};
        if (ingenuity_ != nullptr) {
            ingenuity_->position = ingenuity_world_;
            // Face along the tangent (heading) and bank with the bob.
            ingenuity_->euler_deg.y = -a * kRadToDeg + 90.0f;
            ingenuity_->euler_deg.z = 14.0f * std::cos(a * 2.0f);
            ingenuity_->euler_deg.x = 6.0f * std::sin(a * 2.0f);
        }
    });

    // ── Future-state bloom
    // ──────────────────────────────────────────────────── Each slice fans open
    // with a staggered phase; the lowest (most probable) slice leads and sits
    // thickest, upper slices trail into thinner futures.
    animator_.Add("future_bloom", [this](float t, float) {
        constexpr int kPerStack = 5;
        constexpr float kPeriod = 7.0f;
        for (std::size_t i = 0; i < future_slices_.size(); ++i) {
            const Rest& r = future_slices_[i];
            const int slot = static_cast<int>(i) % kPerStack;
            const float phase = t / kPeriod - static_cast<float>(slot) * 0.08f;
            // Raised-cosine breathing that never collapses to nothing, so the
            // additive glow pulses smoothly instead of blinking on/off.
            const float open = 0.5f + 0.5f * ease::PingPongSine(phase);
            const float lift = 0.075f * static_cast<float>(slot);
            const float grow = 0.45f + 0.55f * open;
            r.node->position.y = r.position.y + lift * open;
            r.node->scale = {r.scale.x * grow, r.scale.y, r.scale.z * grow};
            // Continuous slow spin keeps the slices visibly alive every frame.
            r.node->euler_deg.y =
                r.euler.y + (t * 28.0f + static_cast<float>(slot) * 22.0f);
        }
    });

    // ── Memory panels (keyframed)
    // ───────────────────────────────────────────── Each panel loops through
    // the shared unfold KeyTrack with its own phase, and yaws slowly so
    // different diners see a different private front.
    animator_.Add("memory_unfold", [this](float t, float) {
        const float period = unfold_.Duration() + 1.6f;
        for (std::size_t i = 0; i < memory_panels_.size(); ++i) {
            const Rest& r = memory_panels_[i];
            const float local =
                std::fmod(t + static_cast<float>(i) * period / 3.0f, period);
            const float open = unfold_.Sample(local);
            // Rise from the chair back: anchor the bottom edge (the cube is
            // centred, so lift the centre by half the grown height) instead of
            // pulsing symmetrically about the middle.
            constexpr float kHeight = 0.42f;
            r.node->scale.y = kHeight * open;
            r.node->position.y = r.position.y + 0.5f * kHeight * open;
            r.node->euler_deg.y =
                r.euler.y +
                12.0f * std::sin(t * 0.4f + static_cast<float>(i) * 2.1f) *
                    open;
        }
    });

    // ── Gaze tracking
    // ───────────────────────────────────────────────────────── Robonaut
    // smoothly (critically damped) turns to face the orbiting drone. Runs after
    // orbit_drone so it reads the drone's freshly-updated position.
    animator_.Add("robonaut_track", [this](float, float dt) {
        if (robonaut_ == nullptr) {
            return;
        }
        const Vec3 to = ingenuity_world_ - robonaut_->position;
        // Model forward is offset by its rest yaw; aim that forward at the
        // drone.
        const float target_yaw = std::atan2(to.x, to.z) * kRadToDeg + 180.0f;
        robonaut_->euler_deg.y = ease::SmoothDampAngleDeg(
            robonaut_->euler_deg.y, target_yaw, robonaut_yaw_vel_, 0.7f,
            std::max(dt, 0.0001f));
    });
}

void SceneAnimation::Update(float delta_seconds) {
    animator_.Update(delta_seconds);
}

}  // namespace future_gaze
