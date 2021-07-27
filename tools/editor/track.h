#pragma once

#include <QAudioFormat>
#include <QObject>

#include <memory>

class QAudioDecoder;
class QJsonObject;

class Track : public QObject
{
    Q_OBJECT
public:
    explicit Track(QObject *parent = nullptr);
    ~Track() override;

    bool isValid() const;

    void decode(const QString &audioFile);
    bool isDecoding() const;

    using SampleType = float;

    const SampleType *samples() const;
    int sampleCount() const;

    QString audioFile() const;

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
    void setEventStart(const Event *event, float start);
    void setEventDuration(const Event *event, float duration);
    std::vector<const Event *> events() const;

    QJsonObject save() const;
    void load(const QJsonObject &settings);

signals:
    void decodingFinished();
    void audioFileChanged(const QString &audioFile);
    void rateChanged(int rate);
    void eventTracksChanged(int eventTracks);
    void beatsPerMinuteChanged(int beatsPerMinute);
    void durationChanged(float duration);
    void eventAdded(const Event *event);
    void eventAboutToBeRemoved(const Event *event);
    void eventChanged(const Event *event);
    void eventsReset();

private:
    void audioBufferReady();
    void audioDecoderFinished();

    QString m_audioFile;
    QAudioDecoder *m_decoder;
    QAudioFormat m_format;
    std::vector<SampleType> m_samples;
    int m_eventTracks = 4;
    int m_beatsPerMinute = 100;
    std::vector<std::unique_ptr<Event>> m_events;
};
