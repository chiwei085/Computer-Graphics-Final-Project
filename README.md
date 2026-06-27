# Future's Gaze — 未來的視線

An interactive 3D scene built entirely on the OpenGL fixed pipeline. 

## Requirements

| Dependency   | Minimum version | Notes                                           |
| ------------ | --------------- | ----------------------------------------------- |
| C++ compiler | C++20           | clang 17+, GCC 13+, Apple Clang 15+, MSVC 19.38+ |
| CMake        | 3.24            |                                                 |
| Ninja        | any             | cross-platform; required by all presets         |
| OpenGL       | any legacy      | Must support `GL_LIGHTING`, stencil buffer      |
| FreeGLUT     | 3.x             | provides `<GL/freeglut.h>`                      |

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
# Xcode Command Line Tools supply Apple Clang and OpenGL.
# freeglut must come from Homebrew — the system GLUT.framework
# does not provide <GL/freeglut.h>.
brew install ninja cmake freeglut
```

On **Windows**:

*Option A — MSYS2 (MinGW-w64 or clang64, simplest):*

```bash
# In an MSYS2 shell for your target environment, e.g. clang64:
pacman -S mingw-w64-clang-x86_64-clang \
          mingw-w64-clang-x86_64-cmake \
          mingw-w64-clang-x86_64-ninja \
          mingw-w64-clang-x86_64-freeglut
```

Uses the `release` preset (Ninja available in MSYS2).

*Option B — Visual Studio 2019/2022 + vcpkg (no Ninja required):*

```powershell
# Install freeglut via vcpkg (once):
vcpkg install freeglut:x64-windows

# Open the repository folder in Visual Studio — it reads CMakePresets.json
# automatically and shows the "Release (Visual Studio / no Ninja)" preset.
#
# Or build from a Developer PowerShell:
cmake --preset release-vs -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build --preset release-vs
```

The `release-vs` preset omits the `generator` field so Visual Studio picks its
own (MSBuild). Ninja is not required.

---

## Building (Release)

The `release` preset uses whichever C++20 compiler CMake finds first, so the
same commands work on Linux (GCC or clang), macOS (Apple Clang), and Windows
(MSVC or MinGW).

```bash
# Configure — uses the system default compiler
cmake --preset release

# Compile
cmake --build --preset release
```

To pin clang++ explicitly (e.g. on a system where GCC is the default):

```bash
cmake --preset release-clang
cmake --build --preset release-clang
```

For **Visual Studio on Windows** (Ninja not required):

```powershell
cmake --preset release-vs
cmake --build --preset release-vs
```

The binary is written to `build/release/future_gaze` (Ninja presets) or
`build/release-vs/Release/future_gaze.exe` (Visual Studio preset).

> **macOS**: pass `-DCMAKE_PREFIX_PATH=$(brew --prefix freeglut)` to
> configure so CMake can locate the Homebrew freeglut headers and library:
> ```bash
> cmake --preset release -DCMAKE_PREFIX_PATH="$(brew --prefix freeglut)"
> cmake --build --preset release
> ```

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
