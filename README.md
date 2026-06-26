# Future's Gaze

## Prequest

## Build Modes

Normal development builds use the existing `clang-debug` or `default-debug`
CMake presets. These builds should not include the deterministic recorder hook
or the C++ recorder bridge.

Reliable dynamic-video verification is planned as an observer-only/debug build
path. Use a future `observer-debug` preset, or a CMake option such as
`FUTURE_GAZE_OBSERVER_TOOLS=ON`, to compile the recorder hook and its separate
C++ bridge helper. That code path is only for deterministic dynamic
verification and should not be treated as part of the production app.

The legacy `xwd` burst recorder is an intrusive fallback only. It can starve the
GLUT loop under WSLg, so it must not be used as animation-timing ground truth.
