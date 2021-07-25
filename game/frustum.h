#pragma once

#include <array>
#include <glm/glm.hpp>

struct BoundingBox;

struct Plane {
    Plane() = default;
    Plane(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2);
    Plane(const glm::vec3 &n, const glm::vec3 &p);

    float distance(const glm::vec3 &p) const
    {
        return p.x * a + p.y * b + p.z * c + d;
    }

    float a, b, c, d;
};

struct Frustum {
    bool contains(const BoundingBox &box, const glm::mat4 &modelMatrix) const;
    std::array<Plane, 6> planes;
};
