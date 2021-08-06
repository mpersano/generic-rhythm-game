#pragma once

#include "inputstate.h"

#include <glm/gtx/quaternion.hpp>

#include <memory>
#include <vector>

class ShaderManager;
class Camera;
class Renderer;
class Mesh;
struct Track;
class HUDPainter;
class HUDAnimation;
class ComboCounter;
class OggPlayer;
class ParticleSystem;

class World
{
public:
    explicit World(ShaderManager *shaderManager);
    ~World();

    void resize(int width, int height);
    void update(InputState inputState, float elapsed);
    void render() const;
    void renderHUD(HUDPainter *hudPainter) const;

    void setTrack(const Track* track);

    void startGame();
    bool isPlaying() const;

private:
    void initializeLevel();
    void initializeTrackMesh();
    void initializeBeatMeshes();
    void initializeMarkerMesh();
    void initializeButtonMesh();
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
    void updateBeats(InputState inputState);
    void updateDebris(float elapsed);
    void updateTextAnimations(float elapsed);
    void updateComboPainter(float elapsed);
    void updateParticles(float elapsed);

    ShaderManager *m_shaderManager;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<ParticleSystem> m_particleSystem;
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
    std::unique_ptr<Mesh> m_buttonMesh;
    float m_trackTime = 0.0f;
    const Track *m_track;
    struct Beat {
        enum class Type {
            Tap,
            Hold,
        };
        Type type;
        float start;
        float duration;
        int track;
        glm::mat4 transform;
        enum class State {
            Active,
            Inactive,
            Holding,
            HoldMissed,
        };
        State state;
        std::unique_ptr<Mesh> mesh; // if type == Hold
    };
    struct Debris {
        int track;
        glm::vec3 position;
        glm::mat3 orientation;
        glm::vec3 scale;
        glm::vec3 velocity;
        glm::vec3 rotationAxis;
        float angularSpeed;
        float time;
        float life;
    };
    glm::vec3 m_cameraPosition;
    glm::mat4 m_markerTransform;
    glm::vec4 m_clipPlane; // to clip long notes
    std::vector<std::unique_ptr<Beat>> m_beats;
    std::vector<Debris> m_debris;
    std::vector<std::unique_ptr<HUDAnimation>> m_hudAnimations;
    std::unique_ptr<ComboCounter> m_comboCounter;
    std::unique_ptr<OggPlayer> m_player;
    InputState m_prevInputState = InputState::None;
};
