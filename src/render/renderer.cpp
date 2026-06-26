#include "future_gaze/render/renderer.hpp"

#include <GL/freeglut.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numbers>
#include <sstream>

#include "future_gaze/anim/easing.hpp"
#ifdef FUTURE_GAZE_OBSERVER_TOOLS
#include "future_gaze/observer/frame_stream.hpp"
#endif
#include "future_gaze/render/obj_loader.hpp"
#include "future_gaze/scene/builders.hpp"

namespace future_gaze
{

namespace
{

using render_detail::ZonePalette;

constexpr float kTableTopY = 0.800f;
constexpr float kGroundShadowY = 0.004f;
constexpr float kTableShadowY = kTableTopY + 0.0015f;

// Duration of the table re-stage swing when leaving gaze-drag mode (G key): the
// dinner group orbits the eye to sit in front of its new gaze bearing.
constexpr float kTableRestageSeconds = 1.4f;
// The camera follow rides the same G-exit but offset from the table: it waits
// this long so the table swing reads first, then glides over its own (longer)
// duration, arriving after the table has settled.
constexpr float kCameraRestageDelay = 0.55f;
constexpr float kCameraTransitionSeconds = 1.7f;
constexpr float kCameraFovYDeg = 55.0f;
constexpr float kCameraNearPlane = 0.1f;

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

// ── Per-gaze-zone mood ─────────────────────────────────────────────────────
// Each gaze zone restages the whole table with its own light colours, global
// ambient, background/fog tint and shadow weight. The eased zone weights from
// GazeController blend these every frame, giving the ~0.6s crossfade for free.
// Specular and attenuation are NOT per-zone (set once in Initialize) so the
// light *roles* (key shapes volume, fill lifts the shadow side, rim draws the
// edge) stay constant while only their colour/mood shifts.
constexpr std::array<ZonePalette, 3> kZonePalettes = {{
    // 預見 Foresight — cool, clean, "computed": near-white cool key, strong
    // cyan
    // rim, deep-navy air, fairly defined shadows.
    {{1.10f, 1.06f, 1.00f, 1.0f},
     {0.20f, 0.27f, 0.40f, 1.0f},
     {0.34f, 0.62f, 0.78f, 1.0f},
     {0.050f, 0.062f, 0.085f, 1.0f},
     {0.045f, 0.072f, 0.105f},
     13.0f,
     26.0f,
     0.22f},
    // 眷戀 Longing — warm, soft, intimate: amber key, warm low fill, hazy amber
    // air pulled closer, light soft shadows.
    {{1.26f, 1.00f, 0.70f, 1.0f},
     {0.34f, 0.24f, 0.22f, 1.0f},
     {0.52f, 0.38f, 0.28f, 1.0f},
     {0.094f, 0.068f, 0.050f, 1.0f},
     {0.110f, 0.074f, 0.052f},
     12.0f,
     22.0f,
     0.14f},
    // 盲點 Blindspot — neutral, desaturated, flat and stark: cold even light,
    // raised ambient (low contrast), weak rim, hard dark shadows. The world is
    // simply itself.
    {{0.86f, 0.90f, 0.94f, 1.0f},
     {0.42f, 0.45f, 0.48f, 1.0f},
     {0.18f, 0.20f, 0.22f, 1.0f},
     {0.106f, 0.113f, 0.123f, 1.0f},
     {0.058f, 0.063f, 0.070f},
     14.0f,
     28.0f,
     0.30f},
}};

// Weighted blend of the three palettes by the eased zone weights (which sum to
// ~1). Accumulates from a zeroed palette.
ZonePalette BlendZonePalette(float w0, float w1, float w2) {
    const std::array<float, 3> w = {w0, w1, w2};
    ZonePalette out{};
    for (std::size_t z = 0; z < kZonePalettes.size(); ++z) {
        const ZonePalette& p = kZonePalettes[z];
        const float k = w[z];
        for (int c = 0; c < 4; ++c) {
            out.key_diffuse[static_cast<std::size_t>(c)] +=
                p.key_diffuse[static_cast<std::size_t>(c)] * k;
            out.fill_diffuse[static_cast<std::size_t>(c)] +=
                p.fill_diffuse[static_cast<std::size_t>(c)] * k;
            out.rim_diffuse[static_cast<std::size_t>(c)] +=
                p.rim_diffuse[static_cast<std::size_t>(c)] * k;
            out.ambient[static_cast<std::size_t>(c)] +=
                p.ambient[static_cast<std::size_t>(c)] * k;
        }
        for (int c = 0; c < 3; ++c) {
            out.bg[static_cast<std::size_t>(c)] +=
                p.bg[static_cast<std::size_t>(c)] * k;
        }
        out.fog_start += p.fog_start * k;
        out.fog_end += p.fog_end * k;
        out.shadow_alpha += p.shadow_alpha * k;
    }
    return out;
}

void LoadTexOrCheckerboard(Texture& tex, const std::filesystem::path& path) {
    if (!tex.LoadFromFile(path)) {
        tex.CreateDebugCheckerboard();
    }
}

void DrawLine(Vec3 a, Vec3 b, float r, float g, float bl, float alpha) {
    glColor4f(r, g, bl, alpha);
    glBegin(GL_LINES);
    glVertex3f(a.x, a.y, a.z);
    glVertex3f(b.x, b.y, b.z);
    glEnd();
}

Vec3 RotateAroundY(Vec3 v, float radians) {
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return {v.x * c + v.z * s, v.y, -v.x * s + v.z * c};
}

float WrapDeg180(float deg) {
    deg = std::fmod(deg + 180.0f, 360.0f);
    if (deg < 0.0f) {
        deg += 360.0f;
    }
    return deg - 180.0f;
}

float NearestEquivalentYaw(float target_deg, float from_deg) {
    return from_deg + WrapDeg180(target_deg - from_deg);
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

    // Linear distance fog. Colour/start/end are re-tinted to the active zone
    // every frame (see Render); tuned so only the far floor dissolves into the
    // background while the table stays clear at normal zoom — turns the flat
    // "infinite plane" horizon into atmospheric depth.
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glHint(GL_FOG_HINT, GL_NICEST);

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
    // Crisper than before so metal/porcelain/glass catch a defined highlight
    // against the now-darker stage instead of looking like dull plastic.
    const std::array<float, 4> key_specular = {0.72f, 0.70f, 0.64f, 1.0f};
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
    auto infrared_camera = ObjLoader::Load(obj_dir / "Infrared_Camera.obj");
    auto astronaut_glove = ObjLoader::Load(obj_dir / "Astronaut_Glove.obj");
    auto crew_lock_bag = ObjLoader::Load(obj_dir / "Crew_Lock_Bag.obj");
    auto dirty_plate = ObjLoader::Load(obj_dir / "Dirty_Plate.obj");
    auto bowl_dirty = ObjLoader::Load(obj_dir / "Bowl_Dirty.obj");
    auto stew = ObjLoader::Load(obj_dir / "Stew.obj");
    auto cutting_board = ObjLoader::Load(obj_dir / "Cutting_Board.obj");
    auto chopsticks = ObjLoader::Load(obj_dir / "Chopsticks.obj");
    auto present = ObjLoader::Load(obj_dir / "Present.obj");
    auto wall_corkboard = ObjLoader::Load(obj_dir / "Wall_Corkboard.obj");
    auto mug_office_tool =
        ObjLoader::Load(obj_dir / "Mug_With_Office_Tool.obj");
    auto closed_umbrella = ObjLoader::Load(obj_dir / "Closed_Umbrella.obj");
    auto zz_plant = ObjLoader::Load(obj_dir / "Zz_Plant.obj");
    auto cyberpunk_platform =
        ObjLoader::Load(obj_dir / "Cyberpunk_Platform.obj");
    auto sci_fi_floor_tile = ObjLoader::Load(obj_dir / "Sci_Fi_Floor_Tile.obj");
    auto sci_fi_wall_3 = ObjLoader::Load(obj_dir / "Sci_Fi_Wall_3.obj");
    auto curtains_double = ObjLoader::Load(obj_dir / "Curtains_Double.obj");
    auto lamp_with_shade = ObjLoader::Load(obj_dir / "Lamp_With_Shade.obj");
    auto candles = ObjLoader::Load(obj_dir / "Candles.obj");

    builders::StoryPropAssets story_assets{
        .infrared_camera = std::move(infrared_camera),
        .astronaut_glove = std::move(astronaut_glove),
        .crew_lock_bag = std::move(crew_lock_bag),
        .dirty_plate = std::move(dirty_plate),
        .bowl_dirty = std::move(bowl_dirty),
        .stew = std::move(stew),
        .cutting_board = std::move(cutting_board),
        .chopsticks = std::move(chopsticks),
        .present = std::move(present),
        .wall_corkboard = std::move(wall_corkboard),
        .mug_office_tool = std::move(mug_office_tool),
        .closed_umbrella = std::move(closed_umbrella),
        .zz_plant = std::move(zz_plant),
        .cyberpunk_platform = std::move(cyberpunk_platform),
        .sci_fi_floor_tile = std::move(sci_fi_floor_tile),
        .sci_fi_wall_3 = std::move(sci_fi_wall_3),
        .curtains_double = std::move(curtains_double),
        .lamp_with_shade = std::move(lamp_with_shade),
        .candles = std::move(candles),
    };

    // Build the full scene graph.
    scene_root_ = std::make_unique<SceneGraph>("root");
    SceneNode& root = scene_root_->Root();
    camera_.SetRoomBounds(DefaultRoomBounds());
    camera_.SetProjection(
        DegToRad(kCameraFovYDeg),
        static_cast<float>(width_) / static_cast<float>(height_),
        kCameraNearPlane);
    builders::BuildDinnerTable(root, tex_set);
    builders::BuildPredictionCore(root, tex_set);
    builders::BuildAiCharacters(root, std::move(robonaut), std::move(ingenuity),
                                tex_set);
    builders::BuildStoryProps(root, std::move(story_assets));
    builders::BuildEnvironmentShell(root);

    gaze_.Bind(*scene_root_);
    animation_.Bind(*scene_root_, gaze_);

    // Roots spun together during the timeline showcase (both centred on the
    // table at the world origin, so rotating their Y turns the table in place).
    table_root_ = scene_root_->Find("dinner_scene");
    props_root_ = scene_root_->Find("story_props");

    ApplyProjection();
}

void Renderer::Resize(int width, int height) {
    width_ = std::max(1, width);
    height_ = std::max(1, height);
    glViewport(0, 0, width_, height_);
    camera_.SetProjection(
        DegToRad(kCameraFovYDeg),
        static_cast<float>(width_) / static_cast<float>(height_),
        kCameraNearPlane);
    ApplyProjection();
}

void Renderer::Update(float delta_seconds) {
    elapsed_seconds_ += delta_seconds;
    // Re-stage swing first: it moves the table roots, which the gaze/animation
    // updates below then read for shadows and prop placement.
    UpdateTableTransition(delta_seconds);
    UpdateCameraRestage(delta_seconds);
    camera_.Update(delta_seconds);
    gaze_.Update(elapsed_seconds_, delta_seconds);
    animation_.Update(delta_seconds);
}

// Orbit pose the dinner group should hold so it stays at its initial bearing in
// front of a gaze yawed by `yaw_deg`: rotate the whole group around the eye's
// vertical axis by that yaw. Both roots rest at the world origin, so the orbit
// reduces to position = Pv - Ry(yaw)*Pv (Pv = eye xz) and euler.y = yaw.
namespace
{
struct TablePose
{
    Vec3 position;
    float yaw_deg;
};

TablePose OrbitPoseForYaw(Vec3 eye_origin, float yaw_deg) {
    // Rotating the rest-at-origin group around the eye's vertical axis by
    // `yaw_deg` reduces to position = Pv - Ry(yaw)*Pv (Pv = eye xz, y dropped)
    // and euler.y = yaw, which keeps it orbiting the eye at a fixed bearing.
    const Vec3 pivot{eye_origin.x, 0.0f, eye_origin.z};
    const float r = yaw_deg * std::numbers::pi_v<float> / 180.0f;
    return TablePose{pivot - RotateAroundY(pivot, r), yaw_deg};
}
}  // namespace

void Renderer::UpdateTableTransition(float delta_seconds) {
    if (!table_transition_active_) {
        return;
    }
    table_transition_t_ += delta_seconds;
    const float u =
        std::clamp(table_transition_t_ / kTableRestageSeconds, 0.0f, 1.0f);
    const float e = ease::SmootherStep(u);
    const Vec3 pos = table_from_pos_ + (table_to_pos_ - table_from_pos_) * e;
    const float yaw = ease::Lerp(table_from_yaw_, table_to_yaw_, e);
    if (scene_root_->tf().Contains(table_root_)) {
        Transform local = scene_root_->tf().Local(table_root_);
        local.position = pos;
        local.euler_deg.y = yaw;
        scene_root_->tf().SetLocal(table_root_, local);
    }
    if (scene_root_->tf().Contains(props_root_)) {
        Transform local = scene_root_->tf().Local(props_root_);
        local.position = pos;
        local.euler_deg.y = yaw;
        scene_root_->tf().SetLocal(props_root_, local);
    }
    if (u >= 1.0f) {
        table_transition_active_ = false;
    }
}

void Renderer::StartTableRestage(float target_yaw_deg) {
    const TablePose goal = OrbitPoseForYaw(gaze_.Origin(), target_yaw_deg);
    if (scene_root_->tf().Contains(table_root_)) {
        const Transform local = scene_root_->tf().Local(table_root_);
        table_from_pos_ = local.position;
        table_from_yaw_ = local.euler_deg.y;
    }
    else {
        table_from_pos_ = {};
        table_from_yaw_ = 0.0f;
    }
    table_to_pos_ = goal.position;
    table_to_yaw_ = NearestEquivalentYaw(goal.yaw_deg, table_from_yaw_);
    table_transition_t_ = 0.0f;
    table_transition_active_ = true;
}

void Renderer::SnapTableStage(float yaw_deg) {
    const TablePose pose = OrbitPoseForYaw(gaze_.Origin(), yaw_deg);
    table_transition_active_ = false;
    if (scene_root_->tf().Contains(table_root_)) {
        Transform local = scene_root_->tf().Local(table_root_);
        local.position = pose.position;
        local.euler_deg.y = pose.yaw_deg;
        scene_root_->tf().SetLocal(table_root_, local);
    }
    if (scene_root_->tf().Contains(props_root_)) {
        Transform local = scene_root_->tf().Local(props_root_);
        local.position = pose.position;
        local.euler_deg.y = pose.yaw_deg;
        scene_root_->tf().SetLocal(props_root_, local);
    }
}

void Renderer::UpdateCameraRestage(float delta_seconds) {
    if (!camera_restage_pending_) {
        return;
    }
    camera_restage_delay_ -= delta_seconds;
    if (camera_restage_delay_ > 0.0f) {
        return;
    }
    camera_restage_pending_ = false;
    const OrbitCamera::CameraPose goal = camera_home_pending_
                                             ? camera_.HomePose()
                                             : CameraFollowPoseForRestage();
    camera_home_pending_ = false;
    camera_.StartTransitionTo(goal, kCameraTransitionSeconds);
}

OrbitCamera::CameraPose Renderer::CameraFollowPoseForRestage() const {
    // The table root moves as a rigid restage group; aim at the living volume
    // above the tabletop, not at the root/floor. Pulling the camera from the
    // eye's own gaze direction keeps all three zones using the same geometric
    // rule while still producing different views.
    const Vec3 table_focus = table_to_pos_ + Vec3{0.0f, 1.10f, 0.0f};
    return OrbitCamera::GazePose(gaze_.Origin(), gaze_.Direction(),
                                 table_focus);
}

void Renderer::RefreshZonePaletteCache() {
    const std::size_t revision = gaze_.ZoneWeightsRevision();
    if (zone_palette_cache_revision_ == revision) {
        return;
    }
    zone_palette_cache_ = BlendZonePalette(
        gaze_.ZoneWeight(0), gaze_.ZoneWeight(1), gaze_.ZoneWeight(2));
    zone_palette_cache_revision_ = revision;
}

void Renderer::Render() {
    // Blend the active gaze zone's mood from the eased zone weights. This is
    // the single source of the ~0.6s crossfade — lights, background, fog and
    // shadow weight all read from the same blended palette.
    RefreshZonePaletteCache();
    const ZonePalette& pal = zone_palette_cache_;

    glClearColor(pal.bg[0], pal.bg[1], pal.bg[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    const Mat4 view = camera_.ViewMatrix();
    glLoadMatrixf(view.Data());

    // Per-zone light colours + global ambient (specular/attenuation stay
    // fixed).
    glLightfv(GL_LIGHT0, GL_DIFFUSE, pal.key_diffuse.data());
    glLightfv(GL_LIGHT1, GL_DIFFUSE, pal.fill_diffuse.data());
    glLightfv(GL_LIGHT2, GL_DIFFUSE, pal.rim_diffuse.data());
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, pal.ambient.data());

    // Fog tinted to match the background so the far floor dissolves seamlessly.
    const std::array<float, 4> fog_color = {pal.bg[0], pal.bg[1], pal.bg[2],
                                            1.0f};
    glFogfv(GL_FOG_COLOR, fog_color.data());
    glFogf(GL_FOG_START, pal.fog_start);
    glFogf(GL_FOG_END, pal.fog_end);

    ApplyLighting();  // light positions — must run after the view matrix loads

    DrawGroundReceiver();

    if (scene_root_) {
        scene_root_->Draw();
        DrawPlanarShadowOnTable(pal.shadow_alpha);
        DrawPlanarShadowOnGround(pal.shadow_alpha);
        DrawGazeDebug();
    }

#ifdef FUTURE_GAZE_OBSERVER_TOOLS
    observer::RecordFramebuffer(width_, height_, elapsed_seconds_);
#endif
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
    SnapTableStage(0.0f);
    camera_restage_pending_ = false;
    camera_home_pending_ = false;
}

void Renderer::ToggleGazeControlMode() {
    SetGazeControlMode(!gaze_.control_mode());
}

void Renderer::SetGazeControlMode(bool enabled) {
    if (gaze_.control_mode() == enabled) {
        return;
    }

    gaze_.SetControlMode(enabled);

    if (enabled) {
        // Entering gaze-drag: snapshot the camera wherever it currently is and
        // cancel any in-flight restage. Every entry is treated as a fresh start
        // regardless of how many G presses have happened before.
        camera_restage_pending_ = false;
        camera_home_pending_ = false;
    }
    else {
        // Exiting gaze-drag: swing the dinner group to the new gaze bearing.
        // A completed 360-degree cycle homes the whole presentation instead of
        // preserving the accumulated yaw (e.g. 366°) as a new side view.
        const bool completed_cycle_to_foresight =
            gaze_.ActiveZone() == GazeZone::kForesight &&
            std::abs(gaze_.YawOffsetDeg()) >= 300.0f;
        if (completed_cycle_to_foresight) {
            StartTableRestage(0.0f);
            gaze_.ResetAim();
            camera_home_pending_ = true;
        }
        else {
            StartTableRestage(gaze_.YawOffsetDeg());
            camera_home_pending_ = false;
        }
        camera_restage_pending_ = true;
        camera_restage_delay_ = kCameraRestageDelay;
    }
}

void Renderer::ToggleGazeDebug() {
    gaze_.ToggleDebug();
}

void Renderer::ResetGazeAim() {
    gaze_.ResetAim();
    StartTableRestage(0.0f);
    camera_home_pending_ = true;
    // If not in gaze-drag mode, re-arm the camera to return to the saved pose
    // so it follows the table swinging back to origin.
    if (!gaze_.control_mode()) {
        camera_restage_pending_ = true;
        camera_restage_delay_ = kCameraRestageDelay;
    }
}

bool Renderer::GazeControlMode() const {
    return gaze_.control_mode();
}

void Renderer::BeginGazeDrag(int x, int y) {
    gaze_.BeginDrag(x, y);
}

void Renderer::DragGazeTo(int x, int y) {
    gaze_.DragTo(x, y);
}

void Renderer::EndGazeDrag() {
    gaze_.EndDrag();
}

void Renderer::LogObservationState(const char* label) const {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3)
        << "[observe] t=" << elapsed_seconds_
        << " label=" << ((label != nullptr) ? label : "")
        << " gaze_mode=" << (gaze_.control_mode() ? 1 : 0)
        << std::setprecision(2) << " yaw=" << gaze_.YawOffsetDeg()
        << " pitch=" << gaze_.PitchOffsetDeg()
        << " zone=" << static_cast<int>(gaze_.ActiveZone())
        << std::setprecision(3) << " weights=(" << gaze_.ZoneWeight(0) << ','
        << gaze_.ZoneWeight(1) << ',' << gaze_.ZoneWeight(2) << ')'
        << " table_transition=" << (table_transition_active_ ? 1 : 0)
        << " camera_transition=" << (camera_.transitioning() ? 1 : 0)
        << " camera_restage_pending=" << (camera_restage_pending_ ? 1 : 0)
        << '\n';
    std::cerr << out.str();
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
    const Mat4 proj = Mat4::Perspective(DegToRad(kCameraFovYDeg), aspect,
                                        kCameraNearPlane, 50.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(proj.Data());
}

void Renderer::DrawGroundReceiver() const {
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_TEXTURE_2D);
    ground_material_.Apply();
    // Large enough to keep the dinner group on the floor at any gaze yaw: the
    // table orbits up to 1.5 units from the eye pivot and extends ~2.2 m wide,
    // so the worst-case excursion is ~3.7 m from the pivot in any direction.
    DrawReceiverBox({0.0f, -0.020f, 0.0f}, {12.0f, 0.020f, 12.0f});
    glPopAttrib();
}

void Renderer::DrawPlanarShadowOnGround(float shadow_alpha) {
    if (scene_root_ == nullptr) {
        return;
    }
    DrawShadowPass(MakeShadowMatrix(kKeyLightPos, kGroundPlane),
                   {0.0f, -0.020f, 0.0f}, {12.0f, 0.020f, 12.0f},
                   scene_root_->Root().handle(),
                   -std::numeric_limits<float>::infinity(),
                   std::numeric_limits<float>::infinity(), shadow_alpha);
}

void Renderer::DrawPlanarShadowOnTable(float shadow_alpha) {
    if (scene_root_ == nullptr || !table_root_.IsValid()) {
        return;
    }
    DrawShadowPass(MakeShadowMatrix(kKeyLightPos, kTablePlane),
                   {0.0f, kTableTopY + 0.001f, 0.0f}, {2.10f, 0.001f, 0.95f},
                   table_root_, kTableTopY + 0.001f, 1.80f, shadow_alpha);
}

void Renderer::DrawGazeDebug() const {
    if (!gaze_.debug_enabled()) {
        return;
    }

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT |
                 GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.0f);
    glDepthMask(GL_FALSE);

    const Vec3 origin = gaze_.Origin();
    const Vec3 dir = gaze_.Direction();
    const Vec3 blind = gaze_.BlindDirection();
    const Vec3 forward_end = origin + dir * 4.2f;
    const Vec3 blind_end = origin + blind * 2.4f;

    DrawLine(origin, forward_end, 0.18f, 0.95f, 1.0f, 0.82f);
    DrawLine(origin, blind_end, 0.78f, 0.80f, 0.74f, 0.62f);

    const Vec3 side = Normalize(Cross(dir, {0.0f, 1.0f, 0.0f}));
    const Vec3 stable_side =
        (Length(side) < 0.001f) ? Vec3{1.0f, 0.0f, 0.0f} : side;
    const Vec3 up = Normalize(Cross(stable_side, dir));
    constexpr float kConeLength = 3.6f;
    constexpr float kConeAngle = std::numbers::pi_v<float> / 7.0f;
    const float radius = std::tan(kConeAngle) * kConeLength;
    const Vec3 center = origin + dir * kConeLength;

    constexpr int kSegments = 20;
    Vec3 prev = center + stable_side * radius;
    for (int i = 1; i <= kSegments; ++i) {
        const float a = std::numbers::pi_v<float> * 2.0f *
                        static_cast<float>(i) / static_cast<float>(kSegments);
        const Vec3 rim = center + stable_side * (std::cos(a) * radius) +
                         up * (std::sin(a) * radius);
        DrawLine(prev, rim, 0.18f, 0.95f, 1.0f, 0.30f);
        if (i % 5 == 0) {
            DrawLine(origin, rim, 0.18f, 0.95f, 1.0f, 0.24f);
        }
        prev = rim;
    }

    for (const float sign : {0.34f, -0.34f}) {
        DrawLine(origin, origin + RotateAroundY(blind, sign) * 2.1f, 0.78f,
                 0.80f, 0.74f, 0.28f);
    }

    glPopAttrib();
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
                              TfHandle caster_root, float min_caster_y,
                              float max_caster_y, float shadow_alpha) const {
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
    glColor4f(0.0f, 0.0f, 0.0f, shadow_alpha);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixf(shadow_matrix.data());
    scene_root_->DrawShadow(caster_root, min_caster_y, max_caster_y);
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
