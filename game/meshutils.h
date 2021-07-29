#pragma once

#include "mesh.h"

#include <memory>
#include <string>

struct MeshVertex {
    glm::vec3 position;
    glm::vec2 texcoord;
    glm::vec3 normal;
};

std::unique_ptr<Mesh> makeMesh(const std::vector<MeshVertex> &vertices, GLenum primitive = GL_TRIANGLES);

std::unique_ptr<Mesh> loadMesh(const std::string &path);
