/*
 * QT GUI APPLICATION - SIMPLIFIED OSC PARSING
 */

#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QObject>
#include <QUdpSocket>
#include <QDebug>
#include <QByteArray>
#include <QDataStream>

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

            // Simple manual OSC parsing to avoid library issues
            parseOscManually(datagram);
        }
    }

private:
    void parseOscManually(const QByteArray& data) {
        // Basic OSC message structure:
        // - Null-terminated address string (aligned to 4 bytes)
        // - Type tag string starting with ',' (aligned to 4 bytes)
        // - Data (aligned to 4 bytes)

        if (data.size() < 8) return; // Minimum size for address + type tag

        // Parse address
        const char* ptr = data.constData();
        QString address = QString::fromUtf8(ptr);

        qDebug() << "QtGUI: Raw address:" << address;

        // Check if it's a volume message
        if (address.startsWith("/track/") && address.contains("/volume")) {
            // Parse track number
            QStringList parts = address.split('/');
            if (parts.size() >= 3) {
                bool ok;
                int track_index = parts[2].toInt(&ok);
                if (ok) {
                    // Find type tag
                    int address_len = address.length() + 1; // Include null terminator
                    int padded_addr_len = (address_len + 3) & ~3; // Align to 4 bytes

                    if (padded_addr_len + 4 <= data.size()) {
                        const char* type_tag = ptr + padded_addr_len;
                        if (type_tag[0] == ',' && type_tag[1] == 'f' && type_tag[2] == '\0') {
                            // Type tag is ",f" - we have a float
                            int type_tag_len = 4; // ",f" + null + padding
                            int data_offset = padded_addr_len + type_tag_len;

                            if (data_offset + 4 <= data.size()) {
                                // Extract float (OSC uses big-endian)
                                const unsigned char* float_data =
                                    reinterpret_cast<const unsigned char*>(ptr + data_offset);

                                // Convert from big-endian to host byte order
                                union {
                                    uint32_t i;
                                    float f;
                                } converter;

                                converter.i = (float_data[0] << 24) |
                                              (float_data[1] << 16) |
                                              (float_data[2] << 8) |
                                              float_data[3];

                                float volume = converter.f;

                                qDebug() << "QtGUI: Parsed volume - Track:" << track_index << "Volume:" << volume;
                                emit volumeChanged(track_index - 1, volume); // Convert to 0-index
                            }
                        }
                    }
                }
            }
        }
    }

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
public slots:
    void onVolumeChanged(int trackIndex, float volume) {
        // Track IDs:
        // 0 = Master track
        // 1 = First regular track
        // 2 = Second regular track, etc.

        if (trackIndex == 1) { // First regular track
            qDebug() << "QtGUI: Updating slider for track" << trackIndex << "to" << volume;
            int slider_value = static_cast<int>(volume * 100.0f);
            m_volume_slider->setValue(slider_value);
            m_track_label->setText(QString("Track 1 Volume: %1%").arg(slider_value));
        } else {
            qDebug() << "QtGUI: Ignoring track" << trackIndex << "volume:" << volume;
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
#include "qt_gui_stub.moc"
