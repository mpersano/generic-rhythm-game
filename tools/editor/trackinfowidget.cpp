#include "trackinfowidget.h"

#include "track.h"

#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>

TrackInfoWidget::TrackInfoWidget(Track *track, QWidget *parent)
    : QWidget(parent)
    , m_track(track)
    , m_file(new QLabel(this))
    , m_duration(new QLabel(this))
    , m_rate(new QLabel(this))
    , m_eventTracks(new QSpinBox(this))
    , m_beatsPerMinute(new QSpinBox(this))
{
    auto *layout = new QFormLayout(this);
    layout->addRow(QObject::tr("File"), m_file);
    layout->addRow(QObject::tr("Duration"), m_duration);
    layout->addRow(QObject::tr("Rate"), m_rate);
    layout->addRow(QObject::tr("Event tracks"), m_eventTracks);
    layout->addRow(QObject::tr("Beats per minute"), m_beatsPerMinute);

    auto updateFileName = [this] {
        m_file->setText(QFileInfo(m_track->fileName()).fileName());
    };
    updateFileName();
    connect(m_track, &Track::fileNameChanged, this, updateFileName);

    auto updateDuration = [this] {
        m_duration->setText(QObject::tr("%1 s").arg(m_track->duration(), 0, 'f', 2));
    };
    updateDuration();
    connect(m_track, &Track::durationChanged, this, updateDuration);

    auto updateRate = [this] {
        m_rate->setText(QObject::tr("%1 Hz").arg(m_track->rate()));
    };
    updateRate();
    connect(m_track, &Track::rateChanged, this, updateRate);

    m_eventTracks->setRange(1, 12);
    m_eventTracks->setValue(m_track->eventTracks());
    connect(m_track, &Track::eventTracksChanged, m_eventTracks, &QSpinBox::setValue);
    connect(m_eventTracks, qOverload<int>(&QSpinBox::valueChanged), m_track, &Track::setEventTracks);

    m_beatsPerMinute->setRange(10, 2000);
    m_beatsPerMinute->setValue(m_track->beatsPerMinute());
    connect(m_track, &Track::beatsPerMinuteChanged, m_beatsPerMinute, &QSpinBox::setValue);
    connect(m_beatsPerMinute, qOverload<int>(&QSpinBox::valueChanged), m_track, &Track::setBeatsPerMinute);
}

TrackInfoWidget::~TrackInfoWidget() = default;
