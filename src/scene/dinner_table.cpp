#include <array>

#include "future_gaze/render/material.hpp"
#include "future_gaze/render/mesh.hpp"
#include "future_gaze/scene/builders.hpp"
#include "future_gaze/scene/node_names.hpp"

namespace future_gaze::builders
{

namespace
{

void BuildChair(SceneNode& root, Vec3 base, float yaw_deg,
                const Material& upholstery, const Material& leg_mat,
                const Texture* cloth_tex, const Texture* metal_tex,
                bool memory_panel = false) {
    auto& chair = root.NewChild("chair").at(base).rot_y(yaw_deg);
    chair.NewChild("seat")
        .at({0.0f, 0.45f, 0.0f})
        .scaled({0.22f, 0.022f, 0.22f})
        .draw_as(Mesh::Cube(), upholstery, cloth_tex);
    chair.NewChild("back")
        .at({0.0f, 0.70f, -0.20f})
        .scaled({0.22f, 0.25f, 0.022f})
        .draw_as(Mesh::Cube(), upholstery, cloth_tex);
    const std::array<Vec3, 4> lp = {
        Vec3{-0.18f, 0.0f, -0.18f}, Vec3{0.18f, 0.0f, -0.18f},
        Vec3{-0.18f, 0.0f, 0.18f}, Vec3{0.18f, 0.0f, 0.18f}};
    for (const auto& p : lp) {
        chair.NewChild("leg").at(p).draw_as(Mesh::Cylinder(0.020f, 0.44f),
                                            leg_mat, metal_tex);
    }

    // Anamorphic memory panel: a holographic slab that unfolds from the empty
    // seat toward the absent diner. Built closed (scale.y = 0) above the seat;
    // the SceneAnimation channel keyframes it open and breathes it. Drawn as an
    // additive glow so it reads as a projected "private front", not a solid
    // object.
    if (memory_panel) {
        chair.NewChild(names::kMemoryPanel)
            .at({0.0f, 0.55f, 0.07f})
            .scaled({0.20f, 0.0f, 0.010f})
            .draw_glow(Mesh::Cube(), Material::GlowGreen(0.62f));
    }
}

// A vertical stack of holographic "future-state" slices rising out of a wine
// glass. Built collapsed at the rim; the SceneAnimation fans them open with
// staggered phase so the lowest (most probable future) slice is the thickest
// and the column appears to bloom.
void BuildFutureStack(SceneNode& glass) {
    constexpr int kSlices = 5;
    auto& stack = glass.NewChild("future_stack").at({0.0f, 0.30f, 0.0f});
    for (int i = 0; i < kSlices; ++i) {
        // Lower slices are wider/brighter (higher probability); upper slices
        // thin out into speculative futures. Slabs (not disks) so the glow
        // reads as a solid bar from any orbit angle.
        const float f = static_cast<float>(i) / static_cast<float>(kSlices - 1);
        const float half = 0.100f - 0.052f * f;
        const float intensity = 0.85f - 0.45f * f;
        stack.NewChild(names::kFutureSlice)
            .at({0.0f, 0.0f, 0.0f})
            .scaled({half, 0.012f, half})
            .draw_glow(Mesh::Cube(), Material::GlowCyan(intensity));
    }
}

void BuildPlaceSetting(SceneNode& root, float x, float z,
                       const Material& plate_mat, const Material& glass_mat,
                       const Texture* marble_tex) {
    root.NewChild("plate")
        .at({x, 0.802f, z})
        .draw_as(Mesh::Disk(0.14f), plate_mat, marble_tex);
    auto& glass = root.NewChild("glass").at({x + 0.18f, 0.802f, z - 0.14f});
    glass.NewChild("stem").draw_as(Mesh::Cylinder(0.012f, 0.18f), glass_mat);
    glass.NewChild("bowl")
        .at({0.0f, 0.18f, 0.0f})
        .draw_as(Mesh::Frustum(0.02f, 0.075f, 0.10f), glass_mat);
    BuildFutureStack(glass);
}

}  // namespace

std::unique_ptr<SceneNode> BuildDinnerTable(const TextureSet& tex) {
    auto root = std::make_unique<SceneNode>("dinner_scene");

    const Material wood = Material::Wood();
    const Material dark_wood = Material::DarkWood();
    const Material metal = Material::Metal();
    const Material porcelain = Material::Porcelain();
    const Material glass_mat = Material::Glass();
    const Material cloth = Material::Cloth();

    // ── TABLE
    // ─────────────────────────────────────────────────────────────────
    {
        auto& table = root->NewChild("table");
        table.NewChild("top")
            .at({0.0f, 0.76f, 0.0f})
            .scaled({2.1f, 0.04f, 0.95f})
            .draw_as(Mesh::Cube(), wood, tex.wood);

        const std::array<Vec3, 4> leg_pos = {
            Vec3{-1.98f, 0.0f, -0.88f}, Vec3{1.98f, 0.0f, -0.88f},
            Vec3{-1.98f, 0.0f, 0.88f}, Vec3{1.98f, 0.0f, 0.88f}};
        for (const auto& p : leg_pos) {
            table.NewChild("leg").at(p).draw_as(Mesh::Cylinder(0.045f, 0.72f),
                                                dark_wood, tex.wood);
        }
    }

    // ── CHAIRS
    // ────────────────────────────────────────────────────────────────
    BuildChair(*root, {-1.20f, 0.0f, -1.38f}, -5.0f, cloth, metal, tex.cloth,
               tex.metal);
    BuildChair(*root, {0.00f, 0.0f, -1.50f}, 0.0f, cloth, metal, tex.cloth,
               tex.metal);
    BuildChair(*root, {1.20f, 0.0f, -1.35f}, 7.0f, cloth, metal, tex.cloth,
               tex.metal);
    BuildChair(*root, {-1.20f, 0.0f, 1.35f}, 185.0f, cloth, metal, tex.cloth,
               tex.metal, true);
    BuildChair(*root, {0.00f, 0.0f, 1.22f}, 180.0f, cloth, metal, tex.cloth,
               tex.metal, true);
    BuildChair(*root, {1.20f, 0.0f, 1.38f}, 174.0f, cloth, metal, tex.cloth,
               tex.metal, true);

    // ── PLACE SETTINGS
    // ────────────────────────────────────────────────────────
    BuildPlaceSetting(*root, -1.20f, 0.0f, porcelain, glass_mat, tex.marble);
    BuildPlaceSetting(*root, 0.00f, 0.0f, porcelain, glass_mat, tex.marble);
    BuildPlaceSetting(*root, 1.20f, 0.0f, porcelain, glass_mat, tex.marble);

    // ── CAKE
    // ──────────────────────────────────────────────────────────────────
    {
        const Material cake_mat({0.20f, 0.12f, 0.08f, 1.0f},
                                {0.90f, 0.78f, 0.65f, 1.0f},
                                {0.25f, 0.18f, 0.12f, 1.0f}, 12.0f);
        const Material slice_mat({0.18f, 0.10f, 0.06f, 1.0f},
                                 {0.82f, 0.68f, 0.52f, 1.0f},
                                 {0.12f, 0.08f, 0.06f, 1.0f}, 6.0f);
        auto& cake = root->NewChild("cake").at({0.55f, 0.802f, -0.42f});
        cake.NewChild("body").draw_as(Mesh::Cylinder(0.16f, 0.10f), cake_mat);
        cake.NewChild("cut1")
            .at({0.085f, 0.05f, 0.0f})
            .rot_y(20.0f)
            .scaled({0.085f, 0.050f, 0.003f})
            .draw_as(Mesh::Cube(), slice_mat);
        cake.NewChild("cut2")
            .at({0.085f, 0.05f, 0.0f})
            .rot_y(-20.0f)
            .scaled({0.085f, 0.050f, 0.003f})
            .draw_as(Mesh::Cube(), slice_mat);
    }

    // ── FLOWER BOUQUET
    // ────────────────────────────────────────────────────────
    {
        const Material pink({0.15f, 0.06f, 0.08f, 1.0f},
                            {0.88f, 0.60f, 0.65f, 1.0f},
                            {0.20f, 0.08f, 0.10f, 1.0f}, 12.0f);
        const Material white_petal({0.18f, 0.16f, 0.15f, 1.0f},
                                   {0.94f, 0.92f, 0.88f, 1.0f},
                                   {0.18f, 0.16f, 0.14f, 1.0f}, 8.0f);
        auto& bouquet = root->NewChild("bouquet").at({-1.72f, 0.802f, -0.28f});
        bouquet.NewChild("vase").draw_as(Mesh::Frustum(0.042f, 0.030f, 0.12f),
                                         Material::Chrome());
        const std::array<Vec3, 5> heads = {
            Vec3{0.00f, 0.22f, 0.00f}, Vec3{0.04f, 0.28f, 0.03f},
            Vec3{-0.03f, 0.25f, 0.04f}, Vec3{0.05f, 0.20f, -0.05f},
            Vec3{-0.04f, 0.31f, -0.02f}};
        for (int i = 0; i < 5; ++i) {
            bouquet.NewChild("flower").at(heads[i]).draw_as(
                Mesh::Sphere(0.052f, 10, 7), (i % 2 == 0) ? pink : white_petal);
        }
    }

    // ── LETTER (paper texture)
    // ───────────────────────────────────────────────
    root->NewChild("letter")
        .at({-0.08f, 0.800f, 0.12f})
        .rot_y(14.0f)
        .scaled({0.092f, 0.0015f, 0.125f})
        .draw_as(
            Mesh::Cube(),
            Material({0.16f, 0.15f, 0.12f, 1.0f}, {0.92f, 0.90f, 0.82f, 1.0f},
                     {0.08f, 0.08f, 0.06f, 1.0f}, 4.0f),
            tex.paper);

    return root;
}

}  // namespace future_gaze::builders
