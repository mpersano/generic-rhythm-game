#include "world.h"

#include "bezier.h"
#include "camera.h"
#include "loadmesh.h"
#include "material.h"
#include "mesh.h"
#include "renderer.h"
#include "shadermanager.h"
#include "track.h"

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

const Material *debugMaterial()
{
    return cachedMaterial(MaterialKey { ShaderManager::Program::Debug, {} });
}

std::string meshPath(const std::string &basename)
{
    return std::string("assets/meshes/") + basename;
}

constexpr auto Speed = 0.15f;
constexpr auto TrackWidth = 0.2f;

} // namespace

World::World()
    : m_shaderManager(new ShaderManager)
    , m_camera(new Camera)
    , m_renderer(new Renderer(m_shaderManager.get(), m_camera.get()))
{
    initializeBeatMeshes();
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
    const auto distance = Speed * m_trackTime;
    const auto state = pathStateAt(distance);

    const auto transform = state.transformMatrix();

    constexpr auto EyeOffset = glm::vec4(.3f, 0.f, -.2f, 1.0f);
    const auto eye = glm::vec3(transform * EyeOffset);

    constexpr auto CenterOffset = glm::vec4(0.f, 0.f, .3f, 1.0f);
    const auto center = glm::vec3(transform * CenterOffset);

    const auto up = state.up();

    m_camera->setEye(eye);
    m_camera->setCenter(center);
    m_camera->setUp(up);

    const auto modelMatrix = glm::mat4(1);
#endif

    m_renderer->begin();
    for (const auto &mesh : m_trackSegments) {
        m_renderer->render(mesh.get(), trackMaterial(), modelMatrix);
    }
    for (const auto &beat : m_beats) {
        m_renderer->render(m_beatMesh.get(), debugMaterial(), beat.transform);
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

    const auto center = glm::mix(curPart.state.center, nextPart.state.center, t);

    const auto q0 = glm::quat_cast(curPart.state.orientation);
    const auto q1 = glm::quat_cast(nextPart.state.orientation);
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

struct TrackMeshVertex {
    glm::vec3 position;
    glm::vec2 texcoord;
};

static std::unique_ptr<Mesh> makeTrackMesh(const std::vector<TrackMeshVertex> &vertices)
{
    static const std::vector<Mesh::VertexAttribute> attributes = {
        { 3, GL_FLOAT, offsetof(TrackMeshVertex, position) },
        { 2, GL_FLOAT, offsetof(TrackMeshVertex, texcoord) },
    };

    auto mesh = std::make_unique<Mesh>(GL_TRIANGLE_STRIP);
    mesh->setVertexCount(vertices.size());
    mesh->setVertexSize(sizeof(TrackMeshVertex));
    mesh->setVertexAttributes(attributes);

    mesh->initialize();
    mesh->setVertexData(vertices.data());

    return mesh;
}

void World::initializeTrackMesh()
{
    std::vector<glm::vec3> controlPoints;
    initializeSegment(controlPoints, glm::vec3(-10, 0, 0), glm::vec3(10, 0, 0), 5);

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

            m_pathParts.push_back(PathPart { { orientation, center }, distance });

            currentUp = up;
            prevCenter = center;
        }
    }

    constexpr auto VertsPerSegment = 20;

    for (size_t i = 0, size = m_pathParts.size(); i < size; i += VertsPerSegment) {
        std::vector<TrackMeshVertex> vertices;
        for (size_t j = i, end = std::min(size - 1, i + VertsPerSegment); j <= end; ++j) {
            const auto &part = m_pathParts[j];
            const auto texU = 6.0f * part.distance;
            vertices.push_back({ part.state.center - part.state.side() * 0.5f * TrackWidth, glm::vec2(0.0f, texU) });
            vertices.push_back({ part.state.center + part.state.side() * 0.5f * TrackWidth, glm::vec2(2.0f, texU) });
        }
        auto mesh = makeTrackMesh(vertices);
        m_trackSegments.push_back(std::move(mesh));
    }

    spdlog::info("Initialized track, length={} segments={} parts={}",
                 distance, m_trackSegments.size(), m_pathParts.size());
}

void World::initializeBeatMeshes()
{
    m_beatMesh = loadMesh(meshPath("beat.obj"));
}

void World::initializeLevel(const Track *track)
{
    m_track = track;

    m_beats.clear();
    const auto &events = track->events;
    std::transform(events.begin(), events.end(), std::back_inserter(m_beats),
                   [this, track](const Track::Event &event) -> Beat {
                       const auto distance = Speed * event.start;

                       const auto pathState = pathStateAt(distance);

                       const auto x = -0.5f * TrackWidth + (event.track + 1) * TrackWidth / (track->eventTracks + 1);
                       const auto translate = glm::translate(glm::mat4(1), glm::vec3(0, x, 0));

                       const auto scale = glm::scale(glm::mat4(1), glm::vec3(.01));

                       const auto transform = pathState.transformMatrix() * translate * scale;

                       return { event.start, transform, Beat::State::Active };
                   });
    spdlog::info("drawing {} beats", m_beats.size());
}

glm::mat4 World::PathState::transformMatrix() const
{
    const auto translate = glm::translate(glm::mat4(1), center);
    const auto rotation = glm::mat4(orientation);
    return translate * rotation;
}
