cmake_minimum_required(VERSION 3.24)

project(
    FutureGaze
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FUTURE_GAZE_OBSERVER_TOOLS "Build deterministic observer-only recording tools" OFF)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

add_executable(future_gaze
    src/camera/orbit_camera.cpp
    src/glut_window.cpp
    src/main.cpp
    src/math/mat4.cpp
    src/math/quat.cpp
    src/physics/physics_world.cpp
    src/render/material.cpp
    src/render/mesh.cpp
    src/render/obj_loader.cpp
    src/render/renderer.cpp
    src/render/texture.cpp
    src/scene/scene_graph.cpp
    src/scene/tf_tree.cpp
    src/scene/scene_animation.cpp
    src/scene/dinner_table.cpp
    src/scene/gaze_controller.cpp
    src/scene/story_props.cpp
    src/scene/prediction_core.cpp
    src/scene/ai_characters.cpp
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

enable_testing()

add_executable(future_gaze_tests
    tests/camera_tests.cpp
    tests/geometry_tests.cpp
    tests/physics_tests.cpp
    tests/scene_invariant_tests.cpp
    tests/tf_tree_tests.cpp
    src/camera/orbit_camera.cpp
    src/render/material.cpp
    src/render/mesh.cpp
    src/render/texture.cpp
    src/math/mat4.cpp
    src/math/quat.cpp
    src/physics/physics_world.cpp
    src/scene/scene_graph.cpp
    src/scene/story_props.cpp
    src/scene/tf_tree.cpp
)

target_include_directories(future_gaze_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(future_gaze_tests
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

target_link_libraries(future_gaze_tests
    PRIVATE
        OpenGL::GL
        GLUT::GLUT
)

add_test(NAME future_gaze_tests COMMAND future_gaze_tests)

if(FUTURE_GAZE_OBSERVER_TOOLS)
    target_sources(future_gaze
        PRIVATE
            src/observer/frame_recorder.cpp
    )

    target_compile_definitions(future_gaze
        PRIVATE
            FUTURE_GAZE_OBSERVER_TOOLS=1
    )

    add_executable(future_gaze_frame_bridge
        src/observer/frame_bridge.cpp
    )

    target_include_directories(future_gaze_frame_bridge
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_compile_options(future_gaze_frame_bridge
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()
