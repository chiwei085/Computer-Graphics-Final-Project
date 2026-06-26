#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include "future_gaze/scene/builders.hpp"

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

// OBJ materials are tuned for offline renderers; re-grade for fixed-function GL.
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

enum class RobonautDanceRole
{
    Center,
    LowerBody,
    UpperBody,
    Head,
    LeftArm,
    RightArm,
    LeftAnkle,
    RightAnkle
};

enum class RobonautDanceJoint
{
    Center,
    LowerBody,
    UpperBody,
    Head,
    LeftShoulder,
    LeftArm,
    LeftElbow,
    LeftWrist,
    RightShoulder,
    RightArm,
    RightElbow,
    RightWrist,
    LeftLeg,
    LeftKnee,
    LeftAnkle,
    RightLeg,
    RightKnee,
    RightAnkle,
    Count
};

constexpr std::size_t kRobonautDanceJointCount =
    static_cast<std::size_t>(RobonautDanceJoint::Count);

[[nodiscard]] constexpr std::size_t JointIndex(RobonautDanceJoint joint) {
    return static_cast<std::size_t>(joint);
}

struct RobonautDanceJointNode
{
    SceneNode* node = nullptr;
    Vec3 pivot{};
};

struct RobonautDanceNodes
{
    std::array<RobonautDanceJointNode, kRobonautDanceJointCount> joint{};

    [[nodiscard]] RobonautDanceJointNode& operator[](
        RobonautDanceJoint id) noexcept {
        return joint[JointIndex(id)];
    }

    [[nodiscard]] const RobonautDanceJointNode& operator[](
        RobonautDanceJoint id) const noexcept {
        return joint[JointIndex(id)];
    }
};

struct RobonautDanceJointSpec
{
    RobonautDanceJoint id;
    RobonautDanceJoint parent;
    const char* name;
    Vec3 pivot;
};

constexpr RobonautDanceJoint kNoRobonautDanceParent = RobonautDanceJoint::Count;

constexpr std::array<RobonautDanceJointSpec, kRobonautDanceJointCount>
    kRobonautDanceJointSpecs = {{
        {RobonautDanceJoint::Center,
         kNoRobonautDanceParent,
         names::kRobonautDanceCenter,
         {0.0f, 0.18f, 0.0f}},
        {RobonautDanceJoint::LowerBody,
         RobonautDanceJoint::Center,
         names::kRobonautDanceLowerBody,
         {0.0f, -0.12f, 0.0f}},
        {RobonautDanceJoint::UpperBody,
         RobonautDanceJoint::Center,
         names::kRobonautDanceUpperBody,
         {0.0f, 0.24f, 0.0f}},
        {RobonautDanceJoint::Head,
         RobonautDanceJoint::UpperBody,
         names::kRobonautDanceHead,
         {0.0f, 0.43f, -0.02f}},
        {RobonautDanceJoint::LeftShoulder,
         RobonautDanceJoint::UpperBody,
         names::kRobonautDanceLeftShoulder,
         {-0.18f, 0.31f, -0.04f}},
        {RobonautDanceJoint::LeftArm,
         RobonautDanceJoint::LeftShoulder,
         names::kRobonautDanceLeftArm,
         {-0.32f, 0.28f, -0.08f}},
        {RobonautDanceJoint::LeftElbow,
         RobonautDanceJoint::LeftArm,
         names::kRobonautDanceLeftElbow,
         {-0.40f, 0.24f, -0.08f}},
        {RobonautDanceJoint::LeftWrist,
         RobonautDanceJoint::LeftElbow,
         names::kRobonautDanceLeftWrist,
         {-0.45f, 0.22f, -0.08f}},
        {RobonautDanceJoint::RightShoulder,
         RobonautDanceJoint::UpperBody,
         names::kRobonautDanceRightShoulder,
         {0.18f, 0.31f, -0.04f}},
        {RobonautDanceJoint::RightArm,
         RobonautDanceJoint::RightShoulder,
         names::kRobonautDanceRightArm,
         {0.32f, 0.28f, -0.08f}},
        {RobonautDanceJoint::RightElbow,
         RobonautDanceJoint::RightArm,
         names::kRobonautDanceRightElbow,
         {0.40f, 0.24f, -0.08f}},
        {RobonautDanceJoint::RightWrist,
         RobonautDanceJoint::RightElbow,
         names::kRobonautDanceRightWrist,
         {0.45f, 0.22f, -0.08f}},
        {RobonautDanceJoint::LeftLeg,
         RobonautDanceJoint::LowerBody,
         names::kRobonautDanceLeftLeg,
         {-0.08f, -0.18f, 0.0f}},
        {RobonautDanceJoint::LeftKnee,
         RobonautDanceJoint::LeftLeg,
         names::kRobonautDanceLeftKnee,
         {-0.09f, -0.32f, 0.0f}},
        {RobonautDanceJoint::LeftAnkle,
         RobonautDanceJoint::LeftKnee,
         names::kRobonautDanceLeftAnkle,
         {-0.10f, -0.41f, 0.0f}},
        {RobonautDanceJoint::RightLeg,
         RobonautDanceJoint::LowerBody,
         names::kRobonautDanceRightLeg,
         {0.08f, -0.18f, 0.0f}},
        {RobonautDanceJoint::RightKnee,
         RobonautDanceJoint::RightLeg,
         names::kRobonautDanceRightKnee,
         {0.09f, -0.32f, 0.0f}},
        {RobonautDanceJoint::RightAnkle,
         RobonautDanceJoint::RightKnee,
         names::kRobonautDanceRightAnkle,
         {0.10f, -0.41f, 0.0f}},
    }};

RobonautDanceNodes AddRobonautDancePivots(SceneNode& rb) {
    auto& rig = rb.NewChild(names::kRobonautDanceRoot);
    RobonautDanceNodes nodes;
    for (const RobonautDanceJointSpec& spec : kRobonautDanceJointSpecs) {
        SceneNode* parent = &rig;
        Vec3 parent_pivot{};
        if (spec.parent != kNoRobonautDanceParent) {
            const RobonautDanceJointNode& parent_joint = nodes[spec.parent];
            parent = parent_joint.node;
            parent_pivot = parent_joint.pivot;
        }
        if (parent == nullptr) {
            continue;
        }
        nodes[spec.id] = {
            &parent->NewChild(spec.name).at(spec.pivot - parent_pivot),
            spec.pivot,
        };
    }
    return nodes;
}

struct RobonautDanceTarget
{
    SceneNode* node = nullptr;
    Vec3 pivot;
};

RobonautDanceTarget TargetForJoint(RobonautDanceNodes& nodes,
                                   RobonautDanceJoint joint) {
    const RobonautDanceJointNode& target = nodes[joint];
    return {target.node, target.pivot};
}

RobonautDanceTarget NodeForRole(RobonautDanceNodes& nodes,
                                RobonautDanceRole role) {
    switch (role) {
        case RobonautDanceRole::Center:
            return TargetForJoint(nodes, RobonautDanceJoint::Center);
        case RobonautDanceRole::LowerBody:
            return TargetForJoint(nodes, RobonautDanceJoint::LowerBody);
        case RobonautDanceRole::UpperBody:
            return TargetForJoint(nodes, RobonautDanceJoint::UpperBody);
        case RobonautDanceRole::Head:
            return TargetForJoint(nodes, RobonautDanceJoint::Head);
        case RobonautDanceRole::LeftArm:
            return TargetForJoint(nodes, RobonautDanceJoint::LeftArm);
        case RobonautDanceRole::RightArm:
            return TargetForJoint(nodes, RobonautDanceJoint::RightArm);
        case RobonautDanceRole::LeftAnkle:
            return TargetForJoint(nodes, RobonautDanceJoint::LeftAnkle);
        case RobonautDanceRole::RightAnkle:
            return TargetForJoint(nodes, RobonautDanceJoint::RightAnkle);
    }
    return TargetForJoint(nodes, RobonautDanceJoint::UpperBody);
}

RobonautDanceRole ClassifyRobonautPart(const ModelMesh& part) {
    // No per-joint segmentation; main cloth/spine span full height.
    // Splitting causes tearing, so only helmet pieces (high y) get Head.
    const Vec3 center = part.mesh.Bounds().Center();
    if (center.y > 0.38f) {
        return RobonautDanceRole::Head;
    }
    return RobonautDanceRole::Center;
}

void AddRobonautPartToDanceNode(RobonautDanceNodes& nodes, ModelMesh part) {
    const RobonautDanceTarget target =
        NodeForRole(nodes, ClassifyRobonautPart(part));
    if (target.node == nullptr) {
        return;
    }
    target.node->NewChild("part")
        .at({-target.pivot.x, -target.pivot.y, -target.pivot.z})
        .draw_as(std::move(part.mesh), part.material);
}

}  // namespace

SceneNode& BuildAiCharacters(SceneNode& parent, std::vector<ModelMesh> robonaut,
                             std::vector<ModelMesh> ingenuity,
                             const TextureSet& tex) {
    (void)tex;
    auto& root = parent.NewChild("ai_characters");

    // Model Y: -0.50..+0.50 (normalised). Scale 1.7 → ~1.7 m; offset 0.85 floors feet.
    if (!robonaut.empty()) {
        auto& rb = root.NewChild(names::kRobonaut)
                       .at({2.40f, 0.85f, 0.50f})
                       .scaled({1.7f, 1.7f, 1.7f})
                       .rot_y(-150.0f);
        RobonautDanceNodes dance_nodes = AddRobonautDancePivots(rb);
        for (auto& mm : robonaut) {
            ApplyRobonautLakersPalette(mm.material, mm.material_name);
            AddRobonautPartToDanceNode(dance_nodes, std::move(mm));
        }
    }

    if (!ingenuity.empty()) {
        auto& ing = root.NewChild(names::kIngenuity)
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
