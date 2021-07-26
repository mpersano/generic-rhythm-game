#pragma once

#include "inputstate.h"

#include <glm/gtx/quaternion.hpp>

#include <memory>
#include <vector>

class ShaderManager;
class Camera;
class Renderer;
class Mesh;
class Track;

class World
{
public:
    World();
    ~World();

    void resize(int width, int height);
    void update(InputState inputState, float elapsed);
    void render() const;

    void initializeLevel(const Track *track);

private:
    void initializeTrackMesh();
    void initializeBeatMeshes();
    struct PathState
    {
        glm::mat3 orientation;
        glm::vec3 center;
    };
    PathState pathStateAt(float distance) const;

    std::unique_ptr<ShaderManager> m_shaderManager;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Renderer> m_renderer;
    std::vector<std::unique_ptr<Mesh>> m_trackSegments;
    struct PathPart {
        glm::mat3 orientation;
        glm::vec3 center;
        float distance;

        glm::vec3 up() const { return orientation[0]; }
        glm::vec3 side() const { return orientation[1]; }
        glm::vec3 direction() const { return orientation[2]; }
    };
    std::vector<PathPart> m_pathParts;
    std::unique_ptr<Mesh> m_beatMesh;
    float m_trackTime = 0.0f;
    const Track *m_track;
    struct Beat {
        float start;
        glm::mat4 transform;
        enum class State {
            Active
        };
        State state;
    };
    std::vector<Beat> m_beats;
};
