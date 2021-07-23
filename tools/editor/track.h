#pragma once

#include <QAudioFormat>
#include <QObject>

class QAudioDecoder;

class Track : public QObject
{
    Q_OBJECT
public:
    explicit Track(QObject *parent = nullptr);
    ~Track() override;

    void decode(const QString &fileName);
    bool isDecoding() const;

    using SampleType = float;

    const SampleType *samples() const;
    int sampleCount() const;

    int eventTracks() const;
    int beatsPerMinute() const;
    int rate() const;
    float duration() const; // seconds

signals:
    void decodingFinished();

private:
    void audioBufferReady();
    void audioDecoderFinished();

    QAudioDecoder *m_decoder;
    QAudioFormat m_format;
    std::vector<SampleType> m_samples;
};
