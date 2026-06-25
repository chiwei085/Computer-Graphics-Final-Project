#pragma once

#include <vector>

#include "future_gaze/anim/animator.hpp"
#include "future_gaze/math/vec3.hpp"
#include "future_gaze/scene/scene_node.hpp"

namespace future_gaze
{

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
    void Bind(SceneNode& root);
    void Update(float delta_seconds);

private:
    // Per-node rest pose captured at Bind time so channels animate as offsets.
    struct Rest
    {
        SceneNode* node = nullptr;
        Vec3 position;
        Vec3 euler;
        Vec3 scale;
    };

    Animator animator_;

    // Self-rotating iris ring + the eye it lives on.
    SceneNode* circuit_ring_ = nullptr;
    SceneNode* eye_root_ = nullptr;
    Vec3 eye_base_euler_;

    // Orbiting drone + the character that tracks it.
    SceneNode* ingenuity_ = nullptr;
    SceneNode* robonaut_ = nullptr;
    Vec3 eye_center_;                // world-space orbit centre
    float robonaut_yaw_vel_ = 0.0f;  // SmoothDamp state
    Vec3 ingenuity_world_;           // updated each frame for the tracker

    // Blooming future slices + unfolding memory panels.
    std::vector<Rest> future_slices_;
    std::vector<Rest> memory_panels_;
    KeyTrack unfold_;  // shared keyframed open/close curve for the panels

    void BuildChannels();
};

}  // namespace future_gaze
