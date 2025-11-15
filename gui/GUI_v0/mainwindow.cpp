#include "mainwindow.h"
#include "channelstrip.h"
#include "eqwindow.h"
#include <QHBoxLayout>
#include <QWidget>
#include <QStackedWidget>
#include <QScrollArea> // Include QScrollArea
#include <QDebug>      // Include QDebug for warnings

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Digital Mixer");
    resize(1280, 720); // Default size

    m_stackedWidget = new QStackedWidget(this);

    // --- Bank View (with ScrollArea) ---
    m_bankViewWidget = new QWidget;
    m_bankViewWidget->setObjectName("BankViewWidget"); // For styling

    // This widget will contain all the channel strips
    QWidget *stripContainerWidget = new QWidget();
    stripContainerWidget->setObjectName("StripContainerWidget");

    QHBoxLayout *bankLayout = new QHBoxLayout(stripContainerWidget);
    bankLayout->setSpacing(2); // Tighter spacing
    bankLayout->setContentsMargins(5, 5, 5, 5);

    m_channelStrips.clear();

    const int numChannels = 16; // CHANGED to 16
    for (int i = 0; i < numChannels; ++i) {
        ChannelStrip *strip = new ChannelStrip(i + 1);
        bankLayout->addWidget(strip);
        m_channelStrips.append(strip);
        connect(strip, &ChannelStrip::channelSelected, this, &MainWindow::onChannelSelected);
    }

    // --- Scroll Area ---
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(stripContainerWidget); // Put the container in the scroll area
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // The m_bankViewWidget will just hold the scroll area
    QHBoxLayout* bankViewContainerLayout = new QHBoxLayout(m_bankViewWidget);
    bankViewContainerLayout->setContentsMargins(0,0,0,0);
    bankViewContainerLayout->addWidget(scrollArea);

    // --- EQ View ---
    m_eqViewWidget = new EqWindow;
    connect(m_eqViewWidget, &EqWindow::backClicked, this, &MainWindow::onBackClicked);
    // --- 1. Connect new signals from EqWindow to our new slots ---
    connect(m_eqViewWidget, &EqWindow::eqBandChanged, this, &MainWindow::onEqBandChanged);
    connect(m_eqViewWidget, &EqWindow::eqEnableChanged, this, &MainWindow::onEqEnableChanged);


    // --- Add views to Stacked Widget ---
    m_stackedWidget->addWidget(m_bankViewWidget); // Page index 0
    m_stackedWidget->addWidget(m_eqViewWidget);   // Page index 1

    setCentralWidget(m_stackedWidget);
    m_stackedWidget->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
}

void MainWindow::onChannelSelected(int channelId)
{
    if (!m_eqViewWidget) {
        qWarning() << "ERROR: m_eqViewWidget is null!";
        return;
    }

    // --- 2. Store the currently selected channel ID ---
    m_currentChannelId = channelId;

    // (In a real app, you'd load this channel's settings into EqWindow here)
    m_eqViewWidget->showChannel(channelId);

    for (ChannelStrip *strip : m_channelStrips) {
        strip->setSelected(strip->getChannelId() == channelId);
    }

    m_stackedWidget->setCurrentIndex(1);
}

void MainWindow::onBackClicked()
{
    for (ChannelStrip *strip : m_channelStrips) {
        strip->setSelected(false);
    }
    // --- 3. Clear the current channel ID ---
    m_currentChannelId = -1;
    m_stackedWidget->setCurrentIndex(0);
}

// --- 4. Implement the new slots ---

void MainWindow::onEqBandChanged(int bandIndex, double freq, double gain, double q)
{
    // Check if a valid channel is selected
    if (m_currentChannelId < 1 || m_currentChannelId > m_channelStrips.size()) return;

    // Find the strip (m_channelStrips is 0-indexed, channelId is 1-indexed)
    ChannelStrip* strip = m_channelStrips[m_currentChannelId - 1];
    if (strip) {
        // Call the strip's public slot to update its mini-graph
        strip->updateEqBand(bandIndex, freq, gain, q);
    }
}

void MainWindow::onEqEnableChanged(int bandIndex, bool enabled)
{
    // Check if a valid channel is selected
    if (m_currentChannelId < 1 || m_currentChannelId > m_channelStrips.size()) return;

    // Find the strip
    ChannelStrip* strip = m_channelStrips[m_currentChannelId - 1];
    if (strip) {
        // Call the strip's public slot to update its mini-graph
        strip->updateEqEnabled(bandIndex, enabled);
    }
}

