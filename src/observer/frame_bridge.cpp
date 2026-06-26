#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "future_gaze/observer/frame_stream.hpp"

namespace
{

using future_gaze::observer::FrameRecordHeader;
using future_gaze::observer::FrameStreamHeader;
using future_gaze::observer::kFrameStreamMagic;
using future_gaze::observer::kFrameStreamVersion;
using future_gaze::observer::kPixelFormatRgb24;

void Usage(const char* argv0) {
    std::cerr << "usage: " << argv0
              << " --fifo PATH --manifest PATH\n"
                 "Reads Future's Gaze framebuffer stream, writes flipped rgb24 "
                 "frames to stdout.\n";
}

bool ReadExact(std::istream& in, char* dst, std::size_t size) {
    in.read(dst, static_cast<std::streamsize>(size));
    return static_cast<std::size_t>(in.gcount()) == size;
}

[[noreturn]] void Die(const std::string& message) {
    std::cerr << "future_gaze_frame_bridge: " << message << "\n";
    std::exit(EXIT_FAILURE);
}

}  // namespace

int main(int argc, char** argv) {
    std::string fifo_path;
    std::string manifest_path;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--fifo" && i + 1 < argc) {
            fifo_path = argv[++i];
        }
        else if (arg == "--manifest" && i + 1 < argc) {
            manifest_path = argv[++i];
        }
        else if (arg == "--help" || arg == "-h") {
            Usage(argv[0]);
            return EXIT_SUCCESS;
        }
        else {
            Usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (fifo_path.empty() || manifest_path.empty()) {
        Usage(argv[0]);
        return EXIT_FAILURE;
    }

    std::ifstream input(fifo_path, std::ios::binary);
    if (!input.is_open()) {
        Die("cannot open fifo '" + fifo_path + "'");
    }
    std::ofstream manifest(manifest_path);
    if (!manifest.is_open()) {
        Die("cannot open manifest '" + manifest_path + "'");
    }
    manifest << std::setprecision(17);

    FrameStreamHeader header{};
    if (!ReadExact(input, reinterpret_cast<char*>(&header), sizeof(header))) {
        Die("missing or truncated stream header");
    }
    if (std::memcmp(header.magic, kFrameStreamMagic, sizeof(header.magic)) !=
        0) {
        Die("bad stream magic");
    }
    if (header.version != kFrameStreamVersion) {
        Die("unsupported stream version " + std::to_string(header.version));
    }
    if (header.width == 0 || header.height == 0) {
        Die("invalid frame dimensions");
    }
    if (header.pixel_format != kPixelFormatRgb24) {
        Die("unsupported pixel format " + std::to_string(header.pixel_format));
    }
    if (!(header.fps > 0.0)) {
        Die("invalid fps");
    }

    const std::uint64_t bytes_per_frame64 =
        static_cast<std::uint64_t>(header.width) *
        static_cast<std::uint64_t>(header.height) * 3ULL;
    if (bytes_per_frame64 > std::numeric_limits<std::uint32_t>::max()) {
        Die("frame is too large");
    }
    const auto bytes_per_frame = static_cast<std::uint32_t>(bytes_per_frame64);
    const std::size_t row_bytes = static_cast<std::size_t>(header.width) * 3U;

    std::vector<unsigned char> frame(bytes_per_frame);
    std::vector<unsigned char> flipped(bytes_per_frame);
    std::uint64_t expected_index = 0;

    for (;;) {
        FrameRecordHeader record{};
        input.read(reinterpret_cast<char*>(&record), sizeof(record));
        const std::streamsize got = input.gcount();
        if (got == 0 && input.eof()) {
            break;
        }
        if (got != static_cast<std::streamsize>(sizeof(record))) {
            Die("truncated frame record header");
        }
        if (record.frame_index != expected_index) {
            Die("non-monotonic frame index: expected " +
                std::to_string(expected_index) + ", got " +
                std::to_string(record.frame_index));
        }
        if (record.byte_count != bytes_per_frame) {
            Die("bad frame byte count at frame " +
                std::to_string(record.frame_index));
        }
        if (!ReadExact(input, reinterpret_cast<char*>(frame.data()),
                       frame.size())) {
            Die("truncated frame payload at frame " +
                std::to_string(record.frame_index));
        }

        for (std::uint32_t y = 0; y < header.height; ++y) {
            const auto src =
                static_cast<std::size_t>(header.height - 1U - y) * row_bytes;
            const auto dst = static_cast<std::size_t>(y) * row_bytes;
            std::memcpy(flipped.data() + dst, frame.data() + src, row_bytes);
        }

        std::cout.write(reinterpret_cast<const char*>(flipped.data()),
                        static_cast<std::streamsize>(flipped.size()));
        if (!std::cout) {
            Die("failed writing rgb24 stdout");
        }

        manifest << "{\"frame_index\":" << record.frame_index
                 << ",\"app_time\":" << record.app_time
                 << ",\"width\":" << header.width
                 << ",\"height\":" << header.height << ",\"fps\":" << header.fps
                 << ",\"pixel_format\":\"rgb24\"}\n";
        if (!manifest) {
            Die("failed writing manifest");
        }
        ++expected_index;
    }

    return EXIT_SUCCESS;
}
