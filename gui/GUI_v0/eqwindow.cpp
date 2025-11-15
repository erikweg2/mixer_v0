#include "eqwindow.h"
#include "colorringdial.h"
#include "eqgraphwidget.h" // Include the custom graph widget
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QGroupBox>
#include <QDebug>
#include <QDial> // Still needed for createKnobWithLabel

// --- Helper for creating a single EQ band UI ---
QWidget* EqWindow::createEqBand(const QString &name, const QColor &color,
                                ColorRingDial* &freqDial,
                                ColorRingDial* &gainDial,
                                ColorRingDial* &qDial)
{
    QWidget *bandWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(bandWidget);
    layout->setSpacing(5);
    QLabel *nameLabel = new QLabel(name);
    nameLabel->setObjectName("EqBandNameLabel");
    nameLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(nameLabel);
    QHBoxLayout *dialsLayout = new QHBoxLayout;
    auto createDialColumn = [&](const QString& label, ColorRingDial* &dial) {
        QVBoxLayout* col = new QVBoxLayout;
        col->setSpacing(2);
        QLabel* lbl = new QLabel(label);
        lbl->setObjectName("KnobCaptionLabel");
        lbl->setAlignment(Qt::AlignCenter);
        dial = new ColorRingDial;
        dial->setBandColor(color);
        col->addWidget(lbl, 0, Qt::AlignCenter);
        col->addWidget(dial, 0, Qt::AlignCenter);
        return col;
    };
    dialsLayout->addLayout(createDialColumn("FREQ", freqDial));
    dialsLayout->addLayout(createDialColumn("GAIN", gainDial));
    dialsLayout->addLayout(createDialColumn("Q", qDial));
    layout->addLayout(dialsLayout);
    bandWidget->setObjectName("EqBandWidget");
    return bandWidget;
}

// Helper for Filter/Gate/Comp knobs (standard QDial)
QWidget* EqWindow::createKnobWithLabel(const QString& labelText, QDial*& dial, int value)
{
    QWidget *container = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    QLabel *captionLabel = new QLabel(labelText);
    captionLabel->setObjectName("KnobCaptionLabel");
    captionLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(captionLabel);
    dial = new QDial;
    dial->setFixedSize(60, 60);
    dial->setRange(0, 100);
    dial->setValue(value);
    layout->addWidget(dial, 0, Qt::AlignCenter);
    QLabel *valueLabel = new QLabel(QString::number(value));
    valueLabel->setObjectName("KnobValueLabel");
    valueLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(valueLabel);
    connect(dial, &QDial::valueChanged, valueLabel, [valueLabel](int val) {
        valueLabel->setText(QString::number(val));
    });
    return container;
}


// --- Main Constructor ---
EqWindow::EqWindow(QWidget *parent)
    : QWidget(parent)
{
    this->setObjectName("EqWindow");

    // --- Main Layout ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(5);

    // --- Top Bar (Title and In/Out Placeholder) ---
    QHBoxLayout *topBarLayout = new QHBoxLayout;

    m_titleLabel = new QLabel("FAT CHANNEL");
    m_titleLabel->setObjectName("FatChannelTitleLabel");
    QLabel *inOutLabel = new QLabel("In 1");
    inOutLabel->setObjectName("InOutLabel");
    QPushButton *backButton = new QPushButton("Back");
    backButton->setObjectName("SmallButton");
    connect(backButton, &QPushButton::clicked, this, &EqWindow::backClicked);
    topBarLayout->addWidget(m_titleLabel);
    topBarLayout->addStretch(1);
    topBarLayout->addWidget(inOutLabel);
    topBarLayout->addWidget(backButton);
    mainLayout->addLayout(topBarLayout);
    mainLayout->addSpacing(10);

    // --- Main Processing Sections (Horizontal) ---
    QHBoxLayout *processingSectionsLayout = new QHBoxLayout;
    processingSectionsLayout->setSpacing(10);

    // --- 1. Filter Section ---
    QGroupBox *filterBox = new QGroupBox("FILTER");
    filterBox->setObjectName("ModuleBox");
    QVBoxLayout *filterLayout = new QVBoxLayout(filterBox);
    filterLayout->setContentsMargins(5, 15, 5, 5);
    filterLayout->setSpacing(5);
    QDial *filterFreqDial;
    filterLayout->addWidget(createKnobWithLabel("FREQ", filterFreqDial, 18));
    QPushButton *filterPhaseButton = new QPushButton("Phase");
    filterPhaseButton->setCheckable(true);
    filterPhaseButton->setObjectName("SmallToggleButton");
    filterLayout->addWidget(filterPhaseButton);
    filterLayout->addStretch(1);
    filterBox->setMinimumWidth(120);
    filterBox->setMaximumWidth(120);
    processingSectionsLayout->addWidget(filterBox);

    // --- 2. Noise Gate Section ---
    QGroupBox *noiseGateBox = new QGroupBox("NOISE GATE");
    noiseGateBox->setObjectName("ModuleBox");
    QVBoxLayout *noiseGateLayout = new QVBoxLayout(noiseGateBox);
    noiseGateLayout->setContentsMargins(5, 15, 5, 5);
    noiseGateLayout->setSpacing(5);
    QDial *noiseGateThreshDial;
    noiseGateLayout->addWidget(createKnobWithLabel("THRESH", noiseGateThreshDial, 50));
    QFrame *noiseGateGraph = new QFrame;
    noiseGateGraph->setObjectName("GraphPlaceholder");
    noiseGateGraph->setFixedSize(100, 100);
    noiseGateLayout->addWidget(noiseGateGraph, 0, Qt::AlignCenter);
    noiseGateLayout->addStretch(1);
    noiseGateBox->setMinimumWidth(120);
    noiseGateBox->setMaximumWidth(120);
    processingSectionsLayout->addWidget(noiseGateBox);

    // --- 3. Compressor Section ---
    QGroupBox *compressorBox = new QGroupBox("COMPRESSOR");
    compressorBox->setObjectName("ModuleBox");
    QHBoxLayout *compressorInnerLayout = new QHBoxLayout(compressorBox);
    compressorInnerLayout->setContentsMargins(5, 15, 5, 5);
    compressorInnerLayout->setSpacing(5);
    QDial *compThresh, *compAttack, *compRelease, *compRatio, *compGain;
    QVBoxLayout *compLeftCol = new QVBoxLayout;
    compLeftCol->addWidget(createKnobWithLabel("THRESH", compThresh, 50));
    compLeftCol->addWidget(createKnobWithLabel("ATTACK", compAttack, 20));
    compLeftCol->addWidget(createKnobWithLabel("RELEASE", compRelease, 80));
    compLeftCol->addStretch(1);
    compressorInnerLayout->addLayout(compLeftCol);
    QVBoxLayout *compMidCol = new QVBoxLayout;
    QHBoxLayout *compButtons = new QHBoxLayout;
    QPushButton *compressorAutoButton = new QPushButton("Auto");
    compressorAutoButton->setCheckable(true);
    compressorAutoButton->setObjectName("SmallToggleButton");
    QPushButton *compressorLimitButton = new QPushButton("Limit");
    compressorLimitButton->setCheckable(true);
    compressorLimitButton->setObjectName("SmallToggleButton");
    compButtons->addWidget(compressorAutoButton);
    compButtons->addWidget(compressorLimitButton);
    compMidCol->addLayout(compButtons);
    QFrame *compressorGraph = new QFrame;
    compressorGraph->setObjectName("GraphPlaceholder");
    compressorGraph->setFixedSize(120, 120);
    compMidCol->addWidget(compressorGraph, 0, Qt::AlignCenter);
    compMidCol->addStretch(1);
    compressorInnerLayout->addLayout(compMidCol);
    QVBoxLayout *compRightCol = new QVBoxLayout;
    compRightCol->addWidget(createKnobWithLabel("RATIO", compRatio, 10));
    compRightCol->addWidget(createKnobWithLabel("GAIN", compGain, 50));
    compRightCol->addStretch(1);
    compressorInnerLayout->addLayout(compRightCol);
    processingSectionsLayout->addWidget(compressorBox);

    // --- 4. EQ Section (Dynamic Graph) ---
    QGroupBox *eqBox = new QGroupBox("EQ");
    eqBox->setObjectName("ModuleBox");
    QVBoxLayout *eqLayout = new QVBoxLayout(eqBox);
    eqLayout->setContentsMargins(5, 15, 5, 5);
    eqLayout->setSpacing(5);
    m_eqGraph = new EqGraphWidget;
    m_eqGraph->setObjectName("EqGraphPlaceholder");
    m_eqGraph->setMinimumSize(300, 150);
    eqLayout->addWidget(m_eqGraph);
    connect(m_eqGraph, &EqGraphWidget::bandManuallyChanged, this, &EqWindow::onGraphBandChanged);
    m_eqGraph->setBandColor(0, QColor(227, 137, 38));
    m_eqGraph->setBandColor(1, QColor(50, 180, 80));
    m_eqGraph->setBandColor(2, QColor(60, 180, 200));
    m_eqGraph->setBandColor(3, QColor(100, 100, 220));
    QHBoxLayout *eqBandButtonsLayout = new QHBoxLayout;
    m_eqLowButton = new QPushButton("LOW");
    m_eqLowButton->setObjectName("EqBandButton");
    m_eqLowButton->setCheckable(true);
    m_eqLowButton->setChecked(true);
    connect(m_eqLowButton, &QPushButton::toggled, this, &EqWindow::onLowButtonToggled);
    m_eqLoMidButton = new QPushButton("LO MID");
    m_eqLoMidButton->setObjectName("EqBandButton");
    m_eqLoMidButton->setCheckable(true);
    m_eqLoMidButton->setChecked(true);
    connect(m_eqLoMidButton, &QPushButton::toggled, this, &EqWindow::onLoMidButtonToggled);
    m_eqHiMidButton = new QPushButton("HI MID");
    m_eqHiMidButton->setObjectName("EqBandButton");
    m_eqHiMidButton->setCheckable(true);
    m_eqHiMidButton->setChecked(true);
    connect(m_eqHiMidButton, &QPushButton::toggled, this, &EqWindow::onHiMidButtonToggled);
    m_eqHighButton = new QPushButton("HIGH");
    m_eqHighButton->setObjectName("EqBandButton");
    m_eqHighButton->setCheckable(true);
    m_eqHighButton->setChecked(true);
    connect(m_eqHighButton, &QPushButton::toggled, this, &EqWindow::onHighButtonToggled);
    eqBandButtonsLayout->addStretch(1);
    eqBandButtonsLayout->addWidget(m_eqLowButton);
    eqBandButtonsLayout->addWidget(m_eqLoMidButton);
    eqBandButtonsLayout->addWidget(m_eqHiMidButton);
    eqBandButtonsLayout->addWidget(m_eqHighButton);
    eqBandButtonsLayout->addStretch(1);
    eqLayout->addLayout(eqBandButtonsLayout);
    QHBoxLayout *eqKnobsLayout = new QHBoxLayout;
    eqKnobsLayout->setSpacing(10);
    eqKnobsLayout->addWidget(createEqBand("LOW", QColor(227, 137, 38),
                                          m_eqLowFreqDial, m_eqLowGainDial, m_eqLowQDial));
    connect(m_eqLowFreqDial, &QDial::valueChanged, this, &EqWindow::onLowFreqChanged);
    connect(m_eqLowGainDial, &QDial::valueChanged, this, &EqWindow::onLowGainChanged);
    connect(m_eqLowQDial, &QDial::valueChanged, this, &EqWindow::onLowQChanged);
    m_eqLowFreqDial->setValue(1000);
    m_eqLowQDial->setValue(1800); // Q=1.0
    eqKnobsLayout->addWidget(createEqBand("LO MID", QColor(50, 180, 80),
                                          m_eqLoMidFreqDial, m_eqLoMidGainDial, m_eqLoMidQDial));
    connect(m_eqLoMidFreqDial, &QDial::valueChanged, this, &EqWindow::onLoMidFreqChanged);
    connect(m_eqLoMidGainDial, &QDial::valueChanged, this, &EqWindow::onLoMidGainChanged);
    connect(m_eqLoMidQDial, &QDial::valueChanged, this, &EqWindow::onLoMidQChanged);
    m_eqLoMidFreqDial->setValue(1800);
    m_eqLoMidQDial->setValue(1800);
    eqKnobsLayout->addWidget(createEqBand("HI MID", QColor(60, 180, 200),
                                          m_eqHiMidFreqDial, m_eqHiMidGainDial, m_eqHiMidQDial));
    connect(m_eqHiMidFreqDial, &QDial::valueChanged, this, &EqWindow::onHiMidFreqChanged);
    connect(m_eqHiMidGainDial, &QDial::valueChanged, this, &EqWindow::onHiMidGainChanged);
    connect(m_eqHiMidQDial, &QDial::valueChanged, this, &EqWindow::onHiMidQChanged);
    m_eqHiMidFreqDial->setValue(2200);
    m_eqHiMidQDial->setValue(1800);
    eqKnobsLayout->addWidget(createEqBand("HIGH", QColor(100, 100, 220),
                                          m_eqHighFreqDial, m_eqHighGainDial, m_eqHighQDial));
    connect(m_eqHighFreqDial, &QDial::valueChanged, this, &EqWindow::onHighFreqChanged);
    connect(m_eqHighGainDial, &QDial::valueChanged, this, &EqWindow::onHighGainChanged);
    connect(m_eqHighQDial, &QDial::valueChanged, this, &EqWindow::onHighQChanged);
    m_eqHighFreqDial->setValue(2600);
    m_eqHighQDial->setValue(1800);
    eqLayout->addLayout(eqKnobsLayout);
    eqLayout->addStretch(1);
    processingSectionsLayout->addWidget(eqBox, 1);
    mainLayout->addLayout(processingSectionsLayout, 1);
    mainLayout->addStretch(1);
    onLowButtonToggled(m_eqLowButton->isChecked());
    onLoMidButtonToggled(m_eqLoMidButton->isChecked());
    onHiMidButtonToggled(m_eqHiMidButton->isChecked());
    onHighButtonToggled(m_eqHighButton->isChecked());
    onLowFreqChanged(m_eqLowFreqDial->value());
    onLowGainChanged(m_eqLowGainDial->value());
    onLowQChanged(m_eqLowQDial->value());
    onLoMidFreqChanged(m_eqLoMidFreqDial->value());
    onLoMidGainChanged(m_eqLoMidGainDial->value());
    onLoMidQChanged(m_eqLoMidQDial->value());
    onHiMidFreqChanged(m_eqHiMidFreqDial->value());
    onHiMidGainChanged(m_eqHiMidGainDial->value());
    onHiMidQChanged(m_eqHiMidQDial->value());
    onHighFreqChanged(m_eqHighFreqDial->value());
    onHighGainChanged(m_eqHighGainDial->value());
    onHighQChanged(m_eqHighQDial->value());
}

EqWindow::~EqWindow() {}

void EqWindow::showChannel(int channelId)
{
    m_titleLabel->setText(QString("FAT CHANNEL - CH %1").arg(channelId));
}


// --- Implementation of Slots ---

void EqWindow::onGraphBandChanged(int bandIndex, double newFreq, double newGain)
{
    // --- FIX: Use global scope '::' to call inline functions ---
    int freqVal = ::mapFreqToDial(newFreq);
    int gainVal = ::mapGainToDial(newGain);

    switch (bandIndex) {
    case 0:
        m_eqLowFreqDial->blockSignals(true);
        m_eqLowGainDial->blockSignals(true);
        m_eqLowFreqDial->setValue(freqVal);
        m_eqLowGainDial->setValue(gainVal);
        m_eqLowFreqDial->blockSignals(false);
        m_eqLowGainDial->blockSignals(false);
        onLowFreqChanged(freqVal); // This will call the slot that emits
        break;
    case 1:
        m_eqLoMidFreqDial->blockSignals(true);
        m_eqLoMidGainDial->blockSignals(true);
        m_eqLoMidFreqDial->setValue(freqVal);
        m_eqLoMidGainDial->setValue(gainVal);
        m_eqLoMidFreqDial->blockSignals(false);
        m_eqLoMidGainDial->blockSignals(false);
        onLoMidFreqChanged(freqVal); // This will call the slot that emits
        break;
    case 2:
        m_eqHiMidFreqDial->blockSignals(true);
        m_eqHiMidGainDial->blockSignals(true);
        m_eqHiMidFreqDial->setValue(freqVal);
        m_eqHiMidGainDial->setValue(gainVal);
        m_eqHiMidFreqDial->blockSignals(false);
        m_eqHiMidGainDial->blockSignals(false);
        onHiMidFreqChanged(freqVal); // This will call the slot that emits
        break;
    case 3:
        m_eqHighFreqDial->blockSignals(true);
        m_eqHighGainDial->blockSignals(true);
        m_eqHighFreqDial->setValue(freqVal);
        m_eqHighGainDial->setValue(gainVal);
        m_eqHighFreqDial->blockSignals(false);
        m_eqHighGainDial->blockSignals(false);
        onHighFreqChanged(freqVal); // This will call the slot that emits
        break;
    }
}


// --- Band Toggles ---
// --- ADDED EMIT CALLS ---
void EqWindow::onLowButtonToggled(bool checked)   {
    m_eqGraph->setBandEnabled(0, checked);
    emit eqEnableChanged(0, checked);
}
void EqWindow::onLoMidButtonToggled(bool checked) {
    m_eqGraph->setBandEnabled(1, checked);
    emit eqEnableChanged(1, checked);
}
void EqWindow::onHiMidButtonToggled(bool checked) {
    m_eqGraph->setBandEnabled(2, checked);
    emit eqEnableChanged(2, checked);
}
void EqWindow::onHighButtonToggled(bool checked)  {
    m_eqGraph->setBandEnabled(3, checked);
    emit eqEnableChanged(3, checked);
}

// --- Band 1: LOW ---
// --- FIX: Use global scope '::' to call inline functions ---
void EqWindow::onLowFreqChanged(int value) {
    double freq = ::mapDialToFreq(value);
    double gain = ::mapDialToGain(m_eqLowGainDial->value());
    double q = ::mapDialToQ(m_eqLowQDial->value());
    m_eqGraph->setBandParameters(0, freq, gain, q);
    emit eqBandChanged(0, freq, gain, q);
}
void EqWindow::onLowGainChanged(int value) {
    double freq = ::mapDialToFreq(m_eqLowFreqDial->value());
    double gain = ::mapDialToGain(value);
    double q = ::mapDialToQ(m_eqLowQDial->value());
    m_eqGraph->setBandParameters(0, freq, gain, q);
    emit eqBandChanged(0, freq, gain, q);
}
void EqWindow::onLowQChanged(int value) {
    double freq = ::mapDialToFreq(m_eqLowFreqDial->value());
    double gain = ::mapDialToGain(m_eqLowGainDial->value());
    double q = ::mapDialToQ(value);
    m_eqGraph->setBandParameters(0, freq, gain, q);
    emit eqBandChanged(0, freq, gain, q);
}

// --- Band 2: LO MID ---
// --- FIX: Use global scope '::' to call inline functions ---
void EqWindow::onLoMidFreqChanged(int value) {
    double freq = ::mapDialToFreq(value);
    double gain = ::mapDialToGain(m_eqLoMidGainDial->value());
    double q = ::mapDialToQ(m_eqLoMidQDial->value());
    m_eqGraph->setBandParameters(1, freq, gain, q);
    emit eqBandChanged(1, freq, gain, q);
}
void EqWindow::onLoMidGainChanged(int value) {
    double freq = ::mapDialToFreq(m_eqLoMidFreqDial->value());
    double gain = ::mapDialToGain(value);
    double q = ::mapDialToQ(m_eqLoMidQDial->value());
    m_eqGraph->setBandParameters(1, freq, gain, q);
    emit eqBandChanged(1, freq, gain, q);
}
void EqWindow::onLoMidQChanged(int value) {
    double freq = ::mapDialToFreq(m_eqLoMidFreqDial->value());
    double gain = ::mapDialToGain(m_eqLoMidGainDial->value());
    double q = ::mapDialToQ(value);
    m_eqGraph->setBandParameters(1, freq, gain, q);
    emit eqBandChanged(1, freq, gain, q);
}

// --- Band 3: HI MID ---
// --- FIX: Use global scope '::' to call inline functions ---
void EqWindow::onHiMidFreqChanged(int value) {
    double freq = ::mapDialToFreq(value);
    double gain = ::mapDialToGain(m_eqHiMidGainDial->value());
    double q = ::mapDialToQ(m_eqHiMidQDial->value());
    m_eqGraph->setBandParameters(2, freq, gain, q);
    emit eqBandChanged(2, freq, gain, q);
}
void EqWindow::onHiMidGainChanged(int value) {
    double freq = ::mapDialToFreq(m_eqHiMidFreqDial->value());
    double gain = ::mapDialToGain(value);
    double q = ::mapDialToQ(m_eqHiMidQDial->value());
    m_eqGraph->setBandParameters(2, freq, gain, q);
    emit eqBandChanged(2, freq, gain, q);
}
void EqWindow::onHiMidQChanged(int value) {
    double freq = ::mapDialToFreq(m_eqHiMidFreqDial->value());
    double gain = ::mapDialToGain(m_eqHiMidGainDial->value());
    double q = ::mapDialToQ(value);
    m_eqGraph->setBandParameters(2, freq, gain, q);
    emit eqBandChanged(2, freq, gain, q);
}

// --- Band 4: HIGH ---
// --- FIX: Use global scope '::' to call inline functions ---
void EqWindow::onHighFreqChanged(int value) {
    double freq = ::mapDialToFreq(value);
    double gain = ::mapDialToGain(m_eqHighGainDial->value());
    double q = ::mapDialToQ(m_eqHighQDial->value());
    m_eqGraph->setBandParameters(3, freq, gain, q);
    emit eqBandChanged(3, freq, gain, q);
}
void EqWindow::onHighGainChanged(int value) {
    double freq = ::mapDialToFreq(m_eqHighFreqDial->value());
    double gain = ::mapDialToGain(value);
    double q = ::mapDialToQ(m_eqHighQDial->value());
    m_eqGraph->setBandParameters(3, freq, gain, q);
    emit eqBandChanged(3, freq, gain, q);
}
void EqWindow::onHighQChanged(int value) {
    double freq = ::mapDialToFreq(m_eqHighFreqDial->value());
    double gain = ::mapDialToGain(m_eqHighGainDial->value());
    double q = ::mapDialToQ(value);
    m_eqGraph->setBandParameters(3, freq, gain, q);
    emit eqBandChanged(3, freq, gain, q);
}
