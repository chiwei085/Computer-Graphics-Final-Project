#include "future_gaze/scene/scene_graph.hpp"

#include <utility>

namespace future_gaze
{

SceneNode::SceneNode(SceneGraph& graph, TfHandle handle)
    : graph_(&graph), handle_(handle) {}

SceneNode& SceneNode::NewChild(std::string name) {
    return graph_->NewChild(*this, std::move(name));
}

const std::string& SceneNode::name() const {
    return graph_->tf().Name(handle_);
}

Transform SceneNode::Local() const {
    return graph_->tf().Local(handle_);
}

void SceneNode::SetLocal(Transform local) {
    graph_->tf().SetLocal(handle_, local);
}

void SceneNode::Draw() const {
    const ScopedMatrixPush push;
    glMultMatrixf(graph_->tf().LocalMatrix(handle_).Data());

    if (renderable_.has_value()) {
        renderable_->Draw();
    }

    for (const auto& child : children_) {
        child->Draw();
    }
}

void SceneNode::DrawShadow(float min_world_y, float max_world_y) const {
    const float world_y = graph_->tf().WorldPosition(handle_).y;
    const ScopedMatrixPush push;
    glMultMatrixf(graph_->tf().LocalMatrix(handle_).Data());

    if (renderable_.has_value() && world_y >= min_world_y &&
        world_y <= max_world_y) {
        renderable_->DrawShadow();
    }

    for (const auto& child : children_) {
        child->DrawShadow(min_world_y, max_world_y);
    }
}

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

const SceneNode* SceneGraph::FindInTree(const SceneNode& node,
                                        TfHandle handle) noexcept {
    if (node.handle() == handle) {
        return &node;
    }
    for (const auto& child : node.children_) {
        if (const SceneNode* hit = FindInTree(*child, handle)) {
            return hit;
        }
    }
    return nullptr;
}

const SceneNode* SceneGraph::NodeFor(TfHandle handle) const {
    if (!tf_.Contains(handle)) {
        return nullptr;
    }
    return FindInTree(*root_, handle);
}

}  // namespace future_gaze
