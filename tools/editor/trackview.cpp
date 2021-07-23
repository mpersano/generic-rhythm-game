#include "trackview.h"

#include "track.h"

#include <QGraphicsItem>

#include <cmath>

namespace {
constexpr auto HorizMargin = 20.0f;
constexpr auto VertMargin = 20.0f;
constexpr auto WaveformWidth = 200.0f;
constexpr auto EventAreaWidth = 500.0f;
constexpr auto PixelsPerSecond = 150.0f;

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

QBrush trackBrush(int trackIndex)
{
    static const std::vector<QBrush> brushes = {
        Qt::red, Qt::yellow, Qt::cyan, Qt::magenta, Qt::green, Qt::gray
    };
    return brushes[trackIndex];
}

} // namespace

class EventItem : public QGraphicsItem
{
public:
    EventItem(const Track *track, const Track::Event *event, QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    static constexpr const auto Radius = 10.0;

    const Track *m_track;
    const Track::Event *m_event;
};

EventItem::EventItem(const Track *track, const Track::Event *event, QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , m_track(track)
    , m_event(event)
{
    setFlag(ItemIsMovable);
    setFlag(ItemSendsGeometryChanges);
    setPos(eventPosition(track, event->track, event->start));
}

QRectF EventItem::boundingRect() const
{
    return QRectF(-Radius, -Radius, 2.0f * Radius, 2.0f * Radius);
}

QVariant EventItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange) {
        // stay on your lane!
        QPointF updatePos = value.toPointF();
        updatePos.setX(trackX(m_track, m_event->track));
        return updatePos;
    }
    return value;
}

void EventItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * /* widget */)
{
    painter->setPen(Qt::NoPen);
    painter->setBrush(trackBrush(m_event->track));
    painter->drawEllipse(boundingRect());
}

TrackView::TrackView(Track *track, QWidget *parent)
    : QGraphicsView(parent)
    , m_track(track)
{
    setScene(new QGraphicsScene(this));
    setRenderHint(QPainter::Antialiasing);
    connect(m_track, &Track::decodingFinished, this, &TrackView::adjustSceneRect);
    connect(m_track, &Track::beatsPerMinuteChanged, this, [this] { viewport()->update(); });
    connect(m_track, &Track::eventTracksChanged, this, [this] { viewport()->update(); });
    connect(m_track, &Track::eventAdded, this, [this](const Track::Event *event) {
        auto *item = new EventItem(m_track, event);
        m_eventItems[event] = item;
        scene()->addItem(item);
    });
    connect(m_track, &Track::eventAboutToBeRemoved, this, [this](const Track::Event *event) {
        auto it = m_eventItems.find(event);
        if (it == m_eventItems.end())
            return;
        auto *item = it->second;
        scene()->removeItem(item);
        delete item;
    });
}

void TrackView::drawBackground(QPainter *painter, const QRectF &rect)
{
    if (m_track->sampleCount() == 0)
        return;

    const auto duration = m_track->duration();
    const auto trackHeight = duration * PixelsPerSecond;

    const auto visibleSceneRect = mapToScene(viewport()->rect()).boundingRect();
    const auto yStart = std::max(static_cast<float>(-visibleSceneRect.bottomRight().y()), 0.0f);
    const auto yEnd = std::min(static_cast<float>(-visibleSceneRect.topLeft().y()), trackHeight);

    painter->save();
    painter->setRenderHints(QPainter::Antialiasing, true);

    painter->fillRect(rect, Qt::darkBlue);
    painter->setPen(Qt::white);

    // waveform
    {
        const auto *samples = m_track->samples();
        const auto sampleCount = m_track->sampleCount();

        const auto sampleStart = std::max(static_cast<int>((yStart / PixelsPerSecond) * m_track->rate()), 0);
        const auto sampleEnd = std::min(static_cast<int>((yEnd / PixelsPerSecond) * m_track->rate()), sampleCount - 1);

        const auto dySample = PixelsPerSecond / m_track->rate();

        const auto maxSample = [samples, sampleCount] {
            const auto it = std::max_element(samples, samples + sampleCount, [](float lhs, float rhs) {
                return std::abs(lhs) < std::abs(rhs);
            });
            return std::abs(*it);
        }();
        const auto amplitude = 0.5f * WaveformWidth / (1.1f * maxSample);

        std::vector<QPointF> polyline;
        polyline.reserve(sampleEnd - sampleStart + 1);

        for (size_t i = sampleStart; i <= sampleEnd; ++i) {
            const auto sample = samples[i];

            constexpr auto Amplitude = 0.5f * WaveformWidth;
            constexpr auto CenterX = HorizMargin + Amplitude;

            const auto x = CenterX + sample * amplitude;
            const auto y = -static_cast<float>(i) * dySample;
            polyline.emplace_back(x, y);
        }

        painter->drawPolyline(polyline.data(), polyline.size());
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

        painter->setPen(Qt::darkGray);

        const auto eventTracks = m_track->eventTracks();
        for (int i = 0; i < eventTracks; ++i) {
            const float x = HorizMargin + WaveformWidth + (i + 1) * EventAreaWidth / (eventTracks + 1);
            drawVerticalLine(x);
        }

        const auto secondsPerBeat = 60.0f / m_track->beatsPerMinute();

        const auto startT = snapTime(yStart / PixelsPerSecond, secondsPerBeat);
        const auto endT = snapTime(yEnd / PixelsPerSecond, secondsPerBeat) + secondsPerBeat;

        for (float t = startT; t < endT; t += secondsPerBeat) {
            const float y = -PixelsPerSecond * t;
            painter->drawLine(QPointF(HorizMargin + WaveformWidth, y), QPointF(HorizMargin + WaveformWidth + EventAreaWidth, y));
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
