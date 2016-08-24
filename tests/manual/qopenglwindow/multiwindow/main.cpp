/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QGuiApplication>
#include <QOpenGLWindow>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QPainter>
#include <QElapsedTimer>
#include <QCommandLineParser>
#include <QScreen>

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
    Window(int n) : idx(n) {
        r = g = b = fps = 0;
        y = 0;
        resize(200, 200);

        connect(this, SIGNAL(frameSwapped()), SLOT(frameSwapped()));
        fpsTimer.start();
    }

    void paintGL() {
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
        f->glClearColor(r, g, b, 1);
        f->glClear(GL_COLOR_BUFFER_BIT);
        switch (idx % 3) {
        case 0:
            r += 0.005f;
            break;
        case 1:
            g += 0.005f;
            break;
        case 2:
            b += 0.005f;
            break;
        }
        if (r > 1)
            r = 0;
        if (g > 1)
            g = 0;
        if (b > 1)
            b = 0;

        QPainter p(this);
        p.setPen(Qt::white);
        p.drawText(QPoint(20, y), QString(QLatin1String("Window %1 (%2 FPS)")).arg(idx).arg(fps));
        y += 1;
        if (y > height() - 20)
            y = 20;

        update();
    }

public slots:
    void frameSwapped() {
        ++framesSwapped;
        if (fpsTimer.elapsed() > 1000) {
            fps = qRound(framesSwapped * (1000.0 / fpsTimer.elapsed()));
            framesSwapped = 0;
            fpsTimer.restart();
        }
    }

private:
    int idx;
    GLfloat r, g, b;
    int y;

    int framesSwapped;
    QElapsedTimer fpsTimer;
    int fps;
};

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

    QSurfaceFormat fmt;
    if (parser.isSet(noVsyncOption)) {
        qDebug("swap interval 0 (no throttling)");
        fmt.setSwapInterval(0);
    } else {
        qDebug("swap interval 1 (sync to vblank)");
    }
    QSurfaceFormat::setDefaultFormat(fmt);

    QRect availableGeometry = app.primaryScreen()->availableGeometry();

    int numberOfWindows = qMax(parser.value(numWindowsOption).toInt(), 1);
    QList<QWindow *> windows;
    for (int i = 0; i < numberOfWindows; ++i) {
        Window *w = new Window(i + 1);
        windows << w;

        if (i == 0 && parser.isSet(vsyncOneOption)) {
            qDebug("swap interval 1 for first window only");
            QSurfaceFormat vsyncedSurfaceFormat = fmt;
            vsyncedSurfaceFormat.setSwapInterval(1);
            w->setFormat(vsyncedSurfaceFormat);
            fmt.setSwapInterval(0);
            QSurfaceFormat::setDefaultFormat(fmt);
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
        w->show();
    }

    int r = app.exec();
    qDeleteAll(windows);
    return r;
}

#include "main.moc"
