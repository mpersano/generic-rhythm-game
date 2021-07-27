#include "track.h"

#include <QAudioDecoder>
#include <QAudioOutput>
#include <QBuffer>
#include <QJsonArray>
#include <QJsonObject>

Track::Track(QObject *parent)
    : QObject(parent)
    , m_decoder(new QAudioDecoder(this))
    , m_buffer(new QBuffer(this))
{
    QAudioFormat format;
    format.setCodec("audio/x-raw");
    format.setSampleType(QAudioFormat::Float);
    format.setChannelCount(1);
    format.setSampleRate(24000);
    format.setSampleSize(16);
    m_decoder->setAudioFormat(format);

    connect(m_decoder, &QAudioDecoder::bufferReady, this, &Track::audioBufferReady);
    connect(m_decoder, &QAudioDecoder::finished, this, &Track::audioDecoderFinished);
    connect(m_decoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error), this,
            [this](QAudioDecoder::Error error)
            {
                qWarning() << "Audio decoder error:" << error;
            });
}

Track::~Track() = default;

void Track::decode(const QString &audioFile)
{
    m_audioFile = audioFile;

    m_samples.clear();
    m_format = QAudioFormat();
    Q_ASSERT(!m_format.isValid());

    emit audioFileChanged(audioFile);
    emit durationChanged(duration());
    emit rateChanged(rate());

    m_decoder->setSourceFilename(audioFile);
    m_decoder->start();
}

bool Track::isDecoding() const
{
    return m_decoder->state() == QAudioDecoder::DecodingState;
}

void Track::audioBufferReady()
{
    Q_ASSERT(m_decoder->bufferAvailable());

    const QAudioBuffer buffer = m_decoder->read();

    const QAudioFormat format = buffer.format();
    if (!m_format.isValid()) {
        m_format = format;
        emit rateChanged(rate());
    }

    qDebug() << "**** got buffer: duration:" << buffer.duration() << "samples:" << buffer.sampleCount() << "format:" << format;

    Q_ASSERT(buffer.byteCount() == buffer.sampleCount() * sizeof(SampleType));

    const auto *data = buffer.constData<SampleType>();
    m_samples.insert(m_samples.end(), data, data + buffer.sampleCount());

    emit durationChanged(duration());
}

void Track::audioDecoderFinished()
{
    qDebug() << "**** finished decoding, got" << m_samples.size() << "samples";
    emit decodingFinished();
}

QString Track::audioFile() const
{
    return m_audioFile;
}

const Track::SampleType *Track::samples() const
{
    return m_samples.data();
}

float Track::duration() const
{
    return static_cast<float>(m_samples.size()) / rate();
}

int Track::sampleCount() const
{
    return static_cast<int>(m_samples.size());
}

void Track::setEventTracks(int eventTracks)
{
    if (eventTracks == m_eventTracks)
        return;
    m_eventTracks = eventTracks;
    emit eventTracksChanged(eventTracks);
}

int Track::eventTracks() const
{
    return m_eventTracks;
}

void Track::setBeatsPerMinute(float beatsPerMinute)
{
    if (qFuzzyCompare(beatsPerMinute, m_beatsPerMinute))
        return;
    m_beatsPerMinute = beatsPerMinute;
    emit beatsPerMinuteChanged(beatsPerMinute);
}

float Track::beatsPerMinute() const
{
    return m_beatsPerMinute;
}

int Track::rate() const
{
    return m_format.sampleRate();
}

std::vector<const Track::Event *> Track::events() const
{
    std::vector<const Event *> result;
    result.reserve(m_events.size());
    std::transform(m_events.begin(), m_events.end(), std::back_inserter(result), [](const auto &event) { return event.get(); });
    return result;
}

const Track::Event *Track::addTapEvent(int track, float start)
{
    m_events.emplace_back(new Event { Event::Type::Tap, track, start, 0.0f });
    const auto *event = m_events.back().get();
    emit eventAdded(event);
    return event;
}

const Track::Event *Track::addHoldEvent(int track, float start, float duration)
{
    m_events.emplace_back(new Event { Event::Type::Hold, track, start, 0.0f });
    const auto *event = m_events.back().get();
    emit eventAdded(event);
    return event;
}

void Track::removeEvent(const Event *event)
{
    auto it = std::find_if(m_events.begin(), m_events.end(), [event](auto &item) {
        return item.get() == event;
    });
    if (it == m_events.end())
        return;
    emit eventAboutToBeRemoved(event);
    m_events.erase(it);
}

void Track::setEventStart(const Event *event, float start)
{
    Q_ASSERT(std::any_of(m_events.begin(), m_events.end(), [event](auto &item) {
        return item.get() == event;
    }));
    const_cast<Event *>(event)->start = start;
    emit eventChanged(event);
}

void Track::setEventDuration(const Event *event, float duration)
{
    Q_ASSERT(std::any_of(m_events.begin(), m_events.end(), [event](auto &item) {
        return item.get() == event;
    }));
    const_cast<Event *>(event)->duration = duration;
    emit eventChanged(event);
}

void Track::setPlaybackStartPosition(float position)
{
    if (m_output && m_output->state() == QAudio::ActiveState)
        return;
    m_playbackStartPosition = position;
}

void Track::startPlayback()
{
    if (!isValid())
        return;

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(m_format)) {
        qWarning() << "Format not supported by audio backend, can't play audio";
        return;
    }

    m_buffer->setData(QByteArray::fromRawData(reinterpret_cast<const char *>(m_samples.data()), m_samples.size() * sizeof(SampleType)));
    m_buffer->open(QIODevice::ReadOnly);
    const auto startSample = static_cast<int>(m_playbackStartPosition * m_format.sampleRate());
    m_buffer->seek(startSample * sizeof(SampleType));
    m_output = new QAudioOutput(m_format, this);
    m_output->setNotifyInterval(20);
    connect(m_output, &QAudioOutput::stateChanged, this, &Track::outputStateChanged);
    connect(m_output, &QAudioOutput::notify, this, &Track::playbackPositionUpdated);
    m_output->start(m_buffer);
}

void Track::stopPlayback()
{
    if (!m_output)
        return;
    m_playbackStartPosition = playbackPosition();
    m_output->stop();
}

float Track::playbackPosition() const
{
    if (!m_output)
        return 0.0f;
    return m_playbackStartPosition + static_cast<float>(m_output->elapsedUSecs()) * 1e-6;
}

void Track::outputStateChanged(QAudio::State state)
{
    const auto resetOutput = [this] {
        m_output->stop();
        m_buffer->close();
        delete m_output;
        m_output = nullptr;
        emit playbackStopped();
    };
    switch (state) {
    case QAudio::IdleState:
        // finished playing
        resetOutput();
        break;
    case QAudio::StoppedState:
        if (m_output->error() != QAudio::NoError) {
            qWarning() << "playback error:" << m_output->error();
        }
        resetOutput();
        break;
    default:
        break;
    }
}

namespace {
QString audioFileKey()
{
    return QLatin1String("audioFile");
}

QString eventTracksKey()
{
    return QLatin1String("eventTracks");
}

QString beatsPerMinuteKey()
{
    return QLatin1String("beatsPerMinute");
}

QString typeKey()
{
    return QLatin1String("type");
}

QString trackKey()
{
    return QLatin1String("track");
}

QString startKey()
{
    return QLatin1String("start");
}

QString durationKey()
{
    return QLatin1String("duration");
}

QString eventsKey()
{
    return QLatin1String("events");
}
} // namespace

void Track::load(const QJsonObject &settings)
{
    const auto eventTracks = settings[eventTracksKey()].toInt();
    setEventTracks(eventTracks);

    const auto beatsPerMinute = settings[beatsPerMinuteKey()].toDouble();
    setBeatsPerMinute(beatsPerMinute);

    m_events.clear();
    const auto eventsArray = settings[eventsKey()].toArray();
    std::transform(eventsArray.begin(), eventsArray.end(), std::back_inserter(m_events),
                   [](const QJsonValue &value) {
                       const auto eventSettings = value.toObject();
                       const auto type = static_cast<Event::Type>(eventSettings[typeKey()].toInt());
                       const auto track = eventSettings[trackKey()].toInt();
                       const auto start = static_cast<float>(eventSettings[startKey()].toDouble());
                       const auto duration = static_cast<float>(eventSettings[durationKey()].toDouble());
                       return std::unique_ptr<Event>(new Event { type, track, start, duration });
                   });
    emit eventsReset();

    const auto audioFile = settings[audioFileKey()].toString();
    decode(audioFile);
}

QJsonObject Track::save() const
{
    QJsonObject settings;

    settings[audioFileKey()] = m_audioFile;
    settings[eventTracksKey()] = m_eventTracks;
    settings[beatsPerMinuteKey()] = m_beatsPerMinute;
    QJsonArray eventArray;
    for (auto &event : m_events) {
        QJsonObject eventSettings;
        eventSettings[typeKey()] = static_cast<int>(event->type);
        eventSettings[trackKey()] = event->track;
        eventSettings[startKey()] = event->start;
        eventSettings[durationKey()] = event->duration;
        eventArray.append(eventSettings);
    }
    settings[eventsKey()] = eventArray;

    return settings;
}

bool Track::isValid() const
{
    return m_format.isValid();
}
