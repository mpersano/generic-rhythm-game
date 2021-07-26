#include "track.h"

#include <QAudioDecoder>
#include <QJsonArray>
#include <QJsonObject>

Track::Track(QObject *parent)
    : QObject(parent)
    , m_decoder(new QAudioDecoder(this))
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

void Track::setBeatsPerMinute(int beatsPerMinute)
{
    if (beatsPerMinute == m_beatsPerMinute)
        return;
    m_beatsPerMinute = beatsPerMinute;
    emit beatsPerMinuteChanged(beatsPerMinute);
}

int Track::beatsPerMinute() const
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

    const auto beatsPerMinute = settings[beatsPerMinuteKey()].toInt();
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
