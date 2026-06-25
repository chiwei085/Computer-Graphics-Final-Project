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
        : ambient_(ambient),
          diffuse_(diffuse),
          specular_(specular),
          shininess_(shininess) {}

    void SetEmission(Color emission) { emission_ = emission; }
    void SetAmbient(Color ambient) { ambient_ = ambient; }
    void SetDiffuse(Color diffuse) { diffuse_ = diffuse; }
    void SetSpecular(Color specular) { specular_ = specular; }
    void SetShininess(float shininess) { shininess_ = shininess; }

    [[nodiscard]] const Color& diffuse() const { return diffuse_; }

    void Apply() const;

    // Named presets
    static Material Wood();
    static Material DarkWood();
    static Material Metal();
    static Material Chrome();
    static Material Porcelain();
    static Material Glass();
    static Material Cloth();
    static Material BlackMatte();
    static Material EmissiveCyan();
    static Material EmissiveGreen();

    // Additive-glow tints. The alpha channel sets the halo intensity since the
    // additive pass multiplies the colour by GL_SRC_ALPHA before adding.
    static Material GlowCyan(float intensity = 0.55f);
    static Material GlowGreen(float intensity = 0.45f);

private:
    Color ambient_{0.12f, 0.16f, 0.18f, 1.0f};
    Color diffuse_{0.74f, 0.82f, 0.86f, 1.0f};
    Color specular_{0.7f, 0.78f, 0.82f, 1.0f};
    Color emission_{0.0f, 0.0f, 0.0f, 1.0f};
    float shininess_ = 32.0f;
};

}  // namespace future_gaze
