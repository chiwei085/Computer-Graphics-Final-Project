#include "future_gaze/scene/scene_graph.hpp"

#include <functional>
#include <utility>

namespace future_gaze
{

SceneGraph::SceneGraph(std::string root_name) {
    const TfHandle root = tf_.CreateRoot(std::move(root_name));
    root_ = std::unique_ptr<SceneNode>(new SceneNode(*this, root));
}

TfHandle SceneGraph::Find(const std::string& name) const {
    return tf_.FindFirst(name);
}

void SceneGraph::Collect(const std::string& prefix,
                         std::vector<TfHandle>& out) const {
    tf_.CollectPrefix(prefix, out);
}

void SceneGraph::Draw() const {
    root_->Draw();
}

void SceneGraph::DrawShadow(TfHandle root, float min_world_y,
                            float max_world_y) const {
    const SceneNode* node = NodeFor(root);
    if (node != nullptr) {
        node->DrawShadow(min_world_y, max_world_y);
    }
}

SceneNode& SceneGraph::NewChild(SceneNode& parent, std::string name) {
    const TfHandle child_handle =
        tf_.CreateChild(parent.handle(), std::move(name));
    parent.children_.push_back(
        std::unique_ptr<SceneNode>(new SceneNode(*this, child_handle)));
    return *parent.children_.back();
}

const SceneNode* SceneGraph::NodeFor(TfHandle handle) const {
    if (!tf_.Contains(handle)) {
        return nullptr;
    }
    const std::function<const SceneNode*(const SceneNode&)> find =
        [&](const SceneNode& node) -> const SceneNode* {
        if (node.handle() == handle) {
            return &node;
        }
        for (const auto& child : node.children_) {
            if (const SceneNode* hit = find(*child)) {
                return hit;
            }
        }
        return nullptr;
    };
    return find(*root_);
}

}  // namespace future_gaze
