#ifndef EQGRAPHWIDGET_H
#define EQGRAPHWIDGET_H

#include <QWidget>
#include <QColor>
#include <QVector>

// A simple struct to hold the state of one EQ band
struct EqBandParameters {
    double frequency = 1000.0; // 20Hz - 20000Hz
    double gain = 0.0;       // -24dB - +24dB
    double q = 1.0;          // 0.1 - 10.0
    QColor color;
    bool enabled = true;     // Is the band on?
};

class EqGraphWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EqGraphWidget(QWidget *parent = nullptr);

    // Public API to update the graph from the EqWindow
    void setBandEnabled(int bandIndex, bool enabled);
    void setBandParameters(int bandIndex, double freq, double gain, double q);
    void setBandColor(int bandIndex, const QColor& color);

    // --- ADDED THIS FUNCTION BACK ---
    void setSimpleMode(bool simple);

signals:
    // Emitted when a dot is dragged by the mouse
    void bandManuallyChanged(int bandIndex, double newFreq, double newGain);

protected:
    void paintEvent(QPaintEvent *event) override;

    // Mouse events for dragging dots
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    // Coordinate mapping functions
    double mapFreqToX(double freq);
    double mapGainToY(double gain);
    double mapXToFreq(double x);
    double mapYToGain(double y);

    // Helpers to convert between real EQ values and 0-3600 dial values
    double mapDialToFreq(int value);
    double mapDialToGain(int value);
    double mapDialToQ(int value);

    // Painting functions
    void drawGrid(QPainter &painter);
    void drawCurves(QPainter &painter);

    // Store the state for all 4 bands
    QVector<EqBandParameters> m_bands;

    // Constants for the graph's range
    const double m_minFreq = 20.0;
    const double m_maxFreq = 20000.0;
    const double m_minGain = -24.0;
    const double m_maxGain = 24.0;
    const double m_minQ = 0.1;
    const double m_maxQ = 10.0;

    // State for dragging
    int m_draggedBand; // -1 if no band is being dragged

    // --- ADDED THIS MEMBER BACK ---
    bool m_simpleMode = false;
};

// --- Public helper functions for EqWindow ---
// (Moved mapping functions here so EqWindow and EqGraphWidget use the same logic)

// Dial (0-3600) -> Real Value
inline double mapDialToFreq(int value, double minFreq = 20.0, double maxFreq = 20000.0) {
    double logMin = log(minFreq);
    double logMax = log(maxFreq);
    double range = logMax - logMin;
    return exp(logMin + (value / 3600.0) * range);
}

inline double mapDialToGain(int value, double minGain = -24.0, double maxGain = 24.0) {
    double range = maxGain - minGain;
    return minGain + (value / 3600.0) * range;
}

// --- UPDATED Q MAPPING ---
inline double mapDialToQ(int value, double minQ = 0.1, double maxQ = 10.0) {
    // Logarithmic mapping for Q
    double logMin = log(minQ);
    double logMax = log(maxQ);
    double range = logMax - logMin;
    return exp(logMin + (value / 3600.0) * range);
}

// Real Value -> Dial (0-3600)
inline int mapFreqToDial(double freq, double minFreq = 20.0, double maxFreq = 20000.0) {
    double logMin = log(minFreq);
    double logMax = log(maxFreq);
    double logFreq = log(qBound(minFreq, freq, maxFreq));
    return static_cast<int>(3600.0 * (logFreq - logMin) / (logMax - logMin));
}

inline int mapGainToDial(double gain, double minGain = -24.0, double maxGain = 24.0) {
    double g = qBound(minGain, gain, maxGain);
    double range = maxGain - minGain;
    return static_cast<int>(3600.0 * (g - minGain) / range);
}

// --- NEW Q MAPPING ---
inline int mapQToDial(double q, double minQ = 0.1, double maxQ = 10.0) {
    // Logarithmic mapping for Q
    double logMin = log(minQ);
    double logMax = log(maxQ);
    double logQ = log(qBound(minQ, q, maxQ));
    return static_cast<int>(3600.0 * (logQ - logMin) / (logMax - logMin));
}


#endif // EQGRAPHWIDGET_H
