#pragma once

#include <memory>
#include <vector>

#include "future_gaze/render/obj_loader.hpp"
#include "future_gaze/scene/scene_node.hpp"

namespace future_gaze::builders
{

// 2.2 — dinner table with chairs, place settings, cake, flowers, letter
[[nodiscard]] std::unique_ptr<SceneNode> BuildDinnerTable();

// 2.3 — layered Prediction Core AI eye
[[nodiscard]] std::unique_ptr<SceneNode> BuildPredictionCore();

// 2.4 — Robonaut (table side) + Ingenuity (orbit start position)
[[nodiscard]] std::unique_ptr<SceneNode> BuildAiCharacters(
    std::vector<ModelMesh> robonaut, std::vector<ModelMesh> ingenuity);

}  // namespace future_gaze::builders
