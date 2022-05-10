// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QtGui>

// This example demonstrates how to create QGuiApplication
// without calling exec(), and then exiting main() without
// shutting down the Qt event loop.

class ClickWindow: public QRasterWindow
{
public:

    ClickWindow() {
        qDebug() << "ClickWindow constructor";
    }
    ~ClickWindow() {
        qDebug() << "ClickWindow destructor";
    }

    void paintEvent(QPaintEvent *ev) override {
        QPainter p(this);
        p.fillRect(ev->rect(), QColorConstants::Svg::deepskyblue);
        p.drawText(50, 100, "Application has started. See the developer tools console for debug output");
    }

    void mousePressEvent(QMouseEvent *) override {
        qDebug() << "mousePressEvent(): calling QGuiApplication::quit()";
        QGuiApplication::quit();
    }
};

int main(int argc, char **argv)
{
    qDebug() << "main(): Creating QGuiApplication object";
    QGuiApplication *app = new QGuiApplication(argc, argv);

    QObject::connect(app, &QCoreApplication::aboutToQuit, [](){
        qDebug() << "QCoreApplication::aboutToQuit";
    });

    qDebug() << "main(): Creating ClickWindow object";
    ClickWindow *window = new ClickWindow();
    window->show();

    // We can exit main; the Qt event loop and the emscripten runtime
    // will keep running, as long as Emscriptens EXIT_RUNTIME option
    // has not been enabled.

    qDebug() << "main(): exit";
}

// Global variables are created before main() as usual, but not destroyed
class Global
{
public:
    Global() {
        qDebug() << "Global constructor";
    }
    ~Global() {
        qDebug() << "Global destructor"; // <- will not be printed
    }
};
Global global;



