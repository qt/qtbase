/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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

public:
    tst_QMouseEvent();
    virtual ~tst_QMouseEvent();


public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
    void checkMousePressEvent_data();
    void checkMousePressEvent();
    void checkMouseReleaseEvent_data();
    void checkMouseReleaseEvent();

private:
    MouseEventWidget* testMouseWidget;
};



tst_QMouseEvent::tst_QMouseEvent()
{
}

tst_QMouseEvent::~tst_QMouseEvent()
{

}

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

void tst_QMouseEvent::cleanup()
{
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
    QTest::newRow("midButton-nokey") << int(Qt::MidButton) << int(Qt::NoButton);
    QTest::newRow("midButton-shiftkey") << int(Qt::MidButton) << int(Qt::ShiftModifier);
    QTest::newRow("midButton-controlkey") << int(Qt::MidButton) << int(Qt::ControlModifier);
    QTest::newRow("midButton-altkey") << int(Qt::MidButton) << int(Qt::AltModifier);
    QTest::newRow("midButton-metakey") << int(Qt::MidButton) << int(Qt::MetaModifier);
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
    QTest::newRow("midButton-nokey") << int(Qt::MidButton) << int(Qt::NoButton);
    QTest::newRow("midButton-shiftkey") << int(Qt::MidButton) << int(Qt::ShiftModifier);
    QTest::newRow("midButton-controlkey") << int(Qt::MidButton) << int(Qt::ControlModifier);
    QTest::newRow("midButton-altkey") << int(Qt::MidButton) << int(Qt::AltModifier);
    QTest::newRow("midButton-metakey") << int(Qt::MidButton) << int(Qt::MetaModifier);
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
