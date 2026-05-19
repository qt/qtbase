// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Touch/Mouse tester"));
    parser.addHelpOption();
    const QCommandLineOption mouseMoveOption(QStringLiteral("mousemove"),
                                             QStringLiteral("Log mouse move events"));
    parser.addOption(mouseMoveOption);
    const QCommandLineOption globalFilterOption(QStringLiteral("global"),
                                             QStringLiteral("Global event filter"));
    parser.addOption(globalFilterOption);

    const QCommandLineOption ignoreTouchOption(QStringLiteral("ignore"),
                                               QStringLiteral("Ignore touch events (for testing mouse emulation)."));
    parser.addOption(ignoreTouchOption);
    const QCommandLineOption noTouchLogOption(QStringLiteral("notouchlog"),
                                              QStringLiteral("Do not log touch events (for testing gestures)."));
    parser.addOption(noTouchLogOption);
    const QCommandLineOption noMouseLogOption(QStringLiteral("nomouselog"),
                                              QStringLiteral("Do not log mouse events (for testing gestures)."));
    parser.addOption(noMouseLogOption);
    const QCommandLineOption noGestureLogOption(QStringLiteral("nogesturelog"),
                                                QStringLiteral("Do not log gesture events."));
    parser.addOption(noGestureLogOption);
    const QCommandLineOption noWheelOption(QStringLiteral("nowheel"),
                                           QStringLiteral("Do not log wheel events."));
    parser.addOption(noWheelOption);
    const QCommandLineOption noNativeGestureOption(QStringLiteral("nonative-gesture"),
                                                   QStringLiteral("Do not log native gesture events."));
    parser.addOption(noNativeGestureOption);

    const QCommandLineOption tapGestureOption(QStringLiteral("tap"), QStringLiteral("Grab tap gesture."));
    parser.addOption(tapGestureOption);
    const QCommandLineOption tapAndHoldGestureOption(QStringLiteral("tap-and-hold"),
                                                     QStringLiteral("Grab tap-and-hold gesture."));
    parser.addOption(tapAndHoldGestureOption);
    const QCommandLineOption panGestureOption(QStringLiteral("pan"), QStringLiteral("Grab pan gesture."));
    parser.addOption(panGestureOption);
    const QCommandLineOption pinchGestureOption(QStringLiteral("pinch"), QStringLiteral("Grab pinch gesture."));
    parser.addOption(pinchGestureOption);
    const QCommandLineOption swipeGestureOption(QStringLiteral("swipe"), QStringLiteral("Grab swipe gesture."));
    parser.addOption(swipeGestureOption);
    parser.process(QApplication::arguments());
    optIgnoreTouch = parser.isSet(ignoreTouchOption);
    if (parser.isSet(tapGestureOption))
        optGestures.append(Qt::TapGesture);
    if (parser.isSet(tapAndHoldGestureOption))
        optGestures.append(Qt::TapAndHoldGesture);
    if (parser.isSet(panGestureOption))
        optGestures.append(Qt::PanGesture);
    if (parser.isSet(pinchGestureOption))
        optGestures.append(Qt::PinchGesture);
    if (parser.isSet(swipeGestureOption))
        optGestures.append(Qt::SwipeGesture);
    // If no gesture was selected on the command line, default to the full
    // standard set so the widget grabs them, the gesture event filter logs
    // them, and TouchTestWidget::handleGestureEvent visualises them.
    if (optGestures.isEmpty()) {
        optGestures << Qt::TapGesture << Qt::TapAndHoldGesture
                    << Qt::PanGesture << Qt::PinchGesture << Qt::SwipeGesture;
    }

    if (!parser.isSet(noMouseLogOption))
        eventTypes << QEvent::MouseButtonPress << QEvent::MouseButtonRelease << QEvent::MouseButtonDblClick;
    if (parser.isSet(mouseMoveOption))
        eventTypes << QEvent::MouseMove;
    if (!parser.isSet(noTouchLogOption))
        eventTypes << QEvent::TouchBegin << QEvent::TouchUpdate << QEvent::TouchEnd;
    if (!parser.isSet(noGestureLogOption))
        eventTypes << QEvent::Gesture << QEvent::GestureOverride;
    if (!parser.isSet(noWheelOption))
        eventTypes << QEvent::Wheel;
    if (!parser.isSet(noNativeGestureOption))
        eventTypes << QEvent::NativeGesture;
    if (parser.isSet(globalFilterOption)) {
        globalEventFilter = new EventFilter(eventTypes, &a);
        a.installEventFilter(globalEventFilter);
    }

    MainWindow::createMainWindow();

    const int exitCode = a.exec();
    qDeleteAll(mainWindows);
    return exitCode;
}
