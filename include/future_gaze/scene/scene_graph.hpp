#pragma once

#include <GL/freeglut.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "future_gaze/render/material.hpp"
#include "future_gaze/render/mesh.hpp"
#include "future_gaze/render/texture.hpp"
#include "future_gaze/scene/tf_tree.hpp"

namespace future_gaze
{

// ── Node names ───────────────────────────────────────────────────────────────
// String keys shared between builders (which CREATE nodes) and the animation /
// gaze systems (which FIND them). Centralising here means a rename is a single
// edit and a missing node fails loudly at startup.
namespace names
{

inline constexpr const char* kPredictionCore = "prediction_core";
inline constexpr const char* kCircuitRing = "circuit_ring";
inline constexpr const char* kIngenuity = "ingenuity";
inline constexpr const char* kRobonaut = "robonaut";

inline constexpr const char* kRobonautDanceRoot = "robonaut_dance_root";
inline constexpr const char* kRobonautDanceCenter = "robonaut_dance_center";
inline constexpr const char* kRobonautDanceLowerBody =
    "robonaut_dance_lower_body";
inline constexpr const char* kRobonautDanceUpperBody =
    "robonaut_dance_upper_body";
inline constexpr const char* kRobonautDanceHead = "robonaut_dance_head";
inline constexpr const char* kRobonautDanceLeftShoulder =
    "robonaut_dance_left_shoulder";
inline constexpr const char* kRobonautDanceLeftArm = "robonaut_dance_left_arm";
inline constexpr const char* kRobonautDanceLeftElbow =
    "robonaut_dance_left_elbow";
inline constexpr const char* kRobonautDanceLeftWrist =
    "robonaut_dance_left_wrist";
inline constexpr const char* kRobonautDanceRightShoulder =
    "robonaut_dance_right_shoulder";
inline constexpr const char* kRobonautDanceRightArm =
    "robonaut_dance_right_arm";
inline constexpr const char* kRobonautDanceRightElbow =
    "robonaut_dance_right_elbow";
inline constexpr const char* kRobonautDanceRightWrist =
    "robonaut_dance_right_wrist";
inline constexpr const char* kRobonautDanceLeftLeg = "robonaut_dance_left_leg";
inline constexpr const char* kRobonautDanceLeftKnee =
    "robonaut_dance_left_knee";
inline constexpr const char* kRobonautDanceLeftAnkle =
    "robonaut_dance_left_ankle";
inline constexpr const char* kRobonautDanceRightLeg =
    "robonaut_dance_right_leg";
inline constexpr const char* kRobonautDanceRightKnee =
    "robonaut_dance_right_knee";
inline constexpr const char* kRobonautDanceRightAnkle =
    "robonaut_dance_right_ankle";

inline constexpr const char* kFuturePrefix = "future_";
inline constexpr const char* kMemoryPrefix = "memory_";
inline constexpr const char* kBlindPrefix = "blind_";

// Legacy aliases
inline constexpr const char* kFutureSlice = kFuturePrefix;
inline constexpr const char* kMemoryPanel = kMemoryPrefix;

}  // namespace names

// ── Renderable
// ──────────────────────────────────────────────────────────────── A Mesh and
// its Material treated as one lifecycle unit. texture is non-owning; nullptr
// means no texture (GL_TEXTURE_2D disabled). additive: unlit additive halo
// (fake emissive glow) — used for the Prediction Core without any shader.
struct Renderable
{
    Mesh mesh;
    Material material;
    const Texture* texture = nullptr;
    bool additive = false;

    void Draw() const {
        if (additive) {
            DrawGlow();
            return;
        }
        material.Apply();
        if (texture != nullptr) {
            glEnable(GL_TEXTURE_2D);
            texture->Bind();
        }
        else {
            glDisable(GL_TEXTURE_2D);
        }
        mesh.Draw();
        if (texture != nullptr) {
            glDisable(GL_TEXTURE_2D);
        }
    }

    void DrawShadow() const {
        if (additive) {
            return;
        }
        mesh.Draw();
    }

private:
    void DrawGlow() const {
        glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        material.Apply();
        mesh.Draw();
        glPopAttrib();
    }
};

// ── SceneNode
// ─────────────────────────────────────────────────────────────────
class SceneGraph;

class SceneNode
{
public:
    SceneNode& NewChild(std::string name = {});

    SceneNode& at(Vec3 pos) {
        Transform local = Local();
        local.position = pos;
        SetLocal(local);
        return *this;
    }
    SceneNode& rot_y(float deg) {
        Transform local = Local();
        local.euler_deg.y = deg;
        SetLocal(local);
        return *this;
    }
    SceneNode& rot_x(float deg) {
        Transform local = Local();
        local.euler_deg.x = deg;
        SetLocal(local);
        return *this;
    }
    SceneNode& rot_z(float deg) {
        Transform local = Local();
        local.euler_deg.z = deg;
        SetLocal(local);
        return *this;
    }
    SceneNode& scaled(Vec3 s) {
        Transform local = Local();
        local.scale = s;
        SetLocal(local);
        return *this;
    }
    SceneNode& draw_as(Mesh m, Material mat) {
        renderable_.emplace(Renderable{std::move(m), std::move(mat)});
        return *this;
    }
    SceneNode& draw_as(Mesh m, Material mat, const Texture* tex) {
        renderable_.emplace(Renderable{std::move(m), std::move(mat), tex});
        return *this;
    }
    SceneNode& draw_glow(Mesh m, Material mat) {
        renderable_.emplace(
            Renderable{std::move(m), std::move(mat), nullptr, true});
        return *this;
    }

    void Draw() const;
    void DrawShadow(float min_world_y, float max_world_y) const;

    [[nodiscard]] const std::string& name() const;
    [[nodiscard]] TfHandle handle() const noexcept { return handle_; }
    [[nodiscard]] Transform Local() const;
    void SetLocal(Transform local);

private:
    friend class SceneGraph;

    SceneNode(SceneGraph& graph, TfHandle handle);

    SceneGraph* graph_ = nullptr;
    TfHandle handle_{};
    std::optional<Renderable> renderable_;
    std::vector<std::unique_ptr<SceneNode>> children_;
};

// ── SceneGraph
// ────────────────────────────────────────────────────────────────
class SceneGraph
{
public:
    explicit SceneGraph(std::string root_name = "root");

    [[nodiscard]] SceneNode& Root() noexcept { return *root_; }
    [[nodiscard]] const SceneNode& Root() const noexcept { return *root_; }
    [[nodiscard]] TfTree& tf() noexcept { return tf_; }
    [[nodiscard]] const TfTree& tf() const noexcept { return tf_; }

    [[nodiscard]] TfHandle Find(const std::string& name) const;
    void Collect(const std::string& prefix, std::vector<TfHandle>& out) const;

    void Draw() const;
    void DrawShadow(TfHandle root, float min_world_y, float max_world_y) const;

private:
    friend class SceneNode;

    [[nodiscard]] SceneNode& NewChild(SceneNode& parent, std::string name);
    [[nodiscard]] const SceneNode* NodeFor(TfHandle handle) const;
    [[nodiscard]] static const SceneNode* FindInTree(const SceneNode& node,
                                                     TfHandle handle) noexcept;

    TfTree tf_;
    std::unique_ptr<SceneNode> root_;
};

}  // namespace future_gaze
