#include "particlesystem.h"

#include "camera.h"
#include "material.h"
#include "mesh.h"
#include "tween.h"

#include <gx/texture.h>

#include <algorithm>

using namespace std::string_literals;

namespace {
struct ParticleState {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec2 size;
    float alpha;
};

constexpr auto MaxParticles = 200;

const GX::GL::Texture *particleTexture()
{
    static const GX::GL::Texture *texture = cachedTexture("star.png"s);
    return texture;
}
} // namespace

ParticleSystem::ParticleSystem(ShaderManager *shaderManager, const Camera *camera)
    : m_shaderManager(shaderManager)
    , m_camera(camera)
{
    initializeMesh();
}

ParticleSystem::~ParticleSystem() = default;

void ParticleSystem::update(float elapsed)
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
}

void ParticleSystem::render(const glm::mat4 &worldMatrix) const
{
    if (m_particles.empty())
        return;

    std::vector<ParticleState> particleData;
    std::transform(m_particles.begin(), m_particles.end(), std::back_inserter(particleData), [](const Particle &particle) -> ParticleState {
        const float t = particle.time / particle.life;
        float alpha = .25f * Tweeners::InOutQuadratic<float>()(t);
        return { particle.position, particle.velocity, particle.size, alpha };
    });
    m_mesh->setVertexCount(m_particles.size());
    m_mesh->setVertexData(particleData.data());

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    glDepthMask(GL_FALSE); // disable writing to depth buffer

    m_shaderManager->useProgram(ShaderManager::Billboard);
    m_shaderManager->setUniform(ShaderManager::ProjectionMatrix, m_camera->projectionMatrix());
    m_shaderManager->setUniform(ShaderManager::ViewMatrix, m_camera->viewMatrix());
    m_shaderManager->setUniform(ShaderManager::ModelMatrix, worldMatrix);
    m_shaderManager->setUniform(ShaderManager::ModelViewMatrix, m_camera->viewMatrix() * worldMatrix);
    m_shaderManager->setUniform(ShaderManager::ModelViewProjection, m_camera->projectionMatrix() * m_camera->viewMatrix() * worldMatrix);
    const auto normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldMatrix)));
    m_shaderManager->setUniform(ShaderManager::NormalMatrix, normalMatrix);

    particleTexture()->bind();

    m_mesh->render();

    glDepthMask(GL_TRUE);
}

void ParticleSystem::spawnParticle(const glm::vec3 &position, const glm::vec3 &velocity, const glm::vec2 &size, float lifetime)
{
    if (m_particles.size() >= MaxParticles)
        return;
    m_particles.push_back({ position, velocity, size, 0, lifetime });
}

void ParticleSystem::initializeMesh()
{
    m_mesh = std::make_unique<Mesh>(GL_POINTS);

    static const std::vector<Mesh::VertexAttribute> attributes = {
        { 3, GL_FLOAT, offsetof(ParticleState, position) },
        { 3, GL_FLOAT, offsetof(ParticleState, velocity) },
        { 2, GL_FLOAT, offsetof(ParticleState, size) },
        { 1, GL_FLOAT, offsetof(ParticleState, alpha) }
    };

    m_mesh->setVertexCount(MaxParticles);
    m_mesh->setVertexSize(sizeof(ParticleState));
    m_mesh->setVertexAttributes(attributes);
    m_mesh->initialize();
}
