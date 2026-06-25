#include <string>
#include <string_view>

#include "future_gaze/scene/builders.hpp"
#include "future_gaze/scene/node_names.hpp"

namespace future_gaze::builders
{

namespace
{

constexpr Material::Color kLakersPurple = {85.0f / 255.0f, 37.0f / 255.0f,
                                           131.0f / 255.0f, 1.0f};
constexpr Material::Color kLakersGold = {253.0f / 255.0f, 185.0f / 255.0f,
                                         39.0f / 255.0f, 1.0f};
constexpr Material::Color kFullWhite = {1.0f, 1.0f, 1.0f, 1.0f};
constexpr Material::Color kBlack = {0.0f, 0.0f, 0.0f, 1.0f};

[[nodiscard]] bool Contains(std::string_view text, std::string_view token) {
    return text.find(token) != std::string_view::npos;
}

// Imported OBJ/MTL materials are tuned for offline renderers (e.g. Robonaut
// ships Ks=2.0, which blows out into a flat plastic highlight under fixed-
// function lighting). Re-grade them for the stylised real-time rig:
//   - tight, low-energy specular so highlights stay small and crisp;
//   - ambient pulled toward the diffuse hue so shadowed faces keep their
//     material colour instead of collapsing to neutral grey.
void StylizeImportedMaterial(Material& mat) {
    const Material::Color d = mat.diffuse();
    mat.SetAmbient({d[0] * 0.30f, d[1] * 0.30f, d[2] * 0.34f, 1.0f});
    mat.SetSpecular({0.30f, 0.31f, 0.33f, 1.0f});
    mat.SetShininess(48.0f);
}

void ApplyRobonautLakersPalette(Material& mat, std::string_view material_name) {
    if (Contains(material_name, "Black") || Contains(material_name, "Visor")) {
        mat.SetDiffuse(kBlack);
        mat.SetAmbient({0.006f, 0.005f, 0.007f, 1.0f});
        mat.SetSpecular({0.42f, 0.38f, 0.46f, 1.0f});
        mat.SetShininess(80.0f);
        return;
    }

    if (Contains(material_name, "Gold") ||
        Contains(material_name, "ChestCover")) {
        mat.SetDiffuse(kLakersGold);
        mat.SetAmbient({0.34f, 0.22f, 0.035f, 1.0f});
        mat.SetSpecular({0.82f, 0.66f, 0.26f, 1.0f});
        mat.SetShininess(72.0f);
        return;
    }

    if (Contains(material_name, "Chrome") || Contains(material_name, "Grey")) {
        mat.SetDiffuse(kFullWhite);
        mat.SetAmbient({0.22f, 0.21f, 0.24f, 1.0f});
        mat.SetSpecular({0.80f, 0.78f, 0.84f, 1.0f});
        mat.SetShininess(96.0f);
        return;
    }

    mat.SetDiffuse(kLakersPurple);
    mat.SetAmbient({0.10f, 0.045f, 0.18f, 1.0f});
    mat.SetSpecular({0.36f, 0.24f, 0.46f, 1.0f});
    mat.SetShininess(56.0f);
}

}  // namespace

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
            ApplyRobonautLakersPalette(mm.material, mm.material_name);
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
            StylizeImportedMaterial(mm.material);
            ing.NewChild("part").draw_as(std::move(mm.mesh), mm.material);
        }
    }

    return root;
}

}  // namespace future_gaze::builders
