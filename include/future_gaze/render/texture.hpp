#pragma once

#include <cstdint>
#include <filesystem>
#include <span>

namespace future_gaze
{

class Texture
{
public:
    Texture() = default;
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool LoadFromFile(const std::filesystem::path& path);
    bool LoadFromMemory(std::span<const std::uint8_t> data);
    bool CreateDebugCheckerboard();
    void Bind() const;

private:
    bool UploadPixels(const unsigned char* pixels, int width, int height);
    void Release();

    unsigned int texture_id_ = 0;
};

}  // namespace future_gaze
