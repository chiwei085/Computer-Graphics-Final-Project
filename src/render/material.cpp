#include "future_gaze/render/material.hpp"

#include <GL/freeglut.h>

namespace future_gaze
{

Material Material::Wood() {
    return Material({0.10f, 0.06f, 0.03f, 1.0f}, {0.45f, 0.28f, 0.12f, 1.0f},
                    {0.26f, 0.20f, 0.13f, 1.0f}, 30.0f);
}

Material Material::DarkWood() {
    return Material({0.06f, 0.04f, 0.02f, 1.0f}, {0.22f, 0.14f, 0.07f, 1.0f},
                    {0.10f, 0.06f, 0.03f, 1.0f}, 8.0f);
}

Material Material::Metal() {
    return Material({0.10f, 0.11f, 0.12f, 1.0f}, {0.40f, 0.42f, 0.45f, 1.0f},
                    {0.75f, 0.78f, 0.80f, 1.0f}, 80.0f);
}

Material Material::Chrome() {
    return Material({0.18f, 0.20f, 0.22f, 1.0f}, {0.60f, 0.62f, 0.65f, 1.0f},
                    {0.95f, 0.96f, 0.97f, 1.0f}, 128.0f);
}

Material Material::Porcelain() {
    return Material({0.20f, 0.20f, 0.22f, 1.0f}, {0.88f, 0.88f, 0.92f, 1.0f},
                    {0.70f, 0.72f, 0.75f, 1.0f}, 64.0f);
}

Material Material::Glass() {
    return Material({0.05f, 0.07f, 0.10f, 1.0f}, {0.55f, 0.70f, 0.85f, 1.0f},
                    {0.90f, 0.92f, 0.95f, 1.0f}, 128.0f);
}

Material Material::Cloth() {
    return Material({0.14f, 0.13f, 0.12f, 1.0f}, {0.82f, 0.80f, 0.76f, 1.0f},
                    {0.06f, 0.06f, 0.06f, 1.0f}, 4.0f);
}

Material Material::BlackMatte() {
    return Material({0.02f, 0.02f, 0.02f, 1.0f}, {0.05f, 0.05f, 0.05f, 1.0f},
                    {0.02f, 0.02f, 0.02f, 1.0f}, 1.0f);
}

Material Material::EmissiveCyan() {
    Material mat({0.05f, 0.15f, 0.20f, 1.0f}, {0.10f, 0.30f, 0.40f, 1.0f},
                 {0.40f, 0.70f, 0.80f, 1.0f}, 32.0f);
    mat.SetEmission({0.10f, 0.45f, 0.60f, 1.0f});
    return mat;
}

Material Material::EmissiveGreen() {
    Material mat({0.05f, 0.16f, 0.07f, 1.0f}, {0.30f, 0.75f, 0.38f, 1.0f},
                 {0.25f, 0.55f, 0.32f, 1.0f}, 24.0f);
    mat.SetEmission({0.03f, 0.16f, 0.06f, 1.0f});
    return mat;
}

Material Material::GlowCyan(float intensity) {
    Material mat({0.0f, 0.0f, 0.0f, 1.0f}, {0.35f, 0.85f, 1.0f, intensity},
                 {0.0f, 0.0f, 0.0f, 1.0f}, 0.0f);
    mat.SetEmission({0.35f, 0.85f, 1.0f, 1.0f});
    return mat;
}

Material Material::GlowGreen(float intensity) {
    Material mat({0.0f, 0.0f, 0.0f, 1.0f}, {0.30f, 1.0f, 0.45f, intensity},
                 {0.0f, 0.0f, 0.0f, 1.0f}, 0.0f);
    mat.SetEmission({0.30f, 1.0f, 0.45f, 1.0f});
    return mat;
}

void Material::Apply() const {
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient_.data());
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse_.data());
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular_.data());
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission_.data());
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess_);
    glColor4f(diffuse_[0], diffuse_[1], diffuse_[2], diffuse_[3]);
}

}  // namespace future_gaze
