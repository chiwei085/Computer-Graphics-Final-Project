#pragma once

#include <cstdint>

namespace future_gaze::observer
{

constexpr char kFrameStreamMagic[4] = {'F', 'G', 'F', 'R'};
constexpr std::uint32_t kFrameStreamVersion = 1;
constexpr std::uint32_t kPixelFormatRgb24 = 1;

#pragma pack(push, 1)
struct FrameStreamHeader
{
    char magic[4];
    std::uint32_t version;
    std::uint32_t width;
    std::uint32_t height;
    double fps;
    std::uint32_t pixel_format;
};

struct FrameRecordHeader
{
    std::uint64_t frame_index;
    double app_time;
    std::uint32_t byte_count;
};
#pragma pack(pop)

static_assert(sizeof(FrameStreamHeader) == 28);
static_assert(sizeof(FrameRecordHeader) == 20);

float ObserverDeltaSeconds(float fallback_delta_seconds);
void RecordFramebuffer(int width, int height, float app_time_seconds);

}  // namespace future_gaze::observer
