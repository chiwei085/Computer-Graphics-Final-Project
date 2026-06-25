#include "future_gaze/glut_window.hpp"

#include <GL/freeglut.h>

#include <cstdlib>

namespace future_gaze
{

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
    const float delta_seconds =
        static_cast<float>(current_time_ms - active_window_->last_time_ms_) /
        1000.0f;
    active_window_->last_time_ms_ = current_time_ms;

    active_window_->renderer_.Update(delta_seconds);
    glutPostRedisplay();
}

void GlutWindow::KeyboardCallback(unsigned char key, int, int) {
    if (key == 27) {
        std::exit(EXIT_SUCCESS);
    }
    if (key == 'r' || key == 'R') {
        active_window_->renderer_.ResetCamera();
    }
}

void GlutWindow::MouseCallback(int button, int state, int x, int y) {
    if (active_window_ == nullptr) {
        return;
    }

    // Orbit: left button
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            active_window_->drag_mode_ = DragMode::Orbit;
            active_window_->renderer_.BeginCameraDrag(x, y);
        }
        else {
            active_window_->drag_mode_ = DragMode::None;
            active_window_->renderer_.EndCameraDrag();
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
