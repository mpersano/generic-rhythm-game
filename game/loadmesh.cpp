#include "loadmesh.h"

#include "mesh.h"

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include <boost/algorithm/string.hpp>

#include <fstream>
#include <vector>

struct MeshVertex {
    glm::vec3 position;
    glm::vec3 normal;
};

static std::unique_ptr<Mesh> makeMesh(const std::vector<MeshVertex> &vertices)
{
    static const std::vector<Mesh::VertexAttribute> attributes = {
        { 3, GL_FLOAT, offsetof(MeshVertex, position) },
        { 3, GL_FLOAT, offsetof(MeshVertex, normal) },
    };

    auto mesh = std::make_unique<Mesh>(GL_TRIANGLES);
    mesh->setVertexCount(vertices.size());
    mesh->setVertexSize(sizeof(MeshVertex));
    mesh->setVertexAttributes(attributes);

    mesh->initialize();
    mesh->setVertexData(vertices.data());

    return mesh;
}

std::unique_ptr<Mesh> loadMesh(const std::string &path)
{
    std::ifstream ifs(path);
    if (ifs.fail()) {
        spdlog::warn("Failed to open {}", path);
        return {};
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    struct Vertex {
        int positionIndex;
        int normalIndex;
    };
    using Face = std::vector<Vertex>;
    std::vector<Face> faces;

    std::string line;
    while (std::getline(ifs, line)) {
        std::vector<std::string> tokens;
        boost::split(tokens, line, boost::is_any_of(" \t"), boost::token_compress_on);
        if (tokens.empty())
            continue;
        if (tokens.front() == "v") {
            assert(tokens.size() == 4);
            positions.emplace_back(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
        } else if (tokens.front() == "vn") {
            assert(tokens.size() == 4);
            normals.emplace_back(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
        } else if (tokens.front() == "f") {
            Face f;
            for (auto it = std::next(tokens.begin()); it != tokens.end(); ++it) {
                std::vector<std::string> components;
                boost::split(components, *it, boost::is_any_of("/"), boost::token_compress_off);
                assert(components.size() == 3);
                f.push_back({ std::stoi(components[0]) - 1, std::stoi(components[2]) - 1 });
            }
            faces.push_back(f);
        }
    }

    spdlog::info("positions={} normals={} faces={}", positions.size(), normals.size(), faces.size());

    std::vector<MeshVertex> vertices;

    for (const auto &face : faces) {
        for (size_t i = 1; i < face.size() - 1; ++i) {
            const auto toVertex = [i, &positions, &normals](const auto &vertex) {
                assert(vertex.positionIndex < positions.size());
                assert(vertex.normalIndex < normals.size());
                return MeshVertex { positions[vertex.positionIndex], normals[vertex.normalIndex] };
            };
            const auto v0 = toVertex(face[0]);
            const auto v1 = toVertex(face[i]);
            const auto v2 = toVertex(face[i + 1]);
            vertices.push_back(v0);
            vertices.push_back(v1);
            vertices.push_back(v2);
        }
    }

    return makeMesh(vertices);
}
