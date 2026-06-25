#include "future_gaze/scene/scene_node.hpp"

#include <GL/freeglut.h>

#include "future_gaze/render/scoped_gl.hpp"

namespace future_gaze
{

SceneNode::SceneNode(std::string name) : name_(std::move(name)) {}

SceneNode* SceneNode::AddChild(std::unique_ptr<SceneNode> child) {
    children_.push_back(std::move(child));
    return children_.back().get();
}

SceneNode& SceneNode::NewChild(std::string name) {
    return *AddChild(std::make_unique<SceneNode>(std::move(name)));
}

void SceneNode::Draw() const {
    const ScopedMatrixPush push;

    glTranslatef(position.x, position.y, position.z);
    if (euler_deg.y != 0.0f) {
        glRotatef(euler_deg.y, 0.0f, 1.0f, 0.0f);
    }
    if (euler_deg.x != 0.0f) {
        glRotatef(euler_deg.x, 1.0f, 0.0f, 0.0f);
    }
    if (euler_deg.z != 0.0f) {
        glRotatef(euler_deg.z, 0.0f, 0.0f, 1.0f);
    }
    if (scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f) {
        glScalef(scale.x, scale.y, scale.z);
    }

    if (renderable.has_value()) {
        renderable->Draw();
    }

    for (const auto& child : children_) {
        child->Draw();
    }
}

}  // namespace future_gaze
