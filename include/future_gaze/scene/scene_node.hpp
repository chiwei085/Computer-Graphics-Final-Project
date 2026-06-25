#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "future_gaze/math/vec3.hpp"
#include "future_gaze/render/renderable.hpp"
#include "future_gaze/render/texture.hpp"

namespace future_gaze
{

class SceneNode
{
public:
    explicit SceneNode(std::string name = {});

    // Local-space transform (applied order: Ry → Rx → Rz)
    Vec3 position{};
    Vec3 euler_deg{};
    Vec3 scale{1.0f, 1.0f, 1.0f};

    // Optional geometry + material bound as one unit
    std::optional<Renderable> renderable;

    // ── Tree ─────────────────────────────────────────────────────────────────
    SceneNode* AddChild(std::unique_ptr<SceneNode> child);
    SceneNode& NewChild(std::string name = {});

    // ── Fluent setters (return *this for chaining)
    // ────────────────────────────
    SceneNode& at(Vec3 pos) {
        position = pos;
        return *this;
    }
    SceneNode& rot_y(float deg) {
        euler_deg.y = deg;
        return *this;
    }
    SceneNode& rot_x(float deg) {
        euler_deg.x = deg;
        return *this;
    }
    SceneNode& rot_z(float deg) {
        euler_deg.z = deg;
        return *this;
    }
    SceneNode& scaled(Vec3 s) {
        scale = s;
        return *this;
    }
    SceneNode& draw_as(Mesh m, Material mat) {
        renderable.emplace(Renderable{std::move(m), std::move(mat)});
        return *this;
    }
    SceneNode& draw_as(Mesh m, Material mat, const Texture* tex) {
        renderable.emplace(Renderable{std::move(m), std::move(mat), tex});
        return *this;
    }
    // Unlit additive halo (fake emissive glow, no shader). See Renderable.
    SceneNode& draw_glow(Mesh m, Material mat) {
        renderable.emplace(
            Renderable{std::move(m), std::move(mat), nullptr, true});
        return *this;
    }

    // Pushes transform, draws renderable (if any), recurses children, pops
    void Draw() const;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    // First descendant (DFS, self included) whose name equals `name`, or null.
    [[nodiscard]] SceneNode* Find(const std::string& name);

    // Appends every descendant (DFS, self included) whose name starts with
    // `prefix` to `out`. Used to gather animation targets in scene order.
    void Collect(const std::string& prefix, std::vector<SceneNode*>& out);

private:
    std::string name_;
    std::vector<std::unique_ptr<SceneNode>> children_;
};

}  // namespace future_gaze
