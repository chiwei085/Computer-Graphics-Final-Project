# Future's Gaze

## Prequest

## Build Modes

Normal development builds use the `dev` CMake preset. This clang+ninja Debug
build should not include the deterministic recorder hook or the C++ recorder
bridge.

Reliable dynamic-video verification uses the observer-only/debug `observer`
preset, which enables `FUTURE_GAZE_OBSERVER_TOOLS=ON` to compile the recorder
hook and its separate C++ bridge helper. That code path is only for
deterministic dynamic verification and should not be treated as part of the
production app.

The legacy `xwd` burst recorder is an intrusive fallback only. It can starve the
GLUT loop under WSLg, so it must not be used as animation-timing ground truth.
