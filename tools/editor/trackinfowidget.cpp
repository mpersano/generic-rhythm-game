#include "trackinfowidget.h"

#include "track.h"

#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

TrackInfoWidget::TrackInfoWidget(Track *track, QWidget *parent)
    : QWidget(parent)
    , m_track(track)
    , m_file(new QLabel(this))
    , m_duration(new QLabel(this))
    , m_rate(new QLabel(this))
    , m_eventTracks(new QSpinBox(this))
    , m_beatsPerMinute(new QDoubleSpinBox(this))
    , m_play(new QPushButton(tr("Play"), this))
    , m_stop(new QPushButton(tr("Stop"), this))
{
    auto *layout = new QVBoxLayout(this);

    auto *formLayout = new QFormLayout;
    formLayout->addRow(tr("File"), m_file);
    formLayout->addRow(tr("Duration"), m_duration);
    formLayout->addRow(tr("Rate"), m_rate);
    formLayout->addRow(tr("Event tracks"), m_eventTracks);
    formLayout->addRow(tr("Beats per minute"), m_beatsPerMinute);
    layout->addLayout(formLayout);

    auto *playLayout = new QHBoxLayout(this);
    playLayout->addWidget(m_play);
    playLayout->addWidget(m_stop);
    layout->addLayout(playLayout);

    auto updateFileName = [this] {
        m_file->setText(QFileInfo(m_track->audioFile()).fileName());
    };
    updateFileName();
    connect(m_track, &Track::audioFileChanged, this, updateFileName);

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
    m_beatsPerMinute->setDecimals(2);
    m_beatsPerMinute->setValue(m_track->beatsPerMinute());
    connect(m_track, &Track::beatsPerMinuteChanged, m_beatsPerMinute, &QDoubleSpinBox::setValue);
    connect(m_beatsPerMinute, qOverload<double>(&QDoubleSpinBox::valueChanged), m_track, &Track::setBeatsPerMinute);

    connect(m_play, &QPushButton::clicked, m_track, &Track::startPlayback);
    connect(m_stop, &QPushButton::clicked, m_track, &Track::stopPlayback);
}

TrackInfoWidget::~TrackInfoWidget() = default;
