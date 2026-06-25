#pragma once

#include <memory>
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

// Dinner table with chairs, place settings, cake, flowers, letter.
[[nodiscard]] std::unique_ptr<SceneNode> BuildDinnerTable(
    const TextureSet& tex);

// Layered Prediction Core AI eye.
[[nodiscard]] std::unique_ptr<SceneNode> BuildPredictionCore(
    const TextureSet& tex);

// Robonaut (table side) + Ingenuity (orbit start position).
[[nodiscard]] std::unique_ptr<SceneNode> BuildAiCharacters(
    std::vector<ModelMesh> robonaut, std::vector<ModelMesh> ingenuity,
    const TextureSet& tex);

}  // namespace future_gaze::builders
