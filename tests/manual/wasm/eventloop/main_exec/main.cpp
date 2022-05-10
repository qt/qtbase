// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QtGui>

// This example demonstrates how the standard Qt main()
// pattern works on Emscripten/WebAssambly, where exec()
// does not return.

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
    QGuiApplication app(argc, argv);

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [](){
        qDebug() << "QCoreApplication::aboutToQuit";
    });

    ClickWindow window;
    window.show();

    qDebug() << "main(): calling exec()";
    app.exec();

    // The exec() call above never returns; instead, a JavaScript exception
    // is thrown such that control returns to the browser while preserving
    // the C++ stack.

    // This means that the window object above is not destroyed, and that
    // shutdown code after exec() does not run.

    qDebug() << "main(): after exit"; // <- will not be printed
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
