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

// This application opens three windows and continuously schedules updates for
// them.  Each of them is a separate QOpenGLWindow so there will be a separate
// context and swapBuffers call for each.
//
// By default the swap interval is 1 so the effect of three blocking swapBuffers
// on the main thread can be examined. (the result is likely to be different
// between platforms, for example OS X is buffer queuing meaning that it can
// block outside swap, resulting in perfect vsync for all three windows, while
// other systems that block on swap will kill the frame rate due to blocking the
// thread three times)
//
// Pass --novsync to set a swap interval of 0. This should give an unthrottled
// refresh on all platforms for all three windows.
//
// Passing --vsyncone sets swap interval to 1 for the first window and 0 to the
// others.
//
// Pass --extrawindows N to open N windows in addition to the default 3.
//
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
public:
    Window(int n) : idx(n) {
        r = g = b = fps = 0;
        y = 0;
        resize(200, 200);
        t2.start();
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

        if (t2.elapsed() > 1000) {
            fps = 1000.0 / t.elapsed();
            t2.restart();
        }
        t.restart();

        update();
    }

private:
    int idx;
    GLfloat r, g, b, fps;
    int y;
    QElapsedTimer t, t2;
};

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QSurfaceFormat fmt;
    if (QGuiApplication::arguments().contains(QLatin1String("--novsync"))) {
        qDebug("swap interval 0 (no throttling)");
        fmt.setSwapInterval(0);
    } else {
        qDebug("swap interval 1 (sync to vblank)");
    }
    QSurfaceFormat::setDefaultFormat(fmt);

    Window w1(0);
    if (QGuiApplication::arguments().contains(QLatin1String("--vsyncone"))) {
        qDebug("swap interval 1 for first window only");
        QSurfaceFormat w1fmt = fmt;
        w1fmt.setSwapInterval(1);
        w1.setFormat(w1fmt);
        fmt.setSwapInterval(0);
        QSurfaceFormat::setDefaultFormat(fmt);
    }
    Window w2(1);
    Window w3(2);
    w1.setGeometry(QRect(QPoint(10, 100), w1.size()));
    w2.setGeometry(QRect(QPoint(300, 100), w2.size()));
    w3.setGeometry(QRect(QPoint(600, 100), w3.size()));
    w1.show();
    w2.show();
    w3.show();

    QList<QWindow *> extraWindows;
    int countIdx;
    if ((countIdx = QGuiApplication::arguments().indexOf(QLatin1String("--extrawindows"))) >= 0) {
        int extraWindowCount = QGuiApplication::arguments().at(countIdx + 1).toInt();
        for (int i = 0; i < extraWindowCount; ++i) {
            Window *w = new Window(3 + i);
            extraWindows << w;
            w->show();
        }
    }

    int r = app.exec();
    qDeleteAll(extraWindows);
    return r;
}
