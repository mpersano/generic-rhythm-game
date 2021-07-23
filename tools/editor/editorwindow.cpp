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

    connect(m_track, &Track::decodingFinished, this, [this] {
        // add some random events
        const auto duration = m_track->duration();
        const float dt = 60.0f / m_track->beatsPerMinute();
        for (int i = 0, eventTracks = m_track->eventTracks(); i < eventTracks; ++i) {
            for (float t = 0.0f; t < duration; t += dt) {
                if ((std::rand() % 5) == 0) {
                    m_track->addTapEvent(i, t);
                }
            }
        }
    });
}

EditorWindow::~EditorWindow() = default;
