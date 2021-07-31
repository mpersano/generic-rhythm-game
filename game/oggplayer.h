#pragma once

#include <gx/noncopyable.h>

#include <AL/al.h>

#include <array>
#include <string>

struct stb_vorbis;

class OggPlayer : private GX::NonCopyable
{
public:
    OggPlayer();
    ~OggPlayer();

    bool open(const std::string &path);
    void close();

    void play();
    void stop();
    void update();

    enum class State {
        Playing,
        Stopped,
    };

    State state() const
    {
        return m_state;
    }

    unsigned sampleRate() const
    {
        return m_sampleRate;
    }

    unsigned sampleCount() const
    {
        return m_sampleCount;
    }

private:
    using SampleType = int16_t;

    struct Buffer {
        constexpr static auto Capacity = 64 * 1024;
        std::array<SampleType, Capacity> samples;
        unsigned size; // samples loaded into this buffer
        ALuint id;
    };

    void loadAndQueueBuffer(Buffer &buffer);

    stb_vorbis *m_vorbis = nullptr;
    unsigned m_channels;
    unsigned m_sampleRate;
    unsigned m_sampleCount;
    ALuint m_source;
    ALenum m_format;
    static constexpr auto MaxBuffers = 4;
    std::array<Buffer, MaxBuffers> m_buffers;
    State m_state = State::Stopped;
};
