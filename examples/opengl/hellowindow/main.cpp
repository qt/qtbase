/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "hellowindow.h"

#include <qpa/qplatformintegration.h>

#include <QGuiApplication>
#include <QScreen>
#include <QThread>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Some platforms can only have one window per screen. Therefore we need to differentiate.
    const bool multipleWindows = QGuiApplication::arguments().contains(QStringLiteral("--multiple"));
    const bool multipleScreens = QGuiApplication::arguments().contains(QStringLiteral("--multiscreen"));

    QScreen *screen = QGuiApplication::primaryScreen();

    QRect screenGeometry = screen->availableGeometry();

    QSurfaceFormat format;
    format.setDepthBufferSize(16);
    if (QGuiApplication::arguments().contains(QStringLiteral("--multisample")))
        format.setSamples(4);

    QPoint center = QPoint(screenGeometry.center().x(), screenGeometry.top() + 80);
    QSize windowSize(400, 320);
    int delta = 40;

    QList<QWindow *> windows;
    QSharedPointer<Renderer> rendererA(new Renderer(format));

    HelloWindow *windowA = new HelloWindow(rendererA);
    windowA->setGeometry(QRect(center, windowSize).translated(-windowSize.width() - delta / 2, 0));
    windowA->setTitle(QStringLiteral("Thread A - Context A"));
    windowA->setVisible(true);
    windows.prepend(windowA);

    QList<QThread *> renderThreads;
    if (multipleWindows) {
        QSharedPointer<Renderer> rendererB(new Renderer(format, rendererA.data()));

        QThread *renderThread = new QThread;
        rendererB->moveToThread(renderThread);
        renderThreads << renderThread;

        HelloWindow *windowB = new HelloWindow(rendererA);
        windowB->setGeometry(QRect(center, windowSize).translated(delta / 2, 0));
        windowB->setTitle(QStringLiteral("Thread A - Context A"));
        windowB->setVisible(true);
        windows.prepend(windowB);

        HelloWindow *windowC = new HelloWindow(rendererB);
        windowC->setGeometry(QRect(center, windowSize).translated(-windowSize.width() / 2, windowSize.height() + delta));
        windowC->setTitle(QStringLiteral("Thread B - Context B"));
        windowC->setVisible(true);
        windows.prepend(windowC);
    }
    if (multipleScreens) {
        for (int i = 1; i < QGuiApplication::screens().size(); ++i) {
            QScreen *screen = QGuiApplication::screens().at(i);
            QSharedPointer<Renderer> renderer(new Renderer(format, rendererA.data(), screen));

            QThread *renderThread = new QThread;
            renderer->moveToThread(renderThread);
            renderThreads.prepend(renderThread);

            QRect screenGeometry = screen->availableGeometry();
            QPoint center = screenGeometry.center();

            QSize windowSize = screenGeometry.size() * 0.8;

            HelloWindow *window = new HelloWindow(renderer, screen);
            window->setGeometry(QRect(center, windowSize).translated(-windowSize.width() / 2, -windowSize.height() / 2));

            QChar id = QChar('B' + i);
            window->setTitle(QStringLiteral("Thread ") + id + QStringLiteral(" - Context ") + id);
            window->setVisible(true);
            windows.prepend(window);
        }
    }

    for (int i = 0; i < renderThreads.size(); ++i) {
        QObject::connect(qGuiApp, &QGuiApplication::lastWindowClosed, renderThreads.at(i), &QThread::quit);
        renderThreads.at(i)->start();
    }

    // Quit after 10 seconds. For platforms that do not have windows that are closeable.
    if (QCoreApplication::arguments().contains(QStringLiteral("--timeout")))
        QTimer::singleShot(10000, qGuiApp, &QCoreApplication::quit);

    const int exitValue = app.exec();

    for (int i = 0; i < renderThreads.size(); ++i) {
        renderThreads.at(i)->quit(); // some platforms may not have windows to close so ensure quit()
        renderThreads.at(i)->wait();
    }

    qDeleteAll(windows);
    qDeleteAll(renderThreads);

    return exitValue;
}
