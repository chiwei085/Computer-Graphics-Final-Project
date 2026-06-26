#include "future_gaze/scene/tf_tree.hpp"

#include <algorithm>

namespace future_gaze
{

namespace
{
const Transform kEmptyTransform{};
const std::string kEmptyName;
const Mat4 kIdentityMatrix = Mat4::Identity();
}  // namespace

Mat4 LocalMatrixFromTransform(const Transform& transform) {
    return Mat4::Translation(transform.position) *
           Mat4::RotationY(DegToRad(transform.euler_deg.y)) *
           Mat4::RotationX(DegToRad(transform.euler_deg.x)) *
           Mat4::RotationZ(DegToRad(transform.euler_deg.z)) *
           Mat4::Scale(transform.scale);
}

TfHandle TfTree::CreateRoot(std::string name, Transform local) {
    TfHandle handle = Allocate(std::move(name), local);
    roots_.push_back(handle);
    return handle;
}

TfHandle TfTree::CreateChild(TfHandle parent, std::string name,
                             Transform local) {
    if (!Contains(parent)) {
        return {};
    }
    TfHandle handle = Allocate(std::move(name), local);
    Node* child = MutableNode(handle);
    child->parent = parent;
    MutableNode(parent)->children.push_back(handle);
    return handle;
}

bool TfTree::DestroySubtree(TfHandle handle) {
    if (!Contains(handle)) {
        return false;
    }
    const TfHandle parent = Parent(handle);
    if (Contains(parent)) {
        RemoveChildLink(parent, handle);
    }
    else {
        std::erase(roots_, handle);
    }
    DestroySubtreeUnchecked(handle);
    return true;
}

bool TfTree::ReparentKeepLocal(TfHandle child, TfHandle parent) {
    if (!Contains(child) || !Contains(parent) || child == parent ||
        WouldCreateCycle(child, parent)) {
        return false;
    }

    Node* child_node = MutableNode(child);
    if (Contains(child_node->parent)) {
        RemoveChildLink(child_node->parent, child);
    }
    else {
        std::erase(roots_, child);
    }

    child_node->parent = parent;
    MutableNode(parent)->children.push_back(child);
    MarkWorldDirty(child);
    return true;
}

void TfTree::Clear() {
    nodes_.clear();
    free_list_.clear();
    roots_.clear();
}

bool TfTree::SetLocal(TfHandle handle, Transform local) {
    Node* node = MutableNode(handle);
    if (node == nullptr) {
        return false;
    }
    node->local = local;
    MarkLocalDirty(handle);
    return true;
}

bool TfTree::SetPosition(TfHandle handle, Vec3 position) {
    Node* node = MutableNode(handle);
    if (node == nullptr) {
        return false;
    }
    node->local.position = position;
    MarkLocalDirty(handle);
    return true;
}

bool TfTree::SetEulerDeg(TfHandle handle, Vec3 euler_deg) {
    Node* node = MutableNode(handle);
    if (node == nullptr) {
        return false;
    }
    node->local.euler_deg = euler_deg;
    MarkLocalDirty(handle);
    return true;
}

bool TfTree::SetScale(TfHandle handle, Vec3 scale) {
    Node* node = MutableNode(handle);
    if (node == nullptr) {
        return false;
    }
    node->local.scale = scale;
    MarkLocalDirty(handle);
    return true;
}

bool TfTree::Contains(TfHandle handle) const noexcept {
    return NodeFor(handle) != nullptr;
}

const Transform& TfTree::Local(TfHandle handle) const {
    const Node* node = NodeFor(handle);
    return (node != nullptr) ? node->local : kEmptyTransform;
}

const std::string& TfTree::Name(TfHandle handle) const {
    const Node* node = NodeFor(handle);
    return (node != nullptr) ? node->name : kEmptyName;
}

TfHandle TfTree::Parent(TfHandle handle) const {
    const Node* node = NodeFor(handle);
    return (node != nullptr) ? node->parent : TfHandle{};
}

const Mat4& TfTree::LocalMatrix(TfHandle handle) const {
    const Node* node = NodeFor(handle);
    if (node == nullptr) {
        return kIdentityMatrix;
    }
    EnsureLocalMatrix(*node);
    return node->local_matrix;
}

const Mat4& TfTree::WorldMatrix(TfHandle handle) const {
    const Node* node = NodeFor(handle);
    if (node == nullptr) {
        return kIdentityMatrix;
    }
    EnsureWorldMatrix(handle);
    return node->world_matrix;
}

Vec3 TfTree::WorldPosition(TfHandle handle) const {
    return WorldMatrix(handle).TransformPoint({});
}

Vec3 TfTree::WorldVector(TfHandle handle, Vec3 local_vector) const {
    return WorldMatrix(handle).TransformVector(local_vector);
}

TfHandle TfTree::FindFirst(const std::string& name) const {
    TfHandle hit{};
    for (const TfHandle root : roots_) {
        VisitDepthFirst(root, [&](TfHandle handle) {
            if (!hit.IsValid() && Name(handle) == name) {
                hit = handle;
            }
        });
        if (hit.IsValid()) {
            break;
        }
    }
    return hit;
}

void TfTree::CollectPrefix(const std::string& prefix,
                           std::vector<TfHandle>& out) const {
    for (const TfHandle root : roots_) {
        VisitDepthFirst(root, [&](TfHandle handle) {
            if (Name(handle).rfind(prefix, 0) == 0) {
                out.push_back(handle);
            }
        });
    }
}

void TfTree::ForEachDepthFirst(TfHandle root,
                               const std::function<void(TfHandle)>& fn) const {
    VisitDepthFirst(root, fn);
}

TfHandle TfTree::Allocate(std::string name, Transform local) {
    std::uint32_t index = 0;
    if (!free_list_.empty()) {
        index = free_list_.back();
        free_list_.pop_back();
    }
    else {
        index = static_cast<std::uint32_t>(nodes_.size());
        nodes_.push_back({});
    }

    Node& node = nodes_[index];
    node.name = std::move(name);
    node.local = local;
    node.parent = {};
    node.children.clear();
    node.alive = true;
    node.local_dirty = true;
    node.world_dirty = true;
    if (node.generation == 0) {
        node.generation = 1;
    }
    return TfHandle{index, node.generation};
}

TfTree::Node* TfTree::MutableNode(TfHandle handle) noexcept {
    if (!handle.IsValid() || handle.index >= nodes_.size()) {
        return nullptr;
    }
    Node& node = nodes_[handle.index];
    if (!node.alive || node.generation != handle.generation) {
        return nullptr;
    }
    return &node;
}

const TfTree::Node* TfTree::NodeFor(TfHandle handle) const noexcept {
    if (!handle.IsValid() || handle.index >= nodes_.size()) {
        return nullptr;
    }
    const Node& node = nodes_[handle.index];
    if (!node.alive || node.generation != handle.generation) {
        return nullptr;
    }
    return &node;
}

bool TfTree::WouldCreateCycle(TfHandle child, TfHandle parent) const noexcept {
    for (TfHandle p = parent; Contains(p); p = Parent(p)) {
        if (p == child) {
            return true;
        }
    }
    return false;
}

void TfTree::MarkLocalDirty(TfHandle handle) {
    Node* node = MutableNode(handle);
    if (node == nullptr) {
        return;
    }
    node->local_dirty = true;
    MarkWorldDirty(handle);
}

void TfTree::MarkWorldDirty(TfHandle handle) {
    Node* node = MutableNode(handle);
    if (node == nullptr) {
        return;
    }
    node->world_dirty = true;
    for (const TfHandle child : node->children) {
        MarkWorldDirty(child);
    }
}

void TfTree::DestroySubtreeUnchecked(TfHandle handle) {
    Node* node = MutableNode(handle);
    if (node == nullptr) {
        return;
    }
    const std::vector<TfHandle> children = node->children;
    for (const TfHandle child : children) {
        DestroySubtreeUnchecked(child);
    }

    node->name.clear();
    node->children.clear();
    node->parent = {};
    node->alive = false;
    ++node->generation;
    if (node->generation == 0) {
        node->generation = 1;
    }
    free_list_.push_back(handle.index);
}

void TfTree::RemoveChildLink(TfHandle parent, TfHandle child) {
    Node* parent_node = MutableNode(parent);
    if (parent_node == nullptr) {
        return;
    }
    std::erase(parent_node->children, child);
}

void TfTree::VisitDepthFirst(TfHandle handle,
                             const std::function<void(TfHandle)>& fn) const {
    const Node* node = NodeFor(handle);
    if (node == nullptr) {
        return;
    }
    fn(handle);
    for (const TfHandle child : node->children) {
        VisitDepthFirst(child, fn);
    }
}

void TfTree::EnsureLocalMatrix(const Node& node) const {
    if (!node.local_dirty) {
        return;
    }
    node.local_matrix = LocalMatrixFromTransform(node.local);
    node.local_dirty = false;
}

void TfTree::EnsureWorldMatrix(TfHandle handle) const {
    const Node* node = NodeFor(handle);
    if (node == nullptr || !node->world_dirty) {
        return;
    }
    EnsureLocalMatrix(*node);
    if (Contains(node->parent)) {
        EnsureWorldMatrix(node->parent);
        node->world_matrix = WorldMatrix(node->parent) * node->local_matrix;
    }
    else {
        node->world_matrix = node->local_matrix;
    }
    node->world_dirty = false;
}

}  // namespace future_gaze
