#include "future_gaze/glut_window.hpp"

#include <GL/freeglut.h>

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>
#include <utility>

#ifdef FUTURE_GAZE_OBSERVER_TOOLS
#include "future_gaze/observer/frame_stream.hpp"
#endif

namespace future_gaze
{

namespace
{

float ParseFloatPrefix(std::string_view text, float fallback) {
    float parsed = fallback;
    const auto result =
        std::from_chars(text.data(), text.data() + text.size(), parsed);
    return (result.ptr != text.data()) ? parsed : fallback;
}

}  // namespace

GlutWindow::GlutWindow(int width, int height, const char* title,
                       Renderer& renderer)
    : renderer_(renderer) {
    active_window_ = this;
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowSize(width, height);
    glutCreateWindow(title);
    glutDisplayFunc(DisplayCallback);
    glutIdleFunc(IdleCallback);
    glutKeyboardFunc(KeyboardCallback);
    glutMouseFunc(MouseCallback);
    glutMotionFunc(MotionCallback);
    glutReshapeFunc(ReshapeCallback);
    last_time_ms_ = glutGet(GLUT_ELAPSED_TIME);
    LoadAutomation();
}

void GlutWindow::Run() {
    glutMainLoop();
}

void GlutWindow::DisplayCallback() {
    if (active_window_ == nullptr) {
        return;
    }
    active_window_->renderer_.Render();
}

void GlutWindow::IdleCallback() {
    if (active_window_ == nullptr) {
        return;
    }

    const int current_time_ms = glutGet(GLUT_ELAPSED_TIME);
    float delta_seconds =
        static_cast<float>(current_time_ms - active_window_->last_time_ms_) /
        1000.0f;
    active_window_->last_time_ms_ = current_time_ms;
    if (active_window_->first_idle_) {
        delta_seconds = 0.0f;
        active_window_->first_idle_ = false;
    }
#ifdef FUTURE_GAZE_OBSERVER_TOOLS
    delta_seconds = observer::ObserverDeltaSeconds(delta_seconds);
#endif

    active_window_->renderer_.Update(delta_seconds);
    active_window_->UpdateAutomation(delta_seconds);
    glutPostRedisplay();
}

void GlutWindow::LoadAutomation() {
    if (const char* every = std::getenv("FUTURE_GAZE_LOG_EVERY")) {
        automation_log_interval_ =
            std::max(0.0f, ParseFloatPrefix(every, 0.0f));
        automation_next_log_ = 0.0f;
    }

    const char* path = std::getenv("FUTURE_GAZE_AUTOMATION");
    if (path == nullptr || path[0] == '\0') {
        return;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[automation] cannot open '" << path << "'\n";
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        const std::size_t comment = line.find('#');
        if (comment != std::string::npos) {
            line.erase(comment);
        }

        std::istringstream in(line);
        AutomationCommand command;
        if (!(in >> command.at_seconds >> command.op)) {
            continue;
        }
        std::string arg;
        while (in >> arg) {
            command.args.push_back(arg);
        }
        automation_.push_back(std::move(command));
    }

    std::sort(automation_.begin(), automation_.end(),
              [](const AutomationCommand& a, const AutomationCommand& b) {
                  return a.at_seconds < b.at_seconds;
              });
    std::cerr << "[automation] loaded " << automation_.size()
              << " commands from '" << path << "'\n";
}

void GlutWindow::UpdateAutomation(float delta_seconds) {
    automation_elapsed_ += delta_seconds;

    if (automation_log_interval_ > 0.0f &&
        automation_elapsed_ >= automation_next_log_) {
        renderer_.LogObservationState("tick");
        automation_next_log_ += automation_log_interval_;
    }

    for (AutomationCommand& command : automation_) {
        if (command.at_seconds > automation_elapsed_) {
            break;  // list is sorted; nothing further can fire yet
        }
        if (!command.done) {
            command.done = true;
            RunAutomationCommand(command);
        }
    }
}

void GlutWindow::RunAutomationCommand(const AutomationCommand& command) {
    auto arg_float = [&](std::size_t index, float fallback) {
        if (index >= command.args.size()) {
            return fallback;
        }
        return ParseFloatPrefix(command.args[index], fallback);
    };

    if (command.op == "log") {
        const char* label =
            command.args.empty() ? "script" : command.args.front().c_str();
        renderer_.LogObservationState(label);
        return;
    }
    if (command.op == "gaze_mode") {
        const bool enabled =
            !command.args.empty() &&
            (command.args.front() == "on" || command.args.front() == "1" ||
             command.args.front() == "true");
        renderer_.SetGazeControlMode(enabled);
        renderer_.LogObservationState(enabled ? "gaze_mode_on"
                                              : "gaze_mode_off");
        return;
    }
    if (command.op == "reset_gaze") {
        renderer_.ResetGazeAim();
        renderer_.LogObservationState("reset_gaze");
        return;
    }
    if (command.op == "gaze_drag") {
        const int dx = static_cast<int>(arg_float(0, 0.0f));
        const int dy = static_cast<int>(arg_float(1, 0.0f));
        const int cx = glutGet(GLUT_WINDOW_WIDTH) / 2;
        const int cy = glutGet(GLUT_WINDOW_HEIGHT) / 2;
        renderer_.SetGazeControlMode(true);
        renderer_.BeginGazeDrag(cx, cy);
        renderer_.DragGazeTo(cx + dx, cy + dy);
        renderer_.EndGazeDrag();
        renderer_.LogObservationState("gaze_drag");
        return;
    }
    if (command.op == "orbit_drag") {
        const int dx = static_cast<int>(arg_float(0, 0.0f));
        const int dy = static_cast<int>(arg_float(1, 0.0f));
        const int cx = glutGet(GLUT_WINDOW_WIDTH) / 2;
        const int cy = glutGet(GLUT_WINDOW_HEIGHT) / 2;
        renderer_.BeginCameraDrag(cx, cy);
        renderer_.DragCameraTo(cx + dx, cy + dy);
        renderer_.EndCameraDrag();
        renderer_.LogObservationState("orbit_drag");
        return;
    }
    if (command.op == "zoom") {
        renderer_.ZoomCamera(arg_float(0, 0.0f));
        renderer_.LogObservationState("zoom");
        return;
    }
    if (command.op == "toggle_debug") {
        renderer_.ToggleGazeDebug();
        renderer_.LogObservationState("toggle_debug");
        return;
    }
    if (command.op == "quit") {
        std::cerr << "[automation] quit at " << automation_elapsed_ << "s\n";
        std::exit(EXIT_SUCCESS);
    }

    std::cerr << "[automation] unknown command '" << command.op << "' at "
              << command.at_seconds << "s\n";
}

void GlutWindow::KeyboardCallback(unsigned char key, int, int) {
    if (key == 27) {
        std::exit(EXIT_SUCCESS);
    }
    if (key == 'r' || key == 'R') {
        active_window_->renderer_.ResetCamera();
    }
    if (key == 'g' || key == 'G') {
        active_window_->drag_mode_ = DragMode::None;
        active_window_->renderer_.ToggleGazeControlMode();
    }
    if (key == 'h' || key == 'H') {
        active_window_->drag_mode_ = DragMode::None;
        active_window_->renderer_.ResetGazeAim();
    }
    if (key == 'v' || key == 'V') {
        active_window_->renderer_.ToggleGazeDebug();
    }
}

void GlutWindow::MouseCallback(int button, int state, int x, int y) {
    if (active_window_ == nullptr) {
        return;
    }

    // Orbit: left button
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            if (active_window_->renderer_.GazeControlMode()) {
                active_window_->drag_mode_ = DragMode::Gaze;
                active_window_->renderer_.BeginGazeDrag(x, y);
            }
            else {
                active_window_->drag_mode_ = DragMode::Orbit;
                active_window_->renderer_.BeginCameraDrag(x, y);
            }
        }
        else {
            if (active_window_->drag_mode_ == DragMode::Gaze) {
                active_window_->renderer_.EndGazeDrag();
            }
            else {
                active_window_->renderer_.EndCameraDrag();
            }
            active_window_->drag_mode_ = DragMode::None;
        }
        return;
    }

    // Pan: middle button or right button
    if (button == GLUT_MIDDLE_BUTTON || button == GLUT_RIGHT_BUTTON) {
        if (state == GLUT_DOWN) {
            active_window_->drag_mode_ = DragMode::Pan;
            active_window_->renderer_.BeginCameraPan(x, y);
        }
        else {
            active_window_->drag_mode_ = DragMode::None;
            active_window_->renderer_.EndCameraPan();
        }
        return;
    }

    // Scroll wheel (FreeGLUT reports as buttons 3/4)
    if ((button == 3 || button == 4) && state == GLUT_DOWN) {
        active_window_->renderer_.ZoomCamera(button == 3 ? 1.0f : -1.0f);
    }
}

void GlutWindow::MotionCallback(int x, int y) {
    if (active_window_ == nullptr) {
        return;
    }
    switch (active_window_->drag_mode_) {
        case DragMode::Orbit:
            active_window_->renderer_.DragCameraTo(x, y);
            break;
        case DragMode::Pan:
            active_window_->renderer_.PanCameraTo(x, y);
            break;
        case DragMode::Gaze:
            active_window_->renderer_.DragGazeTo(x, y);
            break;
        case DragMode::None:
            break;
    }
}

void GlutWindow::ReshapeCallback(int width, int height) {
    if (active_window_ == nullptr) {
        return;
    }
    active_window_->renderer_.Resize(width, height);
}

}  // namespace future_gaze
