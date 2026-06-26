#include "future_gaze/render/mesh.hpp"

#include <GL/freeglut.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <numbers>
#include <span>
#include <utility>

namespace future_gaze
{

namespace
{

void AddFace(std::vector<Vertex>& vertices, std::span<const Vec3, 4> corners,
             const Vec3& normal) {
    const std::array<Vertex, 6> face = {
        Vertex{corners[0], normal, {0.0f, 0.0f}},
        Vertex{corners[1], normal, {1.0f, 0.0f}},
        Vertex{corners[2], normal, {1.0f, 1.0f}},
        Vertex{corners[0], normal, {0.0f, 0.0f}},
        Vertex{corners[2], normal, {1.0f, 1.0f}},
        Vertex{corners[3], normal, {0.0f, 1.0f}},
    };
    vertices.insert(vertices.end(), face.begin(), face.end());
}

}  // namespace

Mesh::Mesh(std::vector<Vertex> vertices) : vertices_(std::move(vertices)) {}

MeshBounds Mesh::Bounds() const {
    if (vertices_.empty()) {
        return {};
    }
    const float inf = std::numeric_limits<float>::infinity();
    MeshBounds bounds{{inf, inf, inf}, {-inf, -inf, -inf}};
    for (const Vertex& vertex : vertices_) {
        bounds.min.x = std::min(bounds.min.x, vertex.position.x);
        bounds.min.y = std::min(bounds.min.y, vertex.position.y);
        bounds.min.z = std::min(bounds.min.z, vertex.position.z);
        bounds.max.x = std::max(bounds.max.x, vertex.position.x);
        bounds.max.y = std::max(bounds.max.y, vertex.position.y);
        bounds.max.z = std::max(bounds.max.z, vertex.position.z);
    }
    return bounds;
}

Mesh Mesh::Cube() {
    Mesh mesh;
    mesh.vertices_.reserve(36);

    AddFace(mesh.vertices_,
            std::array{Vec3{-1.0f, -1.0f, 1.0f}, Vec3{1.0f, -1.0f, 1.0f},
                       Vec3{1.0f, 1.0f, 1.0f}, Vec3{-1.0f, 1.0f, 1.0f}},
            Vec3{0.0f, 0.0f, 1.0f});
    AddFace(mesh.vertices_,
            std::array{Vec3{1.0f, -1.0f, -1.0f}, Vec3{-1.0f, -1.0f, -1.0f},
                       Vec3{-1.0f, 1.0f, -1.0f}, Vec3{1.0f, 1.0f, -1.0f}},
            Vec3{0.0f, 0.0f, -1.0f});
    AddFace(mesh.vertices_,
            std::array{Vec3{-1.0f, -1.0f, -1.0f}, Vec3{-1.0f, -1.0f, 1.0f},
                       Vec3{-1.0f, 1.0f, 1.0f}, Vec3{-1.0f, 1.0f, -1.0f}},
            Vec3{-1.0f, 0.0f, 0.0f});
    AddFace(mesh.vertices_,
            std::array{Vec3{1.0f, -1.0f, 1.0f}, Vec3{1.0f, -1.0f, -1.0f},
                       Vec3{1.0f, 1.0f, -1.0f}, Vec3{1.0f, 1.0f, 1.0f}},
            Vec3{1.0f, 0.0f, 0.0f});
    AddFace(mesh.vertices_,
            std::array{Vec3{-1.0f, 1.0f, 1.0f}, Vec3{1.0f, 1.0f, 1.0f},
                       Vec3{1.0f, 1.0f, -1.0f}, Vec3{-1.0f, 1.0f, -1.0f}},
            Vec3{0.0f, 1.0f, 0.0f});
    AddFace(mesh.vertices_,
            std::array{Vec3{-1.0f, -1.0f, -1.0f}, Vec3{1.0f, -1.0f, -1.0f},
                       Vec3{1.0f, -1.0f, 1.0f}, Vec3{-1.0f, -1.0f, 1.0f}},
            Vec3{0.0f, -1.0f, 0.0f});

    return mesh;
}

Mesh Mesh::Frustum(float r_bottom, float r_top, float height, int segments) {
    assert(r_bottom >= 0.0f);
    assert(r_top >= 0.0f);
    assert(r_bottom + r_top > 0.0f);
    assert(height > 0.0f);
    assert(segments >= 3);
    Mesh mesh;
    const float da =
        2.0f * std::numbers::pi_v<float> / static_cast<float>(segments);

    auto side_normal = [&](float angle) -> Vec3 {
        return Normalize(Vec3{height * std::cos(angle), r_bottom - r_top,
                              height * std::sin(angle)});
    };

    for (int i = 0; i < segments; ++i) {
        const float a0 = da * static_cast<float>(i);
        const float a1 = da * static_cast<float>(i + 1);
        const float u0 = static_cast<float>(i) / static_cast<float>(segments);
        const float u1 =
            static_cast<float>(i + 1) / static_cast<float>(segments);

        const Vec3 n0 = side_normal(a0);
        const Vec3 n1 = side_normal(a1);
        const Vec3 bl = {r_bottom * std::cos(a0), 0.0f,
                         r_bottom * std::sin(a0)};
        const Vec3 tl = {r_top * std::cos(a0), height, r_top * std::sin(a0)};
        const Vec3 br = {r_bottom * std::cos(a1), 0.0f,
                         r_bottom * std::sin(a1)};
        const Vec3 tr = {r_top * std::cos(a1), height, r_top * std::sin(a1)};

        mesh.vertices_.push_back({bl, n0, {u0, 0.0f}});
        mesh.vertices_.push_back({tl, n0, {u0, 1.0f}});
        mesh.vertices_.push_back({tr, n1, {u1, 1.0f}});
        mesh.vertices_.push_back({bl, n0, {u0, 0.0f}});
        mesh.vertices_.push_back({tr, n1, {u1, 1.0f}});
        mesh.vertices_.push_back({br, n1, {u1, 0.0f}});
    }

    // Bottom cap: normal -Y; winding: center, p(a0), p(a1)
    if (r_bottom > 0.0f) {
        const Vec3 bc = {};
        const Vec3 bn = {0.0f, -1.0f, 0.0f};
        for (int i = 0; i < segments; ++i) {
            const float a0 = da * static_cast<float>(i);
            const float a1 = da * static_cast<float>(i + 1);
            const Vec3 p0 = {r_bottom * std::cos(a0), 0.0f,
                             r_bottom * std::sin(a0)};
            const Vec3 p1 = {r_bottom * std::cos(a1), 0.0f,
                             r_bottom * std::sin(a1)};
            mesh.vertices_.push_back({bc, bn, {0.5f, 0.5f}});
            mesh.vertices_.push_back(
                {p0,
                 bn,
                 {0.5f + 0.5f * std::cos(a0), 0.5f + 0.5f * std::sin(a0)}});
            mesh.vertices_.push_back(
                {p1,
                 bn,
                 {0.5f + 0.5f * std::cos(a1), 0.5f + 0.5f * std::sin(a1)}});
        }
    }

    // Top cap: normal +Y; winding: center, p(a1), p(a0)
    if (r_top > 0.0f) {
        const Vec3 tc = {0.0f, height, 0.0f};
        const Vec3 tn = {0.0f, 1.0f, 0.0f};
        for (int i = 0; i < segments; ++i) {
            const float a0 = da * static_cast<float>(i);
            const float a1 = da * static_cast<float>(i + 1);
            const Vec3 p0 = {r_top * std::cos(a0), height,
                             r_top * std::sin(a0)};
            const Vec3 p1 = {r_top * std::cos(a1), height,
                             r_top * std::sin(a1)};
            mesh.vertices_.push_back({tc, tn, {0.5f, 0.5f}});
            mesh.vertices_.push_back(
                {p1,
                 tn,
                 {0.5f + 0.5f * std::cos(a1), 0.5f + 0.5f * std::sin(a1)}});
            mesh.vertices_.push_back(
                {p0,
                 tn,
                 {0.5f + 0.5f * std::cos(a0), 0.5f + 0.5f * std::sin(a0)}});
        }
    }

    return mesh;
}

Mesh Mesh::Cylinder(float radius, float height, int segments) {
    assert(radius > 0.0f);
    assert(height > 0.0f);
    assert(segments >= 3);
    return Frustum(radius, radius, height, segments);
}

Mesh Mesh::Sphere(float radius, int slats, int stacks) {
    assert(radius > 0.0f);
    assert(slats >= 3);
    assert(stacks >= 2);
    Mesh mesh;
    const float dtheta =
        2.0f * std::numbers::pi_v<float> / static_cast<float>(slats);
    const float dphi = std::numbers::pi_v<float> / static_cast<float>(stacks);

    auto P = [radius](float theta, float phi) -> Vec3 {
        return {radius * std::sin(phi) * std::cos(theta),
                radius * std::cos(phi),
                radius * std::sin(phi) * std::sin(theta)};
    };
    auto N = [](float theta, float phi) -> Vec3 {
        return {std::sin(phi) * std::cos(theta), std::cos(phi),
                std::sin(phi) * std::sin(theta)};
    };

    for (int j = 0; j < stacks; ++j) {
        for (int i = 0; i < slats; ++i) {
            const float theta0 = dtheta * static_cast<float>(i);
            const float theta1 = dtheta * static_cast<float>(i + 1);
            const float phi0 = dphi * static_cast<float>(j);
            const float phi1 = dphi * static_cast<float>(j + 1);

            const float u0 = static_cast<float>(i) / static_cast<float>(slats);
            const float u1 =
                static_cast<float>(i + 1) / static_cast<float>(slats);
            const float v0 = static_cast<float>(j) / static_cast<float>(stacks);
            const float v1 =
                static_cast<float>(j + 1) / static_cast<float>(stacks);

            // CCW from outside; verified via cross-product analysis
            mesh.vertices_.push_back(
                {P(theta0, phi0), N(theta0, phi0), {u0, v0}});
            mesh.vertices_.push_back(
                {P(theta1, phi0), N(theta1, phi0), {u1, v0}});
            mesh.vertices_.push_back(
                {P(theta1, phi1), N(theta1, phi1), {u1, v1}});
            mesh.vertices_.push_back(
                {P(theta0, phi0), N(theta0, phi0), {u0, v0}});
            mesh.vertices_.push_back(
                {P(theta1, phi1), N(theta1, phi1), {u1, v1}});
            mesh.vertices_.push_back(
                {P(theta0, phi1), N(theta0, phi1), {u0, v1}});
        }
    }

    return mesh;
}

Mesh Mesh::Disk(float radius, int segments) {
    assert(radius > 0.0f);
    assert(segments >= 3);
    Mesh mesh;
    const float da =
        2.0f * std::numbers::pi_v<float> / static_cast<float>(segments);
    const Vec3 center = {};
    const Vec3 normal = {0.0f, 1.0f, 0.0f};

    // +Y normal; winding: center, p(a1), p(a0) — verified
    for (int i = 0; i < segments; ++i) {
        const float a0 = da * static_cast<float>(i);
        const float a1 = da * static_cast<float>(i + 1);
        const Vec3 p0 = {radius * std::cos(a0), 0.0f, radius * std::sin(a0)};
        const Vec3 p1 = {radius * std::cos(a1), 0.0f, radius * std::sin(a1)};
        mesh.vertices_.push_back({center, normal, {0.5f, 0.5f}});
        mesh.vertices_.push_back(
            {p1,
             normal,
             {0.5f + 0.5f * std::cos(a1), 0.5f + 0.5f * std::sin(a1)}});
        mesh.vertices_.push_back(
            {p0,
             normal,
             {0.5f + 0.5f * std::cos(a0), 0.5f + 0.5f * std::sin(a0)}});
    }

    return mesh;
}

Mesh Mesh::Ring(float inner_r, float outer_r, float thickness, int segments,
                float uv_repeat) {
    assert(inner_r >= 0.0f);
    assert(outer_r > inner_r);
    assert(thickness > 0.0f);
    assert(segments >= 3);
    Mesh mesh;
    const float da =
        2.0f * std::numbers::pi_v<float> / static_cast<float>(segments);
    const Vec3 top_n = {0.0f, 1.0f, 0.0f};
    const Vec3 bot_n = {0.0f, -1.0f, 0.0f};

    for (int i = 0; i < segments; ++i) {
        const float a0 = da * static_cast<float>(i);
        const float a1 = da * static_cast<float>(i + 1);
        // U follows the angle (optionally tiled); V runs inner(0)→outer(1).
        const float u0 =
            static_cast<float>(i) / static_cast<float>(segments) * uv_repeat;
        const float u1 = static_cast<float>(i + 1) /
                         static_cast<float>(segments) * uv_repeat;

        // Top annulus (+Y): winding inner(a0),inner(a1),outer(a1),outer(a0)
        {
            const Vec3 ii0 = {inner_r * std::cos(a0), thickness,
                              inner_r * std::sin(a0)};
            const Vec3 ii1 = {inner_r * std::cos(a1), thickness,
                              inner_r * std::sin(a1)};
            const Vec3 oo0 = {outer_r * std::cos(a0), thickness,
                              outer_r * std::sin(a0)};
            const Vec3 oo1 = {outer_r * std::cos(a1), thickness,
                              outer_r * std::sin(a1)};
            mesh.vertices_.push_back({ii0, top_n, {u0, 0.0f}});
            mesh.vertices_.push_back({ii1, top_n, {u1, 0.0f}});
            mesh.vertices_.push_back({oo1, top_n, {u1, 1.0f}});
            mesh.vertices_.push_back({ii0, top_n, {u0, 0.0f}});
            mesh.vertices_.push_back({oo1, top_n, {u1, 1.0f}});
            mesh.vertices_.push_back({oo0, top_n, {u0, 1.0f}});
        }

        // Bottom annulus (-Y): winding inner(a0),outer(a0),outer(a1),inner(a1)
        {
            const Vec3 ii0 = {inner_r * std::cos(a0), 0.0f,
                              inner_r * std::sin(a0)};
            const Vec3 ii1 = {inner_r * std::cos(a1), 0.0f,
                              inner_r * std::sin(a1)};
            const Vec3 oo0 = {outer_r * std::cos(a0), 0.0f,
                              outer_r * std::sin(a0)};
            const Vec3 oo1 = {outer_r * std::cos(a1), 0.0f,
                              outer_r * std::sin(a1)};
            mesh.vertices_.push_back({ii0, bot_n, {u0, 0.0f}});
            mesh.vertices_.push_back({oo0, bot_n, {u0, 1.0f}});
            mesh.vertices_.push_back({oo1, bot_n, {u1, 1.0f}});
            mesh.vertices_.push_back({ii0, bot_n, {u0, 0.0f}});
            mesh.vertices_.push_back({oo1, bot_n, {u1, 1.0f}});
            mesh.vertices_.push_back({ii1, bot_n, {u1, 0.0f}});
        }

        // Outer side wall (outward normal)
        {
            const Vec3 n0 = {std::cos(a0), 0.0f, std::sin(a0)};
            const Vec3 n1 = {std::cos(a1), 0.0f, std::sin(a1)};
            const Vec3 bl = {outer_r * std::cos(a0), 0.0f,
                             outer_r * std::sin(a0)};
            const Vec3 tl = {outer_r * std::cos(a0), thickness,
                             outer_r * std::sin(a0)};
            const Vec3 br = {outer_r * std::cos(a1), 0.0f,
                             outer_r * std::sin(a1)};
            const Vec3 tr = {outer_r * std::cos(a1), thickness,
                             outer_r * std::sin(a1)};
            mesh.vertices_.push_back({bl, n0, {u0, 0.0f}});
            mesh.vertices_.push_back({tl, n0, {u0, 1.0f}});
            mesh.vertices_.push_back({tr, n1, {u1, 1.0f}});
            mesh.vertices_.push_back({bl, n0, {u0, 0.0f}});
            mesh.vertices_.push_back({tr, n1, {u1, 1.0f}});
            mesh.vertices_.push_back({br, n1, {u1, 0.0f}});
        }
    }

    return mesh;
}

void Mesh::Draw() const {
    if (vertices_.empty()) {
        return;
    }

    const ScopedClientState v(GL_VERTEX_ARRAY);
    const ScopedClientState n(GL_NORMAL_ARRAY);
    const ScopedClientState t(GL_TEXTURE_COORD_ARRAY);

    const auto stride = static_cast<GLsizei>(sizeof(Vertex));
    glVertexPointer(3, GL_FLOAT, stride, &vertices_.front().position.x);
    glNormalPointer(GL_FLOAT, stride, &vertices_.front().normal.x);
    glTexCoordPointer(2, GL_FLOAT, stride, vertices_.front().texcoord.data());
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_.size()));
}

}  // namespace future_gaze
