#include "future_gaze/scene/scene_node.hpp"

#include <GL/freeglut.h>

#include "future_gaze/render/scoped_gl.hpp"
#include "future_gaze/scene/scene_graph.hpp"

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

}  // namespace future_gaze
