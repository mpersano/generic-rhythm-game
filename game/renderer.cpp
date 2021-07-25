#include "renderer.h"

#include <gx/shaderprogram.h>
#include <gx/texture.h>

#include "material.h"
#include "mesh.h"
#include "shadermanager.h"

#include <algorithm>
#include <optional>

Renderer::Renderer(ShaderManager *shaderManager, const Camera *camera)
    : m_shaderManager(shaderManager)
    , m_camera(camera)
{
}

Renderer::~Renderer() = default;

void Renderer::resize(int width, int height)
{
    m_width = width;
    m_height = height;
}

void Renderer::begin()
{
    m_drawCalls.clear();
}

void Renderer::render(const Mesh *mesh, const Material *material, const glm::mat4 &worldMatrix)
{
    m_drawCalls.push_back({ mesh, material, worldMatrix });
}

void Renderer::end()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);

    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const auto &frustum = m_camera->frustum();

    std::sort(m_drawCalls.begin(), m_drawCalls.end(), [](const auto &lhs, const auto &rhs) {
        return std::tie(lhs.material->program, lhs.material->texture) < std::tie(rhs.material->program, rhs.material->texture);
    });

    std::optional<ShaderManager::Program> curProgram;
    const GX::GL::Texture *curTexture = nullptr;

    for (const auto &drawCall : m_drawCalls) {
#if 0
        if (!frustum.contains(drawCall.mesh->boundingBox(), drawCall.worldMatrix)) {
            continue;
        }
#endif
        const auto *material = drawCall.material;
        if (const auto program = material->program; curProgram == std::nullopt || *curProgram != program) {
            m_shaderManager->useProgram(program);
            m_shaderManager->setUniform(ShaderManager::ProjectionMatrix, m_camera->projectionMatrix());
            m_shaderManager->setUniform(ShaderManager::ViewMatrix, m_camera->viewMatrix());
            curProgram = program;
        }
        if (const auto *texture = material->texture; curTexture != texture) {
            texture->bind();
            curTexture = texture;
        }
        m_shaderManager->setUniform(ShaderManager::ModelMatrix, drawCall.worldMatrix);
        const auto normalMatrix = glm::transpose(glm::inverse(glm::mat3(drawCall.worldMatrix)));
        m_shaderManager->setUniform(ShaderManager::NormalMatrix, normalMatrix);
        drawCall.mesh->render();
    }
}
