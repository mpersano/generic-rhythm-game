#pragma once

#include "inputstate.h"

#include <memory>
#include <vector>

class ShaderManager;
class Camera;
class Renderer;
class Mesh;

class World
{
public:
    World();
    ~World();

    void resize(int width, int height);
    void update(InputState inputState, float elapsed);
    void render() const;

private:
    void initializeTrackMesh();

    std::unique_ptr<ShaderManager> m_shaderManager;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Renderer> m_renderer;
    std::vector<std::unique_ptr<Mesh>> m_trackSegments;
    float m_angle = 0.0f;
};
