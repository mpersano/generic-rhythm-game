#include "world.h"

#include "bezier.h"
#include "camera.h"
#include "hudpainter.h"
#include "material.h"
#include "mesh.h"
#include "meshutils.h"
#include "oggplayer.h"
#include "renderer.h"
#include "shadermanager.h"
#include "track.h"
#include "tween.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/string_cast.hpp>
#include <spdlog/spdlog.h>

using namespace std::string_literals;

namespace {

const Material *trackMaterial()
{
    static const Material material { ShaderManager::Program::Lighting, Material::Transparent, cachedTexture("track.png"s) };
    return &material;
}

const Material *beatMaterial(int index)
{
    static const std::vector<Material> materials = {
        { ShaderManager::Program::LightingFog, Material::None, cachedTexture("beat0.png"s) },
        { ShaderManager::Program::LightingFog, Material::None, cachedTexture("beat1.png"s) },
        { ShaderManager::Program::LightingFog, Material::None, cachedTexture("beat2.png"s) },
        { ShaderManager::Program::LightingFog, Material::None, cachedTexture("beat3.png"s) },
    };
    assert(index >= 0 && index < materials.size());
    return &materials[index];
}

const Material *longNoteMaterial(int index)
{
    static const std::vector<Material> materials = {
        { ShaderManager::Program::LightingFogBlend, Material::None, cachedTexture("beat0.png"s) },
        { ShaderManager::Program::LightingFogBlend, Material::None, cachedTexture("beat1.png"s) },
        { ShaderManager::Program::LightingFogBlend, Material::None, cachedTexture("beat2.png"s) },
        { ShaderManager::Program::LightingFogBlend, Material::None, cachedTexture("beat3.png"s) },
    };
    assert(index >= 0 && index < materials.size());
    return &materials[index];
}

const Material *debrisMaterial(int index)
{
    static const std::vector<Material> materials = {
        { ShaderManager::Program::Lighting, Material::Transparent, cachedTexture("debris0.png"s) },
        { ShaderManager::Program::Lighting, Material::Transparent, cachedTexture("debris1.png"s) },
        { ShaderManager::Program::Lighting, Material::Transparent, cachedTexture("debris2.png"s) },
        { ShaderManager::Program::Lighting, Material::Transparent, cachedTexture("debris3.png"s) },
    };
    assert(index >= 0 && index < materials.size());
    return &materials[index];
}

const Material *buttonMaterial(int index)
{
    static const std::vector<Material> materials = {
        { ShaderManager::Program::Decal, Material::Transparent, cachedTexture("button0.png"s) },
        { ShaderManager::Program::Decal, Material::Transparent, cachedTexture("button1.png"s) },
        { ShaderManager::Program::Decal, Material::Transparent, cachedTexture("button2.png"s) },
        { ShaderManager::Program::Decal, Material::Transparent, cachedTexture("button3.png"s) },
    };
    assert(index >= 0 && index < materials.size());
    return &materials[index];
}

const Material *buttonMaterial()
{
    static const Material material { ShaderManager::Program::Lighting, Material::Transparent, cachedTexture("button0.png"s) };
    return &material;
}

const Material *particleMaterial()
{
    static const Material material { ShaderManager::Program::Billboard, Material::AdditiveBlend, cachedTexture("star.png"s) };
    return &material;
}

const Material *debugMaterial()
{
    static const Material material { ShaderManager::Program::Debug, Material::None, nullptr };
    return &material;
}

HUDPainter::Font fontRegular(int pixelHeight)
{
    return { "assets/fonts/OpenSans_Regular.ttf"s, pixelHeight };
}

HUDPainter::Font font(int pixelHeight)
{
    return { "assets/fonts/OpenSans-ExtraBold.ttf"s, pixelHeight };
}

std::string meshPath(const std::string &basename)
{
    return std::string("assets/meshes/") + basename;
}

constexpr auto Speed = 0.5f;
constexpr auto TrackWidth = 0.25f;
constexpr auto HitWindow = 0.2f;

struct ParticleState {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec2 size;
    float alpha;
};

constexpr auto MaxParticles = 200;

} // namespace

class AbstractAnimation
{
public:
    virtual ~AbstractAnimation() = default;
    virtual bool update(float elapsed) = 0;
};

struct IdleAnimation : public AbstractAnimation {
public:
    IdleAnimation(float duration)
        : m_duration(duration)
    {
    }

    bool update(float elapsed)
    {
        m_time += elapsed;
        return m_time < m_duration;
    }

private:
    float m_time = 0.0f;
    float m_duration;
};

template<typename T, typename TweenerT>
struct PropertyAnimation : public AbstractAnimation {
public:
    PropertyAnimation(T *property, T startValue, T endValue, float duration)
        : m_property(property)
        , m_startValue(startValue)
        , m_endValue(endValue)
        , m_duration(duration)
    {
    }

    bool update(float elapsed) override
    {
        m_time += elapsed;
        if (m_time > m_duration)
            return false;
        const float t = m_time / m_duration;
        *m_property = tween<TweenerT>(m_startValue, m_endValue, t);
        return true;
    }

private:
    T *m_property;
    T m_startValue;
    T m_endValue;
    float m_time = 0.0f;
    float m_duration;
};

template<template<class> class TweenerT>
using FloatAnimation = PropertyAnimation<float, TweenerT<float>>;

template<template<class> class TweenerT>
using Vec2Animation = PropertyAnimation<glm::vec2, TweenerT<float>>;

struct AnimationGroup : public AbstractAnimation {
public:
    void addAnimation(std::unique_ptr<AbstractAnimation> animation)
    {
        m_animations.push_back(std::move(animation));
    }

    void addIdleAnimation(float duration)
    {
        m_animations.emplace_back(new IdleAnimation(duration));
    }

    template<template<class> class TweenerT>
    void addFloatAnimation(float *property, float startValue, float endValue, float duration)
    {
        m_animations.emplace_back(new FloatAnimation<TweenerT>(property, startValue, endValue, duration));
    }

    template<template<class> class TweenerT>
    void addVec2Animation(glm::vec2 *property, const glm::vec2 &startValue, const glm::vec2 &endValue, float duration)
    {
        m_animations.emplace_back(new Vec2Animation<TweenerT>(property, startValue, endValue, duration));
    }

protected:
    std::vector<std::unique_ptr<AbstractAnimation>> m_animations;
};

struct ParallelAnimation : public AnimationGroup {
public:
    bool update(float elapsed) override
    {
        bool result = false;
        auto it = m_animations.begin();
        while (it != m_animations.end()) {
            if ((*it)->update(elapsed)) {
                ++it;
                result = true;
            } else {
                it = m_animations.erase(it);
            }
        }
        return result;
    }
};

struct SequentialAnimation : public AnimationGroup {
public:
    bool update(float elapsed) override
    {
        if (!m_animations.empty()) {
            auto &first = m_animations.front();
            if (!first->update(elapsed)) {
                m_animations.erase(m_animations.begin());
            }
        }
        return !m_animations.empty();
    }
};

class HUDAnimation
{
public:
    virtual ~HUDAnimation() = default;

    virtual bool update(float elapsed) = 0;
    virtual void render(HUDPainter *hudPainter) = 0;
};

class HitAnimation : public HUDAnimation
{
public:
    HitAnimation(float x, float y, const std::u32string &text);
    ~HitAnimation() = default;

    bool update(float elapsed) override;
    void render(HUDPainter *hudPainter) override;

private:
    glm::vec2 m_center;
    glm::vec2 m_scale;
    float m_alpha;
    std::u32string m_text;
    std::unique_ptr<ParallelAnimation> m_animation;
};

HitAnimation::HitAnimation(float x, float y, const std::u32string &text)
    : m_center(x, y)
    , m_text(text)
    , m_animation(new ParallelAnimation)
{
    auto scaleAnimation = std::make_unique<SequentialAnimation>();
    scaleAnimation->addVec2Animation<Tweeners::OutBounce>(&m_scale, glm::vec2(0), glm::vec2(1), 1.0f);
    scaleAnimation->addIdleAnimation(0.25f);
    scaleAnimation->addVec2Animation<Tweeners::Linear>(&m_scale, glm::vec2(1), glm::vec2(3, 0), 0.5f);
    m_animation->addAnimation(std::move(scaleAnimation));

    auto alphaAnimation = std::make_unique<SequentialAnimation>();
    alphaAnimation->addFloatAnimation<Tweeners::Linear>(&m_alpha, 0.0f, 0.75f, 0.5f);
    alphaAnimation->addIdleAnimation(0.75f);
    alphaAnimation->addFloatAnimation<Tweeners::Linear>(&m_alpha, 0.75, 0.0f, 0.5f);
    m_animation->addAnimation(std::move(alphaAnimation));
}

void HitAnimation::render(HUDPainter *hudPainter)
{
    const HUDPainter::Gradient gradient = {
        { 0, 0 }, { 1, 0 }, { 1, 1, 1, m_alpha }, { 1, 0, 0, m_alpha }
    };
    hudPainter->resetTransform();
    hudPainter->setFont(font(80));

    hudPainter->translate(m_center);
    hudPainter->scale(m_scale);

    hudPainter->drawText(0, 0, gradient, 0, m_text);
}

bool HitAnimation::update(float elapsed)
{
    return m_animation->update(elapsed);
}

class ComboCounter
{
public:
    int count() const { return m_count; }

    void clear()
    {
        m_count = 0;
        m_alpha = 0;
    }

    void increment()
    {
        ++m_count;
        m_scaleDelta = 1.0f;
        updateText();
        m_alpha = 1;
    }

    void update(float elapsed)
    {
        m_scale = m_scale + elapsed * m_scaleDelta;
        m_scaleDelta -= 2.0 * elapsed;

        if (m_scale > MaxScale) {
            m_scale = MaxScale;
            m_scaleDelta = 0.0f;
        }

        if (m_scale < MinScale) {
            m_scale = MinScale;
            m_scaleDelta = 0.0f;
        }

        m_currentAlpha = glm::mix(m_currentAlpha, m_alpha, 0.15f);
    }

    void render(HUDPainter *hudPainter) const
    {
        hudPainter->resetTransform();

        hudPainter->translate(glm::vec2(-400, 0));
        hudPainter->scale(glm::vec2(m_scale, m_currentAlpha * m_scale));

        const auto s = 0.25f * ((m_scale - MinScale) / (MaxScale - MinScale));

        hudPainter->setFont(font(80));
        const HUDPainter::Gradient gradientTop = {
            { 0, 1 }, { 0, 0 }, { 1, 1, 1, m_currentAlpha }, { 1, s, s, m_currentAlpha }
        };
        hudPainter->drawText(0, -50, gradientTop, 0, U"COMBO"s);

        hudPainter->setFont(font(200));
        const HUDPainter::Gradient gradientBottom = {
            { 0, 0 }, { 0, 1 }, { 1, 1, 1, m_currentAlpha }, { 1, s, s, m_currentAlpha }
        };
        hudPainter->drawText(0, 60, gradientBottom, 0, m_text);
    }

private:
    void updateText()
    {
        // why no std::to_string for u32string?!
        m_text.clear();
        int value = m_count;
        while (value) {
            const int digit = value % 10;
            m_text.insert(m_text.begin(), U'0' + digit);
            value /= 10;
        }
    }

    static constexpr auto MaxScale = 1.25f;
    static constexpr auto MinScale = 1.0f;

    float m_scale = MinScale;
    float m_scaleDelta = 0.0f;
    std::u32string m_text;
    int m_count = 0;
    float m_alpha = 0;
    float m_currentAlpha = 0;
};

World::World(ShaderManager *shaderManager)
    : m_shaderManager(shaderManager)
    , m_camera(new Camera)
    , m_renderer(new Renderer(m_shaderManager, m_camera.get()))
    , m_comboCounter(new ComboCounter)
    , m_player(new OggPlayer)
{
    initializeBeatMeshes();
    initializeMarkerMesh();
    initializeButtonMesh();
    initializeTrackMesh();
    initializeParticleMesh();
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
    m_player->update();
    m_trackTime += elapsed;
    updateCamera(false);
    updateBeats(inputState);
    updateDebris(elapsed);
    updateParticles(elapsed);
    updateTextAnimations(elapsed);
    m_comboCounter->update(elapsed);
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

    // clip plane (so we can clip long notes that get behind the marker)
    const auto planePosition = state.center;
    const auto planeNormal = glm::normalize(state.direction());
    m_clipPlane = glm::vec4(planeNormal, -glm::dot(planeNormal, planePosition));
}

void World::updateBeats(InputState inputState)
{
    const auto pressed = [this, inputState](InputState key) {
        return ((inputState & key) != InputState::None) && ((m_prevInputState & key) == InputState::None);
    };
    const auto released = [this, inputState](InputState key) {
        return ((inputState & key) == InputState::None) && ((m_prevInputState & key) != InputState::None);
    };
    constexpr std::array trackInputs { InputState::Fire1, InputState::Fire2, InputState::Fire3, InputState::Fire4 };

    const auto textPosition = [this](int track) {
        const auto Width = 400.0;
        return -.5 * Width + track * Width / (m_track->eventTracks - 1);
    };

    for (auto &beat : m_beats) {
        bool hit = false, miss = false, spawnDebris = false;
        float hitDeltaT = 0.0f;

        switch (beat->state) {
        case Beat::State::Active: {
            if (pressed(trackInputs[beat->track])) {
                // hit start of beat?
                hitDeltaT = std::abs(beat->start - m_trackTime);
                if (hitDeltaT < HitWindow) {
                    hit = true;
                    if (beat->type == Beat::Type::Tap) {
                        beat->state = Beat::State::Inactive;
                        spawnDebris = true;
                    } else {
                        beat->state = Beat::State::Holding;
                    }
                }
            } else {
                // missed start of beat?
                if (beat->start < m_trackTime - HitWindow) {
                    miss = true;
                    if (beat->type == Beat::Type::Tap) {
                        beat->state = Beat::State::Inactive;
                    } else {
                        beat->state = Beat::State::HoldMissed;
                    }
                }
            }
            break;
        }

        case Beat::State::Holding: {
            if (released(trackInputs[beat->track])) {
                // released on end of beat?
                hitDeltaT = std::abs(beat->start + beat->duration - m_trackTime);
                if (hitDeltaT < HitWindow) {
                    hit = true;
                    beat->state = Beat::State::Inactive;
                } else {
                    // released too early
                    miss = true;
                    beat->state = Beat::State::HoldMissed;
                }
            } else {
                // missed end of beat?
                if (beat->start + beat->duration < m_trackTime - HitWindow) {
                    beat->state = Beat::State::Inactive;
                    miss = true;
                }
            }
            break;
        }

        case Beat::State::HoldMissed: {
            if (beat->start + beat->duration < m_trackTime) {
                beat->state = Beat::State::Inactive;
            }
            break;
        }

        case Beat::State::Inactive:
            break;
        }

        if (hit) {
            m_comboCounter->increment();
            const float score = hitDeltaT / HitWindow;
            std::u32string animationText;
            if (score < 0.25) {
                m_hudAnimations.emplace_back(new HitAnimation(textPosition(beat->track), -50, U"PERFECT!"s));
            } else {
                m_hudAnimations.emplace_back(new HitAnimation(textPosition(beat->track), -50, U"GOOD"s));
            }
        }

        if (miss) {
            m_comboCounter->clear();
            m_hudAnimations.emplace_back(new HitAnimation(textPosition(beat->track), 200, U"MISSED"s));
        }

        if (spawnDebris) {
            assert(beat->type == Beat::Type::Tap);
            // FIXME can't just get the submatrix for rotation etc, transform is scaled!
            // it's 4:00 AM right now but fix me later
#if 0
            const glm::vec3 position = glm::vec3(beat->transform[3]);
            const glm::mat3 rotation = glm::mat3(beat->transform);
            const glm::vec3 velocity = 0.1f * glm::vec3(beat->transform[1]);
#endif
            glm::vec3 scale;
            glm::quat rotation;
            glm::vec3 translation;
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(beat->transform, scale, rotation, translation, skew, perspective);
            const auto rotationMatrix = glm::mat3_cast(rotation);
            const glm::vec3 velocity = 0.5f * glm::vec3(rotationMatrix[0]);

            // rotation axis any random vector orthogonal to direction
            glm::vec3 u = glm::ballRand(1.0f);
            glm::vec3 rotationAxis = glm::cross(u, rotationMatrix[0]);
            float angularSpeed = glm::linearRand(5.0f, 10.0f);

            m_debris.push_back(Debris { beat->track, translation, rotationMatrix, scale, velocity, rotationAxis, angularSpeed, 0, 3 });
        }
    }

#if 0
    if (pressed(InputState::Fire1)) {
        m_comboCounter->increment();
    }

    if (pressed(InputState::Fire2)) {
        m_comboCounter->clear();
    }
#endif

    m_prevInputState = inputState;
}

void World::updateDebris(float elapsed)
{
    auto it = m_debris.begin();
    while (it != m_debris.end()) {
        auto &debris = *it;
        debris.time += elapsed;
        if (debris.time >= debris.life) {
            it = m_debris.erase(it);
        } else {
            debris.position += elapsed * debris.velocity;
            auto rotation = glm::rotate(glm::mat4(1), elapsed * debris.angularSpeed, debris.rotationAxis);
            debris.orientation *= glm::mat3(rotation);
            ++it;
        }
    }
}

void World::updateParticles(float elapsed)
{
    auto it = m_particles.begin();
    while (it != m_particles.end()) {
        auto &particle = *it;
        particle.time += elapsed;
        if (particle.time >= particle.life) {
            it = m_particles.erase(it);
        } else {
            particle.position += elapsed * particle.velocity;
            ++it;
        }
    }
    if (m_particles.size() < MaxParticles) {
        constexpr auto UsableTrackWidth = static_cast<float>(720) * TrackWidth / 800;
        const auto laneWidth = UsableTrackWidth / m_track->eventTracks;
        const auto radius = 0.5f * laneWidth;

        for (int i = 0; i < m_track->eventTracks; ++i) {
            if (static_cast<unsigned>(m_prevInputState) & (1 << i)) {
                for (int j = 0; j < 5; ++j) {
                    const auto laneX = -0.5f * UsableTrackWidth + (i + 0.5f) * laneWidth;
                    glm::vec3 p = glm::vec3(0.0, glm::vec2(laneX, 0) + glm::diskRand(radius));
                    const glm::vec3 o = glm::vec3(-.2, laneX, 0);
                    Particle particle;
                    particle.position = o + glm::linearRand(0.1f, .15f) * glm::normalize(p - o);
                    particle.velocity = glm::linearRand(0.1f, .15f) * glm::normalize(p - o);
                    particle.size = glm::vec2(.002, .05);
                    particle.time = 0;
                    particle.life = 1;
                    m_particles.push_back(particle);
                }
            }
        }
    }
}

void World::updateTextAnimations(float elapsed)
{
    auto it = m_hudAnimations.begin();
    while (it != m_hudAnimations.end()) {
        if ((*it)->update(elapsed)) {
            ++it;
        } else {
            it = m_hudAnimations.erase(it);
        }
    }
}

void World::render() const
{
#if 0
    m_camera->setEye(glm::vec3(0, 0, 15));
    m_camera->setCenter(glm::vec3(0, 0, 0));
    m_camera->setUp(glm::vec3(0, 1, 0));

    const auto angle = 0.5f * m_trackTime;
    const auto modelMatrix = glm::rotate(glm::mat4(1), angle, glm::vec3(0, 1, 0));
#else
    const auto modelMatrix = glm::mat4(1);
#endif

    m_shaderManager->clearCurrentProgram();

    m_shaderManager->useProgram(ShaderManager::DecalFog);
    m_shaderManager->setUniform(ShaderManager::Eye, m_camera->eye());
    m_shaderManager->setUniform(ShaderManager::FogColor, glm::vec4(0, 0, 0, 1));
    m_shaderManager->setUniform(ShaderManager::FogDistance, glm::vec2(.1, 5.));

    m_shaderManager->useProgram(ShaderManager::LightingFog);
    m_shaderManager->setUniform(ShaderManager::LightPosition, glm::vec3(0, 10, -10));
    m_shaderManager->setUniform(ShaderManager::Eye, m_camera->eye());
    m_shaderManager->setUniform(ShaderManager::FogColor, glm::vec4(0, 0, 0, 1));
    m_shaderManager->setUniform(ShaderManager::FogDistance, glm::vec2(.1, 5.));

    m_shaderManager->useProgram(ShaderManager::LightingFogClip);
    m_shaderManager->setUniform(ShaderManager::LightPosition, glm::vec3(0, 10, -10));
    m_shaderManager->setUniform(ShaderManager::Eye, m_camera->eye());
    m_shaderManager->setUniform(ShaderManager::FogColor, glm::vec4(0, 0, 0, 1));
    m_shaderManager->setUniform(ShaderManager::FogDistance, glm::vec2(.1, 5.));
    m_shaderManager->setUniform(ShaderManager::ClipPlane, m_clipPlane);

    m_shaderManager->useProgram(ShaderManager::LightingFogBlend);
    m_shaderManager->setUniform(ShaderManager::LightPosition, glm::vec3(0, 10, -10));
    m_shaderManager->setUniform(ShaderManager::Eye, m_camera->eye());
    m_shaderManager->setUniform(ShaderManager::FogColor, glm::vec4(0, 0, 0, 1));
    m_shaderManager->setUniform(ShaderManager::FogDistance, glm::vec2(.1, 5.));

    m_shaderManager->useProgram(ShaderManager::Billboard);
    m_shaderManager->setUniform(ShaderManager::Eye, m_camera->eye());

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

    // sigh.... special case: long notes
    // our renderer sucks

    bool renderedLongNote = false;

    for (const auto &beat : m_beats) {
        if (beat->type == Beat::Type::Hold) {
            if (beat->state == Beat::State::Holding) {
                float t = m_trackTime - beat->start;
                float alpha = 0.5f + 0.5f * sin(5.0f * t);
                m_shaderManager->useProgram(ShaderManager::LightingFogBlend);
                m_shaderManager->setUniform(ShaderManager::BlendColor, glm::vec4(1, 1, 1, alpha));
                m_renderer->begin();
                m_renderer->render(beat->mesh.get(), longNoteMaterial(beat->track), glm::mat4(1));
                m_renderer->end();
            } else if (beat->state == Beat::State::HoldMissed) {
                m_shaderManager->useProgram(ShaderManager::LightingFogBlend);
                m_shaderManager->setUniform(ShaderManager::BlendColor, glm::vec4(.5, .5, .5, 0.75));
                m_renderer->begin();
                m_renderer->render(beat->mesh.get(), longNoteMaterial(beat->track), glm::mat4(1));
                m_renderer->end();
            }
        }
    }

    // now render everything

    m_renderer->begin();

    for (const auto &segment : trackSegments) {
        m_renderer->render(std::get<1>(segment), trackMaterial(), modelMatrix);
    }
    for (const auto &beat : m_beats) {
        if (beat->state == Beat::State::Inactive)
            continue;
        if (beat->type == Beat::Type::Tap) {
            m_renderer->render(m_beatMesh.get(), beatMaterial(beat->track), beat->transform);
        } else {
            assert(beat->mesh);
            if (beat->state != Beat::State::Holding && beat->state != Beat::State::HoldMissed) {
                m_renderer->render(beat->mesh.get(), beatMaterial(beat->track), glm::mat4(1));
            }
        }
    }
    for (const auto &debris : m_debris) {
        const auto rotate = glm::mat4(debris.orientation);
        const auto translate = glm::translate(glm::mat4(1), debris.position);
        const auto scale = glm::scale(glm::mat4(1), debris.scale);
        const auto transform = translate * scale * rotate;
        m_renderer->render(m_beatMesh.get(), debrisMaterial(debris.track), transform);
    }

#if 0
        m_renderer->render(m_markerMesh.get(), debugMaterial(), m_markerTransform);
#endif
    {
        constexpr auto UsableTrackWidth = static_cast<float>(720) * TrackWidth / 800;
        const auto laneWidth = UsableTrackWidth / m_track->eventTracks;
        const auto radius = 0.5f * laneWidth;

        for (int i = 0; i < m_track->eventTracks; ++i) {
            const auto laneX = -0.5f * UsableTrackWidth + (i + 0.5f) * laneWidth;
            const float height = (static_cast<unsigned>(m_prevInputState) & (1 << i)) ? 0.0f : 0.01f;
            const auto translate = glm::translate(glm::mat4(1), glm::vec3(height, laneX, 0));
            const auto scale = glm::scale(glm::mat4(1), glm::vec3(0.4f * laneWidth));
            const auto transform = m_markerTransform * translate * scale;
            m_renderer->render(m_buttonMesh.get(), buttonMaterial(i), transform);
        }
    }

    if (!m_particles.empty()) {
        std::vector<ParticleState> particleData;
        std::transform(m_particles.begin(), m_particles.end(), std::back_inserter(particleData), [](const Particle &particle) -> ParticleState {
            const float t = particle.time / particle.life;
            float alpha = .25f * Tweeners::InOutQuadratic<float>()(t);
            return { particle.position, particle.velocity, particle.size, alpha };
        });
        m_particleMesh->setVertexCount(m_particles.size());
        m_particleMesh->setVertexData(particleData.data());
        m_renderer->render(m_particleMesh.get(), particleMaterial(), m_markerTransform);
    }

    m_renderer->end();
}

static std::u32string timeToString(float t)
{
    const auto seconds = static_cast<int>(t);

    std::u32string result;

    // aaaaaaaaaaaaaaaaaaaaaaaaaaaa

    const auto formatNumber = [&result](int value) {
        // aaaaaaaaaaaaaaaaaaaaaaaaaaaa
        result.push_back((value / 10) + U'0');
        result.push_back((value % 10) + U'0');
    };

    formatNumber(seconds / 60);
    result.push_back(U':');
    formatNumber(seconds % 60);

    return result;
}

void World::renderHUD(HUDPainter *hudPainter) const
{
    hudPainter->setFont(fontRegular(40));
    hudPainter->drawText(-580, -260, glm::vec4(1), 0, U"Galaxies (feat. Diandra Faye)", HUDPainter::Alignment::Left);
    hudPainter->setFont(fontRegular(30));
    hudPainter->drawText(-580, -230, glm::vec4(1), 0, U"Jens East", HUDPainter::Alignment::Left);

    hudPainter->drawText(-580, -200, glm::vec4(1), 0,
            timeToString(m_trackTime)
            + U" / "s + timeToString(m_player->sampleCount() /  m_player->sampleRate())
            ,


            HUDPainter::Alignment::Left);

    for (auto &animation : m_hudAnimations)
        animation->render(hudPainter);
    m_comboCounter->render(hudPainter);
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
        std::vector<MeshVertex> vertices;
        for (size_t j = i, end = std::min(size - 1, i + VertsPerSegment); j <= end; ++j) {
            const auto &part = m_pathParts[j];
            const auto texU = 3.0f * part.distance;
            vertices.push_back({ part.state.center - part.state.side() * 0.5f * TrackWidth, glm::vec2(0.0f, texU), part.state.up() });
            vertices.push_back({ part.state.center + part.state.side() * 0.5f * TrackWidth, glm::vec2(1.0f, texU), part.state.up() });
        }
        auto mesh = makeMesh(vertices, GL_TRIANGLE_STRIP);

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

void World::initializeParticleMesh()
{
    m_particleMesh = std::make_unique<Mesh>(GL_POINTS);

    static const std::vector<Mesh::VertexAttribute> attributes = {
        { 3, GL_FLOAT, offsetof(ParticleState, position) },
        { 3, GL_FLOAT, offsetof(ParticleState, velocity) },
        { 2, GL_FLOAT, offsetof(ParticleState, size) },
        { 1, GL_FLOAT, offsetof(ParticleState, alpha) }
    };

    m_particleMesh->setVertexCount(MaxParticles);
    m_particleMesh->setVertexSize(sizeof(ParticleState));
    m_particleMesh->setVertexAttributes(attributes);
    m_particleMesh->initialize();
}

void World::initializeBeatMeshes()
{
    m_beatMesh = loadMesh(meshPath("beat.obj"));
}

static std::unique_ptr<Mesh> makeDebugMesh(const std::vector<glm::vec3> &vertices, GLenum primitive = GL_TRIANGLE_STRIP)
{
    static const std::vector<Mesh::VertexAttribute> attributes = {
        { 3, GL_FLOAT, 0 }
    };

    auto mesh = std::make_unique<Mesh>(primitive);
    mesh->setVertexCount(vertices.size());
    mesh->setVertexSize(sizeof(glm::vec3));
    mesh->setVertexAttributes(attributes);

    mesh->initialize();
    mesh->setVertexData(vertices.data());

    return mesh;
}

void World::initializeMarkerMesh()
{
    constexpr auto Left = -0.5f * TrackWidth;
    constexpr auto Right = 0.5f * TrackWidth;

    constexpr auto Thick = 0.0125f;
    constexpr auto Height = 0.01f;

    constexpr auto Bottom = -0.5f * Thick;
    constexpr auto Top = 0.5f * Thick;

    static const std::vector<glm::vec3> vertices = {
        { Height, Left, Bottom },
        { Height, Right, Bottom },
        { Height, Left, Top },
        { Height, Right, Top },
    };

    m_markerMesh = makeDebugMesh(vertices);
}

void World::initializeButtonMesh()
{
    static const std::vector<MeshVertex> vertices = {
        { { 0, -1, -1 }, { 0, 0 }, { 1, 0, 0 } },
        { { 0, 1, -1 }, { 1, 0 }, { 1, 0, 0 } },
        { { 0, -1, 1 }, { 0, 1 }, { 1, 0, 0 } },
        { { 0, 1, 1 }, { 1, 1 }, { 1, 0, 0 } },
    };
    m_buttonMesh = makeMesh(vertices, GL_TRIANGLE_STRIP);
}

void World::initializeLevel(const Track *track)
{
    m_track = track;

    m_beats.clear();
    const auto &events = track->events;
    std::transform(events.begin(), events.end(), std::back_inserter(m_beats),
                   [this, track](const Track::Event &event) -> std::unique_ptr<Beat> {
                       const auto distance = Speed * event.start;

                       const auto pathState = pathStateAt(distance);

                       constexpr auto UsableTrackWidth = static_cast<float>(720) * TrackWidth / 800;
                       const auto laneWidth = UsableTrackWidth / track->eventTracks;
                       const auto laneX = -0.5f * UsableTrackWidth + (event.track + 0.5f) * laneWidth;

                       auto beat = std::make_unique<Beat>();

                       beat->type = static_cast<Beat::Type>(event.type);
                       beat->start = event.start;
                       beat->duration = event.duration;
                       beat->track = event.track;

                       if (event.type == Track::Event::Type::Tap) {
                           const auto translate = glm::translate(glm::mat4(1), glm::vec3(0, laneX, 0));
                           const auto scale = glm::scale(glm::mat4(1), glm::vec3(0.4f * laneWidth));
                           beat->transform = pathState.transformMatrix() * translate * scale;
                       } else {
                           beat->transform = glm::mat4(1);

                           const auto from = Speed * event.start;
                           const auto to = Speed * (event.start + event.duration);

                           constexpr auto Height = 0.01f;
                           constexpr auto BevelFraction = 0.3f;

                           const auto radius = 0.4f * laneWidth;
                           const auto smallRadius = (1.0f - BevelFraction) * radius;

                           std::vector<MeshVertex> vertices;

                           const auto addCap = [this, &vertices, radius, smallRadius, laneX](float distance) {
                               const auto state = pathStateAt(distance);
                               const auto transform = state.transformMatrix();

                               const auto n = state.up();

                               const std::vector<glm::vec2> localVertices = {
                                   { -smallRadius, -radius },
                                   { smallRadius, -radius },
                                   { -radius, -smallRadius },
                                   { radius, -smallRadius },
                                   { -radius, smallRadius },
                                   { radius, smallRadius },
                                   { -smallRadius, radius },
                                   { smallRadius, radius },
                               };

                               for (const auto &v : localVertices) {
                                   vertices.push_back({ glm::vec3(transform * glm::vec4(Height, v.x + laneX, v.y, 1)), { 0, 0 }, n });
                               }
                           };

                           addCap(from);

                           const auto vLeft = glm::vec4(Height, laneX - smallRadius, 0.0f, 1.0f);
                           const auto vRight = glm::vec4(Height, laneX + smallRadius, 0.0f, 1.0f);

                           const auto addVertices = [this, &vertices, vLeft, vRight](float distance) {
                               const auto state = pathStateAt(distance);
                               const auto transform = state.transformMatrix();
                               const auto n = state.up();
                               vertices.push_back({ glm::vec3(transform * vLeft), { 0, 0 }, n });
                               vertices.push_back({ glm::vec3(transform * vRight), { 0, 0 }, n });
                           };

                           constexpr auto DistanceDelta = 0.1f;
                           for (float d = from + radius; d < to - radius; d += DistanceDelta) {
                               addVertices(d);
                           }

                           addCap(to);

                           spdlog::info("created mesh for long note: {} vertices", vertices.size());

                           beat->mesh = makeMesh(vertices, GL_TRIANGLE_STRIP);
                       }

                       beat->state = Beat::State::Active;

                       return beat;
                   });
    spdlog::info("drawing {} beats", m_beats.size());

    if (m_player->open(track->audioFile))
        m_player->play();
}

glm::mat4 World::PathState::transformMatrix() const
{
    const auto translate = glm::translate(glm::mat4(1), center);
    const auto rotation = glm::mat4(orientation);
    return translate * rotation;
}
