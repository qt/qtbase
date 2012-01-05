/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QEvent>

class tst_QWindow: public QObject
{
    Q_OBJECT

private slots:
    void mapGlobal();
    void positioning();
    void isActive();
    void testInputEvents();
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
    {
        reset();
        setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    }

    void reset()
    {
        m_received.clear();
    }

    bool event(QEvent *event)
    {
        m_received[event->type()]++;

        return QWindow::event(event);
    }

    int received(QEvent::Type type)
    {
        return m_received.value(type, 0);
    }

private:
    QHash<QEvent::Type, int> m_received;
};

void tst_QWindow::positioning()
{
    QRect geometry(80, 80, 40, 40);

    Window window;
    window.setGeometry(geometry);
    QCOMPARE(window.geometry(), geometry);
    window.show();

    QTRY_COMPARE(window.received(QEvent::Resize), 1);
    QTRY_COMPARE(window.received(QEvent::Map), 1);

    QMargins originalMargins = window.frameMargins();

    QCOMPARE(window.pos(), window.framePos() + QPoint(originalMargins.left(), originalMargins.top()));
    QVERIFY(window.frameGeometry().contains(window.geometry()));

    QPoint originalPos = window.pos();
    QPoint originalFramePos = window.framePos();

    window.setWindowState(Qt::WindowFullScreen);
    QTRY_COMPARE(window.received(QEvent::Resize), 2);

    window.setWindowState(Qt::WindowNoState);
    QTRY_COMPARE(window.received(QEvent::Resize), 3);

    QTRY_COMPARE(originalPos, window.pos());
    QTRY_COMPARE(originalFramePos, window.framePos());
    QTRY_COMPARE(originalMargins, window.frameMargins());

    // if our positioning is actually fully respected by the window manager
    // test whether it correctly handles frame positioning as well
    if (originalPos == geometry.topLeft() && (originalMargins.top() != 0 || originalMargins.left() != 0)) {
        QPoint framePos(40, 40);

        window.reset();
        window.setFramePos(framePos);

        QTRY_VERIFY(window.received(QEvent::Move));
        QTRY_COMPARE(framePos, window.framePos());
        QTRY_COMPARE(originalMargins, window.frameMargins());
        QCOMPARE(window.pos(), window.framePos() + QPoint(originalMargins.left(), originalMargins.top()));

        // and back to regular positioning

        window.reset();
        window.setPos(originalPos);
        QTRY_VERIFY(window.received(QEvent::Move));
        QTRY_COMPARE(originalPos, window.pos());
    }
}

void tst_QWindow::isActive()
{
    Window window;
    window.setGeometry(80, 80, 40, 40);
    window.show();

    QTRY_COMPARE(window.received(QEvent::Map), 1);
    QTRY_COMPARE(window.received(QEvent::Resize), 1);
    QTRY_VERIFY(QGuiApplication::focusWindow() == &window);
    QVERIFY(window.isActive());

    Window child;
    child.setParent(&window);
    child.setGeometry(10, 10, 20, 20);
    child.show();

    QTRY_COMPARE(child.received(QEvent::Map), 1);

    child.requestActivateWindow();

    QTRY_VERIFY(QGuiApplication::focusWindow() == &child);
    QVERIFY(child.isActive());

    // parent shouldn't receive new map or resize events from child being shown
    QTRY_COMPARE(window.received(QEvent::Map), 1);
    QTRY_COMPARE(window.received(QEvent::Resize), 1);
    QTRY_COMPARE(window.received(QEvent::FocusIn), 1);
    QTRY_COMPARE(window.received(QEvent::FocusOut), 1);
    QTRY_COMPARE(child.received(QEvent::FocusIn), 1);

    // child has focus
    QVERIFY(window.isActive());

    Window dialog;
    dialog.setTransientParent(&window);
    dialog.setGeometry(110, 110, 30, 30);
    dialog.show();

    dialog.requestActivateWindow();

    QTRY_COMPARE(dialog.received(QEvent::Map), 1);
    QTRY_COMPARE(dialog.received(QEvent::Resize), 1);
    QTRY_VERIFY(QGuiApplication::focusWindow() == &dialog);
    QVERIFY(dialog.isActive());

    // transient child has focus
    QVERIFY(window.isActive());

    // parent is active
    QVERIFY(child.isActive());

    window.requestActivateWindow();

    QTRY_VERIFY(QGuiApplication::focusWindow() == &window);
    QTRY_COMPARE(dialog.received(QEvent::FocusOut), 1);
    QTRY_COMPARE(window.received(QEvent::FocusIn), 2);

    QVERIFY(window.isActive());

    // transient parent has focus
    QVERIFY(dialog.isActive());

    // parent has focus
    QVERIFY(child.isActive());
}

class InputTestWindow : public QWindow
{
public:
    void keyPressEvent(QKeyEvent *event) {
        keyPressCode = event->key();
    }
    void keyReleaseEvent(QKeyEvent *event) {
        keyReleaseCode = event->key();
    }
    void mousePressEvent(QMouseEvent *event) {
        mousePressButton = event->button();
    }
    void mouseReleaseEvent(QMouseEvent *event) {
        mouseReleaseButton = event->button();
    }
    void touchEvent(QTouchEvent *event) {
        QList<QTouchEvent::TouchPoint> points = event->touchPoints();
        for (int i = 0; i < points.count(); ++i) {
            switch (points.at(i).state()) {
            case Qt::TouchPointPressed:
                ++touchPressedCount;
                break;
            case Qt::TouchPointReleased:
                ++touchReleasedCount;
                break;
            }
        }
    }

    InputTestWindow() {
        keyPressCode = keyReleaseCode = 0;
        mousePressButton = mouseReleaseButton = 0;
        touchPressedCount = touchReleasedCount = 0;
    }

    int keyPressCode, keyReleaseCode;
    int mousePressButton, mouseReleaseButton;
    int touchPressedCount, touchReleasedCount;
};

void tst_QWindow::testInputEvents()
{
    InputTestWindow window;
    window.setGeometry(80, 80, 40, 40);
    window.show();
    QTest::qWaitForWindowShown(&window);

    QWindowSystemInterface::handleKeyEvent(&window, QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
    QWindowSystemInterface::handleKeyEvent(&window, QEvent::KeyRelease, Qt::Key_A, Qt::NoModifier);
    QCoreApplication::processEvents();
    QCOMPARE(window.keyPressCode, int(Qt::Key_A));
    QCOMPARE(window.keyReleaseCode, int(Qt::Key_A));

    QPointF local(12, 34);
    QWindowSystemInterface::handleMouseEvent(&window, local, local, Qt::LeftButton);
    QWindowSystemInterface::handleMouseEvent(&window, local, local, Qt::NoButton);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressButton, int(Qt::LeftButton));
    QCOMPARE(window.mouseReleaseButton, int(Qt::LeftButton));

    QTouchDevice *device = new QTouchDevice;
    device->setType(QTouchDevice::TouchScreen);
    QWindowSystemInterface::registerTouchDevice(device);
    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp1, tp2;
    tp1.id = 1;
    tp1.state = Qt::TouchPointPressed;
    tp2.id = 2;
    tp2.state = Qt::TouchPointPressed;
    points << tp1 << tp2;
    QWindowSystemInterface::handleTouchEvent(&window, device, points);
    points[0].state = Qt::TouchPointReleased;
    points[1].state = Qt::TouchPointReleased;
    QWindowSystemInterface::handleTouchEvent(&window, device, points);
    QCoreApplication::processEvents();
    QCOMPARE(window.touchPressedCount, 2);
    QCOMPARE(window.touchReleasedCount, 2);
}

#include <tst_qwindow.moc>
QTEST_MAIN(tst_QWindow);
