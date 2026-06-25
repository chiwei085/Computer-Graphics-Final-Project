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
// additive: when true the mesh is drawn as an unlit, depth-read-only halo
// blended with GL_SRC_ALPHA/GL_ONE (additive) — used for fake emissive glow
// on the Prediction Core without any shader (P3.3).
struct Renderable
{
    Mesh mesh;
    Material material;
    const Texture* texture = nullptr;
    bool additive = false;

    void Draw() const {
        if (additive) {
            DrawGlow();
            return;
        }
        material.Apply();
        if (texture != nullptr) {
            glEnable(GL_TEXTURE_2D);
            texture->Bind();
        }
        else {
            glDisable(GL_TEXTURE_2D);
        }
        mesh.Draw();
        if (texture != nullptr) {
            glDisable(GL_TEXTURE_2D);
        }
    }

private:
    // Additive halo pass: lighting off so the diffuse colour shows at full
    // strength, additive blend so overlapping layers brighten, and depth
    // writes off so the translucent shell does not occlude geometry behind it
    // (depth test stays on so opaque objects still hide the glow).
    void DrawGlow() const {
        glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        material.Apply();  // sets glColor4f to the diffuse colour (incl. alpha)
        mesh.Draw();
        glPopAttrib();
    }
};

}  // namespace future_gaze
