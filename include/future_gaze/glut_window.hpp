#pragma once

#include <cstdint>

#include "future_gaze/render/renderer.hpp"

namespace future_gaze
{

class GlutWindow
{
public:
    GlutWindow(int width, int height, const char* title, Renderer& renderer);

    GlutWindow(const GlutWindow&) = delete;
    GlutWindow& operator=(const GlutWindow&) = delete;

    void Run();

private:
    static void DisplayCallback();
    static void IdleCallback();
    static void KeyboardCallback(unsigned char key, int x, int y);
    static void MouseCallback(int button, int state, int x, int y);
    static void MotionCallback(int x, int y);
    static void ReshapeCallback(int width, int height);

    enum class DragMode : uint8_t { None, Orbit, Pan };

    inline static GlutWindow* active_window_ = nullptr;

    Renderer& renderer_;
    int last_time_ms_ = 0;
    DragMode drag_mode_ = DragMode::None;
};

}  // namespace future_gaze
