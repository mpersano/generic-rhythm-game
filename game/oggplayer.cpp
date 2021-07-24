#include "oggplayer.h"

#include <spdlog/spdlog.h>
#include <stb_vorbis.h>

OggPlayer::OggPlayer()
{
    alGenSources(1, &m_source);
    for (auto &buffer : m_buffers)
        alGenBuffers(1, &buffer.id);
}

OggPlayer::~OggPlayer()
{
    close();
    for (auto &buffer : m_buffers)
        alDeleteBuffers(1, &buffer.id);
    alDeleteSources(1, &m_source);
}

bool OggPlayer::open(const std::string &path)
{
    int error = 0;
    m_vorbis = stb_vorbis_open_filename(path.c_str(), &error, nullptr);
    if (!m_vorbis) {
        spdlog::error("Failed to open vorbis file {}: {}", path, error);
        return false;
    }

    const stb_vorbis_info info = stb_vorbis_get_info(m_vorbis);
    m_channels = info.channels;
    m_sampleRate = info.sample_rate;

    m_sampleCount = stb_vorbis_stream_length_in_samples(m_vorbis);

    m_format = m_channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

    spdlog::info("Opened {}: channels={} rate={} samples={}", path, m_channels, m_sampleRate, m_sampleCount);

    return true;
}

void OggPlayer::close()
{
    if (m_vorbis) {
        stb_vorbis_close(m_vorbis);
        m_vorbis = nullptr;
    }
}

void OggPlayer::play()
{
    if (!m_vorbis)
        return;

    for (auto &buffer : m_buffers)
        loadAndQueueBuffer(buffer);

    alSourcePlay(m_source);

    m_state = State::Playing;
}

void OggPlayer::stop()
{
    if (m_state != State::Playing)
        return;

    alSourceStop(m_source);

    ALint queued;
    alGetSourcei(m_source, AL_BUFFERS_QUEUED, &queued);
    if (queued > 0) {
        std::array<ALuint, MaxBuffers> buffers;
        alSourceUnqueueBuffers(m_source, queued, buffers.data());
    }

    stb_vorbis_seek_start(m_vorbis);
}

void OggPlayer::update()
{
    if (m_state != State::Playing)
        return;

    ALint processed;
    alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &processed);
    if (processed > 0) {
        spdlog::debug("Processed {} buffers", processed);

        std::array<ALuint, MaxBuffers> buffers;
        alSourceUnqueueBuffers(m_source, processed, buffers.data());

        for (int i = 0; i < processed; ++i) {
            const ALuint id = buffers[i];
            const auto it = std::find_if(m_buffers.begin(), m_buffers.end(), [id](const Buffer &buffer) {
                return buffer.id == id;
            });
            assert(it != m_buffers.end());
            spdlog::debug("Reloading buffer {}", std::distance(m_buffers.begin(), it));
            loadAndQueueBuffer(*it);
        }
    }

    ALint state;
    alGetSourcei(m_source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        ALint queued;
        alGetSourcei(m_source, AL_BUFFERS_QUEUED, &queued);
        if (queued > 0) {
            // we're not done yet, restart
            alSourcePlay(m_source);
        } else {
            // no more data
            m_state = State::Stopped;
            spdlog::debug("Done playing");
        }
    }
}

void OggPlayer::loadAndQueueBuffer(Buffer &buffer)
{
    const int samples = stb_vorbis_get_samples_short_interleaved(m_vorbis, m_channels, buffer.samples.data(), buffer.samples.size());
    spdlog::debug("Read {} samples", samples);
    buffer.size = samples * m_channels;
    if (samples == 0)
        return;
    spdlog::debug("Enqueueing {} samples", buffer.size);
    alBufferData(buffer.id, m_format, buffer.samples.data(), buffer.size * sizeof(SampleType), m_sampleRate);
    alSourceQueueBuffers(m_source, 1, &buffer.id);
}
