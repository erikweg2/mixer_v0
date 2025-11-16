#ifndef CHANNELSTRIP_H
#define CHANNELSTRIP_H

#include <QWidget>
#include "eqgraphwidget.h"

// Forward declarations
class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QSlider;
class QLabel;
class QDial;
class QFrame;
class QProgressBar;
class QTimer;

class ChannelStrip : public QWidget
{
    Q_OBJECT

public:
    explicit ChannelStrip(int channelId, QWidget *parent = nullptr);
    ~ChannelStrip();

    int getChannelId() const;

    // New method to set volume from REAPER
    void setVolumeFromReaper(double volume);

    // New method to set VU meter level from REAPER
    void setVULevel(float levelDb);

public slots:
    void setSelected(bool selected);
    void updateEqBand(int bandIndex, double freq, double gain, double q);
    void updateEqEnabled(int bandIndex, bool enabled);

signals:
    void channelSelected(int channelId);
    void volumeChanged(int channelId, double volume);  // New signal for volume changes

private slots:
    void onSelectClicked();
    void onFaderValueChanged(int value);
    void onVUUpdateTimeout();

private:
    int m_channelId;
    QPushButton *m_selectButton;
    QPushButton *m_soloButton;
    QPushButton *m_muteButton;
    QSlider *m_fader;  // Store reference to fader
    QProgressBar *m_meter;  // Store reference to meter

    EqGraphWidget *m_eqGraph;

    // Volume conversion helpers
    int volumeToFaderPosition(double volume);
    double faderPositionToVolume(int position);

    // VU meter smoothing
    float m_smoothedLevel;
    QTimer *m_vuUpdateTimer;
};

#endif // CHANNELSTRIP_H
