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
