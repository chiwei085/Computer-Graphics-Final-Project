#include "future_gaze/render/obj_loader.hpp"

#include <charconv>
#include <cstdlib>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace future_gaze
{

namespace
{

bool ParseFloat(std::string_view& sv, float& out) {
    while (!sv.empty() && sv.front() == ' ') sv.remove_prefix(1);
    if (sv.empty()) return false;
    char* end = nullptr;
    out = std::strtof(sv.data(), &end);
    if (end == sv.data()) return false;
    sv.remove_prefix(static_cast<std::size_t>(end - sv.data()));
    return true;
}

struct MtlEntry
{
    Material::Color ambient{0.1f, 0.1f, 0.1f, 1.0f};
    Material::Color diffuse{0.8f, 0.8f, 0.8f, 1.0f};
    Material::Color specular{0.5f, 0.5f, 0.5f, 1.0f};
    float shininess = 32.0f;
};

[[nodiscard]] std::unordered_map<std::string, MtlEntry> LoadMtl(
    const std::filesystem::path& mtl_path) {
    std::unordered_map<std::string, MtlEntry> result;
    std::ifstream file(mtl_path);
    if (!file.is_open()) return result;

    std::string current_name;
    MtlEntry current{};

    auto commit = [&] {
        if (!current_name.empty()) result[current_name] = current;
    };

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::string_view sv(line);

        auto token_end = sv.find(' ');
        std::string_view token = sv.substr(0, token_end);
        if (token_end != std::string_view::npos)
            sv.remove_prefix(token_end + 1);
        else
            sv = {};

        if (token == "newmtl") {
            commit();
            current_name = std::string(sv);
            current = {};
        }
        else if (token == "Ka") {
            float r = 0, g = 0, b = 0;
            ParseFloat(sv, r);
            ParseFloat(sv, g);
            ParseFloat(sv, b);
            current.ambient = {r, g, b, 1.0f};
        }
        else if (token == "Kd") {
            float r = 0, g = 0, b = 0;
            ParseFloat(sv, r);
            ParseFloat(sv, g);
            ParseFloat(sv, b);
            current.diffuse = {r, g, b, 1.0f};
        }
        else if (token == "Ks") {
            float r = 0, g = 0, b = 0;
            ParseFloat(sv, r);
            ParseFloat(sv, g);
            ParseFloat(sv, b);
            current.specular = {r, g, b, 1.0f};
        }
        else if (token == "Ns") {
            float ns = 32.0f;
            ParseFloat(sv, ns);
            current.shininess = ns;
        }
    }
    commit();
    return result;
}

// Face index triple: position / texcoord / normal (all 1-based; 0 = absent).
struct FaceIdx
{
    int v = 0, vt = 0, vn = 0;
};

[[nodiscard]] FaceIdx ParseFaceToken(std::string_view token) {
    FaceIdx idx;
    // formats: v  |  v/vt  |  v//vn  |  v/vt/vn
    auto slash1 = token.find('/');
    {
        int val = 0;
        std::from_chars(token.data(),
                        slash1 != std::string_view::npos
                            ? token.data() + slash1
                            : token.data() + token.size(),
                        val);
        idx.v = val;
    }
    if (slash1 == std::string_view::npos) return idx;

    auto after1 = token.substr(slash1 + 1);
    auto slash2 = after1.find('/');
    if (slash2 != 0) {
        int val = 0;
        std::from_chars(after1.data(),
                        slash2 != std::string_view::npos
                            ? after1.data() + slash2
                            : after1.data() + after1.size(),
                        val);
        idx.vt = val;
    }
    if (slash2 == std::string_view::npos) return idx;

    auto after2 = after1.substr(slash2 + 1);
    if (!after2.empty()) {
        int val = 0;
        std::from_chars(after2.data(), after2.data() + after2.size(), val);
        idx.vn = val;
    }
    return idx;
}

// Resolve OBJ's 1-based (optionally negative) index to a 0-based index.
[[nodiscard]] int Resolve(int one_based, int total) {
    if (one_based == 0) return -1;
    return one_based > 0 ? one_based - 1 : total + one_based;
}

}  // namespace

std::vector<ModelMesh> ObjLoader::Load(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return {};

    std::vector<Vec3> positions;
    std::vector<std::array<float, 2>> texcoords;
    std::vector<Vec3> normals;

    std::unordered_map<std::string, MtlEntry> mtl_map;

    struct FaceGroup
    {
        std::string mat_name;
        std::vector<FaceIdx> face_indices;  // flat: 3 per triangle
    };
    std::vector<FaceGroup> groups;
    FaceGroup* current_group = nullptr;

    auto ensure_group = [&](const std::string& name = "") {
        if (!current_group) {
            groups.push_back({name, {}});
            current_group = &groups.back();
        }
    };

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::string_view sv(line);

        auto space = sv.find(' ');
        std::string_view token = sv.substr(0, space);
        if (space != std::string_view::npos)
            sv.remove_prefix(space + 1);
        else
            sv = {};

        if (token == "mtllib") {
            auto mtl_path = path.parent_path() / std::string(sv);
            mtl_map = LoadMtl(mtl_path);
        }
        else if (token == "v") {
            float x = 0, y = 0, z = 0;
            ParseFloat(sv, x);
            ParseFloat(sv, y);
            ParseFloat(sv, z);
            positions.push_back({x, y, z});
        }
        else if (token == "vt") {
            float u = 0, v = 0;
            ParseFloat(sv, u);
            ParseFloat(sv, v);
            texcoords.push_back({u, v});
        }
        else if (token == "vn") {
            float x = 0, y = 0, z = 0;
            ParseFloat(sv, x);
            ParseFloat(sv, y);
            ParseFloat(sv, z);
            normals.push_back({x, y, z});
        }
        else if (token == "usemtl") {
            groups.push_back({std::string(sv), {}});
            current_group = &groups.back();
        }
        else if (token == "f") {
            ensure_group();
            std::vector<FaceIdx> corners;
            while (!sv.empty()) {
                while (!sv.empty() && sv.front() == ' ') sv.remove_prefix(1);
                if (sv.empty()) break;
                auto end = sv.find(' ');
                auto face_token = sv.substr(0, end);
                corners.push_back(ParseFaceToken(face_token));
                if (end == std::string_view::npos) break;
                sv.remove_prefix(end + 1);
            }
            // Fan-triangulate: (0,1,2), (0,2,3), …
            for (std::size_t i = 1; i + 1 < corners.size(); ++i) {
                current_group->face_indices.push_back(corners[0]);
                current_group->face_indices.push_back(corners[i]);
                current_group->face_indices.push_back(corners[i + 1]);
            }
        }
    }

    if (groups.empty()) return {};

    const Vec3 kDefaultNormal{0.0f, 1.0f, 0.0f};
    const std::array<float, 2> kDefaultTexcoord{0.0f, 0.0f};

    std::vector<ModelMesh> result;
    result.reserve(groups.size());

    for (const auto& group : groups) {
        if (group.face_indices.empty()) continue;

        std::vector<Vertex> vertices;
        vertices.reserve(group.face_indices.size());

        for (const auto& fi : group.face_indices) {
            const int vi = Resolve(fi.v, static_cast<int>(positions.size()));
            const int ni = Resolve(fi.vn, static_cast<int>(normals.size()));
            const int ti = Resolve(fi.vt, static_cast<int>(texcoords.size()));

            Vec3 pos = (vi >= 0 && vi < static_cast<int>(positions.size()))
                           ? positions[static_cast<std::size_t>(vi)]
                           : Vec3{};
            Vec3 norm = (ni >= 0 && ni < static_cast<int>(normals.size()))
                            ? normals[static_cast<std::size_t>(ni)]
                            : kDefaultNormal;
            std::array<float, 2> uv =
                (ti >= 0 && ti < static_cast<int>(texcoords.size()))
                    ? texcoords[static_cast<std::size_t>(ti)]
                    : kDefaultTexcoord;

            vertices.push_back({pos, norm, uv});
        }

        const auto it = mtl_map.find(group.mat_name);
        Material mat;
        if (it != mtl_map.end()) {
            const auto& m = it->second;
            mat = Material(m.ambient, m.diffuse, m.specular, m.shininess);
        }

        result.push_back({Mesh(std::move(vertices)), mat, group.mat_name});
    }

    return result;
}

}  // namespace future_gaze
