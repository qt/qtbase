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

#include "../../shared/util.h"

Q_DECLARE_METATYPE(QPushButton*)

//TESTED_CLASS=
//TESTED_FILES=

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
    void setAutoRepeat();
    void pressed();
    void released();
    void text();
    void accel();
    void setAccel();
    void isCheckable();
    void isDown();
    void setDown();
    void popupCrash();
    void isChecked();
    void autoRepeat();
    void animateClick();
    void toggle();
    void clicked();
    void toggled();
    void isEnabled();
    void defaultAndAutoDefault();
    void sizeHint_data();
    void sizeHint();
/*
    void state();
    void group();
    void stateChanged();
*/

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
    testWidget->setAutoRepeat( FALSE );
    testWidget->setDown( FALSE );
    testWidget->setText("Test");
    testWidget->setEnabled( TRUE );
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

void tst_QPushButton::setAutoRepeat()
{
    // If this changes, this test must be completely revised.
    QVERIFY( !testWidget->isCheckable() );

    // verify autorepeat is off by default.
    QPushButton tmp( 0 );
    tmp.setObjectName("tmp");
    QVERIFY( !tmp.autoRepeat() );

    // check if we can toggle the mode
    testWidget->setAutoRepeat( TRUE );
    QVERIFY( testWidget->autoRepeat() );

    testWidget->setAutoRepeat( FALSE );
    QVERIFY( !testWidget->autoRepeat() );

    resetCounters();

    // check that the button is down if we press space and not in autorepeat
    testWidget->setDown( FALSE );
    testWidget->setAutoRepeat( FALSE );
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

    testWidget->setDown( FALSE );
    testWidget->setAutoRepeat( TRUE );
    QTest::keyPress( testWidget, Qt::Key_Space );
    QTest::qWait(900);
    QVERIFY( testWidget->isDown() );
    QVERIFY( toggle_count == 0 );
    QTest::keyRelease( testWidget, Qt::Key_Space );
    QVERIFY(press_count == release_count);
    QVERIFY(release_count == click_count);
    QVERIFY(press_count > 1);

    // #### shouldn't I check here to see if multiple signals have been fired???

    // check that pressing ENTER has no effect
    resetCounters();
    testWidget->setDown( FALSE );
    testWidget->setAutoRepeat( FALSE );
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
    testWidget->setDown( FALSE );
    testWidget->setAutoRepeat( TRUE );
    QTest::keyClick( testWidget, Qt::Key_Enter );
    QTest::qWait( 300 );
    QVERIFY( !testWidget->isDown() );
    QVERIFY( toggle_count == 0 );
    QVERIFY( press_count == 0 );
    QVERIFY( release_count == 0 );
    QVERIFY( click_count == 0 );
}

void tst_QPushButton::autoRepeat()
{
    DEPENDS_ON(" setAutoRepeat" );
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

void tst_QPushButton::released()
{
    DEPENDS_ON( "pressed" );
}

void tst_QPushButton::text()
{
    DEPENDS_ON( "setText" );
}

void tst_QPushButton::isEnabled()
{
    DEPENDS_ON( "setEnabled" );
}

void tst_QPushButton::isCheckable()
{
    QVERIFY( !testWidget->isCheckable() );
}

void tst_QPushButton::isDown()
{
    DEPENDS_ON( "setDown" );
}

void tst_QPushButton::setDown()
{
    testWidget->setDown( FALSE );
    QVERIFY( !testWidget->isDown() );

    testWidget->setDown( TRUE );
    QVERIFY( testWidget->isDown() );

    testWidget->setDown( TRUE );
    QTest::keyClick( testWidget, Qt::Key_Escape );
    QVERIFY( !testWidget->isDown() );
}

void tst_QPushButton::isChecked()
{
    testWidget->setDown( FALSE );
    QVERIFY( !testWidget->isChecked() );

    testWidget->setDown( TRUE );
    QVERIFY( !testWidget->isChecked() );

    testWidget->setDown( FALSE );
    testWidget->toggle();
    QVERIFY( testWidget->isChecked() == testWidget->isCheckable() );
}

void tst_QPushButton::toggle()
{
    // the pushbutton shouldn't toggle the button.
    testWidget->toggle();
    QVERIFY( testWidget->isChecked() == FALSE );
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

void tst_QPushButton::accel()
{
    DEPENDS_ON( "setAccel" );
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
    testWidget->setDown(FALSE);
    for (uint i=0; i<10; i++)
        QTest::mouseClick( testWidget, Qt::LeftButton );
    QCOMPARE( press_count, (uint)10 );
    QCOMPARE( release_count, (uint)10 );
}

/*
void tst_QPushButton::group()
{
}

void tst_QPushButton::state()
{
}

void tst_QPushButton::stateChanged()
{
}
*/
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
    QTest::newRow("motif") << QString::fromAscii("motif");
    QTest::newRow("cde") << QString::fromAscii("cde");
    QTest::newRow("windows") << QString::fromAscii("windows");
    QTest::newRow("cleanlooks") << QString::fromAscii("cleanlooks");
    QTest::newRow("gtk") << QString::fromAscii("gtk");
    QTest::newRow("mac") << QString::fromAscii("mac");
    QTest::newRow("plastique") << QString::fromAscii("plastique");
    QTest::newRow("windowsxp") << QString::fromAscii("windowsxp");
    QTest::newRow("windowsvista") << QString::fromAscii("windowsvista");
}

void tst_QPushButton::sizeHint()
{
    QFETCH(QString, stylename);

    QStyle *style = QStyleFactory::create(stylename);
    if (!style)
        QSKIP(qPrintable(QString::fromLatin1("Qt has been compiled without style: %1")
                         .arg(stylename)), SkipSingle);
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
        dialog->show();
        tabWidget->setCurrentWidget(tab2);
        tabWidget->setCurrentWidget(tab1);
        QTest::qWait(100);
        QApplication::processEvents();

        QCOMPARE(button1_2->size(), button2_2->size());
    }
}

QTEST_MAIN(tst_QPushButton)
#include "tst_qpushbutton.moc"
