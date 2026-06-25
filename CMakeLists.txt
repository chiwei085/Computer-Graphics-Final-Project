cmake_minimum_required(VERSION 3.24)

project(
    FutureGaze
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

add_executable(future_gaze
    src/app.cpp
    src/camera/orbit_camera.cpp
    src/glut_window.cpp
    src/main.cpp
    src/math/mat4.cpp
    src/math/quat.cpp
    src/render/material.cpp
    src/render/mesh.cpp
    src/render/obj_loader.cpp
    src/render/renderer.cpp
    src/render/texture.cpp
)

target_include_directories(future_gaze
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(future_gaze
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

target_link_libraries(future_gaze
    PRIVATE
        OpenGL::GL
        GLUT::GLUT
)
