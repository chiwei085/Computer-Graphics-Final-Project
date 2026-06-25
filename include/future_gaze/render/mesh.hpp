#pragma once

#include <array>
#include <vector>

#include "future_gaze/math/vec3.hpp"

namespace future_gaze
{

struct Vertex
{
    Vec3 position;
    Vec3 normal{0.0f, 1.0f, 0.0f};
    std::array<float, 2> texcoord{};
};

class Mesh
{
public:
    explicit Mesh(std::vector<Vertex> vertices = {});

    static Mesh Cube();
    // Closed cylinder from y=0 to y=height
    static Mesh Cylinder(float radius, float height, int segments = 20);
    // Truncated cone / frustum from y=0 (r_bottom) to y=height (r_top)
    static Mesh Frustum(float r_bottom, float r_top, float height,
                        int segments = 20);
    // Latitude-longitude sphere, centered at origin
    static Mesh Sphere(float radius, int slats = 24, int stacks = 16);
    // Flat disk at y=0, normal +Y
    static Mesh Disk(float radius, int segments = 20);
    // Hollow cylindrical ring: top/bottom annuli + outer side wall.
    // uv_repeat tiles the texture this many times around the circumference
    // (U = angle), with V running 0→1 radially inner→outer.
    static Mesh Ring(float inner_radius, float outer_radius, float thickness,
                     int segments = 20, float uv_repeat = 1.0f);

    void Draw() const;

private:
    std::vector<Vertex> vertices_;
};

}  // namespace future_gaze
