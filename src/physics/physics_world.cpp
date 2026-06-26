#include "future_gaze/physics/physics_world.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

namespace future_gaze::physics
{

namespace
{

constexpr float kPi = 3.14159265358979323846f;
constexpr float kEpsilon = 0.00001f;
constexpr float kHuge = 1000000.0f;

struct Vec2
{
    float x = 0.0f;
    float z = 0.0f;
};

struct SweepHit
{
    float time = 1.0f;
    Vec2 normal{};
};

[[nodiscard]] Vec2 ToVec2(Vec3 value) noexcept {
    return {value.x, value.z};
}

[[nodiscard]] Vec3 ToVec3(Vec2 value, float y = 0.0f) noexcept {
    return {value.x, y, value.z};
}

[[nodiscard]] Vec2 Add(Vec2 lhs, Vec2 rhs) noexcept {
    return {lhs.x + rhs.x, lhs.z + rhs.z};
}

[[nodiscard]] Vec2 Sub(Vec2 lhs, Vec2 rhs) noexcept {
    return {lhs.x - rhs.x, lhs.z - rhs.z};
}

[[nodiscard]] Vec2 Mul(Vec2 value, float scalar) noexcept {
    return {value.x * scalar, value.z * scalar};
}

[[nodiscard]] float Dot2(Vec2 lhs, Vec2 rhs) noexcept {
    return lhs.x * rhs.x + lhs.z * rhs.z;
}

[[nodiscard]] float LengthSq(Vec2 value) noexcept {
    return Dot2(value, value);
}

[[nodiscard]] float Length2(Vec2 value) noexcept {
    return std::sqrt(LengthSq(value));
}

[[nodiscard]] Vec2 Normalize2(Vec2 value) noexcept {
    const float length = Length2(value);
    if (length <= kEpsilon) {
        return {};
    }
    return Mul(value, 1.0f / length);
}

[[nodiscard]] float DegreesToRadians(float degrees) noexcept {
    return degrees * kPi / 180.0f;
}

[[nodiscard]] Vec2 Rotate(Vec2 value, float yaw_deg) noexcept {
    const float r = DegreesToRadians(yaw_deg);
    const float s = std::sin(r);
    const float c = std::cos(r);
    return {value.x * c + value.z * s, -value.x * s + value.z * c};
}

[[nodiscard]] Vec2 InverseRotate(Vec2 value, float yaw_deg) noexcept {
    const float r = DegreesToRadians(yaw_deg);
    const float s = std::sin(r);
    const float c = std::cos(r);
    return {value.x * c - value.z * s, value.x * s + value.z * c};
}

[[nodiscard]] bool HeightOverlaps(HeightRange lhs, HeightRange rhs) noexcept {
    return lhs.min_y <= rhs.max_y && rhs.min_y <= lhs.max_y;
}

[[nodiscard]] bool MasksCollide(CollisionMask lhs, CollisionMask rhs) noexcept {
    return (lhs.collide_bits & rhs.category_bits) != 0u &&
           (rhs.collide_bits & lhs.category_bits) != 0u;
}

[[nodiscard]] float SweepRadius(const Collider2D& collider) noexcept {
    switch (collider.kind) {
        case ColliderKind::Circle:
            return collider.radius;
        case ColliderKind::Capsule:
            return collider.radius + collider.half_length;
        case ColliderKind::Box:
            return std::sqrt(collider.half_extents.x * collider.half_extents.x +
                             collider.half_extents.z * collider.half_extents.z);
    }
    return collider.radius;
}


[[nodiscard]] std::optional<SweepHit> SweepPointCircle(Vec2 start, Vec2 delta,
                                                       Vec2 center,
                                                       float radius) {
    const Vec2 m = Sub(start, center);
    const float c = Dot2(m, m) - radius * radius;
    if (c <= 0.0f) {
        return std::nullopt;
    }

    const float a = Dot2(delta, delta);
    if (a <= kEpsilon) {
        return std::nullopt;
    }

    const float b = Dot2(m, delta);
    if (b >= 0.0f) {
        return std::nullopt;
    }

    const float disc = b * b - a * c;
    if (disc < 0.0f) {
        return std::nullopt;
    }

    const float time = (-b - std::sqrt(disc)) / a;
    if (time < 0.0f || time > 1.0f) {
        return std::nullopt;
    }

    const Vec2 at_hit = Add(start, Mul(delta, time));
    return SweepHit{time, Normalize2(Sub(at_hit, center))};
}

[[nodiscard]] std::optional<SweepHit> SweepPointAabb(Vec2 start, Vec2 delta,
                                                     Vec2 min, Vec2 max) {
    float enter = -kHuge;
    float exit = kHuge;
    Vec2 normal{};

    auto test_axis = [&](float origin, float step, float lo, float hi,
                         Vec2 axis_normal) {
        if (std::abs(step) <= kEpsilon) {
            return origin >= lo && origin <= hi;
        }
        float t0 = (lo - origin) / step;
        float t1 = (hi - origin) / step;
        Vec2 n = axis_normal;
        if (t0 > t1) {
            std::swap(t0, t1);
            n = Mul(axis_normal, -1.0f);
        }
        if (t0 > enter) {
            enter = t0;
            normal = n;
        }
        exit = std::min(exit, t1);
        return enter <= exit;
    };

    if (!test_axis(start.x, delta.x, min.x, max.x, {-1.0f, 0.0f}) ||
        !test_axis(start.z, delta.z, min.z, max.z, {0.0f, -1.0f})) {
        return std::nullopt;
    }
    if (enter < 0.0f || enter > 1.0f) {
        return std::nullopt;
    }
    return SweepHit{enter, normal};
}

[[nodiscard]] Contact CircleAgainstCircle(BodyId other, Vec2 subject_center,
                                          float subject_radius,
                                          Vec2 obstacle_center,
                                          float obstacle_radius) {
    const Vec2 delta = Sub(subject_center, obstacle_center);
    const float min_dist = subject_radius + obstacle_radius;
    const float dist = Length2(delta);
    if (dist >= min_dist) {
        return {};
    }
    const Vec2 normal =
        dist > kEpsilon ? Mul(delta, 1.0f / dist) : Vec2{1.0f, 0.0f};
    return Contact{other, ToVec3(normal), min_dist - dist};
}

[[nodiscard]] Contact CircleAgainstBox(BodyId other, Vec2 subject_center,
                                       float subject_radius,
                                       const Transform2D& box_transform,
                                       Vec3 half_extents) {
    const Vec2 local =
        InverseRotate(Sub(subject_center, ToVec2(box_transform.position)),
                      box_transform.yaw_deg);
    const Vec2 closest{std::clamp(local.x, -half_extents.x, half_extents.x),
                       std::clamp(local.z, -half_extents.z, half_extents.z)};
    const Vec2 offset = Sub(local, closest);
    const float dist = Length2(offset);
    if (dist >= subject_radius) {
        return {};
    }

    Vec2 local_normal{};
    float penetration = subject_radius - dist;
    if (dist > kEpsilon) {
        local_normal = Mul(offset, 1.0f / dist);
    }
    else {
        const float left = local.x + half_extents.x;
        const float right = half_extents.x - local.x;
        const float front = local.z + half_extents.z;
        const float back = half_extents.z - local.z;
        const float best = std::min({left, right, front, back});
        if (best == left) {
            local_normal = {-1.0f, 0.0f};
        }
        else if (best == right) {
            local_normal = {1.0f, 0.0f};
        }
        else if (best == front) {
            local_normal = {0.0f, -1.0f};
        }
        else {
            local_normal = {0.0f, 1.0f};
        }
        penetration = subject_radius + best;
    }

    return Contact{other, ToVec3(Rotate(local_normal, box_transform.yaw_deg)),
                   penetration};
}

}  // namespace

Collider2D Collider2D::Circle(float radius) {
    Collider2D collider;
    collider.kind = ColliderKind::Circle;
    collider.radius = radius;
    return collider;
}

Collider2D Collider2D::Capsule(float radius, float half_length) {
    Collider2D collider;
    collider.kind = ColliderKind::Capsule;
    collider.radius = radius;
    collider.half_length = half_length;
    return collider;
}

Collider2D Collider2D::Box(float half_x, float half_z) {
    Collider2D collider;
    collider.kind = ColliderKind::Box;
    collider.half_extents = {half_x, 0.0f, half_z};
    return collider;
}

BodyId PhysicsWorld::AddStatic(Collider2D collider, Transform2D local,
                               HeightRange height, CollisionMask mask) {
    return AddBody(BodyType::Static, collider, local, height, mask);
}

BodyId PhysicsWorld::AddKinematic(Collider2D collider, Vec3 position,
                                  HeightRange height, CollisionMask mask) {
    return AddBody(BodyType::Kinematic, collider, Transform2D{position, 0.0f},
                   height, mask);
}

BodyId PhysicsWorld::AddSensor(Collider2D collider, Vec3 position,
                               HeightRange height, CollisionMask mask) {
    return AddBody(BodyType::Sensor, collider, Transform2D{position, 0.0f},
                   height, mask);
}

void PhysicsWorld::Clear() {
    bodies_.clear();
}

void PhysicsWorld::SetBodyTransform(BodyId id, Vec3 position, float yaw_deg) {
    Body* body = MutableBody(id);
    if (body == nullptr) {
        return;
    }
    body->transform.position = position;
    body->transform.yaw_deg = yaw_deg;
}

void PhysicsWorld::SetBodyHeight(BodyId id, HeightRange height) {
    Body* body = MutableBody(id);
    if (body == nullptr) {
        return;
    }
    body->height = height;
}

Vec3 PhysicsWorld::BodyPosition(BodyId id) const {
    const Body* body = BodyFor(id);
    return body != nullptr ? body->transform.position : Vec3{};
}

MoveResult PhysicsWorld::MoveKinematic(BodyId id, Vec3 desired_delta, float dt,
                                       MoveOptions options) {
    (void)dt;
    MoveResult result;
    Body* body = MutableBody(id);
    if (body == nullptr || body->type != BodyType::Kinematic) {
        return result;
    }

    result.start_position = body->transform.position;
    result.final_position = body->transform.position;
    result.attempted_delta = desired_delta;

    const float radius = SweepRadius(body->collider);
    for (int i = 0; i < std::max(1, options.max_iterations); ++i) {
        const std::vector<Contact> contacts = QueryOverlaps(id);
        Contact deepest;
        for (const Contact& contact : contacts) {
            const Body* other = BodyFor(contact.other);
            if (other != nullptr && other->type != BodyType::Sensor &&
                contact.penetration > deepest.penetration) {
                deepest = contact;
            }
        }
        if (deepest.penetration <= 0.0f) {
            break;
        }
        body->transform.position =
            body->transform.position +
            deepest.normal * (deepest.penetration + options.skin);
        result.contacts.push_back(deepest);
    }

    Vec2 remaining = ToVec2(desired_delta);
    for (int iteration = 0; iteration < std::max(1, options.max_iterations);
         ++iteration) {
        if (LengthSq(remaining) <= kEpsilon * kEpsilon) {
            break;
        }

        std::optional<SweepHit> best;
        const Vec2 start = ToVec2(body->transform.position);
        for (std::uint32_t i = 0; i < bodies_.size(); ++i) {
            const Body& other = bodies_[i];
            if (!other.alive || other.type == BodyType::Sensor ||
                i == id.index || !HeightOverlaps(body->height, other.height) ||
                !MasksCollide(body->mask, other.mask)) {
                continue;
            }

            std::optional<SweepHit> hit;
            if (other.collider.kind == ColliderKind::Box) {
                const Vec2 local_start =
                    InverseRotate(Sub(start, ToVec2(other.transform.position)),
                                  other.transform.yaw_deg);
                const Vec2 local_delta =
                    InverseRotate(remaining, other.transform.yaw_deg);
                hit = SweepPointAabb(local_start, local_delta,
                                     {-other.collider.half_extents.x - radius,
                                      -other.collider.half_extents.z - radius},
                                     {other.collider.half_extents.x + radius,
                                      other.collider.half_extents.z + radius});
                if (hit.has_value()) {
                    hit->normal = Rotate(hit->normal, other.transform.yaw_deg);
                }
            }
            else {
                hit = SweepPointCircle(start, remaining,
                                       ToVec2(other.transform.position),
                                       radius + SweepRadius(other.collider));
            }
            if (hit.has_value() &&
                (!best.has_value() || hit->time < best->time)) {
                best = hit;
            }
        }

        if (!best.has_value()) {
            body->transform.position =
                body->transform.position + ToVec3(remaining);
            break;
        }

        const float remaining_length = Length2(remaining);
        const float skin_time =
            remaining_length > kEpsilon
                ? std::clamp(options.skin / remaining_length, 0.0f, 0.25f)
                : 0.0f;
        const float move_time = std::max(0.0f, best->time - skin_time);
        body->transform.position =
            body->transform.position + ToVec3(Mul(remaining, move_time));

        result.hit_count += 1;
        result.slide_normal = ToVec3(best->normal);

        Vec2 after_hit = Mul(remaining, 1.0f - move_time);
        const float into = Dot2(after_hit, best->normal);
        if (into < 0.0f) {
            if (options.restitution > 0.0f) {
                after_hit =
                    Sub(after_hit,
                        Mul(best->normal, (1.0f + options.restitution) * into));
            }
            else {
                after_hit = Sub(after_hit, Mul(best->normal, into));
            }
        }
        remaining = after_hit;
    }

    result.final_position = body->transform.position;
    result.applied_delta = result.final_position - result.start_position;
    return result;
}

std::vector<Contact> PhysicsWorld::QueryOverlaps(BodyId id) const {
    std::vector<Contact> contacts;
    const Body* subject = BodyFor(id);
    if (subject == nullptr) {
        return contacts;
    }

    const Vec2 center = ToVec2(subject->transform.position);
    const float radius = SweepRadius(subject->collider);
    for (std::uint32_t i = 0; i < bodies_.size(); ++i) {
        const Body& other = bodies_[i];
        if (!other.alive || i == id.index ||
            !HeightOverlaps(subject->height, other.height) ||
            !MasksCollide(subject->mask, other.mask)) {
            continue;
        }

        Contact contact;
        const BodyId other_id{i, other.generation};
        if (other.collider.kind == ColliderKind::Box) {
            contact =
                CircleAgainstBox(other_id, center, radius, other.transform,
                                 other.collider.half_extents);
        }
        else {
            contact = CircleAgainstCircle(other_id, center, radius,
                                          ToVec2(other.transform.position),
                                          SweepRadius(other.collider));
        }
        if (contact.penetration > 0.0f) {
            contacts.push_back(contact);
        }
    }
    return contacts;
}

BodyId PhysicsWorld::AddBody(BodyType type, Collider2D collider,
                             Transform2D transform, HeightRange height,
                             CollisionMask mask) {
    const std::uint32_t index = static_cast<std::uint32_t>(bodies_.size());
    Body body;
    body.collider = collider;
    body.transform = transform;
    body.height = height;
    body.mask = mask;
    body.type = type;
    body.alive = true;
    bodies_.push_back(body);
    return BodyId{index, body.generation};
}

PhysicsWorld::Body* PhysicsWorld::MutableBody(BodyId id) noexcept {
    if (!id.IsValid() || id.index >= bodies_.size()) {
        return nullptr;
    }
    Body& body = bodies_[id.index];
    if (!body.alive || body.generation != id.generation) {
        return nullptr;
    }
    return &body;
}

const PhysicsWorld::Body* PhysicsWorld::BodyFor(BodyId id) const noexcept {
    if (!id.IsValid() || id.index >= bodies_.size()) {
        return nullptr;
    }
    const Body& body = bodies_[id.index];
    if (!body.alive || body.generation != id.generation) {
        return nullptr;
    }
    return &body;
}

}  // namespace future_gaze::physics
