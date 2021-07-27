#include "oggplayer.h"
#include "track.h"
#include "world.h"

#include <gx/glwindow.h>

#include <AL/al.h>
#include <AL/alc.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

using namespace std::string_literals;

class GameWindow : public GX::GLWindow
{
public:
    GameWindow();
    ~GameWindow();

private:
    void initializeAL();
    void releaseAL();

    void initializeGL() override;
    void paintGL() override;
    void update(double elapsed) override;
    void keyPressEvent(int key) override;
    void keyReleaseEvent(int key) override;

    ALCdevice *m_alDevice = nullptr;
    ALCcontext *m_alContext = nullptr;
    std::unique_ptr<OggPlayer> m_player;
    std::unique_ptr<World> m_world;
    std::unique_ptr<Track> m_track;
    InputState m_inputState = InputState::None;
};

GameWindow::GameWindow()
{
    initializeAL();

    m_player = std::make_unique<OggPlayer>();

    m_track = loadTrack("assets/tracks/track.json"s);
#if 0
    if (m_track) {
        spdlog::info("Loaded track: eventTracks={} beatsPerMinute={}, {} events", m_track->eventTracks, m_track->beatsPerMinute, m_track->events.size());
        if (m_player->open(m_track->audioFile))
            m_player->play();
    }
#endif
}

GameWindow::~GameWindow()
{
    m_player.reset();
    releaseAL();
}

void GameWindow::initializeAL()
{
    if (!(m_alDevice = alcOpenDevice(nullptr))) {
        spdlog::error("Failed to open AL device");
    }

    if (!(m_alContext = alcCreateContext(m_alDevice, nullptr))) {
        spdlog::error("Failed to open AL context");
    }

    alcMakeContextCurrent(m_alContext);
    alGetError();
}

void GameWindow::releaseAL()
{
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(m_alContext);
    alcCloseDevice(m_alDevice);
}

void GameWindow::initializeGL()
{
    m_world = std::make_unique<World>();
    m_world->resize(width(), height());
    m_world->initializeLevel(m_track.get());
}

void GameWindow::paintGL()
{
    m_world->render();
}

void GameWindow::update(double elapsed)
{
#if 0
    m_player->update();
#endif
    m_world->update(m_inputState, elapsed);
}

void GameWindow::keyPressEvent(int key)
{
    switch (key) {
#define UPDATE_INPUT_STATE(key, state)     \
    case GLFW_KEY_##key:                   \
        m_inputState |= InputState::state; \
        break;
        UPDATE_INPUT_STATE(D, Fire1)
        UPDATE_INPUT_STATE(F, Fire2)
        UPDATE_INPUT_STATE(J, Fire3)
        UPDATE_INPUT_STATE(K, Fire4)
#undef UPDATE_INPUT_STATE
    default:
        break;
    }
}

void GameWindow::keyReleaseEvent(int key)
{
    switch (key) {
#define UPDATE_INPUT_STATE(key, state)      \
    case GLFW_KEY_##key:                    \
        m_inputState &= ~InputState::state; \
        break;
        UPDATE_INPUT_STATE(D, Fire1)
        UPDATE_INPUT_STATE(F, Fire2)
        UPDATE_INPUT_STATE(J, Fire3)
        UPDATE_INPUT_STATE(K, Fire4)
#undef UPDATE_INPUT_STATE
    default:
        break;
    }
}

int main(int argc, char *argv[])
{
    GameWindow w;
    w.initialize(1200, 600, "test");
    w.enableGLDebugging(GL_DEBUG_SEVERITY_LOW);
    w.renderLoop();
}
