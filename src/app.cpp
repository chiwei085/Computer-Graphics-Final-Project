#include "future_gaze/app.hpp"

#include <GL/freeglut.h>

namespace future_gaze
{

App::App(int* argc, char** argv) {
    glutInit(argc, argv);
    window_.emplace(1280, 720, "Future's Gaze", renderer_);
}

void App::Run() {
    renderer_.Initialize();
    window_->Run();
}

}  // namespace future_gaze
