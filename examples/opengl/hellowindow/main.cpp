/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QGuiApplication>
#include <QScreen>
#include <QThread>

#include "hellowindow.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    bool multipleWindows = !QGuiApplication::arguments().contains(QLatin1String("--single"));

    QScreen *screen = QGuiApplication::primaryScreen();

    QRect screenGeometry = screen->availableGeometry();

    QSurfaceFormat format;
    format.setDepthBufferSize(16);
    format.setSamples(4);

    QPoint center = QPoint(screenGeometry.center().x(), screenGeometry.top() + 80);
    QSize windowSize(400, 320);
    int delta = 40;

    Renderer *rendererA = new Renderer(format);
    HelloWindow *windowA = new HelloWindow(rendererA);
    windowA->setGeometry(QRect(center, windowSize).translated(-windowSize.width() - delta / 2, 0));
    windowA->setWindowTitle(QLatin1String("Thread A - Context A"));
    windowA->setVisible(true);

    QThread *renderThread = 0;
    if (multipleWindows) {
        Renderer *rendererB = new Renderer(format, rendererA);

        renderThread = new QThread;
        rendererB->moveToThread(renderThread);
        renderThread->start();

        QObject::connect(qGuiApp, SIGNAL(lastWindowClosed()), renderThread, SLOT(quit()));

        HelloWindow *windowB = new HelloWindow(rendererA);
        windowB->setGeometry(QRect(center, windowSize).translated(delta / 2, 0));
        windowB->setWindowTitle(QLatin1String("Thread A - Context A"));
        windowB->setVisible(true);

        HelloWindow *windowC = new HelloWindow(rendererB);
        windowC->setGeometry(QRect(center, windowSize).translated(-windowSize.width() / 2, windowSize.height() + delta));
        windowC->setWindowTitle(QLatin1String("Thread B - Context B"));
        windowC->setVisible(true);
    }

    app.exec();

    if (multipleWindows)
        renderThread->wait();
}
