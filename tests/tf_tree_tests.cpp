#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <vector>

#include "future_gaze/math/quat.hpp"
#include "future_gaze/scene/tf_tree.hpp"

namespace
{

void Fail(std::string_view message) {
    std::fprintf(stderr, "tf_tree_tests: %.*s\n",
                 static_cast<int>(message.size()), message.data());
    std::exit(1);
}

void Require(bool value, std::string_view message) {
    if (!value) {
        Fail(message);
    }
}

void RequireNear(float actual, float expected, std::string_view message) {
    if (std::abs(actual - expected) > 0.0001f) {
        std::fprintf(
            stderr, "tf_tree_tests: %.*s (actual %.6f expected %.6f)\n",
            static_cast<int>(message.size()), message.data(), actual, expected);
        std::exit(1);
    }
}

void RequireNear(future_gaze::Vec3 actual, future_gaze::Vec3 expected,
                 std::string_view message) {
    RequireNear(actual.x, expected.x, message);
    RequireNear(actual.y, expected.y, message);
    RequireNear(actual.z, expected.z, message);
}

void TestLifecycleAndLookup() {
    future_gaze::TfTree tree;
    const future_gaze::TfHandle root = tree.CreateRoot("root");
    const future_gaze::TfHandle first = tree.CreateChild(root, "dup_item");
    const future_gaze::TfHandle second = tree.CreateChild(root, "dup_item");

    std::vector<future_gaze::TfHandle> matches;
    tree.CollectPrefix("dup", matches);
    Require(matches.size() == 2, "duplicate prefix collection count");
    Require(matches[0] == first && matches[1] == second,
            "duplicate prefix collection order");
    Require(tree.FindFirst("dup_item") == first, "find first duplicate");

    Require(tree.DestroySubtree(first), "destroy subtree succeeds");
    Require(!tree.Contains(first), "destroy invalidates old handle");
    const future_gaze::TfHandle replacement =
        tree.CreateChild(root, "replacement");
    Require(replacement.index == first.index, "destroyed slot is reused");
    Require(replacement.generation != first.generation,
            "reused slot receives a new generation");
    Require(!tree.Contains(first), "stale generation remains invalid");
}

void TestReparentRejectsCycles() {
    future_gaze::TfTree tree;
    const future_gaze::TfHandle root = tree.CreateRoot("root");
    const future_gaze::TfHandle a = tree.CreateChild(root, "a");
    const future_gaze::TfHandle b = tree.CreateChild(a, "b");

    Require(!tree.ReparentKeepLocal(a, b), "cycle reparent is rejected");
    Require(tree.Parent(a) == root, "cycle rejection keeps old parent");
}

void TestLocalMatrixOrder() {
    future_gaze::Transform local;
    local.position = {1.0f, 2.0f, 3.0f};
    local.euler_deg = {0.0f, 90.0f, 0.0f};
    local.scale = {2.0f, 3.0f, 4.0f};

    const future_gaze::Vec3 transformed =
        future_gaze::LocalMatrixFromTransform(local).TransformPoint(
            {1.0f, 0.0f, 0.0f});
    RequireNear(transformed, {1.0f, 2.0f, 1.0f},
                "local matrix order is T * Ry * Rx * Rz * S");
}

void TestDirtyWorldCache() {
    future_gaze::TfTree tree;
    const future_gaze::TfHandle root =
        tree.CreateRoot("root", future_gaze::Transform{{1.0f, 0.0f, 0.0f}});
    const future_gaze::TfHandle child = tree.CreateChild(
        root, "child", future_gaze::Transform{{0.0f, 0.0f, 2.0f}});

    RequireNear(tree.WorldPosition(child), {1.0f, 0.0f, 2.0f},
                "initial child world position");
    tree.SetPosition(root, {2.0f, 0.0f, 0.0f});
    RequireNear(tree.WorldPosition(child), {2.0f, 0.0f, 2.0f},
                "parent mutation invalidates child world cache");
}

void TestWorldVectorMatchesQuatConvention() {
    future_gaze::TfTree tree;
    future_gaze::Transform eye;
    eye.euler_deg = {-15.0f, 180.0f, 0.0f};
    const future_gaze::TfHandle handle = tree.CreateRoot("eye", eye);

    const future_gaze::Quat yaw = future_gaze::Quat::FromAxisAngle(
        {0.0f, 1.0f, 0.0f}, future_gaze::DegToRad(eye.euler_deg.y));
    const future_gaze::Quat pitch = future_gaze::Quat::FromAxisAngle(
        {1.0f, 0.0f, 0.0f}, future_gaze::DegToRad(eye.euler_deg.x));
    const future_gaze::Vec3 expected =
        future_gaze::Normalize((yaw * pitch).Rotate({0.0f, 0.0f, -1.0f}));
    const future_gaze::Vec3 actual =
        future_gaze::Normalize(tree.WorldVector(handle, {0.0f, 0.0f, -1.0f}));

    RequireNear(actual, expected, "world vector matches gaze quaternion");
}

}  // namespace

int main() {
    TestLifecycleAndLookup();
    TestReparentRejectsCycles();
    TestLocalMatrixOrder();
    TestDirtyWorldCache();
    TestWorldVectorMatchesQuatConvention();
    return 0;
}
