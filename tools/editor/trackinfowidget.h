#pragma once

#include <QWidget>

class QLabel;
class QSpinBox;
class QPushButton;
class QDoubleSpinBox;

class Track;

class TrackInfoWidget : public QWidget
{
public:
    explicit TrackInfoWidget(Track *track, QWidget *parent = nullptr);
    ~TrackInfoWidget() override;

private:
    Track *m_track;
    QLabel *m_file;
    QLabel *m_duration;
    QLabel *m_rate;
    QSpinBox *m_eventTracks;
    QDoubleSpinBox *m_beatsPerMinute;
    QPushButton *m_play;
    QPushButton *m_stop;
};
