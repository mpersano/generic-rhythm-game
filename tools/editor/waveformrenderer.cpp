#include "waveformrenderer.h"

#include "track.h"

#include <QImage>
#include <QPainter>

WaveformRenderer::WaveformRenderer(const Track *track, const QSize &tileSize, float pixelsPerSecond, QObject *parent)
    : QThread(parent)
    , m_track(track)
    , m_tileSize(tileSize)
    , m_pixelsPerSecond(pixelsPerSecond)
{
}

void WaveformRenderer::run()
{
    const auto totalHeight = static_cast<int>(m_track->duration() * m_pixelsPerSecond);

    const auto *samples = m_track->samples();
    const auto sampleCount = m_track->sampleCount();

    const auto dySample = m_pixelsPerSecond / m_track->rate();

    const auto maxSample = [samples, sampleCount] {
        const auto it = std::max_element(samples, samples + sampleCount, [](float lhs, float rhs) {
            return std::abs(lhs) < std::abs(rhs);
        });
        return std::abs(*it);
    }();

    const auto amplitude = 0.5f * m_tileSize.width() / (1.1f * maxSample);
    const auto centerX = 0.5f * m_tileSize.width();

    for (int yStart = 0; yStart < totalHeight; yStart += m_tileSize.height()) {
        const auto yEnd = std::min(yStart + m_tileSize.height(), totalHeight);

        const auto sampleStart = std::max(static_cast<int>((yStart / m_pixelsPerSecond) * m_track->rate()), 0);
        const auto sampleEnd = std::min(static_cast<int>((yEnd / m_pixelsPerSecond) * m_track->rate()), sampleCount - 1);

        const auto tileHeight = yEnd - yStart;
        QImage image(m_tileSize.width(), tileHeight, QImage::Format_ARGB32);
        image.fill(Qt::transparent);

        std::vector<QPointF> polyline;
        polyline.reserve(sampleCount);

        for (size_t i = sampleStart; i <= sampleEnd; ++i) {
            const auto sample = samples[i];
            const auto x = centerX + sample * amplitude;
            const auto y = tileHeight - static_cast<float>(i - sampleStart) * dySample;
            polyline.emplace_back(x, y);
        }

        QPainter painter(&image);
        painter.setRenderHints(QPainter::Antialiasing, true);
        painter.setPen(Qt::white);
        painter.drawPolyline(polyline.data(), polyline.size());

        emit tileReady(QPixmap::fromImage(image));
    }
}
