// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui>
#include <QtWidgets>

#include <QtGui/qpa/qplatformscreen.h>
#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/qpa/qplatformcursor.h>

// This test app optionally doubles as a manual test for QScreen::mapToNative()
// #define TEST_MAP_TO_NATIVE

// ScreenDisplayer based on original impl from qtbase/tests/manual/highdpi
class ScreenDisplayer : public QWidget
{
public:
    ScreenDisplayer() = default;

    void setTitle() {

        QPoint deviceIndependentPos = pos();
        QPlatformWindow *platformWindow = windowHandle()->handle();
        QPoint nativePos = platformWindow->geometry().topLeft();

        QString windowDebug = QString("pos ")
            + QString("device independent %1 %2 ").arg(deviceIndependentPos.x()).arg(deviceIndependentPos.y())
            + QString("native %1 %2 ").arg(nativePos.x()).arg(nativePos.y())
        ;

        setWindowTitle(windowDebug);
    }

    void updateMapToNative(QPointF pos)
    {
#ifdef TEST_MAP_TO_NATIVE
        mappedToNative = QGuiApplication::mapToNative(QRectF(pos, QPointF(1,1))).topLeft();                  // there
        mappedFromNative = QGuiApplication::mapFromNative(QRectF(mappedToNative, QPointF(1,1))).topLeft();   // and back again
#endif
    }

    void timerEvent(QTimerEvent *) override
    {
        update();
        setTitle();
    }

    void mousePressEvent(QMouseEvent *) override
    {
        displayMappedToNativeCursor = true;
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        setTitle();
        updateMapToNative(e->globalPosition());
    }

    void mouseReleaseEvent(QMouseEvent *) override
    {
    }

    void showEvent(QShowEvent *) override
    {
        refreshTimer.start(60, this);
    }

    void hideEvent(QHideEvent *) override
    {
        refreshTimer.stop();
    }

    void paintScreensInRect(QPainter &p, QRect rect, bool native) {
        p.save();

        QRectF total;
        const auto screens = QGuiApplication::screens();
        for (const QScreen *screen : screens)
            total |= native ? screen->handle()->geometry() : screen->geometry();
        if (total.isEmpty())
            return;

        qreal scaleMargin = 0.9;
        scaleFactor = scaleMargin * qMin(rect.width()/total.width(), rect.height()/total.height());

       // p.fillRect(rect, Qt::black);
        int margin = 20;
        p.translate(margin, margin + rect.y());
        p.scale(scaleFactor, scaleFactor);
        p.translate(-total.topLeft());
        p.setPen(QPen(Qt::white, 10));
        p.setBrush(Qt::gray);

        for (const QScreen *screen : screens) {
            QRect geometry = native ? screen->handle()->geometry() : screen->geometry();
            p.drawRect(geometry);
            QFont f("Courier New");
            f.setPixelSize(geometry.height() / 16);
            p.setFont(f);

            if (displayInfo) {
                QString text =  "Name: " + screen->name() + "\n";
                text += QString("\nGeometry: %1 %2 %3 %4 \n ").arg(geometry.x()).arg(geometry.y()).arg(geometry.width()).arg(geometry.height());
                p.save();
                p.translate(20, 0);
                p.drawText(geometry, Qt::AlignLeft | Qt::AlignVCenter, text);
                p.restore();
            }
        }
        p.setBrush(QColor(200,220,255,127));

        const auto topLevels = QApplication::topLevelWidgets();
        for (QWidget *widget : topLevels) {
            if (!widget->isHidden())
                p.drawRect(native ? widget->windowHandle()->handle()->geometry() : widget->geometry());
        }

        QPolygon cursorShape;
        cursorShape << QPoint(0,0) << QPoint(20, 60)
                    << QPoint(30, 50) << QPoint(60, 80)
                    << QPoint(80, 60) << QPoint(50, 30)
                    << QPoint(60, 20);

        p.save();
        p.translate(native ? qApp->primaryScreen()->handle()->cursor()->pos() : QCursor::pos());
        p.drawPolygon(cursorShape);
        p.restore();

#ifdef TEST_MAP_TO_NATIVE
        // Draw red "mapped to native" cursor. We expect this
        // cursor to track the normal blue cursor if everything
        // works out ok.
        if (displayMappedToNativeCursor) {
            p.save();
            p.setBrush(QColor(230,120,155,127));
            p.translate(native ? mappedToNative: mappedFromNative);
            p.drawPolygon(cursorShape);
            p.restore();
        }
#endif

        p.restore();
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);

        QRect g = geometry();
        int halfHeight = g.height() / 2;
        QRect topHalf = QRect(0, 0, g.width(), halfHeight);
        QRect bottomHalf = QRect(0, halfHeight, g.width(), halfHeight);

        if (displayDeviceIndependentGeometry)
            paintScreensInRect(p, topHalf, false);
        if (displayNativeGeometry)
            paintScreensInRect(p, bottomHalf, true);
    }

    bool displayInfo = false;
    bool displayDeviceIndependentGeometry = true;
    bool displayNativeGeometry = false;
private:
    bool displayMappedToNativeCursor = false;
    QPointF mappedToNative;
    QPointF mappedFromNative;
    qreal scaleFactor = 1;
    QBasicTimer refreshTimer;
};

class Controller : public QWidget {
public:
    Controller(ScreenDisplayer *displayer) {
        setWindowTitle("Controller");

        QVBoxLayout *layout = new QVBoxLayout();
        setLayout(layout);

        layout->addWidget(new QLabel("Coordinate System:"));

        QCheckBox *deviceIndpendentGeometry = new QCheckBox("Device Independent");
        deviceIndpendentGeometry->setChecked(true);
        connect(deviceIndpendentGeometry, &QCheckBox::stateChanged, [displayer](int checked){ displayer->displayDeviceIndependentGeometry = checked > 0; });
        layout->addWidget(deviceIndpendentGeometry);

        QCheckBox *nativeGeometry = new QCheckBox("Native");
        nativeGeometry->setChecked(false);
        connect(nativeGeometry, &QCheckBox::stateChanged, [displayer](int checked){ displayer->displayNativeGeometry = checked > 0; });
        layout->addWidget(nativeGeometry);

        layout->addSpacing(40);

        QCheckBox *debug = new QCheckBox("Debug!");
        debug->setChecked(false);
        connect(debug, &QCheckBox::stateChanged, [displayer](int checked){ displayer->displayInfo = checked > 0; });
        layout->addWidget(debug);

        layout->addStretch();
    }
};

int main(int argc, char **argv) {

    QApplication app(argc, argv);

    ScreenDisplayer displayer;
    displayer.resize(300, 200);
    displayer.show();

    Controller controller(&displayer);
    controller.resize(100, 200);

    QTimer::singleShot(300, [&controller, &displayer](){
        controller.move(displayer.pos() + QPoint(displayer.width(), 0) + QPoint(50, 0));
        controller.show();
    });

    return app.exec();
}
