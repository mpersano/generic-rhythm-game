#include "frustum.h"

#include "geometryutils.h"

#include <algorithm>

Plane::Plane(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2)
{
    const auto u = p1 - p0;
    const auto v = p2 - p0;
    const auto n = glm::normalize(glm::cross(u, v));
    a = n.x;
    b = n.y;
    c = n.z;
    d = -glm::dot(n, p0);
}

Plane::Plane(const glm::vec3 &n, const glm::vec3 &p)
{
    a = n.x;
    b = n.y;
    c = n.z;
    d = -glm::dot(n, p);
}

bool Frustum::contains(const BoundingBox &box, const glm::mat4 &modelMatrix) const
{
    std::array<glm::vec3, 8> verts;
    for (int i = 0; i < 8; ++i) {
        const auto x = (i & 1) ? box.min.x : box.max.x;
        const auto y = (i & 2) ? box.min.y : box.max.y;
        const auto z = (i & 4) ? box.min.z : box.max.z;
        verts[i] = glm::vec3(modelMatrix * glm::vec4(x, y, z, 1));
    }
    for (const auto &plane : planes) {
        const auto outside = std::all_of(verts.begin(), verts.end(), [&plane](const auto &v) {
            return plane.distance(v) < 0.0f;
        });
        if (outside)
            return false;
    }
    return true;
}
