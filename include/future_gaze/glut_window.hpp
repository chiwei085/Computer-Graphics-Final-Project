#pragma once

#include <cstdint>
#include <string>
#include <vector>

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
    struct AutomationCommand
    {
        float at_seconds = 0.0f;
        std::string op;
        std::vector<std::string> args;
        bool done = false;
    };

    static void DisplayCallback();
    static void IdleCallback();
    static void KeyboardCallback(unsigned char key, int x, int y);
    static void MouseCallback(int button, int state, int x, int y);
    static void MotionCallback(int x, int y);
    static void ReshapeCallback(int width, int height);

    void LoadAutomation();
    void UpdateAutomation(float delta_seconds);
    void RunAutomationCommand(const AutomationCommand& command);

    enum class DragMode : uint8_t
    {
        None,
        Orbit,
        Pan,
        Gaze
    };

    inline static GlutWindow* active_window_ = nullptr;

    Renderer& renderer_;
    int last_time_ms_ = 0;
    bool first_idle_ = true;
    DragMode drag_mode_ = DragMode::None;
    std::vector<AutomationCommand> automation_;
    float automation_elapsed_ = 0.0f;
    float automation_log_interval_ = 0.0f;
    float automation_next_log_ = 0.0f;
};

}  // namespace future_gaze
