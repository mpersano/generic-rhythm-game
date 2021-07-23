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
    m_samples.clear();
    m_format = QAudioFormat();
    Q_ASSERT(!m_format.isValid());

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
    if (!m_format.isValid())
        m_format = format;

    qDebug() << "**** got buffer: duration:" << buffer.duration() << "samples:" << buffer.sampleCount() << "format:" << format;

    Q_ASSERT(buffer.byteCount() == buffer.sampleCount() * sizeof(SampleType));

    const auto *data = buffer.constData<SampleType>();
    m_samples.insert(m_samples.end(), data, data + buffer.sampleCount());
}

void Track::audioDecoderFinished()
{
    qDebug() << "**** finished decoding, got" << m_samples.size() << "samples";
    emit decodingFinished();
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

int Track::eventTracks() const
{
    return 4;
}

int Track::beatsPerMinute() const
{
    return 200;
}

int Track::rate() const
{
    return m_format.sampleRate();
}
