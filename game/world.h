#pragma once

#include "bezier.h"
#include "inputstate.h"

#include <glm/gtx/quaternion.hpp>

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
    struct Segment {
        Bezier path;
        std::unique_ptr<Mesh> mesh;
    };
    std::vector<std::unique_ptr<Segment>> m_trackSegments;
    struct PathPart {
        glm::mat3 orientation;
        glm::vec3 center;
        float distance;
    };
    std::vector<PathPart> m_pathParts;
    float m_trackTime = 0.0f;
};
