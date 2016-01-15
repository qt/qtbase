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

#include <qapplication.h>
#include <qevent.h>
#include <qaction.h>
#include <qmenu.h>
#include <qpa/qplatformtheme.h>
#include <private/qguiapplication_p.h>

class tst_QAction : public QObject
{
    Q_OBJECT

public:
    tst_QAction();
    virtual ~tst_QAction();


    void updateState(QActionEvent *e);

public slots:
    void initTestCase();
    void cleanupTestCase();
private slots:
    void getSetCheck();
    void setText_data();
    void setText();
    void setIconText_data() { setText_data(); }
    void setIconText();
    void setUnknownFont();
    void actionEvent();
    void setStandardKeys();
    void alternateShortcuts();
    void enabledVisibleInteraction();
    void task200823_tooltip();
    void task229128TriggeredSignalWithoutActiongroup();
    void task229128TriggeredSignalWhenInActiongroup();
    void repeat();

private:
    int m_lastEventType;
    int m_keyboardScheme;
    QAction *m_lastAction;
    QWidget *m_tstWidget;
};

// Testing get/set functions
void tst_QAction::getSetCheck()
{
    QAction obj1(0);
    // QActionGroup * QAction::actionGroup()
    // void QAction::setActionGroup(QActionGroup *)
    QActionGroup *var1 = new QActionGroup(0);
    obj1.setActionGroup(var1);
    QCOMPARE(var1, obj1.actionGroup());
    obj1.setActionGroup((QActionGroup *)0);
    QCOMPARE((QActionGroup *)0, obj1.actionGroup());
    delete var1;

    // QMenu * QAction::menu()
    // void QAction::setMenu(QMenu *)
    QMenu *var2 = new QMenu(0);
    obj1.setMenu(var2);
    QCOMPARE(var2, obj1.menu());
    obj1.setMenu((QMenu *)0);
    QCOMPARE((QMenu *)0, obj1.menu());
    delete var2;

    QCOMPARE(obj1.priority(), QAction::NormalPriority);
    obj1.setPriority(QAction::LowPriority);
    QCOMPARE(obj1.priority(), QAction::LowPriority);
}

class MyWidget : public QWidget
{
    Q_OBJECT
public:
    MyWidget(tst_QAction *tst, QWidget *parent = 0) : QWidget(parent) { this->tst = tst; }

protected:
    virtual void actionEvent(QActionEvent *e) { tst->updateState(e); }

private:
    tst_QAction *tst;
};

tst_QAction::tst_QAction() : m_keyboardScheme(QPlatformTheme::WindowsKeyboardScheme)
{
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme())
        m_keyboardScheme = theme->themeHint(QPlatformTheme::KeyboardScheme).toInt();
}

tst_QAction::~tst_QAction()
{

}

void tst_QAction::initTestCase()
{
    m_lastEventType = 0;
    m_lastAction = 0;

    MyWidget *mw = new MyWidget(this);
    m_tstWidget = mw;
    mw->show();
    qApp->setActiveWindow(mw);
}

void tst_QAction::cleanupTestCase()
{
    QWidget *testWidget = m_tstWidget;
    if (testWidget) {
        testWidget->hide();
        delete testWidget;
    }
}

void tst_QAction::setText_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("iconText");
    QTest::addColumn<QString>("textFromIconText");

    //next we fill it with data
    QTest::newRow("Normal") << "Action" << "Action" << "Action";
    QTest::newRow("Ampersand") << "Search && Destroy" << "Search & Destroy" << "Search && Destroy";
    QTest::newRow("Mnemonic and ellipsis") << "O&pen File ..." << "Open File" << "Open File";
}

void tst_QAction::setText()
{
    QFETCH(QString, text);

    QAction action(0);
    action.setText(text);

    QCOMPARE(action.text(), text);

    QFETCH(QString, iconText);
    QCOMPARE(action.iconText(), iconText);
}

void tst_QAction::setIconText()
{
    QFETCH(QString, iconText);

    QAction action(0);
    action.setIconText(iconText);
    QCOMPARE(action.iconText(), iconText);

    QFETCH(QString, textFromIconText);
    QCOMPARE(action.text(), textFromIconText);
}

void tst_QAction::setUnknownFont() // QTBUG-42728
{
    QAction action(0);
    QFont font("DoesNotExist", 11);
    action.setFont(font);

    QMenu menu;
    menu.addAction(&action); // should not crash
}

void tst_QAction::updateState(QActionEvent *e)
{
    if (!e) {
        m_lastEventType = 0;
        m_lastAction = 0;
    } else {
        m_lastEventType = (int)e->type();
        m_lastAction = e->action();
    }
}

void tst_QAction::actionEvent()
{
    QAction a(0);
    a.setText("action text");

    // add action
    m_tstWidget->addAction(&a);
    qApp->processEvents();

    QCOMPARE(m_lastEventType, (int)QEvent::ActionAdded);
    QCOMPARE(m_lastAction, &a);

    // change action
    a.setText("new action text");
    qApp->processEvents();

    QCOMPARE(m_lastEventType, (int)QEvent::ActionChanged);
    QCOMPARE(m_lastAction, &a);

    // remove action
    m_tstWidget->removeAction(&a);
    qApp->processEvents();

    QCOMPARE(m_lastEventType, (int)QEvent::ActionRemoved);
    QCOMPARE(m_lastAction, &a);
}

//basic testing of standard keys
void tst_QAction::setStandardKeys()
{
    QAction act(0);
    act.setShortcut(QKeySequence("CTRL+L"));
    QList<QKeySequence> list;
    act.setShortcuts(list);
    act.setShortcuts(QKeySequence::Copy);
    QCOMPARE(act.shortcut(), act.shortcuts().first());

    QList<QKeySequence> expected;
    const QKeySequence ctrlC = QKeySequence(QStringLiteral("CTRL+C"));
    const QKeySequence ctrlInsert = QKeySequence(QStringLiteral("CTRL+INSERT"));
    switch (m_keyboardScheme) {
    case QPlatformTheme::MacKeyboardScheme:
        expected  << ctrlC;
        break;
    case QPlatformTheme::WindowsKeyboardScheme:
        expected  << ctrlC << ctrlInsert;
        break;
    default: // X11
        expected  << ctrlC << QKeySequence(QStringLiteral("F16")) << ctrlInsert;
        break;
    }

    QCOMPARE(act.shortcuts(), expected);
}


void tst_QAction::alternateShortcuts()
{
    //test the alternate shortcuts (by adding more than 1 shortcut)

    QWidget *wid = m_tstWidget;

    {
        QAction act(wid);
        wid->addAction(&act);
        QList<QKeySequence> shlist = QList<QKeySequence>() << QKeySequence("CTRL+P") << QKeySequence("CTRL+A");
        act.setShortcuts(shlist);

        QSignalSpy spy(&act, SIGNAL(triggered()));

        act.setAutoRepeat(true);
        QTest::keyClick(wid, Qt::Key_A, Qt::ControlModifier);
        QCOMPARE(spy.count(), 1); //act should have been triggered

        act.setAutoRepeat(false);
        QTest::keyClick(wid, Qt::Key_A, Qt::ControlModifier);
        QCOMPARE(spy.count(), 2); //act should have been triggered a 2nd time

        //end of the scope of the action, it will be destroyed and removed from wid
        //This action should also unregister its shortcuts
    }


    //this tests a crash (if the action did not unregister its alternate shortcuts)
    QTest::keyClick(wid, Qt::Key_A, Qt::ControlModifier);
}

void tst_QAction::enabledVisibleInteraction()
{
    QAction act(0);
    // check defaults
    QVERIFY(act.isEnabled());
    QVERIFY(act.isVisible());

    // !visible => !enabled
    act.setVisible(false);
    QVERIFY(!act.isEnabled());
    act.setVisible(true);
    QVERIFY(act.isEnabled());
    act.setEnabled(false);
    QVERIFY(act.isVisible());

    // check if shortcut is disabled if not visible
    m_tstWidget->addAction(&act);
    act.setShortcut(QKeySequence("Ctrl+T"));
    QSignalSpy spy(&act, SIGNAL(triggered()));
    act.setEnabled(true);
    act.setVisible(false);
    QTest::keyClick(m_tstWidget, Qt::Key_T, Qt::ControlModifier);
    QCOMPARE(spy.count(), 0); //act is not visible, so don't trigger
    act.setVisible(false);
    act.setEnabled(true);
    QTest::keyClick(m_tstWidget, Qt::Key_T, Qt::ControlModifier);
    QCOMPARE(spy.count(), 0); //act is not visible, so don't trigger
    act.setVisible(true);
    act.setEnabled(true);
    QTest::keyClick(m_tstWidget, Qt::Key_T, Qt::ControlModifier);
    QCOMPARE(spy.count(), 1); //act is visible and enabled, so trigger
}

void tst_QAction::task200823_tooltip()
{
    QAction *action = new QAction("foo", 0);
    QString shortcut("ctrl+o");
    action->setShortcut(shortcut);

    // we want a non-standard tooltip that shows the shortcut
    action->setToolTip(action->text() + QLatin1String(" (") + action->shortcut().toString() + QLatin1Char(')'));

    QString ref = QLatin1String("foo (") + QKeySequence(shortcut).toString() + QLatin1Char(')');
    QCOMPARE(action->toolTip(), ref);
}

void tst_QAction::task229128TriggeredSignalWithoutActiongroup()
{
    // test without a group
    QAction *actionWithoutGroup = new QAction("Test", qApp);
    QSignalSpy spyWithoutGroup(actionWithoutGroup, SIGNAL(triggered(bool)));
    QCOMPARE(spyWithoutGroup.count(), 0);
    actionWithoutGroup->trigger();
    // signal should be emitted
    QCOMPARE(spyWithoutGroup.count(), 1);

    // it is now a checkable checked action
    actionWithoutGroup->setCheckable(true);
    actionWithoutGroup->setChecked(true);
    spyWithoutGroup.clear();
    QCOMPARE(spyWithoutGroup.count(), 0);
    actionWithoutGroup->trigger();
    // signal should be emitted
    QCOMPARE(spyWithoutGroup.count(), 1);
}

void tst_QAction::task229128TriggeredSignalWhenInActiongroup()
{
    QActionGroup ag(0);
    QAction *action = new QAction("Test", &ag);
    QAction *checkedAction = new QAction("Test 2", &ag);
    ag.addAction(action);
    action->setCheckable(true);
    ag.addAction(checkedAction);
    checkedAction->setCheckable(true);
    checkedAction->setChecked(true);

    QSignalSpy actionSpy(checkedAction, SIGNAL(triggered(bool)));
    QSignalSpy actionGroupSpy(&ag, SIGNAL(triggered(QAction*)));
    QCOMPARE(actionGroupSpy.count(), 0);
    QCOMPARE(actionSpy.count(), 0);
    checkedAction->trigger();
    // check that both the group and the action have emitted the signal
    QCOMPARE(actionGroupSpy.count(), 1);
    QCOMPARE(actionSpy.count(), 1);
}

void tst_QAction::repeat()
{
    QWidget *wid = m_tstWidget;
    QAction act(wid);
    wid->addAction(&act);
    act.setShortcut(QKeySequence(Qt::Key_F));
    QSignalSpy spy(&act, SIGNAL(triggered()));

    act.setAutoRepeat(true);
    QTest::keyPress(wid, Qt::Key_F);
    QTest::keyRelease(wid, Qt::Key_F);
    QCOMPARE(spy.count(), 1);

    spy.clear();
    QTest::keyPress(wid, Qt::Key_F);
    // repeat event
    QTest::simulateEvent(wid, true, Qt::Key_F, Qt::NoModifier, QString("f"), true);
    QTest::simulateEvent(wid, true, Qt::Key_F, Qt::NoModifier, QString("f"), true);
    QTest::keyRelease(wid, Qt::Key_F);
    QCOMPARE(spy.count(), 3);

    spy.clear();
    act.setAutoRepeat(false);
    QTest::keyPress(wid, Qt::Key_F);
    QTest::simulateEvent(wid, true, Qt::Key_F, Qt::NoModifier, QString("f"), true);
    QTest::simulateEvent(wid, true, Qt::Key_F, Qt::NoModifier, QString("f"), true);
    QTest::keyRelease(wid, Qt::Key_F);
    QCOMPARE(spy.count(), 1);

    spy.clear();
    act.setAutoRepeat(true);
    QTest::keyPress(wid, Qt::Key_F);
    QTest::simulateEvent(wid, true, Qt::Key_F, Qt::NoModifier, QString("f"), true);
    QTest::keyRelease(wid, Qt::Key_F);
    QCOMPARE(spy.count(), 2);
}

QTEST_MAIN(tst_QAction)
#include "tst_qaction.moc"
