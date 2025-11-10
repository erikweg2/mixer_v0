/*
 * QT GUI APPLICATION STUB (Component 4, Part 1)
 *
 * This is a minimal standalone Qt 6 C++ application.
 *
 * WHAT IT DOES:
 * 1.  Runs an OSC Client (listener) on port 9000.
 * 2.  Listens for OSC messages from the "Hub" (e.g., /track/1/volume).
 * 3.  Updates a simple GUI (a QLabel) when a message is received.
 *
 * DEPENDENCIES:
 * - Qt 6 (Core, Widgets, Network)
 * - An OSC library (oscpack or a Qt-specific one like QOsc)
 *
 * COMPILE:
 * This requires a C++ compiler, Qt 6, and moc/uic.
 * Using CMake or qmake is highly recommended.
 */

// --- Qt 6 ---
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QObject>
#include <QUdpSocket>
#include <QDebug>

// --- OSC Library (oscpack example) ---
#include "osc/OscReceivedElements.h"
#include "osc/OscPacketListener.h"

#define OSC_LISTEN_PORT 9000

// --- OSC Listener Class ---
class OscListener : public QObject {
    Q_OBJECT

public:
    OscListener(QObject* parent = nullptr) : QObject(parent) {
        m_socket = new QUdpSocket(this);

        // Bind to the port the Hub is broadcasting to
        if (m_socket->bind(QHostAddress::LocalHost, OSC_LISTEN_PORT)) {
            qDebug() << "QtGUI: OSC Listener bound to" << OSC_LISTEN_PORT;
            connect(m_socket, &QUdpSocket::readyRead, this, &OscListener::onReadyRead);
        } else {
            qDebug() << "QtGUI: Failed to bind to port" << OSC_LISTEN_PORT;
        }
    }

signals:
    // Signal to emit when a volume message is parsed
    void volumeChanged(int trackIndex, float volume);

private slots:
    void onReadyRead() {
        while (m_socket->hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(m_socket->pendingDatagramSize());
            m_socket->readDatagram(datagram.data(), datagram.size());

            qDebug() << "QtGUI: Received UDP packet of size" << datagram.size();

            // --- OSC Parsing (oscpack) ---

            try {
                // We must explicitly cast datagram.size() (a qsizetype)
                // to a type that oscpack understands, like std::size_t.
                osc::ReceivedPacket p(datagram.data(), static_cast<std::size_t>(datagram.size()));

                if (p.IsBundle()) {
                    osc::ReceivedBundle b(p);
                    for (osc::ReceivedBundle::const_iterator i = b.ElementsBegin();
                         i != b.ElementsEnd(); ++i) {
                        if (i->IsMessage()) {
                            osc::ReceivedMessage m(*i);
                            // --- Handle message (duplicated from else block) ---
                            const char* address = m.AddressPattern();
                            int track_index;
                            if (sscanf(address, "/track/%d/volume", &track_index) == 1) {
                                auto it = m.ArgumentsBegin();
                                if (it != m.ArgumentsEnd() && it->IsFloat()) {
                                    float volume = it->AsFloat();
                                    qDebug() << "QtGUI: Parsed OSC:" << address << volume;
                                    emit volumeChanged(track_index - 1, volume);
                                }
                            }
                            // --- End handle message ---
                        }
                    }
                } else {
                    // Handle single message
                    osc::ReceivedMessage m(p);
                    const char* address = m.AddressPattern();

                    // Example: /track/1/volume
                    int track_index;
                    if (sscanf(address, "/track/%d/volume", &track_index) == 1) {
                        // Get the first argument (the volume)
                        auto it = m.ArgumentsBegin();
                        if (it != m.ArgumentsEnd() && it->IsFloat()) {
                            float volume = it->AsFloat();
                            qDebug() << "QtGUI: Parsed OSC:" << address << volume;
                            // Emit our Qt signal
                            emit volumeChanged(track_index - 1, volume); // Convert back to 0-index
                        }
                    }
                }
            } catch(osc::Exception& e) {
                qDebug() << "QtGUI: Error parsing OSC: " << e.what();
            }
        }
    }

private:
    QUdpSocket* m_socket;
};

// --- Main Window Class ---
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Mixer GUI (Stub)");

        // --- Setup GUI ---
        QWidget* central_widget = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(central_widget);

        m_track_label = new QLabel("Track 1 Volume:", this);
        m_volume_slider = new QSlider(Qt::Horizontal, this);
        m_volume_slider->setRange(0, 100);
        m_volume_slider->setEnabled(false); // Read-only

        layout->addWidget(m_track_label);
        layout->addWidget(m_volume_slider);
        setCentralWidget(central_widget);

        // --- Setup OSC Listener ---
        m_listener = new OscListener(this);

        // --- Connect OSC signal to GUI slot ---
        connect(m_listener, &OscListener::volumeChanged, this, &MainWindow::onVolumeChanged);
    }

public slots:
    void onVolumeChanged(int trackIndex, float volume) {
        // This slot is called when the listener gets a valid message
        if (trackIndex == 0) { // We only care about track 1 (index 0)
            qDebug() << "QtGUI: Updating slider to" << volume;

            // --- FIX for static_s typo ---
            // It should be static_cast
            m_volume_slider->setValue(static_cast<int>(volume * 100.0f));
            // --- End FIX ---
        }
    }

private:
    QLabel* m_track_label;
    QSlider* m_volume_slider;
    OscListener* m_listener;
};

// --- Main Application Entry ---
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    MainWindow main_window;
    main_window.resize(400, 150);
    main_window.show();

    return app.exec();
}

// --- FIX for AutoMoc Error ---
// This file includes multiple Q_OBJECT classes,
// so we must manually include the moc file.
#include "qt_gui_stub.moc"
// --- End FIX ---
