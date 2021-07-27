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

template<typename Iterator>
void Renderer::render(Iterator first, Iterator last) const
{
    std::stable_sort(first, last, [](const auto &lhs, const auto &rhs) {
        return std::tie(lhs.material->program, lhs.material->texture) < std::tie(rhs.material->program, rhs.material->texture);
    });

    std::optional<ShaderManager::Program> curProgram;
    const GX::GL::Texture *curTexture = nullptr;

    for (auto it = first; it != last; ++it) {
#if 0
        if (!frustum.contains(drawCall.mesh->boundingBox(), drawCall.worldMatrix)) {
            continue;
        }
#endif
        const auto &drawCall = *it;

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
        m_shaderManager->setUniform(ShaderManager::ModelViewProjection, m_camera->projectionMatrix() * m_camera->viewMatrix() * drawCall.worldMatrix);
        const auto normalMatrix = glm::transpose(glm::inverse(glm::mat3(drawCall.worldMatrix)));
        m_shaderManager->setUniform(ShaderManager::NormalMatrix, normalMatrix);
        drawCall.mesh->render();
    }
}

void Renderer::end()
{
    auto it = std::stable_partition(m_drawCalls.begin(), m_drawCalls.end(), [](const DrawCall &drawCall) {
        return (drawCall.material->flags & Material::Transparent) == 0;
    });

    // render solid meshes

    glDisable(GL_BLEND);
    render(m_drawCalls.begin(), it);

    // render transparent meshes

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // disable writing to depth buffer
    render(it, m_drawCalls.end());
    glDepthMask(GL_TRUE);
}
