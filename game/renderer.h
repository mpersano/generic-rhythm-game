#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <vector>

#include "camera.h"

class Mesh;
struct Material;

class ShaderManager;

class Renderer
{
public:
    Renderer(ShaderManager *shaderManager, const Camera *camera);
    ~Renderer();

    void resize(int width, int height);

    void begin();
    void render(const Mesh *mesh, const Material *material, const glm::mat4 &worldMatrix);
    void end();

private:
    template<typename Iterator>
    void render(Iterator first, Iterator last) const;

    int m_width = 1;
    int m_height = 1;
    struct DrawCall {
        const Mesh *mesh;
        const Material *material;
        glm::mat4 worldMatrix;
    };
    std::vector<DrawCall> m_drawCalls;
    ShaderManager *m_shaderManager;
    const Camera *m_camera;
};
