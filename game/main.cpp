#include "hudpainter.h"
#include "shadermanager.h"
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
    std::unique_ptr<ShaderManager> m_shaderManager;
    std::unique_ptr<HUDPainter> m_hudPainter;
    std::unique_ptr<World> m_world;
    std::unique_ptr<Track> m_track;
    InputState m_inputState = InputState::None;
};

GameWindow::GameWindow()
{
    initializeAL();

    m_track = loadTrack("assets/tracks/bomb.json"s);
    if (m_track) {
        spdlog::info("Loaded track: eventTracks={} beatsPerMinute={}, {} events", m_track->eventTracks, m_track->beatsPerMinute, m_track->events.size());
    }
}

GameWindow::~GameWindow()
{
    m_world.reset();
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
    m_shaderManager = std::make_unique<ShaderManager>();

    m_hudPainter = std::make_unique<HUDPainter>();
    m_hudPainter->resize(width(), height());

    m_world = std::make_unique<World>(m_shaderManager.get());
    m_world->resize(width(), height());
    m_world->initializeLevel(m_track.get());
}

void GameWindow::paintGL()
{
    // render world

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    m_world->render();

    // render HUD

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_hudPainter->startPainting();
    m_world->renderHUD(m_hudPainter.get());
    m_hudPainter->donePainting();
}

void GameWindow::update(double elapsed)
{
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
