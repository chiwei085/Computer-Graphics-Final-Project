#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "future_gaze/render/renderable.hpp"
#include "future_gaze/scene/tf_tree.hpp"

namespace future_gaze
{

class SceneGraph;

class SceneNode
{
public:
    SceneNode& NewChild(std::string name = {});

    // ── Fluent setters (return *this for chaining)
    // ────────────────────────────
    SceneNode& at(Vec3 pos) {
        Transform local = Local();
        local.position = pos;
        SetLocal(local);
        return *this;
    }
    SceneNode& rot_y(float deg) {
        Transform local = Local();
        local.euler_deg.y = deg;
        SetLocal(local);
        return *this;
    }
    SceneNode& rot_x(float deg) {
        Transform local = Local();
        local.euler_deg.x = deg;
        SetLocal(local);
        return *this;
    }
    SceneNode& rot_z(float deg) {
        Transform local = Local();
        local.euler_deg.z = deg;
        SetLocal(local);
        return *this;
    }
    SceneNode& scaled(Vec3 s) {
        Transform local = Local();
        local.scale = s;
        SetLocal(local);
        return *this;
    }
    SceneNode& draw_as(Mesh m, Material mat) {
        renderable_.emplace(Renderable{std::move(m), std::move(mat)});
        return *this;
    }
    SceneNode& draw_as(Mesh m, Material mat, const Texture* tex) {
        renderable_.emplace(Renderable{std::move(m), std::move(mat), tex});
        return *this;
    }
    // Unlit additive halo (fake emissive glow, no shader). See Renderable.
    SceneNode& draw_glow(Mesh m, Material mat) {
        renderable_.emplace(
            Renderable{std::move(m), std::move(mat), nullptr, true});
        return *this;
    }

    // Pushes transform, draws renderable (if any), recurses children, pops
    void Draw() const;
    void DrawShadow(float min_world_y, float max_world_y) const;

    [[nodiscard]] const std::string& name() const;
    [[nodiscard]] TfHandle handle() const noexcept { return handle_; }
    [[nodiscard]] Transform Local() const;
    void SetLocal(Transform local);

private:
    friend class SceneGraph;

    SceneNode(SceneGraph& graph, TfHandle handle);

    SceneGraph* graph_ = nullptr;
    TfHandle handle_{};
    std::optional<Renderable> renderable_;
    std::vector<std::unique_ptr<SceneNode>> children_;
};

}  // namespace future_gaze
