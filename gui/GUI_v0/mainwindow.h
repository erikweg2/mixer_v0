#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>

// Forward declarations
class QStackedWidget;
class ChannelStrip;
class EqWindow;
class QUdpSocket;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Slot to handle navigation to the EQ page
    void onChannelSelected(int channelId);

    // Slot to handle navigation back to the bank
    void onBackClicked();

    // --- 1. Add slots to receive EQ data from EqWindow ---
    void onEqBandChanged(int bandIndex, double freq, double gain, double q);
    void onEqEnableChanged(int bandIndex, bool enabled);

    // --- OSC Communication Slots ---
    void onOscDataReceived();
    void sendVolumeToReaper(int channelId, double volume);
    void reconnectTimerTimeout();

private:
    QStackedWidget *m_stackedWidget;
    QWidget *m_bankViewWidget;
    EqWindow *m_eqViewWidget;

    // Keep a list of strips to manage their 'selected' state
    QList<ChannelStrip*> m_channelStrips;

    // --- 2. Add member to track active channel ---
    int m_currentChannelId = -1;

    // --- OSC Communication ---
    QUdpSocket *m_oscReceiveSocket;
    QUdpSocket *m_oscSendSocket;
    QTimer *m_reconnectTimer;

    // Configuration
    const int OSC_LISTEN_PORT = 9002;    // GUI listens on this port (Hub sends here)
    const int OSC_SEND_PORT = 9000;      // GUI sends on this port (Hub listens here)
    const QString HUB_HOST = "127.0.0.1";

    // Helper methods
    void initializeOscCommunication();
    void parseOscMessage(const QByteArray &data);
    QByteArray buildOscMessage(const QString &address, float value);
};

#endif // MAINWINDOW_H
