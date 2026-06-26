#pragma once

#include "future_gaze/math/geometry.hpp"

namespace future_gaze
{

struct RoomBounds
{
    Aabb3 interior{{-8.0f, 0.0f, -9.0f}, {8.0f, 6.4f, 11.5f}};
    float wall_thickness = 0.24f;
    float camera_radius = 0.45f;
    float near_plane_clearance = 0.35f;
    float ceiling_clearance = 0.18f;

    [[nodiscard]] Aabb3 CameraInterior() const noexcept {
        return interior.Inset({camera_radius + near_plane_clearance,
                               camera_radius,
                               camera_radius + near_plane_clearance});
    }

    [[nodiscard]] CameraFraming DefaultFraming() const noexcept {
        return {{{0.0f, 1.95f, -0.85f}, 2.65f}, 1.22f, 4.8f};
    }
};

[[nodiscard]] constexpr RoomBounds DefaultRoomBounds() noexcept {
    return {};
}

}  // namespace future_gaze
