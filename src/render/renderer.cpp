#include "future_gaze/render/renderer.hpp"

#include <GL/freeglut.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>

#include "future_gaze/render/obj_loader.hpp"
#include "future_gaze/scene/builders.hpp"

namespace future_gaze
{

namespace
{

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
    glEnable(GL_LIGHT0);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glClearColor(0.05f, 0.09f, 0.13f, 1.0f);

    const std::array<float, 4> light_ambient = {0.08f, 0.10f, 0.13f, 1.0f};
    const std::array<float, 4> light_diffuse = {0.86f, 0.91f, 0.95f, 1.0f};
    const std::array<float, 4> light_specular = {0.55f, 0.62f, 0.68f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient.data());
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse.data());
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular.data());

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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    const Mat4 view = camera_.ViewMatrix();
    glLoadMatrixf(view.Data());

    ApplyLighting();

    if (scene_root_) {
        scene_root_->Draw();
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
    // Light position is set in eye space (after view matrix), so world position
    // needs to be given each frame.
    const std::array<float, 4> pos = {-0.35f, 0.80f, 0.45f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, pos.data());
}

void Renderer::ApplyProjection() const {
    const float aspect =
        static_cast<float>(width_) / static_cast<float>(height_);
    const Mat4 proj = Mat4::Perspective(DegToRad(55.0f), aspect, 0.1f, 50.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(proj.Data());
}

}  // namespace future_gaze
