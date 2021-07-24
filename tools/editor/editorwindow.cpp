#include "editorwindow.h"

#include "track.h"
#include "trackinfowidget.h"
#include "trackview.h"

#include <QDebug>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

EditorWindow::EditorWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_track(new Track(this))
    , m_trackView(new TrackView(m_track, this))
    , m_trackInfo(new TrackInfoWidget(m_track, this))
{
    m_trackView->setDragMode(QGraphicsView::ScrollHandDrag);
    setCentralWidget(m_trackView);

    auto *fileMenu = menuBar()->addMenu(tr("&File"));

    auto *newAction = new QAction(tr("&New"), this);
    connect(newAction, &QAction::triggered, this, [this] {
    });
    fileMenu->addAction(newAction);

    auto *openAction = new QAction(tr("&Open..."), this);
    connect(openAction, &QAction::triggered, this, [this] {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Track"));
        if (fileName.isEmpty())
            return;
        QFile file(fileName);
        if (!file.open(QFile::ReadOnly)) {
            const auto message = tr("Failed to open %1\n%2").arg(fileName, file.errorString());
            QMessageBox::warning(this, tr("Error"), message);
            return;
        }
        QJsonParseError error;
        const auto settings = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            const auto message = tr("Failed to parse %1").arg(fileName);
            QMessageBox::warning(this, tr("Error"), message);
            return;
        }
        m_track->load(settings.object());
    });
    fileMenu->addAction(openAction);

    auto *saveAction = new QAction(tr("&Save..."), this);
    connect(saveAction, &QAction::triggered, this, [this] {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Track"));
        if (fileName.isEmpty())
            return;
        QFile file(fileName);
        if (!file.open(QFile::WriteOnly)) {
            const auto message = tr("Failed to open %1\n%2").arg(fileName, file.errorString());
            QMessageBox::warning(this, tr("Error"), message);
            return;
        }
        file.write(QJsonDocument(m_track->save()).toJson(QJsonDocument::Indented));
        statusBar()->showMessage(tr("Saved %1").arg(fileName), 2000);
    });
    fileMenu->addAction(saveAction);

    auto *viewMenu = menuBar()->addMenu(tr("&View"));

    auto *dock = new QDockWidget(tr("Settings"), this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dock->setWidget(m_trackInfo);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    viewMenu->addAction(dock->toggleViewAction());

    statusBar()->showMessage(tr("Ready"));

#if 0
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
#endif
}

EditorWindow::~EditorWindow() = default;
