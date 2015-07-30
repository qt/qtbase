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


#include "qpushbutton.h"
#include <qapplication.h>

#include <qpushbutton.h>
#include <qmenu.h>
#include <qtimer.h>
#include <QDialog>
#include <QGridLayout>
#include <QStyleFactory>
#include <QTabWidget>

class tst_QPushButton : public QObject
{
Q_OBJECT
public:
    tst_QPushButton();
    virtual ~tst_QPushButton();


public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
    void getSetCheck();
    void autoRepeat();
    void pressed();
    void setAccel();
    void isCheckable();
    void setDown();
    void popupCrash();
    void isChecked();
    void animateClick();
    void toggle();
    void clicked();
    void toggled();
    void defaultAndAutoDefault();
    void sizeHint_data();
    void sizeHint();
    void taskQTBUG_20191_shortcutWithKeypadModifer();
    void emitReleasedAfterChange();

protected slots:
    void resetCounters();
    void onClicked();
    void onToggled( bool on );
    void onPressed();
    void onReleased();
    void helperSlotDelete();

private:
    uint click_count;
    uint toggle_count;
    uint press_count;
    uint release_count;

    QPushButton *testWidget;
};

// Testing get/set functions
void tst_QPushButton::getSetCheck()
{
    QPushButton obj1;
    // QMenu* QPushButton::menu()
    // void QPushButton::setMenu(QMenu*)
    QMenu *var1 = new QMenu;
    obj1.setMenu(var1);
    QCOMPARE(var1, obj1.menu());
    obj1.setMenu((QMenu *)0);
    QCOMPARE((QMenu *)0, obj1.menu());
    delete var1;
}

tst_QPushButton::tst_QPushButton()
{
}

tst_QPushButton::~tst_QPushButton()
{
}

void tst_QPushButton::initTestCase()
{
    // Create the test class
    testWidget = new QPushButton( "&Start", 0 );
    testWidget->setObjectName("testWidget");
    testWidget->resize( 200, 200 );
    testWidget->show();

    connect( testWidget, SIGNAL(clicked()), this, SLOT(onClicked()) );
    connect( testWidget, SIGNAL(pressed()), this, SLOT(onPressed()) );
    connect( testWidget, SIGNAL(released()), this, SLOT(onReleased()) );
    connect( testWidget, SIGNAL(toggled(bool)), this, SLOT(onToggled(bool)) );
}

void tst_QPushButton::cleanupTestCase()
{
    delete testWidget;
    testWidget = 0;
}

void tst_QPushButton::init()
{
    testWidget->setAutoRepeat( false );
    testWidget->setDown( false );
    testWidget->setText("Test");
    testWidget->setEnabled( true );
    QKeySequence seq;
    testWidget->setShortcut( seq );

    resetCounters();
}

void tst_QPushButton::cleanup()
{
}


void tst_QPushButton::resetCounters()
{
    toggle_count = 0;
    press_count = 0;
    release_count = 0;
    click_count = 0;
}

void tst_QPushButton::onClicked()
{
    click_count++;
}

void tst_QPushButton::onToggled( bool /*on*/ )
{
    toggle_count++;
}

void tst_QPushButton::onPressed()
{
    press_count++;
}

void tst_QPushButton::onReleased()
{
    release_count++;
}

void tst_QPushButton::autoRepeat()
{
    // If this changes, this test must be completely revised.
    QVERIFY( !testWidget->isCheckable() );

    // verify autorepeat is off by default.
    QPushButton tmp( 0 );
    tmp.setObjectName("tmp");
    QVERIFY( !tmp.autoRepeat() );

    // check if we can toggle the mode
    testWidget->setAutoRepeat( true );
    QVERIFY( testWidget->autoRepeat() );

    testWidget->setAutoRepeat( false );
    QVERIFY( !testWidget->autoRepeat() );

    resetCounters();

    // check that the button is down if we press space and not in autorepeat
    testWidget->setDown( false );
    testWidget->setAutoRepeat( false );
    QTest::keyPress( testWidget, Qt::Key_Space );

    QTest::qWait( 300 );

    QVERIFY( testWidget->isDown() );
    QVERIFY( toggle_count == 0 );
    QVERIFY( press_count == 1 );
    QVERIFY( release_count == 0 );
    QVERIFY( click_count == 0 );

    QTest::keyRelease( testWidget, Qt::Key_Space );
    resetCounters();

    // check that the button is down if we press space while in autorepeat
    // we can't actually confirm how many times it is fired, more than 1 is enough.

    testWidget->setDown( false );
    testWidget->setAutoRepeat( true );
    QTest::keyPress( testWidget, Qt::Key_Space );
    QTest::qWait(900);
    QVERIFY( testWidget->isDown() );
    QVERIFY( toggle_count == 0 );
    QTest::keyRelease( testWidget, Qt::Key_Space );
    QCOMPARE(press_count, release_count);
    QCOMPARE(release_count, click_count);
    QVERIFY(press_count > 1);

    // #### shouldn't I check here to see if multiple signals have been fired???

    // check that pressing ENTER has no effect
    resetCounters();
    testWidget->setDown( false );
    testWidget->setAutoRepeat( false );
    QTest::keyPress( testWidget, Qt::Key_Enter );

    QTest::qWait( 300 );

    QVERIFY( !testWidget->isDown() );
    QVERIFY( toggle_count == 0 );
    QVERIFY( press_count == 0 );
    QVERIFY( release_count == 0 );
    QVERIFY( click_count == 0 );
    QTest::keyRelease( testWidget, Qt::Key_Enter );

    // check that pressing ENTER has no effect
    resetCounters();
    testWidget->setDown( false );
    testWidget->setAutoRepeat( true );
    QTest::keyClick( testWidget, Qt::Key_Enter );
    QTest::qWait( 300 );
    QVERIFY( !testWidget->isDown() );
    QVERIFY( toggle_count == 0 );
    QVERIFY( press_count == 0 );
    QVERIFY( release_count == 0 );
    QVERIFY( click_count == 0 );
}

void tst_QPushButton::pressed()
{
    QTest::keyPress( testWidget, ' ' );
//    QTest::qWait( 300 );
    QCOMPARE( press_count, (uint)1 );
    QCOMPARE( release_count, (uint)0 );

    QTest::keyRelease( testWidget, ' ' );
//    QTest::qWait( 300 );
    QCOMPARE( press_count, (uint)1 );
    QCOMPARE( release_count, (uint)1 );

    QTest::keyPress( testWidget,Qt::Key_Enter );
//    QTest::qWait( 300 );
    QCOMPARE( press_count, (uint)1 );
    QCOMPARE( release_count, (uint)1 );

    testWidget->setAutoDefault(true);
    QTest::keyPress( testWidget,Qt::Key_Enter );
//    QTest::qWait( 300 );
    QCOMPARE( press_count, (uint)2 );
    QCOMPARE( release_count, (uint)2 );
    testWidget->setAutoDefault(false);

}

void tst_QPushButton::isCheckable()
{
    QVERIFY( !testWidget->isCheckable() );
}

void tst_QPushButton::setDown()
{
    testWidget->setDown( false );
    QVERIFY( !testWidget->isDown() );

    testWidget->setDown( true );
    QVERIFY( testWidget->isDown() );

    testWidget->setDown( true );
    QTest::keyClick( testWidget, Qt::Key_Escape );
    QVERIFY( !testWidget->isDown() );
}

void tst_QPushButton::isChecked()
{
    testWidget->setDown( false );
    QVERIFY( !testWidget->isChecked() );

    testWidget->setDown( true );
    QVERIFY( !testWidget->isChecked() );

    testWidget->setDown( false );
    testWidget->toggle();
    QVERIFY( testWidget->isChecked() == testWidget->isCheckable() );
}

void tst_QPushButton::toggle()
{
    // the pushbutton shouldn't toggle the button.
    testWidget->toggle();
    QVERIFY( testWidget->isChecked() == false );
}

void tst_QPushButton::toggled()
{
    // the pushbutton shouldn't send a toggled signal when we call the toggle slot.
    QVERIFY( !testWidget->isCheckable() );

    testWidget->toggle();
    QVERIFY( toggle_count == 0 );

    // do it again, just to be sure
    resetCounters();
    testWidget->toggle();
    QVERIFY( toggle_count == 0 );

    // finally check that we can toggle using the mouse
    resetCounters();
    QTest::mousePress( testWidget, Qt::LeftButton );
    QVERIFY( toggle_count == 0 );
    QVERIFY( click_count == 0 );

    QTest::mouseRelease( testWidget, Qt::LeftButton );
    QVERIFY( click_count == 1 );
}

/*
    If we press an accelerator key we ONLY get a pressed signal and
    NOT a released or clicked signal.
*/

void tst_QPushButton::setAccel()
{
    testWidget->setText("&AccelTest");
    QKeySequence seq( Qt::ALT + Qt::Key_A );
    testWidget->setShortcut( seq );

    // The shortcut will not be activated unless the button is in a active
    // window and has focus
    QApplication::setActiveWindow(testWidget);
    testWidget->setFocus();
    for (int i = 0; !testWidget->isActiveWindow() && i < 1000; ++i) {
        testWidget->activateWindow();
        QApplication::instance()->processEvents();
        QTest::qWait(100);
    }
    QVERIFY(testWidget->isActiveWindow());
    QTest::keyClick( testWidget, 'A', Qt::AltModifier );
    QTest::qWait( 50 );
    QTRY_VERIFY( click_count == 1 );
    QVERIFY( press_count == 1 );
    QVERIFY( release_count == 1 );
    QVERIFY( toggle_count == 0 );

    // wait 200 ms because setAccel uses animateClick.
    // if we don't wait this may screw up a next test.
    QTest::qWait(200);
    QTRY_VERIFY( !testWidget->isDown() );
}

void tst_QPushButton::animateClick()
{
    QVERIFY( !testWidget->isDown() );
    testWidget->animateClick();
    QVERIFY( testWidget->isDown() );
    QTest::qWait( 200 );
    QVERIFY( !testWidget->isDown() );

    QVERIFY( click_count == 1 );
    QVERIFY( press_count == 1 );
    QVERIFY( release_count == 1 );
    QVERIFY( toggle_count == 0 );
}

void tst_QPushButton::clicked()
{
    QTest::mousePress( testWidget, Qt::LeftButton );
    QVERIFY( press_count == 1 );
    QVERIFY( release_count == 0 );

    QTest::mouseRelease( testWidget, Qt::LeftButton );
    QCOMPARE( press_count, (uint)1 );
    QCOMPARE( release_count, (uint)1 );

    press_count = 0;
    release_count = 0;
    testWidget->setDown(false);
    for (uint i=0; i<10; i++)
        QTest::mouseClick( testWidget, Qt::LeftButton );
    QCOMPARE( press_count, (uint)10 );
    QCOMPARE( release_count, (uint)10 );
}

QPushButton *pb = 0;
void tst_QPushButton::helperSlotDelete()
{
    delete pb;
    pb = 0;
}

void tst_QPushButton::popupCrash()
{
    pb = new QPushButton("foo");
    QMenu *menu = new QMenu("bar", pb);
    pb->setMenu(menu);
    QTimer::singleShot(1000, this, SLOT(helperSlotDelete()));
    pb->show();
    pb->click();
}

void tst_QPushButton::defaultAndAutoDefault()
{
    {
    // Adding buttons directly to QDialog
    QDialog dialog;

    QPushButton button1(&dialog);
    QVERIFY(button1.autoDefault());
    QVERIFY(!button1.isDefault());

    QPushButton button2(&dialog);
    QVERIFY(button2.autoDefault());
    QVERIFY(!button2.isDefault());

    button1.setDefault(true);
    QVERIFY(button1.autoDefault());
    QVERIFY(button1.isDefault());
    QVERIFY(button2.autoDefault());
    QVERIFY(!button2.isDefault());

    dialog.show();
    QVERIFY(dialog.isVisible());

    QObject::connect(&button1, SIGNAL(clicked()), &dialog, SLOT(hide()));
    QKeyEvent event(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QApplication::sendEvent(&dialog, &event);
    QVERIFY(!dialog.isVisible());
    }

    {
    // Adding buttons to QDialog through a layout
    QDialog dialog;

    QPushButton button3;
    button3.setAutoDefault(false);

    QPushButton button1;
    QVERIFY(!button1.autoDefault());
    QVERIFY(!button1.isDefault());

    QPushButton button2;
    QVERIFY(!button2.autoDefault());
    QVERIFY(!button2.isDefault());

    button1.setDefault(true);
    QVERIFY(!button1.autoDefault());
    QVERIFY(button1.isDefault());
    QVERIFY(!button2.autoDefault());
    QVERIFY(!button2.isDefault());

    QGridLayout layout;
    layout.addWidget(&button3, 0, 3);
    layout.addWidget(&button2, 0, 2);
    layout.addWidget(&button1, 0, 1);
    dialog.setLayout(&layout);
    button3.setFocus();
    QVERIFY(button1.autoDefault());
    QVERIFY(button1.isDefault());
    QVERIFY(button2.autoDefault());
    QVERIFY(!button2.isDefault());

    dialog.show();
    QVERIFY(dialog.isVisible());

    QObject::connect(&button1, SIGNAL(clicked()), &dialog, SLOT(hide()));
    QKeyEvent event(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QApplication::sendEvent(&dialog, &event);
    QVERIFY(!dialog.isVisible());
    }

    {
    // autoDefault behavior.
    QDialog dialog;
    QPushButton button2(&dialog);
    QPushButton button1(&dialog);
    dialog.show();
    QVERIFY(dialog.isVisible());

    // No default button is set, and button2 is the first autoDefault button
    // that is next in the tab order
    QObject::connect(&button2, SIGNAL(clicked()), &dialog, SLOT(hide()));
    QKeyEvent event(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QApplication::sendEvent(&dialog, &event);
    QVERIFY(!dialog.isVisible());

    // Reparenting
    QVERIFY(button2.autoDefault());
    button2.setParent(0);
    QVERIFY(!button2.autoDefault());
    button2.setAutoDefault(false);
    button2.setParent(&dialog);
    QVERIFY(!button2.autoDefault());

    button1.setAutoDefault(true);
    button1.setParent(0);
    QVERIFY(button1.autoDefault());
    }
}

void tst_QPushButton::sizeHint_data()
{
    QTest::addColumn<QString>("stylename");
#if !defined(QT_NO_STYLE_WINDOWS)
    QTest::newRow("windows") << QString::fromLatin1("windows");
#endif
#if !defined(QT_NO_STYLE_GTK)
    QTest::newRow("gtk") << QString::fromLatin1("gtk");
#endif
#if defined(Q_OS_MAC) && !defined(QT_NO_STYLE_MAC)
    QTest::newRow("macintosh") << QString::fromLatin1("macintosh");
#endif
#if !defined(QT_NO_STYLE_FUSION)
    QTest::newRow("fusion") << QString::fromLatin1("fusion");
#endif
#if defined(Q_OS_WIN) && !defined(QT_NO_STYLE_WINDOWSXP)
    QTest::newRow("windowsxp") << QString::fromLatin1("windowsxp");
#endif
#if defined(Q_OS_WIN) && !defined(QT_NO_STYLE_WINDOWSVISTA)
    QTest::newRow("windowsvista") << QString::fromLatin1("windowsvista");
#endif
#if defined(Q_OS_WINCE) && !defined(QT_NO_STYLE_WINDOWSCE)
    QTest::newRow("windowsce") << QString::fromLatin1("windowsce");
#endif
#if defined(Q_OS_WINCE_WM) && !defined(QT_NO_STYLE_WINDOWSCE)
    QTest::newRow("windowsmobile") << QString::fromLatin1("windowsmobile");
#endif
}

void tst_QPushButton::sizeHint()
{
    QFETCH(QString, stylename);

    QStyle *style = QStyleFactory::create(stylename);
    if (!style)
        QFAIL(qPrintable(QString::fromLatin1("Cannot create style: %1").arg(stylename)));
    QApplication::setStyle(style);

// Test 1
    {
        QPushButton *button = new QPushButton("123");
        QSize initSizeHint = button->sizeHint();

        QDialog *dialog = new QDialog;
        QWidget *widget = new QWidget(dialog);
        button->setParent(widget);
        button->sizeHint();

        widget->setParent(0);
        delete dialog;
        button->setDefault(false);
        QCOMPARE(button->sizeHint(), initSizeHint);
        delete button;
    }

// Test 2
    {
        QWidget *tab1 = new QWidget;
        QHBoxLayout *layout1 = new QHBoxLayout(tab1);
        QPushButton *button1_1 = new QPushButton("123");
        QPushButton *button1_2 = new QPushButton("123");
        layout1->addWidget(button1_1);
        layout1->addWidget(button1_2);

        QWidget *tab2 = new QWidget;
        QHBoxLayout *layout2 = new QHBoxLayout(tab2);
        QPushButton *button2_1 = new QPushButton("123");
        QPushButton *button2_2 = new QPushButton("123");
        layout2->addWidget(button2_1);
        layout2->addWidget(button2_2);

        QDialog *dialog = new QDialog;
        QTabWidget *tabWidget = new QTabWidget;
        tabWidget->addTab(tab1, "1");
        tabWidget->addTab(tab2, "2");
        QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
        mainLayout->addWidget(tabWidget);
        dialog->showNormal();
        tabWidget->setCurrentWidget(tab2);
        tabWidget->setCurrentWidget(tab1);
        QTest::qWait(100);
        QApplication::processEvents();

        QCOMPARE(button1_2->size(), button2_2->size());
    }
}

void tst_QPushButton::taskQTBUG_20191_shortcutWithKeypadModifer()
{
    // setup a dialog with two buttons
    QPushButton *button1 = new QPushButton("5");
    QPushButton *button2 = new QPushButton("5 + KeypadModifier");
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(button1);
    layout->addWidget(button2);
    QDialog dialog;
    dialog.setLayout(layout);
    dialog.show();
    QTest::qWaitForWindowExposed(&dialog);
    QApplication::setActiveWindow(&dialog);

    // add shortcut '5' to button1 and test with keyboard and keypad '5' keys
    QSignalSpy spy1(button1, SIGNAL(clicked()));
    button1->setShortcut(Qt::Key_5);
    QTest::keyClick(&dialog, Qt::Key_5);
    QTest::qWait(300);
    QTest::keyClick(&dialog, Qt::Key_5, Qt::KeypadModifier);
    QTest::qWait(300);
    QCOMPARE(spy1.count(), 2);

    // add shortcut 'keypad 5' to button2
    spy1.clear();
    QSignalSpy spy2(button2, SIGNAL(clicked()));
    button2->setShortcut(Qt::Key_5 + Qt::KeypadModifier);
    QTest::keyClick(&dialog, Qt::Key_5);
    QTest::qWait(300);
    QTest::keyClick(&dialog, Qt::Key_5, Qt::KeypadModifier);
    QTest::qWait(300);
    QCOMPARE(spy1.count(), 1);
    QCOMPARE(spy2.count(), 1);

    // remove shortcut from button1
    spy1.clear();
    spy2.clear();
    button1->setShortcut(QKeySequence());
    QTest::keyClick(&dialog, Qt::Key_5);
    QTest::qWait(300);
    QTest::keyClick(&dialog, Qt::Key_5, Qt::KeypadModifier);
    QTest::qWait(300);
    QCOMPARE(spy1.count(), 0);
    QCOMPARE(spy2.count(), 1);
}

void tst_QPushButton::emitReleasedAfterChange()
{
    QPushButton *button1 = new QPushButton("A");
    QPushButton *button2 = new QPushButton("B");
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(button1);
    layout->addWidget(button2);
    QDialog dialog;
    dialog.setLayout(layout);
    dialog.show();
    QTest::qWaitForWindowExposed(&dialog);
    QApplication::setActiveWindow(&dialog);
    button1->setFocus();

    QSignalSpy spy(button1, SIGNAL(released()));
    QTest::mousePress(button1, Qt::LeftButton);
    QVERIFY(button1->isDown());
    QTest::keyClick(&dialog, Qt::Key_Tab);
    QVERIFY(!button1->isDown());
    QCOMPARE(spy.count(), 1);
    spy.clear();

    QCOMPARE(spy.count(), 0);
    button1->setFocus();
    QTest::mousePress(button1, Qt::LeftButton);
    QVERIFY(button1->isDown());
    button1->setEnabled(false);
    QVERIFY(!button1->isDown());
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(tst_QPushButton)
#include "tst_qpushbutton.moc"
