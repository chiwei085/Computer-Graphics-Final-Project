#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace future_gaze::ease
{

[[nodiscard]] constexpr float Clamp01(float t) noexcept {
    return std::clamp(t, 0.0f, 1.0f);
}

[[nodiscard]] constexpr float Lerp(float a, float b, float t) noexcept {
    return a + (b - a) * t;
}

// Hermite smoothstep — zero first-derivative at both ends (no velocity jump).
[[nodiscard]] constexpr float SmoothStep(float t) noexcept {
    t = Clamp01(t);
    return t * t * (3.0f - 2.0f * t);
}

// Quintic smootherstep — zero first and second derivatives at both ends.
[[nodiscard]] constexpr float SmootherStep(float t) noexcept {
    t = Clamp01(t);
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

[[nodiscard]] inline float InOutSine(float t) noexcept {
    t = Clamp01(t);
    return -0.5f * (std::cos(std::numbers::pi_v<float> * t) - 1.0f);
}

[[nodiscard]] inline float InOutCubic(float t) noexcept {
    t = Clamp01(t);
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    }
    const float f = -2.0f * t + 2.0f;
    return 1.0f - f * f * f * 0.5f;
}

// Slight overshoot then settle.
[[nodiscard]] inline float OutBack(float t,
                                   float overshoot = 1.70158f) noexcept {
    t = Clamp01(t) - 1.0f;
    const float c3 = overshoot + 1.0f;
    return 1.0f + c3 * t * t * t + overshoot * t * t;
}

// Single-argument adaptor so OutBack can be used as a KeyTrack easing pointer.
[[nodiscard]] inline float OutBackEase(float t) noexcept {
    return OutBack(t);
}

// Symmetric 0→1→0 ramp with eased turn-around.
[[nodiscard]] inline float PingPongSine(float phase01) noexcept {
    const float t = phase01 - std::floor(phase01);
    return 0.5f * (1.0f - std::cos(2.0f * std::numbers::pi_v<float> * t));
}

// Critically damped follower, frame-rate independent. velocity is in/out state.
[[nodiscard]] inline float SmoothDamp(
    float current, float target, float& velocity, float smooth_time, float dt,
    float max_speed = std::numeric_limits<float>::infinity()) noexcept {
    smooth_time = std::max(0.0001f, smooth_time);
    const float omega = 2.0f / smooth_time;
    const float x = omega * dt;
    const float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    float change = current - target;
    const float max_change = max_speed * smooth_time;
    change = std::clamp(change, -max_change, max_change);
    const float temp = (velocity + omega * change) * dt;
    velocity = (velocity - omega * temp) * exp;
    float result = (current - change) + (change + temp) * exp;
    if ((target - current > 0.0f) == (result > target)) {
        result = target;
        velocity = (result - target) / dt;
    }
    return result;
}

// Shortest-arc SmoothDamp in degrees; wraps error into [-180, 180].
[[nodiscard]] inline float SmoothDampAngleDeg(float current_deg,
                                              float target_deg, float& velocity,
                                              float smooth_time,
                                              float dt) noexcept {
    float delta = std::fmod(target_deg - current_deg, 360.0f);
    if (delta > 180.0f) {
        delta -= 360.0f;
    }
    else if (delta < -180.0f) {
        delta += 360.0f;
    }
    return SmoothDamp(current_deg, current_deg + delta, velocity, smooth_time,
                      dt);
}

}  // namespace future_gaze::ease
