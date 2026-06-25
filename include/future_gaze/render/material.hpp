#pragma once

#include <array>

namespace future_gaze
{

class Material
{
public:
    using Color = std::array<float, 4>;

    Material() = default;
    Material(Color ambient, Color diffuse, Color specular, float shininess)
        : ambient_(ambient), diffuse_(diffuse), specular_(specular),
          shininess_(shininess) {}

    void Apply() const;

private:
    Color ambient_{0.12f, 0.16f, 0.18f, 1.0f};
    Color diffuse_{0.74f, 0.82f, 0.86f, 1.0f};
    Color specular_{0.7f, 0.78f, 0.82f, 1.0f};
    Color emission_{0.0f, 0.0f, 0.0f, 1.0f};
    float shininess_ = 32.0f;
};

}  // namespace future_gaze
