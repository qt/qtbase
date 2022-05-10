// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "hellowindow.h"

#include <qpa/qplatformintegration.h>

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QGuiApplication>
#include <QScreen>
#include <QThread>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setApplicationName("Qt HelloWindow GL Example");
    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::applicationName());
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption multipleOption("multiple", "Create multiple windows");
    parser.addOption(multipleOption);
    QCommandLineOption multipleSampleOption("multisample", "Multisampling");
    parser.addOption(multipleSampleOption);
    QCommandLineOption multipleScreenOption("multiscreen", "Run on multiple screens");
    parser.addOption(multipleScreenOption);
    QCommandLineOption timeoutOption("timeout", "Close after 10s");
    parser.addOption(timeoutOption);
    parser.process(app);

    // Some platforms can only have one window per screen. Therefore we need to differentiate.
    const bool multipleWindows = parser.isSet(multipleOption);
    const bool multipleScreens = parser.isSet(multipleScreenOption);

    QScreen *screen = QGuiApplication::primaryScreen();

    QRect screenGeometry = screen->availableGeometry();

    QSurfaceFormat format;
    format.setDepthBufferSize(16);
    if (parser.isSet(multipleSampleOption))
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
    if (parser.isSet(timeoutOption))
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
