/*
 * QT GUI APPLICATION - 12-BIT FADER RESOLUTION
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
#include <cmath>

#define OSC_LISTEN_PORT 9002    // GUI listens on this port (Hub sends here)
#define OSC_SEND_PORT 9000      // GUI sends on this port (Hub listens here)

// 12-bit resolution constants
static constexpr int FADER_RESOLUTION = 4096;  // 12-bit = 4096 steps
static constexpr double MIN_DB = -100.0;       // Professional range
static constexpr double MAX_DB = 12.0;         // Maximum dB value

// High-resolution logarithmic scaling for 12-bit faders
class FaderScale {
public:
    // Convert linear volume (0.0-4.0) to 12-bit fader position (0-4095)
    static int volumeToFader12Bit(double volume) {
        if (volume <= 0.0) return 0;

        double db = volumeToDb(volume);

        // Multi-segment scaling for professional fader response
        double normalized;
        if (db <= -60.0) {
            // Ultra-fine resolution: -100 dB to -60 dB (first 10% of travel)
            normalized = (db + 100.0) / 40.0 * 0.1;
        } else if (db <= -20.0) {
            // Fine resolution: -60 dB to -20 dB (next 30% of travel)
            normalized = 0.1 + (db + 60.0) / 40.0 * 0.3;
        } else {
            // Standard resolution: -20 dB to +12 dB (last 60% of travel)
            normalized = 0.4 + (db + 20.0) / 32.0 * 0.6;
        }

        normalized = qBound(0.0, normalized, 1.0);
        return static_cast<int>(normalized * (FADER_RESOLUTION - 1));
    }

    // Convert 12-bit fader position (0-4095) to linear volume (0.0-4.0)
    static double fader12BitToVolume(int faderPos) {
        if (faderPos <= 0) return 0.0;

        double normalized = static_cast<double>(faderPos) / (FADER_RESOLUTION - 1);
        double db;

        // Inverse of the scaling curve
        if (normalized <= 0.1) {
            // -100 dB to -60 dB
            db = -100.0 + (normalized / 0.1) * 40.0;
        } else if (normalized <= 0.4) {
            // -60 dB to -20 dB
            db = -60.0 + ((normalized - 0.1) / 0.3) * 40.0;
        } else {
            // -20 dB to +12 dB
            db = -20.0 + ((normalized - 0.4) / 0.6) * 32.0;
        }

        return dbToVolume(db);
    }

    // Convert linear volume to dB
    static double volumeToDb(double volume) {
        if (volume <= 0.0) return MIN_DB;
        double db = 20.0 * log10(volume);
        return std::max(db, MIN_DB);
    }

    // Convert dB to linear volume
    static double dbToVolume(double db) {
        if (db <= MIN_DB) return 0.0;
        return pow(10.0, db / 20.0);
    }

    // Format volume for display
    static QString formatVolume(double volume) {
        if (volume <= 0.0) return "-∞ dB";

        double db = volumeToDb(volume);
        if (db < -90.0) return "-∞ dB";

        return QString("%1 dB").arg(db, 0, 'f', 1);
    }

    // Get dB value for precise calculations
    static double getDbValue(double volume) {
        if (volume <= 0.0) return MIN_DB;
        return volumeToDb(volume);
    }
};

// --- OSC Handler Class ---
class OscHandler : public QObject {
    Q_OBJECT

public:
    OscHandler(QObject* parent = nullptr) : QObject(parent) {
        // Setup receive socket
        m_receive_socket = new QUdpSocket(this);
        if (m_receive_socket->bind(QHostAddress::LocalHost, OSC_LISTEN_PORT)) {
            qDebug() << "QtGUI: OSC Listener bound to port" << OSC_LISTEN_PORT;
            connect(m_receive_socket, &QUdpSocket::readyRead, this, &OscHandler::onReadyRead);
        } else {
            qDebug() << "QtGUI: Failed to bind to port" << OSC_LISTEN_PORT;
        }

        // Setup send socket
        m_send_socket = new QUdpSocket(this);
        qDebug() << "QtGUI: OSC Sender will send to port" << OSC_SEND_PORT;
    }

    void sendVolume(int trackIndex, float volume) {
        // Send high-resolution volume as float
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
        osc_message.append('\0');

        // Float value (big-endian) - full 32-bit precision
        union {
            float f;
            uint32_t i;
        } converter;
        converter.f = volume;

        uint32_t be_value = qToBigEndian(converter.i);
        osc_message.append(reinterpret_cast<const char*>(&be_value), 4);

        // Send to Hub
        m_send_socket->writeDatagram(osc_message, QHostAddress::LocalHost, OSC_SEND_PORT);

        double db = FaderScale::getDbValue(volume);
        qDebug() << "QtGUI: Sent OSC:" << address << QString::number(volume, 'f', 6) << "(" << db << "dB)";
    }

signals:
    void volumeChanged(int trackIndex, float volume);

private slots:
    void onReadyRead() {
        while (m_receive_socket->hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(m_receive_socket->pendingDatagramSize());
            QHostAddress sender;
            quint16 senderPort;

            m_receive_socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

            parseOscManually(datagram);
        }
    }

private:
    void parseOscManually(const QByteArray& data) {
        if (data.size() < 8) return;

        const char* ptr = data.constData();
        QString address = QString::fromUtf8(ptr);

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

                                double db = FaderScale::getDbValue(volume);
                                qDebug() << "QtGUI: Parsed volume - Track:" << track_index
                                         << QString::number(volume, 'f', 6) << "(" << db << "dB)";
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
        setWindowTitle("Mixer GUI - 12-Bit Fader Resolution");

        // --- Setup GUI ---
        QWidget* central_widget = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(central_widget);

        m_track_label = new QLabel("Track 1: -∞ dB", this);
        m_volume_slider = new QSlider(Qt::Horizontal, this);
        m_volume_slider->setRange(0, FADER_RESOLUTION - 1); // 12-bit resolution: 0-4095
        m_volume_slider->setValue(FaderScale::volumeToFader12Bit(1.0)); // Start at unity

        // Add resolution info and scale
        QLabel* resolution_label = new QLabel(QString("12-bit Resolution (%1 steps)").arg(FADER_RESOLUTION));
        QLabel* scale_label = new QLabel("[-∞ ─ -60 ─ -40 ─ -20 ─ -10 ─ 0 dB ─ +12 dB]", this);
        scale_label->setAlignment(Qt::AlignCenter);

        QFont smallFont = scale_label->font();
        smallFont.setPointSize(8);
        scale_label->setFont(smallFont);
        resolution_label->setFont(smallFont);

        layout->addWidget(m_track_label);
        layout->addWidget(m_volume_slider);
        layout->addWidget(resolution_label);
        layout->addWidget(scale_label);
        setCentralWidget(central_widget);

        // --- Setup OSC Handler ---
        m_osc_handler = new OscHandler(this);

        // --- Connect signals ---
        connect(m_osc_handler, &OscHandler::volumeChanged, this, &MainWindow::onVolumeChangedFromReaper);
        connect(m_volume_slider, &QSlider::valueChanged, this, &MainWindow::onSliderMoved);

        m_volume_slider->setTracking(true);
    }

private slots:
    void onVolumeChangedFromReaper(int trackIndex, float volume) {
        if (trackIndex == 1) {
            double db = FaderScale::getDbValue(volume);
            qDebug() << "QtGUI: REAPER updated track" << trackIndex
                     << "to" << QString::number(volume, 'f', 6) << "(" << db << "dB)";

            m_volume_slider->blockSignals(true);
            int fader_pos = FaderScale::volumeToFader12Bit(volume);
            m_volume_slider->setValue(fader_pos);
            updateLabel(volume);
            m_volume_slider->blockSignals(false);
        }
    }

    void onSliderMoved(int fader_pos) {
        double volume = FaderScale::fader12BitToVolume(fader_pos);
        double db = FaderScale::getDbValue(volume);
        qDebug() << "QtGUI: Fader position" << fader_pos << "/" << (FADER_RESOLUTION - 1)
                 << "-> Volume:" << QString::number(volume, 'f', 6) << "(" << db << "dB)";

        updateLabel(volume);
        m_osc_handler->sendVolume(1, static_cast<float>(volume));
    }

private:
    void updateLabel(double volume) {
        QString db_text = FaderScale::formatVolume(volume);
        m_track_label->setText(QString("Track 1: %1").arg(db_text));
    }

    QLabel* m_track_label;
    QSlider* m_volume_slider;
    OscHandler* m_osc_handler;
};

// --- Main Application Entry ---
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    MainWindow main_window;
    main_window.resize(600, 180);
    main_window.show();

    return app.exec();
}

#include "qt_gui_stub.moc"
