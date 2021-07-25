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
    if ((inputState & InputState::Fire1) == InputState::None)
        m_trackTime += elapsed;
}

void World::render() const
{
    glClearColor(0, 0, 0, 0);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if 0
    m_camera->setEye(glm::vec3(0, 0, 5));
    m_camera->setCenter(glm::vec3(0, 0, 0));
    m_camera->setUp(glm::vec3(0, 1, 0));

    const auto angle = 0.5f * m_trackTime;
    const auto modelMatrix = glm::rotate(glm::mat4(1), angle, glm::vec3(0, 1, 0));
#else
    const auto distance = 0.1f * m_trackTime;

    // reference: https://i.kym-cdn.com/photos/images/newsfeed/000/234/765/b7e.jpg

    const auto partIndex = [this, distance]() -> std::size_t {
        auto it = std::upper_bound(
                m_pathParts.begin(), m_pathParts.end(),
                distance,
                [](float distance, const auto &part) {
                    return distance < part.distance;
                });
        if (it == m_pathParts.begin())
            return 0;
        return std::distance(m_pathParts.begin(), it) - 1;
    }();

    assert(partIndex < m_pathParts.size() - 1);

    const auto &curPart = m_pathParts[partIndex];
    const auto &nextPart = m_pathParts[partIndex + 1];

    assert(distance >= curPart.distance && distance < nextPart.distance);

    const float t = (distance - curPart.distance) / (nextPart.distance - curPart.distance);
    assert(t >= 0.0f && t < 1.0f);

    const auto center = glm::mix(curPart.center, nextPart.center, t);

#if 0
    // tried to use quaternions to interpolate the orientation but ran it some weird discontinuities,
    // let's do it the stupid way instead
    const auto dir = glm::normalize(glm::mix(curPart.orientation[2], nextPart.orientation[2], t));
    auto up = glm::normalize(glm::mix(curPart.orientation[0], nextPart.orientation[0], t));
    const auto side = glm::normalize(glm::cross(dir, up));
    up = glm::normalize(glm::cross(side, dir));
#else
    const auto q0 = glm::quat_cast(curPart.orientation);
    const auto q1 = glm::quat_cast(nextPart.orientation);
    const auto q = glm::mix(q0, q1, t);
    const auto orientation = glm::mat3_cast(q);
    const auto dir = orientation[2];
    const auto up = orientation[0];
#endif

    m_camera->setEye(center + 0.3f * up - 0.2f * dir);
    m_camera->setCenter(center);
    m_camera->setUp(up);

    const auto modelMatrix = glm::mat4(1);
#endif

    m_renderer->begin();
    for (const auto &segment : m_trackSegments) {
        m_renderer->render(segment->mesh.get(), debugMaterial(), modelMatrix);
    }
    m_renderer->end();
}

static void initializeSegment(std::vector<glm::vec3> &vertices, const glm::vec3 &from, const glm::vec3 &to, int level)
{
    if (level == 0) {
        vertices.push_back(from);
        return;
    }

    const auto dist = glm::distance(to, from);
    const auto perturb = glm::linearRand(0.25f, .5f) * dist;

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
    std::vector<glm::vec3> controlPoints;
    initializeSegment(controlPoints, glm::vec3(-4, 0, 0), glm::vec3(4, 0, 0), 5);

    glm::vec3 currentUp(0, 0, 1);
    std::optional<glm::vec3> prevCenter;
    float distance = 0.0f;

    for (size_t i = 1, size = controlPoints.size() - 1; i < size; ++i) {
        const auto prev = controlPoints[i - 1];
        const auto cur = controlPoints[i];
        const auto next = controlPoints[i + 1];

        const auto path = Bezier { 0.5f * (prev + cur), cur, 0.5f * (cur + next) };

        auto segment = std::make_unique<Segment>();
        segment->path = path;

        constexpr auto SegmentVertices = 40;
        constexpr auto TrackWidth = 0.1f;

        std::vector<glm::vec3> vertices;
        for (size_t j = 0; j < SegmentVertices; ++j) {
            const auto t = static_cast<float>(j) / SegmentVertices;

            const auto center = path.eval(t);

            if (prevCenter != std::nullopt)
                distance += glm::distance(*prevCenter, center);

            const auto dir = glm::normalize(path.direction(t));
            const auto side = glm::normalize(glm::cross(dir, currentUp));
            const auto up = glm::normalize(glm::cross(side, dir));

            const auto orientation = glm::mat3(up, side, dir);

            m_pathParts.push_back(PathPart { orientation, center, distance });

            vertices.push_back(center - side * TrackWidth);
            vertices.push_back(center + side * TrackWidth);

            currentUp = up;
            prevCenter = center;
        }

        segment->mesh = makeLineMesh(vertices);
        m_trackSegments.push_back(std::move(segment));
    }

    spdlog::info("Initialized track, length={} segments={} parts={}",
                 distance, m_trackSegments.size(), m_pathParts.size());
}
