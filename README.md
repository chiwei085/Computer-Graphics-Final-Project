# Future's Gaze — 未來的視線

An interactive 3D scene built entirely on the OpenGL fixed pipeline. 

## Requirements

| Dependency   | Minimum version | Notes                                      |
| ------------ | --------------- | ------------------------------------------ |
| C++ compiler | C++20           | Tested with **clang++ 17+**                |
| CMake        | 3.24            |                                            |
| Ninja        | any             |                                            |
| OpenGL       | any legacy      | Must support `GL_LIGHTING`, stencil buffer |
| FreeGLUT     | 3.x             | `libglut-dev` on Debian/Ubuntu             |

On **Ubuntu / Debian**:

```bash
sudo apt install clang ninja-build cmake libgl-dev freeglut3-dev
```

On **Arch / Manjaro**:

```bash
sudo pacman -S clang ninja cmake mesa freeglut
```

On **macOS (Homebrew)** — OpenGL is deprecated but still functional:

```bash
brew install ninja cmake
# Xcode Command Line Tools supply clang and OpenGL/GLUT
```

---

## Building (Release)

```bash
# Configure
cmake --preset release

# Compile
cmake --build --preset release
```

The binary is written to `build/release/future_gaze`.

To also run the bundled test suite after building:

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

---

## Running

```bash
./build/release/future_gaze
```

The window opens at **1280 × 720** and is freely resizable.

---

## Controls

### Camera

| Input               | Action                           |
| ------------------- | -------------------------------- |
| Left-drag           | Orbit camera around the scene    |
| Middle / Right-drag | Pan camera                       |
| Scroll wheel        | Zoom in / out                    |
| `R`                 | Reset camera to default position |

### Gaze

| Input                      | Action                                                                          |
| -------------------------- | ------------------------------------------------------------------------------- |
| `G`                        | Toggle **gaze-control mode** (left-drag now steers the eye instead of orbiting) |
| Left-drag *(in gaze mode)* | Rotate the Prediction Core's line of sight                                      |
| `H`                        | Return gaze to its resting direction                                            |
| `V`                        | Toggle gaze-debug overlay (visualises zone boundaries and weights)              |

### Other

| Input | Action |
| ----- | ------ |
| `Esc` | Exit   |

---

## Architecture

```
src/
  main.cpp                   — entry point, window creation
  glut_window.cpp            — GLUT callback dispatch and input routing
  render/renderer.cpp        — frame loop, lighting, planar shadows
  scene/
    prediction_core.cpp      — procedural layered eye model
    gaze_controller.cpp      — zone detection and eased weights
    dinner_table.cpp         — table, chairs, story objects
    ai_characters.cpp        — Robonaut 2 service robot, Ingenuity drone
    story_props.cpp          — foresight / longing / blindspot prop sets
    scene_animation.cpp      — self-rotation, orbit, custom animations
  camera/orbit_camera.cpp    — arc-ball orbit with zoom and pan
  math/                      — mat4, quat, vec3 (no GLM dependency)
  physics/physics_world.cpp  — planar shadow matrix computation

include/future_gaze/         — public headers mirroring src/
assets/
  textures/                  — six 512×512 procedural PNGs
  obj/                       — normalised OBJ models
```

The renderer uses only the **OpenGL fixed pipeline** — no GLSL shaders,
no VAOs, no FBOs. Shadows are cast via planar shadow projection onto
the floor with stencil clipping.

---

## Asset Credits

All external 3D models are public-domain or CC0. Full attribution is in
[assets/CREDITS.md](assets/CREDITS.md). Key models:

- **Robonaut 2** — NASA (public domain) — humanoid service robot
- **Ingenuity Mars Helicopter** — NASA (public domain) — orbit drone
- Various tableware and props — Poly Pizza / Kay Lousberg / CreativeTrio (CC0)

Textures (wood, cloth, circuit, marble, metal, paper) are procedurally
generated and released as CC0.
