#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <vector>

class Camera;
class Mesh;
class ShaderManager;

class ParticleSystem
{
public:
    explicit ParticleSystem(ShaderManager *shaderManager, const Camera *camera);
    ~ParticleSystem();

    void update(float elapsed);
    void render(const glm::mat4 &worldMatrix) const;

    void spawnParticle(const glm::vec3 &position, const glm::vec3 &velocity, const glm::vec2 &size, float lifetime);

private:
    void initializeMesh();

    struct Particle {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec2 size;
        float time;
        float life;
    };

    ShaderManager *m_shaderManager;
    const Camera *m_camera;
    std::vector<Particle> m_particles;
    std::unique_ptr<Mesh> m_mesh;
};
