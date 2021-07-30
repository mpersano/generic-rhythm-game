#include "trackview.h"

#include "track.h"
#include "waveformrenderer.h"

#include <QAction>
#include <QDebug>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QScopedValueRollback>
#include <QStyleOptionGraphicsItem>
#include <QThread>

#include <cmath>

namespace {
constexpr auto HorizMargin = 20.0f;
constexpr auto VertMargin = 20.0f;
constexpr auto WaveformWidth = 100.0f;
constexpr auto EventAreaWidth = 500.0f;
constexpr auto PixelsPerSecond = 300.0f;

float trackX(const Track *track, int trackIndex)
{
    return HorizMargin + WaveformWidth + (trackIndex + 1) * EventAreaWidth / (track->eventTracks() + 1);
}

QPointF eventPosition(const Track *track, int trackIndex, float time)
{
    const float x = trackX(track, trackIndex);
    const float y = -PixelsPerSecond * time;
    return QPointF(x, y);
}

QColor trackColor(int trackIndex)
{
    static const std::vector<QColor> colors = {
        Qt::red, Qt::yellow, Qt::cyan, Qt::magenta, Qt::green, Qt::gray
    };
    return colors[trackIndex];
}

} // namespace

class EventItem : public QGraphicsItem
{
public:
    EventItem(Track *track, const Track::Event *event, QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    static constexpr const auto Radius = 10.0;

    Track *m_track;
    const Track::Event *m_event;
};

EventItem::EventItem(Track *track, const Track::Event *event, QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , m_track(track)
    , m_event(event)
{
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
    setPos(eventPosition(track, event->track, event->start));
}

QRectF EventItem::boundingRect() const
{
    return QRectF(-Radius, -Radius, 2.0f * Radius, 2.0f * Radius);
}

QVariant EventItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    switch (change) {
    case ItemPositionChange: {
        // stay on your lane!
        QPointF updatePos = value.toPointF();
        updatePos.setX(trackX(m_track, m_event->track));
        return updatePos;
    }
    case ItemPositionHasChanged: {
        const auto pos = value.toPointF();
        const auto eventStart = static_cast<float>(-pos.y()) / PixelsPerSecond;
        m_track->setEventStart(m_event, eventStart);
        break;
    }
    default:
        break;
    }
    return value;
}

void EventItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * /* widget */)
{
    auto color = trackColor(m_event->track);
    if (option->state & QStyle::State_Selected)
        color = color.darker(200);
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawEllipse(boundingRect());
}

class MarkerItem : public QGraphicsLineItem
{
public:
    explicit MarkerItem(Track *track, QGraphicsItem *parent = nullptr);

    // HACK
    bool updatingTrackPosition() const
    {
        return m_updatingTrackPosition;
    }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    Track *m_track;
    bool m_updatingTrackPosition = false;
};

MarkerItem::MarkerItem(Track *track, QGraphicsItem *parent)
    : QGraphicsLineItem(parent)
    , m_track(track)
{
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
}

QVariant MarkerItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    switch (change) {
    case ItemPositionChange: {
        QPointF updatePos = value.toPointF();
        updatePos.setX(0);
        return updatePos;
    }
    case ItemPositionHasChanged: {
        if (m_updatingTrackPosition)
            break;
        QScopedValueRollback<bool> r(m_updatingTrackPosition, true);
        const auto pos = value.toPointF();
        const auto start = static_cast<float>(-pos.y()) / PixelsPerSecond;
        m_track->setPlaybackStartPosition(start);
        break;
    }
    default:
        break;
    }
    return value;
}

void TrackView::initializeWaveformTiles()
{
    const auto height = static_cast<int>(m_track->duration() * PixelsPerSecond);

    const auto *samples = m_track->samples();
    const auto sampleCount = m_track->sampleCount();

    const auto dySample = PixelsPerSecond / m_track->rate();

    const auto maxSample = [samples, sampleCount] {
        const auto it = std::max_element(samples, samples + sampleCount, [](float lhs, float rhs) {
            return std::abs(lhs) < std::abs(rhs);
        });
        return std::abs(*it);
    }();
    const auto amplitude = 0.5f * WaveformWidth / (1.1f * maxSample);

    m_waveformTiles.clear();

    for (int yStart = 0; yStart < height; yStart += WaveformTileHeight) {
        const auto yEnd = std::min(yStart + WaveformTileHeight, height);

        const auto sampleStart = std::max(static_cast<int>((yStart / PixelsPerSecond) * m_track->rate()), 0);
        const auto sampleEnd = std::min(static_cast<int>((yEnd / PixelsPerSecond) * m_track->rate()), sampleCount - 1);

        const auto tileHeight = yEnd - yStart;
        QImage image(WaveformWidth, tileHeight, QImage::Format_ARGB32);
        image.fill(Qt::transparent);

        std::vector<QPointF> polyline;
        polyline.reserve(sampleCount);

        for (size_t i = sampleStart; i <= sampleEnd; ++i) {
            const auto sample = samples[i];

            constexpr auto Amplitude = 0.5f * WaveformWidth;
            constexpr auto CenterX = 0.5f * WaveformWidth;

            const auto x = CenterX + sample * amplitude;
            const auto y = tileHeight - static_cast<float>(i - sampleStart) * dySample;
            polyline.emplace_back(x, y);
        }

        QPainter painter(&image);
        painter.setRenderHints(QPainter::Antialiasing, true);
        painter.setPen(Qt::white);
        painter.drawPolyline(polyline.data(), polyline.size());

        m_waveformTiles.push_back(QPixmap::fromImage(image));
    }
}

TrackView::TrackView(Track *track, QWidget *parent)
    : QGraphicsView(parent)
    , m_track(track)
{
    setFocusPolicy(Qt::StrongFocus);
    setScene(new QGraphicsScene(this));
    connect(m_track, &Track::decodingFinished, this, &TrackView::trackDecodingFinished);
    connect(m_track, &Track::beatsPerMinuteChanged, this, [this] { viewport()->update(); });
    connect(m_track, &Track::offsetChanged, this, [this] { viewport()->update(); });
    connect(m_track, &Track::eventTracksChanged, this, [this] { viewport()->update(); });
    auto addEvent = [this](const Track::Event *event) {
        auto *item = new EventItem(m_track, event);
        m_eventItems[event] = item;
        scene()->addItem(item);
    };
    connect(m_track, &Track::eventAdded, this, addEvent);
    connect(m_track, &Track::eventAboutToBeRemoved, this, [this](const Track::Event *event) {
        auto it = m_eventItems.find(event);
        if (it == m_eventItems.end())
            return;
        auto *item = it->second;
        scene()->removeItem(item);
        delete item;
    });
    auto removeAllEvents = [this] {
        for (auto &[event, item] : m_eventItems) {
            scene()->removeItem(item);
            delete item;
        }
    };
    connect(m_track, &Track::eventsReset, this, [this, removeAllEvents, addEvent] {
        removeAllEvents();
        for (const auto *event : m_track->events())
            addEvent(event);
    });

    auto *deleteAction = new QAction(this);
    deleteAction->setShortcuts({ QKeySequence::Delete, Qt::Key_Backspace });
    connect(deleteAction, &QAction::triggered, this, [this] {
        const auto selectedItems = scene()->selectedItems();
        for (const auto *item : selectedItems) {
            auto it = std::find_if(m_eventItems.begin(), m_eventItems.end(),
                                   [item](const auto &entry) {
                                       return entry.second == item;
                                   });
            if (it != m_eventItems.end()) {
                const auto *event = it->first;
                m_track->removeEvent(event);
            }
        }
    });
    addAction(deleteAction);

    m_markerItem = new MarkerItem(m_track);
    m_markerItem->setPen(QPen(Qt::red, 8));
    m_markerItem->setVisible(m_track->isValid());
    m_markerItem->setLine(HorizMargin, 0, HorizMargin + WaveformWidth + EventAreaWidth, 0);
    scene()->addItem(m_markerItem);

    updatePlaybackPosition();
    connect(m_track, &Track::playbackPositionChanged, this, &TrackView::updatePlaybackPosition);
}

void TrackView::updatePlaybackPosition()
{
    if (m_markerItem->updatingTrackPosition())
        return;
    const auto y = -PixelsPerSecond * m_track->playbackPosition();
    m_markerItem->setPos(0, y);
    centerOn(sceneRect().center().x(), y);
}

void TrackView::trackDecodingFinished()
{
    auto *worker = new WaveformRenderer(m_track, QSize(static_cast<int>(WaveformWidth), WaveformTileHeight), PixelsPerSecond, this);
    connect(worker, &WaveformRenderer::tileReady, this, [this, worker](const QPixmap &pixmap) {
        m_waveformTiles.push_back(pixmap);
        viewport()->update();
    });
    connect(worker, &WaveformRenderer::finished, worker, &QObject::deleteLater);
    worker->start();

    m_markerItem->setVisible(true);

    adjustSceneRect();
}

void TrackView::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->fillRect(rect, Qt::darkBlue);

    if (m_track->sampleCount() == 0)
        return;

    const auto duration = m_track->duration();
    const auto trackHeight = duration * PixelsPerSecond;

    const auto visibleSceneRect = mapToScene(viewport()->rect()).boundingRect();
    const auto yStart = std::max(static_cast<float>(-visibleSceneRect.bottomRight().y()), 0.0f);
    const auto yEnd = std::min(static_cast<float>(-visibleSceneRect.topLeft().y()), trackHeight);

    painter->save();

    painter->setPen(Qt::white);

    // waveform
    {
        auto tileIndex = static_cast<int>(yStart) / WaveformTileHeight;
        int y = -tileIndex * WaveformTileHeight;
        while (tileIndex < m_waveformTiles.size() && y > -yEnd) {
            const auto &tile = m_waveformTiles[tileIndex];
            painter->drawPixmap(HorizMargin, y - tile.height(), tile);
            y -= tile.height();
            ++tileIndex;
        }
    }

    const auto snapTime = [](float t, float interval) {
        return t - std::fmod(t, interval);
    };

    // time marks
    {
        constexpr auto TickWidth = 5.0f;
        constexpr auto TickInterval = 0.5f;

        const auto startT = snapTime(yStart / PixelsPerSecond, TickInterval);
        const auto endT = snapTime(yEnd / PixelsPerSecond, TickInterval) + TickInterval;

        for (float t = startT; t < endT; t += TickInterval) {
            const float y = -PixelsPerSecond * t;
            painter->drawLine(QPointF(HorizMargin, y), QPointF(HorizMargin - TickWidth, y));

            const auto text = QString::number(t, 'f', 2);

            auto textRect = QRectF(painter->fontMetrics().boundingRect(text));
            textRect.adjust(-2, -2, 2, 2);
            textRect.moveBottomRight(QPointF(HorizMargin - TickWidth - 2, y + 0.5f * textRect.height()));
            painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignRight, text);
        }
    }

    // grid
    {
        const auto drawVerticalLine = [painter, trackHeight](float x) {
            painter->drawLine(QPointF(x, 0), QPointF(x, -trackHeight));
        };

        drawVerticalLine(HorizMargin);
        drawVerticalLine(HorizMargin + WaveformWidth);
        drawVerticalLine(HorizMargin + WaveformWidth + EventAreaWidth);

        painter->setPen(Qt::gray);

        for (int i = 0, eventTracks = m_track->eventTracks(); i < eventTracks; ++i) {
            const float x = HorizMargin + WaveformWidth + (i + 1) * EventAreaWidth / (eventTracks + 1);
            drawVerticalLine(x);
        }

        const auto secondsPerBeat = 60.0f / m_track->beatsPerMinute();
        const auto secondsPerDivison = secondsPerBeat / m_track->beatDivisor();
        const auto tOffset = std::fmod(m_track->offset(), secondsPerBeat);
        const auto startT = snapTime(yStart / PixelsPerSecond, secondsPerBeat) + tOffset;
        const auto endT = snapTime(yEnd / PixelsPerSecond, secondsPerBeat) + secondsPerBeat + tOffset;

        const auto drawHorizontalLine = [painter](float t) {
            const float y = -PixelsPerSecond * t;
            painter->drawLine(QPointF(HorizMargin + WaveformWidth, y), QPointF(HorizMargin + WaveformWidth + EventAreaWidth, y));
        };

        for (float t = startT; t < endT; t += secondsPerBeat) {
            painter->setPen(QPen(Qt::gray, 2));
            drawHorizontalLine(t);

            painter->setPen(Qt::darkGray);
            for (int i = 1; i < m_track->beatDivisor(); ++i) {
                drawHorizontalLine(t + i * secondsPerDivison);
            }
        }
    }

    painter->restore();

    QGraphicsView::drawBackground(painter, rect);
}

void TrackView::adjustSceneRect()
{
    const auto trackHeight = m_track->duration() * PixelsPerSecond;

    const auto topLeft = QPointF(0, -trackHeight - VertMargin);
    const auto bottomRight = QPointF(2 * HorizMargin + WaveformWidth + EventAreaWidth, VertMargin);
    scene()->setSceneRect(QRectF(topLeft, bottomRight));

    viewport()->update();
}

void TrackView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && (event->modifiers() & Qt::ShiftModifier)) {
        if (m_track->isValid()) {
            const auto scenePos = mapToScene(event->pos());
            const auto eventTracks = m_track->eventTracks();
            const auto eventTrackWidth = EventAreaWidth / (eventTracks + 1);
            const auto xLeft = HorizMargin + WaveformWidth + 0.5f * eventTrackWidth;
            if (scenePos.x() > xLeft) {
                const auto trackIndex = static_cast<int>((scenePos.x() - xLeft) / eventTrackWidth);
                if (trackIndex < eventTracks) {
                    const float t = -scenePos.y() / PixelsPerSecond;
                    m_track->addTapEvent(trackIndex, t);
                }
            }
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void TrackView::keyPressEvent(QKeyEvent *event)
{
    if (m_track->isPlaying()) {
        auto addEvent = [this](int trackIndex) {
            m_track->addTapEvent(trackIndex, m_track->playbackPosition());
        };
        switch (event->key()) {
        case Qt::Key_D:
            addEvent(0);
            break;
        case Qt::Key_F:
            addEvent(1);
            break;
        case Qt::Key_J:
            addEvent(2);
            break;
        case Qt::Key_K:
            addEvent(3);
        default:
            break;
        }
    }
    QGraphicsView::keyPressEvent(event);
}
