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


#include "qcommandlinkbutton.h"
#include <qapplication.h>

#include <qcommandlinkbutton.h>
#include <qmenu.h>
#include <qtimer.h>
#include <QDialog>
#include <QGridLayout>
#include <QPainter>

class tst_QCommandLinkButton : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

    void getSetCheck();
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
    void setAutoRepeat();
    void heightForWithWithIcon();

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

    QCommandLinkButton *testWidget;
};

// Testing get/set functions
void tst_QCommandLinkButton::getSetCheck()
{
    QCommandLinkButton obj1;

    QString text("mytext");
    QVERIFY(obj1.description().isEmpty());
    obj1.setDescription(text);
    QCOMPARE(obj1.description(), text);

    QVERIFY(obj1.text().isEmpty());
    obj1.setText(text);
    QCOMPARE(obj1.text(), text);

    QMenu *var1 = new QMenu;
    obj1.setMenu(var1);
    QCOMPARE(var1, obj1.menu());
    obj1.setMenu((QMenu *)0);
    QCOMPARE((QMenu *)0, obj1.menu());
    delete var1;
}

void tst_QCommandLinkButton::initTestCase()
{
    // Create the test class
    testWidget = new QCommandLinkButton( "&Start", 0 );
    testWidget->setObjectName("testWidget");
    testWidget->resize( 200, 200 );
    testWidget->show();

    connect( testWidget, SIGNAL(clicked()), this, SLOT(onClicked()) );
    connect( testWidget, SIGNAL(pressed()), this, SLOT(onPressed()) );
    connect( testWidget, SIGNAL(released()), this, SLOT(onReleased()) );
    connect( testWidget, SIGNAL(toggled(bool)), this, SLOT(onToggled(bool)) );
}

void tst_QCommandLinkButton::cleanupTestCase()
{
    delete testWidget;
    testWidget = 0;
}

void tst_QCommandLinkButton::init()
{
    testWidget->setAutoRepeat( false );
    testWidget->setDown( false );
    testWidget->setText("Test");
    testWidget->setDescription("Description text.");
    testWidget->setEnabled( true );
    QKeySequence seq;
    testWidget->setShortcut( seq );

    resetCounters();
}

void tst_QCommandLinkButton::resetCounters()
{
    toggle_count = 0;
    press_count = 0;
    release_count = 0;
    click_count = 0;
}

void tst_QCommandLinkButton::onClicked()
{
    click_count++;
}

void tst_QCommandLinkButton::onToggled( bool /*on*/ )
{
    toggle_count++;
}

void tst_QCommandLinkButton::onPressed()
{
    press_count++;
}

void tst_QCommandLinkButton::onReleased()
{
    release_count++;
}

void tst_QCommandLinkButton::setAutoRepeat()
{
    // Give the last tests time to finish - i.e., wait for the window close
    // and deactivate to avoid a race condition here. We can't add this to the
    // end of the defaultAndAutoDefault test, since any failure in that test
    // will return out of that function.
    QTest::qWait( 1000 );

    // If this changes, this test must be completely revised.
    QVERIFY( !testWidget->isCheckable() );

    // verify autorepeat is off by default.
    QCommandLinkButton tmp( 0 );
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

void tst_QCommandLinkButton::pressed()
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

void tst_QCommandLinkButton::isCheckable()
{
    QVERIFY( !testWidget->isCheckable() );
}

void tst_QCommandLinkButton::setDown()
{
    testWidget->setDown( false );
    QVERIFY( !testWidget->isDown() );

    testWidget->setDown( true );
    QVERIFY( testWidget->isDown() );

    testWidget->setDown( true );
    QTest::keyClick( testWidget, Qt::Key_Escape );
    QVERIFY( !testWidget->isDown() );
}

void tst_QCommandLinkButton::isChecked()
{
    testWidget->setDown( false );
    QVERIFY( !testWidget->isChecked() );

    testWidget->setDown( true );
    QVERIFY( !testWidget->isChecked() );

    testWidget->setDown( false );
    testWidget->toggle();
    QVERIFY( testWidget->isChecked() == testWidget->isCheckable() );
}

void tst_QCommandLinkButton::toggle()
{
    // the pushbutton shouldn't toggle the button.
    testWidget->toggle();
    QVERIFY(!testWidget->isChecked());
}

void tst_QCommandLinkButton::toggled()
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

void tst_QCommandLinkButton::setAccel()
{
    testWidget->setText("&AccelTest");
    QKeySequence seq( Qt::ALT + Qt::Key_A );
    testWidget->setShortcut( seq );

    // The shortcut will not be activated unless the button is in a active
    // window and has focus
    testWidget->setFocus();

    // QWidget::isActiveWindow() can report window active before application
    // has handled the asynchronous activation event on platforms that have
    // implemented QPlatformWindow::isActive(), so process events to sync up.
    QApplication::instance()->processEvents();

    for (int i = 0; !testWidget->isActiveWindow() && i < 1000; ++i) {
        testWidget->activateWindow();
        QApplication::instance()->processEvents();
        QTest::qWait(100);
    }

    QVERIFY(testWidget->isActiveWindow());

    QTest::keyClick( testWidget, 'A', Qt::AltModifier );
    QTest::qWait( 500 );
    QVERIFY( click_count == 1 );
    QVERIFY( press_count == 1 );
    QVERIFY( release_count == 1 );
    QVERIFY( toggle_count == 0 );

    // wait 200 ms because setAccel uses animateClick.
    // if we don't wait this may screw up a next test.
    QTest::qWait(200);
}

void tst_QCommandLinkButton::animateClick()
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

void tst_QCommandLinkButton::clicked()
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

QCommandLinkButton *pb = 0;
void tst_QCommandLinkButton::helperSlotDelete()
{
    delete pb;
    pb = 0;
}

void tst_QCommandLinkButton::popupCrash()
{
    pb = new QCommandLinkButton("foo", "description");
    QMenu *menu = new QMenu("bar", pb);
    pb->setMenu(menu);
    QTimer::singleShot(1000, this, SLOT(helperSlotDelete()));
    pb->show();
    pb->click();
}

void tst_QCommandLinkButton::defaultAndAutoDefault()
{
    {
    // Adding buttons directly to QDialog
    QDialog dialog;

    QCommandLinkButton button1(&dialog);
    QVERIFY(button1.autoDefault());
    QVERIFY(!button1.isDefault());

    QCommandLinkButton button2(&dialog);
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

    QCommandLinkButton button3;
    button3.setAutoDefault(false);

    QCommandLinkButton button1;
    QVERIFY(!button1.autoDefault());
    QVERIFY(!button1.isDefault());

    QCommandLinkButton button2;
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
    QCommandLinkButton button2(&dialog);
    QCommandLinkButton button1(&dialog);
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

void tst_QCommandLinkButton::heightForWithWithIcon()
{
    QWidget mainWin;

    QPixmap pixmap(64, 64);
    {
        pixmap.fill(Qt::white);
        QPainter painter(&pixmap);
        painter.setBrush(Qt::black);
        painter.drawEllipse(0, 0, 63, 63);
    }

    QCommandLinkButton *largeIconButton = new QCommandLinkButton(QString("Large Icon"),
                    QString("Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Mauris nibh lectus, adipiscing eu."),
                    &mainWin);
    largeIconButton->setIconSize(QSize(64, 64));
    largeIconButton->setIcon(pixmap);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(largeIconButton);
    layout->addStretch();
    mainWin.setLayout(layout);
    mainWin.showMaximized();
    QVERIFY(QTest::qWaitForWindowExposed(&mainWin));
    QVERIFY(largeIconButton->height() > 68);  //enough room for the icon

}

QTEST_MAIN(tst_QCommandLinkButton)
#include "tst_qcommandlinkbutton.moc"
