#include "future_gaze/render/mesh.hpp"

#include <GL/freeglut.h>

#include <array>
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

void Mesh::Draw() const {
    if (vertices_.empty()) {
        return;
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    const auto stride = static_cast<GLsizei>(sizeof(Vertex));
    glVertexPointer(3, GL_FLOAT, stride, &vertices_.front().position.x);
    glNormalPointer(GL_FLOAT, stride, &vertices_.front().normal.x);
    glTexCoordPointer(2, GL_FLOAT, stride, vertices_.front().texcoord.data());
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_.size()));

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

}  // namespace future_gaze
