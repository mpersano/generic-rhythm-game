#include "track.h"

#include <QAudioDecoder>

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
}

Track::~Track() = default;

void Track::decode(const QString &fileName)
{
    m_fileName = fileName;

    m_samples.clear();
    m_format = QAudioFormat();
    Q_ASSERT(!m_format.isValid());

    emit fileNameChanged(fileName);
    emit durationChanged(duration());
    emit rateChanged(rate());

    m_decoder->setSourceFilename(fileName);
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

QString Track::fileName() const
{
    return m_fileName;
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
