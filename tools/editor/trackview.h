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
    void adjustSceneRect();

    Track *m_track = nullptr;
    std::unordered_map<const Track::Event *, EventItem *> m_eventItems;
};
