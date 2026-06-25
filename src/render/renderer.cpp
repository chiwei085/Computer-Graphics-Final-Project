#include "future_gaze/render/renderer.hpp"

#include <GL/freeglut.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <limits>

#include "future_gaze/render/obj_loader.hpp"
#include "future_gaze/scene/builders.hpp"

namespace future_gaze
{

namespace
{

constexpr float kTableTopY = 0.800f;
constexpr float kGroundShadowY = 0.004f;
constexpr float kTableShadowY = kTableTopY + 0.0015f;

// Three-point stylised rig.
//   KEY  : warm point light, upper front-left — shapes the primary volume from
//          a 3/4 angle (never frontal-flat) and casts the planar shadow.
//   FILL : cool directional light from front-right — a weak (~25% of key)
//          opposing wash that lifts the shadow side without flattening it.
//   RIM  : cyan directional light from behind-above — rakes the top/back edges
//          so the robot and drone separate cleanly from the dark backdrop.
constexpr std::array<float, 4> kKeyLightPos = {-2.60f, 6.40f, 3.40f, 1.0f};
constexpr std::array<float, 4> kFillLightDir = {0.66f, 0.34f, 0.55f, 0.0f};
constexpr std::array<float, 4> kRimLightDir = {-0.24f, 0.52f, -0.92f, 0.0f};

constexpr std::array<float, 4> kBlack = {0.0f, 0.0f, 0.0f, 1.0f};

constexpr std::array<float, 4> kGroundPlane = {0.0f, 1.0f, 0.0f,
                                               -kGroundShadowY};
constexpr std::array<float, 4> kTablePlane = {0.0f, 1.0f, 0.0f, -kTableShadowY};

void LoadTexOrCheckerboard(Texture& tex, const std::filesystem::path& path) {
    if (!tex.LoadFromFile(path)) {
        tex.CreateDebugCheckerboard();
    }
}

}  // namespace

void Renderer::Initialize() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);  // key
    glEnable(GL_LIGHT1);  // fill
    glEnable(GL_LIGHT2);  // rim
    glShadeModel(GL_SMOOTH);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glClearColor(0.045f, 0.072f, 0.105f, 1.0f);

    // Compute specular relative to the real camera (tighter, more localised
    // highlights) and add it on top of the textured colour so highlights stay
    // crisp instead of being multiplied down into a dull plastic sheen.
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);

    // Low, cool global ambient — keeps the darkest faces from going pure black
    // while staying well below the lit side so volume is never washed out.
    const std::array<float, 4> scene_ambient = {0.060f, 0.072f, 0.098f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, scene_ambient.data());

    // KEY — warm, the dominant volume-shaping light. Mild attenuation lets the
    // far floor fall off so brightness concentrates around the characters.
    const std::array<float, 4> key_ambient = {0.030f, 0.030f, 0.035f, 1.0f};
    const std::array<float, 4> key_diffuse = {1.18f, 1.08f, 0.92f, 1.0f};
    const std::array<float, 4> key_specular = {0.55f, 0.52f, 0.46f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, key_ambient.data());
    glLightfv(GL_LIGHT0, GL_DIFFUSE, key_diffuse.data());
    glLightfv(GL_LIGHT0, GL_SPECULAR, key_specular.data());
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.022f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0009f);

    // FILL — cool, directional, ~25% of key. No specular so it only lifts the
    // diffuse shadow side (preserving material colour) without adding
    // highlights.
    const std::array<float, 4> fill_diffuse = {0.24f, 0.29f, 0.40f, 1.0f};
    const std::array<float, 4> fill_specular = {0.0f, 0.0f, 0.0f, 1.0f};
    glLightfv(GL_LIGHT1, GL_AMBIENT, kBlack.data());
    glLightfv(GL_LIGHT1, GL_DIFFUSE, fill_diffuse.data());
    glLightfv(GL_LIGHT1, GL_SPECULAR, fill_specular.data());

    // RIM — cyan, directional from behind. Because it lights back/top faces it
    // never reaches the camera-facing front, so it draws a separating edge
    // without flattening the form.
    const std::array<float, 4> rim_diffuse = {0.34f, 0.60f, 0.74f, 1.0f};
    const std::array<float, 4> rim_specular = {0.36f, 0.58f, 0.68f, 1.0f};
    glLightfv(GL_LIGHT2, GL_AMBIENT, kBlack.data());
    glLightfv(GL_LIGHT2, GL_DIFFUSE, rim_diffuse.data());
    glLightfv(GL_LIGHT2, GL_SPECULAR, rim_specular.data());

    // Load textures — fall back to debug checkerboard if file is missing
    const std::filesystem::path tex_dir = "assets/textures";
    LoadTexOrCheckerboard(textures_["wood"], tex_dir / "wood.png");
    LoadTexOrCheckerboard(textures_["cloth"], tex_dir / "cloth.png");
    LoadTexOrCheckerboard(textures_["circuit"], tex_dir / "circuit.png");
    LoadTexOrCheckerboard(textures_["marble"], tex_dir / "marble.png");
    LoadTexOrCheckerboard(textures_["metal"], tex_dir / "metal.png");
    LoadTexOrCheckerboard(textures_["paper"], tex_dir / "paper.png");

    builders::TextureSet tex_set{
        .wood = GetTexture("wood"),
        .cloth = GetTexture("cloth"),
        .circuit = GetTexture("circuit"),
        .marble = GetTexture("marble"),
        .metal = GetTexture("metal"),
        .paper = GetTexture("paper"),
    };

    const std::filesystem::path obj_dir = "assets/obj";
    auto robonaut = ObjLoader::Load(obj_dir / "Robonaut_2.obj");
    auto ingenuity = ObjLoader::Load(obj_dir / "Ingenuity_Mars_Helicopter.obj");

    // Build the full scene graph
    scene_root_ = std::make_unique<SceneNode>("root");
    scene_root_->AddChild(builders::BuildDinnerTable(tex_set));
    scene_root_->AddChild(builders::BuildPredictionCore(tex_set));
    scene_root_->AddChild(builders::BuildAiCharacters(
        std::move(robonaut), std::move(ingenuity), tex_set));

    animation_.Bind(*scene_root_);

    ApplyProjection();
}

void Renderer::Resize(int width, int height) {
    width_ = std::max(1, width);
    height_ = std::max(1, height);
    glViewport(0, 0, width_, height_);
    ApplyProjection();
}

void Renderer::Update(float delta_seconds) {
    elapsed_seconds_ += delta_seconds;
    animation_.Update(delta_seconds);
}

void Renderer::Render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    const Mat4 view = camera_.ViewMatrix();
    glLoadMatrixf(view.Data());

    ApplyLighting();

    DrawGroundReceiver();

    if (scene_root_) {
        scene_root_->Draw();
        DrawPlanarShadowOnTable();
        DrawPlanarShadowOnGround();
    }

    glutSwapBuffers();
}

void Renderer::BeginCameraDrag(int x, int y) {
    camera_.BeginDrag(x, y);
}

void Renderer::DragCameraTo(int x, int y) {
    camera_.DragTo(x, y);
}

void Renderer::EndCameraDrag() {
    camera_.EndDrag();
}

void Renderer::BeginCameraPan(int x, int y) {
    camera_.BeginPan(x, y);
}

void Renderer::PanCameraTo(int x, int y) {
    camera_.PanTo(x, y);
}

void Renderer::EndCameraPan() {
    camera_.EndPan();
}

void Renderer::ZoomCamera(float wheel_steps) {
    camera_.Zoom(wheel_steps);
}

void Renderer::ResetCamera() {
    camera_.Reset();
}

const Texture* Renderer::GetTexture(const std::string& name) const {
    auto it = textures_.find(name);
    return (it != textures_.end()) ? &it->second : nullptr;
}

void Renderer::ApplyLighting() const {
    // Light positions/directions are transformed by the current model-view, so
    // re-submit them every frame after the camera matrix is loaded. Key is a
    // positional light (w=1); fill and rim are directional (w=0).
    glLightfv(GL_LIGHT0, GL_POSITION, kKeyLightPos.data());
    glLightfv(GL_LIGHT1, GL_POSITION, kFillLightDir.data());
    glLightfv(GL_LIGHT2, GL_POSITION, kRimLightDir.data());
}

void Renderer::ApplyProjection() const {
    const float aspect =
        static_cast<float>(width_) / static_cast<float>(height_);
    const Mat4 proj = Mat4::Perspective(DegToRad(55.0f), aspect, 0.1f, 50.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(proj.Data());
}

void Renderer::DrawGroundReceiver() const {
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_TEXTURE_2D);
    ground_material_.Apply();
    DrawReceiverBox({0.0f, -0.020f, 0.0f}, {5.20f, 0.020f, 4.20f});
    glPopAttrib();
}

void Renderer::DrawPlanarShadowOnGround() {
    if (scene_root_ == nullptr) {
        return;
    }
    DrawShadowPass(MakeShadowMatrix(kKeyLightPos, kGroundPlane),
                   {0.0f, -0.020f, 0.0f}, {5.20f, 0.020f, 4.20f}, *scene_root_,
                   -std::numeric_limits<float>::infinity(),
                   std::numeric_limits<float>::infinity());
}

void Renderer::DrawPlanarShadowOnTable() {
    if (scene_root_ == nullptr) {
        return;
    }
    SceneNode* dinner_scene = scene_root_->Find("dinner_scene");
    if (dinner_scene == nullptr) {
        return;
    }
    DrawShadowPass(MakeShadowMatrix(kKeyLightPos, kTablePlane),
                   {0.0f, kTableTopY + 0.001f, 0.0f}, {2.10f, 0.001f, 0.95f},
                   *dinner_scene, kTableTopY + 0.001f, 1.80f);
}

void Renderer::DrawReceiverBox(Vec3 center, Vec3 scale) const {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(center.x, center.y, center.z);
    glScalef(scale.x, scale.y, scale.z);
    receiver_mesh_.Draw();
    glPopMatrix();
}

void Renderer::DrawShadowPass(const std::array<float, 16>& shadow_matrix,
                              Vec3 receiver_center, Vec3 receiver_scale,
                              SceneNode& caster_root, float min_caster_y,
                              float max_caster_y) const {
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                 GL_STENCIL_BUFFER_BIT | GL_POLYGON_BIT);

    glEnable(GL_STENCIL_TEST);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    DrawReceiverBox(receiver_center, receiver_scale);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    // Write-once shadow accumulation: the receiver mask starts at 1. The first
    // shadow fragment that survives depth testing blends once, then increments
    // the pixel to 2 so overlapping projected triangles cannot darken it again.
    glStencilMask(0xFF);
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glColor4f(0.0f, 0.0f, 0.0f, 0.16f);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixf(shadow_matrix.data());
    caster_root.DrawShadow(min_caster_y, max_caster_y);
    glPopMatrix();

    glPopAttrib();
}

std::array<float, 16> Renderer::MakeShadowMatrix(
    const std::array<float, 4>& light_pos, const std::array<float, 4>& plane) {
    const float dot = plane[0] * light_pos[0] + plane[1] * light_pos[1] +
                      plane[2] * light_pos[2] + plane[3] * light_pos[3];

    std::array<float, 16> matrix{};
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            matrix[static_cast<std::size_t>(col * 4 + row)] =
                ((row == col) ? dot : 0.0f) - light_pos[row] * plane[col];
        }
    }
    return matrix;
}

}  // namespace future_gaze
