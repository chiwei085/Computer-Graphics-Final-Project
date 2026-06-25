#include "future_gaze/render/texture.hpp"

#include <GL/freeglut.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <array>
#include <limits>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace future_gaze
{

namespace
{

constexpr int kDesiredChannels = 4;

struct StbiDeleter
{
    void operator()(stbi_uc* pixels) const noexcept { stbi_image_free(pixels); }
};

using StbiPixels = std::unique_ptr<stbi_uc, StbiDeleter>;

constexpr int kCheckerWidth = 4;
constexpr int kCheckerHeight = 4;

[[nodiscard]] std::vector<std::uint8_t> MakeCheckerPixels() {
    const std::array<std::array<std::uint8_t, 3>, 2> colors = {
        std::array<std::uint8_t, 3>{214, 232, 238},
        std::array<std::uint8_t, 3>{48, 105, 122},
    };
    std::vector<std::uint8_t> pixels;
    pixels.reserve(kCheckerWidth * kCheckerHeight * kDesiredChannels);
    for (int y = 0; y < kCheckerHeight; ++y) {
        for (int x = 0; x < kCheckerWidth; ++x) {
            const auto& color = colors[(x + y) % 2];
            pixels.push_back(color[0]);
            pixels.push_back(color[1]);
            pixels.push_back(color[2]);
            pixels.push_back(255);
        }
    }
    return pixels;
}

}  // namespace

Texture::~Texture() {
    Release();
}

Texture::Texture(Texture&& other) noexcept
    : texture_id_(std::exchange(other.texture_id_, 0)) {}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        Release();
        texture_id_ = std::exchange(other.texture_id_, 0);
    }
    return *this;
}

bool Texture::LoadFromFile(const std::filesystem::path& path) {
    int width = 0;
    int height = 0;
    int channels = 0;
    StbiPixels pixels{stbi_load(path.string().c_str(), &width, &height,
                                &channels, kDesiredChannels)};
    if (pixels == nullptr) {
        return false;
    }

    return UploadPixels(pixels.get(), width, height);
}

bool Texture::LoadFromMemory(std::span<const std::uint8_t> data) {
    if (data.size() >
        static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return false;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    StbiPixels pixels{
        stbi_load_from_memory(data.data(), static_cast<int>(data.size()),
                              &width, &height, &channels, kDesiredChannels)};
    if (pixels == nullptr) {
        return false;
    }

    return UploadPixels(pixels.get(), width, height);
}

bool Texture::CreateDebugCheckerboard() {
    const std::vector<std::uint8_t> pixels = MakeCheckerPixels();
    return UploadPixels(pixels.data(), kCheckerWidth, kCheckerHeight);
}

void Texture::Bind() const {
    glBindTexture(GL_TEXTURE_2D, texture_id_);
}

bool Texture::UploadPixels(const unsigned char* pixels, int width, int height) {
    Release();
    glGenTextures(1, &texture_id_);
    Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels);
    return texture_id_ != 0;
}

void Texture::Release() {
    if (texture_id_ != 0) {
        glDeleteTextures(1, &texture_id_);
        texture_id_ = 0;
    }
}

}  // namespace future_gaze
