#pragma once

#include <functional>
#include <string>
#include <vector>

#include "future_gaze/anim/easing.hpp"

namespace future_gaze
{

// Piecewise keyframe curve sampled at arbitrary time with per-segment easing.
class KeyTrack
{
public:
    using Easing = float (*)(float);

    struct Key
    {
        float time = 0.0f;
        float value = 0.0f;
        Easing ease = ease::SmoothStep;  // applied to the segment ending at this key
    };

    KeyTrack& Add(float time, float value, Easing ease = ease::SmoothStep) {
        keys_.push_back({time, value, ease});
        return *this;
    }

    // Sample with the track held (clamped) outside its time range.
    [[nodiscard]] float Sample(float t) const {
        if (keys_.empty()) {
            return 0.0f;
        }
        if (t <= keys_.front().time) {
            return keys_.front().value;
        }
        if (t >= keys_.back().time) {
            return keys_.back().value;
        }
        for (std::size_t i = 1; i < keys_.size(); ++i) {
            const Key& b = keys_[i];
            if (t <= b.time) {
                const Key& a = keys_[i - 1];
                const float span = b.time - a.time;
                const float local = span > 0.0f ? (t - a.time) / span : 1.0f;
                return ease::Lerp(a.value, b.value, b.ease(local));
            }
        }
        return keys_.back().value;
    }

    [[nodiscard]] float Duration() const {
        return keys_.empty() ? 0.0f : keys_.back().time;
    }

private:
    std::vector<Key> keys_;
};

// Owns a master clock and named per-frame update channels.
class Animator
{
public:
    // (clock_seconds, delta_seconds)
    using Channel = std::function<void(float, float)>;

    void Add(std::string name, Channel fn, bool enabled = true) {
        channels_.push_back({std::move(name), std::move(fn), enabled});
    }

    void SetEnabled(const std::string& name, bool enabled) {
        for (auto& c : channels_) {
            if (c.name == name) {
                c.enabled = enabled;
            }
        }
    }

    void Update(float delta_seconds) {
        clock_ += delta_seconds;
        for (const auto& c : channels_) {
            if (c.enabled) {
                c.fn(clock_, delta_seconds);
            }
        }
    }

    [[nodiscard]] float clock() const noexcept { return clock_; }

private:
    struct Entry
    {
        std::string name;
        Channel fn;
        bool enabled = true;
    };

    std::vector<Entry> channels_;
    float clock_ = 0.0f;
};

}  // namespace future_gaze
