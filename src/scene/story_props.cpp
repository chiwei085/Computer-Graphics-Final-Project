#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "future_gaze/render/material.hpp"
#include "future_gaze/render/mesh.hpp"
#include "future_gaze/scene/builders.hpp"
#include "future_gaze/scene/node_names.hpp"

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

}  // namespace

SceneNode& BuildStoryProps(SceneNode& parent, StoryPropAssets assets) {
    auto& root = parent.NewChild("story_props");
    AddFutureAccidentSet(root, assets);
    AddLongingAltars(root, assets);
    AddBlindCompleteWorld(root, assets);
    return root;
}

}  // namespace future_gaze::builders
