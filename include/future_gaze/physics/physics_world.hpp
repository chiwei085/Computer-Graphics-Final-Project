#pragma once

#include <cstdint>
#include <limits>
#include <vector>

#include "future_gaze/math/vec3.hpp"

namespace future_gaze::physics
{

struct BodyId
{
    std::uint32_t index = std::numeric_limits<std::uint32_t>::max();
    std::uint32_t generation = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept {
        return index != std::numeric_limits<std::uint32_t>::max() &&
               generation != 0;
    }

    [[nodiscard]] friend constexpr bool operator==(BodyId lhs,
                                                   BodyId rhs) noexcept {
        return lhs.index == rhs.index && lhs.generation == rhs.generation;
    }
    [[nodiscard]] friend constexpr bool operator!=(BodyId lhs,
                                                   BodyId rhs) noexcept {
        return !(lhs == rhs);
    }
};

struct CollisionMask
{
    std::uint32_t category_bits = 0x1u;
    std::uint32_t collide_bits = 0xFFFFFFFFu;
};

struct HeightRange
{
    float min_y = 0.0f;
    float max_y = 1.0f;
};

struct Transform2D
{
    Vec3 position{};
    float yaw_deg = 0.0f;
};

enum class ColliderKind
{
    Circle,
    Capsule,
    Box
};

struct Collider2D
{
    ColliderKind kind = ColliderKind::Circle;
    float radius = 0.5f;
    float half_length = 0.0f;
    Vec3 half_extents{0.5f, 0.0f, 0.5f};

    [[nodiscard]] static Collider2D Circle(float radius);
    [[nodiscard]] static Collider2D Capsule(float radius, float half_length);
    [[nodiscard]] static Collider2D Box(float half_x, float half_z);
};

struct MoveOptions
{
    int max_iterations = 4;
    float skin = 0.002f;
    float restitution = 0.0f;
};

struct Contact
{
    BodyId other{};
    Vec3 normal{};
    float penetration = 0.0f;
};

struct MoveResult
{
    Vec3 start_position{};
    Vec3 final_position{};
    Vec3 attempted_delta{};
    Vec3 applied_delta{};
    Vec3 slide_normal{};
    int hit_count = 0;
    std::vector<Contact> contacts;
};

class PhysicsWorld
{
public:
    [[nodiscard]] BodyId AddStatic(Collider2D collider, Transform2D local,
                                   HeightRange height, CollisionMask mask = {});
    [[nodiscard]] BodyId AddKinematic(Collider2D collider, Vec3 position,
                                      HeightRange height,
                                      CollisionMask mask = {});
    [[nodiscard]] BodyId AddSensor(Collider2D collider, Vec3 position,
                                   HeightRange height, CollisionMask mask = {});

    void Clear();
    void SetBodyTransform(BodyId id, Vec3 position, float yaw_deg);
    void SetBodyHeight(BodyId id, HeightRange height);

    [[nodiscard]] Vec3 BodyPosition(BodyId id) const;
    [[nodiscard]] MoveResult MoveKinematic(BodyId id, Vec3 desired_delta,
                                           float dt, MoveOptions options = {});
    [[nodiscard]] std::vector<Contact> QueryOverlaps(BodyId id) const;

private:
    enum class BodyType
    {
        Static,
        Kinematic,
        Sensor
    };

    struct Body
    {
        Collider2D collider;
        Transform2D transform;
        HeightRange height;
        CollisionMask mask;
        BodyType type = BodyType::Static;
        std::uint32_t generation = 1;
        bool alive = false;
    };

    [[nodiscard]] BodyId AddBody(BodyType type, Collider2D collider,
                                 Transform2D transform, HeightRange height,
                                 CollisionMask mask);
    [[nodiscard]] Body* MutableBody(BodyId id) noexcept;
    [[nodiscard]] const Body* BodyFor(BodyId id) const noexcept;

    std::vector<Body> bodies_;
};

}  // namespace future_gaze::physics
