// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QScreen>
#include <QRect>
#include <QSharedPointer>

#include "window.h"

int main(int argc, char **argv)
{
    typedef QSharedPointer<QWindow> WindowPtr;

    QGuiApplication app(argc, argv);

    Window a;
    a.setFramePosition(QPoint(10, 10));
    a.setTitle(QStringLiteral("Window A"));
    a.setObjectName(a.title());
    a.setVisible(true);

    Window b;
    b.setFramePosition(QPoint(100, 100));
    b.setTitle(QStringLiteral("Window B"));
    b.setObjectName(b.title());
    b.setVisible(true);

    Window child(&b);
    child.setObjectName(QStringLiteral("ChildOfB"));
    child.setVisible(true);

    // create one window on each additional screen as well

    QList<WindowPtr> windows;
    const QList<QScreen *> screens = app.screens();
    for (QScreen *screen : screens) {
        if (screen == app.primaryScreen())
            continue;
        WindowPtr window(new Window(screen));
        QRect geometry = window->geometry();
        geometry.moveCenter(screen->availableGeometry().center());
        window->setGeometry(geometry);
        window->setVisible(true);
        window->setTitle(screen->name());
        window->setObjectName(window->title());
        windows.push_back(window);
    }
    return app.exec();
}
