#include <GL/freeglut.h>

#include "future_gaze/glut_window.hpp"
#include "future_gaze/render/renderer.hpp"

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    future_gaze::Renderer renderer;
    future_gaze::GlutWindow window(1280, 720, "Future's Gaze", renderer);
    renderer.Initialize();
    window.Run();
    return 0;
}
