#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <vector>

#include "future_gaze/scene/builders.hpp"
#include "future_gaze/scene/scene_graph.hpp"

namespace
{

void FailScene(std::string_view message) {
    std::fprintf(stderr, "scene_invariant_tests: %.*s\n",
                 static_cast<int>(message.size()), message.data());
    std::exit(1);
}

void RequireScene(bool value, std::string_view message) {
    if (!value) {
        FailScene(message);
    }
}

void RequireNearScene(future_gaze::Vec3 actual, future_gaze::Vec3 expected,
                      std::string_view message) {
    if (!future_gaze::NearlyEqual(actual, expected, 0.0001f)) {
        std::fprintf(stderr,
                     "scene_invariant_tests: %.*s "
                     "(actual %.6f %.6f %.6f expected %.6f %.6f %.6f)\n",
                     static_cast<int>(message.size()), message.data(), actual.x,
                     actual.y, actual.z, expected.x, expected.y, expected.z);
        std::exit(1);
    }
}

void TestEnvironmentShellIsSeparateAndFixed() {
    future_gaze::SceneGraph graph("root");
    future_gaze::builders::BuildStoryProps(
        graph.Root(), future_gaze::builders::StoryPropAssets{});
    future_gaze::builders::BuildEnvironmentShell(graph.Root());

    const future_gaze::TfHandle story = graph.Find("story_props");
    const future_gaze::TfHandle environment = graph.Find("environment_shell");
    RequireScene(story.IsValid(), "story_props exists");
    RequireScene(environment.IsValid(), "environment_shell exists");

    std::vector<future_gaze::TfHandle> future_backdrop;
    std::vector<future_gaze::TfHandle> memory_backdrop;
    std::vector<future_gaze::TfHandle> blind_backdrop;
    graph.Collect("fgbg_", future_backdrop);
    graph.Collect("mgbg_", memory_backdrop);
    graph.Collect("blind_bg_", blind_backdrop);
    RequireScene(!future_backdrop.empty(),
                 "future backdrop remains collectable");
    RequireScene(!memory_backdrop.empty(),
                 "memory backdrop remains collectable");
    RequireScene(!blind_backdrop.empty(), "blind backdrop remains collectable");

    future_gaze::Transform local = graph.tf().Local(story);
    local.position = {4.0f, 0.0f, -2.0f};
    local.euler_deg.y = 120.0f;
    graph.tf().SetLocal(story, local);

    RequireNearScene(graph.tf().Local(environment).position, {},
                     "environment shell position is fixed");
    RequireNearScene(graph.tf().Local(environment).euler_deg, {},
                     "environment shell rotation is fixed");
}

}  // namespace

void RunSceneInvariantTests() {
    TestEnvironmentShellIsSeparateAndFixed();
}
