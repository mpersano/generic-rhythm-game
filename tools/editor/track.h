#pragma once

#include <QAudioFormat>
#include <QObject>

#include <memory>

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

    struct Event {
        enum class Type {
            Tap,
            Hold
        };
        Type type;
        int track;
        float start;
        float duration;
    };

    const Event *addTapEvent(int track, float start);
    const Event *addHoldEvent(int track, float start, float duration);
    void removeEvent(const Event *event);
    std::vector<const Event *> events() const;

signals:
    void decodingFinished();
    void fileNameChanged(const QString &fileName);
    void rateChanged(int rate);
    void eventTracksChanged(int eventTracks);
    void beatsPerMinuteChanged(int beatsPerMinute);
    void durationChanged(float duration);
    void eventAdded(const Event *event);
    void eventAboutToBeRemoved(const Event *event);

private:
    void audioBufferReady();
    void audioDecoderFinished();

    QString m_fileName;
    QAudioDecoder *m_decoder;
    QAudioFormat m_format;
    std::vector<SampleType> m_samples;
    int m_eventTracks = 4;
    int m_beatsPerMinute = 200;
    std::vector<std::unique_ptr<Event>> m_events;
};
