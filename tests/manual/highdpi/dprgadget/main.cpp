// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qtversion.h>
#include <QtGui/qpa/qplatformscreen.h>
#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtWidgets/QtWidgets>
#include <iostream>

/*
    DprGadget: The focused High-DPI settings debug utility

    DprGadget displays the device pixel ratio ("DPR") for the screen
    it's on in a large font, as well as the inputs (from the platform
    plugin or environment) currently in use for determinging the DPR.

    Non-relevant inputs are not displayed. See qttools/src/qtdiag for
    an utility which displays all inputs.
*/

bool g_qtScaleFactor = false;
bool g_qtUsePhysicalDpi = false;
bool g_qtFontDpi = false;
bool g_qtScaleFactorRoundingPolicy = false;
bool g_qtHighDpiDownscale = false;
bool g_displayEvents = false;


class DprGadget : public QWidget
{
public:
    std::function<void(void)> m_clearFn;
    std::function<void(void)> m_updateFn;
    qreal m_currentDpr = -1;
    QString m_eventsText;

    DprGadget() {
        setWindowTitle(QString("DprGadget - Qt %1").arg(qVersion()));

        QFont tinyFont;
        tinyFont.setPointSize(8);
        QFont smallFont;
        smallFont.setPointSize(12);
        QFont bigFont;
        bigFont.setPointSize(42);
        QFont biggerFont;
        biggerFont.setPointSize(80);

        QLabel *dprLabel = new QLabel("Device Pixel Ratio");
        dprLabel->setFont(bigFont);
        dprLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        QLabel *dprValue = new QLabel();
        dprValue->setFont(biggerFont);
        dprValue->setTextInteractionFlags(Qt::TextSelectableByMouse);

        QLabel *screenLabel = new QLabel("Current Screen:");
        screenLabel->setFont(smallFont);

        QLabel *sizeLabel = new QLabel("Window size:");
        sizeLabel->setFont(smallFont);
        sizeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        QLabel *nativeSizeLabel = new QLabel("Native:");
        nativeSizeLabel->setFont(smallFont);
        nativeSizeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        QLabel *dpiLabel = new QLabel("Logical DPI:");
        dpiLabel->setFont(smallFont);
        dpiLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        QLabel *windowDpiLabel = new QLabel("Window DPI:");
        windowDpiLabel->setFont(smallFont);
        windowDpiLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        QLabel *platformDpiLabel = new QLabel("Native Device Pixel Ratio:");
        platformDpiLabel->setFont(smallFont);
        platformDpiLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        QLabel *windowDprLabel = new QLabel("Window DPR:");
        windowDprLabel->setFont(smallFont);
        windowDprLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        QLabel *plarformDprLabel = new QLabel("Native DPI:");
        plarformDprLabel->setFont(smallFont);
        plarformDprLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        QLabel *qtScaleFactorLabel = new QLabel("Qt Internal Scale Factor:");
        qtScaleFactorLabel->setFont(smallFont);

        QLabel *eventsLabel = new QLabel(m_eventsText);
        eventsLabel->setFont(tinyFont);
        eventsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        QVBoxLayout *layout = new QVBoxLayout();
        layout->addWidget(dprLabel);
        layout->setAlignment(dprLabel, Qt::AlignHCenter);
        layout->addWidget(dprValue);
        layout->setAlignment(dprValue, Qt::AlignHCenter);

        bool displayLogicalDpi = false;
        if (displayLogicalDpi)
            layout->addWidget(dpiLabel);

        layout->addStretch();

        QHBoxLayout *screenLabelLayout = new QHBoxLayout();
        screenLabelLayout->addStretch();
        screenLabelLayout->addWidget(screenLabel);
        screenLabelLayout->addStretch();
        layout->addLayout(screenLabelLayout);

        QHBoxLayout *windowSizeLayout = new QHBoxLayout();
        windowSizeLayout->addWidget(sizeLabel);
        windowSizeLayout->addStretch();
        windowSizeLayout->addWidget(nativeSizeLabel);
        layout->addLayout(windowSizeLayout);

        QHBoxLayout *dpiLayout = new QHBoxLayout();
        dpiLayout->addWidget(windowDpiLabel);
        dpiLayout->addStretch();
        dpiLayout->addWidget(platformDpiLabel);
        layout->addLayout(dpiLayout);

        QHBoxLayout *dprLayout = new QHBoxLayout();
        dprLayout->addWidget(windowDprLabel);
        dprLayout->addStretch();
        dprLayout->addWidget(plarformDprLabel);
        layout->addLayout(dprLayout);

        QHBoxLayout *qtScaleFactorLabelLayout = new QHBoxLayout();
        qtScaleFactorLabelLayout->addStretch();
        qtScaleFactorLabelLayout->addWidget(qtScaleFactorLabel);
        qtScaleFactorLabelLayout->addStretch();
        layout->addLayout(qtScaleFactorLabelLayout);

        if (g_displayEvents)
            layout->addWidget(eventsLabel);

        bool activeEnvironment = g_qtScaleFactor || g_qtUsePhysicalDpi || g_qtFontDpi || g_qtScaleFactorRoundingPolicy || g_qtHighDpiDownscale;
        if (activeEnvironment) {
            layout->addWidget(new QLabel("Active Environment:"));
            if (g_qtScaleFactor) {
                QString text = QString("QT_SCALE_FACTOR=") + qgetenv("QT_SCALE_FACTOR");
                layout->addWidget(new QLabel(text));
            }
            if (g_qtUsePhysicalDpi) {
                QString text = QString("QT_USE_PHYSICAL_DPI=") + qgetenv("QT_USE_PHYSICAL_DPI");
                layout->addWidget(new QLabel(text));
            }
            if (g_qtFontDpi) {
                QString text = QString("QT_FONT_DPI=") + qgetenv("QT_FONT_DPI");
                layout->addWidget(new QLabel(text));
            }
            if (g_qtScaleFactorRoundingPolicy) {
                QString text = QString("QT_SCALE_FACTOR_ROUNDING_POLICY=") + qgetenv("QT_SCALE_FACTOR_ROUNDING_POLICY");
                layout->addWidget(new QLabel(text));
            }
            if (g_qtHighDpiDownscale) {
                QString text = QString("QT_WIDGETS_HIGHDPI_DOWNSCALE=") + qgetenv("QT_WIDGETS_HIGHDPI_DOWNSCALE");
                layout->addWidget(new QLabel(text));
            }
        }

        auto updateValues = [=]() {
            dprValue->setText(QString("%1").arg(devicePixelRatioF()));
            windowDpiLabel->setText(QString("QWindow DPI: %1").arg(logicalDpiX()));
            dpiLabel->setText(QString("Logical DPI: %1").arg(logicalDpiX()));
            sizeLabel->setText(QString("QWindow size: %1 %2").arg(width()).arg(height()));

            QPlatformWindow *platformWindow = windowHandle()->handle();
            nativeSizeLabel->setText(QString("native size %1 %2").arg(platformWindow->geometry().width())
                                                            .arg(platformWindow->geometry().height()));
            QPlatformScreen *pscreen = screen()->handle();
            if (g_qtUsePhysicalDpi) {
                int physicalDpi = qRound(pscreen->geometry().width() / pscreen->physicalSize().width() * qreal(25.4));
                platformDpiLabel->setText(QString("Native Physical DPI: %1").arg(physicalDpi));
            } else {
                platformDpiLabel->setText(QString("native logical DPI: %1").arg(pscreen->logicalDpi().first));
            }

            windowDprLabel->setText(QString("QWindow DPR: %1").arg(windowHandle()->devicePixelRatio()));
            plarformDprLabel->setText(QString("native DPR: %1").arg(pscreen->devicePixelRatio()));

            screenLabel->setText(QString("Current Screen: %1").arg(screen()->name()));
            qtScaleFactorLabel->setText(QString("Qt Internal Scale Factor: %1").arg(QHighDpiScaling::factor(windowHandle())));
            eventsLabel->setText(QString(m_eventsText));
        };
        m_updateFn = updateValues;

        m_clearFn = [=]() {
            dprValue->setText(QString(""));
            m_eventsText.clear();
        };

        create();

        QObject::connect(this->windowHandle(), &QWindow::screenChanged, [updateValues, this](QScreen *screen){
            Q_UNUSED(screen);
            this->m_eventsText.prepend(QString("ScreenChange "));
            this->m_eventsText.truncate(80);
            updateValues();
        });

        setLayout(layout);

        updateValues();
    }

    void paintEvent(QPaintEvent *) override  {

        // Update the UI in the paint event - normally not good
        // practice but it looks like we can get away with it here
        this->m_eventsText.prepend(QString("Paint "));
        this->m_eventsText.truncate(80);

        // Dpr change should trigger a repaint, update display values here
        if (m_currentDpr == devicePixelRatioF())
            return;
        m_currentDpr = devicePixelRatioF();

        m_updateFn();
    }

    void resizeEvent(QResizeEvent *event) override {
        QSize size = event->size();
        m_eventsText.prepend(QString("Resize(%1 %2) ").arg(size.width()).arg(size.height()));
        m_eventsText.truncate(80);
        m_updateFn();
    }

    void mousePressEvent(QMouseEvent *) override {
        m_clearFn();
        QTimer::singleShot(500, this, [this](){
            m_updateFn();
        });
    }
};

int main(int argc, char **argv) {

    // Set sensible defaults
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    // Display any set high-dpi eviornment variable.
    g_qtScaleFactor = qEnvironmentVariableIsSet("QT_SCALE_FACTOR");
    g_qtUsePhysicalDpi = qgetenv("QT_USE_PHYSICAL_DPI") == QByteArray("1");
    g_qtFontDpi = qEnvironmentVariableIsSet("QT_FONT_DPI");
    g_qtScaleFactorRoundingPolicy = qEnvironmentVariableIsSet("QT_SCALE_FACTOR_ROUNDING_POLICY");
    g_qtHighDpiDownscale = qEnvironmentVariableIsSet("QT_WIDGETS_HIGHDPI_DOWNSCALE");

    QApplication app(argc, argv);

    DprGadget dprGadget;
    dprGadget.show();

    return app.exec();
}
