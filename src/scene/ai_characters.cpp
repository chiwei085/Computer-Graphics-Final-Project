#include "future_gaze/scene/builders.hpp"
#include "future_gaze/scene/node_names.hpp"

namespace future_gaze::builders
{

std::unique_ptr<SceneNode> BuildAiCharacters(std::vector<ModelMesh> robonaut,
                                             std::vector<ModelMesh> ingenuity,
                                             const TextureSet& tex) {
    (void)tex;
    auto root = std::make_unique<SceneNode>("ai_characters");

    // ── ROBONAUT 2 (standing beside the table)
    // ──────────────────────────────── Model spans Y: -0.50..+0.50
    // (normalised). Scale 1.7 → ~1.7 m tall. Translate up by scale*0.5 = 0.85
    // so feet sit at y=0.
    if (!robonaut.empty()) {
        auto& rb = root->NewChild(names::kRobonaut)
                       .at({2.40f, 0.85f, 0.50f})
                       .scaled({1.7f, 1.7f, 1.7f})
                       .rot_y(-150.0f);
        for (auto& mm : robonaut) {
            rb.NewChild("part").draw_as(std::move(mm.mesh), mm.material);
        }
    }

    // ── INGENUITY (initial orbit position near the Prediction Core)
    // ───────────
    if (!ingenuity.empty()) {
        auto& ing = root->NewChild(names::kIngenuity)
                        .at({1.6f, 3.8f, -1.5f})
                        .scaled({0.55f, 0.55f, 0.55f})
                        .rot_y(20.0f);
        for (auto& mm : ingenuity) {
            ing.NewChild("part").draw_as(std::move(mm.mesh), mm.material);
        }
    }

    return root;
}

}  // namespace future_gaze::builders
