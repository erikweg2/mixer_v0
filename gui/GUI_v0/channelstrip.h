#ifndef CHANNELSTRIP_H
#define CHANNELSTRIP_H

#include <QWidget>
#include "eqgraphwidget.h" // <-- 1. Include the graph widget

// Forward declarations
class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QSlider;
class QLabel;
class QDial;
class QFrame;
class QProgressBar;
// class EqGraphWidget; // No longer needed, included above

/*
 * ChannelStrip Widget
 * This custom widget represents a single vertical channel strip
 * in the mixer, based on your new layout.
 */
class ChannelStrip : public QWidget
{
    Q_OBJECT

public:
    explicit ChannelStrip(int channelId, QWidget *parent = nullptr);

    int getChannelId() const;

public slots:
    void setSelected(bool selected);
    // --- 2. Add public slots to update the mini-graph ---
    void updateEqBand(int bandIndex, double freq, double gain, double q);
    void updateEqEnabled(int bandIndex, bool enabled);

signals:
    void channelSelected(int channelId);

private slots:
    void onSelectClicked();

private:
    int m_channelId;
    QPushButton *m_selectButton;
    QPushButton *m_soloButton;
    QPushButton *m_muteButton;

    // --- 3. Add member variable for the graph ---
    EqGraphWidget *m_eqGraph;
};

#endif // CHANNELSTRIP_H

