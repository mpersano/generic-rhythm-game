#pragma once

#include <glm/glm.hpp>

#include <optional>

struct Triangle;
struct Ray;
struct BoundingBox;

struct LineSegment {
    glm::vec3 from;
    glm::vec3 to;

    Ray ray() const;
    glm::vec3 pointAt(float t) const;
    std::optional<float> intersection(const Triangle &triangle) const;
    bool intersects(const BoundingBox &box) const;
};

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;

    glm::vec3 pointAt(float t) const;
    std::optional<float> intersection(const Triangle &triangle) const;
    bool intersects(const BoundingBox &box) const;
};

struct BoundingBox {
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(std::numeric_limits<float>::min());

    bool contains(const glm::vec3 &p) const;
    BoundingBox operator|(const glm::vec3 &p) const;
    BoundingBox &operator|=(const glm::vec3 &p);

    bool intersects(const LineSegment &segment) const;
    bool intersects(const Ray &ray) const;
};

struct Triangle {
    glm::vec3 v0, v1, v2;

    std::optional<float> intersection(const LineSegment &segment) const;
    std::optional<float> intersection(const Ray &ray) const;
};
