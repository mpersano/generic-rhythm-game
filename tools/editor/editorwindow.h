#pragma once

#include <QWidget>

class TrackView;
class Track;

class QGraphicsScene;

class EditorWindow : public QWidget
{
    Q_OBJECT
public:
    explicit EditorWindow(QWidget *parent = nullptr);
    ~EditorWindow() override;

private:
    void trackDecodingFinished();

    Track *m_track;
    TrackView *m_trackView;
    QGraphicsScene *m_scene;
};
