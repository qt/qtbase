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

#include <QDialog>
#include <QMainWindow>
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

    void updateState(QActionEvent *e);

private slots:
    void init();
    void cleanup();
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
    void setData();
    void keysequence(); // QTBUG-53381
    void disableShortcutsWithBlockedWidgets_data();
    void disableShortcutsWithBlockedWidgets();
    void shortcutFromKeyEvent(); // QTBUG-48325

private:
    int m_lastEventType;
    const int m_keyboardScheme;
    QAction *m_lastAction;
};

tst_QAction::tst_QAction()
    : m_keyboardScheme(QGuiApplicationPrivate::platformTheme()->themeHint(QPlatformTheme::KeyboardScheme).toInt())
{
}

void tst_QAction::init()
{
    m_lastEventType = 0;
    m_lastAction = nullptr;
}

void tst_QAction::cleanup()
{
    QVERIFY(QApplication::topLevelWidgets().isEmpty());
}

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
    explicit MyWidget(tst_QAction *tst, QWidget *parent = nullptr) : QWidget(parent), m_test(tst)
    { setWindowTitle(QTest::currentTestFunction()); }

protected:
    void actionEvent(QActionEvent *e) override { m_test->updateState(e); }

private:
    tst_QAction *m_test;
};

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
    MyWidget testWidget(this);
    testWidget.show();
    QApplication::setActiveWindow(&testWidget);
    testWidget.addAction(&a);
    qApp->processEvents();

    QCOMPARE(m_lastEventType, (int)QEvent::ActionAdded);
    QCOMPARE(m_lastAction, &a);

    // change action
    a.setText("new action text");
    qApp->processEvents();

    QCOMPARE(m_lastEventType, (int)QEvent::ActionChanged);
    QCOMPARE(m_lastAction, &a);

    // remove action
    testWidget.removeAction(&a);
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
        expected  << ctrlC << ctrlInsert << QKeySequence(QStringLiteral("F16"));
        break;
    }

    QCOMPARE(act.shortcuts(), expected);
}


void tst_QAction::alternateShortcuts()
{
    //test the alternate shortcuts (by adding more than 1 shortcut)

    MyWidget testWidget(this);
    testWidget.show();
    QApplication::setActiveWindow(&testWidget);

    {
        QAction act(&testWidget);
        testWidget.addAction(&act);
        QList<QKeySequence> shlist = QList<QKeySequence>() << QKeySequence("CTRL+P") << QKeySequence("CTRL+A");
        act.setShortcuts(shlist);

        QSignalSpy spy(&act, SIGNAL(triggered()));

        act.setAutoRepeat(true);
        QTest::keyClick(&testWidget, Qt::Key_A, Qt::ControlModifier);
        QCOMPARE(spy.count(), 1); //act should have been triggered

        act.setAutoRepeat(false);
        QTest::keyClick(&testWidget, Qt::Key_A, Qt::ControlModifier);
        QCOMPARE(spy.count(), 2); //act should have been triggered a 2nd time

        //end of the scope of the action, it will be destroyed and removed from wid
        //This action should also unregister its shortcuts
    }


    //this tests a crash (if the action did not unregister its alternate shortcuts)
    QTest::keyClick(&testWidget, Qt::Key_A, Qt::ControlModifier);
}

void tst_QAction::keysequence()
{
    MyWidget testWidget(this);
    testWidget.show();
    QApplication::setActiveWindow(&testWidget);

    {
        QAction act(&testWidget);
        testWidget.addAction(&act);

        QKeySequence ks(QKeySequence::SelectAll);

        act.setShortcut(ks);

        QSignalSpy spy(&act, &QAction::triggered);

        act.setAutoRepeat(true);
        QTest::keySequence(&testWidget, ks);
        QCoreApplication::processEvents();
        QCOMPARE(spy.count(), 1); // act should have been triggered

        act.setAutoRepeat(false);
        QTest::keySequence(&testWidget, ks);
        QCoreApplication::processEvents();
        QCOMPARE(spy.count(), 2); //act should have been triggered a 2nd time

        // end of the scope of the action, it will be destroyed and removed from widget
        // This action should also unregister its shortcuts
    }

    // this tests a crash (if the action did not unregister its alternate shortcuts)
    QTest::keyClick(&testWidget, Qt::Key_A, Qt::ControlModifier);
}

void tst_QAction::enabledVisibleInteraction()
{
    MyWidget testWidget(this);
    testWidget.show();
    QApplication::setActiveWindow(&testWidget);

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
    testWidget.addAction(&act);
    act.setShortcut(QKeySequence("Ctrl+T"));
    QSignalSpy spy(&act, SIGNAL(triggered()));
    act.setEnabled(true);
    act.setVisible(false);
    QTest::keyClick(&testWidget, Qt::Key_T, Qt::ControlModifier);
    QCOMPARE(spy.count(), 0); //act is not visible, so don't trigger
    act.setVisible(false);
    act.setEnabled(true);
    QTest::keyClick(&testWidget, Qt::Key_T, Qt::ControlModifier);
    QCOMPARE(spy.count(), 0); //act is not visible, so don't trigger
    act.setVisible(true);
    act.setEnabled(true);
    QTest::keyClick(&testWidget, Qt::Key_T, Qt::ControlModifier);
    QCOMPARE(spy.count(), 1); //act is visible and enabled, so trigger
}

void tst_QAction::task200823_tooltip()
{
    const QScopedPointer<QAction> action(new QAction("foo", nullptr));
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
    const QScopedPointer<QAction> actionWithoutGroup(new QAction("Test", nullptr));
    QSignalSpy spyWithoutGroup(actionWithoutGroup.data(), SIGNAL(triggered(bool)));
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
    MyWidget testWidget(this);
    testWidget.show();
    QApplication::setActiveWindow(&testWidget);
    QVERIFY(QTest::qWaitForWindowActive(&testWidget));

    QAction act(&testWidget);
    testWidget.addAction(&act);
    act.setShortcut(QKeySequence(Qt::Key_F));
    QSignalSpy spy(&act, SIGNAL(triggered()));

    act.setAutoRepeat(true);
    QTest::keyPress(&testWidget, Qt::Key_F);
    QTest::keyRelease(&testWidget, Qt::Key_F);
    QCOMPARE(spy.count(), 1);

    spy.clear();
    QTest::keyPress(&testWidget, Qt::Key_F);
    // repeat event
    QTest::simulateEvent(&testWidget, true, Qt::Key_F, Qt::NoModifier, QString("f"), true);
    QTest::simulateEvent(&testWidget, true, Qt::Key_F, Qt::NoModifier, QString("f"), true);
    QTest::keyRelease(&testWidget, Qt::Key_F);
    QCOMPARE(spy.count(), 3);

    spy.clear();
    act.setAutoRepeat(false);
    QTest::keyPress(&testWidget, Qt::Key_F);
    QTest::simulateEvent(&testWidget, true, Qt::Key_F, Qt::NoModifier, QString("f"), true);
    QTest::simulateEvent(&testWidget, true, Qt::Key_F, Qt::NoModifier, QString("f"), true);
    QTest::keyRelease(&testWidget, Qt::Key_F);
    QCOMPARE(spy.count(), 1);

    spy.clear();
    act.setAutoRepeat(true);
    QTest::keyPress(&testWidget, Qt::Key_F);
    QTest::simulateEvent(&testWidget, true, Qt::Key_F, Qt::NoModifier, QString("f"), true);
    QTest::keyRelease(&testWidget, Qt::Key_F);
    QCOMPARE(spy.count(), 2);
}

void tst_QAction::setData() // QTBUG-62006
{
    QAction act(nullptr);
    QSignalSpy spy(&act, &QAction::changed);
    QCOMPARE(act.data(), QVariant());
    QCOMPARE(spy.count(), 0);
    act.setData(QVariant());
    QCOMPARE(spy.count(), 0);

    act.setData(-1);
    QCOMPARE(spy.count(), 1);
    act.setData(-1);
    QCOMPARE(spy.count(), 1);
}

void tst_QAction::disableShortcutsWithBlockedWidgets_data()
{
    QTest::addColumn<Qt::ShortcutContext>("shortcutContext");
    QTest::addColumn<Qt::WindowModality>("windowModality");

    QTest::newRow("application modal dialog should block window shortcut.")
        << Qt::WindowShortcut << Qt::ApplicationModal;

    QTest::newRow("application modal dialog should block application shortcut.")
        << Qt::ApplicationShortcut << Qt::ApplicationModal;

    QTest::newRow("window modal dialog should block application shortcut.")
        << Qt::ApplicationShortcut << Qt::WindowModal;

    QTest::newRow("window modal dialog should block window shortcut.")
        << Qt::WindowShortcut << Qt::WindowModal;
}


void tst_QAction::disableShortcutsWithBlockedWidgets()
{
    QMainWindow window;

    QFETCH(Qt::ShortcutContext, shortcutContext);
    QAction action(&window);
    window.addAction(&action);
    action.setShortcut(QKeySequence(Qt::Key_1));
    action.setShortcutContext(shortcutContext);

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QDialog dialog(&window);
    QFETCH(Qt::WindowModality, windowModality);
    dialog.setWindowModality(windowModality);

    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));

    QApplication::setActiveWindow(&window);
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QSignalSpy spy(&action, &QAction::triggered);
    QTest::keyPress(&window, Qt::Key_1);
    QCOMPARE(spy.count(), 0);
}

class ShortcutOverrideWidget : public QWidget
{
public:
    ShortcutOverrideWidget(QWidget *parent = 0) : QWidget(parent), shortcutOverrideCount(0) {}
    int shortcutOverrideCount;
protected:
    bool event(QEvent *e)
    {
        if (e->type() == QEvent::ShortcutOverride)
            ++shortcutOverrideCount;
        return QWidget::event(e);
    }
};

// Test that a key press event sent with sendEvent() still gets handled as a possible
// ShortcutOverride event first before passing it on as a normal KeyEvent.
void tst_QAction::shortcutFromKeyEvent()
{
    ShortcutOverrideWidget testWidget;
    QAction action;
    action.setShortcut(Qt::Key_1);
    testWidget.addAction(&action);
    testWidget.show();
    QSignalSpy spy(&action, &QAction::triggered);
    QVERIFY(spy.isValid());
    QVERIFY(QTest::qWaitForWindowActive(&testWidget));
    QCOMPARE(testWidget.shortcutOverrideCount, 0);

    // Don't use the QTest::keyPress approach as this will take the
    // shortcut route for us
    QKeyEvent e(QEvent::KeyPress, Qt::Key_1, Qt::NoModifier);
    QApplication::sendEvent(&testWidget, &e);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(testWidget.shortcutOverrideCount, 1);
}

QTEST_MAIN(tst_QAction)
#include "tst_qaction.moc"
