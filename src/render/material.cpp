#include "future_gaze/render/material.hpp"

#include <GL/freeglut.h>

namespace future_gaze
{

void Material::Apply() const {
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient_.data());
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse_.data());
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular_.data());
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission_.data());
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess_);
    glColor4f(diffuse_[0], diffuse_[1], diffuse_[2], diffuse_[3]);
}

}  // namespace future_gaze
