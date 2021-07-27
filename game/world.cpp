#include "world.h"

#include "bezier.h"
#include "camera.h"
#include "hudpainter.h"
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
    static const Material material { ShaderManager::Program::Decal, Material::Transparent, cachedTexture("track.png"s) };
    return &material;
}

const Material *beatMaterial()
{
    static const Material material { ShaderManager::Program::Decal, Material::None, cachedTexture("beat.png"s) };
    return &material;
}

const Material *debugMaterial()
{
    static const Material material { ShaderManager::Program::Debug, Material::None, nullptr };
    return &material;
}

std::string meshPath(const std::string &basename)
{
    return std::string("assets/meshes/") + basename;
}

constexpr auto Speed = 0.3f;
constexpr auto TrackWidth = 0.25f;
constexpr auto HitWindow = 0.2f;

} // namespace

World::World(ShaderManager *shaderManager)
    : m_shaderManager(shaderManager)
    , m_camera(new Camera)
    , m_renderer(new Renderer(m_shaderManager, m_camera.get()))
{
    initializeBeatMeshes();
    initializeMarkerMesh();
    initializeTrackMesh();
    updateCamera(true);
}

World::~World() = default;

void World::resize(int width, int height)
{
    m_camera->setAspectRatio(static_cast<float>(width) / height);
    m_renderer->resize(width, height);
}

void World::update(InputState inputState, float elapsed)
{
    m_trackTime += elapsed;
    updateCamera(false);
    updateBeats(inputState);
}

void World::updateCamera(bool snapToPosition)
{
    const auto distance = Speed * m_trackTime;
    const auto state = pathStateAt(distance);

    const auto transform = state.transformMatrix();

    constexpr auto EyeOffset = glm::vec4(.15f, 0.f, -.2f, 1.0f);
    const auto wantedPosition = glm::vec3(transform * EyeOffset);

    if (snapToPosition) {
        m_cameraPosition = wantedPosition;
    } else {
        constexpr auto CameraSpringiness = 0.15f;
        m_cameraPosition = glm::mix(m_cameraPosition, wantedPosition, CameraSpringiness);
    }

    // constexpr auto CenterOffset = glm::vec4(0.f, 0.f, .4f, 1.0f);
    constexpr auto CenterOffset = glm::vec4(0.f, 0.f, .2f, 1.0f);
    const auto center = glm::vec3(transform * CenterOffset);

    const auto up = state.up();

    m_camera->setEye(m_cameraPosition);
    m_camera->setCenter(center);
    m_camera->setUp(up);

    m_markerTransform = transform;
}

void World::updateBeats(InputState inputState)
{
    const auto pressed = [this, inputState](InputState key) {
        return ((inputState & key) != InputState::None) && ((m_prevInputState & key) == InputState::None);
    };
    constexpr std::array trackInputs { InputState::Fire1, InputState::Fire2, InputState::Fire3, InputState::Fire4 };

    for (size_t i = 0; i < trackInputs.size(); ++i) {
        if (pressed(trackInputs[i])) {
            for (auto &beat : m_beats) {
                if (beat.state != Beat::State::Active || beat.track != i)
                    continue;
                const auto dt = std::abs(beat.start - m_trackTime);
                if (dt < HitWindow) {
                    spdlog::info("hit track={} dt={}", beat.track, dt);
                    beat.state = Beat::State::Inactive;
                }
            }
        }
    }

    for (auto &beat : m_beats) {
        if (beat.state != Beat::State::Active)
            continue;
        if (beat.start < m_trackTime - HitWindow) {
            spdlog::info("missed track={}", beat.track);
            beat.state = Beat::State::Inactive;
        }
    }

    m_prevInputState = inputState;
}

void World::render() const
{
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

#if 0
    m_camera->setEye(glm::vec3(0, 0, 5));
    m_camera->setCenter(glm::vec3(0, 0, 0));
    m_camera->setUp(glm::vec3(0, 1, 0));

    const auto angle = 0.5f * m_trackTime;
    const auto modelMatrix = glm::rotate(glm::mat4(1), angle, glm::vec3(0, 1, 0));
#else
    const auto modelMatrix = glm::mat4(1);
#endif

    // sort track segments back-to-front for proper transparency

    std::vector<std::tuple<float, const Mesh *>> trackSegments;
    trackSegments.reserve(m_trackSegments.size());

    const auto cameraDir = glm::normalize(m_camera->center() - m_camera->eye());

    std::transform(m_trackSegments.begin(), m_trackSegments.end(), std::back_inserter(trackSegments),
                   [this, &cameraDir](const TrackSegment &segment) {
                       const auto z = glm::dot(segment.position - m_camera->eye(), cameraDir);
                       return std::make_tuple(z, segment.mesh.get());
                   });

    std::sort(trackSegments.begin(), trackSegments.end(), [](const auto &lhs, const auto &rhs) {
        return std::get<0>(lhs) > std::get<0>(rhs);
    });

    m_renderer->begin();
    for (const auto &segment : trackSegments) {
        m_renderer->render(std::get<1>(segment), trackMaterial(), modelMatrix);
    }
    for (const auto &beat : m_beats) {
        if (beat.state == Beat::State::Active)
            m_renderer->render(m_beatMesh.get(), beatMaterial(), beat.transform);
    }
    m_renderer->render(m_markerMesh.get(), debugMaterial(), m_markerTransform);
    m_renderer->end();
}

void World::renderHUD(HUDPainter *hudPainter) const
{
    static const HUDPainter::Gradient gradient = {
        { 0, 0 }, { 1, 0 }, { 1, 1, 1, 1 }, { 1, 0, 0, 1 }
    };
    hudPainter->drawGradientText(glm::vec2(0, 0), gradient, 0, U"hello"s);
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

    constexpr auto VertsPerSegment = 10;

    for (size_t i = 0, size = m_pathParts.size(); i < size; i += VertsPerSegment) {
        std::vector<TrackMeshVertex> vertices;
        for (size_t j = i, end = std::min(size - 1, i + VertsPerSegment); j <= end; ++j) {
            const auto &part = m_pathParts[j];
            const auto texU = 3.0f * part.distance;
            vertices.push_back({ part.state.center - part.state.side() * 0.5f * TrackWidth, glm::vec2(0.0f, texU) });
            vertices.push_back({ part.state.center + part.state.side() * 0.5f * TrackWidth, glm::vec2(1.0f, texU) });
        }
        auto mesh = makeTrackMesh(vertices);

        glm::vec3 position = glm::vec3(0.0);
        for (auto &vertex : vertices) {
            position += vertex.position;
        }
        position *= 1.0f / vertices.size();

        m_trackSegments.push_back({ position, std::move(mesh) });
    }

    spdlog::info("Initialized track, length={} segments={} parts={}",
                 distance, m_trackSegments.size(), m_pathParts.size());
}

void World::initializeBeatMeshes()
{
    m_beatMesh = loadMesh(meshPath("beat.obj"));
}

void World::initializeMarkerMesh()
{
    constexpr auto Left = -0.5f * TrackWidth;
    constexpr auto Right = 0.5f * TrackWidth;

    constexpr auto Thick = 0.025f;
    constexpr auto Height = 0.01f;

    constexpr auto Bottom = -0.5f * Thick;
    constexpr auto Top = 0.5f * Thick;

    static const std::vector<glm::vec3> vertices = {
        { Height, Left, Bottom },
        { Height, Right, Bottom },
        { Height, Left, Top },
        { Height, Right, Top },
    };

    static const std::vector<Mesh::VertexAttribute> attributes = {
        { 3, GL_FLOAT, 0 }
    };

    m_markerMesh = std::make_unique<Mesh>(GL_TRIANGLE_STRIP);
    m_markerMesh->setVertexCount(vertices.size());
    m_markerMesh->setVertexSize(sizeof(glm::vec3));
    m_markerMesh->setVertexAttributes(attributes);

    m_markerMesh->initialize();
    m_markerMesh->setVertexData(vertices.data());
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

                       const auto laneWidth = TrackWidth / track->eventTracks;
                       const auto x = -0.5f * TrackWidth + (event.track + 0.5f) * laneWidth;
                       const auto translate = glm::translate(glm::mat4(1), glm::vec3(0, x, 0));

                       const auto scale = glm::scale(glm::mat4(1), glm::vec3(0.4f * laneWidth));

                       const auto transform = pathState.transformMatrix() * translate * scale;

                       return { event.start, event.track, transform, Beat::State::Active };
                   });
    spdlog::info("drawing {} beats", m_beats.size());
}

glm::mat4 World::PathState::transformMatrix() const
{
    const auto translate = glm::translate(glm::mat4(1), center);
    const auto rotation = glm::mat4(orientation);
    return translate * rotation;
}
