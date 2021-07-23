#include "editorwindow.h"

#include "track.h"
#include "trackinfowidget.h"
#include "trackview.h"

#include <QDir>
#include <QHBoxLayout>

EditorWindow::EditorWindow(QWidget *parent)
    : QWidget(parent)
    , m_track(new Track(this))
    , m_trackView(new TrackView(m_track, this))
    , m_trackInfo(new TrackInfoWidget(m_track, this))
{
    m_trackView->setDragMode(QGraphicsView::ScrollHandDrag);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_trackView);
    layout->addWidget(m_trackInfo);

    m_track->decode(QDir::current().filePath("test.ogg"));
}

EditorWindow::~EditorWindow() = default;
