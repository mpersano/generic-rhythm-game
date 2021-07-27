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
    void initializeMarkerMesh();
    struct PathState
    {
        glm::mat3 orientation;
        glm::vec3 center;

        glm::vec3 up() const { return orientation[0]; }
        glm::vec3 side() const { return orientation[1]; }
        glm::vec3 direction() const { return orientation[2]; }
        glm::mat4 transformMatrix() const;
    };
    PathState pathStateAt(float distance) const;
    void updateCamera(bool snapToPosition);

    std::unique_ptr<ShaderManager> m_shaderManager;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Renderer> m_renderer;
    struct TrackSegment {
        glm::vec3 position;
        std::unique_ptr<Mesh> mesh;
    };
    std::vector<TrackSegment> m_trackSegments;
    struct PathPart {
        PathState state;
        float distance;
    };
    std::vector<PathPart> m_pathParts;
    std::unique_ptr<Mesh> m_beatMesh;
    std::unique_ptr<Mesh> m_markerMesh;
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
    glm::vec3 m_cameraPosition;
    glm::mat4 m_markerTransform;
    std::vector<Beat> m_beats;
};
