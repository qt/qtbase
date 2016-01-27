/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qtoolbutton.h>
#include <qmenu.h>
#include <qaction.h>
#include <qstyleoption.h>
#include <qscreen.h>
#include <qlabel.h>

class tst_QToolButton : public QObject
{
Q_OBJECT

public:
    tst_QToolButton();
    virtual ~tst_QToolButton();

private slots:
    void getSetCheck();
    void triggered();
    void collapseTextOnPriority();
    void task230994_iconSize();
    void task176137_autoRepeatOfAction();
    void qtbug_26956_popupTimerDone();
    void qtbug_34759_sizeHintResetWhenSettingMenu();

protected slots:
    void sendMouseClick();
private:
    QPointer<QWidget> m_menu;
};

tst_QToolButton::tst_QToolButton()
{
}

tst_QToolButton::~tst_QToolButton()
{
}

// Testing get/set functions
void tst_QToolButton::getSetCheck()
{
    QToolButton obj1;
    // QMenu* QToolButton::menu()
    // void QToolButton::setMenu(QMenu*)
    QMenu *var1 = new QMenu;
    obj1.setMenu(var1);
    QCOMPARE(var1, obj1.menu());
    obj1.setMenu((QMenu *)0);
    QCOMPARE((QMenu *)0, obj1.menu());
    delete var1;

    // ToolButtonPopupMode QToolButton::popupMode()
    // void QToolButton::setPopupMode(ToolButtonPopupMode)
    obj1.setPopupMode(QToolButton::ToolButtonPopupMode(QToolButton::DelayedPopup));
    QCOMPARE(QToolButton::ToolButtonPopupMode(QToolButton::DelayedPopup), obj1.popupMode());
    obj1.setPopupMode(QToolButton::ToolButtonPopupMode(QToolButton::MenuButtonPopup));
    QCOMPARE(QToolButton::ToolButtonPopupMode(QToolButton::MenuButtonPopup), obj1.popupMode());
    obj1.setPopupMode(QToolButton::ToolButtonPopupMode(QToolButton::InstantPopup));
    QCOMPARE(QToolButton::ToolButtonPopupMode(QToolButton::InstantPopup), obj1.popupMode());

    // bool QToolButton::autoRaise()
    // void QToolButton::setAutoRaise(bool)
    obj1.setAutoRaise(false);
    QCOMPARE(false, obj1.autoRaise());
    obj1.setAutoRaise(true);
    QCOMPARE(true, obj1.autoRaise());

    // QAction * QToolButton::defaultAction()
    // void QToolButton::setDefaultAction(QAction *)
    QAction *var4 = new QAction(0);
    obj1.setDefaultAction(var4);
    QCOMPARE(var4, obj1.defaultAction());
    obj1.setDefaultAction((QAction *)0);
    QCOMPARE((QAction *)0, obj1.defaultAction());
    delete var4;
}

void tst_QToolButton::triggered()
{
    qRegisterMetaType<QAction *>("QAction *");
    QWidget mainWidget;
    mainWidget.setWindowTitle(QStringLiteral("triggered"));
    mainWidget.resize(200, 200);
    mainWidget.move(QGuiApplication::primaryScreen()->availableGeometry().center() - QPoint(100, 100));
    QToolButton *toolButton = new QToolButton(&mainWidget);
    QSignalSpy spy(toolButton,SIGNAL(triggered(QAction*)));
    QScopedPointer<QMenu> menu(new QMenu(QStringLiteral("Menu")));
    QAction *one = menu->addAction("one");
    menu->addAction("two");
    QAction *defaultAction = new QAction(QStringLiteral("def"), this);

    toolButton->setMenu(menu.data());
    toolButton->setDefaultAction(defaultAction);

    mainWidget.show();
    QApplication::setActiveWindow(&mainWidget);
    QVERIFY(QTest::qWaitForWindowActive(&mainWidget));

    defaultAction->trigger();
    QCOMPARE(spy.count(),1);
    QCOMPARE(qvariant_cast<QAction *>(spy.at(0).at(0)), defaultAction);

    m_menu = menu.data();

    QTimer *timer = new QTimer(this);
    timer->setInterval(50);
    connect(timer, SIGNAL(timeout()), this, SLOT(sendMouseClick()));
    timer->start();
    QTimer::singleShot(10000, &mainWidget, SLOT(close())); // Emergency bail-out
    toolButton->showMenu();
    QTest::qWait(20);
    QCOMPARE(spy.count(),2);
    QCOMPARE(qvariant_cast<QAction *>(spy.at(1).at(0)), one);
}

void tst_QToolButton::collapseTextOnPriority()
{
    class MyToolButton : public QToolButton
    {
        friend class tst_QToolButton;
    public:
        void initStyleOption(QStyleOptionToolButton *option)
        {
            QToolButton::initStyleOption(option);
        }
    };

    MyToolButton button;
    button.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    QAction action(button.style()->standardIcon(QStyle::SP_ArrowBack), "test", 0);
    button.setDefaultAction(&action);

    QStyleOptionToolButton option;
    button.initStyleOption(&option);
    QCOMPARE(option.toolButtonStyle, Qt::ToolButtonTextBesideIcon);
    action.setPriority(QAction::LowPriority);
    button.initStyleOption(&option);
    QCOMPARE(option.toolButtonStyle, Qt::ToolButtonIconOnly);
}


void tst_QToolButton::task230994_iconSize()
{
    //we check that the iconsize returned bu initStyleOption is valid
    //when the toolbutton has no parent
    class MyToolButton : public QToolButton
    {
        friend class tst_QToolButton;
    public:
        void initStyleOption(QStyleOptionToolButton *option)
        {
            QToolButton::initStyleOption(option);
        }
    };

    MyToolButton button;
    QStyleOptionToolButton option;
    button.initStyleOption(&option);
    QVERIFY(option.iconSize.isValid());
}

void tst_QToolButton::task176137_autoRepeatOfAction()
{
    QAction action(0);
    QWidget mainWidget;
    mainWidget.setWindowTitle(QStringLiteral("task176137_autoRepeatOfAction"));
    mainWidget.resize(200, 200);
    mainWidget.move(QGuiApplication::primaryScreen()->availableGeometry().center() - QPoint(100, 100));
    QToolButton *toolButton = new QToolButton(&mainWidget);
    toolButton->setDefaultAction (&action);
    QLabel *label = new QLabel(QStringLiteral("This test takes a while."), &mainWidget);
    label->move(0, 50);

    mainWidget.show();
    QApplication::setActiveWindow(&mainWidget);
    QVERIFY(QTest::qWaitForWindowActive(&mainWidget));

    QSignalSpy spy(&action,SIGNAL(triggered()));
    QTest::mousePress (toolButton, Qt::LeftButton);
    QTest::mouseRelease (toolButton, Qt::LeftButton, 0, QPoint (), 2000);
    QCOMPARE(spy.count(),1);

    // try again with auto repeat
    toolButton->setAutoRepeat (true);
    QSignalSpy repeatSpy(&action,SIGNAL(triggered())); // new spy
    QTest::mousePress (toolButton, Qt::LeftButton);
    QTest::mouseRelease (toolButton, Qt::LeftButton, 0, QPoint (), 3000);
    const qreal expected = (3000 - toolButton->autoRepeatDelay()) / toolButton->autoRepeatInterval() + 1;
    //we check that the difference is small (on some systems timers are not super accurate)
    qreal diff = (expected - repeatSpy.count()) / expected;
    QVERIFY2(qAbs(diff) < 0.2, qPrintable(
        QString("expected: %1, actual: %2, diff (fraction): %3")
            .arg(expected)
            .arg(repeatSpy.count())
            .arg(diff)));
}


void tst_QToolButton::sendMouseClick()
{
    if (m_menu.isNull()) {
        qWarning("m_menu is NULL");
        return;
    }
    if (!m_menu->isVisible())
        return;
    QTest::mouseClick(m_menu.data(), Qt::LeftButton, 0, QPoint(7, 7));
    if (QTimer *timer = qobject_cast<QTimer *>(sender())) {
        timer->stop();
        timer->deleteLater();
    }
}

void tst_QToolButton::qtbug_26956_popupTimerDone()
{
    QToolButton *tb = new QToolButton;
    tb->setMenu(new QMenu(tb));
    tb->menu()->addAction("Qt");
    tb->deleteLater();
    tb->showMenu();
}

void tst_QToolButton::qtbug_34759_sizeHintResetWhenSettingMenu()
{
    // There is no reliable way of checking what's ultimately a style-dependent
    // sizing. So the idea is checking if the size is the "correct" size w.r.t.
    // another toolbutton which has had a menu set before it was shown for the first time

    QToolButton button1;
    QToolButton button2;

    button1.setToolButtonStyle(Qt::ToolButtonIconOnly);
    button1.setPopupMode(QToolButton::MenuButtonPopup);

    button2.setToolButtonStyle(Qt::ToolButtonIconOnly);
    button2.setPopupMode(QToolButton::MenuButtonPopup);

    button2.setMenu(new QMenu(&button2));

    button1.show();
    button2.show();

    QVERIFY(QTest::qWaitForWindowExposed(&button1));
    QVERIFY(QTest::qWaitForWindowExposed(&button2));

    button1.setMenu(new QMenu(&button1));
    QTRY_COMPARE(button1.sizeHint(), button2.sizeHint());
}

QTEST_MAIN(tst_QToolButton)
#include "tst_qtoolbutton.moc"
