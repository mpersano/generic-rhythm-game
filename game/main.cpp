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

    ALCdevice *m_alDevice = nullptr;
    ALCcontext *m_alContext = nullptr;
    std::unique_ptr<OggPlayer> m_player;
    std::unique_ptr<World> m_world;
    std::unique_ptr<Track> m_track;
};

GameWindow::GameWindow()
{
    initializeAL();

#if 0
    m_player = std::make_unique<OggPlayer>();

    const auto track = "test.json"s;

    m_track = loadTrack(track);
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
    m_world->update(InputState::None, elapsed);
}

void GameWindow::keyPressEvent(int key)
{
    if (key == GLFW_KEY_SPACE) {
        if (m_player->state() == OggPlayer::State::Playing) {
            m_player->stop();
        } else {
            m_player->play();
        }
    }
}

int main(int argc, char *argv[])
{
    GameWindow w;
    w.initialize(1200, 600, "test");
    w.enableGLDebugging(GL_DEBUG_SEVERITY_LOW);
    w.renderLoop();
}
