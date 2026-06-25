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

    void Draw() const;

private:
    std::vector<Vertex> vertices_;
};

}  // namespace future_gaze
