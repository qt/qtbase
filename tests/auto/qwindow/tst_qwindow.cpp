/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qwindow.h>

#include <QtTest/QtTest>

#include "../../shared/util.h"

class tst_QWindow: public QObject
{
    Q_OBJECT

private slots:
    void mapGlobal();
    void positioning();
};


void tst_QWindow::mapGlobal()
{
    QWindow a;
    QWindow b(&a);
    QWindow c(&b);

    a.setGeometry(10, 10, 300, 300);
    b.setGeometry(20, 20, 200, 200);
    c.setGeometry(40, 40, 100, 100);

    QCOMPARE(a.mapToGlobal(QPoint(100, 100)), QPoint(110, 110));
    QCOMPARE(b.mapToGlobal(QPoint(100, 100)), QPoint(130, 130));
    QCOMPARE(c.mapToGlobal(QPoint(100, 100)), QPoint(170, 170));

    QCOMPARE(a.mapFromGlobal(QPoint(100, 100)), QPoint(90, 90));
    QCOMPARE(b.mapFromGlobal(QPoint(100, 100)), QPoint(70, 70));
    QCOMPARE(c.mapFromGlobal(QPoint(100, 100)), QPoint(30, 30));
}

class Window : public QWindow
{
public:
    Window()
        : gotResizeEvent(false)
        , gotMapEvent(false)
        , gotMoveEvent(false)
    {
        setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    }

    bool event(QEvent *event)
    {
        switch (event->type()) {
        case QEvent::Map:
            gotMapEvent = true;
            break;
        case QEvent::Resize:
            gotResizeEvent = true;
            break;
        case QEvent::Move:
            gotMoveEvent = true;
            break;
        default:
            break;
        }

        return QWindow::event(event);
    }

    bool gotResizeEvent;
    bool gotMapEvent;
    bool gotMoveEvent;
};

void tst_QWindow::positioning()
{
    QRect geometry(80, 80, 40, 40);

    Window window;
    window.setGeometry(geometry);
    QCOMPARE(window.geometry(), geometry);
    window.show();

    QTRY_VERIFY(window.gotResizeEvent && window.gotMapEvent);

    QMargins originalMargins = window.frameMargins();

    QCOMPARE(window.pos(), window.framePos() + QPoint(originalMargins.left(), originalMargins.top()));
    QVERIFY(window.frameGeometry().contains(window.geometry()));

    QPoint originalPos = window.pos();
    QPoint originalFramePos = window.framePos();

    window.gotResizeEvent = false;

    window.setWindowState(Qt::WindowFullScreen);
    QTRY_VERIFY(window.gotResizeEvent);

    window.gotResizeEvent = false;
    window.setWindowState(Qt::WindowNoState);
    QTRY_VERIFY(window.gotResizeEvent);

    QTRY_COMPARE(originalPos, window.pos());
    QTRY_COMPARE(originalFramePos, window.framePos());
    QTRY_COMPARE(originalMargins, window.frameMargins());

    // if our positioning is actually fully respected by the window manager
    // test whether it correctly handles frame positioning as well
    if (originalPos == geometry.topLeft() && (originalMargins.top() != 0 || originalMargins.left() != 0)) {
        QPoint framePos(40, 40);

        window.gotMoveEvent = false;
        window.setFramePos(framePos);

        QTRY_VERIFY(window.gotMoveEvent);
        QTRY_COMPARE(framePos, window.framePos());
        QTRY_COMPARE(originalMargins, window.frameMargins());
        QCOMPARE(window.pos(), window.framePos() + QPoint(originalMargins.left(), originalMargins.top()));

        // and back to regular positioning

        window.gotMoveEvent = false;
        window.setPos(originalPos);
        QTRY_VERIFY(window.gotMoveEvent);
        QTRY_COMPARE(originalPos, window.pos());
    }
}

#include <tst_qwindow.moc>
QTEST_MAIN(tst_QWindow);
