/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <qevent.h>
#include <qwindow.h>

class MouseEventWidget : public QWindow
{
public:
    MouseEventWidget(QWindow *parent = 0) : QWindow(parent)
    {
    }
    bool mousePressEventRecieved;
    bool mouseReleaseEventRecieved;
    int mousePressButton;
    int mousePressButtons;
    int mousePressModifiers;
    int mouseReleaseButton;
    int mouseReleaseButtons;
    int mouseReleaseModifiers;
protected:
    void mousePressEvent(QMouseEvent *e)
    {
        QWindow::mousePressEvent(e);
        mousePressButton = e->button();
        mousePressButtons = e->buttons();
        mousePressModifiers = e->modifiers();
        mousePressEventRecieved = true;
        e->accept();
    }
    void mouseReleaseEvent(QMouseEvent *e)
    {
        QWindow::mouseReleaseEvent(e);
        mouseReleaseButton = e->button();
        mouseReleaseButtons = e->buttons();
        mouseReleaseModifiers = e->modifiers();
        mouseReleaseEventRecieved = true;
        e->accept();
    }
};

class tst_QMouseEvent : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
private slots:
    void mouseEventBasic();
    void checkMousePressEvent_data();
    void checkMousePressEvent();
    void checkMouseReleaseEvent_data();
    void checkMouseReleaseEvent();

private:
    MouseEventWidget* testMouseWidget;
};

void tst_QMouseEvent::initTestCase()
{
    testMouseWidget = new MouseEventWidget(0);
    testMouseWidget->show();
}

void tst_QMouseEvent::cleanupTestCase()
{
    delete testMouseWidget;
}

void tst_QMouseEvent::init()
{
    testMouseWidget->mousePressEventRecieved = false;
    testMouseWidget->mouseReleaseEventRecieved = false;
    testMouseWidget->mousePressButton = 0;
    testMouseWidget->mousePressButtons = 0;
    testMouseWidget->mousePressModifiers = 0;
    testMouseWidget->mouseReleaseButton = 0;
    testMouseWidget->mouseReleaseButtons = 0;
    testMouseWidget->mouseReleaseModifiers = 0;
}

void tst_QMouseEvent::mouseEventBasic()
{
    QPointF local(100, 100);
    QPointF scene(200, 200);
    QPointF screen(300, 300);
    QMouseEvent me(QEvent::MouseButtonPress, local, scene, screen, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCOMPARE(me.isAccepted(), true);
    QCOMPARE(me.button(), Qt::LeftButton);
    QCOMPARE(me.buttons(), Qt::LeftButton);
    QCOMPARE(me.localPos(), local);
    QCOMPARE(me.windowPos(), scene);
    QCOMPARE(me.screenPos(), screen);

    QPointF changedLocal(33, 66);
    me.setLocalPos(changedLocal);
    QCOMPARE(me.localPos(), changedLocal);
    QCOMPARE(me.windowPos(), scene);
    QCOMPARE(me.screenPos(), screen);
}

void tst_QMouseEvent::checkMousePressEvent_data()
{
    QTest::addColumn<int>("buttonPressed");
    QTest::addColumn<int>("keyPressed");

    QTest::newRow("leftButton-nokey") << int(Qt::LeftButton) << int(Qt::NoButton);
    QTest::newRow("leftButton-shiftkey") << int(Qt::LeftButton) << int(Qt::ShiftModifier);
    QTest::newRow("leftButton-controlkey") << int(Qt::LeftButton) << int(Qt::ControlModifier);
    QTest::newRow("leftButton-altkey") << int(Qt::LeftButton) << int(Qt::AltModifier);
    QTest::newRow("leftButton-metakey") << int(Qt::LeftButton) << int(Qt::MetaModifier);
    QTest::newRow("rightButton-nokey") << int(Qt::RightButton) << int(Qt::NoButton);
    QTest::newRow("rightButton-shiftkey") << int(Qt::RightButton) << int(Qt::ShiftModifier);
    QTest::newRow("rightButton-controlkey") << int(Qt::RightButton) << int(Qt::ControlModifier);
    QTest::newRow("rightButton-altkey") << int(Qt::RightButton) << int(Qt::AltModifier);
    QTest::newRow("rightButton-metakey") << int(Qt::RightButton) << int(Qt::MetaModifier);
    QTest::newRow("middleButton-nokey") << int(Qt::MiddleButton) << int(Qt::NoButton);
    QTest::newRow("middleButton-shiftkey") << int(Qt::MiddleButton) << int(Qt::ShiftModifier);
    QTest::newRow("middleButton-controlkey") << int(Qt::MiddleButton) << int(Qt::ControlModifier);
    QTest::newRow("middleButton-altkey") << int(Qt::MiddleButton) << int(Qt::AltModifier);
    QTest::newRow("middleButton-metakey") << int(Qt::MiddleButton) << int(Qt::MetaModifier);
}

void tst_QMouseEvent::checkMousePressEvent()
{
    QFETCH(int,buttonPressed);
    QFETCH(int,keyPressed);
    int button = buttonPressed;
    int buttons = button;
    int modifiers = keyPressed;

    QTest::mousePress(testMouseWidget, Qt::MouseButton(buttonPressed), Qt::KeyboardModifiers(keyPressed));
    qApp->processEvents();
    QVERIFY(testMouseWidget->mousePressEventRecieved);
    QCOMPARE(testMouseWidget->mousePressButton, button);
    QCOMPARE(testMouseWidget->mousePressButtons, buttons);
    QCOMPARE(testMouseWidget->mousePressModifiers, modifiers);

    QTest::mouseRelease(testMouseWidget, Qt::MouseButton(buttonPressed), Qt::KeyboardModifiers(keyPressed));
    qApp->processEvents();
}

void tst_QMouseEvent::checkMouseReleaseEvent_data()
{
    QTest::addColumn<int>("buttonReleased");
    QTest::addColumn<int>("keyPressed");

    QTest::newRow("leftButton-nokey") << int(Qt::LeftButton) << int(Qt::NoButton);
    QTest::newRow("leftButton-shiftkey") << int(Qt::LeftButton) << int(Qt::ShiftModifier);
    QTest::newRow("leftButton-controlkey") << int(Qt::LeftButton) << int(Qt::ControlModifier);
    QTest::newRow("leftButton-altkey") << int(Qt::LeftButton) << int(Qt::AltModifier);
    QTest::newRow("leftButton-metakey") << int(Qt::LeftButton) << int(Qt::MetaModifier);
    QTest::newRow("rightButton-nokey") << int(Qt::RightButton) << int(Qt::NoButton);
    QTest::newRow("rightButton-shiftkey") << int(Qt::RightButton) << int(Qt::ShiftModifier);
    QTest::newRow("rightButton-controlkey") << int(Qt::RightButton) << int(Qt::ControlModifier);
    QTest::newRow("rightButton-altkey") << int(Qt::RightButton) << int(Qt::AltModifier);
    QTest::newRow("rightButton-metakey") << int(Qt::RightButton) << int(Qt::MetaModifier);
    QTest::newRow("middleButton-nokey") << int(Qt::MiddleButton) << int(Qt::NoButton);
    QTest::newRow("middleButton-shiftkey") << int(Qt::MiddleButton) << int(Qt::ShiftModifier);
    QTest::newRow("middleButton-controlkey") << int(Qt::MiddleButton) << int(Qt::ControlModifier);
    QTest::newRow("middleButton-altkey") << int(Qt::MiddleButton) << int(Qt::AltModifier);
    QTest::newRow("middleButton-metakey") << int(Qt::MiddleButton) << int(Qt::MetaModifier);
}

void tst_QMouseEvent::checkMouseReleaseEvent()
{
    QFETCH(int,buttonReleased);
    QFETCH(int,keyPressed);
    int button = buttonReleased;
    int buttons = 0;
    int modifiers = keyPressed;

    QTest::mouseClick(testMouseWidget, Qt::MouseButton(buttonReleased), Qt::KeyboardModifiers(keyPressed));
    qApp->processEvents();
    QVERIFY(testMouseWidget->mouseReleaseEventRecieved);
    QCOMPARE(testMouseWidget->mouseReleaseButton, button);
    QCOMPARE(testMouseWidget->mouseReleaseButtons, buttons);
    QCOMPARE(testMouseWidget->mouseReleaseModifiers, modifiers);
}

QTEST_MAIN(tst_QMouseEvent)
#include "tst_qmouseevent.moc"
