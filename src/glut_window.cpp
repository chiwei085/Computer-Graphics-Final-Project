#include "future_gaze/glut_window.hpp"

#include <GL/freeglut.h>

#include <cstdlib>

namespace future_gaze
{

GlutWindow::GlutWindow(int width, int height, const char* title,
                       Renderer& renderer)
    : renderer_(renderer) {
    active_window_ = this;
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
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
}

void GlutWindow::MouseCallback(int button, int state, int x, int y) {
    if (active_window_ == nullptr) {
        return;
    }

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        active_window_->renderer_.BeginCameraDrag(x, y);
        return;
    }
    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        active_window_->renderer_.EndCameraDrag();
        return;
    }
    if ((button == 3 || button == 4) && state == GLUT_DOWN) {
        active_window_->renderer_.ZoomCamera(button == 3 ? 1.0f : -1.0f);
    }
}

void GlutWindow::MotionCallback(int x, int y) {
    if (active_window_ == nullptr) {
        return;
    }
    active_window_->renderer_.DragCameraTo(x, y);
}

void GlutWindow::ReshapeCallback(int width, int height) {
    if (active_window_ == nullptr) {
        return;
    }
    active_window_->renderer_.Resize(width, height);
}

}  // namespace future_gaze
