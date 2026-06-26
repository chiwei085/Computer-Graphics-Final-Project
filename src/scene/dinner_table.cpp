#include <array>
#include <string>

#include "future_gaze/render/material.hpp"
#include "future_gaze/render/mesh.hpp"
#include "future_gaze/scene/builders.hpp"

namespace future_gaze::builders
{

namespace
{

std::string StoryName(const char* prefix, int target, const char* kind,
                      int slot) {
    return std::string(prefix) + std::to_string(target) + "_" + kind + "_" +
           std::to_string(slot);
}

Material GlowGold(float intensity) {
    Material mat({0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.68f, 0.24f, intensity},
                 {0.0f, 0.0f, 0.0f, 1.0f}, 0.0f);
    mat.SetEmission({1.0f, 0.55f, 0.16f, 1.0f});
    return mat;
}

Material GlowRose(float intensity) {
    Material mat({0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.30f, 0.42f, intensity},
                 {0.0f, 0.0f, 0.0f, 1.0f}, 0.0f);
    mat.SetEmission({0.9f, 0.16f, 0.26f, 1.0f});
    return mat;
}

Material DesaturatedPaper() {
    return Material({0.10f, 0.095f, 0.078f, 1.0f}, {0.60f, 0.56f, 0.47f, 1.0f},
                    {0.05f, 0.05f, 0.04f, 1.0f}, 3.0f);
}

void BuildChair(SceneNode& root, Vec3 base, float yaw_deg,
                const Material& upholstery, const Material& leg_mat,
                const Texture* cloth_tex, const Texture* metal_tex,
                int memory_target = -1) {
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

    // Longing/private-front kit. The pieces are authored fully open, then
    // SceneAnimation collapses them until the Prediction Core aligns with the
    // chair. Each chair gets a different grammar: body trace, childhood fort,
    // or argument slashes.
    if (memory_target >= 0) {
        chair
            .NewChild(
                StoryName(names::kMemoryPrefix, memory_target, "panel", 0))
            .at({0.0f, 0.66f, 0.105f})
            .scaled({0.27f, 0.46f, 0.012f})
            .draw_glow(Mesh::Cube(), GlowGold(0.24f));

        if (memory_target == 0) {
            chair
                .NewChild(
                    StoryName(names::kMemoryPrefix, memory_target, "figure", 0))
                .at({0.0f, 0.86f, 0.095f})
                .scaled({0.078f, 0.078f, 0.012f})
                .draw_glow(Mesh::Sphere(1.0f, 12, 8), GlowGold(0.64f));
            chair
                .NewChild(
                    StoryName(names::kMemoryPrefix, memory_target, "figure", 1))
                .at({0.0f, 0.70f, 0.098f})
                .scaled({0.115f, 0.175f, 0.012f})
                .draw_glow(Mesh::Cube(), GlowGold(0.50f));
            chair
                .NewChild(
                    StoryName(names::kMemoryPrefix, memory_target, "figure", 2))
                .at({-0.070f, 0.64f, 0.097f})
                .rot_z(-18.0f)
                .scaled({0.022f, 0.145f, 0.012f})
                .draw_glow(Mesh::Cube(), GlowGold(0.45f));
            chair
                .NewChild(
                    StoryName(names::kMemoryPrefix, memory_target, "figure", 3))
                .at({0.070f, 0.64f, 0.097f})
                .rot_z(18.0f)
                .scaled({0.022f, 0.145f, 0.012f})
                .draw_glow(Mesh::Cube(), GlowGold(0.45f));
        }
        else if (memory_target == 1) {
            chair
                .NewChild(
                    StoryName(names::kMemoryPrefix, memory_target, "fort", 0))
                .at({0.0f, 0.76f, 0.100f})
                .rot_z(45.0f)
                .scaled({0.125f, 0.024f, 0.012f})
                .draw_glow(Mesh::Cube(), Material::GlowGreen(0.66f));
            chair
                .NewChild(
                    StoryName(names::kMemoryPrefix, memory_target, "fort", 1))
                .at({0.0f, 0.76f, 0.100f})
                .rot_z(-45.0f)
                .scaled({0.125f, 0.024f, 0.012f})
                .draw_glow(Mesh::Cube(), Material::GlowGreen(0.66f));
            chair
                .NewChild(
                    StoryName(names::kMemoryPrefix, memory_target, "fort", 2))
                .at({-0.125f, 0.61f, 0.100f})
                .scaled({0.024f, 0.180f, 0.012f})
                .draw_glow(Mesh::Cube(), Material::GlowGreen(0.50f));
            chair
                .NewChild(
                    StoryName(names::kMemoryPrefix, memory_target, "fort", 3))
                .at({0.125f, 0.61f, 0.100f})
                .scaled({0.024f, 0.180f, 0.012f})
                .draw_glow(Mesh::Cube(), Material::GlowGreen(0.50f));
            chair
                .NewChild(
                    StoryName(names::kMemoryPrefix, memory_target, "fort", 4))
                .at({0.0f, 0.55f, 0.103f})
                .scaled({0.145f, 0.022f, 0.012f})
                .draw_glow(Mesh::Cube(), Material::GlowGreen(0.46f));
        }
        else {
            for (int i = 0; i < 4; ++i) {
                const float x = -0.105f + 0.070f * static_cast<float>(i);
                const float tilt = (i % 2 == 0) ? -24.0f : 24.0f;
                chair
                    .NewChild(StoryName(names::kMemoryPrefix, memory_target,
                                        "argument", i))
                    .at({x, 0.66f + 0.050f * static_cast<float>(i % 2), 0.102f})
                    .rot_z(tilt)
                    .scaled({0.020f, 0.220f, 0.012f})
                    .draw_glow(Mesh::Cube(), GlowRose(0.68f));
            }
        }
    }
}

// A vertical stack of holographic "future-state" slices rising out of a wine
// glass. Built collapsed at the rim; the SceneAnimation fans them open with
// staggered phase so the lowest (most probable future) slice is the thickest
// and the column appears to bloom.
void BuildFutureStack(SceneNode& glass, int target) {
    constexpr int kSlices = 5;
    auto& stack = glass.NewChild("future_stack").at({0.0f, 0.30f, 0.0f});
    for (int i = 0; i < kSlices; ++i) {
        // Lower slices are wider/brighter (higher probability); upper slices
        // thin out into speculative futures. Slabs (not disks) so the glow
        // reads as a solid bar from any orbit angle.
        const float f = static_cast<float>(i) / static_cast<float>(kSlices - 1);
        const float half = 0.100f - 0.052f * f;
        const float intensity = 0.85f - 0.45f * f;
        stack.NewChild(StoryName(names::kFuturePrefix, target, "slice", i))
            .at({0.0f, 0.0f, 0.0f})
            .scaled({half, 0.012f, half})
            .draw_glow(Mesh::Cube(), Material::GlowCyan(intensity));
    }

    // A falling-arc prediction: several thin cyan bars lean out of the current
    // glass, showing the likely path before the glass has actually moved.
    for (int i = 0; i < 4; ++i) {
        const float f = static_cast<float>(i);
        glass.NewChild(StoryName(names::kFuturePrefix, target, "trajectory", i))
            .at({0.08f + 0.055f * f, 0.26f + 0.030f * f, 0.030f + 0.018f * f})
            .rot_z(-18.0f - 8.0f * f)
            .rot_y(12.0f * f)
            .scaled({0.020f, 0.105f + 0.020f * f, 0.012f})
            .draw_glow(Mesh::Cube(), Material::GlowCyan(0.42f - 0.055f * f));
    }

    // Water stains are future consequences appearing early on the table plane.
    for (int i = 0; i < 2; ++i) {
        glass.NewChild(StoryName(names::kFuturePrefix, target, "stain", i))
            .at({0.18f + 0.095f * static_cast<float>(i), 0.004f,
                 0.070f + 0.045f * static_cast<float>(i)})
            .rot_y(18.0f + 23.0f * static_cast<float>(i))
            .scaled({0.15f - 0.035f * static_cast<float>(i), 1.0f,
                     0.080f - 0.018f * static_cast<float>(i)})
            .draw_glow(Mesh::Disk(1.0f, 32),
                       Material::GlowCyan(0.30f - 0.065f * i));
    }

    // Possible shards sit farther along the predicted fall, small and sharp so
    // the future reads as multiple incompatible outcomes.
    for (int i = 0; i < 5; ++i) {
        const float f = static_cast<float>(i);
        glass.NewChild(StoryName(names::kFuturePrefix, target, "shard", i))
            .at({0.28f + 0.060f * f, 0.010f, -0.050f + 0.045f * f})
            .rot_y(24.0f * f)
            .rot_z(-18.0f + 9.0f * f)
            .scaled({0.040f - 0.004f * f, 0.004f, 0.014f + 0.003f * f})
            .draw_glow(Mesh::Cube(), Material::GlowCyan(0.34f - 0.035f * f));
    }
}

void BuildPlaceSetting(SceneNode& root, float x, float z,
                       const Material& plate_mat, const Material& glass_mat,
                       const Texture* marble_tex, int target) {
    root.NewChild("plate")
        .at({x, 0.802f, z})
        .draw_as(Mesh::Disk(0.14f), plate_mat, marble_tex);
    auto& glass = root.NewChild("glass").at({x + 0.18f, 0.802f, z - 0.14f});
    glass.NewChild("stem").draw_as(Mesh::Cylinder(0.012f, 0.18f), glass_mat);
    glass.NewChild("bowl")
        .at({0.0f, 0.18f, 0.0f})
        .draw_as(Mesh::Frustum(0.02f, 0.075f, 0.10f), glass_mat);
    BuildFutureStack(glass, target);
}

void BuildBlindArtifacts(SceneNode& root, const Texture* paper_tex,
                         const Texture* wood_tex) {
    const Material paper = DesaturatedPaper();
    const Material photo({0.04f, 0.045f, 0.050f, 1.0f},
                         {0.28f, 0.31f, 0.34f, 1.0f},
                         {0.04f, 0.04f, 0.045f, 1.0f}, 5.0f);
    const Material leaf({0.035f, 0.080f, 0.038f, 1.0f},
                        {0.18f, 0.36f, 0.16f, 1.0f},
                        {0.03f, 0.05f, 0.03f, 1.0f}, 4.0f);
    const Material crack({0.010f, 0.009f, 0.008f, 1.0f},
                         {0.035f, 0.030f, 0.025f, 1.0f},
                         {0.0f, 0.0f, 0.0f, 1.0f}, 1.0f);

    // Unopened letter under the table: detailed, quiet, and unaffected by the
    // prediction/longing deformation system.
    root.NewChild("blind_unopened_letter")
        .at({-0.34f, 0.018f, 0.54f})
        .rot_y(-18.0f)
        .scaled({0.150f, 0.0025f, 0.090f})
        .draw_as(Mesh::Cube(), paper, paper_tex);
    root.NewChild("blind_letter_fold")
        .at({-0.34f, 0.025f, 0.54f})
        .rot_y(-18.0f)
        .scaled({0.008f, 0.003f, 0.086f})
        .draw_as(Mesh::Cube(), crack);

    // A fallen photo with a small pale centre, still legible as an object but
    // deliberately not glowing.
    root.NewChild("blind_photo_back")
        .at({0.42f, 0.016f, 0.64f})
        .rot_y(23.0f)
        .scaled({0.100f, 0.002f, 0.070f})
        .draw_as(Mesh::Cube(), photo);
    root.NewChild("blind_photo_core")
        .at({0.42f, 0.021f, 0.64f})
        .rot_y(23.0f)
        .scaled({0.064f, 0.002f, 0.040f})
        .draw_as(Mesh::Cube(), paper, paper_tex);

    // A small plant growing from a floor crack, the only organic future that is
    // not predicted by the AI.
    root.NewChild("blind_floor_crack")
        .at({-0.68f, 0.010f, 0.78f})
        .rot_y(8.0f)
        .scaled({0.185f, 0.002f, 0.012f})
        .draw_as(Mesh::Cube(), crack);
    root.NewChild("blind_plant_stem")
        .at({-0.68f, 0.012f, 0.78f})
        .rot_z(-12.0f)
        .draw_as(Mesh::Cylinder(0.008f, 0.16f, 10), leaf);
    root.NewChild("blind_plant_leaf_l")
        .at({-0.715f, 0.105f, 0.78f})
        .rot_z(42.0f)
        .scaled({0.028f, 0.007f, 0.014f})
        .draw_as(Mesh::Sphere(1.0f, 10, 6), leaf);
    root.NewChild("blind_plant_leaf_r")
        .at({-0.640f, 0.130f, 0.78f})
        .rot_z(-36.0f)
        .scaled({0.032f, 0.008f, 0.015f})
        .draw_as(Mesh::Sphere(1.0f, 10, 6), leaf);

    // The table leg has a hairline fracture. It is narratively important
    // because it is not converted into a prediction effect.
    root.NewChild("blind_table_leg_crack_a")
        .at({1.98f, 0.41f, 0.884f})
        .rot_z(15.0f)
        .scaled({0.006f, 0.080f, 0.003f})
        .draw_as(Mesh::Cube(), crack, wood_tex);
    root.NewChild("blind_table_leg_crack_b")
        .at({1.98f, 0.31f, 0.886f})
        .rot_z(-18.0f)
        .scaled({0.005f, 0.070f, 0.003f})
        .draw_as(Mesh::Cube(), crack, wood_tex);
}

}  // namespace

SceneNode& BuildDinnerTable(SceneNode& parent, const TextureSet& tex) {
    auto& root = parent.NewChild("dinner_scene");

    const Material wood = Material::Wood();
    const Material dark_wood = Material::DarkWood();
    const Material metal = Material::Metal();
    const Material porcelain = Material::Porcelain();
    const Material glass_mat = Material::Glass();
    const Material cloth = Material::Cloth();

    // ── TABLE
    // ─────────────────────────────────────────────────────────────────
    {
        auto& table = root.NewChild("table");
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
    BuildChair(root, {-1.20f, 0.0f, -1.38f}, -5.0f, cloth, metal, tex.cloth,
               tex.metal);
    BuildChair(root, {0.00f, 0.0f, -1.50f}, 0.0f, cloth, metal, tex.cloth,
               tex.metal);
    BuildChair(root, {1.20f, 0.0f, -1.35f}, 7.0f, cloth, metal, tex.cloth,
               tex.metal);
    BuildChair(root, {-1.20f, 0.0f, 1.35f}, 185.0f, cloth, metal, tex.cloth,
               tex.metal, 0);
    BuildChair(root, {0.00f, 0.0f, 1.22f}, 180.0f, cloth, metal, tex.cloth,
               tex.metal, 1);
    BuildChair(root, {1.20f, 0.0f, 1.38f}, 174.0f, cloth, metal, tex.cloth,
               tex.metal, 2);

    // ── PLACE SETTINGS
    // ────────────────────────────────────────────────────────
    BuildPlaceSetting(root, -1.20f, 0.0f, porcelain, glass_mat, tex.marble, 0);
    BuildPlaceSetting(root, 0.00f, 0.0f, porcelain, glass_mat, tex.marble, 1);
    BuildPlaceSetting(root, 1.20f, 0.0f, porcelain, glass_mat, tex.marble, 2);

    // ── CAKE
    // ──────────────────────────────────────────────────────────────────
    {
        const Material cake_mat({0.20f, 0.12f, 0.08f, 1.0f},
                                {0.90f, 0.78f, 0.65f, 1.0f},
                                {0.25f, 0.18f, 0.12f, 1.0f}, 12.0f);
        const Material slice_mat({0.18f, 0.10f, 0.06f, 1.0f},
                                 {0.82f, 0.68f, 0.52f, 1.0f},
                                 {0.12f, 0.08f, 0.06f, 1.0f}, 6.0f);
        auto& cake = root.NewChild("cake").at({0.55f, 0.802f, -0.42f});
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
        auto& bouquet = root.NewChild("bouquet").at({-1.72f, 0.802f, -0.28f});
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
    root.NewChild("letter")
        .at({-0.08f, 0.800f, 0.12f})
        .rot_y(14.0f)
        .scaled({0.092f, 0.0015f, 0.125f})
        .draw_as(
            Mesh::Cube(),
            Material({0.16f, 0.15f, 0.12f, 1.0f}, {0.92f, 0.90f, 0.82f, 1.0f},
                     {0.08f, 0.08f, 0.06f, 1.0f}, 4.0f),
            tex.paper);

    BuildBlindArtifacts(root, tex.paper, tex.wood);

    return root;
}

}  // namespace future_gaze::builders
