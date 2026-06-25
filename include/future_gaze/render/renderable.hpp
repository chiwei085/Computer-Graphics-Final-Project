#pragma once

#include <GL/freeglut.h>

#include "future_gaze/render/material.hpp"
#include "future_gaze/render/mesh.hpp"
#include "future_gaze/render/texture.hpp"

namespace future_gaze
{

// A Mesh and its Material treated as one lifecycle unit.
// Drawing always applies material state before submitting geometry.
// texture is non-owning; nullptr means no texture (GL_TEXTURE_2D disabled).
struct Renderable
{
    Mesh mesh;
    Material material;
    const Texture* texture = nullptr;

    void Draw() const {
        material.Apply();
        if (texture != nullptr) {
            glEnable(GL_TEXTURE_2D);
            texture->Bind();
        } else {
            glDisable(GL_TEXTURE_2D);
        }
        mesh.Draw();
        if (texture != nullptr) {
            glDisable(GL_TEXTURE_2D);
        }
    }
};

}  // namespace future_gaze
