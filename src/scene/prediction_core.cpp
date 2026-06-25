#include <cmath>
#include <numbers>

#include "future_gaze/render/material.hpp"
#include "future_gaze/render/mesh.hpp"
#include "future_gaze/scene/builders.hpp"

namespace future_gaze::builders
{

std::unique_ptr<SceneNode> BuildPredictionCore() {
    auto root = std::make_unique<SceneNode>("prediction_core");
    root->position = {0.0f, 3.0f, -1.5f};
    // Ry(180°) flips local -Z to world +Z so iris faces the camera.
    // Rx(-15°) tilts gaze slightly downward toward the dinner table.
    root->euler_deg.y = 180.0f;
    root->euler_deg.x = -15.0f;

    constexpr float kR = 1.2f;

    // ── SCLERA
    // ────────────────────────────────────────────────────────────────
    root->NewChild("sclera").draw_as(Mesh::Sphere(kR, 32, 24),
                                     Material::Porcelain());

    // ── EQUATORIAL CHROME SEAM
    // ────────────────────────────────────────────────
    root->NewChild("seam")
        .at({0.0f, -0.06f, 0.0f})
        .draw_as(Mesh::Ring(1.18f, 1.235f, 0.12f), Material::Chrome());

    // Iris assembly — rings/disk live in local XY plane (rot_x(-90°) maps
    // +Y normal to -Z, facing viewer after the parent's Ry(180°)).
    // Stacked at z = -1.22 … -1.25 to avoid z-fighting.

    // ── IRIS FRAME
    // ────────────────────────────────────────────────────────────
    root->NewChild("iris_frame")
        .at({0.0f, 0.0f, -1.22f})
        .rot_x(-90.0f)
        .draw_as(Mesh::Ring(0.26f, 0.88f, 0.055f), Material::Metal());

    // ── CIRCUIT RING (emissive green)
    // ─────────────────────────────────────────
    root->NewChild("circuit_ring")
        .at({0.0f, 0.0f, -1.23f})
        .rot_x(-90.0f)
        .draw_as(Mesh::Ring(0.36f, 0.58f, 0.04f), Material::EmissiveGreen());

    // ── SCAN RING (emissive cyan halo)
    // ────────────────────────────────────────
    root->NewChild("scan_ring")
        .at({0.0f, 0.0f, -1.24f})
        .rot_x(-90.0f)
        .draw_as(Mesh::Ring(1.02f, 1.28f, 0.038f), Material::EmissiveCyan());

    // ── PUPIL
    // ─────────────────────────────────────────────────────────────────
    root->NewChild("pupil")
        .at({0.0f, 0.0f, -1.25f})
        .rot_x(-90.0f)
        .draw_as(Mesh::Disk(0.26f), Material::BlackMatte());

    // ── CABLE BUNDLE (exits from back of sphere in local +Z)
    // ────────────────── rot_x(+90°) rotates the cylinder's Y-axis to +Z so
    // cables run outward.
    for (int i = 0; i < 6; ++i) {
        const float angle =
            std::numbers::pi_v<float> * 2.0f * static_cast<float>(i) / 6.0f;
        root->NewChild("cable")
            .at({0.30f * std::cos(angle), 0.30f * std::sin(angle), 1.18f})
            .rot_x(90.0f)
            .draw_as(Mesh::Cylinder(0.030f, 0.90f), Material::Metal());
    }

    return root;
}

}  // namespace future_gaze::builders
