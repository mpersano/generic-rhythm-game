#pragma once

#include <QGraphicsView>

class Track;

class TrackView : public QGraphicsView
{
public:
    TrackView(Track *track, QWidget *parent = nullptr);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect);

private:
    void adjustScene();

    Track *m_track = nullptr;
};
