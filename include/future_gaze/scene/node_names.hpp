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

// Procedural pivot nodes for Robonaut OBJ material chunks. They draw no extra
// geometry; the imported NASA mesh pieces hang under them so the static OBJ can
// read as animated even though it is not skinned.
inline constexpr const char* kRobonautDanceRoot = "robonaut_dance_root";
inline constexpr const char* kRobonautDanceCenter = "robonaut_dance_center";
inline constexpr const char* kRobonautDanceLowerBody =
    "robonaut_dance_lower_body";
inline constexpr const char* kRobonautDanceUpperBody =
    "robonaut_dance_upper_body";
inline constexpr const char* kRobonautDanceHead = "robonaut_dance_head";
inline constexpr const char* kRobonautDanceLeftShoulder =
    "robonaut_dance_left_shoulder";
inline constexpr const char* kRobonautDanceLeftArm = "robonaut_dance_left_arm";
inline constexpr const char* kRobonautDanceLeftElbow =
    "robonaut_dance_left_elbow";
inline constexpr const char* kRobonautDanceLeftWrist =
    "robonaut_dance_left_wrist";
inline constexpr const char* kRobonautDanceRightShoulder =
    "robonaut_dance_right_shoulder";
inline constexpr const char* kRobonautDanceRightArm =
    "robonaut_dance_right_arm";
inline constexpr const char* kRobonautDanceRightElbow =
    "robonaut_dance_right_elbow";
inline constexpr const char* kRobonautDanceRightWrist =
    "robonaut_dance_right_wrist";
inline constexpr const char* kRobonautDanceLeftLeg = "robonaut_dance_left_leg";
inline constexpr const char* kRobonautDanceLeftKnee =
    "robonaut_dance_left_knee";
inline constexpr const char* kRobonautDanceLeftAnkle =
    "robonaut_dance_left_ankle";
inline constexpr const char* kRobonautDanceRightLeg =
    "robonaut_dance_right_leg";
inline constexpr const char* kRobonautDanceRightKnee =
    "robonaut_dance_right_knee";
inline constexpr const char* kRobonautDanceRightAnkle =
    "robonaut_dance_right_ankle";

// Gaze-driven story layers. Names are prefixed with the target index:
//   future_0_slice_3, memory_2_argument_1, ...
// SceneAnimation parses that index so each authored fragment follows the
// correct gaze anchor.
inline constexpr const char* kFuturePrefix = "future_";
inline constexpr const char* kMemoryPrefix = "memory_";
inline constexpr const char* kBlindPrefix = "blind_";

// Legacy aliases — identical to kFuturePrefix / kMemoryPrefix above.
inline constexpr const char* kFutureSlice = kFuturePrefix;
inline constexpr const char* kMemoryPanel = kMemoryPrefix;

}  // namespace future_gaze::names
