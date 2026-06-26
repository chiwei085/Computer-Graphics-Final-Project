#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "future_gaze/render/material.hpp"
#include "future_gaze/render/mesh.hpp"

namespace future_gaze
{

struct ModelMesh
{
    Mesh mesh;
    Material material;
    std::string material_name;
};

class ObjLoader
{
public:
    // Returns one ModelMesh per usemtl group; loads .mtl sidecar if present.
    [[nodiscard]] static std::vector<ModelMesh> Load(
        const std::filesystem::path& path);
};

}  // namespace future_gaze
