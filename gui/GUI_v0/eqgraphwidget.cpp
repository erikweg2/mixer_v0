#include "eqgraphwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <cmath>
#include <QDebug>
#include <QMouseEvent>

EqGraphWidget::EqGraphWidget(QWidget *parent)
    : QWidget(parent), m_draggedBand(-1)
{
    m_bands.resize(4);
    m_bands[0].frequency = 130.0;
    m_bands[1].frequency = 960.0;
    m_bands[2].frequency = 2500.0;
    m_bands[3].frequency = 6500.0;

    // --- THIS IS THE FIX ---
    // setMinimumSize(300, 150); // BUG: This forces the widget to be 300px wide

    // FIX: Allow the widget to be any size, and expand to fill space
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    // --- END FIX ---

    setMouseTracking(true); // Needed for hover/drag cursors
}

// --- Public API ---
void EqGraphWidget::setBandEnabled(int bandIndex, bool enabled) {
    if (bandIndex < 0 || bandIndex >= m_bands.size()) return;
    m_bands[bandIndex].enabled = enabled;
    update();
}

void EqGraphWidget::setBandParameters(int bandIndex, double freq, double gain, double q) {
    if (bandIndex < 0 || bandIndex >= m_bands.size()) return;
    m_bands[bandIndex].frequency = freq;
    m_bands[bandIndex].gain = gain;
    m_bands[bandIndex].q = q;
    update();
}

void EqGraphWidget::setBandColor(int bandIndex, const QColor& color) {
    if (bandIndex < 0 || bandIndex >= m_bands.size()) return;
    m_bands[bandIndex].color = color;
    update();
}

void EqGraphWidget::setSimpleMode(bool simple)
{
    m_simpleMode = simple;
    // Disable mouse interactions if in simple mode
    if (m_simpleMode) {
        setMouseTracking(false);
    } else {
        setMouseTracking(true);
    }
}


// --- Coordinate Mapping Helpers ---
double EqGraphWidget::mapFreqToX(double freq) {
    double logMin = log(m_minFreq);
    double logMax = log(m_maxFreq);
    double logFreq = log(qBound(m_minFreq, freq, m_maxFreq));
    return width() * (logFreq - logMin) / (logMax - logMin);
}

double EqGraphWidget::mapGainToY(double gain) {
    double g = qBound(m_minGain, gain, m_maxGain);
    return height() * (m_maxGain - g) / (m_maxGain - m_minGain);
}

double EqGraphWidget::mapXToFreq(double x) {
    double logMin = log(m_minFreq);
    double logMax = log(m_maxFreq);
    double range = logMax - logMin;
    return exp(logMin + (x / width()) * range);
}

double EqGraphWidget::mapYToGain(double y) {
    double range = m_maxGain - m_minGain;
    return m_maxGain - (y / height()) * range;
}

// --- Dial Value Mapping (Internal) ---
double EqGraphWidget::mapDialToFreq(int value) {
    return ::mapDialToFreq(value, m_minFreq, m_maxFreq);
}
double EqGraphWidget::mapDialToGain(int value) {
    return ::mapDialToGain(value, m_minGain, m_maxGain);
}
double EqGraphWidget::mapDialToQ(int value) {
    // Use the new logarithmic mapping
    return ::mapDialToQ(value, m_minQ, m_maxQ);
}

// --- Painting Logic ---
void EqGraphWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect(), QColor(43, 43, 43)); // #2B2B2B

    if (!m_simpleMode) {
        drawGrid(painter);
    }
    drawCurves(painter);
}

void EqGraphWidget::drawGrid(QPainter &painter) {
    painter.save();

    QPen majorGridPen(QColor(120, 120, 120), 1.0, Qt::DotLine); // For 10, 100, 1k, 10k
    QPen minorGridPen(QColor(80, 80, 80), 1.0, Qt::DotLine);   // For 20, 30, 50...
    QPen labelsPen(QColor(150, 150, 150));
    painter.setFont(QFont("Arial", 8));

    // --- Vertical (Frequency) Lines ---
    for (double f = 10; f <= 10000; f *= 10) {
        painter.setPen(majorGridPen);
        double x = mapFreqToX(f);
        painter.drawLine(x, 0, x, height());

        painter.setPen(labelsPen);
        QString label = (f < 1000) ? QString::number(f) : QString::number(f/1000.0, 'f', 0) + "k";
        painter.drawText(x + 3, height() - 3, label);

        painter.setPen(minorGridPen);
        for (int i = 2; i < 10; ++i) {
            double minorF = f * i;
            if (minorF > m_maxFreq) break;
            double minorX = mapFreqToX(minorF);

            if (i == 5) {
                painter.setPen(majorGridPen);
                painter.drawLine(minorX, 0, minorX, height());
                painter.setPen(labelsPen);
                QString minorLabel = (minorF < 1000) ? QString::number(minorF) : QString::number(minorF/1000.0, 'f', 0) + "k";
                painter.drawText(minorX + 3, height() - 3, minorLabel);
                painter.setPen(minorGridPen);
            } else {
                painter.drawLine(minorX, 0, minorX, height());
            }
        }
    }

    // --- Horizontal (Gain) Lines ---
    double zeroY = mapGainToY(0);
    majorGridPen.setStyle(Qt::SolidLine); // 0dB line is solid
    painter.setPen(majorGridPen);
    painter.drawLine(0, zeroY, width(), zeroY);

    minorGridPen.setStyle(Qt::DotLine);
    painter.setPen(minorGridPen);

    double gains[] = {-24, -18, -12, -6, 6, 12, 18, 24};
    for (double g : gains) {
        if (g == 0) continue;
        if (fmod(g, 6) == 0) {
            painter.setPen(majorGridPen);
        } else {
            painter.setPen(minorGridPen);
        }

        double y = mapGainToY(g);
        painter.drawLine(0, y, width(), y);

        painter.setPen(labelsPen);
        painter.drawText(3, y - 3, QString::number(g) + "dB");
    }

    painter.restore();
}

// ---
// --- THIS IS THE CORRECTED FUNCTION ---
// ---
void EqGraphWidget::drawCurves(QPainter &painter) {
    painter.save();

    const double zeroY = mapGainToY(0);
    const int w = width();

    // We must have at least 2 pixels to draw a line
    if (w < 2) {
        painter.restore();
        return;
    }

    QVector<double> compositeGain(w);
    compositeGain.fill(0.0);

    QVector<QVector<double>> bandGains(m_bands.size(), QVector<double>(w, 0.0));

    // --- 1. Pre-calculate all gain values ---
    for (int i = 0; i < m_bands.size(); ++i) {
        const auto& band = m_bands[i];
        if (!band.enabled) continue;

        const double peakGainDb = band.gain;
        const double peakX = mapFreqToX(band.frequency); // Get peak X once

        // Use the Q-factor from your working code
        const double C_baseline = 0.042 * w;
        double c = C_baseline / band.q;

        // --- THIS IS THE FIX ---
        // The original "if (c < 0.5)" was an absolute clamp that broke scaling
        // on small widgets. This new clamp is relative to the widget width.
        const double min_c = 0.0017 * w; // (0.0017 * 300) is ~0.5, matching original
        if (c < min_c) c = min_c;
        // --- END FIX ---


        // Loop through *every pixel* from 0 to width()
        // This guarantees the graph is always perfectly scaled.
        for (int x = 0; x < w; ++x) {
            const double dist = x - peakX;
            const double gainDb = peakGainDb * exp(-(dist * dist) / (2 * c * c));

            bandGains[i][x] = gainDb;
            compositeGain[x] += gainDb;
        }
    }

    // --- 2. Draw Individual Band Curves (Fill + Outline) ---
    // This part now draws the *full* curve for each band
    for (int i = 0; i < m_bands.size(); ++i)
    {
        const auto& band = m_bands[i];
        if (!band.enabled) continue;

        // Path 1: Just the curve, for the outline (stroke)
        // We draw from pixel 0 to pixel w-1
        QPainterPath strokePath;
        strokePath.moveTo(0, mapGainToY(bandGains[i][0]));
        for (int x = 1; x < w; ++x) {
            strokePath.lineTo(x, mapGainToY(bandGains[i][x]));
        }

        // Path 2: The filled area (curve + bottom line)
        QPainterPath fillPath = strokePath;
        fillPath.lineTo(w - 1, zeroY); // Go to bottom-right
        fillPath.lineTo(0, zeroY);     // Go to bottom-left
        fillPath.closeSubpath();       // Close

        // --- DRAW ---
        if (band.gain != 0.0) {
            QColor fillCol = band.color;
            fillCol.setAlpha(80);
            painter.fillPath(fillPath, fillCol);
        }
        QPen linePen(band.color, 1.5);
        painter.setPen(linePen);
        painter.drawPath(strokePath);
    }

    // --- 3. Draw Composite Curve (White Outline) ---
    QPainterPath compositePath;
    compositePath.moveTo(0, mapGainToY(compositeGain[0]));
    for (int x = 1; x < w; ++x) {
        double gainDb = qBound(m_minGain, compositeGain[x], m_maxGain);
        compositePath.lineTo(x, mapGainToY(gainDb));
    }

    QPen compositePen(Qt::white, 1.5, Qt::SolidLine);
    painter.setPen(compositePen);
    painter.drawPath(compositePath);


    // --- 4. Draw the Dots (on top of everything) ---
    if (!m_simpleMode) {
        for (const auto& band : m_bands) {
            if (!band.enabled) continue;

            double peakX = mapFreqToX(band.frequency);
            double peakY = mapGainToY(band.gain);

            painter.setBrush(band.color);
            painter.setPen(QPen(Qt::white, 1.5));
            painter.drawEllipse(QPointF(peakX, peakY), 5, 5);
        }
    }

    painter.restore();
}


// --- Mouse Events ---
void EqGraphWidget::mousePressEvent(QMouseEvent *event) {
    if (m_simpleMode) return;
    if (event->button() != Qt::LeftButton) return;

    for (int i = 0; i < m_bands.size(); ++i) {
        if (!m_bands[i].enabled) continue;

        double peakX = mapFreqToX(m_bands[i].frequency);
        double peakY = mapGainToY(m_bands[i].gain);

        double dist = QLineF(event->pos(), QPointF(peakX, peakY)).length();
        if (dist < 10) { // 10px click radius
            m_draggedBand = i;
            setCursor(Qt::CrossCursor);
            event->accept();
            return;
        }
    }
}

void EqGraphWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_simpleMode) return;

    if (m_draggedBand == -1) {
        bool nearDot = false;
        for (int i = 0; i < m_bands.size(); ++i) {
            if (!m_bands[i].enabled) continue;
            double peakX = mapFreqToX(m_bands[i].frequency);
            double peakY = mapGainToY(m_bands[i].gain);
            if (QLineF(event->pos(), QPointF(peakX, peakY)).length() < 10) {
                nearDot = true;
                break;
            }
        }
        setCursor(nearDot ? Qt::PointingHandCursor : Qt::ArrowCursor);
        return;
    }

    double newFreq = mapXToFreq(event->pos().x());
    double newGain = mapYToGain(event->pos().y());

    emit bandManuallyChanged(m_draggedBand, newFreq, newGain);
}

void EqGraphWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (m_simpleMode) return;

    if (event->button() == Qt::LeftButton) {
        m_draggedBand = -1;
        setCursor(Qt::ArrowCursor);
    }
}
