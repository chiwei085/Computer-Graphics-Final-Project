#pragma once

#include <vector>

#include "future_gaze/render/obj_loader.hpp"
#include "future_gaze/render/texture.hpp"
#include "future_gaze/scene/scene_node.hpp"

namespace future_gaze::builders
{

struct TextureSet
{
    const Texture* wood = nullptr;
    const Texture* cloth = nullptr;
    const Texture* circuit = nullptr;
    const Texture* marble = nullptr;
    const Texture* metal = nullptr;
    const Texture* paper = nullptr;
};

struct StoryPropAssets
{
    std::vector<ModelMesh> infrared_camera;
    std::vector<ModelMesh> astronaut_glove;
    std::vector<ModelMesh> crew_lock_bag;
    std::vector<ModelMesh> dirty_plate;
    std::vector<ModelMesh> bowl_dirty;
    std::vector<ModelMesh> stew;
    std::vector<ModelMesh> cutting_board;
    std::vector<ModelMesh> chopsticks;
    std::vector<ModelMesh> present;
    std::vector<ModelMesh> wall_corkboard;
    std::vector<ModelMesh> mug_office_tool;
    std::vector<ModelMesh> closed_umbrella;
    std::vector<ModelMesh> zz_plant;
    std::vector<ModelMesh> cyberpunk_platform;
    std::vector<ModelMesh> sci_fi_floor_tile;
    std::vector<ModelMesh> sci_fi_wall_3;
    std::vector<ModelMesh> curtains_double;
    std::vector<ModelMesh> lamp_with_shade;
    std::vector<ModelMesh> candles;
};

// Dinner table with chairs, place settings, cake, flowers, letter.
SceneNode& BuildDinnerTable(SceneNode& parent, const TextureSet& tex);

// Layered Prediction Core AI eye.
SceneNode& BuildPredictionCore(SceneNode& parent, const TextureSet& tex);

// Robonaut (table side) + Ingenuity (orbit start position).
SceneNode& BuildAiCharacters(SceneNode& parent, std::vector<ModelMesh> robonaut,
                             std::vector<ModelMesh> ingenuity,
                             const TextureSet& tex);

// Imported story props: accident chain, memory altars, quiet blind objects.
SceneNode& BuildStoryProps(SceneNode& parent, StoryPropAssets assets);

// Fixed world-space room shells/backdrops. These are intentionally separate
// from story_props so G-key restaging cannot rotate walls into the camera.
SceneNode& BuildEnvironmentShell(SceneNode& parent);

}  // namespace future_gaze::builders
