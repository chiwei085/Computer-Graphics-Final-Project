#pragma once

#include <GL/freeglut.h>

#include <array>
#include <vector>

#include "future_gaze/math/vec3.hpp"

namespace future_gaze
{

// RAII wrapper for glPushMatrix / glPopMatrix.
class ScopedMatrixPush
{
public:
    ScopedMatrixPush() { glPushMatrix(); }
    ~ScopedMatrixPush() { glPopMatrix(); }
    ScopedMatrixPush(const ScopedMatrixPush&) = delete;
    ScopedMatrixPush& operator=(const ScopedMatrixPush&) = delete;
};

// RAII wrapper for glEnableClientState / glDisableClientState.
class ScopedClientState
{
public:
    explicit ScopedClientState(GLenum state) : state_(state) {
        glEnableClientState(state_);
    }
    ~ScopedClientState() { glDisableClientState(state_); }
    ScopedClientState(const ScopedClientState&) = delete;
    ScopedClientState& operator=(const ScopedClientState&) = delete;

private:
    GLenum state_;
};

struct Vertex
{
    Vec3 position;
    Vec3 normal{0.0f, 1.0f, 0.0f};
    std::array<float, 2> texcoord{};
};

struct MeshBounds
{
    Vec3 min;
    Vec3 max;

    [[nodiscard]] Vec3 Center() const {
        return {(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f,
                (min.z + max.z) * 0.5f};
    }

    [[nodiscard]] Vec3 Size() const {
        return {max.x - min.x, max.y - min.y, max.z - min.z};
    }
};

class Mesh
{
public:
    explicit Mesh(std::vector<Vertex> vertices = {});

    static Mesh Cube();
    static Mesh Cylinder(float radius, float height, int segments = 20);
    static Mesh Frustum(float r_bottom, float r_top, float height,
                        int segments = 20);
    static Mesh Sphere(float radius, int slats = 24, int stacks = 16);
    static Mesh Disk(float radius, int segments = 20);
    // uv_repeat tiles texture around circumference (U=angle, V=inner→outer).
    static Mesh Ring(float inner_radius, float outer_radius, float thickness,
                     int segments = 20, float uv_repeat = 1.0f);

    [[nodiscard]] MeshBounds Bounds() const;
    void Draw() const;

private:
    std::vector<Vertex> vertices_;
};

}  // namespace future_gaze
