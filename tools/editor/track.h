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

    QString fileName() const;

    void setEventTracks(int eventTracks);
    int eventTracks() const;

    void setBeatsPerMinute(int beatsPerMinute);
    int beatsPerMinute() const;

    int rate() const;

    float duration() const; // seconds

signals:
    void decodingFinished();
    void fileNameChanged(const QString &fileName);
    void rateChanged(int rate);
    void eventTracksChanged(int eventTracks);
    void beatsPerMinuteChanged(int beatsPerMinute);
    void durationChanged(float duration);

private:
    void audioBufferReady();
    void audioDecoderFinished();

    QString m_fileName;
    QAudioDecoder *m_decoder;
    QAudioFormat m_format;
    std::vector<SampleType> m_samples;
    int m_eventTracks = 4;
    int m_beatsPerMinute = 200;
};
