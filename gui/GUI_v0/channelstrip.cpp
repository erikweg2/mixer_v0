#include "channelstrip.h"
#include "eqgraphwidget.h" // Make sure this is included

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QDial>
#include <QFrame>
#include <QProgressBar>

/*
 * ChannelStrip Widget
 * This custom widget represents a single vertical channel strip
 * in the mixer.
 */
ChannelStrip::ChannelStrip(int channelId, QWidget *parent)
    : QWidget(parent), m_channelId(channelId)
{
    // Use the class name as the object name for top-level CSS
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

    // 1. Comp Placeholder (MODIFIED - now includes "COMP" text)
    QFrame *compPlaceholder = new QFrame();
    compPlaceholder->setObjectName("compPlaceholder");
    compPlaceholder->setMinimumHeight(30);
    compPlaceholder->setFrameShape(QFrame::StyledPanel);
    compPlaceholder->setFrameShadow(QFrame::Sunken);
    QVBoxLayout *compLayout = new QVBoxLayout(compPlaceholder);
    compLayout->setContentsMargins(5, 5, 5, 5);
    QLabel *compLabel = new QLabel("COMP");
    compLabel->setObjectName("compLabel"); // Added object name for styling
    compLayout->addWidget(compLabel, 0, Qt::AlignLeft | Qt::AlignTop); // Align text to top-left
    compLayout->addStretch(); // Push label to top
    processingLayout->addWidget(compPlaceholder);

    // 2. EQ Graph Widget (FIXED)

    // --- NEW: Create a container frame for the EQ section ---
    QFrame *eqContainer = new QFrame();
    eqContainer->setObjectName("eqPlaceholder"); // Use same object name for styling
    eqContainer->setMinimumHeight(30);

    // --- NEW: Create a layout for this container ---
    QVBoxLayout *eqLayout = new QVBoxLayout(eqContainer);
    eqLayout->setContentsMargins(5, 5, 5, 5);
    eqLayout->setSpacing(2); // Add some spacing

    // --- NEW: Add the label to the container's layout ---
    QLabel *eqLabel = new QLabel("EQ");
    eqLabel->setObjectName("eqLabel");
    eqLayout->addWidget(eqLabel, 0, Qt::AlignLeft | Qt::AlignTop);

    // --- NEW: Instantiate and add the graph widget to the layout ---
    m_eqGraph = new EqGraphWidget();
    m_eqGraph->setSimpleMode(true);
    // Add the graph with a stretch factor so it fills remaining space
    eqLayout->addWidget(m_eqGraph, 1);

    // --- NEW: Add the container (with its label and graph) to the main layout ---
    processingLayout->addWidget(eqContainer);


    // 3. Aux Sends
    QFrame *auxFrame = new QFrame();
    auxFrame->setObjectName("auxFrame");
    QVBoxLayout *auxLayout = new QVBoxLayout(auxFrame);
    auxLayout->setSpacing(1);
    auxLayout->setContentsMargins(0, 0, 0, 0);
    for (int i = 0; i < 4; ++i) { // 4 aux sends as an example
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
    QSlider *fader = new QSlider(Qt::Vertical);
    fader->setObjectName("mainFader");
    fader->setRange(0, 100);
    fader->setValue(75);
    fader->setMinimumHeight(200);
    faderLayout->addWidget(fader, 3); // Give fader more stretch

    // Level Meter (using QProgressBar)
    QProgressBar *meter = new QProgressBar();
    meter->setOrientation(Qt::Vertical);
    meter->setRange(0, 100);
    meter->setValue(0); // Set to 0, would be updated by audio data
    meter->setTextVisible(false);
    meter->setObjectName("levelMeter");
    faderLayout->addWidget(meter, 1); // Fader is 3x wider than meter

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

// --- ADDED MISSING FUNCTION IMPLEMENTATIONS ---

void ChannelStrip::updateEqBand(int bandIndex, double freq, double gain, double q)
{
    if (m_eqGraph) {
        // Pass the data to the mini-graph
        m_eqGraph->setBandParameters(bandIndex, freq, gain, q);
    }
}

void ChannelStrip::updateEqEnabled(int bandIndex, bool enabled)
{
    if (m_eqGraph) {
        // Pass the data to the mini-graph
        m_eqGraph->setBandEnabled(bandIndex, enabled);
    }
}
