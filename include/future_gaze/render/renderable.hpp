#pragma once

#include "future_gaze/render/material.hpp"
#include "future_gaze/render/mesh.hpp"

namespace future_gaze
{

// A Mesh and its Material treated as one lifecycle unit.
// Drawing always applies material state before submitting geometry.
struct Renderable
{
    Mesh mesh;
    Material material;

    void Draw() const {
        material.Apply();
        mesh.Draw();
    }
};

}  // namespace future_gaze
