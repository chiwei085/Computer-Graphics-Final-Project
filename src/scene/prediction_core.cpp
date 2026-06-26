#include <cmath>
#include <numbers>

#include "future_gaze/render/material.hpp"
#include "future_gaze/render/mesh.hpp"
#include "future_gaze/scene/builders.hpp"

namespace future_gaze::builders
{

SceneNode& BuildPredictionCore(SceneNode& parent, const TextureSet& tex) {
    auto& root = parent.NewChild(names::kPredictionCore);
    root.at({0.0f, 3.0f, -1.5f});
    // Ry(180) faces the iris toward the camera; Rx(-15) tilts gaze toward the table.
    root.rot_y(180.0f).rot_x(-15.0f);

    constexpr float kR = 1.2f;

    root.NewChild("sclera").draw_as(Mesh::Sphere(kR, 32, 24),
                                    Material::Porcelain(), tex.marble);

    root.NewChild("seam")
        .at({0.0f, -0.06f, 0.0f})
        .draw_as(Mesh::Ring(1.18f, 1.235f, 0.12f, 96, 24.0f),
                 Material::Chrome(), tex.metal);

    // Iris parts live in local XY plane (rot_x(-90) maps +Y normal to -Z).
    // Stacked at z = -1.22…-1.25 to avoid z-fighting.
    root.NewChild("iris_frame")
        .at({0.0f, 0.0f, -1.22f})
        .rot_x(-90.0f)
        .draw_as(Mesh::Ring(0.26f, 0.88f, 0.055f, 96), Material::Metal(),
                 tex.metal);

    // circuit_tilt holds the static rot_x(-90); circuit_ring (child) carries the
    // animated rot_y spin. Splitting tilt from spin keeps rotation in-plane and
    // prevents the ring from tumbling into the sclera.
    {
        auto& circuit_tilt = root.NewChild("circuit_tilt")
                                 .at({0.0f, 0.0f, -1.28f})
                                 .rot_x(-90.0f);
        circuit_tilt.NewChild(names::kCircuitRing)
            .draw_as(Mesh::Ring(0.36f, 0.58f, 0.04f, 96, 10.0f),
                     Material::EmissiveGreen(), tex.circuit);
    }

    root.NewChild("scan_ring")
        .at({0.0f, 0.0f, -1.24f})
        .rot_x(-90.0f)
        .draw_as(Mesh::Ring(1.02f, 1.28f, 0.038f, 128),
                 Material::EmissiveCyan());

    root.NewChild("pupil")
        .at({0.0f, 0.0f, -1.25f})
        .rot_x(-90.0f)
        .draw_as(Mesh::Disk(0.26f), Material::BlackMatte());

    // rot_x(+90) maps the cylinder's +Y axis to +Z so cables exit the rear.
    for (int i = 0; i < 6; ++i) {
        const float angle =
            std::numbers::pi_v<float> * 2.0f * static_cast<float>(i) / 6.0f;
        root.NewChild("cable")
            .at({0.30f * std::cos(angle), 0.30f * std::sin(angle), 1.18f})
            .rot_x(90.0f)
            .draw_as(Mesh::Cylinder(0.030f, 0.90f), Material::Metal(),
                     tex.metal);
    }

    // Additive unlit glow shells at kGlowZ, in front of the emissive rings.
    constexpr float kGlowZ = -1.31f;

    root.NewChild("scan_glow_inner")
        .at({0.0f, 0.0f, kGlowZ})
        .rot_x(-90.0f)
        .draw_glow(Mesh::Ring(0.96f, 1.36f, 0.02f, 128),
                   Material::GlowCyan(0.55f));
    root.NewChild("scan_glow_outer")
        .at({0.0f, 0.0f, kGlowZ})
        .rot_x(-90.0f)
        .draw_glow(Mesh::Ring(0.84f, 1.52f, 0.02f, 128),
                   Material::GlowCyan(0.26f));

    root.NewChild("circuit_glow_outer")
        .at({0.0f, 0.0f, kGlowZ})
        .rot_x(-90.0f)
        .draw_glow(Mesh::Ring(0.56f, 0.66f, 0.02f, 96),
                   Material::GlowGreen(0.40f));
    root.NewChild("circuit_glow_inner")
        .at({0.0f, 0.0f, kGlowZ})
        .rot_x(-90.0f)
        .draw_glow(Mesh::Ring(0.30f, 0.38f, 0.02f, 96),
                   Material::GlowGreen(0.35f));

    return root;
}

}  // namespace future_gaze::builders
