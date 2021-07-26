#include "world.h"

#include "bezier.h"
#include "camera.h"
#include "material.h"
#include "mesh.h"
#include "renderer.h"
#include "shadermanager.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/string_cast.hpp>
#include <spdlog/spdlog.h>

using namespace std::string_literals;

namespace {

const Material *trackMaterial()
{
    return cachedMaterial(MaterialKey { ShaderManager::Program::Decal, "track.png"s });
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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // TODO: will need to sort track pieces back-to-front for proper alpha blending, is this even a good idea?
    // there are only 3 days left and there isn't even any gameplay code yet

#if 0
    m_camera->setEye(glm::vec3(0, 0, 5));
    m_camera->setCenter(glm::vec3(0, 0, 0));
    m_camera->setUp(glm::vec3(0, 1, 0));

    const auto angle = 0.5f * m_trackTime;
    const auto modelMatrix = glm::rotate(glm::mat4(1), angle, glm::vec3(0, 1, 0));
#else
    const auto distance = 0.25f * m_trackTime;
    const auto state = pathStateAt(distance);
    const auto &center = state.center;
    const auto dir = state.orientation[2];
    const auto up = state.orientation[0];

    m_camera->setEye(center + 0.3f * up - 0.2f * dir);
    m_camera->setCenter(center);
    m_camera->setUp(up);

    const auto modelMatrix = glm::mat4(1);
#endif

    m_renderer->begin();
    for (const auto &mesh : m_trackSegments) {
        m_renderer->render(mesh.get(), trackMaterial(), modelMatrix);
    }
    m_renderer->end();
}

World::PathState World::pathStateAt(float distance) const
{
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

    const auto q0 = glm::quat_cast(curPart.orientation);
    const auto q1 = glm::quat_cast(nextPart.orientation);
    const auto q = glm::mix(q0, q1, t);
    const auto orientation = glm::mat3_cast(q);

    return { orientation, center };
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

struct MeshVertex {
    glm::vec3 position;
    glm::vec2 texcoord;
};

static std::unique_ptr<Mesh> makeLineMesh(const std::vector<MeshVertex> &vertices)
{
    static const std::vector<Mesh::VertexAttribute> attributes = {
        { 3, GL_FLOAT, offsetof(MeshVertex, position) },
        { 2, GL_FLOAT, offsetof(MeshVertex, texcoord) },
    };

    auto mesh = std::make_unique<Mesh>(GL_TRIANGLE_STRIP);
    mesh->setVertexCount(vertices.size());
    mesh->setVertexSize(sizeof(MeshVertex));
    mesh->setVertexAttributes(attributes);

    mesh->initialize();
    mesh->setVertexData(vertices.data());

    return mesh;
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

        constexpr auto PartsPerSegment = 20;
        for (size_t j = 0; j < PartsPerSegment; ++j) {
            const auto t = static_cast<float>(j) / PartsPerSegment;

            const auto center = path.eval(t);

            if (prevCenter != std::nullopt)
                distance += glm::distance(*prevCenter, center);

            const auto dir = glm::normalize(path.direction(t));
            const auto side = glm::normalize(glm::cross(dir, currentUp));
            const auto up = glm::normalize(glm::cross(side, dir));

            const auto orientation = glm::mat3(up, side, dir);

            m_pathParts.push_back(PathPart { orientation, center, distance });

            currentUp = up;
            prevCenter = center;
        }
    }

    constexpr auto VertsPerSegment = 20;
    constexpr auto TrackWidth = 0.1f;

    for (size_t i = 0, size = m_pathParts.size(); i < size; i += VertsPerSegment) {
        std::vector<MeshVertex> vertices;
        for (size_t j = i, end = std::min(size - 1, i + VertsPerSegment); j <= end; ++j) {
            const auto &part = m_pathParts[j];
            const auto texU = 6.0f * part.distance;
            vertices.push_back({ part.center - part.side() * TrackWidth, glm::vec2(0.0f, texU) });
            vertices.push_back({ part.center + part.side() * TrackWidth, glm::vec2(2.0f, texU) });
        }
        auto mesh = makeLineMesh(vertices);
        m_trackSegments.push_back(std::move(mesh));
    }

    spdlog::info("Initialized track, length={} segments={} parts={}",
                 distance, m_trackSegments.size(), m_pathParts.size());
}
