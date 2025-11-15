#ifndef COLORRINGDIAL_H
#define COLORRINGDIAL_H

#include <QDial>
#include <QColor>

class ColorRingDial : public QDial
{
    Q_OBJECT
    // Define a new property 'bandColor' that we can set
    Q_PROPERTY(QColor bandColor READ bandColor WRITE setBandColor NOTIFY bandColorChanged)

public:
    explicit ColorRingDial(QWidget *parent = nullptr);

    const QColor &bandColor() const;
    void setBandColor(const QColor &newBandColor);

signals:
    void bandColorChanged();

protected:
    // This is the magic: we override the paint event
    void paintEvent(QPaintEvent *event) override;

private:
    QColor m_bandColor;
};

#endif // COLORRINGDIAL_H
