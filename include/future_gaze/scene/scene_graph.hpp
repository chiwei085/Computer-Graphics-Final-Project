#pragma once

#include <memory>
#include <string>
#include <vector>

#include "future_gaze/scene/scene_node.hpp"
#include "future_gaze/scene/tf_tree.hpp"

namespace future_gaze
{

class SceneGraph
{
public:
    explicit SceneGraph(std::string root_name = "root");

    [[nodiscard]] SceneNode& Root() noexcept { return *root_; }
    [[nodiscard]] const SceneNode& Root() const noexcept { return *root_; }
    [[nodiscard]] TfTree& tf() noexcept { return tf_; }
    [[nodiscard]] const TfTree& tf() const noexcept { return tf_; }

    [[nodiscard]] TfHandle Find(const std::string& name) const;
    void Collect(const std::string& prefix, std::vector<TfHandle>& out) const;

    void Draw() const;
    void DrawShadow(TfHandle root, float min_world_y, float max_world_y) const;

private:
    friend class SceneNode;

    [[nodiscard]] SceneNode& NewChild(SceneNode& parent, std::string name);
    [[nodiscard]] const SceneNode* NodeFor(TfHandle handle) const;

    TfTree tf_;
    std::unique_ptr<SceneNode> root_;
};

}  // namespace future_gaze
