#include <cmath>
#include <numbers>

#include "future_gaze/render/material.hpp"
#include "future_gaze/render/mesh.hpp"
#include "future_gaze/scene/builders.hpp"
#include "future_gaze/scene/node_names.hpp"

namespace future_gaze::builders
{

std::unique_ptr<SceneNode> BuildPredictionCore(const TextureSet& tex) {
    auto root = std::make_unique<SceneNode>(names::kPredictionCore);
    root->position = {0.0f, 3.0f, -1.5f};
    // Ry(180°) flips local -Z to world +Z so iris faces the camera.
    // Rx(-15°) tilts gaze slightly downward toward the dinner table.
    root->euler_deg.y = 180.0f;
    root->euler_deg.x = -15.0f;

    constexpr float kR = 1.2f;

    // ── SCLERA (marble texture)
    // ──────────────────────────────────────────────
    root->NewChild("sclera").draw_as(Mesh::Sphere(kR, 32, 24),
                                     Material::Porcelain(), tex.marble);

    // ── EQUATORIAL CHROME SEAM
    // ────────────────────────────────────────────────
    root->NewChild("seam")
        .at({0.0f, -0.06f, 0.0f})
        .draw_as(Mesh::Ring(1.18f, 1.235f, 0.12f, 96, 24.0f),
                 Material::Chrome(), tex.metal);

    // Iris assembly — rings/disk live in local XY plane (rot_x(-90°) maps
    // +Y normal to -Z, facing viewer after the parent's Ry(180°)).
    // Stacked at z = -1.22 … -1.25 to avoid z-fighting.

    // ── IRIS FRAME (metal texture)
    // ───────────────────────────────────────────
    root->NewChild("iris_frame")
        .at({0.0f, 0.0f, -1.22f})
        .rot_x(-90.0f)
        .draw_as(Mesh::Ring(0.26f, 0.88f, 0.055f, 96), Material::Metal(),
                 tex.metal);

    // ── CIRCUIT RING — emissive green + circuit texture
    // Placed at z=-1.28 so its top face (world-z +1.32) sits in front of
    // the iris frame top face (world-z +1.275), ensuring it is visible.
    // High segment count keeps the rim smooth; uv_repeat tiles the circuit
    // traces around the band so they read as crisp lines, not smeared noise.
    // The circuit ring self-rotates. The spin must happen IN the ring's
    // own plane, otherwise it tumbles around the vertical axis and clips into
    // the sclera. So a static tilt node (rot_x -90) faces the ring forward, and
    // its child carries the animated in-plane spin (rot_y about the ring's own
    // +Y normal, applied *before* the tilt). Animation drives "circuit_ring".
    {
        auto& circuit_tilt = root->NewChild("circuit_tilt")
                                 .at({0.0f, 0.0f, -1.28f})
                                 .rot_x(-90.0f);
        circuit_tilt.NewChild(names::kCircuitRing)
            .draw_as(Mesh::Ring(0.36f, 0.58f, 0.04f, 96, 10.0f),
                     Material::EmissiveGreen(), tex.circuit);
    }

    // ── SCAN RING (emissive cyan halo)
    // ────────────────────────────────────────
    root->NewChild("scan_ring")
        .at({0.0f, 0.0f, -1.24f})
        .rot_x(-90.0f)
        .draw_as(Mesh::Ring(1.02f, 1.28f, 0.038f, 128),
                 Material::EmissiveCyan());

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
            .draw_as(Mesh::Cylinder(0.030f, 0.90f), Material::Metal(),
                     tex.metal);
    }

    // ── EMISSIVE GLOW ─────────────────────────────────────────────────────
    // Fake bloom via additive, unlit, depth-read-only shells layered just in
    // front of the emissive rings (local z < the rings so they face the
    // viewer). Concentric rings of growing radius and falling intensity give
    // the scan ring a soft halo; a green shell wraps the circuit ring.
    constexpr float kGlowZ = -1.31f;

    // Cyan halo around the scan ring (opaque ring spans 1.02 … 1.28).
    root->NewChild("scan_glow_inner")
        .at({0.0f, 0.0f, kGlowZ})
        .rot_x(-90.0f)
        .draw_glow(Mesh::Ring(0.96f, 1.36f, 0.02f, 128),
                   Material::GlowCyan(0.55f));
    root->NewChild("scan_glow_outer")
        .at({0.0f, 0.0f, kGlowZ})
        .rot_x(-90.0f)
        .draw_glow(Mesh::Ring(0.84f, 1.52f, 0.02f, 128),
                   Material::GlowCyan(0.26f));

    // Green halo framing the circuit ring (opaque ring spans 0.36 … 0.58).
    // Two thin rims hug the inner and outer edges so the glow frames the
    // circuit traces instead of flooding the whole band.
    root->NewChild("circuit_glow_outer")
        .at({0.0f, 0.0f, kGlowZ})
        .rot_x(-90.0f)
        .draw_glow(Mesh::Ring(0.56f, 0.66f, 0.02f, 96),
                   Material::GlowGreen(0.40f));
    root->NewChild("circuit_glow_inner")
        .at({0.0f, 0.0f, kGlowZ})
        .rot_x(-90.0f)
        .draw_glow(Mesh::Ring(0.30f, 0.38f, 0.02f, 96),
                   Material::GlowGreen(0.35f));

    return root;
}

}  // namespace future_gaze::builders
