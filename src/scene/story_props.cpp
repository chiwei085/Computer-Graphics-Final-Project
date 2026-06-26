#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "future_gaze/render/material.hpp"
#include "future_gaze/render/mesh.hpp"
#include "future_gaze/scene/builders.hpp"
#include "future_gaze/scene/node_names.hpp"
#include "future_gaze/scene/room_bounds.hpp"

namespace future_gaze::builders
{

namespace
{

std::string StoryName(const char* prefix, int target, std::string_view kind,
                      std::string_view role, int slot) {
    return std::string(prefix) + std::to_string(target) + "_" +
           std::string(kind) + "_" + std::string(role) + "_" +
           std::to_string(slot);
}

void TuneFutureDevice(Material& mat) {
    const Material::Color d = mat.diffuse();
    mat.SetAmbient({d[0] * 0.12f, d[1] * 0.16f, d[2] * 0.22f, 1.0f});
    mat.SetDiffuse({d[0] * 0.62f + 0.04f, d[1] * 0.70f + 0.08f,
                    d[2] * 0.94f + 0.16f, 1.0f});
    mat.SetSpecular({0.62f, 0.82f, 0.90f, 1.0f});
    mat.SetShininess(96.0f);
}

void TuneFutureProp(Material& mat) {
    const Material::Color d = mat.diffuse();
    mat.SetAmbient({d[0] * 0.18f, d[1] * 0.20f, d[2] * 0.24f, 1.0f});
    mat.SetDiffuse({d[0] * 0.72f + 0.05f, d[1] * 0.75f + 0.07f,
                    d[2] * 0.84f + 0.10f, 1.0f});
    mat.SetSpecular({0.24f, 0.34f, 0.38f, 1.0f});
    mat.SetShininess(26.0f);
}

void TuneMemoryWarm(Material& mat) {
    const Material::Color d = mat.diffuse();
    mat.SetAmbient({0.16f + d[0] * 0.10f, 0.12f + d[1] * 0.08f,
                    0.08f + d[2] * 0.05f, 1.0f});
    mat.SetDiffuse({d[0] * 0.62f + 0.32f, d[1] * 0.54f + 0.26f,
                    d[2] * 0.45f + 0.16f, 1.0f});
    mat.SetSpecular({0.18f, 0.13f, 0.09f, 1.0f});
    mat.SetShininess(12.0f);
}

void TuneMemoryGreen(Material& mat) {
    const Material::Color d = mat.diffuse();
    mat.SetAmbient({0.08f + d[0] * 0.08f, 0.15f + d[1] * 0.10f,
                    0.10f + d[2] * 0.06f, 1.0f});
    mat.SetDiffuse({d[0] * 0.48f + 0.20f, d[1] * 0.58f + 0.36f,
                    d[2] * 0.44f + 0.22f, 1.0f});
    mat.SetSpecular({0.10f, 0.18f, 0.12f, 1.0f});
    mat.SetShininess(10.0f);
}

void TuneMemoryRose(Material& mat) {
    const Material::Color d = mat.diffuse();
    mat.SetAmbient({0.17f + d[0] * 0.08f, 0.08f + d[1] * 0.05f,
                    0.10f + d[2] * 0.06f, 1.0f});
    mat.SetDiffuse({d[0] * 0.56f + 0.34f, d[1] * 0.42f + 0.18f,
                    d[2] * 0.44f + 0.22f, 1.0f});
    mat.SetSpecular({0.20f, 0.10f, 0.13f, 1.0f});
    mat.SetShininess(14.0f);
}

void TuneBlindObject(Material& mat) {
    const Material::Color d = mat.diffuse();
    mat.SetAmbient({d[0] * 0.16f, d[1] * 0.15f, d[2] * 0.13f, 1.0f});
    mat.SetDiffuse({d[0] * 0.42f + 0.12f, d[1] * 0.42f + 0.11f,
                    d[2] * 0.38f + 0.09f, 1.0f});
    mat.SetSpecular({0.045f, 0.040f, 0.035f, 1.0f});
    mat.SetShininess(3.0f);
}

void TuneFutureBackdrop(Material& mat) {
    const Material::Color d = mat.diffuse();
    mat.SetAmbient({d[0] * 0.08f, d[1] * 0.12f, d[2] * 0.18f, 1.0f});
    mat.SetDiffuse({d[0] * 0.42f + 0.03f, d[1] * 0.54f + 0.08f,
                    d[2] * 0.82f + 0.18f, 1.0f});
    mat.SetSpecular({0.68f, 0.92f, 1.0f, 1.0f});
    mat.SetShininess(110.0f);
}

void TuneMemoryBackdrop(Material& mat) {
    const Material::Color d = mat.diffuse();
    mat.SetAmbient({0.18f + d[0] * 0.12f, 0.09f + d[1] * 0.08f,
                    0.06f + d[2] * 0.05f, 1.0f});
    mat.SetDiffuse({d[0] * 0.58f + 0.36f, d[1] * 0.42f + 0.20f,
                    d[2] * 0.36f + 0.14f, 1.0f});
    mat.SetSpecular({0.28f, 0.16f, 0.10f, 1.0f});
    mat.SetShininess(18.0f);
}

void AddModelParts(SceneNode& parent, std::vector<ModelMesh> meshes,
                   void (*tune)(Material&)) {
    for (auto& mm : meshes) {
        tune(mm.material);
        parent.NewChild("part").draw_as(std::move(mm.mesh), mm.material);
    }
}

Material GlowGold(float intensity) {
    Material mat({0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.66f, 0.24f, intensity},
                 {0.0f, 0.0f, 0.0f, 1.0f}, 0.0f);
    mat.SetEmission({0.85f, 0.42f, 0.12f, 1.0f});
    return mat;
}

Material GlowRose(float intensity) {
    Material mat({0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.28f, 0.42f, intensity},
                 {0.0f, 0.0f, 0.0f, 1.0f}, 0.0f);
    mat.SetEmission({0.78f, 0.13f, 0.22f, 1.0f});
    return mat;
}

Material WarmCanvas() {
    Material mat({0.090f, 0.055f, 0.030f, 1.0f}, {0.42f, 0.25f, 0.12f, 1.0f},
                 {0.040f, 0.025f, 0.015f, 1.0f}, 4.0f);
    mat.SetEmission({0.055f, 0.030f, 0.012f, 1.0f});
    return mat;
}

Material RoseCanvas() {
    Material mat({0.085f, 0.035f, 0.040f, 1.0f}, {0.36f, 0.15f, 0.18f, 1.0f},
                 {0.035f, 0.015f, 0.018f, 1.0f}, 4.0f);
    mat.SetEmission({0.045f, 0.014f, 0.018f, 1.0f});
    return mat;
}

void AddFutureEdge(SceneNode& prop, float radius, float intensity, float yaw) {
    prop.NewChild("cyan_probability_rim")
        .at({0.0f, -0.002f, 0.0f})
        .rot_y(yaw)
        .scaled({radius, 1.0f, radius * 0.44f})
        .draw_glow(Mesh::Disk(1.0f, 40), Material::GlowCyan(intensity));
}

void AddMemoryThread(SceneNode& prop, Material mat, float height, float yaw) {
    prop.NewChild("private_front_thread")
        .at({0.0f, height, 0.0f})
        .rot_y(yaw)
        .scaled({0.018f, 0.32f, 0.010f})
        .draw_glow(Mesh::Cube(), mat);
}

void AddFutureDataColumn(SceneNode& root, int target, Vec3 position,
                         float height, int slot) {
    auto& column = root.NewChild(StoryName(names::kFuturePrefix, target,
                                           "branch", "data_column", slot))
                       .at(position)
                       .scaled({1.0f, 1.0f, 1.0f});
    column.NewChild("glass_core")
        .at({0.0f, height * 0.5f, 0.0f})
        .draw_glow(Mesh::Cylinder(0.030f, height, 18),
                   Material::GlowCyan(0.28f));
    for (int i = 0; i < 5; ++i) {
        const float y = 0.22f + static_cast<float>(i) * height * 0.17f;
        column.NewChild("probability_tick")
            .at({0.0f, y, 0.0f})
            .rot_y(34.0f * static_cast<float>(i))
            .scaled({0.18f - 0.020f * static_cast<float>(i), 0.006f, 0.010f})
            .draw_glow(Mesh::Cube(), Material::GlowCyan(0.38f - 0.035f * i));
    }
}

void AddFuturePredictionLab(SceneNode& root, StoryPropAssets& assets) {
    const Material dark_metal({0.018f, 0.026f, 0.034f, 1.0f},
                              {0.070f, 0.120f, 0.150f, 1.0f},
                              {0.35f, 0.55f, 0.62f, 1.0f}, 72.0f);

    if (!assets.cyberpunk_platform.empty()) {
        auto& platform = root.NewChild(StoryName(names::kFuturePrefix, 0,
                                                 "branch", "cyber_platform", 5))
                             .at({-2.70f, 0.035f, -1.84f})
                             .rot_y(-18.0f)
                             .scaled({1.35f, 1.35f, 1.35f});
        AddFutureEdge(platform, 1.25f, 0.20f, 0.0f);
        AddModelParts(platform, std::move(assets.cyberpunk_platform),
                      TuneFutureBackdrop);
    }

    if (!assets.sci_fi_floor_tile.empty()) {
        for (int i = 0; i < 3; ++i) {
            auto& tile = root.NewChild(StoryName(names::kFuturePrefix, i,
                                                 "branch", "floor_tile", 6))
                             .at({-2.15f + 2.15f * static_cast<float>(i),
                                  0.018f, -2.02f})
                             .rot_y(90.0f * static_cast<float>(i % 2))
                             .scaled({0.72f, 0.72f, 0.72f});
            if (i == 0) {
                AddModelParts(tile, std::move(assets.sci_fi_floor_tile),
                              TuneFutureBackdrop);
            }
            else {
                tile.NewChild("procedural_tile")
                    .scaled({0.74f, 0.010f, 0.74f})
                    .draw_as(Mesh::Cube(), dark_metal);
            }
            AddFutureEdge(tile, 0.64f, 0.18f, 45.0f);
        }
    }

    if (!assets.sci_fi_wall_3.empty()) {
        auto& wall = root.NewChild(StoryName(names::kFuturePrefix, 1, "branch",
                                             "sci_wall", 7))
                         .at({2.62f, 1.34f, -1.92f})
                         .rot_y(160.0f)
                         .scaled({1.32f, 1.32f, 1.32f});
        AddModelParts(wall, std::move(assets.sci_fi_wall_3),
                      TuneFutureBackdrop);
        AddFutureEdge(wall, 1.08f, 0.16f, 0.0f);
    }

    for (int i = 0; i < 4; ++i) {
        const float x = -3.06f + 2.04f * static_cast<float>(i);
        root.NewChild(StoryName(names::kFuturePrefix, i % 3, "branch",
                                "scan_grid", 8 + i))
            .at({x, 1.42f, -1.78f})
            .rot_y(0.0f)
            .scaled({0.026f, 0.86f, 0.018f})
            .draw_glow(Mesh::Cube(), Material::GlowCyan(0.40f));
    }
    AddFutureDataColumn(root, 0, {-3.18f, 0.04f, -1.56f}, 1.74f, 5);
    AddFutureDataColumn(root, 1, {-2.20f, 0.04f, -1.92f}, 1.52f, 6);
    AddFutureDataColumn(root, 2, {2.18f, 0.04f, -1.92f}, 1.52f, 7);
    AddFutureDataColumn(root, 2, {3.18f, 0.04f, -1.56f}, 1.74f, 8);

    for (int i = 0; i < 3; ++i) {
        const float x = -2.40f + 2.40f * static_cast<float>(i);
        root.NewChild(StoryName(names::kFuturePrefix, i, "branch",
                                "overhead_lane", 5 + i))
            .at({x, 2.34f, -1.70f})
            .rot_z(90.0f)
            .scaled({0.018f, 0.76f, 0.018f})
            .draw_glow(Mesh::Cube(), Material::GlowCyan(0.34f));
    }
}

void AddRomanticMemorySet(SceneNode& root, StoryPropAssets& assets) {
    const Material dusk_wall({0.105f, 0.070f, 0.070f, 1.0f},
                             {0.38f, 0.22f, 0.20f, 1.0f},
                             {0.08f, 0.05f, 0.04f, 1.0f}, 8.0f);
    const Material warm_paper({0.18f, 0.13f, 0.08f, 1.0f},
                              {0.72f, 0.52f, 0.32f, 1.0f},
                              {0.08f, 0.055f, 0.035f, 1.0f}, 4.0f);

    auto& window = root.NewChild(StoryName(names::kMemoryPrefix, 1, "relic",
                                           "memory_window", 4))
                       .at({0.0f, 1.28f, 2.28f})
                       .rot_y(180.0f);
    window.NewChild("back_wall")
        .scaled({1.28f, 0.74f, 0.030f})
        .draw_as(Mesh::Cube(), dusk_wall);
    window.NewChild("left_frame")
        .at({-0.47f, 0.0f, -0.036f})
        .scaled({0.028f, 0.70f, 0.022f})
        .draw_as(Mesh::Cube(), warm_paper);
    window.NewChild("right_frame")
        .at({0.47f, 0.0f, -0.036f})
        .scaled({0.028f, 0.70f, 0.022f})
        .draw_as(Mesh::Cube(), warm_paper);
    window.NewChild("cross_frame")
        .at({0.0f, 0.0f, -0.040f})
        .scaled({0.50f, 0.020f, 0.020f})
        .draw_as(Mesh::Cube(), warm_paper);
    window.NewChild("soft_square")
        .at({0.0f, 0.0f, -0.055f})
        .scaled({0.42f, 0.52f, 0.008f})
        .draw_as(Mesh::Cube(), WarmCanvas());
    AddMemoryThread(window, GlowGold(0.32f), 0.18f, 0.0f);

    if (!assets.curtains_double.empty()) {
        auto& curtains = root.NewChild(StoryName(names::kMemoryPrefix, 1,
                                                 "relic", "curtains", 5))
                             .at({0.0f, 1.24f, 2.16f})
                             .rot_y(180.0f)
                             .scaled({0.88f, 0.88f, 0.88f});
        AddMemoryThread(curtains, GlowRose(0.30f), 0.22f, 0.0f);
        AddModelParts(curtains, std::move(assets.curtains_double),
                      TuneMemoryBackdrop);
    }

    if (!assets.lamp_with_shade.empty()) {
        auto& lamp = root.NewChild(StoryName(names::kMemoryPrefix, 0, "relic",
                                             "standing_lamp", 6))
                         .at({-1.92f, 0.08f, 1.96f})
                         .rot_y(20.0f)
                         .scaled({0.72f, 0.72f, 0.72f});
        AddMemoryThread(lamp, GlowGold(0.44f), 0.58f, 0.0f);
        AddModelParts(lamp, std::move(assets.lamp_with_shade),
                      TuneMemoryBackdrop);
        lamp.NewChild("lamp_halo")
            .at({0.0f, 1.08f, 0.0f})
            .scaled({0.38f, 1.0f, 0.38f})
            .draw_glow(Mesh::Disk(1.0f, 40), GlowGold(0.22f));
    }

    if (!assets.candles.empty()) {
        auto& candles = root.NewChild(StoryName(names::kMemoryPrefix, 2,
                                                "relic", "candles", 7))
                            .at({1.72f, 0.82f, 1.76f})
                            .rot_y(-18.0f)
                            .scaled({0.42f, 0.42f, 0.42f});
        AddMemoryThread(candles, GlowRose(0.36f), 0.20f, -16.0f);
        AddModelParts(candles, std::move(assets.candles), TuneMemoryBackdrop);
        for (int i = 0; i < 3; ++i) {
            candles.NewChild("small_flame")
                .at({-0.08f + 0.08f * static_cast<float>(i), 0.42f, 0.0f})
                .scaled({0.030f, 0.060f, 0.030f})
                .draw_glow(Mesh::Sphere(1.0f, 10, 6), GlowGold(0.40f));
        }
    }
}

void AddBlindBackgroundWorld(SceneNode& root) {
    const Material wall({0.030f, 0.028f, 0.027f, 1.0f},
                        {0.16f, 0.15f, 0.13f, 1.0f},
                        {0.018f, 0.016f, 0.014f, 1.0f}, 2.0f);
    const Material floor({0.020f, 0.019f, 0.018f, 1.0f},
                         {0.10f, 0.095f, 0.085f, 1.0f},
                         {0.010f, 0.009f, 0.008f, 1.0f}, 1.0f);
    const Material shadow({0.006f, 0.006f, 0.006f, 1.0f},
                          {0.018f, 0.017f, 0.015f, 1.0f},
                          {0.0f, 0.0f, 0.0f, 1.0f}, 1.0f);
    const Material edge({0.060f, 0.055f, 0.050f, 1.0f},
                        {0.24f, 0.22f, 0.19f, 1.0f},
                        {0.020f, 0.018f, 0.016f, 1.0f}, 3.0f);

    auto& corridor =
        root.NewChild("blind_bg_silent_corridor_0").at({0.0f, 0.04f, 2.58f});
    corridor.NewChild("floor_slab")
        .at({0.0f, 0.0f, 0.0f})
        .scaled({1.55f, 0.018f, 0.72f})
        .draw_as(Mesh::Cube(), floor);
    corridor.NewChild("back_wall")
        .at({0.0f, 0.78f, 0.66f})
        .scaled({1.52f, 0.74f, 0.030f})
        .draw_as(Mesh::Cube(), wall);
    corridor.NewChild("left_wall")
        .at({-1.28f, 0.62f, 0.20f})
        .rot_y(-12.0f)
        .scaled({0.038f, 0.62f, 0.70f})
        .draw_as(Mesh::Cube(), wall);
    corridor.NewChild("right_wall")
        .at({1.28f, 0.62f, 0.20f})
        .rot_y(12.0f)
        .scaled({0.038f, 0.62f, 0.70f})
        .draw_as(Mesh::Cube(), wall);
    corridor.NewChild("long_shadow")
        .at({0.0f, 0.022f, -0.08f})
        .rot_y(5.0f)
        .scaled({1.12f, 0.004f, 0.17f})
        .draw_as(Mesh::Cube(), shadow);

    auto& doorway =
        root.NewChild("blind_bg_empty_doorframe_1").at({0.0f, 0.40f, 3.18f});
    doorway.NewChild("left_jamb")
        .at({-0.34f, 0.32f, 0.0f})
        .scaled({0.040f, 0.56f, 0.040f})
        .draw_as(Mesh::Cube(), edge);
    doorway.NewChild("right_jamb")
        .at({0.34f, 0.32f, 0.0f})
        .scaled({0.040f, 0.56f, 0.040f})
        .draw_as(Mesh::Cube(), edge);
    doorway.NewChild("lintel")
        .at({0.0f, 0.89f, 0.0f})
        .scaled({0.38f, 0.040f, 0.040f})
        .draw_as(Mesh::Cube(), edge);

    for (int i = 0; i < 5; ++i) {
        root.NewChild("blind_bg_floor_crack_" + std::to_string(i + 2))
            .at({-0.68f + 0.34f * static_cast<float>(i), 0.032f,
                 2.18f + 0.10f * static_cast<float>(i % 2)})
            .rot_y(-24.0f + 11.0f * static_cast<float>(i))
            .scaled({0.20f - 0.018f * static_cast<float>(i), 0.004f, 0.010f})
            .draw_as(Mesh::Cube(), shadow);
    }
}

void AddLeftChairNotes(SceneNode& root) {
    const Material paper({0.13f, 0.11f, 0.08f, 1.0f},
                         {0.82f, 0.74f, 0.55f, 1.0f},
                         {0.06f, 0.05f, 0.04f, 1.0f}, 3.0f);
    const Material photo({0.06f, 0.06f, 0.07f, 1.0f},
                         {0.34f, 0.37f, 0.40f, 1.0f},
                         {0.04f, 0.04f, 0.05f, 1.0f}, 4.0f);
    auto& notes =
        root.NewChild(StoryName(names::kMemoryPrefix, 0, "relic", "notes", 3))
            .at({-1.18f, 1.02f, 1.02f})
            .rot_y(184.0f)
            .rot_z(-5.0f);
    for (int i = 0; i < 3; ++i) {
        const float x = -0.13f + 0.13f * static_cast<float>(i);
        notes.NewChild("note_card")
            .at({x, 0.05f * static_cast<float>(i % 2), 0.0f})
            .rot_z(-8.0f + 7.0f * static_cast<float>(i))
            .scaled({0.060f, 0.003f, 0.082f})
            .draw_as(Mesh::Cube(), (i == 1) ? photo : paper);
    }
    AddMemoryThread(notes, GlowGold(0.30f), 0.04f, 14.0f);
}

void AddCenterFort(SceneNode& root) {
    const Material blanket({0.045f, 0.080f, 0.052f, 1.0f},
                           {0.24f, 0.45f, 0.28f, 1.0f},
                           {0.04f, 0.06f, 0.04f, 1.0f}, 5.0f);
    const Material pillow({0.12f, 0.10f, 0.075f, 1.0f},
                          {0.60f, 0.54f, 0.42f, 1.0f},
                          {0.05f, 0.045f, 0.035f, 1.0f}, 4.0f);
    auto& fort = root.NewChild(StoryName(names::kMemoryPrefix, 1, "relic",
                                         "fort_cloth", 1))
                     .at({0.00f, 0.96f, 1.00f})
                     .rot_y(180.0f);
    fort.NewChild("left_blanket")
        .at({-0.16f, 0.08f, 0.0f})
        .rot_z(-24.0f)
        .scaled({0.035f, 0.25f, 0.18f})
        .draw_as(Mesh::Cube(), blanket);
    fort.NewChild("right_blanket")
        .at({0.16f, 0.08f, 0.0f})
        .rot_z(24.0f)
        .scaled({0.035f, 0.25f, 0.18f})
        .draw_as(Mesh::Cube(), blanket);
    fort.NewChild("pillow_floor")
        .at({0.0f, -0.10f, 0.04f})
        .scaled({0.18f, 0.035f, 0.13f})
        .draw_as(Mesh::Cube(), pillow);
    AddMemoryThread(fort, Material::GlowGreen(0.34f), 0.14f, 0.0f);
}

void AddRightChairScatter(SceneNode& root) {
    const Material metal({0.07f, 0.07f, 0.065f, 1.0f},
                         {0.42f, 0.42f, 0.39f, 1.0f},
                         {0.18f, 0.17f, 0.15f, 1.0f}, 18.0f);
    const Material cloth({0.14f, 0.060f, 0.075f, 1.0f},
                         {0.54f, 0.22f, 0.25f, 1.0f},
                         {0.06f, 0.035f, 0.035f, 1.0f}, 4.0f);
    auto& scatter =
        root.NewChild(StoryName(names::kMemoryPrefix, 2, "relic", "scatter", 1))
            .at({1.20f, 0.86f, 1.05f})
            .rot_y(174.0f);
    for (int i = 0; i < 4; ++i) {
        scatter.NewChild("cutlery_line")
            .at({-0.17f + 0.11f * static_cast<float>(i),
                 -0.02f + 0.025f * static_cast<float>(i % 2), 0.0f})
            .rot_z(-38.0f + 21.0f * static_cast<float>(i))
            .scaled({0.012f, 0.14f, 0.010f})
            .draw_as(Mesh::Cube(), metal);
    }
    scatter.NewChild("pushed_cloth")
        .at({0.06f, 0.08f, -0.03f})
        .rot_z(-18.0f)
        .scaled({0.19f, 0.018f, 0.13f})
        .draw_as(Mesh::Cube(), cloth);
    AddMemoryThread(scatter, GlowRose(0.36f), 0.08f, -24.0f);
}

void AddFutureAccidentSet(SceneNode& root, StoryPropAssets& assets) {
    if (!assets.infrared_camera.empty()) {
        auto& camera = root.NewChild("prediction_infrared_camera")
                           .at({-1.74f, 0.84f, 0.46f})
                           .rot_y(142.0f)
                           .rot_x(-8.0f)
                           .scaled({0.34f, 0.34f, 0.34f});
        AddModelParts(camera, std::move(assets.infrared_camera),
                      TuneFutureDevice);
    }

    if (!assets.dirty_plate.empty()) {
        auto& plate = root.NewChild(StoryName(names::kFuturePrefix, 0, "branch",
                                              "dirty_plate", 0))
                          .at({-0.78f, 0.840f, -0.06f})
                          .rot_y(28.0f)
                          .rot_x(-8.0f)
                          .rot_z(5.0f)
                          .scaled({0.34f, 0.34f, 0.34f});
        AddFutureEdge(plate, 0.58f, 0.24f, 18.0f);
        AddModelParts(plate, std::move(assets.dirty_plate), TuneFutureProp);
    }

    if (!assets.bowl_dirty.empty()) {
        auto& bowl = root.NewChild(StoryName(names::kFuturePrefix, 1, "branch",
                                             "bowl", 1))
                         .at({0.18f, 0.858f, -0.06f})
                         .rot_y(-18.0f)
                         .rot_x(10.0f)
                         .rot_z(-18.0f)
                         .scaled({0.30f, 0.30f, 0.30f});
        AddFutureEdge(bowl, 0.50f, 0.20f, -12.0f);
        AddModelParts(bowl, std::move(assets.bowl_dirty), TuneFutureProp);
    }

    if (!assets.stew.empty()) {
        auto& stew = root.NewChild(StoryName(names::kFuturePrefix, 1, "branch",
                                             "stew", 2))
                         .at({0.44f, 0.828f, -0.18f})
                         .rot_y(42.0f)
                         .rot_z(6.0f)
                         .scaled({0.25f, 0.11f, 0.25f});
        AddFutureEdge(stew, 0.46f, 0.18f, 33.0f);
        AddModelParts(stew, std::move(assets.stew), TuneFutureProp);
    }

    if (!assets.cutting_board.empty()) {
        auto& board = root.NewChild(StoryName(names::kFuturePrefix, 2, "branch",
                                              "cutting_board", 3))
                          .at({1.20f, 0.836f, 0.04f})
                          .rot_y(-38.0f)
                          .rot_x(4.0f)
                          .rot_z(-4.0f)
                          .scaled({0.38f, 0.16f, 0.34f});
        AddFutureEdge(board, 0.64f, 0.16f, -24.0f);
        AddModelParts(board, std::move(assets.cutting_board), TuneFutureProp);
    }

    if (!assets.chopsticks.empty()) {
        auto& sticks = root.NewChild(StoryName(names::kFuturePrefix, 2,
                                               "branch", "chopsticks", 4))
                           .at({1.44f, 0.858f, -0.12f})
                           .rot_y(62.0f)
                           .rot_z(13.0f)
                           .scaled({0.44f, 0.44f, 0.44f});
        AddFutureEdge(sticks, 0.42f, 0.14f, 52.0f);
        AddModelParts(sticks, std::move(assets.chopsticks), TuneFutureProp);
    }
}

void AddLongingAltars(SceneNode& root, StoryPropAssets& assets) {
    if (!assets.present.empty()) {
        auto& present = root.NewChild(StoryName(names::kMemoryPrefix, 0,
                                                "relic", "present", 0))
                            .at({-1.42f, 0.93f, 1.02f})
                            .rot_y(172.0f)
                            .rot_z(4.0f)
                            .scaled({0.42f, 0.42f, 0.42f});
        AddMemoryThread(present, GlowGold(0.42f), 0.18f, 0.0f);
        AddModelParts(present, std::move(assets.present), TuneMemoryWarm);
    }

    if (!assets.wall_corkboard.empty()) {
        auto& board = root.NewChild(StoryName(names::kMemoryPrefix, 0, "relic",
                                              "corkboard", 1))
                          .at({-1.06f, 1.18f, 0.95f})
                          .rot_y(188.0f)
                          .rot_x(-6.0f)
                          .rot_z(-8.0f)
                          .scaled({0.62f, 0.62f, 0.62f});
        AddMemoryThread(board, GlowGold(0.34f), 0.05f, 18.0f);
        AddModelParts(board, std::move(assets.wall_corkboard), TuneMemoryWarm);
    }

    if (!assets.astronaut_glove.empty()) {
        auto& glove = root.NewChild(StoryName(names::kMemoryPrefix, 0, "relic",
                                              "glove", 2))
                          .at({-1.70f, 0.82f, 1.30f})
                          .rot_y(-34.0f)
                          .rot_x(74.0f)
                          .rot_z(-18.0f)
                          .scaled({0.40f, 0.40f, 0.40f});
        AddMemoryThread(glove, GlowGold(0.30f), 0.06f, -22.0f);
        AddModelParts(glove, std::move(assets.astronaut_glove), TuneMemoryWarm);
    }

    if (!assets.mug_office_tool.empty()) {
        auto& mug =
            root.NewChild(StoryName(names::kMemoryPrefix, 1, "relic", "mug", 0))
                .at({0.05f, 0.90f, 0.98f})
                .rot_y(184.0f)
                .rot_z(3.0f)
                .scaled({0.45f, 0.45f, 0.45f});
        AddMemoryThread(mug, Material::GlowGreen(0.38f), 0.18f, 0.0f);
        AddModelParts(mug, std::move(assets.mug_office_tool), TuneMemoryGreen);
    }

    if (!assets.closed_umbrella.empty()) {
        auto& umbrella = root.NewChild(StoryName(names::kMemoryPrefix, 2,
                                                 "relic", "umbrella", 0))
                             .at({1.50f, 0.74f, 1.05f})
                             .rot_y(146.0f)
                             .rot_x(6.0f)
                             .rot_z(-46.0f)
                             .scaled({0.78f, 0.78f, 0.78f});
        AddMemoryThread(umbrella, GlowRose(0.42f), 0.12f, -28.0f);
        AddModelParts(umbrella, std::move(assets.closed_umbrella),
                      TuneMemoryRose);
    }

    AddLeftChairNotes(root);
    AddCenterFort(root);
    AddRightChairScatter(root);
}

void AddBlindCompleteWorld(SceneNode& root, StoryPropAssets& assets) {
    if (!assets.crew_lock_bag.empty()) {
        auto& bag = root.NewChild("blind_complete_crew_lock_bag")
                        .at({0.58f, 0.12f, 0.72f})
                        .rot_y(18.0f)
                        .rot_z(-4.0f)
                        .scaled({0.40f, 0.40f, 0.40f});
        AddModelParts(bag, std::move(assets.crew_lock_bag), TuneBlindObject);
    }

    if (!assets.zz_plant.empty()) {
        auto& plant = root.NewChild("blind_complete_zz_plant")
                          .at({-0.74f, 0.07f, 0.72f})
                          .rot_y(-22.0f)
                          .scaled({0.34f, 0.34f, 0.34f});
        AddModelParts(plant, std::move(assets.zz_plant), TuneBlindObject);
    }

    const Material paper({0.10f, 0.09f, 0.075f, 1.0f},
                         {0.58f, 0.54f, 0.44f, 1.0f},
                         {0.04f, 0.035f, 0.030f, 1.0f}, 2.0f);
    const Material graphite({0.012f, 0.011f, 0.010f, 1.0f},
                            {0.055f, 0.052f, 0.047f, 1.0f},
                            {0.0f, 0.0f, 0.0f, 1.0f}, 1.0f);
    const Material muted_metal({0.055f, 0.055f, 0.052f, 1.0f},
                               {0.30f, 0.31f, 0.30f, 1.0f},
                               {0.08f, 0.08f, 0.075f, 1.0f}, 8.0f);

    auto& letter = root.NewChild("blind_complete_unread_mail")
                       .at({-0.18f, 0.020f, 0.86f})
                       .rot_y(-9.0f);
    letter.NewChild("envelope")
        .scaled({0.17f, 0.003f, 0.10f})
        .draw_as(Mesh::Cube(), paper);
    letter.NewChild("sealed_edge")
        .at({0.0f, 0.006f, 0.0f})
        .rot_y(45.0f)
        .scaled({0.012f, 0.002f, 0.13f})
        .draw_as(Mesh::Cube(), graphite);

    auto& tool = root.NewChild("blind_complete_fallen_tool")
                     .at({0.18f, 0.030f, 0.64f})
                     .rot_y(67.0f)
                     .rot_z(4.0f);
    tool.NewChild("handle")
        .scaled({0.018f, 0.018f, 0.16f})
        .draw_as(Mesh::Cube(), muted_metal);
    tool.NewChild("head")
        .at({0.0f, 0.0f, 0.18f})
        .scaled({0.045f, 0.012f, 0.026f})
        .draw_as(Mesh::Cube(), muted_metal);
}

// ─── Future sci-fi backdrop shell
// ───────────────────────────────────────────── fgbg_* nodes: revealed with
// FutureWeight. Large room fills the void above and around the existing
// platform/column set.
void AddFutureBgShell(SceneNode& root) {
    const RoomBounds room = DefaultRoomBounds();
    const Aabb3 b = room.interior;
    const float t = room.wall_thickness;
    // Hull: medium-dark steel-blue — visible in the moody sci-fi lighting.
    const Material hull({0.018f, 0.030f, 0.048f, 1.0f},
                        {0.10f, 0.17f, 0.26f, 1.0f},
                        {0.40f, 0.60f, 0.75f, 1.0f}, 72.0f);
    const Material panel({0.014f, 0.024f, 0.038f, 1.0f},
                         {0.075f, 0.130f, 0.190f, 1.0f},
                         {0.28f, 0.45f, 0.60f, 1.0f}, 52.0f);
    const Material dark_floor({0.010f, 0.016f, 0.026f, 1.0f},
                              {0.042f, 0.072f, 0.108f, 1.0f},
                              {0.14f, 0.24f, 0.36f, 1.0f}, 32.0f);

    // ── fgbg_main_0: outer shell — walls behind the eye (z > 0.5),
    //   ceiling spans far overhead, floor covers the full area.
    {
        auto& s = root.NewChild("fgbg_main_0").at({0.0f, 0.0f, 0.0f});

        // ── Far wall: derived from RoomBounds on the negative-Z side, behind
        // the Prediction Core from the default spectator camera. Keeping this
        // wall on the actual far side prevents it from becoming a legal-but-
        // unusable foreground occluder when the camera solver frames the eye.
        s.NewChild("back_wall")
            .at({0.0f, (b.min.y + b.max.y) * 0.5f, b.min.z - t * 0.5f})
            .scaled({b.Size().x, b.Size().y, t})
            .draw_as(Mesh::Cube(), hull);
        for (int i = 0; i < 9; ++i) {
            const float u = static_cast<float>(i) / 8.0f;
            s.NewChild("bw_rib_v")
                .at({Lerp(b.min.x + 1.0f, b.max.x - 1.0f, u),
                     (b.min.y + b.max.y) * 0.5f, b.min.z + 0.02f})
                .scaled({0.014f, b.Size().y - 0.3f, 0.014f})
                .draw_glow(Mesh::Cube(), Material::GlowCyan(
                                             0.18f - 0.010f * std::abs(i - 4)));
        }
        for (int i = 0; i < 5; ++i) {
            const float u = static_cast<float>(i) / 4.0f;
            s.NewChild("bw_rib_h")
                .at({0.0f, Lerp(b.min.y + 0.6f, b.max.y - 0.6f, u),
                     b.min.z + 0.02f})
                .scaled({b.Size().x - 0.4f, 0.012f, 0.012f})
                .draw_glow(Mesh::Cube(), Material::GlowCyan(0.13f - 0.01f * i));
        }

        // ── Side walls span the full RoomBounds depth.
        s.NewChild("left_wall")
            .at({b.min.x - t * 0.5f, (b.min.y + b.max.y) * 0.5f,
                 (b.min.z + b.max.z) * 0.5f})
            .scaled({t, b.Size().y, b.Size().z})
            .draw_as(Mesh::Cube(), hull);
        for (int i = 0; i < 4; ++i) {
            const float u = static_cast<float>(i + 1) / 5.0f;
            s.NewChild("lw_rib_v")
                .at({b.min.x + 0.02f, (b.min.y + b.max.y) * 0.5f,
                     Lerp(b.min.z + 0.8f, b.max.z - 0.8f, u)})
                .scaled({0.014f, b.Size().y - 0.3f, 0.014f})
                .draw_glow(Mesh::Cube(), Material::GlowCyan(0.15f));
        }

        s.NewChild("right_wall")
            .at({b.max.x + t * 0.5f, (b.min.y + b.max.y) * 0.5f,
                 (b.min.z + b.max.z) * 0.5f})
            .scaled({t, b.Size().y, b.Size().z})
            .draw_as(Mesh::Cube(), hull);
        for (int i = 0; i < 4; ++i) {
            const float u = static_cast<float>(i + 1) / 5.0f;
            s.NewChild("rw_rib_v")
                .at({b.max.x - 0.02f, (b.min.y + b.max.y) * 0.5f,
                     Lerp(b.min.z + 0.8f, b.max.z - 0.8f, u)})
                .scaled({0.014f, b.Size().y - 0.3f, 0.014f})
                .draw_glow(Mesh::Cube(), Material::GlowCyan(0.15f));
        }

        // ── Ceiling and floor share RoomBounds, so camera clearance is
        // mathematically tied to the rendered shell.
        s.NewChild("ceiling")
            .at({0.0f, b.max.y + t * 0.5f, (b.min.z + b.max.z) * 0.5f})
            .scaled({b.Size().x, t, b.Size().z})
            .draw_as(Mesh::Cube(), panel);
        for (int i = 0; i < 9; ++i) {
            const float u = static_cast<float>(i) / 8.0f;
            s.NewChild("ceil_rib_x")
                .at({Lerp(b.min.x + 1.0f, b.max.x - 1.0f, u), b.max.y - 0.03f,
                     (b.min.z + b.max.z) * 0.5f})
                .scaled({0.012f, 0.012f, b.Size().z - 0.5f})
                .draw_glow(Mesh::Cube(), Material::GlowCyan(
                                             0.16f - 0.008f * std::abs(i - 4)));
        }
        for (int i = 0; i < 7; ++i) {
            const float u = static_cast<float>(i) / 6.0f;
            s.NewChild("ceil_rib_z")
                .at({0.0f, b.max.y - 0.03f,
                     Lerp(b.min.z + 1.0f, b.max.z - 1.0f, u)})
                .rot_y(90.0f)
                .scaled({0.012f, 0.012f, b.Size().x - 0.5f})
                .draw_glow(Mesh::Cube(), Material::GlowCyan(0.12f));
        }

        s.NewChild("floor_ext")
            .at({0.0f, b.min.y - t * 0.5f, (b.min.z + b.max.z) * 0.5f})
            .scaled({b.Size().x, t, b.Size().z})
            .draw_as(Mesh::Cube(), dark_floor);
        for (int i = 0; i < 7; ++i) {
            const float u = static_cast<float>(i) / 6.0f;
            s.NewChild("fl_line_x")
                .at({Lerp(b.min.x + 1.2f, b.max.x - 1.2f, u), 0.022f,
                     (b.min.z + b.max.z) * 0.5f})
                .scaled({0.009f, 0.002f, b.Size().z - 0.8f})
                .draw_glow(Mesh::Cube(), Material::GlowCyan(0.10f));
        }
        for (int i = 0; i < 8; ++i) {
            const float u = static_cast<float>(i) / 7.0f;
            s.NewChild("fl_line_z")
                .at({0.0f, 0.022f, Lerp(b.min.z + 1.0f, b.max.z - 1.0f, u)})
                .scaled({b.Size().x - 0.8f, 0.002f, 0.009f})
                .draw_glow(Mesh::Cube(), Material::GlowCyan(0.08f));
        }
    }

    // ── fgbg_col_towers_1: tall glowing column array
    {
        auto& s = root.NewChild("fgbg_col_towers_1").at({0.0f, 0.0f, 0.0f});
        constexpr float kCx[6] = {-4.8f, -3.2f, -1.6f, 1.6f, 3.2f, 4.8f};
        constexpr float kCz[2] = {-0.8f, 3.0f};
        for (float cx : kCx) {
            for (float cz : kCz) {
                auto& col = s.NewChild("col").at({cx, 0.04f, cz});
                col.NewChild("shaft")
                    .at({0.0f, 2.0f, 0.0f})
                    .draw_glow(Mesh::Cylinder(0.028f, 4.0f, 14),
                               Material::GlowCyan(0.22f));
                for (int r = 0; r < 6; ++r) {
                    col.NewChild("ring")
                        .at({0.0f, 0.4f + 0.6f * r, 0.0f})
                        .scaled({1.0f, 1.0f, 1.0f})
                        .draw_glow(Mesh::Disk(0.058f, 10),
                                   Material::GlowCyan(0.30f - 0.025f * r));
                }
                // Base platform cap
                col.NewChild("cap")
                    .at({0.0f, 0.08f, 0.0f})
                    .scaled({0.22f, 0.08f, 0.22f})
                    .draw_as(Mesh::Cube(), panel);
            }
        }
    }

    // ── fgbg_arch_ring_2: overhead scan arches spanning the ceiling
    {
        auto& s = root.NewChild("fgbg_arch_ring_2").at({0.0f, 5.4f, 2.0f});
        // Large overhead ring
        s.NewChild("ring_outer")
            .scaled({3.8f, 1.0f, 3.8f})
            .draw_glow(Mesh::Disk(1.0f, 48), Material::GlowCyan(0.15f));
        s.NewChild("ring_inner")
            .scaled({2.6f, 1.0f, 2.6f})
            .draw_glow(Mesh::Disk(1.0f, 36), Material::GlowCyan(0.10f));
        // Cross bars
        s.NewChild("cross_a")
            .scaled({0.012f, 0.012f, 4.0f})
            .draw_glow(Mesh::Cube(), Material::GlowCyan(0.25f));
        s.NewChild("cross_b")
            .rot_y(90.0f)
            .scaled({0.012f, 0.012f, 4.0f})
            .draw_glow(Mesh::Cube(), Material::GlowCyan(0.25f));
        s.NewChild("cross_c")
            .rot_y(45.0f)
            .scaled({0.009f, 0.009f, 5.2f})
            .draw_glow(Mesh::Cube(), Material::GlowCyan(0.18f));
        s.NewChild("cross_d")
            .rot_y(-45.0f)
            .scaled({0.009f, 0.009f, 5.2f})
            .draw_glow(Mesh::Cube(), Material::GlowCyan(0.18f));
    }

    // ── fgbg_scan_tower_l/r_3/4: bilaterally symmetric scanner towers.
    // beam_x and beam_yaw_a/b flip sign between left and right.
    auto add_scan_tower = [&](SceneNode& parent, const std::string& name,
                              float x, float beam_x, float yaw_a, float yaw_b) {
        auto& s = parent.NewChild(name).at({x, 0.04f, 1.2f});
        s.NewChild("base")
            .at({0.0f, 0.32f, 0.0f})
            .scaled({0.58f, 0.64f, 0.58f})
            .draw_as(Mesh::Cube(), panel);
        s.NewChild("shaft")
            .at({0.0f, 2.5f, 0.0f})
            .draw_glow(Mesh::Cylinder(0.072f, 5.0f, 16),
                       Material::GlowCyan(0.20f));
        for (int i = 0; i < 8; ++i) {
            s.NewChild("ring")
                .at({0.0f, 0.5f + 0.55f * i, 0.0f})
                .scaled({1.0f, 1.0f, 1.0f})
                .draw_glow(Mesh::Disk(0.10f, 12), Material::GlowCyan(0.28f));
        }
        s.NewChild("beam_a")
            .at({beam_x, 4.0f, 0.5f})
            .rot_x(14.0f)
            .rot_y(yaw_a)
            .scaled({0.009f, 0.009f, 1.2f})
            .draw_glow(Mesh::Cube(), Material::GlowCyan(0.45f));
        s.NewChild("beam_b")
            .at({beam_x, 3.5f, -0.5f})
            .rot_x(10.0f)
            .rot_y(yaw_b)
            .scaled({0.009f, 0.009f, 0.9f})
            .draw_glow(Mesh::Cube(), Material::GlowCyan(0.35f));
    };
    add_scan_tower(root, "fgbg_scan_tower_l_3", -5.2f, 0.5f, -40.0f, 28.0f);
    add_scan_tower(root, "fgbg_scan_tower_r_4", 5.2f, -0.5f, 40.0f, -28.0f);

    // ── fgbg_upper_band_5: horizontal scan band across mid-height
    {
        auto& s = root.NewChild("fgbg_upper_band_5").at({0.0f, 3.6f, 2.2f});
        // Floating horizontal bar spanning the full width
        s.NewChild("bar_main")
            .scaled({11.0f, 0.014f, 0.014f})
            .draw_glow(Mesh::Cube(), Material::GlowCyan(0.22f));
        // Tick marks hanging down from the bar
        for (int i = 0; i < 12; ++i) {
            const float x = -5.0f + 10.0f * static_cast<float>(i) / 11.0f;
            s.NewChild("tick")
                .at({x, -0.08f - 0.04f * (i % 3), 0.0f})
                .scaled({0.008f, 0.08f + 0.04f * (i % 2), 0.008f})
                .draw_glow(Mesh::Cube(),
                           Material::GlowCyan(0.30f - 0.02f * (i % 3)));
        }
    }

    // ── fgbg_wall_panels_6: inset display panels on the far wall. The z comes
    // from RoomBounds; old mid-room placement intersected the spectator sight
    // line and hid the eye/table while still satisfying camera room bounds.
    {
        auto& s = root.NewChild("fgbg_wall_panels_6")
                      .at({0.0f, 0.0f, b.min.z + 0.16f});
        // Three large display recesses
        for (int i = 0; i < 3; ++i) {
            const float x = -2.8f + 2.8f * i;
            s.NewChild("panel_bg")
                .at({x, 2.6f, 0.0f})
                .scaled({1.6f, 2.0f, 0.04f})
                .draw_as(Mesh::Cube(), panel);
            s.NewChild("panel_glow")
                .at({x, 2.6f, -0.025f})
                .scaled({1.3f, 1.7f, 0.010f})
                .draw_glow(Mesh::Cube(),
                           Material::GlowCyan(0.16f - 0.03f * std::abs(i - 1)));
            // Horizontal bars inside panel (like data readouts)
            for (int j = 0; j < 5; ++j) {
                const float w = 1.0f - 0.12f * j;
                s.NewChild("data_bar")
                    .at({x - 0.48f + w * 0.24f, 1.6f + 0.32f * j, -0.030f})
                    .scaled({w * 0.90f, 0.055f, 0.008f})
                    .draw_glow(Mesh::Cube(), Material::GlowCyan(0.26f));
            }
        }
    }
}

// ─── Memory warm-room backdrop shell
// ────────────────────────────────────────── mgbg_* nodes: revealed with
// MemoryWeight. Warm domestic interior wrapping around the existing
// lamp/curtain/candle set.
void AddMemoryBgShell(SceneNode& root) {
    const RoomBounds room = DefaultRoomBounds();
    const Aabb3 b = room.interior;
    const float t = room.wall_thickness;
    const Material warm_wall({0.095f, 0.058f, 0.042f, 1.0f},
                             {0.36f, 0.20f, 0.14f, 1.0f},
                             {0.055f, 0.030f, 0.020f, 1.0f}, 5.0f);
    const Material warm_trim({0.130f, 0.078f, 0.052f, 1.0f},
                             {0.52f, 0.30f, 0.18f, 1.0f},
                             {0.09f, 0.054f, 0.034f, 1.0f}, 10.0f);
    const Material warm_ceiling({0.075f, 0.048f, 0.034f, 1.0f},
                                {0.28f, 0.16f, 0.11f, 1.0f},
                                {0.03f, 0.018f, 0.012f, 1.0f}, 3.0f);
    const Material rug({0.080f, 0.026f, 0.018f, 1.0f},
                       {0.35f, 0.10f, 0.07f, 1.0f},
                       {0.025f, 0.008f, 0.006f, 1.0f}, 2.0f);
    const Material rug_border({0.110f, 0.065f, 0.040f, 1.0f},
                              {0.46f, 0.26f, 0.15f, 1.0f},
                              {0.04f, 0.022f, 0.014f, 1.0f}, 4.0f);
    const Material dark_wood({0.060f, 0.034f, 0.020f, 1.0f},
                             {0.24f, 0.13f, 0.08f, 1.0f},
                             {0.045f, 0.024f, 0.014f, 1.0f}, 8.0f);
    const Material canvas({0.048f, 0.034f, 0.024f, 1.0f},
                          {0.22f, 0.15f, 0.10f, 1.0f},
                          {0.020f, 0.014f, 0.010f, 1.0f}, 2.0f);
    const Material curtain({0.085f, 0.038f, 0.022f, 1.0f},
                           {0.40f, 0.16f, 0.09f, 1.0f},
                           {0.032f, 0.014f, 0.008f, 1.0f}, 3.0f);

    // ── mgbg_room_shell_0: main room walls + ceiling + floor.
    //   All shell geometry is derived from RoomBounds.
    {
        auto& s = root.NewChild("mgbg_room_shell_0").at({0.0f, 0.0f, 0.0f});

        // Back wall
        s.NewChild("back_wall")
            .at({0.0f, (b.min.y + b.max.y) * 0.5f, b.max.z + t * 0.5f})
            .rot_y(180.0f)
            .scaled({b.Size().x, b.Size().y, t})
            .draw_as(Mesh::Cube(), warm_wall);
        s.NewChild("wainscot")
            .at({0.0f, 0.54f, b.max.z - 0.02f})
            .rot_y(180.0f)
            .scaled({b.Size().x - 0.4f, 0.96f, 0.06f})
            .draw_as(Mesh::Cube(), warm_trim);
        s.NewChild("chair_rail")
            .at({0.0f, 1.06f, b.max.z - 0.02f})
            .rot_y(180.0f)
            .scaled({b.Size().x - 0.4f, 0.058f, 0.040f})
            .draw_as(Mesh::Cube(), warm_trim);
        s.NewChild("crown")
            .at({0.0f, b.max.y - 0.12f, b.max.z - 0.02f})
            .rot_y(180.0f)
            .scaled({b.Size().x - 0.4f, 0.15f, 0.058f})
            .draw_as(Mesh::Cube(), warm_trim);
        s.NewChild("baseboard")
            .at({0.0f, 0.070f, b.max.z - 0.02f})
            .rot_y(180.0f)
            .scaled({b.Size().x - 0.4f, 0.13f, 0.042f})
            .draw_as(Mesh::Cube(), dark_wood);

        // Left side wall
        s.NewChild("left_wall")
            .at({b.min.x - t * 0.5f, (b.min.y + b.max.y) * 0.5f,
                 (b.min.z + b.max.z) * 0.5f})
            .scaled({t, b.Size().y, b.Size().z})
            .draw_as(Mesh::Cube(), warm_wall);
        s.NewChild("lw_wainscot")
            .at({b.min.x + 0.02f, 0.54f, (b.min.z + b.max.z) * 0.5f})
            .scaled({0.055f, 0.96f, b.Size().z - 0.4f})
            .draw_as(Mesh::Cube(), warm_trim);

        // Right side wall
        s.NewChild("right_wall")
            .at({b.max.x + t * 0.5f, (b.min.y + b.max.y) * 0.5f,
                 (b.min.z + b.max.z) * 0.5f})
            .scaled({t, b.Size().y, b.Size().z})
            .draw_as(Mesh::Cube(), warm_wall);
        s.NewChild("rw_wainscot")
            .at({b.max.x - 0.02f, 0.54f, (b.min.z + b.max.z) * 0.5f})
            .scaled({0.055f, 0.96f, b.Size().z - 0.4f})
            .draw_as(Mesh::Cube(), warm_trim);

        s.NewChild("ceiling")
            .at({0.0f, b.max.y + t * 0.5f, (b.min.z + b.max.z) * 0.5f})
            .rot_y(180.0f)
            .scaled({b.Size().x, t, b.Size().z})
            .draw_as(Mesh::Cube(), warm_ceiling);
        s.NewChild("ceiling_glow_c")
            .at({0.0f, 5.56f, 3.0f})
            .rot_x(90.0f)
            .scaled({3.0f, 3.0f, 1.0f})
            .draw_glow(Mesh::Disk(1.0f, 40), GlowGold(0.24f));
        s.NewChild("ceiling_glow_l")
            .at({-3.0f, 5.56f, 3.0f})
            .rot_x(90.0f)
            .scaled({1.6f, 1.6f, 1.0f})
            .draw_glow(Mesh::Disk(1.0f, 28), GlowGold(0.16f));
        s.NewChild("ceiling_glow_r")
            .at({3.0f, 5.56f, 3.0f})
            .rot_x(90.0f)
            .scaled({1.6f, 1.6f, 1.0f})
            .draw_glow(Mesh::Disk(1.0f, 28), GlowGold(0.16f));

        // Floor + rug
        s.NewChild("floor_ext")
            .at({0.0f, b.min.y - t * 0.5f, (b.min.z + b.max.z) * 0.5f})
            .scaled({b.Size().x, t, b.Size().z})
            .draw_as(Mesh::Cube(), warm_ceiling);
        s.NewChild("rug_main")
            .at({0.0f, 0.020f, 2.8f})
            .scaled({6.0f, 0.005f, 4.0f})
            .draw_as(Mesh::Cube(), rug);
        s.NewChild("rug_border")
            .at({0.0f, 0.019f, 2.8f})
            .scaled({6.5f, 0.004f, 4.5f})
            .draw_as(Mesh::Cube(), rug_border);
    }

    // ── mgbg_drape_left/right_1/2: bilaterally symmetric curtain clusters.
    // xi_sign flips fold offsets; glow_fn selects thread/tassel colour.
    auto add_drape_cluster = [&](const std::string& name, float x,
                                 float xi_sign, Material (*glow_fn)(float),
                                 float tassel_x) {
        auto& s = root.NewChild(name).at({x, 0.0f, 4.4f});
        s.NewChild("rod")
            .at({0.0f, 5.1f, 0.0f})
            .rot_z(90.0f)
            .scaled({0.015f, 1.8f, 0.015f})
            .draw_as(Mesh::Cube(), warm_trim);
        for (int i = 0; i < 3; ++i) {
            const float xi = xi_sign * 0.28f * i;
            s.NewChild("fold")
                .at({xi, 2.5f, 0.04f * i})
                .rot_z(xi_sign * (-3.0f + 2.0f * i))
                .scaled({0.32f - 0.02f * i, 5.0f, 0.048f})
                .draw_as(Mesh::Cube(), curtain);
        }
        s.NewChild("thread")
            .at({0.0f, 3.5f, 0.0f})
            .scaled({0.007f, 2.2f, 0.007f})
            .draw_glow(Mesh::Cube(), glow_fn(0.30f));
        s.NewChild("tassel")
            .at({tassel_x, 2.8f, 0.0f})
            .scaled({0.06f, 0.06f, 0.06f})
            .draw_glow(Mesh::Sphere(1.0f, 8, 5), glow_fn(0.24f));
    };
    add_drape_cluster("mgbg_drape_left_1", -3.8f, 1.0f, GlowGold, 0.22f);
    add_drape_cluster("mgbg_drape_right_2", 3.8f, -1.0f, GlowRose, -0.22f);

    // ── mgbg_wall_art_3: picture frames + wall decorations on back wall
    {
        auto& s = root.NewChild("mgbg_wall_art_3").at({0.0f, 2.8f, 4.90f});

        // Central large painting
        s.NewChild("frame_c")
            .at({0.0f, 0.0f, 0.0f})
            .scaled({1.05f, 0.78f, 0.045f})
            .draw_as(Mesh::Cube(), dark_wood);
        s.NewChild("canvas_c")
            .at({0.0f, 0.0f, -0.024f})
            .scaled({0.88f, 0.62f, 0.010f})
            .draw_as(Mesh::Cube(), WarmCanvas());

        // Left small painting
        s.NewChild("frame_l")
            .at({-2.2f, 0.26f, 0.0f})
            .rot_z(4.0f)
            .scaled({0.52f, 0.40f, 0.040f})
            .draw_as(Mesh::Cube(), dark_wood);
        s.NewChild("canvas_l")
            .at({-2.2f, 0.26f, -0.022f})
            .rot_z(4.0f)
            .scaled({0.38f, 0.28f, 0.008f})
            .draw_as(Mesh::Cube(), WarmCanvas());

        // Right small painting
        s.NewChild("frame_r")
            .at({2.2f, -0.18f, 0.0f})
            .rot_z(-3.0f)
            .scaled({0.44f, 0.36f, 0.040f})
            .draw_as(Mesh::Cube(), dark_wood);
        s.NewChild("canvas_r")
            .at({2.2f, -0.18f, -0.022f})
            .rot_z(-3.0f)
            .scaled({0.30f, 0.24f, 0.008f})
            .draw_as(Mesh::Cube(), RoseCanvas());

        // Decorative sconce / wall light — left
        s.NewChild("sconce_l")
            .at({-3.6f, -0.5f, -0.02f})
            .scaled({0.08f, 0.08f, 0.08f})
            .draw_glow(Mesh::Sphere(1.0f, 10, 7), GlowGold(0.35f));
        s.NewChild("sconce_r")
            .at({3.6f, -0.5f, -0.02f})
            .scaled({0.08f, 0.08f, 0.08f})
            .draw_glow(Mesh::Sphere(1.0f, 10, 7), GlowGold(0.35f));
    }

    // ── mgbg_candle_cluster_4: extra candle groups flanking the table area
    {
        auto& s = root.NewChild("mgbg_candle_cluster_4").at({0.0f, 0.0f, 0.0f});
        // Left candle group on side table
        const Material side_table({0.058f, 0.034f, 0.022f, 1.0f},
                                  {0.26f, 0.14f, 0.09f, 1.0f},
                                  {0.04f, 0.022f, 0.014f, 1.0f}, 6.0f);
        {
            auto& ct = s.NewChild("left_side_table").at({-3.8f, 0.0f, 2.2f});
            ct.NewChild("tabletop")
                .at({0.0f, 0.56f, 0.0f})
                .scaled({0.50f, 0.040f, 0.36f})
                .draw_as(Mesh::Cube(), side_table);
            ct.NewChild("leg_a")
                .at({-0.22f, 0.28f, -0.15f})
                .scaled({0.030f, 0.55f, 0.030f})
                .draw_as(Mesh::Cube(), side_table);
            ct.NewChild("leg_b")
                .at({0.22f, 0.28f, -0.15f})
                .scaled({0.030f, 0.55f, 0.030f})
                .draw_as(Mesh::Cube(), side_table);
            ct.NewChild("leg_c")
                .at({-0.22f, 0.28f, 0.15f})
                .scaled({0.030f, 0.55f, 0.030f})
                .draw_as(Mesh::Cube(), side_table);
            ct.NewChild("leg_d")
                .at({0.22f, 0.28f, 0.15f})
                .scaled({0.030f, 0.55f, 0.030f})
                .draw_as(Mesh::Cube(), side_table);
            // Three candles on side table
            for (int i = 0; i < 3; ++i) {
                const float xc = -0.10f + 0.10f * i;
                const float hc = 0.12f + 0.06f * i;
                ct.NewChild("candle_body")
                    .at({xc, 0.58f + hc * 0.5f, 0.0f})
                    .scaled({0.025f, hc, 0.025f})
                    .draw_as(Mesh::Cylinder(1.0f, 1.0f, 8), side_table);
                ct.NewChild("flame")
                    .at({xc, 0.58f + hc + 0.025f, 0.0f})
                    .scaled({0.022f, 0.022f, 0.022f})
                    .draw_glow(Mesh::Sphere(1.0f, 8, 5), GlowGold(0.50f));
            }
        }
        // Right candle group
        {
            auto& ct = s.NewChild("right_side_table").at({3.8f, 0.0f, 2.2f});
            ct.NewChild("tabletop")
                .at({0.0f, 0.56f, 0.0f})
                .scaled({0.50f, 0.040f, 0.36f})
                .draw_as(Mesh::Cube(), side_table);
            for (int i = 0; i < 4; ++i) {
                const float xc = -0.16f + 0.10f * i;
                const float hc = 0.10f + 0.04f * i;
                ct.NewChild("candle_body")
                    .at({xc, 0.58f + hc * 0.5f, 0.0f})
                    .scaled({0.022f, hc, 0.022f})
                    .draw_as(Mesh::Cylinder(1.0f, 1.0f, 8), side_table);
                ct.NewChild("flame")
                    .at({xc, 0.58f + hc + 0.022f, 0.0f})
                    .scaled({0.020f, 0.020f, 0.020f})
                    .draw_glow(Mesh::Sphere(1.0f, 8, 5), GlowGold(0.45f));
            }
        }
    }
}

// ─── Expanded Blind zone corridor shell
// ─────────────────────────────────────── Additional blind_bg_* nodes (slots
// 7-9) — deep narrow hall, flanking walls, and carpet-shadow field that appear
// with BlindWeight.
void AddBlindBgExpansion(SceneNode& root) {
    const RoomBounds room = DefaultRoomBounds();
    const Aabb3 b = room.interior;
    const Material wall({0.030f, 0.028f, 0.027f, 1.0f},
                        {0.16f, 0.15f, 0.13f, 1.0f},
                        {0.018f, 0.016f, 0.014f, 1.0f}, 2.0f);
    const Material floor({0.020f, 0.019f, 0.018f, 1.0f},
                         {0.10f, 0.095f, 0.085f, 1.0f},
                         {0.010f, 0.009f, 0.008f, 1.0f}, 1.0f);
    const Material shadow({0.006f, 0.006f, 0.006f, 1.0f},
                          {0.018f, 0.017f, 0.015f, 1.0f},
                          {0.0f, 0.0f, 0.0f, 1.0f}, 1.0f);
    const Material edge({0.060f, 0.055f, 0.050f, 1.0f},
                        {0.24f, 0.22f, 0.19f, 1.0f},
                        {0.020f, 0.018f, 0.016f, 1.0f}, 3.0f);
    const Material cracked({0.018f, 0.016f, 0.014f, 1.0f},
                           {0.075f, 0.070f, 0.062f, 1.0f},
                           {0.005f, 0.005f, 0.004f, 1.0f}, 1.0f);
    const Material void_mat({0.000f, 0.000f, 0.000f, 1.0f},
                            {0.002f, 0.002f, 0.002f, 1.0f},
                            {0.0f, 0.0f, 0.0f, 1.0f}, 1.0f);

    // ── blind_bg_side_hall_7: flanking side shell derived from RoomBounds.
    // This used to be a hand-authored low corridor through the middle of the
    // room; from several gaze-follow poses its ceiling slab sat between the
    // camera and table. Keep the blind-zone shell at the room perimeter
    // instead.
    {
        auto& s = root.NewChild("blind_bg_side_hall_7").at({0.0f, 0.0f, 0.0f});
        const float shell_z = (b.min.z + b.max.z) * 0.5f;
        const float shell_h = b.Size().y;

        // Left flanking wall
        s.NewChild("left_wall")
            .at({b.min.x + 0.30f, shell_h * 0.5f, shell_z})
            .scaled({0.045f, shell_h, b.Size().z})
            .draw_as(Mesh::Cube(), wall);
        // Crack marks left wall
        s.NewChild("crack_l_a")
            .at({-1.97f, 0.68f, -0.9f})
            .rot_z(22.0f)
            .scaled({0.005f, 0.36f, 0.008f})
            .draw_as(Mesh::Cube(), shadow);
        s.NewChild("crack_l_b")
            .at({-1.97f, 1.2f, -0.3f})
            .rot_z(-14.0f)
            .scaled({0.005f, 0.22f, 0.007f})
            .draw_as(Mesh::Cube(), shadow);
        s.NewChild("crack_l_c")
            .at({-1.97f, 0.85f, 1.2f})
            .rot_z(32.0f)
            .scaled({0.005f, 0.28f, 0.008f})
            .draw_as(Mesh::Cube(), shadow);
        s.NewChild("crack_l_d")
            .at({-1.97f, 0.58f, 2.0f})
            .rot_z(-9.0f)
            .scaled({0.005f, 0.18f, 0.006f})
            .draw_as(Mesh::Cube(), shadow);

        // Right flanking wall
        s.NewChild("right_wall")
            .at({b.max.x - 0.30f, shell_h * 0.5f, shell_z})
            .scaled({0.045f, shell_h, b.Size().z})
            .draw_as(Mesh::Cube(), wall);
        s.NewChild("crack_r_a")
            .at({1.97f, 0.92f, 0.4f})
            .rot_z(-26.0f)
            .scaled({0.005f, 0.32f, 0.008f})
            .draw_as(Mesh::Cube(), shadow);
        s.NewChild("crack_r_b")
            .at({1.97f, 1.35f, 1.4f})
            .rot_z(17.0f)
            .scaled({0.005f, 0.24f, 0.007f})
            .draw_as(Mesh::Cube(), shadow);
        s.NewChild("crack_r_c")
            .at({1.97f, 0.66f, -0.6f})
            .rot_z(8.0f)
            .scaled({0.005f, 0.20f, 0.007f})
            .draw_as(Mesh::Cube(), shadow);

        // No full floor/ceiling slabs here. The renderer already owns the
        // ground receiver, and full blind slabs can dominate the depth buffer
        // from gaze-follow angles. Use thin perimeter rails for enclosure cues.
        s.NewChild("left_floor_rail")
            .at({b.min.x + 0.55f, 0.06f, shell_z})
            .scaled({0.10f, 0.045f, b.Size().z})
            .draw_as(Mesh::Cube(), floor);
        s.NewChild("right_floor_rail")
            .at({b.max.x - 0.55f, 0.06f, shell_z})
            .scaled({0.10f, 0.045f, b.Size().z})
            .draw_as(Mesh::Cube(), floor);
        s.NewChild("left_ceiling_rail")
            .at({b.min.x + 0.55f, b.max.y - 0.16f, shell_z})
            .scaled({0.12f, 0.040f, b.Size().z})
            .draw_as(Mesh::Cube(), wall);
        s.NewChild("right_ceiling_rail")
            .at({b.max.x - 0.55f, b.max.y - 0.16f, shell_z})
            .scaled({0.12f, 0.040f, b.Size().z})
            .draw_as(Mesh::Cube(), wall);
    }

    // ── blind_bg_deep_frames_8: receding doorframe layers creating depth
    {
        auto& s =
            root.NewChild("blind_bg_deep_frames_8").at({0.0f, 0.0f, 4.8f});

        // Three progressively smaller doorframe layers
        for (int i = 0; i < 4; ++i) {
            const float z_off = 0.7f * i;
            const float sc = 1.0f - 0.18f * i;
            s.NewChild("frame_l")
                .at({-0.46f * sc, 0.72f * sc, z_off})
                .scaled({0.038f, 1.36f * sc, 0.038f})
                .draw_as(Mesh::Cube(), edge);
            s.NewChild("frame_r")
                .at({0.46f * sc, 0.72f * sc, z_off})
                .scaled({0.038f, 1.36f * sc, 0.038f})
                .draw_as(Mesh::Cube(), edge);
            s.NewChild("lintel")
                .at({0.0f, 1.44f * sc, z_off})
                .scaled({0.50f * sc, 0.038f, 0.038f})
                .draw_as(Mesh::Cube(), edge);
        }

        // Floor at this depth
        s.NewChild("floor")
            .at({0.0f, 0.012f, 1.0f})
            .scaled({1.8f, 0.012f, 3.2f})
            .draw_as(Mesh::Cube(), floor);

        // Back dead-end wall
        s.NewChild("dead_end")
            .at({0.0f, 0.92f, 2.6f})
            .scaled({0.94f, 1.72f, 0.055f})
            .draw_as(Mesh::Cube(), cracked);
        // Large shadow cast on the dead-end wall
        s.NewChild("wall_shadow")
            .at({-0.18f, 0.76f, 2.55f})
            .rot_y(8.0f)
            .scaled({0.52f, 1.24f, 0.006f})
            .draw_as(Mesh::Cube(), shadow);
        s.NewChild("missing_void")
            .at({0.18f, 0.92f, 2.535f})
            .scaled({0.34f, 0.72f, 0.008f})
            .draw_as(Mesh::Cube(), void_mat);
        s.NewChild("void_frame_l")
            .at({-0.19f, 0.92f, 2.525f})
            .rot_z(-4.0f)
            .scaled({0.018f, 0.78f, 0.010f})
            .draw_as(Mesh::Cube(), edge);
        s.NewChild("void_frame_r_broken")
            .at({0.55f, 1.10f, 2.525f})
            .rot_z(9.0f)
            .scaled({0.018f, 0.40f, 0.010f})
            .draw_as(Mesh::Cube(), edge);
        s.NewChild("void_frame_top")
            .at({0.18f, 1.68f, 2.525f})
            .rot_z(2.0f)
            .scaled({0.36f, 0.018f, 0.010f})
            .draw_as(Mesh::Cube(), edge);
    }

    // ── blind_bg_shadow_field_9: diagonal long shadows on the floor
    {
        auto& s =
            root.NewChild("blind_bg_shadow_field_9").at({0.0f, 0.032f, 3.2f});
        constexpr float kSx[8] = {-1.4f, -0.9f, -0.4f, 0.1f,
                                  0.5f,  0.9f,  1.3f,  -0.1f};
        constexpr float kSz[8] = {0.0f,  0.4f, -0.3f, 0.7f,
                                  -0.5f, 0.2f, 0.6f,  -0.8f};
        constexpr float kSa[8] = {-18.0f, 27.0f, -11.0f, 34.0f,
                                  -23.0f, 15.0f, -8.0f,  42.0f};
        constexpr float kSw[8] = {0.45f, 0.30f, 0.52f, 0.26f,
                                  0.40f, 0.22f, 0.36f, 0.18f};
        constexpr float kSd[8] = {0.10f, 0.07f, 0.12f, 0.06f,
                                  0.09f, 0.05f, 0.08f, 0.05f};
        for (int i = 0; i < 8; ++i) {
            s.NewChild("shadow")
                .at({kSx[i], 0.0f, kSz[i]})
                .rot_y(kSa[i])
                .scaled({kSw[i], 0.003f, kSd[i]})
                .draw_as(Mesh::Cube(), shadow);
        }
    }
}

}  // namespace

SceneNode& BuildStoryProps(SceneNode& parent, StoryPropAssets assets) {
    auto& root = parent.NewChild("story_props");
    AddFutureAccidentSet(root, assets);
    AddFuturePredictionLab(root, assets);
    AddLongingAltars(root, assets);
    AddRomanticMemorySet(root, assets);
    AddBlindCompleteWorld(root, assets);
    AddBlindBackgroundWorld(root);
    return root;
}

SceneNode& BuildEnvironmentShell(SceneNode& parent) {
    auto& root = parent.NewChild("environment_shell");
    AddFutureBgShell(root);
    AddMemoryBgShell(root);
    AddBlindBgExpansion(root);
    return root;
}

}  // namespace future_gaze::builders
