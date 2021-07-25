#pragma once

#include <QSize>
#include <QThread>

class Track;

class WaveformRenderer : public QThread
{
    Q_OBJECT
public:
    WaveformRenderer(const Track *track, const QSize &tileSize, float pixelsPerSecond, QObject *parent = nullptr);

    void run() override;

signals:
    void tileReady(const QPixmap &pixmap);

private:
    const Track *m_track;
    QSize m_tileSize;
    float m_pixelsPerSecond;
};
