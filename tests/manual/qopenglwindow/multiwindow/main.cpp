/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtGui>

const char applicationDescription[] = "\n\
This application opens multiple windows and continuously schedules updates for\n\
them. Each of them is a separate QOpenGLWindow so there will be a separate\n\
context and swapBuffers call for each.\n\
\n\
By default the swap interval is 1 so the effect of multiple blocking swapBuffers\n\
on the main thread can be examined. (the result is likely to be different\n\
between platforms, for example OS X is buffer queuing meaning that it can\n\
block outside swap, resulting in perfect vsync for all three windows, while\n\
other systems that block on swap will kill the frame rate due to blocking the\n\
thread three times)\
";

// For reference, below is a table of some test results.
//
//                                    swap interval 1 for all             swap interval 1 for only one and 0 for others
// --------------------------------------------------------------------------------------------------------------------
// OS X (Intel HD)                    60 FPS for all                      60 FPS for all
// Windows Intel opengl32             20 FPS for all (each swap blocks)   60 FPS for all
// Windows ANGLE D3D9/D3D11           60 FPS for all                      60 FPS for all
// Windows ANGLE D3D11 WARP           20 FPS for all                      60 FPS for all
// Windows Mesa llvmpipe              does not really vsync anyway

class Window : public QOpenGLWindow
{
    Q_OBJECT
public:
    Window(int index) : windowNumber(index + 1), x(0), framesSwapped(0) {

        color = QColor::fromHsl((index * 30) % 360, 255, 127).toRgb();

        resize(200, 200);

        setObjectName(QString("Window %1").arg(windowNumber));

        connect(this, SIGNAL(frameSwapped()), SLOT(frameSwapped()));
    }

    void paintGL() {
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
        f->glClearColor(color.redF(), color.greenF(), color.blueF(), 1);
        f->glClear(GL_COLOR_BUFFER_BIT);

        QPainter painter(this);
        painter.drawLine(x, 0, x, height());
        x = ++x % width();
    }

public slots:
    void frameSwapped() {
        ++framesSwapped;
        update();
    }

protected:
    void exposeEvent(QExposeEvent *event) {
        if (!isExposed())
            return;

        QSurfaceFormat format = context()->format();
        qDebug() << this << format.swapBehavior() << "with Vsync =" << (format.swapInterval() ? "ON" : "OFF");
        if (format.swapInterval() != requestedFormat().swapInterval())
            qWarning() << "WARNING: Did not get requested swap interval of" << requestedFormat().swapInterval() << "for" << this;

        QOpenGLWindow::exposeEvent(event);
    }

    void mousePressEvent(QMouseEvent *event) {
        qDebug() << this << event;
        color.setHsl((color.hue() + 90) % 360, color.saturation(), color.lightness());
        color = color.toRgb();
    }

private:
    int windowNumber;
    QColor color;
    int x;

    int framesSwapped;
    friend void printFps();
};

static const qreal kFpsInterval = 500;

void printFps()
{
    static QElapsedTimer timer;
    if (!timer.isValid()) {
        timer.start();
        return;
    }

    const qreal frameFactor = (kFpsInterval / timer.elapsed()) * (1000.0 / kFpsInterval);

    QDebug output = qDebug().nospace();

    qreal averageFps = 0;
    const QWindowList windows = QGuiApplication::topLevelWindows();
    for (int i = 0; i < windows.size(); ++i) {
        Window *w = qobject_cast<Window*>(windows.at(i));
        Q_ASSERT(w);

        int fps = qRound(w->framesSwapped * frameFactor);
        output << (i + 1) << "=" << fps << ", ";

        averageFps += fps;
        w->framesSwapped = 0;
    }
    averageFps = qRound(averageFps / windows.size());
    qreal msPerFrame = 1000.0 / averageFps;

    output << "avg=" << averageFps << ", ms=" << msPerFrame;

    timer.restart();
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(applicationDescription);
    parser.addHelpOption();

    QCommandLineOption noVsyncOption("novsync", "Disable Vsync by setting swap interval to 0. "
        "This should give an unthrottled refresh on all platforms for all windows.");
    parser.addOption(noVsyncOption);

    QCommandLineOption vsyncOneOption("vsyncone", "Enable Vsync only for first window, "
        "by setting swap interval to 1 for the first window and 0 for the others.");
    parser.addOption(vsyncOneOption);

    QCommandLineOption numWindowsOption("numwindows", "Open <N> windows instead of the default 3.", "N", "3");
    parser.addOption(numWindowsOption);

    parser.process(app);

    QSurfaceFormat defaultSurfaceFormat;
    defaultSurfaceFormat.setSwapInterval(parser.isSet(noVsyncOption) ? 0 : 1);
    QSurfaceFormat::setDefaultFormat(defaultSurfaceFormat);

    QRect availableGeometry = app.primaryScreen()->availableGeometry();

    int numberOfWindows = qMax(parser.value(numWindowsOption).toInt(), 1);
    QList<QWindow *> windows;
    for (int i = 0; i < numberOfWindows; ++i) {
        Window *w = new Window(i);
        windows << w;

        if (i == 0 && parser.isSet(vsyncOneOption)) {
            QSurfaceFormat vsyncedSurfaceFormat = defaultSurfaceFormat;
            vsyncedSurfaceFormat.setSwapInterval(1);
            w->setFormat(vsyncedSurfaceFormat);
            defaultSurfaceFormat.setSwapInterval(0);
            QSurfaceFormat::setDefaultFormat(defaultSurfaceFormat);
        }

        static int windowWidth = w->width() + 20;
        static int windowHeight = w->height() + 20;

        static int windowsPerRow = availableGeometry.width() / windowWidth;

        int col = i;
        int row = col / windowsPerRow;
        col -= row * windowsPerRow;

        QPoint position = availableGeometry.topLeft();
        position += QPoint(col * windowWidth, row * windowHeight);
        w->setFramePosition(position);
        w->showNormal();
    }

    QTimer fpsTimer;
    fpsTimer.setInterval(kFpsInterval);
    fpsTimer.setTimerType(Qt::PreciseTimer);
    QObject::connect(&fpsTimer, &QTimer::timeout, &printFps);
    fpsTimer.start();

    int r = app.exec();
    qDeleteAll(windows);
    return r;
}

#include "main.moc"
