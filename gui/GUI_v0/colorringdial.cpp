#include "colorringdial.h"
#include <QPainter>
#include <QPen>

ColorRingDial::ColorRingDial(QWidget *parent)
    : QDial(parent), m_bandColor(Qt::darkGray) // Default color
{
    // We set a fixed size to make painting easier
    setFixedSize(60, 60);
    // Use the full circle. 0-3600 maps to 0-360.0 degrees
    setRange(0, 3600);
    setValue(1800); // Start in middle
    setWrapping(false);
    setNotchesVisible(false); // We are custom painting everything
}

const QColor &ColorRingDial::bandColor() const
{
    return m_bandColor;
}

void ColorRingDial::setBandColor(const QColor &newBandColor)
{
    if (m_bandColor == newBandColor)
        return;
    m_bandColor = newBandColor;
    emit bandColorChanged();
    update(); // Repaint when color changes
}

// --- This is the custom painting logic ---
void ColorRingDial::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Get widget dimensions
    int w = width();
    int h = height();
    int side = qMin(w, h);

    // Outer rectangle for drawing arcs
    // We inset by 2px for a small margin
    QRectF outerRect(2, 2, side - 4, side - 4);

    // --- 1. Draw the dark groove (background) ---
    painter.save();
    QPen pen;
    pen.setWidth(4); // 4px thick ring
    pen.setColor(QColor(40, 44, 49)); // Dark background color
    pen.setCapStyle(Qt::FlatCap);
    painter.setPen(pen);
    // Draw the full circle background
    painter.drawArc(outerRect, 0, 360 * 16);
    painter.restore();

    // --- 2. Draw the colored value arc ---
    painter.save();
    pen.setColor(m_bandColor); // Use our band color
    painter.setPen(pen);

    // QDial uses 240 degrees as the "start" (bottom-left)
    // and -60 as the "end" (bottom-right), with a span of 300 degrees.
    // We will map our 0-3600 range to this 300-degree span.

    // Map value (0-3600) to angle span (0-300)
    double valueSpan = (value() / (double)maximum()) * 300.0;

    // Draw the arc starting from the bottom-left
    // Angle is in 1/16ths of a degree
    painter.drawArc(outerRect, 240 * 16, -valueSpan * 16);
    painter.restore();

    // --- 3. Draw the inner knob ---
    painter.save();
    // Inset by 10px for the inner knob
    QRectF innerRect(10, 10, side - 20, side - 20);

    // Gradient for the knob face
    QRadialGradient knobGradient(innerRect.center(), innerRect.width() / 2.0);
    knobGradient.setColorAt(0, QColor(200, 200, 200)); // Lighter center
    knobGradient.setColorAt(1, QColor(160, 160, 160)); // Darker edge
    painter.setBrush(knobGradient);

    // Border for the knob
    pen.setWidth(1);
    pen.setColor(QColor(100, 100, 100));
    painter.setPen(pen);
    painter.drawEllipse(innerRect);

    // --- 4. Draw the indicator dot ---
    pen.setWidth(2);
    pen.setColor(QColor(40, 44, 49)); // Dark dot
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    // 1. THIS IS THE FIRST FIX
    //    We start at 240 degrees and move clockwise (by subtracting the span)
    double angleRad = qDegreesToRadians(240.0 - valueSpan); // Convert angle to radians

    double radius = (innerRect.width() / 2.0) - 5; // Radius for dot

    // 2. THIS IS THE SECOND FIX
    //    We must INVERT the Y-axis from qSin()
    QPointF dotPos(
        innerRect.center().x() + radius * qCos(angleRad),
        innerRect.center().y() - radius * qSin(angleRad) // <-- NOTICE THE MINUS SIGN
        );

    painter.drawEllipse(dotPos, 2, 2); // Draw a 2x2 ellipse for the dot
    painter.restore();
}
