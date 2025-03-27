// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
#include <QSignalSpy>

#include "qpushbutton.h"
#include <qapplication.h>

#include <qpushbutton.h>
#include <qmenu.h>
#include <qtimer.h>
#include <QDialog>
#include <QGridLayout>
#include <QStyleFactory>
#include <QTabWidget>
#include <QStyleOption>

#include <private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>

#include <QtWidgets/private/qapplication_p.h>

class tst_QPushButton : public QObject
{
Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

    void getSetCheck();
    void autoRepeat();
    void pressed();
#if QT_CONFIG(shortcut)
    void setAccel();
#endif
    void isCheckable();
    void setDown();
    void popupCrash();
    void isChecked();
    void toggle();
    void clicked();
    void touchTap();
    void toggled();
    void defaultAndAutoDefault();
    void sizeHint_data();
    void sizeHint();
#if QT_CONFIG(shortcut)
    void taskQTBUG_20191_shortcutWithKeypadModifer();
#endif
    void emitReleasedAfterChange();
    void hitButton();
    void iconOnlyStyleSheet();
    void mousePressAndMove();
    void reactToMenuClosed();

protected slots:
    void resetCounters();
    void onClicked();
    void onToggled(bool on);
    void onPressed();
    void onReleased();

private:
    int click_count;
    int toggle_count;
    int press_count;
    int release_count;

    QPushButton *testWidget;
    std::unique_ptr<QPointingDevice> m_touchScreen{QTest::createTouchDevice()};
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
    obj1.setMenu(nullptr);
    QCOMPARE(obj1.menu(), nullptr);
    delete var1;
}

void tst_QPushButton::initTestCase()
{
    // Create the test class
    testWidget = new QPushButton("&Start", 0);
    testWidget->setObjectName("testWidget");
    testWidget->resize(200, 200);
    testWidget->show();

    connect(testWidget, SIGNAL(clicked()), this, SLOT(onClicked()));
    connect(testWidget, SIGNAL(pressed()), this, SLOT(onPressed()));
    connect(testWidget, SIGNAL(released()), this, SLOT(onReleased()));
    connect(testWidget, SIGNAL(toggled(bool)), this, SLOT(onToggled(bool)));
}

void tst_QPushButton::cleanupTestCase()
{
    delete testWidget;
    testWidget = nullptr;
}

void tst_QPushButton::init()
{
    testWidget->setAutoRepeat(false);
    testWidget->setDown(false);
    testWidget->setText("Test");
    testWidget->setEnabled(true);
#if QT_CONFIG(shortcut)
    QKeySequence seq;
    testWidget->setShortcut(seq);
#endif

    resetCounters();
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

void tst_QPushButton::onToggled(bool /*on*/)
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
    QVERIFY(!testWidget->isCheckable());

    // verify autorepeat is off by default.
    QPushButton tmp;
    tmp.setObjectName("tmp");
    QVERIFY(!tmp.autoRepeat());

    // check if we can toggle the mode
    testWidget->setAutoRepeat(true);
    QVERIFY(testWidget->autoRepeat());

    testWidget->setAutoRepeat(false);
    QVERIFY(!testWidget->autoRepeat());

    resetCounters();

    // check that the button is down if we press space and not in autorepeat
    testWidget->setDown(false);
    testWidget->setAutoRepeat(false);
    QTest::keyPress(testWidget, Qt::Key_Space);

    QTRY_VERIFY(testWidget->isDown());
    QCOMPARE(toggle_count, 0);
    QCOMPARE(press_count, 1);
    QCOMPARE(release_count, 0);
    QCOMPARE(click_count, 0);

    QTest::keyRelease(testWidget, Qt::Key_Space);
    resetCounters();

    // check that the button is down if we press space while in autorepeat
    // we can't actually confirm how many times it is fired, more than 1 is enough.

    testWidget->setDown(false);
    testWidget->setAutoRepeat(true);
    QTest::keyPress(testWidget, Qt::Key_Space);
    QTRY_VERIFY(press_count > 3);
    QVERIFY(testWidget->isDown());
    QCOMPARE(toggle_count, 0);
    QTest::keyRelease(testWidget, Qt::Key_Space);
    QCOMPARE(press_count, release_count);
    QCOMPARE(release_count, click_count);

    // #### shouldn't I check here to see if multiple signals have been fired???

    // check that pressing ENTER has no effect
    resetCounters();
    testWidget->setDown(false);
    // Skip after reset if ButtonPressKeys has Key_Enter
    const auto buttonPressKeys = QGuiApplicationPrivate::platformTheme()
                                         ->themeHint(QPlatformTheme::ButtonPressKeys)
                                         .value<QList<Qt::Key>>();
    if (buttonPressKeys.contains(Qt::Key_Enter)) {
        return;
    }
    testWidget->setAutoRepeat(false);
    QTest::keyPress(testWidget, Qt::Key_Enter);

    QTest::qWait(300);

    QVERIFY(!testWidget->isDown());
    QCOMPARE(toggle_count, 0);
    QCOMPARE(press_count, 0);
    QCOMPARE(release_count, 0);
    QCOMPARE(click_count, 0);
    QTest::keyRelease(testWidget, Qt::Key_Enter);

    // check that pressing ENTER has no effect
    resetCounters();
    testWidget->setDown(false);
    testWidget->setAutoRepeat(true);
    QTest::keyClick(testWidget, Qt::Key_Enter);
    QTest::qWait(300);
    QVERIFY(!testWidget->isDown());
    QCOMPARE(toggle_count, 0);
    QCOMPARE(press_count, 0);
    QCOMPARE(release_count, 0);
    QCOMPARE(click_count, 0);
}

void tst_QPushButton::pressed()
{
    QTest::keyPress(testWidget, ' ');
    QCOMPARE(press_count, 1);
    QCOMPARE(release_count, 0);

    QTest::keyRelease(testWidget, ' ');
    QCOMPARE(press_count, 1);
    QCOMPARE(release_count, 1);

    // Skip if ButtonPressKeys has Key_Enter
    const auto buttonPressKeys = QGuiApplicationPrivate::platformTheme()
                                         ->themeHint(QPlatformTheme::ButtonPressKeys)
                                         .value<QList<Qt::Key>>();
    if (buttonPressKeys.contains(Qt::Key_Enter)) {
        return;
    }

    QTest::keyPress(testWidget,Qt::Key_Enter);
    QCOMPARE(press_count, 1);
    QCOMPARE(release_count, 1);

    testWidget->setAutoDefault(true);
    QTest::keyPress(testWidget,Qt::Key_Enter);
    QCOMPARE(press_count, 2);
    QCOMPARE(release_count, 2);
    testWidget->setAutoDefault(false);

}

void tst_QPushButton::isCheckable()
{
    QVERIFY(!testWidget->isCheckable());
}

void tst_QPushButton::setDown()
{
    testWidget->setDown(false);
    QVERIFY(!testWidget->isDown());

    testWidget->setDown(true);
    QVERIFY(testWidget->isDown());

    testWidget->setDown(true);
    QTest::keyClick(testWidget, Qt::Key_Escape);
    QVERIFY(!testWidget->isDown());
}

void tst_QPushButton::isChecked()
{
    testWidget->setDown(false);
    QVERIFY(!testWidget->isChecked());

    testWidget->setDown(true);
    QVERIFY(!testWidget->isChecked());

    testWidget->setDown(false);
    testWidget->toggle();
    QCOMPARE(testWidget->isChecked(), testWidget->isCheckable());
}

void tst_QPushButton::toggle()
{
    // the pushbutton shouldn't toggle the button.
    testWidget->toggle();
    QCOMPARE(testWidget->isChecked(), false);
}

void tst_QPushButton::toggled()
{
    // the pushbutton shouldn't send a toggled signal when we call the toggle slot.
    QVERIFY(!testWidget->isCheckable());

    testWidget->toggle();
    QCOMPARE(toggle_count, 0);

    // do it again, just to be sure
    resetCounters();
    testWidget->toggle();
    QCOMPARE(toggle_count, 0);

    // finally check that we can toggle using the mouse
    resetCounters();
    QTest::mousePress(testWidget, Qt::LeftButton);
    QCOMPARE(toggle_count, 0);
    QCOMPARE(click_count, 0);

    QTest::mouseRelease(testWidget, Qt::LeftButton);
    QCOMPARE(click_count, 1);
}

#if QT_CONFIG(shortcut)

/*
    If we press an accelerator key we ONLY get a pressed signal and
    NOT a released or clicked signal.
*/

void tst_QPushButton::setAccel()
{
    testWidget->setText("&AccelTest");
    QKeySequence seq(Qt::ALT | Qt::Key_A);
    testWidget->setShortcut(seq);

    // The shortcut will not be activated unless the button is in a active
    // window and has focus
    testWidget->setFocus();
    QVERIFY(QTest::qWaitForWindowActive(testWidget));
    QTest::keyClick(testWidget, 'A', Qt::AltModifier);
    QTRY_VERIFY(click_count == 1);
    QCOMPARE(press_count, 1);
    QCOMPARE(release_count, 1);
    QCOMPARE(toggle_count, 0);

    // wait 200 ms because setAccel uses animateClick.
    // if we don't wait this may screw up a next test.
    QTest::qWait(200);
    QTRY_VERIFY(!testWidget->isDown());
}

#endif // QT_CONFIG(shortcut)

void tst_QPushButton::clicked()
{
    QTest::mousePress(testWidget, Qt::LeftButton);
    QCOMPARE(press_count, 1);
    QCOMPARE(release_count, 0);

    QTest::mouseRelease(testWidget, Qt::LeftButton);
    QCOMPARE(press_count, 1);
    QCOMPARE(release_count, 1);

    press_count = 0;
    release_count = 0;
    testWidget->setDown(false);
    for (uint i=0; i<10; i++)
        QTest::mouseClick(testWidget, Qt::LeftButton);
    QCOMPARE(press_count, 10);
    QCOMPARE(release_count, 10);
}

void tst_QPushButton::touchTap()
{
    QTest::touchEvent(testWidget, m_touchScreen.get()).press(0, QPoint(10, 10));
    QCOMPARE(press_count, 1);
    QCOMPARE(release_count, 0);
    QTest::touchEvent(testWidget, m_touchScreen.get()).release(0, QPoint(10, 10));
    QCOMPARE(press_count, 1);
    QCOMPARE(release_count, 1);
    QCOMPARE(click_count, 1);

    press_count = 0;
    release_count = 0;
    click_count = 0;
    testWidget->setDown(false);
    for (uint i = 0; i < 10; i++) {
        QTest::touchEvent(testWidget, m_touchScreen.get()).press(0, QPoint(10, 10));
        QTest::touchEvent(testWidget, m_touchScreen.get()).release(0, QPoint(10, 10));
    }
    QCOMPARE(press_count, 10);
    QCOMPARE(release_count, 10);
    QCOMPARE(click_count, 10);
}

void tst_QPushButton::popupCrash()
{
    QPushButton *pb = new QPushButton("foo");
    QMenu *menu = new QMenu("bar", pb);
    pb->setMenu(menu);
    QTimer::singleShot(1000, this, [&pb]{
        delete pb;
        pb = nullptr;
    });
    pb->show();
    pb->click();
    QTRY_COMPARE(pb, nullptr);
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
    button2.setParent(nullptr);
    QVERIFY(!button2.autoDefault());
    button2.setAutoDefault(false);
    button2.setParent(&dialog);
    QVERIFY(!button2.autoDefault());

    button1.setAutoDefault(true);
    button1.setParent(nullptr);
    QVERIFY(button1.autoDefault());
    }
}

void tst_QPushButton::sizeHint_data()
{
    QTest::addColumn<QString>("stylename");
#if !defined(QT_NO_STYLE_WINDOWS)
    QTest::newRow("windows") << QString::fromLatin1("windows");
#endif
#if defined(Q_OS_MAC) && !defined(QT_NO_STYLE_MAC)
    QTest::newRow("macos") << QString::fromLatin1("macos");
#endif
#if !defined(QT_NO_STYLE_FUSION)
    QTest::newRow("fusion") << QString::fromLatin1("fusion");
#endif
#if defined(Q_OS_WIN) && !defined(QT_NO_STYLE_WINDOWSVISTA)
    QTest::newRow("windowsvista") << QString::fromLatin1("windowsvista");
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

        widget->setParent(nullptr);
        delete dialog;
        button->setDefault(false);
        QCOMPARE(button->sizeHint(), initSizeHint);
        delete button;

        delete widget;
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

        QTRY_COMPARE(button1_2->size(), button2_2->size());

        delete dialog;
    }
}

#if QT_CONFIG(shortcut)

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
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QApplicationPrivate::setActiveWindow(&dialog);

    // add shortcut '5' to button1 and test with keyboard and keypad '5' keys
    QSignalSpy spy1(button1, SIGNAL(clicked()));
    button1->setShortcut(Qt::Key_5);
    QTest::keyClick(&dialog, Qt::Key_5);
    QTest::qWait(300);
    QTest::keyClick(&dialog, Qt::Key_5, Qt::KeypadModifier);
    QTest::qWait(300);
    QCOMPARE(spy1.size(), 2);

    // add shortcut 'keypad 5' to button2
    spy1.clear();
    QSignalSpy spy2(button2, SIGNAL(clicked()));
    button2->setShortcut(Qt::Key_5 | Qt::KeypadModifier);
    QTest::keyClick(&dialog, Qt::Key_5);
    QTest::qWait(300);
    QTest::keyClick(&dialog, Qt::Key_5, Qt::KeypadModifier);
    QTest::qWait(300);
    QCOMPARE(spy1.size(), 1);
    QCOMPARE(spy2.size(), 1);

    // remove shortcut from button1
    spy1.clear();
    spy2.clear();
    button1->setShortcut(QKeySequence());
    QTest::keyClick(&dialog, Qt::Key_5);
    QTest::qWait(300);
    QTest::keyClick(&dialog, Qt::Key_5, Qt::KeypadModifier);
    QTest::qWait(300);
    QCOMPARE(spy1.size(), 0);
    QCOMPARE(spy2.size(), 1);
}

#endif // QT_CONFIG(shortcut)

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
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QApplicationPrivate::setActiveWindow(&dialog);
    button1->setFocus();

    QSignalSpy spy(button1, SIGNAL(released()));
    QTest::mousePress(button1, Qt::LeftButton);
    QVERIFY(button1->isDown());
    QTest::keyClick(&dialog, Qt::Key_Tab);
    QVERIFY(!button1->isDown());
    QCOMPARE(spy.size(), 1);
    spy.clear();

    QCOMPARE(spy.size(), 0);
    button1->setFocus();
    QTest::mousePress(button1, Qt::LeftButton);
    QVERIFY(button1->isDown());
    button1->setEnabled(false);
    QVERIFY(!button1->isDown());
    QCOMPARE(spy.size(), 1);
}

/*
    Test that QPushButton::hitButton returns true for points that
    are certainly inside the bevel, also when a style sheet is set.
*/
void tst_QPushButton::hitButton()
{
    class PushButton : public QPushButton
    {
    public:
        PushButton(const QString &text = {})
        : QPushButton(text)
        {}

        bool hitButton(const QPoint &point) const override
        {
            return QPushButton::hitButton(point);
        }
    };

    QDialog dialog;
    QVBoxLayout *layout = new QVBoxLayout;
    PushButton *button1 = new PushButton("Ok");
    PushButton *button2 = new PushButton("Cancel");
    button2->setStyleSheet("QPushButton {"
        "padding: 5px;"
        "margin: 5px;"
        "border-radius: 4px;"
        "border: 1px solid black; }"
    );

    layout->addWidget(button1);
    layout->addWidget(button2);

    dialog.setLayout(layout);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));

    const QPoint button1Center = button1->rect().center();
    QVERIFY(button1->hitButton(button1Center));

    const QPoint button2Center = button2->rect().center();
    QVERIFY(button2->hitButton(button2Center));
    QVERIFY(button2->hitButton(QPoint(6, 6)));
    QVERIFY(!button2->hitButton(QPoint(2, 2)));
}

/*
    Test that a style sheet with only icon doesn't crash.
    QTBUG-91735
*/
void tst_QPushButton::iconOnlyStyleSheet()
{
    QIcon icon(":/qt-project.org/styles/commonstyle/images/dvd-32.png");
    QVERIFY(!icon.isNull());
    QPushButton pb;
    pb.setStyleSheet("QPushButton {"
        "icon: url(:/qt-project.org/styles/commonstyle/images/dvd-32.png);"
        "border: red;"
    "}");
    pb.show();
    QVERIFY(QTest::qWaitForWindowExposed(&pb));
}

/*
    Test that mouse has been pressed,the signal is sent when moving the mouse.
    QTBUG-97937
*/
void tst_QPushButton::mousePressAndMove()
{
    QPushButton button;
    button.setGeometry(0, 0, 20, 20);
    QSignalSpy pressSpy(&button, &QAbstractButton::pressed);
    QSignalSpy releaseSpy(&button, &QAbstractButton::released);

    QTest::mousePress(&button, Qt::LeftButton);
    QCOMPARE(pressSpy.size(), 1);
    QCOMPARE(releaseSpy.size(), 0);

    // mouse pressed and moving out
    QTest::mouseMove(&button, QPoint(100, 100));

    // should emit released signal when the mouse is dragged out of boundary
    QCOMPARE(pressSpy.size(), 1);
    QCOMPARE(releaseSpy.size(), 1);

    // mouse pressed and moving into
    QTest::mouseMove(&button, QPoint(10, 10));

    // should emit pressed signal when the mouse is dragged into of boundary
    QCOMPARE(pressSpy.size(), 2);
    QCOMPARE(releaseSpy.size(), 1);
}

/*
    Test checking that a QPushButton with a QMenu has a sunken style only
    when the menu is open
    QTBUG-120976
*/
void tst_QPushButton::reactToMenuClosed()
{
    // create a subclass of QPushButton to expose the initStyleOption method
    class PushButton : public QPushButton {
    public:
        virtual void initStyleOption(QStyleOptionButton *option) const override
        {
            QPushButton::initStyleOption(option);
        }
    };

    PushButton button;
    QStyleOptionButton opt;
    QMenu menu;

    // add a menu to the button
    menu.addAction(tr("string"));
    button.setMenu(&menu);

    // give the button a size and show it
    button.setGeometry(0, 0, 50, 50);
    button.show();
    QVERIFY(QTest::qWaitForWindowExposed(&button));

    // click the button to open the menu
    QTest::mouseClick(&button, Qt::LeftButton);

    // check the menu is visible and the button style is sunken
    QTRY_VERIFY(menu.isVisible());
    button.initStyleOption(&opt);
    QVERIFY(opt.state.testFlag(QStyle::StateFlag::State_Sunken));

    // close the menu
    menu.close();

    // check the menu isn't visible and the style isn't sunken
    QTRY_VERIFY(!menu.isVisible());
    button.initStyleOption(&opt);
    QVERIFY(!opt.state.testFlag(QStyle::StateFlag::State_Sunken));
}

QTEST_MAIN(tst_QPushButton)
#include "tst_qpushbutton.moc"
