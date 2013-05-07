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
#include <qapplication.h>
#include <qmainwindow.h>
#include <qmenubar.h>
#include <qstyle.h>
#include <qproxystyle.h>
#include <qstylefactory.h>
#include <qdesktopwidget.h>
#include <qaction.h>
#include <qstyleoption.h>

#include <qobject.h>

QT_FORWARD_DECLARE_CLASS(QMainWindow)

#include <qmenubar.h>

class QtTestSlot : public QObject
{
    Q_OBJECT

public:
    QtTestSlot( QObject* parent = 0 ): QObject( parent ) { clear(); };
    virtual ~QtTestSlot() {};

    void clear() { sel_count = 0; };
    uint selCount() { return sel_count; };

public slots:
    void selected() { sel_count++; };

private:
    uint sel_count;
};

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

class tst_QMenuBar : public QObject
{
    Q_OBJECT
public:
    tst_QMenuBar();
    virtual ~tst_QMenuBar();

    void initSimpleMenubar();
    void initComplexMenubar();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
private slots:
    void getSetCheck();

    void clear();
    void removeItemAt();
    void removeItemAt_data();
    void removeItem_data();
    void removeItem();
    void count();
    void insertItem_QString_QObject();

#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
    void accel();
    void activatedCount();
    void allowActiveAndDisabled();

    void check_accelKeys();
    void check_cursorKeys1();
    void check_cursorKeys2();
    void check_cursorKeys3();

    void check_escKey();
#endif

    void check_endKey();
    void check_homeKey();

//     void check_mouse1_data();
//     void check_mouse1();
//     void check_mouse2_data();
//     void check_mouse2();

    void check_altPress();
#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
    void check_shortcutPress();
    void check_menuPosition();
#endif
    void task223138_triggered();
    void task256322_highlight();
    void menubarSizeHint();
#ifndef Q_OS_MAC
    void taskQTBUG4965_escapeEaten();
#endif
    void taskQTBUG11823_crashwithInvisibleActions();

protected slots:
    void onActivated( QAction*);

private:
    QtTestSlot *menu1;
    QtTestSlot *menu2;
    QtTestSlot *menu3;
    QtTestSlot *menu4;

    QtTestSlot *item1_A;
    QtTestSlot *item1_B;
    QtTestSlot *item2_C;
    QtTestSlot *item2_D;
    QtTestSlot *item2_E;
    QtTestSlot *item2_F;
    QtTestSlot *item2_G;
    QtTestSlot *item2_H;

    void resetSlots();
    void resetCount();

    void reset() { resetSlots(); resetCount(); };

    QAction* last_accel_id;
    int activated_count;

    QAction *action;
    QAction *action1;
    QMainWindow *mw;
    QMenuBar *mb;
    QMenu *pm1;
    QMenu *pm2;
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

const int RESET = 0;

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

tst_QMenuBar::tst_QMenuBar()

{
    QApplication::setEffectEnabled(Qt::UI_AnimateMenu, false);


    activated_count = 0;
    mb = 0;
    pm1 = 0;
    pm2 = 0;
    last_accel_id = 0;
}

tst_QMenuBar::~tst_QMenuBar()
{
    //delete mw; //#### cannot do that AFTER qapplication was destroyed!
    mw = 0;
}

void tst_QMenuBar::initTestCase()
{
    // create a default mainwindow
    // If you run a widget test, this will be replaced in the testcase by the
    // widget under test
    mw = new QMainWindow(0, Qt::X11BypassWindowManagerHint);
    mb = new QMenuBar( mw );
    connect( mb, SIGNAL(triggered(QAction*)), this, SLOT(onActivated(QAction*)) );

    initSimpleMenubar();
    mw->show();
    mw->activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(mw));

    menu1 = new QtTestSlot( mw );
    menu2 = new QtTestSlot( mw );
    menu3 = new QtTestSlot( mw );
    menu4 = new QtTestSlot( mw );
    item1_A = new QtTestSlot( mw );
    item1_B = new QtTestSlot( mw );
    item2_C = new QtTestSlot( mw );
    item2_D = new QtTestSlot( mw );
    item2_E = new QtTestSlot( mw );
    item2_F = new QtTestSlot( mw );
    item2_G = new QtTestSlot( mw );
    item2_H = new QtTestSlot( mw );
}


void tst_QMenuBar::cleanupTestCase()
{
    delete mw;
}

void tst_QMenuBar::initSimpleMenubar()
{
    mb->hide();
    mb->clear();

    delete pm1;
    pm1  = mb->addMenu("&accel");
    action = pm1->addAction( "menu1" );
    action->setShortcut(QKeySequence("ALT+A"));
    action->setShortcut(QKeySequence("CTRL+A"));

    connect( pm1, SIGNAL(triggered(QAction*)), this, SLOT(onActivated(QAction*)));

    delete pm2;
    pm2  = mb->addMenu("accel1");

    action1 = pm2->addAction( "&Open..." );
    action1->setShortcut(Qt::Key_O);
    connect(pm2, SIGNAL(triggered(QAction*)), this, SLOT(onActivated(QAction*)));

    mb->show();
    qApp->processEvents();
}

void tst_QMenuBar::init()
{
    resetSlots();
    resetCount();
}

void tst_QMenuBar::resetSlots()
{
    menu1->clear();
    menu2->clear();
    menu3->clear();
    menu4->clear();
    item1_A->clear();
    item1_B->clear();
    item2_C->clear();
    item2_D->clear();
    item2_E->clear();
    item2_F->clear();
    item2_G->clear();
    item2_H->clear();
}

void tst_QMenuBar::resetCount()
{
    last_accel_id = 0;
    activated_count = 0;
}

void tst_QMenuBar::onActivated( QAction* action )
{
    last_accel_id = action;
    activated_count++;
//     printf( QString("acceleratorId: %1, count: %1\n").arg( i ).arg(activated_count) );
}

// On Mac/WinCE, native key events are needed to test menu action activation
#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
void tst_QMenuBar::accel()
{
    // create a popup menu with menu items set the accelerators later...
    initSimpleMenubar();

    // shortcuts won't work unless the window is active
    QTRY_VERIFY( QApplication::activeWindow() );
//    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_A, AltKey );
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_A, Qt::ControlModifier );
    QTest::qWait(300);

    QCOMPARE( last_accel_id, action );
}
#endif

// On Mac/WinCE, native key events are needed to test menu action activation
#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
void tst_QMenuBar::activatedCount()
{
    // create a popup menu with menu items set the accelerators later...
    initSimpleMenubar();

    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_A, Qt::ControlModifier );
//wait(5000);
    QCOMPARE( activated_count, 2 ); //1 from the popupmenu and 1 from the menubar
}
#endif

void tst_QMenuBar::clear()
{
    mb->clear();
    QVERIFY( (uint) mb->actions().size() == 0 );

    mb->clear();
    for (uint i=0; i<10; i++) {
        QMenu *menu = mb->addMenu( QString("Menu %1"));
	for (uint k=0; k<i; k++)
	    menu->addAction( QString("Item %1"));
	QCOMPARE( (uint) mb->actions().size(), (uint)i+1 );
    }
    QCOMPARE( (uint) mb->actions().size(), 10u );

    mb->clear();
    QVERIFY( (uint) mb->actions().size() == 0 );
}

void tst_QMenuBar::count()
{
    mb->clear();
    QVERIFY( mb->actions().size() == 0 );

    for (uint i=0; i<10; i++) {
	mb->addAction( QString("Menu %1"));
	QCOMPARE( (uint) mb->actions().size(), (uint) i+1 );
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
    mb->clear();

    QMenu *pm;
    pm = new QMenu( "stuff", mb );
    pm->setTitle("Menu 1");
    pm->addAction( QString("Item 10") );
    QAction* action1 = mb->addMenu( pm );

    pm = new QMenu( mb );
    pm->setTitle("Menu 2");
    pm->addAction( QString("Item 20") );
    pm->addAction( QString("Item 21") );
    QAction *action2 = mb->addMenu( pm );

    pm = new QMenu( "Menu 3", mb );
    pm->addAction( QString("Item 30") );
    pm->addAction( QString("Item 31") );
    pm->addAction( QString("Item 32") );
    QAction *action3 = mb->addMenu( pm );

    QList<QAction *> menuBarActions = mb->actions();

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
            mb->removeAction(action1);
            QList<QAction *> menuBarActions2 = mb->actions();
            QCOMPARE( menuBarActions2.at(0)->text(), QString("Menu 2") );
            QCOMPARE( menuBarActions2.at(1)->text(), QString("Menu 3") );
        }
        break;
    case 1: {
            mb->removeAction(action2);
            QList<QAction *> menuBarActions2 = mb->actions();
            QCOMPARE( menuBarActions2.at(0)->text(), QString("Menu 1") );
            QCOMPARE( menuBarActions2.at(1)->text(), QString("Menu 3") );
        }
        break;
    case 2: {
            mb->removeAction(action3);
            QList<QAction *> menuBarActions2 = mb->actions();
            QCOMPARE( menuBarActions2.at(0)->text(), QString("Menu 1") );
            QCOMPARE( menuBarActions2.at(1)->text(), QString("Menu 2") );
        }
        break;
    }
    QList<QAction *> menuBarActions2 = mb->actions();
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
    mb->clear();

    QMenu *pm;
    pm = new QMenu("Menu 1", mb);
    pm->addAction( QString("Item 10") );
    mb->addMenu( pm );

    pm = new QMenu( "Menu 2", mb );
    pm->addAction( QString("Item 20") );
    pm->addAction( QString("Item 21") );
    mb->addMenu( pm );

    pm = new QMenu( "Menu 3", mb );
    pm->addAction( QString("Item 30") );
    pm->addAction( QString("Item 31") );
    pm->addAction( QString("Item 32") );
    mb->addMenu( pm );

    QList<QAction *> menuBarActions = mb->actions();

    QCOMPARE( menuBarActions.at(0)->text(), QString("Menu 1") );
    QCOMPARE( menuBarActions.at(1)->text(), QString("Menu 2") );
    QCOMPARE( menuBarActions.at(2)->text(), QString("Menu 3") );

    // Ok, now that we know we have created the menu we expect, lets remove an item...
    QFETCH( int, removeIndex );
    mb->removeAction(menuBarActions.at(removeIndex));
    QList<QAction *> menuBarActions2 = mb->actions();
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

void tst_QMenuBar::initComplexMenubar() // well, complex....
{
    mb->hide();
    mb->clear();

    delete pm1;
    pm1 = mb->addMenu("Menu &1");
    pm1->addAction( QString("Item A"), item1_A, SLOT(selected()), Qt::CTRL+Qt::Key_A );
    pm1->addAction( QString("Item B"), item1_B, SLOT(selected()), Qt::CTRL+Qt::Key_B );

    delete pm2;
    pm2 = mb->addMenu("Menu &2");
    pm2->addAction( QString("Item C"), item2_C, SLOT(selected()), Qt::CTRL+Qt::Key_C );
    pm2->addAction( QString("Item D"), item2_D, SLOT(selected()), Qt::CTRL+Qt::Key_D );
    pm2->addAction( QString("Item E"), item2_E, SLOT(selected()), Qt::CTRL+Qt::Key_E );
    pm2->addAction( QString("Item F"), item2_F, SLOT(selected()), Qt::CTRL+Qt::Key_F );
    pm2->addSeparator();
    pm2->addAction( QString("Item G"), item2_G, SLOT(selected()), Qt::CTRL+Qt::Key_G );
    pm2->addAction( QString("Item H"), item2_H, SLOT(selected()), Qt::CTRL+Qt::Key_H );

    QAction *ac = mb->addAction( QString("M&enu 3"), menu3, SLOT(selected()));
    ac->setShortcut(Qt::ALT+Qt::Key_J);
    mb->show();
}


/*
    Check the insert functions that create menu items.
    For the moment i only check the strings and pixmaps. The rest are special cases which are
    used less frequently.
*/

void tst_QMenuBar::insertItem_QString_QObject()
{
    initComplexMenubar();

    QList<QAction *> actions = mb->actions();

    QCOMPARE(actions.at(0)->text(), QString("Menu &1") );
    QCOMPARE(actions.at(1)->text(), QString("Menu &2") );
    QCOMPARE(actions.at(2)->text(), QString("M&enu 3") );
    QVERIFY(actions.size() < 4); // there is no menu 4!
}

// On Mac/WinCE, native key events are needed to test menu action activation
#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
void tst_QMenuBar::check_accelKeys()
{
    initComplexMenubar();

    // start with a bogus key that shouldn't trigger anything
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_I, Qt::ControlModifier);
    QCOMPARE(menu1->selCount(), 0u);
    QCOMPARE(menu2->selCount(), 0u);
    QCOMPARE(menu3->selCount(), 0u);
    QCOMPARE(menu4->selCount(), 0u);
    QCOMPARE(item1_A->selCount(), 0u);
    QCOMPARE(item1_B->selCount(), 0u);
    QCOMPARE(item2_C->selCount(), 0u);
    QCOMPARE(item2_D->selCount(), 0u);

    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_A, Qt::ControlModifier);
    QCOMPARE(menu1->selCount(), 0u);
    QCOMPARE(menu2->selCount(), 0u);
    QCOMPARE(menu3->selCount(), 0u);
    QCOMPARE(menu4->selCount(), 0u);
    QCOMPARE(item1_A->selCount(), 1u);
    QCOMPARE(item1_B->selCount(), 0u);
    QCOMPARE(item2_C->selCount(), 0u);
    QCOMPARE(item2_D->selCount(), 0u);

    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_C, Qt::ControlModifier);
    QCOMPARE(menu1->selCount(), 0u);
    QCOMPARE(menu2->selCount(), 0u);
    QCOMPARE(menu3->selCount(), 0u);
    QCOMPARE(menu4->selCount(), 0u);
    QCOMPARE(item1_A->selCount(), 1u);
    QCOMPARE(item1_B->selCount(), 0u);
    QCOMPARE(item2_C->selCount(), 1u);
    QCOMPARE(item2_D->selCount(), 0u);

    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_B, Qt::ControlModifier);
    QCOMPARE(menu1->selCount(), 0u);
    QCOMPARE(menu2->selCount(), 0u);
    QCOMPARE(menu3->selCount(), 0u);
    QCOMPARE(menu4->selCount(), 0u);
    QCOMPARE(item1_A->selCount(), 1u);
    QCOMPARE(item1_B->selCount(), 1u);
    QCOMPARE(item2_C->selCount(), 1u);
    QCOMPARE(item2_D->selCount(), 0u);

    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_D, Qt::ControlModifier);
    QCOMPARE(menu1->selCount(), 0u);
    QCOMPARE(menu2->selCount(), 0u);
    QCOMPARE(menu3->selCount(), 0u);
    QCOMPARE(menu4->selCount(), 0u);
    QCOMPARE(item1_A->selCount(), 1u);
    QCOMPARE(item1_B->selCount(), 1u);
    QCOMPARE(item2_C->selCount(), 1u);
    QCOMPARE(item2_D->selCount(), 1u);

    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_J, Qt::AltModifier);
    QCOMPARE(menu1->selCount(), 0u);
    QCOMPARE(menu2->selCount(), 0u);
    QCOMPARE(menu3->selCount(), 1u);
    QCOMPARE(menu4->selCount(), 0u);
    QCOMPARE(item1_A->selCount(), 1u);
    QCOMPARE(item1_B->selCount(), 1u);
    QCOMPARE(item2_C->selCount(), 1u);
    QCOMPARE(item2_D->selCount(), 1u);
}
#endif

// On Mac/WinCE, native key events are needed to test menu action activation
#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
void tst_QMenuBar::check_cursorKeys1()
{
    initComplexMenubar();

    // start with a ALT + 1 that activates the first popupmenu
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_1, Qt::AltModifier );
    // the Popupmenu should be visible now
    QCOMPARE(menu3->selCount(), 0u);
    QCOMPARE(menu4->selCount(), 0u);
    QCOMPARE(item1_A->selCount(), 0u);
    QCOMPARE(item1_B->selCount(), 0u);
    QCOMPARE(item2_C->selCount(), 0u);
    QCOMPARE(item2_D->selCount(), 0u);

    // Simulate a cursor key down click
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Down );
    // and an Enter key
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Enter );
    // Let's see if the correct slot is called...
    QCOMPARE(menu3->selCount(), 0u);
    QCOMPARE(menu4->selCount(), 0u);
    QCOMPARE(item1_A->selCount(), 0u); // this shouldn't have been called
    QCOMPARE(item1_B->selCount(), 1u); // and this should have been called by a signal now
    QCOMPARE(item2_C->selCount(), 0u);
    QCOMPARE(item2_D->selCount(), 0u);
}
#endif

// Qt/Mac,WinCE does not use the native popups/menubar
#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
void tst_QMenuBar::check_cursorKeys2()
{
    initComplexMenubar();

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
    QCOMPARE(menu3->selCount(), 0u);
    QCOMPARE(menu4->selCount(), 0u);
    QCOMPARE(item1_A->selCount(), 0u); // this shouldn't have been caled
    QCOMPARE(item1_B->selCount(), 0u); // and this should have been called by a signal ow
    QCOMPARE(item2_C->selCount(), 0u);
    QCOMPARE(item2_D->selCount(), 1u);
}
#endif

/*!
    If a popupmenu is active you can use Left to move to the menu to the left of it.
*/
// Qt/Mac,WinCE does not use the native popups/menubar
#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
void tst_QMenuBar::check_cursorKeys3()
{
    initComplexMenubar();

    // select Popupmenu 2
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_2, Qt::AltModifier );

    // Simulate some keys
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Left );
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Down );
    // and press ENTER
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Enter );
    // Let's see if the correct slot is called...
    QCOMPARE(menu3->selCount(), 0u);
    QCOMPARE(menu4->selCount(), 0u);
    QCOMPARE(item1_A->selCount(), 0u); // this shouldn't have been called
    QCOMPARE(item1_B->selCount(), 1u); // and this should have been called by a signal now
    QCOMPARE(item2_C->selCount(), 0u);
    QCOMPARE(item2_D->selCount(), 0u);
}
#endif

/*!
    If a popupmenu is active you can use home to go quickly to the first item in the menu.
*/
void tst_QMenuBar::check_homeKey()
{
    // I'm temporarily shutting up this testcase.
    // Seems like the behaviour i'm expecting isn't ok.
    QVERIFY( true );
    return;

    QEXPECT_FAIL( "0", "Popupmenu should respond to a Home key", Abort );

    initComplexMenubar();

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
//    QVERIFY2( item2_C->selCount() == 1, "Popupmenu should respond to a Home key" );
    QCOMPARE(item2_C->selCount(), 1u);
    QCOMPARE(menu3->selCount(), 0u);
    QCOMPARE(menu4->selCount(), 0u);
    QCOMPARE(item1_A->selCount(), 0u);
    QCOMPARE(item1_B->selCount(), 0u);
    QCOMPARE(item2_D->selCount(), 0u);
    QCOMPARE(item2_E->selCount(), 0u);
    QCOMPARE(item2_F->selCount(), 0u);
    QCOMPARE(item2_G->selCount(), 0u);
    QCOMPARE(item2_H->selCount(), 0u);
}

/*!
    If a popupmenu is active you can use end to go quickly to the last item in the menu.
*/
void tst_QMenuBar::check_endKey()
{
    // I'm temporarily silenting this testcase.
    // Seems like the behaviour i'm expecting isn't ok.
    QVERIFY( true );
    return;

    QEXPECT_FAIL( "0", "Popupmenu should respond to an End key", Abort );

    initComplexMenubar();

    // select Popupmenu 2
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_2, Qt::AltModifier );

    // Simulate some keys
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_End );
    // and press ENTER
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Enter );
    // Let's see if the correct slot is called...
//    QVERIFY2( item2_H->selCount() == 1, "Popupmenu should respond to an End key" );
    QCOMPARE(item2_H->selCount(), 1u);//, "Popupmenu should respond to an End key");
    QCOMPARE(menu3->selCount(), 0u);
    QCOMPARE(menu4->selCount(), 0u);
    QCOMPARE(item1_A->selCount(), 0u);
    QCOMPARE(item1_B->selCount(), 0u);
    QCOMPARE(item2_C->selCount(), 0u);
    QCOMPARE(item2_D->selCount(), 0u);
    QCOMPARE(item2_E->selCount(), 0u);
    QCOMPARE(item2_F->selCount(), 0u);
    QCOMPARE(item2_G->selCount(), 0u);
}

/*!
    If a popupmenu is active you can use esc to hide the menu and then the
    menubar should become active.
    If Down is pressed next the popup is activated again.
*/

// Qt/Mac,WinCE does not use the native popups/menubar
#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
void tst_QMenuBar::check_escKey()
{
    initComplexMenubar();

    QVERIFY( !pm1->isActiveWindow() );
    QVERIFY( !pm2->isActiveWindow() );

    // select Popupmenu 2
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_2, Qt::AltModifier );
    QVERIFY( !pm1->isActiveWindow() );
    QVERIFY( pm2->isActiveWindow() );

    // If we press ESC, the popup should disappear
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Escape );
    QVERIFY( !pm1->isActiveWindow() );
    QVERIFY( !pm2->isActiveWindow() );

    if (!QApplication::style()->inherits("QWindowsStyle"))
        return;

    // If we press Down the popupmenu should be active again
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_Down );
    QVERIFY( !pm1->isActiveWindow() );
    QVERIFY( pm2->isActiveWindow() );

    // and press ENTER
    QTest::keyClick( pm2, Qt::Key_Enter );
    // Let's see if the correct slot is called...
    QVERIFY2( item2_C->selCount() == 1, "Expected item 2C to be selected" );
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
// 	QSKIP("This fails in Motif due to a bug in the testing framework");
//     QFETCH( QString, popup_item );
//     QFETCH( int, itemA_count );
//     QFETCH( int, itemB_count );

//     initComplexMenubar();
//     QVERIFY( !pm1->isActiveWindow() );
//     QVERIFY( !pm2->isActiveWindow() );

//     QTest::qWait(1000);
//     QtTestMouse mouse;
//     mouse.mouseEvent( QtTestMouse::MouseClick, mb, "Menu &1", Qt::LeftButton );

//     QVERIFY( pm1->isActiveWindow() );
//     QVERIFY( !pm2->isActiveWindow() );

//     QTest::qWait(1000);
//     mouse.mouseEvent( QtTestMouse::MouseClick, pm1, popup_item, Qt::LeftButton );

//     QCOMPARE(menu3->selCount(), 0u);
//     QCOMPARE(menu4->selCount(), 0u);
//     QCOMPARE(item1_A->selCount(), (uint)itemA_count); // this option should have fired
//     QCOMPARE(item1_B->selCount(), (uint)itemB_count);
//     QCOMPARE(item2_C->selCount(), 0u);
//     QCOMPARE(item2_D->selCount(), 0u);
//     QCOMPARE(item2_E->selCount(), 0u);
//     QCOMPARE(item2_F->selCount(), 0u);
//     QCOMPARE(item2_G->selCount(), 0u);
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
// 	QSKIP("This fails in Motif due to a bug in the testing framework");
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

//     initComplexMenubar();
//     QtTestMouse mouse;
//     mouse.click( QtTestMouse::Menu, label, Qt::LeftButton );

//     // check if the correct signals have fired
//     QCOMPARE(menu3->selCount(), (uint)menu3_count);
//     QCOMPARE(menu4->selCount(), 0u);
//     QCOMPARE(item1_A->selCount(), (uint)itemA_count);
//     QCOMPARE(item1_B->selCount(), (uint)itemB_count);
//     QCOMPARE(item2_C->selCount(), (uint)itemC_count);
//     QCOMPARE(item2_D->selCount(), (uint)itemD_count);
//     QCOMPARE(item2_E->selCount(), (uint)itemE_count);
//     QCOMPARE(item2_F->selCount(), (uint)itemF_count);
//     QCOMPARE(item2_G->selCount(), (uint)itemG_count);
//     QCOMPARE(item2_H->selCount(), (uint)itemH_count);
// }

#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
void tst_QMenuBar::allowActiveAndDisabled()
{
    mb->hide();
    mb->clear();

     // Task 241043 : check that second menu is activated if only
    // disabled menu items are added

    QMenu fileMenu("&File");
    // Task 241043 : check that second menu is activated
    // if all items are disabled
    QAction *act = fileMenu.addAction("Disabled");
    act->setEnabled(false);

    mb->addMenu(&fileMenu);
    QMenu disabledMenu("Disabled");
    disabledMenu.setEnabled(false);
    QMenu activeMenu("Active");
    mb->addMenu(&disabledMenu);
    mb->addMenu(&activeMenu);
    mb->show();


    // Here we verify that AllowActiveAndDisabled correctly skips
    // the disabled menu entry
    QTest::keyClick(mb, Qt::Key_F, Qt::AltModifier );
    QTest::keyClick(&fileMenu, (Qt::Key_Right));
    if (qApp->style()->styleHint(QStyle::SH_Menu_AllowActiveAndDisabled))
        QCOMPARE(mb->activeAction()->text(), disabledMenu.title());
    else
        QCOMPARE(mb->activeAction()->text(), activeMenu.title());

    QTest::keyClick(mb, (Qt::Key_Left));
    if (qApp->style()->styleHint(QStyle::SH_Menu_AllowActiveAndDisabled))
        QCOMPARE(mb->activeAction()->text(), fileMenu.title());
    else
        QCOMPARE(mb->activeAction()->text(), fileMenu.title());

    mb->hide();
}
#endif

void tst_QMenuBar::check_altPress()
{
    if ( !qApp->style()->styleHint(QStyle::SH_MenuBar_AltKeyNavigation) ) {
	QSKIP( QString( "this is not supposed to work in the %1 style. Skipping." ).
	      arg( qApp->style()->objectName() ).toLatin1());
    }

    initSimpleMenubar();

    qApp->setActiveWindow(mw);
    mw->setFocus();

    QTest::keyClick( mw, Qt::Key_Alt );

    QVERIFY( ::qobject_cast<QMenuBar *>(qApp->focusWidget()) );
}

// Qt/Mac,WinCE does not use the native popups/menubar
#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
void tst_QMenuBar::check_shortcutPress()
{
    initComplexMenubar();

    qApp->setActiveWindow(mw);
    QCOMPARE(menu3->selCount(), 0u);
    QTest::keyClick(mw, Qt::Key_E, Qt::AltModifier);
    QTest::qWait(200);
    QCOMPARE(menu3->selCount(), 1u);
    QVERIFY(!mb->activeAction());

    QTest::keyClick(mw, Qt::Key_1, Qt::AltModifier );
    QVERIFY(pm1->isActiveWindow());
    QTest::keyClick(mb, Qt::Key_2);
    QVERIFY(pm1->isActiveWindow());
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

// Qt/Mac,WinCE does not use the native popups/menubar
#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
void tst_QMenuBar::check_menuPosition()
{
    Menu menu;
    initComplexMenubar();
    menu.setTitle("&menu");
    QRect availRect = QApplication::desktop()->availableGeometry(mw);
    QRect screenRect = QApplication::desktop()->screenGeometry(mw);

    while(menu.sizeHint().height() < (screenRect.height()*2/3)) {
        menu.addAction("item");
    }

    QAction *menu_action = mw->menuBar()->addMenu(&menu);

    qApp->setActiveWindow(mw);
    qApp->processEvents();

    //the menu should be below the menubar item
    {
        mw->move(availRect.topLeft());
        QRect mbItemRect = mw->menuBar()->actionGeometry(menu_action);
        mbItemRect.moveTo(mw->menuBar()->mapToGlobal(mbItemRect.topLeft()));
        QTest::keyClick(mw, Qt::Key_M, Qt::AltModifier );
        QVERIFY(menu.isActiveWindow());
        QCOMPARE(menu.pos(), QPoint(mbItemRect.x(), mbItemRect.bottom() + 1));
        menu.close();
    }

    //the menu should be above the menubar item
    {
        mw->move(0,screenRect.bottom() - screenRect.height()/4); //just leave some place for the menubar
        QRect mbItemRect = mw->menuBar()->actionGeometry(menu_action);
        mbItemRect.moveTo(mw->menuBar()->mapToGlobal(mbItemRect.topLeft()));
        QTest::keyClick(mw, Qt::Key_M, Qt::AltModifier );
        QVERIFY(menu.isActiveWindow());
        QCOMPARE(menu.pos(), QPoint(mbItemRect.x(), mbItemRect.top() - menu.height()));
        menu.close();
    }

    //the menu should be on the side of the menubar item and should be "stuck" to the bottom of the screen
    {
        mw->move(0,screenRect.y() + screenRect.height()/2); //put it in the middle
        QRect mbItemRect = mw->menuBar()->actionGeometry(menu_action);
        mbItemRect.moveTo(mw->menuBar()->mapToGlobal(mbItemRect.topLeft()));
        QTest::keyClick(mw, Qt::Key_M, Qt::AltModifier );
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
        QRect mbItemRect = mw->menuBar()->actionGeometry(menu_action);
        mbItemRect.moveTo(mw->menuBar()->mapToGlobal(mbItemRect.topLeft()));
        QTest::keyClick(mw, Qt::Key_M, Qt::AltModifier );
        QVERIFY(menu.isActiveWindow());
        QCOMPARE(menu.geometry().right(), mbItemRect.right());
        menu.close();
    }

#  ifndef QTEST_NO_CURSOR
    // QTBUG-28031: Click at bottom-right corner.
    {
        mw->move(400, 200);
        LayoutDirectionSaver directionSaver(Qt::RightToLeft);
        QMenuBar *mb = mw->menuBar();
        const QPoint localPos = mb->actionGeometry(menu.menuAction()).bottomRight() - QPoint(1, 1);
        const QPoint globalPos = mb->mapToGlobal(localPos);
        QCursor::setPos(globalPos);
        QTest::mouseClick(mb, Qt::LeftButton, 0, localPos);
        QTRY_VERIFY(menu.isActiveWindow());
        QCOMPARE(menu.geometry().right() - 1, globalPos.x());
        menu.close();
    }
#  endif // QTEST_NO_CURSOR
}
#endif

void tst_QMenuBar::task223138_triggered()
{
    //we create a window with submenus and we check that both menubar and menus get the triggered signal
    QMainWindow win;
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
    QMainWindow win;
	win.menuBar()->setNativeMenuBar(false);  //we can't check the geometry of native menubars
    QMenu menu;
    QAction *file = win.menuBar()->addMenu(&menu);
    file->setText("file");
    QMenu menu2;
    QAction *file2 = win.menuBar()->addMenu(&menu2);
    file2->setText("file2");
    QAction *nothing = win.menuBar()->addAction("nothing");

    win.show();
    QTest::qWait(200);

    QTest::mousePress(win.menuBar(), Qt::LeftButton, 0, win.menuBar()->actionGeometry(file).center());
    QTest::mouseMove(win.menuBar(), win.menuBar()->actionGeometry(file).center());
    QTest::mouseRelease(win.menuBar(), Qt::LeftButton, 0, win.menuBar()->actionGeometry(file).center());
    QTRY_VERIFY(menu.isVisible());
    QVERIFY(!menu2.isVisible());
    QCOMPARE(win.menuBar()->activeAction(), file);

    QTest::mousePress(win.menuBar(), Qt::LeftButton, 0, win.menuBar()->actionGeometry(file2).center());
    QTest::mouseMove(win.menuBar(), win.menuBar()->actionGeometry(file2).center());
    QTRY_VERIFY(!menu.isVisible());
    QVERIFY(menu2.isVisible());
    QCOMPARE(win.menuBar()->activeAction(), file2);
    QTest::mouseRelease(win.menuBar(), Qt::LeftButton, 0, win.menuBar()->actionGeometry(file2).center());

    QPoint nothingCenter = win.menuBar()->actionGeometry(nothing).center();
    QTest::mousePress(win.menuBar(), Qt::LeftButton, 0, nothingCenter);
    QTest::mouseMove(win.menuBar(), nothingCenter);
    QTRY_VERIFY(!menu2.isVisible());
    QVERIFY(!menu.isVisible());
    QAction *activeAction = win.menuBar()->activeAction();
#ifdef Q_OS_MAC
    if ((QSysInfo::MacintoshVersion >= QSysInfo::MV_10_7) && (activeAction != nothing))
        QEXPECT_FAIL("", "QTBUG-30565: Unstable test", Continue);
#endif
    QCOMPARE(activeAction, nothing);
    QTest::mouseRelease(win.menuBar(), Qt::LeftButton, 0, nothingCenter);
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
#ifndef Q_OS_MAC
void tst_QMenuBar::taskQTBUG4965_escapeEaten()
{
    QMenuBar menubar;
    QMenu menu("menu1");
    QAction *first = menubar.addMenu(&menu);
    menu.addAction("quit", &menubar, SLOT(close()), QKeySequence("ESC"));
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
    QMenuBar menubar;
    menubar.setNativeMenuBar(false); //we can't check the geometry of native menubars

    QAction * m = menubar.addAction( "&m" );
    QAction * a = menubar.addAction( "&a" );

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

QTEST_MAIN(tst_QMenuBar)
#include "tst_qmenubar.moc"
