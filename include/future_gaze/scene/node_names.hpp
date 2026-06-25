#pragma once

namespace future_gaze::names
{

// Names of the scene nodes that the animation system binds to by lookup.
//
// These are shared between the builders (which CREATE the nodes) and
// SceneAnimation (which FINDS them after the graph is built). Referencing one
// symbol from both sides means a rename is a single edit and the two halves can
// never silently drift apart — the old failure mode where renaming a node in a
// builder quietly disabled its animation channel. SceneAnimation::Bind() also
// warns at startup if any of these is missing, so a typo fails loudly.
inline constexpr const char* kPredictionCore = "prediction_core";
inline constexpr const char* kCircuitRing = "circuit_ring";
inline constexpr const char* kIngenuity = "ingenuity";
inline constexpr const char* kRobonaut = "robonaut";
inline constexpr const char* kFutureSlice = "future_slice";
inline constexpr const char* kMemoryPanel = "memory_panel";

}  // namespace future_gaze::names
