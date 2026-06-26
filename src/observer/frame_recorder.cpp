#include <GL/freeglut.h>

#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "future_gaze/observer/frame_stream.hpp"

namespace future_gaze::observer
{
namespace
{

bool EnvFlag(const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return false;
    }
    const std::string_view flag{value};
    return flag != "0" && flag != "false" && flag != "FALSE";
}

double EnvDouble(const char* name, double fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }
    double parsed = fallback;
    const std::string_view text{value};
    const auto result =
        std::from_chars(text.data(), text.data() + text.size(), parsed);
    return (result.ptr != text.data() && parsed > 0.0) ? parsed : fallback;
}

std::uint64_t EnvU64(const char* name, std::uint64_t fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }
    std::uint64_t parsed = fallback;
    const std::string_view text{value};
    const auto result =
        std::from_chars(text.data(), text.data() + text.size(), parsed);
    return (result.ptr != text.data()) ? parsed : fallback;
}

class FrameRecorder
{
public:
    static FrameRecorder& Instance() {
        static FrameRecorder recorder;
        return recorder;
    }

    [[nodiscard]] float DeltaSeconds(float fallback_delta_seconds) const {
        if (!enabled_ || !fixed_dt_) {
            return fallback_delta_seconds;
        }
        return static_cast<float>(1.0 / fps_);
    }

    void Record(int width, int height, float app_time_seconds) {
        if (!enabled_ || failed_ || frame_index_ >= max_frames_) {
            return;
        }
        if (width <= 0 || height <= 0) {
            Fail("invalid framebuffer dimensions");
            return;
        }

        const std::uint64_t byte_count64 = static_cast<std::uint64_t>(width) *
                                           static_cast<std::uint64_t>(height) *
                                           3ULL;
        if (byte_count64 > std::numeric_limits<std::uint32_t>::max()) {
            Fail("framebuffer is too large");
            return;
        }
        const auto byte_count = static_cast<std::uint32_t>(byte_count64);

        OpenIfNeeded(width, height);
        if (!stream_.is_open()) {
            return;
        }

        pixels_.resize(byte_count);
        GLint old_pack_alignment = 4;
        glGetIntegerv(GL_PACK_ALIGNMENT, &old_pack_alignment);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadBuffer(GL_BACK);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE,
                     pixels_.data());
        glPixelStorei(GL_PACK_ALIGNMENT, old_pack_alignment);

        const double recorded_time =
            fixed_dt_ ? static_cast<double>(frame_index_) / fps_
                      : static_cast<double>(app_time_seconds);
        const FrameRecordHeader frame{
            frame_index_,
            recorded_time,
            byte_count,
        };
        stream_.write(reinterpret_cast<const char*>(&frame), sizeof(frame));
        stream_.write(reinterpret_cast<const char*>(pixels_.data()),
                      static_cast<std::streamsize>(pixels_.size()));
        stream_.flush();
        if (!stream_) {
            Fail("failed while writing framebuffer stream");
            return;
        }

        ++frame_index_;
        if (frame_index_ >= max_frames_) {
            std::cerr << "[recorder] wrote " << frame_index_
                      << " deterministic frames; exiting\n";
            std::exit(EXIT_SUCCESS);
        }
    }

private:
    FrameRecorder() {
        const char* fifo = std::getenv("FUTURE_GAZE_RECORD_FIFO");
        enabled_ = fifo != nullptr && fifo[0] != '\0';
        if (!enabled_) {
            return;
        }
        fifo_path_ = fifo;
        fps_ = EnvDouble("FUTURE_GAZE_RECORD_FPS", 30.0);
        max_frames_ = EnvU64("FUTURE_GAZE_RECORD_MAX_FRAMES", 1);
        fixed_dt_ = EnvFlag("FUTURE_GAZE_RECORD_FIXED_DT");
        std::cerr << "[recorder] enabled fifo='" << fifo_path_
                  << "' fps=" << fps_ << " max_frames=" << max_frames_
                  << " fixed_dt=" << (fixed_dt_ ? 1 : 0) << '\n';
    }

    void OpenIfNeeded(int width, int height) {
        if (stream_.is_open() || failed_) {
            return;
        }

        stream_.open(fifo_path_, std::ios::binary | std::ios::out);
        if (!stream_.is_open()) {
            const std::error_code open_error{errno, std::generic_category()};
            std::cerr << "[recorder] cannot open '" << fifo_path_
                      << "': " << open_error.message() << '\n';
            failed_ = true;
            return;
        }

        const FrameStreamHeader header{
            {kFrameStreamMagic[0], kFrameStreamMagic[1], kFrameStreamMagic[2],
             kFrameStreamMagic[3]},
            kFrameStreamVersion,
            static_cast<std::uint32_t>(width),
            static_cast<std::uint32_t>(height),
            fps_,
            kPixelFormatRgb24,
        };
        stream_.write(reinterpret_cast<const char*>(&header), sizeof(header));
        stream_.flush();
        if (!stream_) {
            Fail("failed while writing stream header");
        }
    }

    void Fail(const char* message) {
        std::cerr << "[recorder] " << message << '\n';
        failed_ = true;
    }

    bool enabled_ = false;
    bool fixed_dt_ = false;
    bool failed_ = false;
    double fps_ = 30.0;
    std::uint64_t max_frames_ = 1;
    std::uint64_t frame_index_ = 0;
    std::string fifo_path_;
    std::ofstream stream_;
    std::vector<unsigned char> pixels_;
};

}  // namespace

float ObserverDeltaSeconds(float fallback_delta_seconds) {
    return FrameRecorder::Instance().DeltaSeconds(fallback_delta_seconds);
}

void RecordFramebuffer(int width, int height, float app_time_seconds) {
    FrameRecorder::Instance().Record(width, height, app_time_seconds);
}

}  // namespace future_gaze::observer
