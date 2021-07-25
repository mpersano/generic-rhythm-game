#pragma once

#include <glm/glm.hpp>

struct Bezier {
    glm::vec3 p0, p1, p2;

    glm::vec3 eval(float t) const
    {
        return p1 + (1.0f - t) * (1.0f - t) * (p0 - p1) + t * t * (p2 - p1);
    }

    glm::vec3 direction(float t) const
    {
        return 2.0f * (1.0f - t) * (p1 - p0) + 2.0f * t * (p2 - p1);
    }
};
