#include "channelstrip.h"
#include "eqgraphwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QDial>
#include <QFrame>
#include <QProgressBar>
#include <QDebug>
#include <cmath>

/*
 * ChannelStrip Widget
 * This custom widget represents a single vertical channel strip
 * in the mixer.
 */
ChannelStrip::ChannelStrip(int channelId, QWidget *parent)
    : QWidget(parent), m_channelId(channelId), m_fader(nullptr), m_meter(nullptr)
{
    this->setObjectName("ChannelStrip");

    // --- Main Layout ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // --- Processing Section (Top) ---
    QFrame *processingFrame = new QFrame();
    processingFrame->setObjectName("processingFrame");
    QVBoxLayout *processingLayout = new QVBoxLayout(processingFrame);
    processingLayout->setSpacing(2);
    processingLayout->setContentsMargins(2, 2, 2, 2);

    // 1. Comp Placeholder
    QFrame *compPlaceholder = new QFrame();
    compPlaceholder->setObjectName("compPlaceholder");
    compPlaceholder->setMinimumHeight(30);
    compPlaceholder->setFrameShape(QFrame::StyledPanel);
    compPlaceholder->setFrameShadow(QFrame::Sunken);
    QVBoxLayout *compLayout = new QVBoxLayout(compPlaceholder);
    compLayout->setContentsMargins(5, 5, 5, 5);
    QLabel *compLabel = new QLabel("COMP");
    compLabel->setObjectName("compLabel");
    compLayout->addWidget(compLabel, 0, Qt::AlignLeft | Qt::AlignTop);
    compLayout->addStretch();
    processingLayout->addWidget(compPlaceholder);

    // 2. EQ Graph Widget
    QFrame *eqContainer = new QFrame();
    eqContainer->setObjectName("eqPlaceholder");
    eqContainer->setMinimumHeight(30);

    QVBoxLayout *eqLayout = new QVBoxLayout(eqContainer);
    eqLayout->setContentsMargins(5, 5, 5, 5);
    eqLayout->setSpacing(2);

    QLabel *eqLabel = new QLabel("EQ");
    eqLabel->setObjectName("eqLabel");
    eqLayout->addWidget(eqLabel, 0, Qt::AlignLeft | Qt::AlignTop);

    m_eqGraph = new EqGraphWidget();
    m_eqGraph->setSimpleMode(true);
    eqLayout->addWidget(m_eqGraph, 1);

    processingLayout->addWidget(eqContainer);

    // 3. Aux Sends
    QFrame *auxFrame = new QFrame();
    auxFrame->setObjectName("auxFrame");
    QVBoxLayout *auxLayout = new QVBoxLayout(auxFrame);
    auxLayout->setSpacing(1);
    auxLayout->setContentsMargins(0, 0, 0, 0);
    for (int i = 0; i < 4; ++i) {
        QSlider *auxSlider = new QSlider(Qt::Horizontal);
        auxSlider->setObjectName("auxSlider");
        auxLayout->addWidget(auxSlider);
    }
    processingLayout->addWidget(auxFrame);

    mainLayout->addWidget(processingFrame);

    // 4. Pan Dial
    QDial *panDial = new QDial();
    panDial->setObjectName("panDial");
    panDial->setNotchesVisible(true);
    panDial->setRange(-100, 100);
    panDial->setValue(0);
    panDial->setFixedSize(40, 40);

    QHBoxLayout* panLayout = new QHBoxLayout();
    panLayout->addStretch();
    panLayout->addWidget(panDial);
    panLayout->addStretch();
    mainLayout->addLayout(panLayout);

    // 5. Main Channel Buttons
    m_selectButton = new QPushButton("SELECT");
    m_selectButton->setCheckable(true);
    m_selectButton->setObjectName("selectButton");
    connect(m_selectButton, &QPushButton::clicked, this, &ChannelStrip::onSelectClicked);

    m_soloButton = new QPushButton("SOLO");
    m_soloButton->setCheckable(true);
    m_soloButton->setObjectName("soloButton");

    m_muteButton = new QPushButton("MUTE");
    m_muteButton->setCheckable(true);
    m_muteButton->setObjectName("muteButton");

    mainLayout->addWidget(m_selectButton);
    mainLayout->addWidget(m_soloButton);
    mainLayout->addWidget(m_muteButton);

    // 6. Channel Label
    QLabel *channelLabel = new QLabel(QString("Ch %1").arg(m_channelId));
    channelLabel->setAlignment(Qt::AlignCenter);
    channelLabel->setObjectName("channelLabel");
    mainLayout->addWidget(channelLabel);

    // 7. Fader and Meter
    QHBoxLayout *faderLayout = new QHBoxLayout();
    faderLayout->setSpacing(5);

    // Main Fader
    m_fader = new QSlider(Qt::Vertical);
    m_fader->setObjectName("mainFader");
    m_fader->setRange(0, 1000); // Higher resolution for smoother movement
    m_fader->setValue(750); // Start at unity gain (0 dB)
    m_fader->setMinimumHeight(200);
    connect(m_fader, &QSlider::valueChanged, this, &ChannelStrip::onFaderValueChanged);
    faderLayout->addWidget(m_fader, 3);

    // Level Meter
    m_meter = new QProgressBar();
    m_meter->setOrientation(Qt::Vertical);
    m_meter->setRange(0, 100);
    m_meter->setValue(0);
    m_meter->setTextVisible(false);
    m_meter->setObjectName("levelMeter");
    faderLayout->addWidget(m_meter, 1);

    mainLayout->addLayout(faderLayout);

    // Channel Number at bottom
    QLabel *bottomLabel = new QLabel(QString::number(m_channelId));
    bottomLabel->setAlignment(Qt::AlignCenter);
    bottomLabel->setObjectName("bottomLabel");
    mainLayout->addWidget(bottomLabel);

    setLayout(mainLayout);
}

int ChannelStrip::getChannelId() const
{
    return m_channelId;
}

void ChannelStrip::setVolumeFromReaper(double volume)
{
    // Block signals to prevent feedback loop
    m_fader->blockSignals(true);

    int faderPos = volumeToFaderPosition(volume);
    m_fader->setValue(faderPos);

    // Update meter based on volume (simplified)
    int meterValue = static_cast<int>(volume * 25.0); // Scale to 0-100
    m_meter->setValue(qMin(100, meterValue));

    m_fader->blockSignals(false);

    qDebug() << "ChannelStrip: Set volume from REAPER - Track:" << m_channelId
             << "Volume:" << volume << "FaderPos:" << faderPos;
}

void ChannelStrip::setSelected(bool selected)
{
    m_selectButton->blockSignals(true);
    m_selectButton->setChecked(selected);
    m_selectButton->blockSignals(false);
}

void ChannelStrip::onSelectClicked()
{
    emit channelSelected(m_channelId);
}

void ChannelStrip::onFaderValueChanged(int value)
{
    double volume = faderPositionToVolume(value);
    emit volumeChanged(m_channelId, volume);

    // Update meter
    int meterValue = static_cast<int>(volume * 25.0);
    m_meter->setValue(qMin(100, meterValue));

    qDebug() << "ChannelStrip: Fader moved - Track:" << m_channelId
             << "Position:" << value << "Volume:" << volume;
}

int ChannelStrip::volumeToFaderPosition(double volume)
{
    // Convert linear volume (0.0-4.0) to fader position (0-1000)
    // REAPER uses 0.0-4.0 range where 1.0 = 0 dB
    if (volume <= 0.0) return 0;

    // Simple linear mapping for now - can be improved with logarithmic scaling
    double normalized = qBound(0.0, volume / 4.0, 1.0);
    return static_cast<int>(normalized * 1000);
}

double ChannelStrip::faderPositionToVolume(int position)
{
    // Convert fader position (0-1000) to linear volume (0.0-4.0)
    double normalized = static_cast<double>(position) / 1000.0;
    return normalized * 4.0;
}

void ChannelStrip::updateEqBand(int bandIndex, double freq, double gain, double q)
{
    if (m_eqGraph) {
        m_eqGraph->setBandParameters(bandIndex, freq, gain, q);
    }
}

void ChannelStrip::updateEqEnabled(int bandIndex, bool enabled)
{
    if (m_eqGraph) {
        m_eqGraph->setBandEnabled(bandIndex, enabled);
    }
}
