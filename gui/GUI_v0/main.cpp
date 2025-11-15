#include "mainwindow.h"
#include <QApplication>
#include <QString>

/*
 * Global Stylesheet (QSS)
 * This defines the dark theme for the application.
 */
const QString c_styleSheet = R"(
    QWidget {
        background-color: #353535;
        color: #E0E0E0;
        font-family: Arial, sans-serif;
    }

    QMainWindow, #BankViewWidget, #StripContainerWidget {
        background-color: #2B2B2B;
    }

    QScrollArea {
        border: none;
    }

    /* --- Channel Strip Styling --- */
    ChannelStrip {
        background-color: #4A4A4A;
        border: 1px solid #2B2B2B;
        border-radius: 4px;
        min-width: 120px;
    }

    #processingFrame {
        background-color: #3E3E3E;
        border: 1px solid #2B2B2B;
        border-radius: 4px;
    }

    #compPlaceholder {
        background-color: #2B2B2B;
        border: 1px solid #555;
    }

    #eqPlaceholder {
        background-color: #2B2B2B;
        border: 1px solid #555;
    }

    #auxFrame {
        background-color: transparent;
        border: none;
    }

    /* --- Buttons --- */
    QPushButton {
        background-color: #555555;
        border: 1px solid #444;
        border-radius: 3px;
        padding: 6px;
        font-size: 10px;
        font-weight: bold;
    }
    QPushButton:hover {
        background-color: #666666;
    }
    QPushButton:pressed, QPushButton:checked {
        background-color: #777;
        border: 1px solid #888;
    }

    /* Specific Button Colors */
    #selectButton:checked {
        background-color: #3C80D0;
        color: white;
    }
    #soloButton:checked {
        background-color: #D8B030;
        color: #1A1A1A;
    }
    #muteButton:checked {
        background-color: #D04040;
        color: white;
    }

    /* --- Faders & Sliders --- */
    #mainFader::groove:vertical {
        background: #2B2B2B;
        border: 1px solid #222;
        width: 10px;
        border-radius: 4px;
    }
    #mainFader::handle:vertical {
        background: #C0C5C9;
        border: 1px solid #808080;
        height: 35px;
        margin: 0 -10px;
        border-radius: 4px;
    }

    #auxSlider::groove:horizontal {
        background: #2B2B2B;
        border: 1px solid #222;
        height: 4px;
        border-radius: 2px;
    }
    #auxSlider::handle:horizontal {
        background: #999;
        border: 1px solid #777;
        width: 10px;
        margin: -4px 0;
        border-radius: 2px;
    }

    /* --- Pan Dial --- */
    QDial#panDial {
        background-color: #666;
        border-radius: 20px;
    }

    /* --- Labels --- */
    #channelLabel {
        background-color: #2B2B2B;
        border: 1px solid #555;
        border-radius: 3px;
        padding: 4px;
        font-weight: bold;
    }

    #bottomLabel {
        background-color: transparent;
        font-size: 9pt;
        font-weight: bold;
        color: #909090;
    }

    /* --- Level Meter --- */
    #levelMeter {
        background-color: #2B2B2B;
        border: 1px solid #222;
        border-radius: 4px;
    }
    #levelMeter::chunk {
        background-color: qlineargradient(x1:0, y1:1, x2:0, y2:0,
                                          stop:0 #00AEEF,
                                          stop:0.8 #7FFF00,
                                          stop:1 #FF0000);
        border-radius: 2px;
        margin: 1px;
    }

    /* --- EQ Window Styles --- */
    QGroupBox#ModuleBox {
        font-size: 10pt;
        font-weight: bold;
        color: #909090;
        border: 1px solid #45494F;
        border-radius: 5px;
        margin-top: 20px;
        background-color: #23262A;
        padding: 5px;
        padding-top: 15px;
    }
    QGroupBox#ModuleBox::title {
        subcontrol-origin: padding;
        subcontrol-position: top left;
        padding: 2px 10px;
        background-color: #23262a;
        border: 1px solid #45494F;
        border-radius: 3px;
        margin-left: 8px;
        margin-top: -5px;
    }

    QLabel#KnobCaptionLabel {
        font-size: 8pt;
        font-weight: normal;
        color: #B0B0B0;
        background-color: transparent;
        padding: 0;
    }

    QLabel#KnobValueLabel {
        font-size: 8pt;
        font-weight: bold;
        color: #E0E0E0;
        background-color: transparent;
        padding: 0;
    }

    QFrame#EqGraphPlaceholder {
        background-color: #3C4045;
        border: 1px solid #2D3035;
        border-radius: 4px;
        min-height: 120px;
    }

    QPushButton#EqBandButton {
        font-size: 8pt;
        padding: 3px 8px;
        min-height: 20px;
        border-radius: 2px;
        background-color: #4A4E54;
        border: 1px solid #5A5E65;
        color: #B0B0B0;
    }

    QPushButton#EqBandButton:checked {
        background-color: #E6B04C;
        border: 1px solid #F0C86C;
        color: #2D3035;
    }

    /* --- EQ Window Top Bar Styles --- */
    QLabel#FatChannelTitleLabel {
        font-size: 14pt;
        font-weight: bold;
        color: #E0E0E0;
        background-color: transparent;
        padding: 2px 5px;
    }

    QLabel#InOutLabel {
        font-size: 10pt;
        color: #B0B0B0;
        background-color: transparent;
        padding: 2px 5px;
    }

    QPushButton#SmallButton {
        font-size: 9pt;
        padding: 4px 8px;
        min-width: 50px;
    }

    QPushButton#SmallToggleButton {
        font-size: 8pt;
        padding: 3px 6px;
        min-height: 18px;
    }
)";


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Apply the stylesheet to the entire application
    a.setStyleSheet(c_styleSheet);

    MainWindow w;
    w.show();
    return a.exec();
}
