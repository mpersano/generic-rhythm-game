#pragma once

#include "track.h"

#include <QGraphicsView>

#include <unordered_map>

class EventItem;

class TrackView : public QGraphicsView
{
public:
    TrackView(Track *track, QWidget *parent = nullptr);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect);

private:
    void trackDecodingFinished();
    void initializeWaveformTiles();
    void adjustSceneRect();

    Track *m_track = nullptr;
    static constexpr auto WaveformTileHeight = 1024;
    std::vector<QPixmap> m_waveformTiles;
    // max QPixmap size is 32767
    std::unordered_map<const Track::Event *, EventItem *> m_eventItems;
};
