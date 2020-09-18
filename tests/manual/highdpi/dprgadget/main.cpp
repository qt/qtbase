/****************************************************************************
 **
 ** Copyright (C) 2020 The Qt Company Ltd.
 ** Contact: https://www.qt.io/licensing/
 **
 ** This file is part of the test suite of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:GPL-EXCEPT$
 ** Commercial License Usage
 ** Licensees holding valid commercial Qt licenses may use this file in
 ** accordance with the commercial license agreement provided with the
 ** Software or, alternatively, in accordance with the terms contained in
 ** a written agreement between you and The Qt Company. For licensing terms
 ** and conditions see https://www.qt.io/terms-conditions. For further
 ** information use the contact form at https://www.qt.io/contact-us.
 **
 ** GNU General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU
 ** General Public License version 3 as published by the Free Software
 ** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
 ** included in the packaging of this file. Please review the following
 ** information to ensure the GNU General Public License requirements will
 ** be met: https://www.gnu.org/licenses/gpl-3.0.html.
 **
 ** $QT_END_LICENSE$
 **
 ****************************************************************************/

#include <QtWidgets/QtWidgets>
#include <QtGui/qpa/qplatformscreen.h>

/*
    DprGadget: The focused High-DPI settings debug utility

    DprGadget displays the device pixel ratio ("DPR") for the screen
    it's on in a large font, as well as the inputs (from the platform
    plugin or environment) currently in use for determinging the DPR.

    Non-relevant inputs are not displayed. See qttools/src/qtdiag for
    an utility which displays all inputs.
*/

bool g_qtUsePhysicalDpi = false;
bool g_qtScaleFactor = false;

class DprGadget : public QWidget
{
public:
    std::function<void(void)> m_clearFn;
    std::function<void(void)> m_updateFn;
    qreal m_currentDpr = -1;

    DprGadget() {
        setWindowTitle("DprGadget");

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

        QLabel *dpiLabel = new QLabel("Logical DPI:");
        dpiLabel->setFont(smallFont);
        dpiLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        QLabel *sizeLabel = new QLabel("Window size:");
        sizeLabel->setFont(smallFont);
        sizeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        QLabel *platformDpiLabel = new QLabel("Native Device Pixel Ratio:");
        QLabel *plarformDprLabel = new QLabel("Native Logical DPI:");
        platformDpiLabel->setFont(smallFont);
        platformDpiLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        plarformDprLabel->setFont(smallFont);
        plarformDprLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        QLabel *screenLabel = new QLabel("Current Screen:");
        screenLabel->setFont(smallFont);

        QVBoxLayout *layout = new QVBoxLayout();
        layout->addWidget(dprLabel);
        layout->setAlignment(dprLabel, Qt::AlignHCenter);
        layout->addWidget(dprValue);
        layout->setAlignment(dprValue, Qt::AlignHCenter);
        layout->addWidget(sizeLabel);

        bool displayLogicalDpi = false;
        if (displayLogicalDpi)
            layout->addWidget(dpiLabel);

        if (g_qtScaleFactor) {
            QString text = QString("QT_SCALE_FACTOR ") + qgetenv("QT_SCALE_FACTOR");
            layout->addWidget(new QLabel(text));
        }

        layout->addStretch();
        layout->addWidget(screenLabel);
        layout->addWidget(platformDpiLabel);
        layout->addWidget(plarformDprLabel);

        auto updateValues = [=]() {
            dprValue->setText(QString("%1").arg(devicePixelRatioF()));
            dpiLabel->setText(QString("Logical DPI: %1").arg(logicalDpiX()));
            sizeLabel->setText(QString("Window size: %1 %2").arg(width()).arg(height()));

            QPlatformScreen *pscreen = screen()->handle();

            if (g_qtUsePhysicalDpi)
                platformDpiLabel->setText(QString("Native Physical DPI: TODO"));
            else
                platformDpiLabel->setText(QString("Native Logical DPI: %1").arg(pscreen->logicalDpi().first));

            plarformDprLabel->setText(QString("Native Device Pixel Ratio: %1").arg(pscreen->devicePixelRatio()));

            screenLabel->setText(QString("Current Screen: %1").arg(screen()->name()));
        };
        m_updateFn = updateValues;

        m_clearFn = [=]() {
            dprValue->setText(QString(""));
        };

        create();

        QObject::connect(this->windowHandle(), &QWindow::screenChanged, [updateValues](QScreen *screen){
            Q_UNUSED(screen);
            updateValues();
        });

        setLayout(layout);

        updateValues();
    }

    void paintEvent(QPaintEvent *) override  {
        // Dpr change should trigger a repaint, update display values here
        if (m_currentDpr == devicePixelRatioF())
            return;
        m_currentDpr = devicePixelRatioF();

        m_updateFn();
    }

    void resizeEvent(QResizeEvent *) override {
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

    // react to (some) high-dpi eviornment variables.
    g_qtUsePhysicalDpi = qgetenv("QT_USE_PHYSICAL_DPI") == QByteArray("1");
    g_qtScaleFactor = qEnvironmentVariableIsSet("QT_SCALE_FACTOR");

    QApplication app(argc, argv);

    DprGadget dprGadget;

    // Set initial size. We expect this size to be preserved across screen
    // and DPI changes
    dprGadget.resize(560, 380);

    dprGadget.show();

    return app.exec();
}
