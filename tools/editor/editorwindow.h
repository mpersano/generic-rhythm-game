#pragma once

#include <QMainWindow>

class TrackView;
class TrackInfoWidget;
class Track;

class QGraphicsScene;

class EditorWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit EditorWindow(QWidget *parent = nullptr);
    ~EditorWindow() override;

private:
    Track *m_track;
    TrackView *m_trackView;
    TrackInfoWidget *m_trackInfo;
};
