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
#include <qpainter.h>
#include <qstyleoption.h>
#include <qkeysequence.h>
#include <qevent.h>
#include <qgridlayout.h>
#include <qabstractbutton.h>

#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

class tst_QAbstractButton : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

    void setAutoRepeat_data();
    void setAutoRepeat();

    void pressed();
    void released();
    void setText();
    void setIcon();

    void setShortcut();

    void animateClick();

    void isCheckable();
    void setDown();
    void isChecked();
    void toggled();
    void setEnabled();
    void shortcutEvents();
    void stopRepeatTimer();

    void mouseReleased(); // QTBUG-53244
#ifdef QT_KEYPAD_NAVIGATION
    void keyNavigation();
#endif

protected slots:
    void onClicked();
    void onToggled( bool on );
    void onPressed();
    void onReleased();
    void resetValues();

private:
    uint click_count;
    uint toggle_count;
    uint press_count;
    uint release_count;

    QAbstractButton *testWidget;
};

// QAbstractButton is an abstract class in 4.0
class MyButton : public QAbstractButton
{
public:
    MyButton(QWidget *p = 0) : QAbstractButton(p) {}
    void paintEvent(QPaintEvent *)
    {
        QPainter p(this);
        QRect r = rect();
        p.fillRect(r, isDown() ? Qt::black : (isChecked() ? Qt::lightGray : Qt::white));
        p.setPen(isDown() ? Qt::white : Qt::black);
        p.drawRect(r);
        p.drawText(r, Qt::AlignCenter | Qt::TextShowMnemonic, text());
        if (hasFocus()) {
            r.adjust(2, 2, -2, -2);
            QStyleOptionFocusRect opt;
            opt.rect = r;
            opt.palette = palette();
            opt.state = QStyle::State_None;
            style()->drawPrimitive(QStyle::PE_FrameFocusRect, &opt, &p, this);
#ifdef Q_OS_MAC
            p.setPen(Qt::red);
            p.drawRect(r);
#endif
        }
    }
    QSize sizeHint() const
    {
        QSize sh(8, 8);
        if (!text().isEmpty())
            sh += fontMetrics().boundingRect(text()).size();
        return sh;
    }

    void resetTimerEvents() { timerEvents = 0; }
    int timerEventCount() const { return timerEvents; }

private:

    int timerEvents;

    void timerEvent(QTimerEvent *event)
    {
        ++timerEvents;
        QAbstractButton::timerEvent(event);
    }
};

void tst_QAbstractButton::initTestCase()
{
    testWidget = new MyButton(0);
    testWidget->setObjectName("testObject");
    testWidget->resize( 200, 200 );
    testWidget->show();

    connect( testWidget, SIGNAL(clicked()), this, SLOT(onClicked()) );
    connect( testWidget, SIGNAL(pressed()), this, SLOT(onPressed()) );
    connect( testWidget, SIGNAL(released()), this, SLOT(onReleased()) );
    connect( testWidget, SIGNAL(toggled(bool)), this, SLOT(onToggled(bool)) );
}

void tst_QAbstractButton::cleanupTestCase()
{
    delete testWidget;
}

void tst_QAbstractButton::init()
{
    testWidget->setText("Test");
    testWidget->setEnabled( true );
    testWidget->setDown( false );
    testWidget->setAutoRepeat( false );
    QKeySequence seq;
    testWidget->setShortcut( seq );

    toggle_count = 0;
    press_count = 0;
    release_count = 0;
    click_count = 0;
}

void tst_QAbstractButton::resetValues()
{
    toggle_count = 0;
    press_count = 0;
    release_count = 0;
    click_count = 0;
}

void tst_QAbstractButton::onClicked()
{
    click_count++;
}

void tst_QAbstractButton::onToggled( bool /*on*/ )
{
    toggle_count++;
}

void tst_QAbstractButton::onPressed()
{
    press_count++;
}

void tst_QAbstractButton::onReleased()
{
    release_count++;
}

void tst_QAbstractButton::setAutoRepeat_data()
{
    QTest::addColumn<int>("mode");
    QTest::newRow("mode 0") << 0;
    QTest::newRow("mode 1") << 1;
    QTest::newRow("mode 2") << 2;
    QTest::newRow("mode 3") << 3;
    QTest::newRow("mode 4") << 4;
    QTest::newRow("mode 5") << 5;
    QTest::newRow("mode 6") << 6;
}

#define REPEAT_DELAY 1000

int test_count = 0;
int last_mode = 0;

void tst_QAbstractButton::setAutoRepeat()
{
    QFETCH( int, mode );

    //FIXME: temp code to check that the test fails consistenly
    //retest( 3 );

    switch (mode)
    {
    case 0:
        QVERIFY( !testWidget->isCheckable() );
        break;
    case 1:
        // check if we can toggle the mode
        testWidget->setAutoRepeat( true );
        QVERIFY( testWidget->autoRepeat() );

        testWidget->setAutoRepeat( false );
        QVERIFY( !testWidget->autoRepeat() );
        break;
    case 2:
        // check that the button is down if we press space and not in autorepeat
        testWidget->setDown( false );
        testWidget->setAutoRepeat( false );
        QTest::keyPress( testWidget, Qt::Key_Space );

        QTest::qWait( REPEAT_DELAY );

        QVERIFY( release_count == 0 );
        QVERIFY( testWidget->isDown() );
        QVERIFY( toggle_count == 0 );
        QVERIFY( press_count == 1 );
        QVERIFY( click_count == 0 );

        QTest::keyRelease( testWidget, Qt::Key_Space );
        QVERIFY( click_count == 1 );
        QVERIFY( release_count == 1 );
        break;
    case 3:
        // check that the button is down if we press space while in autorepeat
        testWidget->setDown(false);
        testWidget->setAutoRepeat(true);
        QTest::keyPress(testWidget, Qt::Key_Space);
        QTest::qWait(REPEAT_DELAY);
        QVERIFY(testWidget->isDown());
        QTest::keyRelease(testWidget, Qt::Key_Space);
        QCOMPARE(release_count, press_count);
        QCOMPARE(toggle_count, uint(0));
        QCOMPARE(press_count, click_count);
        QVERIFY(click_count > 1);
        break;
    case 4:
        // check that pressing ENTER has no effect when autorepeat is false
        testWidget->setDown( false );
        testWidget->setAutoRepeat( false );
        QTest::keyPress( testWidget, Qt::Key_Enter );

        QTest::qWait( REPEAT_DELAY );

        QVERIFY( !testWidget->isDown() );
        QVERIFY( toggle_count == 0 );
        QVERIFY( press_count == 0 );
        QVERIFY( release_count == 0 );
        QVERIFY( click_count == 0 );
        QTest::keyRelease( testWidget, Qt::Key_Enter );

        QVERIFY( click_count == 0 );
        break;
    case 5:
        // check that pressing ENTER has no effect when autorepeat is true
        testWidget->setDown( false );
        testWidget->setAutoRepeat( true );
        QTest::keyPress( testWidget, Qt::Key_Enter );

        QTest::qWait( REPEAT_DELAY );

        QVERIFY( !testWidget->isDown() );
        QVERIFY( toggle_count == 0 );
        QVERIFY( press_count == 0 );
        QVERIFY( release_count == 0 );
        QVERIFY( click_count == 0 );

        QTest::keyRelease( testWidget, Qt::Key_Enter );

        QVERIFY( click_count == 0 );
        break;
    case 6:
        // verify autorepeat is off by default.
        MyButton tmp( 0);
        tmp.setObjectName("tmp" );
        QVERIFY( !tmp.autoRepeat() );
        break;
    }
}

void tst_QAbstractButton::pressed()
{
    // pressed/released signals expected for a QAbstractButton
    QTest::keyPress( testWidget, ' ' );
    QCOMPARE( press_count, (uint)1 );
}

void tst_QAbstractButton::released()
{
    // pressed/released signals expected for a QAbstractButton
    QTest::keyPress( testWidget, ' ' );
    QTest::keyRelease( testWidget, ' ' );
    QCOMPARE( release_count, (uint)1 );
}

void tst_QAbstractButton::setText()
{
    testWidget->setText("");
    QCOMPARE( testWidget->text(), QString("") );
    testWidget->setText("simple");
    QCOMPARE( testWidget->text(), QString("simple") );
    testWidget->setText("&ampersand");
    QCOMPARE( testWidget->text(), QString("&ampersand") );
#ifndef Q_OS_MAC // no mneonics on Mac.
    QCOMPARE( testWidget->shortcut(), QKeySequence("ALT+A"));
#endif
    testWidget->setText("te&st");
    QCOMPARE( testWidget->text(), QString("te&st") );
#ifndef Q_OS_MAC // no mneonics on Mac.
    QCOMPARE( testWidget->shortcut(), QKeySequence("ALT+S"));
#endif
    testWidget->setText("foo");
    QCOMPARE( testWidget->text(), QString("foo") );
#ifndef Q_OS_MAC // no mneonics on Mac.
    QCOMPARE( testWidget->shortcut(), QKeySequence());
#endif
}

void tst_QAbstractButton::setIcon()
{
    const char *test1_xpm[] = {
    "12 8 2 1",
    ". c None",
    "c c #ff0000",
    ".........ccc",
    "........ccc.",
    ".......ccc..",
    "ccc...ccc...",
    ".ccc.ccc....",
    "..ccccc.....",
    "...ccc......",
    "....c.......",
    };

    QPixmap p(test1_xpm);
    testWidget->setIcon( p );
    QCOMPARE( testWidget->icon().pixmap(12, 8), p );

    // Test for #14793

    const char *test2_xpm[] = {
    "12 8 2 1",
    ". c None",
    "c c #ff0000",
    "ccc......ccc",
    ".ccc....ccc.",
    "..ccc..ccc..",
    "....cc.cc...",
    ".....ccc....",
    "....cc.cc...",
    "...ccc.ccc..",
    "..ccc...ccc.",
    };

    int currentHeight = testWidget->height();
    int currentWidth = testWidget->width();

    QPixmap p2( test2_xpm );
    for ( int a = 0; a<5; a++ )
        testWidget->setIcon( p2 );

    QCOMPARE( testWidget->icon().pixmap(12, 8), p2 );

    QCOMPARE( testWidget->height(), currentHeight );
    QCOMPARE( testWidget->width(), currentWidth );
}

void tst_QAbstractButton::setEnabled()
{
    testWidget->setEnabled( false );
    QVERIFY( !testWidget->isEnabled() );
//    QTEST( testWidget, "disabled" );

    testWidget->setEnabled( true );
    QVERIFY( testWidget->isEnabled() );
//    QTEST( testWidget, "enabled" );
}

void tst_QAbstractButton::isCheckable()
{
    QVERIFY( !testWidget->isCheckable() );
}

void tst_QAbstractButton::setDown()
{
    testWidget->setDown( false );
    QVERIFY( !testWidget->isDown() );

    testWidget->setDown( true );
    QTest::qWait(300);
    QVERIFY( testWidget->isDown() );

    testWidget->setDown( true );

    // add some debugging stuff
    QWidget *grab = QWidget::keyboardGrabber();
    if (grab != 0 && grab != testWidget)
        qDebug( "testWidget != keyboardGrabber" );
    grab = qApp->focusWidget();
    if (grab != 0 && grab != testWidget)
        qDebug( "testWidget != focusWidget" );

    QTest::keyClick( testWidget, Qt::Key_Escape );
    QVERIFY( !testWidget->isDown() );
}

void tst_QAbstractButton::isChecked()
{
    testWidget->setDown( false );
    QVERIFY( !testWidget->isChecked() );

    testWidget->setDown( true );
    QVERIFY( !testWidget->isChecked() );

    testWidget->setDown( false );
    testWidget->toggle();
    QVERIFY( testWidget->isChecked() == testWidget->isCheckable() );
}

void tst_QAbstractButton::toggled()
{
    testWidget->toggle();
    QVERIFY( toggle_count == 0 );

    QTest::mousePress( testWidget, Qt::LeftButton );
    QVERIFY( toggle_count == 0 );
    QVERIFY( click_count == 0 );

    QTest::mouseRelease( testWidget, Qt::LeftButton );
    QVERIFY( click_count == 1 );

    testWidget->setCheckable(true);
    testWidget->toggle();
    testWidget->toggle();
    QCOMPARE(int(toggle_count), 2);
    testWidget->setCheckable(false);
}

void tst_QAbstractButton::setShortcut()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QKeySequence seq( Qt::Key_A );
    testWidget->setShortcut( seq );
    QApplication::setActiveWindow(testWidget);
    testWidget->activateWindow();
    // must be active to get shortcuts
    QVERIFY(QTest::qWaitForWindowActive(testWidget));

    QTest::keyClick( testWidget, 'A' );
    QTest::qWait(300);                      // Animate click takes time
    QCOMPARE(click_count,  (uint)1);
    QCOMPARE(press_count,  (uint)1); // Press is part of a click
    QCOMPARE(release_count,(uint)1); // Release is part of a click

    QVERIFY( toggle_count == 0 );

//     resetValues();
//     QTest::keyPress( testWidget, 'A' );
//     QTest::qWait(10000);
//     QTest::keyRelease( testWidget, 'A' );
//     QCOMPARE(click_count,  (uint)1);
//     QCOMPARE(press_count,  (uint)1);
//     QCOMPARE(release_count,(uint)1);

//     qDebug() << click_count;

}

void tst_QAbstractButton::animateClick()
{
    testWidget->animateClick();
    QVERIFY( testWidget->isDown() );
    qApp->processEvents();
    QVERIFY( testWidget->isDown() );
    QTRY_VERIFY( !testWidget->isDown() );
}

void tst_QAbstractButton::shortcutEvents()
{
    MyButton button;
    QSignalSpy pressedSpy(&button, SIGNAL(pressed()));
    QSignalSpy releasedSpy(&button, SIGNAL(released()));
    QSignalSpy clickedSpy(&button, SIGNAL(clicked(bool)));

    for (int i = 0; i < 4; ++i) {
        QKeySequence sequence;
        // Default shortcutId for QAbstractButton is 0, so the shortcut event will work.
        QShortcutEvent event(sequence, /*shortcutId*/ 0);
        QApplication::sendEvent(&button, &event);
        if (i < 2)
            QTest::qWait(500);
    }

    QTest::qWait(1000); // ensure animate timer is expired

    QCOMPARE(pressedSpy.count(), 3);
    QCOMPARE(releasedSpy.count(), 3);
    QCOMPARE(clickedSpy.count(), 3);
}

void tst_QAbstractButton::stopRepeatTimer()
{
    MyButton button;
    button.setAutoRepeat(true);

    // Mouse trigger case:
    button.resetTimerEvents();
    QTest::mousePress(&button, Qt::LeftButton);
    QTest::qWait(1000);
    QVERIFY(button.timerEventCount() > 0);

    QTest::mouseRelease(&button, Qt::LeftButton);
    button.resetTimerEvents();
    QTest::qWait(1000);
    QCOMPARE(button.timerEventCount(), 0);

    // Key trigger case:
    button.resetTimerEvents();
    QTest::keyPress(&button, Qt::Key_Space);
    QTest::qWait(1000);
    QVERIFY(button.timerEventCount() > 0);

    QTest::keyRelease(&button, Qt::Key_Space);
    button.resetTimerEvents();
    QTest::qWait(1000);
    QCOMPARE(button.timerEventCount(), 0);
}

void tst_QAbstractButton::mouseReleased() // QTBUG-53244
{
    MyButton button(nullptr);
    button.setObjectName("button");
    button.setGeometry(0, 0, 20, 20);
    QSignalSpy spyPress(&button, &QAbstractButton::pressed);
    QSignalSpy spyRelease(&button, &QAbstractButton::released);

    QTest::mousePress(&button, Qt::LeftButton);
    QCOMPARE(spyPress.count(), 1);
    QCOMPARE(button.isDown(), true);
    QCOMPARE(spyRelease.count(), 0);

    QTest::mouseClick(&button, Qt::RightButton);
    QCOMPARE(spyPress.count(), 1);
    QCOMPARE(button.isDown(), true);
    QCOMPARE(spyRelease.count(), 0);

    QPointF posOutOfWidget = QPointF(30, 30);
    QMouseEvent me(QEvent::MouseMove,
                     posOutOfWidget, Qt::NoButton,
                     Qt::MouseButtons(Qt::LeftButton),
                     Qt::NoModifier); // mouse press and move

    qApp->sendEvent(&button, &me);
    // should emit released signal once mouse is dragging out of boundary
    QCOMPARE(spyPress.count(), 1);
    QCOMPARE(button.isDown(), false);
    QCOMPARE(spyRelease.count(), 1);
}

#ifdef QT_KEYPAD_NAVIGATION
void tst_QAbstractButton::keyNavigation()
{
    QApplication::setNavigationMode(Qt::NavigationModeKeypadDirectional);

    QWidget widget;
    QGridLayout *layout = new QGridLayout(&widget);
    QAbstractButton *buttons[3][3];
    for(int y = 0; y < 3; y++) {
        for(int x = 0; x < 3; x++) {
            buttons[y][x] = new MyButton(&widget);
            buttons[y][x]->setFocusPolicy(Qt::StrongFocus);
            layout->addWidget(buttons[y][x], y, x);
        }
    }

    widget.show();
    qApp->setActiveWindow(&widget);
    widget.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&widget));

    buttons[1][1]->setFocus();
    QTest::qWait(400);
    QVERIFY(buttons[1][1]->hasFocus());
    QTest::keyPress(buttons[1][1], Qt::Key_Up);
    QTest::qWait(100);
    QVERIFY(buttons[0][1]->hasFocus());
    QTest::keyPress(buttons[0][1], Qt::Key_Down);
    QTest::qWait(100);
    QVERIFY(buttons[1][1]->hasFocus());
    QTest::keyPress(buttons[1][1], Qt::Key_Left);
    QTest::qWait(100);
    QVERIFY(buttons[1][0]->hasFocus());
    QTest::keyPress(buttons[1][0], Qt::Key_Down);
    QTest::qWait(100);
    QVERIFY(buttons[2][0]->hasFocus());
    QTest::keyPress(buttons[2][0], Qt::Key_Right);
    QTest::qWait(100);
    QVERIFY(buttons[2][1]->hasFocus());
    QTest::keyPress(buttons[2][1], Qt::Key_Right);
    QTest::qWait(100);
    QVERIFY(buttons[2][2]->hasFocus());
    QTest::keyPress(buttons[2][2], Qt::Key_Up);
    QTest::qWait(100);
    QVERIFY(buttons[1][2]->hasFocus());
    QTest::keyPress(buttons[1][2], Qt::Key_Up);
    QTest::qWait(100);
    QVERIFY(buttons[0][2]->hasFocus());
    buttons[0][1]->hide();
    QTest::keyPress(buttons[0][2], Qt::Key_Left);
    QTest::qWait(100);
    QTest::keyPress(buttons[0][2], Qt::Key_Left);
    QEXPECT_FAIL("", "QTBUG-22286" ,Abort);
    QVERIFY(buttons[0][0]->hasFocus());
}
#endif

QTEST_MAIN(tst_QAbstractButton)
#include "tst_qabstractbutton.moc"
