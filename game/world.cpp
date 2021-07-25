#include "world.h"

#include "camera.h"
#include "material.h"
#include "mesh.h"
#include "renderer.h"
#include "shadermanager.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/string_cast.hpp>
#include <spdlog/spdlog.h>

namespace {

struct Bezier {
    glm::vec3 p0, p1, p2;

    glm::vec3 eval(float t) const
    {
        const float k0 = (1.0f - t) * (1.0f - t);
        const float k1 = 2.0f * (1.0f - t) * t;
        const float k2 = t * t;
        return k0 * p0 + k1 * p1 + k2 * p2;
    }
};

const Material *debugMaterial()
{
    return cachedMaterial(MaterialKey { ShaderManager::Program::Debug, {} });
}
} // namespace

World::World()
    : m_shaderManager(new ShaderManager)
    , m_camera(new Camera)
    , m_renderer(new Renderer(m_shaderManager.get(), m_camera.get()))
{
    initializeTrackMesh();
}

World::~World() = default;

void World::resize(int width, int height)
{
    m_camera->setAspectRatio(static_cast<float>(width) / height);
    m_renderer->resize(width, height);
}

void World::update(InputState inputState, float elapsed)
{
    m_angle += 0.5 * elapsed;
}

void World::render() const
{
    glClearColor(0, 0, 0, 0);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    m_camera->setEye(glm::vec3(0, 0, 5));
    m_camera->setCenter(glm::vec3(0, 0, 0));
    m_camera->setUp(glm::vec3(0, 1, 0));

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const auto modelMatrix = glm::rotate(glm::mat4(1), m_angle, glm::vec3(0, 1, 0));

    m_renderer->begin();
    for (auto &mesh : m_trackSegments)
        m_renderer->render(mesh.get(), debugMaterial(), modelMatrix);
    m_renderer->end();
}

static void initializeSegment(std::vector<glm::vec3> &vertices, const glm::vec3 &from, const glm::vec3 &to, int level)
{
    if (level == 0) {
        vertices.push_back(from);
        return;
    }

    const auto dist = glm::distance(to, from);
    const auto perturb = glm::linearRand(0.0f, 0.5f * dist);

    // perturb midpoint along a random vector orthogonal to the segment direction
    const auto dir = glm::normalize(to - from);
    const auto side = glm::sphericalRand(1.0f);
    const auto up = glm::normalize(glm::cross(dir, side));

    const auto mid = 0.5f * (from + to) + perturb * up;

    initializeSegment(vertices, from, mid, level - 1);
    initializeSegment(vertices, mid, to, level - 1);
}

static std::unique_ptr<Mesh> makeLineMesh(const std::vector<glm::vec3> &vertices)
{
    static const std::vector<Mesh::VertexAttribute> attributes = {
        { 3, GL_FLOAT, 0 }
    };

    auto mesh = std::make_unique<Mesh>(GL_LINES);
    mesh->setVertexCount(vertices.size());
    mesh->setVertexSize(sizeof(glm::vec3));
    mesh->setVertexAttributes(attributes);

    mesh->initialize();
    mesh->setVertexData(vertices.data());

    return mesh;
}

static std::unique_ptr<Mesh> makeSegmentMesh(const Bezier &segment)
{
    constexpr auto Vertices = 12;

    std::vector<glm::vec3> vertices;
    for (size_t i = 0; i < Vertices; ++i) {
        const auto from = static_cast<float>(i) / Vertices;
        const auto to = static_cast<float>(i + 1) / Vertices;
        vertices.push_back(segment.eval(from));
        vertices.push_back(segment.eval(to));
    }

    return makeLineMesh(vertices);
}

void World::initializeTrackMesh()
{
    constexpr auto LargeRadius = 5.0f;
    constexpr auto InitialSegments = 5;
    constexpr auto AngleDelta = 2.0f * M_PI / InitialSegments;

    std::vector<glm::vec3> controlPoints;
    for (int i = 0; i < InitialSegments; ++i) {
        const auto pointAt = [](float angle) {
            return glm::vec3(std::cos(angle), std::sin(angle), 0.0f);
        };
        const auto start = pointAt(i * AngleDelta);
        const auto end = pointAt((i + 1) * AngleDelta);
        initializeSegment(controlPoints, start, end, 2);
    }

    for (size_t i = 0, size = controlPoints.size(); i < size; ++i) {
        const auto prev = controlPoints[(i + size - 1) % size];
        const auto cur = controlPoints[i];
        const auto next = controlPoints[(i + 1) % size];

        const auto p0 = 0.5f * (prev + cur);
        const auto p1 = cur;
        const auto p2 = 0.5f * (cur + next);

        const auto segment = Bezier { p0, p1, p2 };
        m_trackSegments.push_back(makeSegmentMesh(segment));
    }
}
