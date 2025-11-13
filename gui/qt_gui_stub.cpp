/*
 * QT GUI APPLICATION - BIDIRECTIONAL CONTROL
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
#define OSC_SEND_PORT 9000  // Send to Hub (same port)

// --- OSC Listener & Sender Class ---
class OscHandler : public QObject {
    Q_OBJECT

public:
    OscHandler(QObject* parent = nullptr) : QObject(parent) {
        // Setup receive socket
        m_receive_socket = new QUdpSocket(this);
        if (m_receive_socket->bind(QHostAddress::LocalHost, OSC_LISTEN_PORT)) {
            qDebug() << "QtGUI: OSC Listener bound to" << OSC_LISTEN_PORT;
            connect(m_receive_socket, &QUdpSocket::readyRead, this, &OscHandler::onReadyRead);
        } else {
            qDebug() << "QtGUI: Failed to bind to port" << OSC_LISTEN_PORT;
        }

        // Setup send socket
        m_send_socket = new QUdpSocket(this);
    }

    void sendVolume(int trackIndex, float volume) {
        // Build OSC message manually: /track/X/volume f value
        QByteArray osc_message;

        // Address pattern: /track/X/volume
        QString address = QString("/track/%1/volume").arg(trackIndex);
        osc_message.append(address.toUtf8());
        osc_message.append('\0');

        // Pad to 4 bytes
        while (osc_message.size() % 4 != 0) {
            osc_message.append('\0');
        }

        // Type tag: ",f"
        osc_message.append(',');
        osc_message.append('f');
        osc_message.append('\0');
        osc_message.append('\0'); // Pad to 4 bytes

        // Float value (big-endian)
        union {
            float f;
            uint32_t i;
        } converter;
        converter.f = volume;

        // Convert to big-endian
        uint32_t be_value = qToBigEndian(converter.i);
        osc_message.append(reinterpret_cast<const char*>(&be_value), 4);

        // Send to Hub
        m_send_socket->writeDatagram(osc_message, QHostAddress::LocalHost, OSC_SEND_PORT);
        qDebug() << "QtGUI: Sent OSC:" << address << volume;
    }

signals:
    void volumeChanged(int trackIndex, float volume);

private slots:
    void onReadyRead() {
        while (m_receive_socket->hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(m_receive_socket->pendingDatagramSize());
            m_receive_socket->readDatagram(datagram.data(), datagram.size());

            qDebug() << "QtGUI: Received UDP packet of size" << datagram.size();
            parseOscManually(datagram);
        }
    }

private:
    void parseOscManually(const QByteArray& data) {
        if (data.size() < 8) return;

        const char* ptr = data.constData();
        QString address = QString::fromUtf8(ptr);

        qDebug() << "QtGUI: Raw address:" << address;

        if (address.startsWith("/track/") && address.contains("/volume")) {
            QStringList parts = address.split('/');
            if (parts.size() >= 3) {
                bool ok;
                int track_index = parts[2].toInt(&ok);
                if (ok) {
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

                                qDebug() << "QtGUI: Parsed volume - Track:" << track_index << "Volume:" << volume;
                                emit volumeChanged(track_index, volume);
                            }
                        }
                    }
                }
            }
        }
    }

    QUdpSocket* m_receive_socket;
    QUdpSocket* m_send_socket;
};

// --- Main Window Class ---
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Mixer GUI - Bidirectional Control");

        // --- Setup GUI ---
        QWidget* central_widget = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(central_widget);

        m_track_label = new QLabel("Track 1 Volume: 50%", this);
        m_volume_slider = new QSlider(Qt::Horizontal, this);
        m_volume_slider->setRange(0, 100);
        m_volume_slider->setValue(50); // Default position

        layout->addWidget(m_track_label);
        layout->addWidget(m_volume_slider);
        setCentralWidget(central_widget);

        // --- Setup OSC Handler ---
        m_osc_handler = new OscHandler(this);

        // --- Connect signals ---
        // OSC → GUI updates
        connect(m_osc_handler, &OscHandler::volumeChanged, this, &MainWindow::onVolumeChangedFromReaper);

        // GUI → OSC sends
        connect(m_volume_slider, &QSlider::valueChanged, this, &MainWindow::onSliderMoved);

        // Prevent feedback loop - only send when user interacts
        m_volume_slider->setTracking(true);
    }

private slots:
    void onVolumeChangedFromReaper(int trackIndex, float volume) {
        if (trackIndex == 1) {
            // Only update if the change is significant (prevents jitter)
            float current_volume = m_volume_slider->value() / 100.0f;
            if (fabs(current_volume - volume) > 0.001f) {
                qDebug() << "QtGUI: REAPER updated track" << trackIndex << "to" << volume;

                m_volume_slider->blockSignals(true);
                int slider_value = static_cast<int>(volume * 100.0f);
                m_volume_slider->setValue(slider_value);
                m_track_label->setText(QString("Track 1 Volume: %1%").arg(slider_value));
                m_volume_slider->blockSignals(false);
            }
        }
    }

    void onSliderMoved(int value) {
        // This is called when user moves the slider
        float volume = value / 100.0f;
        qDebug() << "QtGUI: User moved slider to" << value << "(" << volume << ")";
        m_track_label->setText(QString("Track 1 Volume: %1%").arg(value));

        // Send volume command to REAPER
        m_osc_handler->sendVolume(1, volume);
    }

private:
    QLabel* m_track_label;
    QSlider* m_volume_slider;
    OscHandler* m_osc_handler;
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
