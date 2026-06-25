#pragma once

#include <optional>

#include "future_gaze/glut_window.hpp"
#include "future_gaze/render/renderer.hpp"

namespace future_gaze
{

class App
{
public:
    App(int* argc, char** argv);

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void Run();

private:
    Renderer renderer_;
    std::optional<GlutWindow> window_;
};

}  // namespace future_gaze
