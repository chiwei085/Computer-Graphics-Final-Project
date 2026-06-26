#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <vector>

#include "future_gaze/math/mat4.hpp"
#include "future_gaze/math/vec3.hpp"

namespace future_gaze
{

struct Transform
{
    Vec3 position{};
    Vec3 euler_deg{};
    Vec3 scale{1.0f, 1.0f, 1.0f};
};

struct TfHandle
{
    std::uint32_t index = std::numeric_limits<std::uint32_t>::max();
    std::uint32_t generation = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept {
        return index != std::numeric_limits<std::uint32_t>::max() &&
               generation != 0;
    }

    [[nodiscard]] friend constexpr bool operator==(TfHandle lhs,
                                                   TfHandle rhs) noexcept {
        return lhs.index == rhs.index && lhs.generation == rhs.generation;
    }
    [[nodiscard]] friend constexpr bool operator!=(TfHandle lhs,
                                                   TfHandle rhs) noexcept {
        return !(lhs == rhs);
    }
};

class TfTree
{
public:
    [[nodiscard]] TfHandle CreateRoot(std::string name, Transform local = {});
    [[nodiscard]] TfHandle CreateChild(TfHandle parent, std::string name,
                                       Transform local = {});

    [[nodiscard]] bool DestroySubtree(TfHandle handle);
    [[nodiscard]] bool ReparentKeepLocal(TfHandle child, TfHandle parent);
    void Clear();

    bool SetLocal(TfHandle handle, Transform local);
    bool SetPosition(TfHandle handle, Vec3 position);
    bool SetEulerDeg(TfHandle handle, Vec3 euler_deg);
    bool SetScale(TfHandle handle, Vec3 scale);

    [[nodiscard]] bool Contains(TfHandle handle) const noexcept;
    [[nodiscard]] const Transform& Local(TfHandle handle) const;
    [[nodiscard]] const std::string& Name(TfHandle handle) const;
    [[nodiscard]] TfHandle Parent(TfHandle handle) const;
    [[nodiscard]] const Mat4& LocalMatrix(TfHandle handle) const;
    [[nodiscard]] const Mat4& WorldMatrix(TfHandle handle) const;
    [[nodiscard]] Vec3 WorldPosition(TfHandle handle) const;
    [[nodiscard]] Vec3 WorldVector(TfHandle handle, Vec3 local_vector) const;

    [[nodiscard]] TfHandle FindFirst(const std::string& name) const;
    void CollectPrefix(const std::string& prefix,
                       std::vector<TfHandle>& out) const;
    void ForEachDepthFirst(TfHandle root,
                           const std::function<void(TfHandle)>& fn) const;

private:
    struct Node
    {
        std::string name;
        Transform local{};
        TfHandle parent{};
        std::vector<TfHandle> children;
        std::uint32_t generation = 1;
        bool alive = false;
        mutable bool local_dirty = true;
        mutable bool world_dirty = true;
        mutable Mat4 local_matrix = Mat4::Identity();
        mutable Mat4 world_matrix = Mat4::Identity();
    };

    [[nodiscard]] TfHandle Allocate(std::string name, Transform local);
    [[nodiscard]] Node* MutableNode(TfHandle handle) noexcept;
    [[nodiscard]] const Node* NodeFor(TfHandle handle) const noexcept;
    [[nodiscard]] bool WouldCreateCycle(TfHandle child,
                                        TfHandle parent) const noexcept;
    void MarkLocalDirty(TfHandle handle);
    void MarkWorldDirty(TfHandle handle);
    void DestroySubtreeUnchecked(TfHandle handle);
    void RemoveChildLink(TfHandle parent, TfHandle child);
    void VisitDepthFirst(TfHandle handle,
                         const std::function<void(TfHandle)>& fn) const;
    void EnsureLocalMatrix(const Node& node) const;
    void EnsureWorldMatrix(TfHandle handle) const;

    std::vector<Node> nodes_;
    std::vector<std::uint32_t> free_list_;
    std::vector<TfHandle> roots_;
};

[[nodiscard]] Mat4 LocalMatrixFromTransform(const Transform& transform);

}  // namespace future_gaze
