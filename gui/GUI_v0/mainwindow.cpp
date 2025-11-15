#include "mainwindow.h"
#include "channelstrip.h"
#include "eqwindow.h"
#include <QHBoxLayout>
#include <QWidget>
#include <QStackedWidget>
#include <QScrollArea>
#include <QDebug>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>
#include <QByteArray>
#include <QDataStream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    m_oscReceiveSocket(nullptr),
    m_oscSendSocket(nullptr),
    m_reconnectTimer(nullptr)
{
    setWindowTitle("Digital Mixer - REAPER Connected");
    resize(1280, 720);

    // Initialize OSC communication
    initializeOscCommunication();

    m_stackedWidget = new QStackedWidget(this);

    // --- Bank View (with ScrollArea) ---
    m_bankViewWidget = new QWidget;
    m_bankViewWidget->setObjectName("BankViewWidget");

    QWidget *stripContainerWidget = new QWidget();
    stripContainerWidget->setObjectName("StripContainerWidget");

    QHBoxLayout *bankLayout = new QHBoxLayout(stripContainerWidget);
    bankLayout->setSpacing(2);
    bankLayout->setContentsMargins(5, 5, 5, 5);

    m_channelStrips.clear();

    const int numChannels = 16;
    for (int i = 0; i < numChannels; ++i) {
        ChannelStrip *strip = new ChannelStrip(i + 1);
        bankLayout->addWidget(strip);
        m_channelStrips.append(strip);
        connect(strip, &ChannelStrip::channelSelected, this, &MainWindow::onChannelSelected);
        connect(strip, &ChannelStrip::volumeChanged, this, &MainWindow::sendVolumeToReaper);
    }

    // --- Scroll Area ---
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(stripContainerWidget);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QHBoxLayout* bankViewContainerLayout = new QHBoxLayout(m_bankViewWidget);
    bankViewContainerLayout->setContentsMargins(0,0,0,0);
    bankViewContainerLayout->addWidget(scrollArea);

    // --- EQ View ---
    m_eqViewWidget = new EqWindow;
    connect(m_eqViewWidget, &EqWindow::backClicked, this, &MainWindow::onBackClicked);
    connect(m_eqViewWidget, &EqWindow::eqBandChanged, this, &MainWindow::onEqBandChanged);
    connect(m_eqViewWidget, &EqWindow::eqEnableChanged, this, &MainWindow::onEqEnableChanged);

    // --- Add views to Stacked Widget ---
    m_stackedWidget->addWidget(m_bankViewWidget);
    m_stackedWidget->addWidget(m_eqViewWidget);

    setCentralWidget(m_stackedWidget);
    m_stackedWidget->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
    if (m_oscReceiveSocket) {
        m_oscReceiveSocket->close();
        delete m_oscReceiveSocket;
    }
    if (m_oscSendSocket) {
        m_oscSendSocket->close();
        delete m_oscSendSocket;
    }
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
        delete m_reconnectTimer;
    }
}

void MainWindow::initializeOscCommunication()
{
    // Setup receive socket
    m_oscReceiveSocket = new QUdpSocket(this);
    if (m_oscReceiveSocket->bind(QHostAddress::LocalHost, OSC_LISTEN_PORT)) {
        qDebug() << "GUI: OSC Listener bound to port" << OSC_LISTEN_PORT;
        connect(m_oscReceiveSocket, &QUdpSocket::readyRead, this, &MainWindow::onOscDataReceived);
    } else {
        qDebug() << "GUI: Failed to bind to port" << OSC_LISTEN_PORT;
    }

    // Setup send socket
    m_oscSendSocket = new QUdpSocket(this);
    qDebug() << "GUI: OSC Sender will send to port" << OSC_SEND_PORT;

    // Setup reconnect timer
    m_reconnectTimer = new QTimer(this);
    connect(m_reconnectTimer, &QTimer::timeout, this, &MainWindow::reconnectTimerTimeout);
    m_reconnectTimer->start(5000); // Check connection every 5 seconds
}

void MainWindow::onOscDataReceived()
{
    while (m_oscReceiveSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_oscReceiveSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        m_oscReceiveSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        parseOscMessage(datagram);
    }
}

void MainWindow::parseOscMessage(const QByteArray &data)
{
    if (data.size() < 8) return;

    const char* ptr = data.constData();
    QString address = QString::fromUtf8(ptr);

    if (address.startsWith("/track/") && address.contains("/volume")) {
        QStringList parts = address.split('/');
        if (parts.size() >= 3) {
            bool ok;
            int track_index = parts[2].toInt(&ok);
            if (ok && track_index >= 1 && track_index <= m_channelStrips.size()) {
                int address_len = address.length() + 1;
                int padded_addr_len = (address_len + 3) & ~3;

                if (padded_addr_len + 4 <= data.size()) {
                    const char* type_tag = ptr + padded_addr_len;
                    if (type_tag[0] == ',' && type_tag[1] == 'f' && type_tag[2] == '\0') {
                        int type_tag_len = 4;
                        int data_offset = padded_addr_len + type_tag_len;

                        if (data_offset + 4 <= data.size()) {
                            const unsigned char* float_data =
                                reinterpret_cast<const unsigned char*>(ptr + data_offset);

                            union {
                                uint32_t i;
                                float f;
                            } converter;

                            converter.i = (float_data[0] << 24) |
                                          (float_data[1] << 16) |
                                          (float_data[2] << 8) |
                                          float_data[3];

                            float volume = converter.f;

                            // Update the corresponding channel strip
                            ChannelStrip* strip = m_channelStrips[track_index - 1];
                            if (strip) {
                                strip->setVolumeFromReaper(volume);
                                qDebug() << "GUI: Updated track" << track_index << "volume to" << volume;
                            }
                        }
                    }
                }
            }
        }
    }
}

QByteArray MainWindow::buildOscMessage(const QString &address, float value)
{
    QByteArray osc_message;

    // Address pattern
    osc_message.append(address.toUtf8());
    osc_message.append('\0');
    while (osc_message.size() % 4 != 0) {
        osc_message.append('\0');
    }

    // Type tag
    osc_message.append(',');
    osc_message.append('f');
    osc_message.append('\0');
    osc_message.append('\0');

    // Float value (big-endian)
    union {
        float f;
        uint32_t i;
    } converter;
    converter.f = value;

    uint32_t be_value = qToBigEndian(converter.i);
    osc_message.append(reinterpret_cast<const char*>(&be_value), 4);

    return osc_message;
}

void MainWindow::sendVolumeToReaper(int channelId, double volume)
{
    QString address = QString("/track/%1/volume").arg(channelId);
    QByteArray osc_message = buildOscMessage(address, static_cast<float>(volume));

    m_oscSendSocket->writeDatagram(osc_message, QHostAddress(HUB_HOST), OSC_SEND_PORT);

    qDebug() << "GUI: Sent volume to REAPER - Track:" << channelId << "Volume:" << volume;
}

void MainWindow::reconnectTimerTimeout()
{
    // Simple connection health check - try to rebind if needed
    if (m_oscReceiveSocket->state() != QUdpSocket::BoundState) {
        qDebug() << "GUI: Attempting to reconnect OSC receiver...";
        if (m_oscReceiveSocket->bind(QHostAddress::LocalHost, OSC_LISTEN_PORT)) {
            qDebug() << "GUI: Successfully reconnected OSC receiver";
        }
    }
}

void MainWindow::onChannelSelected(int channelId)
{
    if (!m_eqViewWidget) {
        qWarning() << "ERROR: m_eqViewWidget is null!";
        return;
    }

    m_currentChannelId = channelId;
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
    m_currentChannelId = -1;
    m_stackedWidget->setCurrentIndex(0);
}

void MainWindow::onEqBandChanged(int bandIndex, double freq, double gain, double q)
{
    if (m_currentChannelId < 1 || m_currentChannelId > m_channelStrips.size()) return;

    ChannelStrip* strip = m_channelStrips[m_currentChannelId - 1];
    if (strip) {
        strip->updateEqBand(bandIndex, freq, gain, q);
    }
}

void MainWindow::onEqEnableChanged(int bandIndex, bool enabled)
{
    if (m_currentChannelId < 1 || m_currentChannelId > m_channelStrips.size()) return;

    ChannelStrip* strip = m_channelStrips[m_currentChannelId - 1];
    if (strip) {
        strip->updateEqEnabled(bandIndex, enabled);
    }
}
