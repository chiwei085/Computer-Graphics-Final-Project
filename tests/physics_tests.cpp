#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string_view>

#include "future_gaze/physics/physics_world.hpp"

namespace
{

using future_gaze::Vec3;
using future_gaze::physics::BodyId;
using future_gaze::physics::Collider2D;
using future_gaze::physics::HeightRange;
using future_gaze::physics::MoveResult;
using future_gaze::physics::PhysicsWorld;
using future_gaze::physics::Transform2D;

void Fail(std::string_view message) {
    std::fprintf(stderr, "physics_tests: %.*s\n",
                 static_cast<int>(message.size()), message.data());
    std::exit(1);
}

void Require(bool value, std::string_view message) {
    if (!value) {
        Fail(message);
    }
}

void RequireNear(float actual, float expected, float tolerance,
                 std::string_view message) {
    if (std::abs(actual - expected) > tolerance) {
        std::fprintf(
            stderr,
            "physics_tests: %.*s (actual %.6f expected %.6f tolerance %.6f)\n",
            static_cast<int>(message.size()), message.data(), actual, expected,
            tolerance);
        std::exit(1);
    }
}

void TestSweptMovementStopsBeforeThinBox() {
    PhysicsWorld world;
    const BodyId blocker = world.AddStatic(
        Collider2D::Box(0.05f, 0.50f), Transform2D{{0.0f, 0.0f, 0.0f}, 0.0f},
        HeightRange{0.0f, 2.0f});
    Require(blocker.IsValid(), "thin box blocker is valid");
    const BodyId mover =
        world.AddKinematic(Collider2D::Circle(0.20f), {-2.0f, 0.0f, 0.0f},
                           HeightRange{0.0f, 1.8f});

    const MoveResult result =
        world.MoveKinematic(mover, {4.0f, 0.0f, 0.0f}, 0.5f);

    Require(result.hit_count > 0, "thin box sweep registers a hit");
    Require(result.final_position.x <= -0.249f,
            "swept movement stops before inflated thin box");
}

void TestSlidingPreservesTangentialMovement() {
    PhysicsWorld world;
    const BodyId blocker = world.AddStatic(
        Collider2D::Box(0.50f, 0.50f), Transform2D{{0.0f, 0.0f, 0.0f}, 0.0f},
        HeightRange{0.0f, 2.0f});
    Require(blocker.IsValid(), "slide blocker is valid");
    const BodyId mover =
        world.AddKinematic(Collider2D::Circle(0.20f), {-1.40f, 0.0f, -0.70f},
                           HeightRange{0.0f, 1.8f});

    const MoveResult result =
        world.MoveKinematic(mover, {1.20f, 0.0f, 0.40f}, 0.5f);

    Require(result.hit_count > 0, "slide test collides with box side");
    Require(result.final_position.z > -0.62f,
            "slide keeps the tangential z movement");
    Require(result.final_position.x <= -0.699f,
            "slide keeps the mover outside the box side");
}

void TestDepenetrationResolvesInitialOverlap() {
    PhysicsWorld world;
    const BodyId blocker = world.AddStatic(
        Collider2D::Circle(0.50f), Transform2D{{0.0f, 0.0f, 0.0f}, 0.0f},
        HeightRange{0.0f, 2.0f});
    Require(blocker.IsValid(), "depenetration blocker is valid");
    const BodyId mover =
        world.AddKinematic(Collider2D::Circle(0.25f), {0.10f, 0.0f, 0.0f},
                           HeightRange{0.0f, 1.8f});

    const MoveResult result = world.MoveKinematic(mover, {}, 0.016f);

    Require(!result.contacts.empty(), "initial overlap creates contact");
    Require(result.final_position.x >= 0.751f,
            "depenetration moves circle outside overlap");
}

void TestHeightRangesFilterAerialSensors() {
    PhysicsWorld world;
    const BodyId blocker = world.AddStatic(
        Collider2D::Box(1.0f, 1.0f), Transform2D{{0.0f, 0.0f, 0.0f}, 0.0f},
        HeightRange{0.0f, 0.90f});
    Require(blocker.IsValid(), "aerial-height blocker is valid");
    const BodyId aerial =
        world.AddSensor(Collider2D::Circle(0.35f), {0.0f, 3.0f, 0.0f},
                        HeightRange{2.70f, 3.30f});

    Require(world.QueryOverlaps(aerial).empty(),
            "aerial sensor ignores table-height blocker");
}

void TestCircleSweepDoesNotTunnelThroughChair() {
    PhysicsWorld world;
    const BodyId blocker = world.AddStatic(
        Collider2D::Circle(0.40f), Transform2D{{0.0f, 0.0f, 0.0f}, 0.0f},
        HeightRange{0.0f, 1.2f});
    Require(blocker.IsValid(), "chair blocker is valid");
    const BodyId mover =
        world.AddKinematic(Collider2D::Circle(0.30f), {-2.0f, 0.0f, 0.0f},
                           HeightRange{0.0f, 1.2f});

    const MoveResult result =
        world.MoveKinematic(mover, {4.0f, 0.0f, 0.0f}, 0.5f);

    Require(result.hit_count > 0, "circle sweep registers chair hit");
    Require(result.final_position.x <= -0.699f,
            "circle sweep stops before chair radius");
}

void TestTransformedStaticColliderMovesWithRoot() {
    PhysicsWorld world;
    const BodyId table_part = world.AddStatic(
        Collider2D::Box(0.25f, 0.25f), Transform2D{{1.0f, 0.0f, 0.0f}, 0.0f},
        HeightRange{0.0f, 1.0f});
    const BodyId mover =
        world.AddKinematic(Collider2D::Circle(0.20f), {-2.0f, 0.0f, 0.0f},
                           HeightRange{0.0f, 1.0f});

    world.SetBodyTransform(table_part, {0.0f, 0.0f, -1.0f}, 90.0f);
    MoveResult result = world.MoveKinematic(mover, {4.0f, 0.0f, 0.0f}, 0.5f);
    Require(result.hit_count == 0,
            "rotated-away collider does not block x path");

    world.SetBodyTransform(mover, {-2.0f, 0.0f, 0.0f}, 0.0f);
    world.SetBodyTransform(table_part, {0.0f, 0.0f, 0.0f}, 0.0f);
    result = world.MoveKinematic(mover, {4.0f, 0.0f, 0.0f}, 0.5f);
    Require(result.hit_count > 0, "moved-back collider blocks x path");
    RequireNear(result.final_position.x, -0.45f, 0.01f,
                "moved-back collider stops at inflated edge");
}

}  // namespace

void RunPhysicsTests() {
    TestSweptMovementStopsBeforeThinBox();
    TestSlidingPreservesTangentialMovement();
    TestDepenetrationResolvesInitialOverlap();
    TestHeightRangesFilterAerialSensors();
    TestCircleSweepDoesNotTunnelThroughChair();
    TestTransformedStaticColliderMovesWithRoot();
}
