#include "oggplayer.h"

#include <gx/glwindow.h>

#include <AL/al.h>
#include <AL/alc.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

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
};

GameWindow::GameWindow()
{
    initializeAL();

    m_player = std::make_unique<OggPlayer>();
    if (m_player->open("test.ogg"))
        m_player->play();
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
}

void GameWindow::paintGL()
{
}

void GameWindow::update(double /* elapsed */)
{
    m_player->update();
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
