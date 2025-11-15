#ifndef EQWINDOW_H
#define EQWINDOW_H

#include <QWidget>
#include "eqgraphwidget.h" // Include the custom graph widget

// Forward declarations
class QLabel;
class QPushButton;
class QFrame;
class QGroupBox;
class QHBoxLayout;
class QVBoxLayout;
class ColorRingDial;
class QDial;

class EqWindow : public QWidget
{
    Q_OBJECT

public:
    explicit EqWindow(QWidget *parent = nullptr);
    ~EqWindow(); // Destructor

signals:
    // Signal to go back to the mixer bank
    void backClicked();

    // --- ADDED SIGNALS BACK ---
    // Signals to update the mini-graph on the ChannelStrip
    void eqBandChanged(int bandIndex, double freq, double gain, double q);
    void eqEnableChanged(int bandIndex, bool enabled);


public slots:
    // Slot to update UI for the selected channel
    void showChannel(int channelId);

    // Slot to receive updates from graph dragging
    void onGraphBandChanged(int bandIndex, double newFreq, double newGain);

private slots:
    // Slots for EQ dial value changes
    void onLowFreqChanged(int value);
    void onLowGainChanged(int value);
    void onLowQChanged(int value);
    void onLoMidFreqChanged(int value);
    void onLoMidGainChanged(int value);
    void onLoMidQChanged(int value);
    void onHiMidFreqChanged(int value);
    void onHiMidGainChanged(int value);
    void onHiMidQChanged(int value);
    void onHighFreqChanged(int value);
    void onHighGainChanged(int value);
    void onHighQChanged(int value);

    // Slots for EQ band enable/disable buttons
    void onLowButtonToggled(bool checked);
    void onLoMidButtonToggled(bool checked);
    void onHiMidButtonToggled(bool checked);
    void onHighButtonToggled(bool checked);

private:
    // Helper function to create one full EQ band UI
    QWidget* createEqBand(const QString &name, const QColor &color,
                          ColorRingDial* &freqDial,
                          ColorRingDial* &gainDial,
                          ColorRingDial* &qDial);

    // Helper function for standard knobs (Filter, Gate, Comp)
    QWidget* createKnobWithLabel(const QString& labelText, QDial*& dial, int value);

    // --- Top Bar ---
    QLabel *m_titleLabel;

    // --- EQ Section ---
    EqGraphWidget *m_eqGraph; // Changed from QFrame*

    // 4 Band-select buttons
    QPushButton *m_eqLowButton;
    QPushButton *m_eqLoMidButton;
    QPushButton *m_eqHiMidButton;
    QPushButton *m_eqHighButton;

    // 4x Freq Dials
    ColorRingDial *m_eqLowFreqDial;
    ColorRingDial *m_eqLoMidFreqDial;
    ColorRingDial *m_eqHiMidFreqDial;
    ColorRingDial *m_eqHighFreqDial;

    // 4x Gain Dials
    ColorRingDial *m_eqLowGainDial;
    ColorRingDial *m_eqLoMidGainDial;
    ColorRingDial *m_eqHiMidGainDial;
    ColorRingDial *m_eqHighGainDial;

    // 4x Q Dials
    ColorRingDial *m_eqLowQDial;
    ColorRingDial *m_eqLoMidQDial;
    ColorRingDial *m_eqHiMidQDial;
    ColorRingDial *m_eqHighQDial;
};

#endif // EQWINDOW_H
