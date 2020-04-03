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
#include <qmainwindow.h>
#include <qmenubar.h>
#include <qstyle.h>
#include <qproxystyle.h>
#include <qstylefactory.h>
#include <qaction.h>
#include <qstyleoption.h>
#include <QVBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <qscreen.h>

#include <qobject.h>

QT_FORWARD_DECLARE_CLASS(QMainWindow)

#include <qmenubar.h>

#include <QtTest/private/qtesthelpers_p.h>

using namespace QTestPrivate;

// Helper to calculate the action position in window coordinates
static inline QPoint widgetToWindowPos(const QWidget *w, const QPoint &pos)
{
    const QWindow *window = w->window()->windowHandle();
    Q_ASSERT(window);
    return window->mapFromGlobal(w->mapToGlobal(pos));
}

static QPoint menuBarActionWindowPos(const QMenuBar *mb, QAction *a)
{
    return widgetToWindowPos(mb, mb->actionGeometry(a).center());
}

class Menu : public QMenu
{
    Q_OBJECT
        public slots:
            void addActions()
            {
                //this will change the geometry of the menu
                addAction("action1");
                addAction("action2");
            }
};

struct TestMenu
{
    QList<QMenu *> menus;
    QList<QAction *> actions;
};

class tst_QMenuBar : public QObject
{
    Q_OBJECT
public:
    tst_QMenuBar();

private slots:
    void getSetCheck();
    void cleanup();

    void clear();
    void removeItemAt();
    void removeItemAt_data();
    void removeItem_data();
    void removeItem();
    void count();
    void insertItem_QString_QObject();

#if !defined(Q_OS_DARWIN)
    void accel();
    void activatedCount();
    void activatedCount_data();

    void check_accelKeys();
    void check_cursorKeys1();
    void check_cursorKeys2();
    void check_cursorKeys3();

    void check_escKey();
#endif
    void allowActiveAndDisabled();
    void taskQTBUG56860_focus();
    void check_endKey();
    void check_homeKey();

//     void check_mouse1_data();
//     void check_mouse1();
//     void check_mouse2_data();
//     void check_mouse2();

    void check_altPress();
    void check_altClosePress();
#if !defined(Q_OS_DARWIN)
    void check_shortcutPress();
    void check_menuPosition();
    void taskQTBUG46812_doNotLeaveMenubarHighlighted();
#endif
    void task223138_triggered();
    void task256322_highlight();
    void menubarSizeHint();
#ifndef Q_OS_MACOS
    void taskQTBUG4965_escapeEaten();
#endif
    void taskQTBUG11823_crashwithInvisibleActions();
    void closeOnSecondClickAndOpenOnThirdClick();
    void cornerWidgets_data();
    void cornerWidgets();
    void taskQTBUG53205_crashReparentNested();
#ifdef Q_OS_MACOS
    void taskQTBUG56275_reinsertMenuInParentlessQMenuBar();
    void QTBUG_57404_existingMenuItemException();
#endif
    void QTBUG_25669_menubarActionDoubleTriggered();
    void taskQTBUG55966_subMenuRemoved();
    void QTBUG_58344_invalidIcon();
    void platformMenu();
    void addActionQt5connect();
    void QTBUG_65488_hiddenActionTriggered();
protected slots:
    void onSimpleActivated( QAction*);
    void onComplexActionTriggered();
    void slotForTaskQTBUG53205();

private:
    TestMenu initSimpleMenuBar(QMenuBar *mb, bool forceNonNative = true);
    TestMenu initWindowWithSimpleMenuBar(QMainWindow &w, bool forceNonNative = true);
    QAction *createCharacterAction(QMenu *menu, char lowerAscii);
    QMenu *addNumberedMenu(QMenuBar *mb, int n);
    TestMenu initComplexMenuBar(QMenuBar *mb);
    TestMenu initWindowWithComplexMenuBar(QMainWindow &w);

    QAction* m_lastSimpleAcceleratorId;
    int m_simpleActivatedCount;
    int m_complexTriggerCount[int('k')];
    QMenuBar* taskQTBUG53205MenuBar;
};

// Testing get/set functions
void tst_QMenuBar::getSetCheck()
{
    QMenuBar obj1;
    // QAction * QMenuBar::activeAction()
    // void QMenuBar::setActiveAction(QAction *)
    QAction *var1 = new QAction(0);
    obj1.setActiveAction(var1);
    QCOMPARE(var1, obj1.activeAction());
    obj1.setActiveAction((QAction *)0);
    QCOMPARE((QAction *)0, obj1.activeAction());
    delete var1;
}

#include <qcursor.h>

/*!
    Test plan:
        insertItem (all flavors and combinations)
        removing menu items
        clearing the menu

        check the common behaviour + emitted signals for:
            accelerator keys
            navigating tru the menu and then pressing ENTER
            mouse clicks
            mouse drags
            combinations of key + mouse (if possible)
            checked / unckecked state of menu options
            active / inactive state

    Can't test these without pixmap comparison...
        show and hide
        icons in a menu
        pixmaps in a menu

*/

tst_QMenuBar::tst_QMenuBar() : m_lastSimpleAcceleratorId(0), m_simpleActivatedCount(0)
{
    QApplication::setEffectEnabled(Qt::UI_AnimateMenu, false);
}

void tst_QMenuBar::onSimpleActivated( QAction* action )
{
    m_lastSimpleAcceleratorId = action;
    m_simpleActivatedCount++;
}

void tst_QMenuBar::cleanup()
{
    QVERIFY(QApplication::topLevelWidgets().isEmpty());
}

// Create a simple menu bar and connect its actions to onSimpleActivated().

TestMenu tst_QMenuBar::initSimpleMenuBar(QMenuBar *mb, bool forceNonNative) {
    TestMenu result;
    if (forceNonNative)
        mb->setNativeMenuBar(false);
    connect(mb, SIGNAL(triggered(QAction*)), this, SLOT(onSimpleActivated(QAction*)));
    QMenu *menu = mb->addMenu(QStringLiteral("&accel"));
    QAction *action = menu->addAction(QStringLiteral("menu1") );
    action->setShortcut(QKeySequence(Qt::ALT + Qt::Key_A));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_A));
    connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(onSimpleActivated(QAction*)));
    result.menus << menu;
    result.actions << action;

    menu = mb->addMenu(QStringLiteral("accel1"));
    action = menu->addAction(QStringLiteral("&Open...") );
    action->setShortcut(Qt::Key_O);
    result.actions << action;

    action = menu->addAction(QStringLiteral("action"));
    action->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Z));
    result.actions << action;

    result.menus << menu;
    connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(onSimpleActivated(QAction*)));

    m_lastSimpleAcceleratorId = 0;
    m_simpleActivatedCount = 0;

    return result;
}

inline TestMenu tst_QMenuBar::initWindowWithSimpleMenuBar(QMainWindow &w, bool forceNonNative)
{
    w.resize(200, 200);
    centerOnScreen(&w);
    return initSimpleMenuBar(w.menuBar(), forceNonNative);
}

// add a menu with number n, set number as data.
QMenu *tst_QMenuBar::addNumberedMenu(QMenuBar *mb, int n)
{
    const QString text = QStringLiteral("Menu &") + QString::number(n);
    QMenu *menu = mb->addMenu(text);
    menu->setObjectName(text);
    QAction *action = menu->menuAction();
    action->setObjectName(text + QStringLiteral("Action"));
    action->setData(QVariant(n));
    connect(action, SIGNAL(triggered()), this, SLOT(onComplexActionTriggered()));
    return menu;
}

// Create an action triggering on Ctrl+character, set number as data.
QAction *tst_QMenuBar::createCharacterAction(QMenu *menu, char lowerAscii)
{
    const QString text = QStringLiteral("Item ") + QChar(QLatin1Char(lowerAscii)).toUpper();
    QAction *action = menu->addAction(text);
    action->setObjectName(text);
    action->setData(QVariant(int(lowerAscii)));
    action->setShortcut(Qt::CTRL + (lowerAscii - 'a' + Qt::Key_A));
    connect(action, SIGNAL(triggered()), this, SLOT(onComplexActionTriggered()));
    return action;
}

void tst_QMenuBar::onComplexActionTriggered()
{
    if (QAction *a = qobject_cast<QAction *>(sender()))
        m_complexTriggerCount[a->data().toInt()]++;
}

// Create a complex menu bar and connect its actions to onComplexActionTriggered()
// for their invocations to be counted in m_complexTriggerCount. The index is the
// menu number (1..n) for the menu bar actions and the ASCII-code of the shortcut
// character for the actions in the menus.
TestMenu tst_QMenuBar::initComplexMenuBar(QMenuBar *mb)
{
    TestMenu result;
    mb->setNativeMenuBar(false);
    QMenu *menu = addNumberedMenu(mb, 1);
    result.menus << menu;
    for (char c = 'a'; c < 'c'; ++c)
        result.actions << createCharacterAction(menu, c);

    menu = addNumberedMenu(mb, 2);
    menu->menuAction()->setData(QVariant(2));
    result.menus << menu;
    for (char c = 'c'; c < 'g'; ++c)
        result.actions << createCharacterAction(menu, c);
    menu->addSeparator();
    for (char c = 'g'; c < 'i'; ++c)
        result.actions << createCharacterAction(menu, c);

    QAction *action = mb->addAction(QStringLiteral("M&enu 3"));
    action->setData(QVariant(3));
    action->setShortcut(Qt::ALT + Qt::Key_J);
    connect(action, SIGNAL(triggered()), this, SLOT(onComplexActionTriggered()));
    result.actions << action;

    std::fill(m_complexTriggerCount, m_complexTriggerCount + sizeof(m_complexTriggerCount) / sizeof(int), 0);

    return result;
}

inline TestMenu tst_QMenuBar::initWindowWithComplexMenuBar(QMainWindow &w)
{
    w.resize(400, 200);
    centerOnScreen(&w);
    return initComplexMenuBar(w.menuBar());
}

// On Mac native key events are needed to test menu action activation
#if !defined(Q_OS_DARWIN)
void tst_QMenuBar::accel()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    // create a popup menu with menu items set the accelerators later...
    QMainWindow w;
    const TestMenu menu = initWindowWithSimpleMenuBar(w);
    w.show();
    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));
    // shortcuts won't work unless the window is active
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_A, Qt::ControlModifier );
    QTest::qWait(300);

    QCOMPARE( m_lastSimpleAcceleratorId, menu.actions.front() );
}
#endif

// On Mac native key events are needed to test menu action activation
#if !defined(Q_OS_DARWIN)
void tst_QMenuBar::activatedCount()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    // create a popup menu with menu items set the accelerators later...
    QMainWindow w;
    QFETCH( bool, forceNonNative );
    initWindowWithSimpleMenuBar(w, forceNonNative);
    w.show();
    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_A, Qt::ControlModifier );
//wait(5000);
    QCOMPARE( m_simpleActivatedCount, 2 ); //1 from the popupmenu and 1 from the menubar
}

void tst_QMenuBar::activatedCount_data()
{
    QTest::addColumn<bool>("forceNonNative");
    QTest::newRow( "forcing non-native menubar" ) << true;
    QTest::newRow( "not forcing non-native menubar" ) << false;
}
#endif

void tst_QMenuBar::clear()
{
    QMenuBar menuBar;
    initSimpleMenuBar(&menuBar);
    menuBar.clear();
    QCOMPARE( menuBar.actions().size(), 0 );

    menuBar.clear();
    for (int i = 0; i < 10; i++) {
        QMenu *menu = menuBar.addMenu( QStringLiteral("Menu ") + QString::number(i));
        for (int k = 0; k < i; k++)
            menu->addAction( QStringLiteral("Item ") + QString::number(k));
        QCOMPARE( menuBar.actions().size(), i + 1 );
    }
    QCOMPARE( menuBar.actions().size(), 10 );
    menuBar.clear();
    QCOMPARE( menuBar.actions().size(), 0 );
}

void tst_QMenuBar::count()
{
    QMenuBar menuBar;
    QCOMPARE( menuBar.actions().size(), 0 );

    for (int i = 0; i < 10; i++) {
        menuBar.addAction( QStringLiteral("Menu ") + QString::number(i));
        QCOMPARE( menuBar.actions().size(), i + 1 );
    }
}

void tst_QMenuBar::removeItem_data()
{
    QTest::addColumn<int>("removeIndex");
    QTest::newRow( "first" ) << 0;
    QTest::newRow( "middle" ) << 1;
    QTest::newRow( "last" ) << 2;
}

// Basically the same test as removeItemAt, except that we remember and remove id's.
void tst_QMenuBar::removeItem()
{
    QMenuBar menuBar;

    QMenu *pm = new QMenu( "stuff", &menuBar );
    pm->setTitle("Menu 1");
    pm->addAction( QString("Item 10") );
    QAction* action1 = menuBar.addMenu( pm );

    pm = new QMenu( &menuBar );
    pm->setTitle("Menu 2");
    pm->addAction( QString("Item 20") );
    pm->addAction( QString("Item 21") );
    QAction *action2 = menuBar.addMenu( pm );

    pm = new QMenu( "Menu 3", &menuBar );
    pm->addAction( QString("Item 30") );
    pm->addAction( QString("Item 31") );
    pm->addAction( QString("Item 32") );
    QAction *action3 = menuBar.addMenu( pm );

    const QList<QAction *> menuBarActions = menuBar.actions();

    QCOMPARE( action1->text(), QString("Menu 1") );
    QCOMPARE( action2->text(), QString("Menu 2") );
    QCOMPARE( action3->text(), QString("Menu 3") );

    QVERIFY( menuBarActions.at(0) == action1 );
    QVERIFY( menuBarActions.at(1) == action2 );
    QVERIFY( menuBarActions.at(2) == action3 );

    // Ok, now that we know we have created the menu we expect, lets remove an item...
    QFETCH( int, removeIndex );
    switch (removeIndex )
    {
    case 0: {
            menuBar.removeAction(action1);
            const QList<QAction *> menuBarActions2 = menuBar.actions();
            QCOMPARE( menuBarActions2.at(0)->text(), QString("Menu 2") );
            QCOMPARE( menuBarActions2.at(1)->text(), QString("Menu 3") );
        }
        break;
    case 1: {
            menuBar.removeAction(action2);
            const QList<QAction *> menuBarActions2 = menuBar.actions();
            QCOMPARE( menuBarActions2.at(0)->text(), QString("Menu 1") );
            QCOMPARE( menuBarActions2.at(1)->text(), QString("Menu 3") );
        }
        break;
    case 2: {
            menuBar.removeAction(action3);
            const QList<QAction *> menuBarActions2 = menuBar.actions();
            QCOMPARE( menuBarActions2.at(0)->text(), QString("Menu 1") );
            QCOMPARE( menuBarActions2.at(1)->text(), QString("Menu 2") );
        }
        break;
    }
    QList<QAction *> menuBarActions2 = menuBar.actions();
    QVERIFY( menuBarActions2.size() == 2 );
}

void tst_QMenuBar::removeItemAt_data()
{
    QTest::addColumn<int>("removeIndex");
    QTest::newRow( "first" ) << 0;
    QTest::newRow( "middle" ) << 1;
    QTest::newRow( "last" ) << 2;
}

void tst_QMenuBar::removeItemAt()
{
    QMenuBar menuBar;
    QMenu *pm = new QMenu("Menu 1", &menuBar);
    pm->addAction( QString("Item 10") );
    menuBar.addMenu( pm );

    pm = new QMenu( "Menu 2", &menuBar);
    pm->addAction( QString("Item 20") );
    pm->addAction( QString("Item 21") );
    menuBar.addMenu( pm );

    pm = new QMenu( "Menu 3", &menuBar);
    pm->addAction( QString("Item 30") );
    pm->addAction( QString("Item 31") );
    pm->addAction( QString("Item 32") );
    menuBar.addMenu( pm );

    QList<QAction *> menuBarActions = menuBar.actions();

    QCOMPARE( menuBarActions.at(0)->text(), QString("Menu 1") );
    QCOMPARE( menuBarActions.at(1)->text(), QString("Menu 2") );
    QCOMPARE( menuBarActions.at(2)->text(), QString("Menu 3") );

    // Ok, now that we know we have created the menu we expect, lets remove an item...
    QFETCH( int, removeIndex );
    menuBar.removeAction(menuBarActions.at(removeIndex));
    const QList<QAction *> menuBarActions2 = menuBar.actions();
    switch (removeIndex )
    {
    case 0:
        QCOMPARE( menuBarActions2.at(0)->text(), QString("Menu 2") );
        QCOMPARE( menuBarActions2.at(1)->text(), QString("Menu 3") );
        break;
    case 1:
        QCOMPARE( menuBarActions2.at(0)->text(), QString("Menu 1") );
        QCOMPARE( menuBarActions2.at(1)->text(), QString("Menu 3") );
        break;
    case 2:
        QCOMPARE( menuBarActions2.at(0)->text(), QString("Menu 1") );
        QCOMPARE( menuBarActions2.at(1)->text(), QString("Menu 2") );
        break;
    }

    QVERIFY( menuBarActions2.size() == 2 );
}

/*
    Check the insert functions that create menu items.
    For the moment i only check the strings and pixmaps. The rest are special cases which are
    used less frequently.
*/

void tst_QMenuBar::insertItem_QString_QObject()
{
    QMenuBar menuBar;
    initComplexMenuBar(&menuBar);

    const QList<QAction *> actions = menuBar.actions();

    QCOMPARE(actions.at(0)->text(), QString("Menu &1") );
    QCOMPARE(actions.at(1)->text(), QString("Menu &2") );
    QCOMPARE(actions.at(2)->text(), QString("M&enu 3") );
    QVERIFY(actions.size() < 4); // there is no menu 4!
}

// On Mac native key events are needed to test menu action activation
#if !defined(Q_OS_DARWIN)
void tst_QMenuBar::check_accelKeys()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QMainWindow w;
    initWindowWithComplexMenuBar(w);
    w.show();
    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    // start with a bogus key that shouldn't trigger anything
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_I, Qt::ControlModifier);
    QCOMPARE(m_complexTriggerCount[1], 0);
    QCOMPARE(m_complexTriggerCount[2], 0);
    QCOMPARE(m_complexTriggerCount[3], 0);
    QCOMPARE(m_complexTriggerCount[4], 0);
    QCOMPARE(m_complexTriggerCount[int('a')], 0);
    QCOMPARE(m_complexTriggerCount[int('b')], 0);
    QCOMPARE(m_complexTriggerCount[int('c')], 0);
    QCOMPARE(m_complexTriggerCount[int('d')], 0);

    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_A, Qt::ControlModifier);
    QCOMPARE(m_complexTriggerCount[1], 0);
    QCOMPARE(m_complexTriggerCount[2], 0);
    QCOMPARE(m_complexTriggerCount[3], 0);
    QCOMPARE(m_complexTriggerCount[4], 0);
    QCOMPARE(m_complexTriggerCount[int('a')], 1);
    QCOMPARE(m_complexTriggerCount[int('b')], 0);
    QCOMPARE(m_complexTriggerCount[int('c')], 0);
    QCOMPARE(m_complexTriggerCount[int('d')], 0);

    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_C, Qt::ControlModifier);
    QCOMPARE(m_complexTriggerCount[1], 0);
    QCOMPARE(m_complexTriggerCount[2], 0);
    QCOMPARE(m_complexTriggerCount[3], 0);
    QCOMPARE(m_complexTriggerCount[4], 0);
    QCOMPARE(m_complexTriggerCount[int('a')], 1);
    QCOMPARE(m_complexTriggerCount[int('b')], 0);
    QCOMPARE(m_complexTriggerCount[int('c')], 1);
    QCOMPARE(m_complexTriggerCount[int('d')], 0);

    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_B, Qt::ControlModifier);
    QCOMPARE(m_complexTriggerCount[1], 0);
    QCOMPARE(m_complexTriggerCount[2], 0);
    QCOMPARE(m_complexTriggerCount[3], 0);
    QCOMPARE(m_complexTriggerCount[4], 0);
    QCOMPARE(m_complexTriggerCount[int('a')], 1);
    QCOMPARE(m_complexTriggerCount[int('b')], 1);
    QCOMPARE(m_complexTriggerCount[int('c')], 1);
    QCOMPARE(m_complexTriggerCount[int('d')], 0);

    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_D, Qt::ControlModifier);
    QCOMPARE(m_complexTriggerCount[1], 0);
    QCOMPARE(m_complexTriggerCount[2], 0);
    QCOMPARE(m_complexTriggerCount[3], 0);
    QCOMPARE(m_complexTriggerCount[4], 0);
    QCOMPARE(m_complexTriggerCount[int('a')], 1);
    QCOMPARE(m_complexTriggerCount[int('b')], 1);
    QCOMPARE(m_complexTriggerCount[int('c')], 1);
    QCOMPARE(m_complexTriggerCount[int('d')], 1);

    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_J, Qt::AltModifier);
    QCOMPARE(m_complexTriggerCount[1], 0);
    QCOMPARE(m_complexTriggerCount[2], 0);
    QCOMPARE(m_complexTriggerCount[3], 1);
    QCOMPARE(m_complexTriggerCount[4], 0);
    QCOMPARE(m_complexTriggerCount[int('a')], 1);
    QCOMPARE(m_complexTriggerCount[int('b')], 1);
    QCOMPARE(m_complexTriggerCount[int('c')], 1);
    QCOMPARE(m_complexTriggerCount[int('d')], 1);
}
#endif

// On Mac native key events are needed to test menu action activation
#if !defined(Q_OS_DARWIN)
void tst_QMenuBar::check_cursorKeys1()
{
    if (qgetenv("XDG_CURRENT_DESKTOP") == "Unity")
        QSKIP("This test is flaky on Ubuntu/Unity due to regression introduced by QTBUG-39362");

    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QMainWindow w;
    initWindowWithComplexMenuBar(w);
    w.show();
    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    // start with a ALT + 1 that activates the first popupmenu
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_1, Qt::AltModifier );
    // the Popupmenu should be visible now
    QCOMPARE(m_complexTriggerCount[3], 0);
    QCOMPARE(m_complexTriggerCount[4], 0);
    QCOMPARE(m_complexTriggerCount[int('a')], 0);
    QCOMPARE(m_complexTriggerCount[int('b')], 0);
    QCOMPARE(m_complexTriggerCount[int('c')], 0);
    QCOMPARE(m_complexTriggerCount[int('d')], 0);

    // Simulate a cursor key down click
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Down );
    // and an Enter key
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Enter );
    // Let's see if the correct slot is called...
    QCOMPARE(m_complexTriggerCount[3], 0);
    QCOMPARE(m_complexTriggerCount[4], 0);
    QCOMPARE(m_complexTriggerCount[int('a')], 0); // this shouldn't have been called
    QCOMPARE(m_complexTriggerCount[int('b')], 1); // and this should have been called by a signal now
    QCOMPARE(m_complexTriggerCount[int('c')], 0);
    QCOMPARE(m_complexTriggerCount[int('d')], 0);
}
#endif

// Qt/Mac does not use the native popups/menubar
#if !defined(Q_OS_DARWIN)
void tst_QMenuBar::check_cursorKeys2()
{
    if (qgetenv("XDG_CURRENT_DESKTOP") == "Unity")
        QSKIP("This test is flaky on Ubuntu/Unity due to regression introduced by QTBUG-39362");

    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QMainWindow w;
    initWindowWithComplexMenuBar(w);
    w.show();
    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    // select popupmenu2
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_2, Qt::AltModifier );

    // Simulate some cursor keys
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Left );
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Down );
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Right );
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Down );
    // and an Enter key
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Enter );
    // Let's see if the correct slot is called...
    QCOMPARE(m_complexTriggerCount[3], 0);
    QCOMPARE(m_complexTriggerCount[4], 0);
    QCOMPARE(m_complexTriggerCount[int('a')], 0); // this shouldn't have been caled
    QCOMPARE(m_complexTriggerCount[int('b')], 0); // and this should have been called by a signal ow
    QCOMPARE(m_complexTriggerCount[int('c')], 0);
    QCOMPARE(m_complexTriggerCount[int('d')], 1);
}
#endif

/*!
    If a popupmenu is active you can use Left to move to the menu to the left of it.
*/
// Qt/Mac does not use the native popups/menubar
#if !defined(Q_OS_DARWIN)
void tst_QMenuBar::check_cursorKeys3()
{
    if (qgetenv("XDG_CURRENT_DESKTOP") == "Unity")
        QSKIP("This test is flaky on Ubuntu/Unity due to regression introduced by QTBUG-39362");

    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QMainWindow w;
    initWindowWithComplexMenuBar(w);
    w.show();
    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    // select Popupmenu 2
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_2, Qt::AltModifier );

    // Simulate some keys
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Left );
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Down );
    // and press ENTER
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Enter );
    // Let's see if the correct slot is called...
    QCOMPARE(m_complexTriggerCount[3], 0);
    QCOMPARE(m_complexTriggerCount[4], 0);
    QCOMPARE(m_complexTriggerCount[int('a')], 0); // this shouldn't have been called
    QCOMPARE(m_complexTriggerCount[int('b')], 1); // and this should have been called by a signal now
    QCOMPARE(m_complexTriggerCount[int('c')], 0);
    QCOMPARE(m_complexTriggerCount[int('d')], 0);
}
#endif

void tst_QMenuBar::taskQTBUG56860_focus()
{
#if defined(Q_OS_DARWIN)
    QSKIP("Native key events are needed to test menu action activation on macOS.");
#endif
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QMainWindow w;
    QMenuBar *mb = w.menuBar();
    mb->setNativeMenuBar(false);

    QMenu *em = mb->addMenu("&Edit");
    em->setObjectName("EditMenu");
    em->addAction("&Cut");
    em->addAction("C&opy");
    QPlainTextEdit *e = new QPlainTextEdit;
    e->setObjectName("edit");

    w.setCentralWidget(e);
    w.show();
    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    QTRY_COMPARE(QApplication::focusWidget(), e);

    // Open menu
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_E, Qt::AltModifier );
    QTRY_COMPARE(QApplication::activePopupWidget(), em);
    // key down to trigger focus
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Down );
    // and press ENTER to close
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Enter );
    QTRY_COMPARE(QApplication::activePopupWidget(), nullptr);
    // focus should have returned to the editor by now
    QTRY_COMPARE(QApplication::focusWidget(), e);

    // Now do it all over again...
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_E, Qt::AltModifier );
    QTRY_COMPARE(QApplication::activePopupWidget(), em);
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Down );
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Enter );
    QTRY_COMPARE(QApplication::activePopupWidget(), nullptr);
    QTRY_COMPARE(QApplication::focusWidget(), e);

}

/*!
    If a popupmenu is active you can use home to go quickly to the first item in the menu.
*/
void tst_QMenuBar::check_homeKey()
{
    // I'm temporarily shutting up this testcase.
    // Seems like the behaviour i'm expecting isn't ok.
    QSKIP("This test has been \"temporarily\" disabled at least since 2009 :)");

    QEXPECT_FAIL( "0", "Popupmenu should respond to a Home key", Abort );

    QMainWindow w;
    initWindowWithComplexMenuBar(w);
    w.show();
    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    // select Popupmenu 2
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_2, Qt::AltModifier );

    // Simulate some keys
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Down );
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Down );
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Down );
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Home );
    // and press ENTER
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Enter );
    // Let's see if the correct slot is called...
//    QVERIFY2( m_complexActionTriggerCount[int('c')] == 1, "Popupmenu should respond to a Home key" );
    QCOMPARE(m_complexTriggerCount[int('c')], 1);
    QCOMPARE(m_complexTriggerCount[3], 0);
    QCOMPARE(m_complexTriggerCount[4], 0);
    QCOMPARE(m_complexTriggerCount[int('a')], 0);
    QCOMPARE(m_complexTriggerCount[int('b')], 0);
    QCOMPARE(m_complexTriggerCount[int('d')], 0);
    QCOMPARE(m_complexTriggerCount[int('e')], 0);
    QCOMPARE(m_complexTriggerCount[int('f')], 0);
    QCOMPARE(m_complexTriggerCount[int('g')], 0);
    QCOMPARE(m_complexTriggerCount[int('h')], 0);
}

/*!
    If a popupmenu is active you can use end to go quickly to the last item in the menu.
*/
void tst_QMenuBar::check_endKey()
{
    // I'm temporarily silenting this testcase.
    // Seems like the behaviour i'm expecting isn't ok.
    QSKIP("This test has been \"temporarily\" disabled at least since 2009 :)");

    QEXPECT_FAIL( "0", "Popupmenu should respond to an End key", Abort );

    QMainWindow w;
    initWindowWithComplexMenuBar(w);
    w.show();
    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    // select Popupmenu 2
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_2, Qt::AltModifier );

    // Simulate some keys
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_End );
    // and press ENTER
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Enter );
    // Let's see if the correct slot is called...
//    QVERIFY2( m_complexActionTriggerCount[int('h')] == 1, "Popupmenu should respond to an End key" );
    QCOMPARE(m_complexTriggerCount[int('h')], 1);//, "Popupmenu should respond to an End key");
    QCOMPARE(m_complexTriggerCount[3], 0);
    QCOMPARE(m_complexTriggerCount[4], 0);
    QCOMPARE(m_complexTriggerCount[int('a')], 0);
    QCOMPARE(m_complexTriggerCount[int('b')], 0);
    QCOMPARE(m_complexTriggerCount[int('c')], 0);
    QCOMPARE(m_complexTriggerCount[int('d')], 0);
    QCOMPARE(m_complexTriggerCount[int('e')], 0);
    QCOMPARE(m_complexTriggerCount[int('f')], 0);
    QCOMPARE(m_complexTriggerCount[int('g')], 0);
}

/*!
    If a popupmenu is active you can use esc to hide the menu and then the
    menubar should become active.
    If Down is pressed next the popup is activated again.
*/

// Qt/Mac does not use the native popups/menubar
#if !defined(Q_OS_DARWIN)
void tst_QMenuBar::check_escKey()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QMainWindow w;
    const TestMenu menu = initWindowWithComplexMenuBar(w);
    w.show();
    w.setFocus();
    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    QVERIFY( !menu.menus.at(0)->isActiveWindow() );
    QVERIFY( !menu.menus.at(1)->isActiveWindow() );

    // select Popupmenu 2
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_2, Qt::AltModifier );
    QVERIFY( !menu.menus.at(0)->isActiveWindow() );
    QVERIFY( menu.menus.at(1)->isActiveWindow() );

    // If we press ESC, the popup should disappear
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Escape );
    QVERIFY( !menu.menus.at(0)->isActiveWindow() );
    QVERIFY( !menu.menus.at(1)->isActiveWindow() );

    if (!QApplication::style()->inherits("QWindowsStyle"))
        return;

    if (!QGuiApplication::platformName().compare(QLatin1String("minimal"), Qt::CaseInsensitive)
        || !QGuiApplication::platformName().compare(QLatin1String("offscreen"), Qt::CaseInsensitive)) {
        QWARN("Skipping menu button test on minimal/offscreen platforms");
        return;
    }

    // If we press Down the popupmenu should be active again
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Down );
    QVERIFY( !menu.menus.at(0)->isActiveWindow() );
    QVERIFY( menu.menus.at(1)->isActiveWindow() );

    // and press ENTER
    QTest::keyClick( menu.menus.at(1), Qt::Key_Enter );
    // Let's see if the correct slot is called...
    QVERIFY2(m_complexTriggerCount[int('c')] == 1, "Expected item 2C to be selected");
}
#endif


// void tst_QMenuBar::check_mouse1_data()
// {
//     QTest::addColumn<QString>("popup_item");
//     QTest::addColumn<int>("itemA_count");
//     QTest::addColumn<int>("itemB_count");

//     QTest::newRow( "A" ) << QString( "Item A Ctrl+A" ) << 1 << 0;
//     QTest::newRow( "B" ) << QString( "Item B Ctrl+B" ) << 0 << 1;
// }

// /*!
//     Check if the correct signals are emitted if we select a popupmenu.
// */
// void tst_QMenuBar::check_mouse1()
// {
//     if (QSystem::curStyle() == "Motif")
//         QSKIP("This fails in Motif due to a bug in the testing framework");
//     QFETCH( QString, popup_item );
//     QFETCH( int, itemA_count );
//     QFETCH( int, itemB_count );

// //    initComplexMenubar();
//     QVERIFY( !pm1->isActiveWindow() );
//     QVERIFY( !pm2->isActiveWindow() );

//     QTest::qWait(1000);
//     QtTestMouse mouse;
//     mouse.mouseEvent( QtTestMouse::MouseClick, mb, "Menu &1", Qt::LeftButton );

//     QVERIFY( pm1->isActiveWindow() );
//     QVERIFY( !pm2->isActiveWindow() );

//     QTest::qWait(1000);
//     mouse.mouseEvent( QtTestMouse::MouseClick, pm1, popup_item, Qt::LeftButton );

//     QCOMPARE(m_complexActionTriggerCount[3], 0);
//     QCOMPARE(m_complexActionTriggerCount[4], 0);
//     QCOMPARE(m_complexActionTriggerCount['a'], (uint)itemA_count); // this option should have fired
//     QCOMPARE(m_complexActionTriggerCount['b'], (uint)itemB_count);
//     QCOMPARE(m_complexActionTriggerCount['c'], 0);
//     QCOMPARE(m_complexActionTriggerCount['d'], 0);
//     QCOMPARE(m_complexActionTriggerCount['e'], 0);
//     QCOMPARE(m_complexActionTriggerCount['f'], 0);
//     QCOMPARE(m_complexActionTriggerCount['g'], 0);
// }

// void tst_QMenuBar::check_mouse2_data()
// {
//     QTest::addColumn<QString>("label");
//     QTest::addColumn<int>("itemA_count");
//     QTest::addColumn<int>("itemB_count");
//     QTest::addColumn<int>("itemC_count");
//     QTest::addColumn<int>("itemD_count");
//     QTest::addColumn<int>("itemE_count");
//     QTest::addColumn<int>("itemF_count");
//     QTest::addColumn<int>("itemG_count");
//     QTest::addColumn<int>("itemH_count");
//     QTest::addColumn<int>("menu3_count");

//     QTest::newRow( "A" ) << QString( "Menu &1/Item A Ctrl+A" ) << 1 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0;
//     QTest::newRow( "B" ) << QString( "Menu &1/Item B Ctrl+B" ) << 0 << 1 << 0 << 0 << 0 << 0 << 0 << 0 << 0;
//     QTest::newRow( "C" ) << QString( "Menu &2/Item C Ctrl+C" ) << 0 << 0 << 1 << 0 << 0 << 0 << 0 << 0 << 0;
//     QTest::newRow( "D" ) << QString( "Menu &2/Item D Ctrl+D" ) << 0 << 0 << 0 << 1 << 0 << 0 << 0 << 0 << 0;
//     QTest::newRow( "E" ) << QString( "Menu &2/Item E Ctrl+E" ) << 0 << 0 << 0 << 0 << 1 << 0 << 0 << 0 << 0;
//     QTest::newRow( "F" ) << QString( "Menu &2/Item F Ctrl+F" ) << 0 << 0 << 0 << 0 << 0 << 1 << 0 << 0 << 0;
//     QTest::newRow( "G" ) << QString( "Menu &2/Item G Ctrl+G" ) << 0 << 0 << 0 << 0 << 0 << 0 << 1 << 0 << 0;
//     QTest::newRow( "H" ) << QString( "Menu &2/Item H Ctrl+H" ) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 1 << 0;
//     QTest::newRow( "menu 3" ) << QString( "M&enu 3" )          << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 1;
// }

// /*!
//     Check if the correct signals are emitted if we select a popupmenu.
//     This time, we use a little bit more magic from the testframework.
// */
// void tst_QMenuBar::check_mouse2()
// {
//     if (QSystem::curStyle() == "Motif")
//         QSKIP("This fails in Motif due to a bug in the testing framework");
//     QFETCH( QString, label );
//     QFETCH( int, itemA_count );
//     QFETCH( int, itemB_count );
//     QFETCH( int, itemC_count );
//     QFETCH( int, itemD_count );
//     QFETCH( int, itemE_count );
//     QFETCH( int, itemF_count );
//     QFETCH( int, itemG_count );
//     QFETCH( int, itemH_count );
//     QFETCH( int, menu3_count );

// //    initComplexMenubar();
//     QtTestMouse mouse;
//     mouse.click( QtTestMouse::Menu, label, Qt::LeftButton );

//     // check if the correct signals have fired
//     QCOMPARE(m_complexActionTriggerCount[3], (uint)menu3_count);
//     QCOMPARE(m_complexActionTriggerCount[4], 0);
//     QCOMPARE(m_complexActionTriggerCount['a'], (uint)itemA_count);
//     QCOMPARE(m_complexActionTriggerCount['b'], (uint)itemB_count);
//     QCOMPARE(m_complexActionTriggerCount['c'], (uint)itemC_count);
//     QCOMPARE(m_complexActionTriggerCount['d'], (uint)itemD_count);
//     QCOMPARE(m_complexActionTriggerCount['e'], (uint)itemE_count);
//     QCOMPARE(m_complexActionTriggerCount['f'], (uint)itemF_count);
//     QCOMPARE(m_complexActionTriggerCount['g'], (uint)itemG_count);
//     QCOMPARE(m_complexActionTriggerCount['h'], (uint)itemH_count);
// }

void tst_QMenuBar::allowActiveAndDisabled()
{
    QMenuBar menuBar;
    menuBar.setNativeMenuBar(false);

     // Task 241043 : check that second menu is activated if only
    // disabled menu items are added

    QMenu fileMenu("&File");
    // Task 241043 : check that second menu is activated
    // if all items are disabled
    QAction *act = fileMenu.addAction("Disabled");
    act->setEnabled(false);

    menuBar.addMenu(&fileMenu);
    QMenu disabledMenu("Disabled");
    disabledMenu.setEnabled(false);
    QMenu activeMenu("Active");
    menuBar.addMenu(&disabledMenu);
    menuBar.addMenu(&activeMenu);
    centerOnScreen(&menuBar);
    menuBar.show();
    QVERIFY(QTest::qWaitForWindowExposed(&menuBar));

    // Here we verify that AllowActiveAndDisabled correctly skips
    // the disabled menu entry
    QTest::keyClick(&menuBar, Qt::Key_F, Qt::AltModifier );
    QTest::keyClick(&fileMenu, (Qt::Key_Right));
    if (qApp->style()->styleHint(QStyle::SH_Menu_AllowActiveAndDisabled))
        QCOMPARE(menuBar.activeAction()->text(), disabledMenu.title());
    else
        QCOMPARE(menuBar.activeAction()->text(), activeMenu.title());

    QTest::keyClick(&menuBar, (Qt::Key_Left));
    if (qApp->style()->styleHint(QStyle::SH_Menu_AllowActiveAndDisabled))
        QCOMPARE(menuBar.activeAction()->text(), fileMenu.title());
    else
        QCOMPARE(menuBar.activeAction()->text(), fileMenu.title());
}

void tst_QMenuBar::check_altPress()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    if ( !qApp->style()->styleHint(QStyle::SH_MenuBar_AltKeyNavigation) ) {
        QSKIP(QString( "this is not supposed to work in the %1 style. Skipping." ).
              arg(qApp->style()->objectName()).toLatin1());
    }

    QMainWindow w;
    initWindowWithSimpleMenuBar(w);
    w.show();
    w.setFocus();
    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    QTest::keyClick( &w, Qt::Key_Alt );
    QTRY_VERIFY( ::qobject_cast<QMenuBar *>(qApp->focusWidget()) );
}

// QTBUG-47377: Pressing 'Alt' after opening a menu by pressing 'Alt+Accelerator'
// should close it and QMenuBar::activeAction() should be 0.
void tst_QMenuBar::check_altClosePress()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    const QStyle *style = QApplication::style();
    if (!style->styleHint(QStyle::SH_MenuBar_AltKeyNavigation) ) {
        QSKIP(("This test is not supposed to work in the " + style->objectName().toLatin1()
               + " style. Skipping.").constData());
    }

    QMainWindow w;
    w.setWindowTitle(QTest::currentTestFunction());
    w.menuBar()->setNativeMenuBar(false);
    QMenu *menuFile = w.menuBar()->addMenu(tr("&File"));
    menuFile->addAction("Quit");
    QMenu *menuEdit = w.menuBar()->addMenu(tr("&Edit"));
    menuEdit->addAction("Copy");

    w.show();
    w.move(QGuiApplication::primaryScreen()->availableGeometry().center());
    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    QTest::keyClick(&w, Qt::Key_F, Qt::AltModifier);
    QTRY_VERIFY(menuFile->isVisible());
    QTest::keyClick(menuFile, Qt::Key_Alt, Qt::AltModifier);
    QTRY_VERIFY(!menuFile->isVisible());
    QTRY_VERIFY(!w.menuBar()->activeAction());
}

// Qt/Mac does not use the native popups/menubar
#if !defined(Q_OS_DARWIN)
void tst_QMenuBar::check_shortcutPress()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QMainWindow w;
    const TestMenu menu = initWindowWithComplexMenuBar(w);
    w.show();
    w.setFocus();
    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    QCOMPARE(m_complexTriggerCount[3], 0);
    QTest::keyClick(&w, Qt::Key_E, Qt::AltModifier);
    QTest::qWait(200);
    QCOMPARE(m_complexTriggerCount[3], 1);
    QVERIFY(!w.menuBar()->activeAction());

    QTest::keyClick(&w, Qt::Key_1, Qt::AltModifier );
    QVERIFY(menu.menus.at(0)->isActiveWindow());
    QTest::keyClick(&w, Qt::Key_2);
    QVERIFY(menu.menus.at(0)->isActiveWindow());
}
#endif

class LayoutDirectionSaver
{
    Q_DISABLE_COPY(LayoutDirectionSaver)
public:
    explicit LayoutDirectionSaver(Qt::LayoutDirection direction)
        : m_oldDirection(qApp->layoutDirection())
    {
        qApp->setLayoutDirection(direction);
    }

    ~LayoutDirectionSaver()
    {
        qApp->setLayoutDirection(m_oldDirection);
    }

private:
    const Qt::LayoutDirection m_oldDirection;
};

// Qt/Mac does not use the native popups/menubar
#if !defined(Q_OS_DARWIN)
void tst_QMenuBar::check_menuPosition()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QMainWindow w;

    Menu menu;
    menu.setTitle("&menu");
    QRect availRect = w.screen()->availableGeometry();
    QRect screenRect = w.screen()->geometry();

    while(menu.sizeHint().height() < (screenRect.height()*2/3)) {
        menu.addAction("item");
    }

    w.menuBar()->setNativeMenuBar(false);
    QAction *menu_action = w.menuBar()->addMenu(&menu);
    centerOnScreen(&w);
    w.show();
    qApp->setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    //the menu should be below the menubar item
    {
        w.move(availRect.topLeft());
        QRect mbItemRect = w.menuBar()->actionGeometry(menu_action);
        mbItemRect.moveTo(w.menuBar()->mapToGlobal(mbItemRect.topLeft()));
        QTest::keyClick(&w, Qt::Key_M, Qt::AltModifier );
        QVERIFY(menu.isActiveWindow());
        QCOMPARE(menu.pos(), QPoint(mbItemRect.x(), mbItemRect.bottom() + 1));
        menu.close();
    }

    //the menu should be above the menubar item
    {
        w.move(0,screenRect.bottom() - screenRect.height()/4); //just leave some place for the menubar
        QRect mbItemRect = w.menuBar()->actionGeometry(menu_action);
        mbItemRect.moveTo(w.menuBar()->mapToGlobal(mbItemRect.topLeft()));
        QTest::keyClick(&w, Qt::Key_M, Qt::AltModifier );
        QVERIFY(menu.isActiveWindow());
#ifdef Q_OS_WINRT
        QEXPECT_FAIL("", "QTest::keyClick does not work on WinRT.", Abort);
#endif
        QCOMPARE(menu.pos(), QPoint(mbItemRect.x(), mbItemRect.top() - menu.height()));
        menu.close();
    }

    //the menu should be on the side of the menubar item and should be "stuck" to the bottom of the screen
    {
        w.move(0,screenRect.y() + screenRect.height()/2); //put it in the middle
        QRect mbItemRect = w.menuBar()->actionGeometry(menu_action);
        mbItemRect.moveTo(w.menuBar()->mapToGlobal(mbItemRect.topLeft()));
        QTest::keyClick(&w, Qt::Key_M, Qt::AltModifier );
        QVERIFY(menu.isActiveWindow());
        QPoint firstPoint = QPoint(mbItemRect.right()+1, screenRect.bottom() - menu.height() + 1);
        QPoint secondPoint = QPoint(mbItemRect.right()+1, availRect.bottom() - menu.height() + 1);
        QVERIFY(menu.pos() == firstPoint || menu.pos() == secondPoint);
        menu.close();
    }

    // QTBUG-2596: in RTL, the menu should be stuck at the right of the action geometry
    {
        LayoutDirectionSaver directionSaver(Qt::RightToLeft);
        menu.clear();
        QObject::connect(&menu, SIGNAL(aboutToShow()), &menu, SLOT(addActions()));
        QRect mbItemRect = w.menuBar()->actionGeometry(menu_action);
        mbItemRect.moveTo(w.menuBar()->mapToGlobal(mbItemRect.topLeft()));
        QTest::keyClick(&w, Qt::Key_M, Qt::AltModifier );
        QVERIFY(menu.isActiveWindow());
        QCOMPARE(menu.geometry().right(), mbItemRect.right());
        menu.close();
    }

    // QTBUG-28031: Click at bottom-right corner.
    {
        w.move(400, 200);
        LayoutDirectionSaver directionSaver(Qt::RightToLeft);
        QMenuBar *mb = w.menuBar();
        const QPoint bottomRight = mb->actionGeometry(menu.menuAction()).bottomRight() - QPoint(1, 1);
        const QPoint localPos = widgetToWindowPos(mb, bottomRight);
        const QPoint globalPos = w.mapToGlobal(localPos);
        QTest::mouseClick(w.windowHandle(), Qt::LeftButton, {}, localPos);
        QTRY_VERIFY(menu.isActiveWindow());
        QCOMPARE(menu.geometry().right() - 1, globalPos.x());
        menu.close();
    }
}
#endif

void tst_QMenuBar::task223138_triggered()
{
    //we create a window with submenus and we check that both menubar and menus get the triggered signal
    QMainWindow win;
    centerOnScreen(&win);
    QMenu *menu = win.menuBar()->addMenu("test");
    QAction *top = menu->addAction("toplevelaction");
    QMenu *submenu = menu->addMenu("nested menu");
    QAction *action = submenu->addAction("nested action");

    QSignalSpy menubarSpy(win.menuBar(), SIGNAL(triggered(QAction*)));
    QSignalSpy menuSpy(menu, SIGNAL(triggered(QAction*)));
    QSignalSpy submenuSpy(submenu, SIGNAL(triggered(QAction*)));

    //let's trigger the first action
    top->trigger();

    QCOMPARE(menubarSpy.count(), 1);
    QCOMPARE(menuSpy.count(), 1);
    QCOMPARE(submenuSpy.count(), 0);

    menubarSpy.clear();
    menuSpy.clear();
    submenuSpy.clear();

    //let's trigger the sub action
    action->trigger();
    QCOMPARE(menubarSpy.count(), 1);
    QCOMPARE(menuSpy.count(), 1);
    QCOMPARE(submenuSpy.count(), 1);
}

void tst_QMenuBar::task256322_highlight()
{
    if (!QGuiApplication::platformName().compare(QLatin1String("minimal"), Qt::CaseInsensitive))
        QSKIP("Highlighting does not work correctly for minimal platform");

    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QMainWindow win;
    win.menuBar()->setNativeMenuBar(false);  //we can't check the geometry of native menubars
    QMenu menu;
    QAction *file = win.menuBar()->addMenu(&menu);
    file->setText("file");
    QMenu menu2;
    QAction *file2 = win.menuBar()->addMenu(&menu2);
    file2->setText("file2");
    QAction *nothing = win.menuBar()->addAction("nothing");

    centerOnScreen(&win);
    win.show();
    QApplication::setActiveWindow(&win);
    QVERIFY(QTest::qWaitForWindowActive(&win));

    const QPoint filePos = menuBarActionWindowPos(win.menuBar(), file);
    QWindow *window = win.windowHandle();
    QTest::mousePress(window, Qt::LeftButton, {}, filePos);
    QTest::mouseMove(window, filePos);
    QTest::mouseRelease(window, Qt::LeftButton, {}, filePos);
    QTRY_VERIFY(menu.isVisible());
    QVERIFY(!menu2.isVisible());
    QCOMPARE(win.menuBar()->activeAction(), file);

    const QPoint file2Pos = menuBarActionWindowPos(win.menuBar(), file2);
    QTest::mouseMove(window, file2Pos);
    QTRY_VERIFY(!menu.isVisible());
    QTRY_VERIFY(menu2.isVisible());
    QCOMPARE(win.menuBar()->activeAction(), file2);

    QPoint nothingCenter = menuBarActionWindowPos(win.menuBar(), nothing);
    QTest::mouseMove(window, nothingCenter);
    QTRY_VERIFY(!menu2.isVisible());
    QVERIFY(!menu.isVisible());
    QTRY_COMPARE(win.menuBar()->activeAction(), nothing);
}

void tst_QMenuBar::menubarSizeHint()
{
    struct MyStyle : public QProxyStyle
    {
        MyStyle() : QProxyStyle(QStyleFactory::create("windows")) { }

        virtual int pixelMetric(PixelMetric metric, const QStyleOption * option = 0, const QWidget * widget = 0 ) const
        {
            // I chose strange values (prime numbers to be more sure that the size of the menubar is correct)
            switch (metric)
            {
            case QStyle::PM_MenuBarItemSpacing:
                return 7;
            case PM_MenuBarHMargin:
                return 13;
            case PM_MenuBarVMargin:
                return 11;
            case PM_MenuBarPanelWidth:
                return 1;
            default:
              return QProxyStyle::pixelMetric(metric, option, widget);
            }
        }
    } style;

    QMenuBar mb;
    mb.setNativeMenuBar(false); //we can't check the geometry of native menubars

    mb.setStyle(&style);
    //this is a list of arbitrary strings so that we check the geometry
    QStringList list = QStringList() << "trer" << "ezrfgtgvqd" << "sdgzgzerzerzer" << "eerzertz"  << "er";
    foreach(QString str, list)
        mb.addAction(str);

    const int panelWidth = style.pixelMetric(QStyle::PM_MenuBarPanelWidth);
    const int hmargin = style.pixelMetric(QStyle::PM_MenuBarHMargin);
    const int vmargin = style.pixelMetric(QStyle::PM_MenuBarVMargin);
    const int spacing = style.pixelMetric(QStyle::PM_MenuBarItemSpacing);

    centerOnScreen(&mb);
    mb.show();
    QRect result;
    foreach(QAction *action, mb.actions()) {
        const QRect actionRect = mb.actionGeometry(action);
        if (!result.isNull()) //this is the first item
            QCOMPARE(actionRect.left() - result.right() - 1, spacing);
        result |= actionRect;
        QCOMPARE(result.x(), panelWidth + hmargin + spacing);
        QCOMPARE(result.y(), panelWidth + vmargin);
    }

    //this code is copied from QMenuBar
    //there is no public member that allows to initialize a styleoption instance
    QStyleOptionMenuItem opt;
    opt.rect = mb.rect();
    opt.menuRect = mb.rect();
    opt.state = QStyle::State_None;
    opt.menuItemType = QStyleOptionMenuItem::Normal;
    opt.checkType = QStyleOptionMenuItem::NotCheckable;
    opt.palette = mb.palette();

    QSize resSize = QSize(result.x(), result.y()) + result.size()
        + QSize(panelWidth + hmargin, panelWidth + vmargin);


    resSize = style.sizeFromContents(QStyle::CT_MenuBar, &opt,
                                         resSize.expandedTo(QApplication::globalStrut()),
                                         &mb);

    QCOMPARE(resSize, mb.sizeHint());
}

// On Mac, do not test the menubar with escape key
#ifndef Q_OS_MACOS
void tst_QMenuBar::taskQTBUG4965_escapeEaten()
{
    QMenuBar menubar;
    menubar.setNativeMenuBar(false);
    QMenu menu("menu1");
    QAction *first = menubar.addMenu(&menu);
    menu.addAction("quit", &menubar, SLOT(close()), QKeySequence("ESC"));
    centerOnScreen(&menubar);
    menubar.show();
    QApplication::setActiveWindow(&menubar);
    QVERIFY(QTest::qWaitForWindowExposed(&menubar));
    menubar.setActiveAction(first);
    QTRY_VERIFY(menu.isVisible());
    QCOMPARE(menubar.activeAction(), first);
    QVERIFY(QTest::qWaitForWindowExposed(&menu));
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Escape);
    QVERIFY(!menu.isVisible());
    QTRY_VERIFY(menubar.hasFocus());
    QCOMPARE(menubar.activeAction(), first);
    QTest::qWait(200);
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Escape);
    QVERIFY(!menubar.activeAction());
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Escape); //now the action should be triggered
    QTRY_VERIFY(!menubar.isVisible());
}
#endif

void tst_QMenuBar::taskQTBUG11823_crashwithInvisibleActions()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QMenuBar menubar;
    menubar.setNativeMenuBar(false); //we can't check the geometry of native menubars

    QAction * m = menubar.addAction( "&m" );
    QAction * a = menubar.addAction( "&a" );

    centerOnScreen(&menubar);
    menubar.show();
    QApplication::setActiveWindow(&menubar);
    QVERIFY(QTest::qWaitForWindowActive(&menubar));
    menubar.setActiveAction(m);
    QCOMPARE(menubar.activeAction(), m);
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Right);
    QCOMPARE(menubar.activeAction(), a);
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Right);
    QCOMPARE(menubar.activeAction(), m);
    a->setVisible(false);

    menubar.setActiveAction(m);
    QCOMPARE(menubar.activeAction(), m); //the active action shouldn't have changed

    //it used to crash here because the action is invisible
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Right);
    QCOMPARE(menubar.activeAction(), m); //the active action shouldn't have changed
}

void tst_QMenuBar::closeOnSecondClickAndOpenOnThirdClick() // QTBUG-32807, menu should close on 2nd click.
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QMainWindow mainWindow;
    mainWindow.resize(300, 200);
    centerOnScreen(&mainWindow);
    QMenuBar *menuBar = mainWindow.menuBar();
    menuBar->setNativeMenuBar(false);
    QMenu *fileMenu = menuBar->addMenu(QStringLiteral("OpenCloseOpen"));
    fileMenu->addAction(QStringLiteral("Quit"));
    mainWindow.show();
    QApplication::setActiveWindow(&mainWindow);
    QVERIFY(QTest::qWaitForWindowActive(&mainWindow));

    const QPoint center = menuBarActionWindowPos(mainWindow.menuBar(), fileMenu->menuAction());
    const QPoint globalPos = mainWindow.mapToGlobal(center);

    QWindow *window = mainWindow.windowHandle();
    QTest::mouseMove(window, center);
    QTest::mouseClick(window, Qt::LeftButton, {}, center);
    QTRY_VERIFY(fileMenu->isVisible());
    QTest::mouseClick(window, Qt::LeftButton, {}, fileMenu->mapFromGlobal(globalPos));
    QTRY_VERIFY(!fileMenu->isVisible());
    QTest::mouseClick(window, Qt::LeftButton, {}, center);
    QTRY_VERIFY(fileMenu->isVisible());
}

Q_DECLARE_METATYPE(Qt::Corner)

void tst_QMenuBar::cornerWidgets_data()
{
    QTest::addColumn<Qt::Corner>("corner");
    QTest::newRow("left") << Qt::TopLeftCorner;
    QTest::newRow("right") << Qt::TopRightCorner;
}

static QByteArray msgComparison(int v1, const char *op, int v2)
{
    QString result;
    QDebug(&result) << v1 << op << v2 << "failed";
    return result.toLocal8Bit();
}

void tst_QMenuBar::cornerWidgets()
{
    enum { cornerWidgetWidth = 100 };

    QFETCH(Qt::Corner, corner);

#if defined(Q_OS_MACOS)
    QSKIP("Test interferes with native menu bars on this platform");
#endif

    QWidget widget;
    const QString dataTag = QLatin1String(QTest::currentDataTag());
    widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()) + dataTag);
    QVBoxLayout *layout = new QVBoxLayout(&widget);
    QMenuBar *menuBar = new QMenuBar(&widget);
    menuBar->setNativeMenuBar(false);
    layout->addWidget(menuBar);
    QMenu *fileMenu = menuBar->addMenu("File");
    fileMenu->addAction("Quit");
    QMenu *editMenu =menuBar->addMenu("Edit");
    editMenu->addAction("Copy");
    centerOnScreen(&widget);

    QLabel *cornerLabel = new QLabel(dataTag);
    cornerLabel->setFixedWidth(cornerWidgetWidth);
    menuBar->setCornerWidget(cornerLabel, corner);
    QCOMPARE(menuBar->cornerWidget(corner), cornerLabel);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    const QRect fileMenuGeometry = menuBar->actionGeometry(fileMenu->menuAction());
    const QRect editMenuGeometry = menuBar->actionGeometry(editMenu->menuAction());
    const int menuBarWidth = menuBar->width();
    switch (corner) { // QTBUG-36010 , verify corner widget geometry is correct
    case Qt::TopLeftCorner:
        QVERIFY2(fileMenuGeometry.left() >= cornerWidgetWidth,
                 msgComparison(fileMenuGeometry.left(), ">=", cornerWidgetWidth));
#ifdef Q_OS_WINRT
        QEXPECT_FAIL("", "Broken on WinRT - QTBUG-68297", Abort);
#endif
        QVERIFY2(menuBarWidth - editMenuGeometry.right() < cornerWidgetWidth,
                 msgComparison(menuBarWidth - editMenuGeometry.right(), "<", cornerWidgetWidth));
        break;
    case Qt::TopRightCorner:
        QVERIFY2(fileMenuGeometry.left() < cornerWidgetWidth,
                 msgComparison(fileMenuGeometry.left(), "<", cornerWidgetWidth));
        QVERIFY2(menuBarWidth - editMenuGeometry.right() >= cornerWidgetWidth,
                 msgComparison(menuBarWidth - editMenuGeometry.right(), ">=", cornerWidgetWidth));
        break;
    default:
        break;
    }

    menuBar->setCornerWidget(0, corner); // Don't crash.
    QVERIFY(!menuBar->cornerWidget(corner));
    delete cornerLabel;
}


void tst_QMenuBar::taskQTBUG53205_crashReparentNested()
{
    // This test was largely inspired by the test case submitted for the bug
    QMainWindow mainWindow;
    mainWindow.resize(300, 200);
    centerOnScreen(&mainWindow);
    const TestMenu testMenus = initWindowWithComplexMenuBar(mainWindow);
    QApplication::setActiveWindow(&mainWindow);

    // they can't be windows
    QWidget hiddenParent(&mainWindow, {});
    //this one is going to be moved around
    QWidget movingParent(&hiddenParent, {});

    //set up the container widget
    QWidget containerWidget(&movingParent, {});

    //set the new parent, a window
    QScopedPointer<QWidget> windowedParent;
    windowedParent.reset(new QWidget(nullptr, Qt::WindowFlags()));
    windowedParent->setGeometry(400, 10, 300, 300);

    windowedParent->show();
    QVERIFY(QTest::qWaitForWindowExposed(windowedParent.data()));

    //set the "container", can't be a window
    QWidget containedWidget(&containerWidget, {});

    taskQTBUG53205MenuBar = new QMenuBar(&containedWidget);

    connect(testMenus.actions[0], &QAction::triggered, this, &tst_QMenuBar::slotForTaskQTBUG53205);
    //now, move things around
    //from : QMainWindow<-hiddenParent<-movingParent<-containerWidget<-containedWidget<-menuBar
    //to windowedParent<-movingParent<-containerWidget<-containedWidget<-menuBar
    movingParent.setParent(windowedParent.data(), {});
    // this resets the parenting and the menu bar's window
    taskQTBUG53205MenuBar->setParent(nullptr);
    taskQTBUG53205MenuBar->setParent(&containedWidget);
    //from windowedParent<-movingParent<-containerWidget<-containedWidget<-menuBar
    //to : QMainWindow<-hiddenParent<-movingParent<-containerWidget<-containedWidget<-menuBar
    movingParent.setParent(&hiddenParent, {});
    windowedParent.reset(); //make the old window invalid
    // trigger the aciton,  reset the menu bar's window, this used to crash here.
    testMenus.actions[0]->trigger();
}

void tst_QMenuBar::QTBUG_65488_hiddenActionTriggered()
{
    QMainWindow win;
    win.menuBar()->setNativeMenuBar(false);
    QAction *act1 = win.menuBar()->addAction("A very long named action that make menuBar item wide enough");
    QSignalSpy spy(win.menuBar(), &QMenuBar::triggered);

    QRect actRect = win.menuBar()->actionGeometry(act1);
    // resize to action's size to make Action1 hidden
    win.resize(actRect.width() - 10, win.size().height());
    win.show();
    QApplication::setActiveWindow(&win);
    QVERIFY(QTest::qWaitForWindowExposed(&win));
    // click center of the blank area on the menubar where Action1 resided
    QTest::mouseClick(win.windowHandle(), Qt::LeftButton, Qt::NoModifier, win.menuBar()->geometry().center());
    QCoreApplication::sendPostedEvents(); // make sure all queued events also dispatched
    QCOMPARE(spy.count(), 0);
}

// QTBUG-56526
void tst_QMenuBar::platformMenu()
{
    QMenuBar menuBar;
    QPlatformMenuBar *platformMenuBar = menuBar.platformMenuBar();
    if (!platformMenuBar)
        QSKIP("No platform menubar implementation available on this platform.");

    // QMenu must not create a platform menu instance at creation time, because
    // on Unity the type of the platform menu instance must be different (QGtk3Menu
    // vs. QDbusPlatformMenu) depending on whether the menu is in the global menubar
    // or a standalone context menu.
    QMenu *menu = new QMenu(&menuBar);
    QVERIFY(!menu->platformMenu());

    menuBar.addMenu(menu);
    QVERIFY(menu->platformMenu());
}

class TestObject : public QObject
{
    Q_OBJECT
public:
    bool flag = false;
    void setFlag()
    {
        flag = true;
    }
};

void tst_QMenuBar::addActionQt5connect()
{
    bool flag = false;
    auto functor = [&flag](){ flag = true; };

    TestObject obj;

    QMenuBar menuBar;

    auto action1 = menuBar.addAction(QStringLiteral("1"), &obj, &TestObject::setFlag);
    auto action2 = menuBar.addAction(QStringLiteral("2"), functor);

    action1->activate(QAction::Trigger);
    action2->activate(QAction::Trigger);

    QVERIFY(obj.flag);
    QVERIFY(flag);

    flag = false;

    auto action3 = menuBar.addAction(QStringLiteral("3"), this, functor);
    action3->activate(QAction::Trigger);
    QVERIFY(flag);
}

void tst_QMenuBar::QTBUG_25669_menubarActionDoubleTriggered()
{
    QMainWindow win;
    win.menuBar()->setNativeMenuBar(false);
    QAction *act1 = win.menuBar()->addAction("Action1");
    QAction *act2 = win.menuBar()->addAction("Action2");
    QSignalSpy spy(win.menuBar(), &QMenuBar::triggered);

    win.show();
    QApplication::setActiveWindow(&win);
    QVERIFY(QTest::qWaitForWindowExposed(&win));

    QPoint posAct1 = menuBarActionWindowPos(win.menuBar(), act1);
    QPoint posAct2 = menuBarActionWindowPos(win.menuBar(), act2);

    QTest::mouseClick(win.windowHandle(), Qt::LeftButton, Qt::NoModifier, posAct1);
    QTRY_COMPARE(spy.count(), 1);

    QTest::mouseClick(win.windowHandle(), Qt::LeftButton, Qt::NoModifier, posAct2);
    QTRY_COMPARE(spy.count(), 2);

    QTest::mouseClick(win.windowHandle(), Qt::LeftButton, Qt::NoModifier, posAct2);
    QTRY_COMPARE(spy.count(), 3);
}

void tst_QMenuBar::slotForTaskQTBUG53205()
{
    QWidget *parent = taskQTBUG53205MenuBar->parentWidget();
    taskQTBUG53205MenuBar->setParent(nullptr);
    taskQTBUG53205MenuBar->setParent(parent);
}

// Qt/Mac does not use the native popups/menubar
#if !defined(Q_OS_DARWIN)
void tst_QMenuBar::taskQTBUG46812_doNotLeaveMenubarHighlighted()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QMainWindow mainWindow;
    QWidget *centralWidget = new QWidget;
    centralWidget->setFocusPolicy(Qt::StrongFocus);
    mainWindow.setCentralWidget(centralWidget);
    initWindowWithSimpleMenuBar(mainWindow);

    mainWindow.show();
    QApplication::setActiveWindow(&mainWindow);
    QVERIFY(QTest::qWaitForWindowActive(&mainWindow));

    QVERIFY(!mainWindow.menuBar()->hasFocus());
    QCOMPARE(m_simpleActivatedCount, 0);

    QTest::keyPress(&mainWindow, Qt::Key_Alt, Qt::AltModifier);
    QVERIFY(!mainWindow.menuBar()->hasFocus());
    QCOMPARE(m_simpleActivatedCount, 0);

    QTest::keyPress(&mainWindow, Qt::Key_Z, Qt::AltModifier);
    QVERIFY(!mainWindow.menuBar()->hasFocus());
    QCOMPARE(m_simpleActivatedCount, 2); // the action AND the menu will activate

    QTest::keyRelease(&mainWindow, Qt::Key_Alt, Qt::NoModifier);
    QVERIFY(!mainWindow.menuBar()->hasFocus());
    QCOMPARE(m_simpleActivatedCount, 2);

    QTest::keyRelease(&mainWindow, Qt::Key_Z, Qt::NoModifier);
    QVERIFY(!mainWindow.menuBar()->hasFocus());
    QCOMPARE(m_simpleActivatedCount, 2);
}
#endif

#ifdef Q_OS_MACOS
extern bool tst_qmenubar_taskQTBUG56275(QMenuBar *);

void tst_QMenuBar::taskQTBUG56275_reinsertMenuInParentlessQMenuBar()
{
    QMenuBar menubar;

    QMenu *menu = new QMenu("menu", &menubar);
    QMenu* submenu = menu->addMenu("submenu");
    submenu->addAction("action1");
    submenu->addAction("action2");
    menu->addAction("action3");
    menubar.addMenu(menu);

    QTest::qWait(100);
    menubar.clear();
    menubar.addMenu(menu);
    QTest::qWait(100);

    QVERIFY(tst_qmenubar_taskQTBUG56275(&menubar));
}

void tst_QMenuBar::QTBUG_57404_existingMenuItemException()
{
    QMainWindow mw1;
    QMainWindow mw2;
    mw1.show();
    mw2.show();

    QMenuBar *mb = new QMenuBar(&mw1);
    mw1.setMenuBar(mb);
    mb->show();
    QMenu *editMenu = new QMenu(QLatin1String("Edit"), &mw1);
    mb->addMenu(editMenu);
    QAction *copyAction = editMenu->addAction("&Copy");
    copyAction->setShortcut(QKeySequence("Ctrl+C"));
    copyAction->setMenuRole(QAction::NoRole);

    QVERIFY(QTest::qWaitForWindowExposed(&mw2));
    QTest::qWait(100);
    mw2.close();
    mw1.activateWindow();
    QTest::qWait(100);
    // No crash, all fine. Ideally, there should be only one warning.
}
#endif // Q_OS_MACOS

void tst_QMenuBar::taskQTBUG55966_subMenuRemoved()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QMainWindow window;
    QMenuBar *menubar = window.menuBar();
    QMenu *parentMenu = menubar->addMenu("Parent menu");

    QAction *action = parentMenu->addAction("Action in parent menu");
    QMenu *subMenu = new QMenu("Submenu");
    action->setMenu(subMenu);
    delete subMenu;

    window.show();
    QApplication::setActiveWindow(&window);
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QTest::qWait(500);
}

void tst_QMenuBar::QTBUG_58344_invalidIcon()
{
    QMenuBar menuBar;
    QMenu menu("menu");
    menu.addAction(QIcon("crash.png"), "crash");
    menuBar.addMenu(&menu);
    // No crash, all fine.
}

QTEST_MAIN(tst_QMenuBar)
#include "tst_qmenubar.moc"
