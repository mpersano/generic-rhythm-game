#pragma once

#include <QAudio>
#include <QAudioFormat>
#include <QObject>
#include <QSound>

#include <memory>

class QAudioDecoder;
class QJsonObject;
class QAudioOutput;
class QBuffer;

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

    void setBeatsPerMinute(float beatsPerMinute);
    float beatsPerMinute() const;

    void setOffset(float offset);
    float offset() const;

    int beatDivisor() const;

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

    void startPlayback();
    void stopPlayback();
    float playbackPosition() const;
    bool isPlaying() const;

    float playbackStartPosition() const;
    void setPlaybackStartPosition(float start);

    QJsonObject save() const;
    void load(const QJsonObject &settings);

signals:
    void decodingFinished();
    void audioFileChanged(const QString &audioFile);
    void rateChanged(int rate);
    void eventTracksChanged(int eventTracks);
    void beatsPerMinuteChanged(float beatsPerMinute);
    void offsetChanged(float offset);
    void durationChanged(float duration);
    void eventAdded(const Event *event);
    void eventAboutToBeRemoved(const Event *event);
    void eventChanged(const Event *event);
    void eventsReset();
    void playbackPositionChanged();
    void playbackStopped();

private:
    void audioBufferReady();
    void audioDecoderFinished();
    void outputStateChanged(QAudio::State state);

    QString m_audioFile;
    QAudioDecoder *m_decoder;
    QAudioFormat m_format;
    QAudioOutput *m_output = nullptr;
    QBuffer *m_buffer = nullptr;
    std::vector<SampleType> m_samples;
    int m_eventTracks = 4;
    float m_beatsPerMinute = 100.0f;
    float m_offset = 0.0f;
    std::vector<std::unique_ptr<Event>> m_events;
    float m_playbackStartPosition = 0.0f;
    QSound *m_clap;
    float m_previousPlaybackPosition = 0.0f;
};
