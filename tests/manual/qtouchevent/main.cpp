// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QApplication>
#include <QTest>
#include <QTimer>

#include "ui_form.h"
#include "touchwidget.h"

class MultitouchTestWidget : public QWidget, public Ui::Form
{
    Q_OBJECT

public:
    MultitouchTestWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_QuitOnClose, false);
        setupUi(this);
        setWindowTitle(QT_VERSION_STR);
    }

    void closeEvent(QCloseEvent *event)
    {
        event->accept();
        QTimer::singleShot(1000, qApp, SLOT(quit()));
    }
};

class tst_ManualMultitouch : public QObject
{
    Q_OBJECT

public:
    tst_ManualMultitouch();
    ~tst_ManualMultitouch();

private slots:
    void ignoringTouchEventsEmulatesMouseEvents();
    void basicSingleTouchEventHandling();
    void basicMultiTouchEventHandling();
    void acceptingTouchBeginStopsPropagation();
    void ignoringTouchBeginPropagatesToParent();
    void secondTouchPointOnParentGoesToChild();
    void secondTouchPointOnChildGoesToParent();
    void secondTouchPointOnSiblingGoesToSibling();
    void secondTouchPointOnCousinGoesToCousin();
};

tst_ManualMultitouch::tst_ManualMultitouch()
{ }

tst_ManualMultitouch::~tst_ManualMultitouch()
{ }

void tst_ManualMultitouch::ignoringTouchEventsEmulatesMouseEvents()
{
    // first, make sure that we get mouse events when not enabling touch events
    MultitouchTestWidget testWidget;
    testWidget.testNameLabel->setText("Mouse Event Emulation Test");
    testWidget.testDescriptionLabel->setText("Touch, hold, and release your finger on the green widget.");
    testWidget.redWidget->hide();
    testWidget.blueWidget->hide();
    testWidget.greenWidget->closeWindowOnMouseRelease = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(!testWidget.greenWidget->seenTouchBegin);
    // QVERIFY(!testWidget.greenWidget->seenTouchUpdate);
    QVERIFY(!testWidget.greenWidget->seenTouchEnd);
    QVERIFY(testWidget.greenWidget->seenMousePress);
    // QVERIFY(testWidget.greenWidget->seenMouseMove);
    QVERIFY(testWidget.greenWidget->seenMouseRelease);

    // enable touch, but don't accept the events
    testWidget.greenWidget->reset();
    testWidget.greenWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    testWidget.greenWidget->closeWindowOnMouseRelease = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.greenWidget->seenTouchBegin);
    // QVERIFY(!testWidget.greenWidget->seenTouchUpdate);
    QVERIFY(!testWidget.greenWidget->seenTouchEnd);
    QVERIFY(testWidget.greenWidget->seenMousePress);
    // QVERIFY(testWidget.greenWidget->seenMouseMove);
    QVERIFY(testWidget.greenWidget->seenMouseRelease);
}

void tst_ManualMultitouch::basicSingleTouchEventHandling()
{
    // now enable touch and make sure we get the touch events
    MultitouchTestWidget testWidget;
    testWidget.testNameLabel->setText("Basic Single-Touch Event Handling Test");
    testWidget.testDescriptionLabel->setText("Touch, hold, and release your finger on the green widget.");
    testWidget.redWidget->hide();
    testWidget.blueWidget->hide();
    testWidget.greenWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    testWidget.greenWidget->acceptTouchBegin = true;
    testWidget.greenWidget->acceptTouchUpdate = true;
    testWidget.greenWidget->acceptTouchEnd = true;
    testWidget.greenWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.greenWidget->seenTouchBegin);
    // QVERIFY(testWidget.greenWidget->seenTouchUpdate);
    QVERIFY(testWidget.greenWidget->seenTouchEnd);
    QVERIFY(!testWidget.greenWidget->seenMousePress);
    QVERIFY(!testWidget.greenWidget->seenMouseMove);
    QVERIFY(!testWidget.greenWidget->seenMouseRelease);

    // again, ignoring the TouchEnd
    testWidget.greenWidget->reset();
    testWidget.greenWidget->acceptTouchBegin = true;
    testWidget.greenWidget->acceptTouchUpdate = true;
    // testWidget.greenWidget->acceptTouchEnd = true;
    testWidget.greenWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.greenWidget->seenTouchBegin);
    // QVERIFY(testWidget.greenWidget->seenTouchUpdate);
    QVERIFY(testWidget.greenWidget->seenTouchEnd);
    QVERIFY(!testWidget.greenWidget->seenMousePress);
    QVERIFY(!testWidget.greenWidget->seenMouseMove);
    QVERIFY(!testWidget.greenWidget->seenMouseRelease);

    // again, ignoring TouchUpdates
    testWidget.greenWidget->reset();
    testWidget.greenWidget->acceptTouchBegin = true;
    // testWidget.greenWidget->acceptTouchUpdate = true;
    testWidget.greenWidget->acceptTouchEnd = true;
    testWidget.greenWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.greenWidget->seenTouchBegin);
    // QVERIFY(testWidget.greenWidget->seenTouchUpdate);
    QVERIFY(testWidget.greenWidget->seenTouchEnd);
    QVERIFY(!testWidget.greenWidget->seenMousePress);
    QVERIFY(!testWidget.greenWidget->seenMouseMove);
    QVERIFY(!testWidget.greenWidget->seenMouseRelease);

    // last time, ignoring TouchUpdates and TouchEnd
    testWidget.greenWidget->reset();
    testWidget.greenWidget->acceptTouchBegin = true;
    // testWidget.greenWidget->acceptTouchUpdate = true;
    // testWidget.greenWidget->acceptTouchEnd = true;
    testWidget.greenWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.greenWidget->seenTouchBegin);
    // QVERIFY(testWidget.greenWidget->seenTouchUpdate);
    QVERIFY(testWidget.greenWidget->seenTouchEnd);
    QVERIFY(!testWidget.greenWidget->seenMousePress);
    QVERIFY(!testWidget.greenWidget->seenMouseMove);
    QVERIFY(!testWidget.greenWidget->seenMouseRelease);
}

void tst_ManualMultitouch::basicMultiTouchEventHandling()
{
    // repeat, this time looking for multiple fingers
    MultitouchTestWidget testWidget;
    testWidget.testNameLabel->setText("Basic Multi-Touch Event Handling Test");
    testWidget.testDescriptionLabel->setText("Touch, hold, and release several fingers on the red widget.");
    testWidget.greenWidget->hide();
    testWidget.greyWidget->hide();
    testWidget.redWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    testWidget.redWidget->acceptTouchBegin = true;
    testWidget.redWidget->acceptTouchUpdate = true;
    testWidget.redWidget->acceptTouchEnd = true;
    testWidget.redWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.redWidget->seenTouchBegin);
    QVERIFY(testWidget.redWidget->seenTouchUpdate);
    QVERIFY(testWidget.redWidget->seenTouchEnd);
    QVERIFY(testWidget.redWidget->touchPointCount > 1);
    QVERIFY(!testWidget.greenWidget->seenMousePress);
    QVERIFY(!testWidget.greenWidget->seenMouseMove);
    QVERIFY(!testWidget.greenWidget->seenMouseRelease);

    testWidget.redWidget->reset();
    testWidget.redWidget->acceptTouchBegin = true;
    // testWidget.redWidget->acceptTouchUpdate = true;
    testWidget.redWidget->acceptTouchEnd = true;
    testWidget.redWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.redWidget->seenTouchBegin);
    QVERIFY(testWidget.redWidget->seenTouchUpdate);
    QVERIFY(testWidget.redWidget->seenTouchEnd);
    QVERIFY(testWidget.redWidget->touchPointCount > 1);
    QVERIFY(!testWidget.greenWidget->seenMousePress);
    QVERIFY(!testWidget.greenWidget->seenMouseMove);
    QVERIFY(!testWidget.greenWidget->seenMouseRelease);

    testWidget.redWidget->reset();
    testWidget.redWidget->acceptTouchBegin = true;
    testWidget.redWidget->acceptTouchUpdate = true;
    // testWidget.redWidget->acceptTouchEnd = true;
    testWidget.redWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.redWidget->seenTouchBegin);
    QVERIFY(testWidget.redWidget->seenTouchUpdate);
    QVERIFY(testWidget.redWidget->seenTouchEnd);
    QVERIFY(testWidget.redWidget->touchPointCount > 1);
    QVERIFY(!testWidget.greenWidget->seenMousePress);
    QVERIFY(!testWidget.greenWidget->seenMouseMove);
    QVERIFY(!testWidget.greenWidget->seenMouseRelease);

    testWidget.redWidget->reset();
    testWidget.redWidget->acceptTouchBegin = true;
    // testWidget.redWidget->acceptTouchUpdate = true;
    // testWidget.redWidget->acceptTouchEnd = true;
    testWidget.redWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.redWidget->seenTouchBegin);
    QVERIFY(testWidget.redWidget->seenTouchUpdate);
    QVERIFY(testWidget.redWidget->seenTouchEnd);
    QVERIFY(testWidget.redWidget->touchPointCount > 1);
    QVERIFY(!testWidget.greenWidget->seenMousePress);
    QVERIFY(!testWidget.greenWidget->seenMouseMove);
    QVERIFY(!testWidget.greenWidget->seenMouseRelease);
}

void tst_ManualMultitouch::acceptingTouchBeginStopsPropagation()
{
    // test that accepting the TouchBegin event on the
    // blueWidget stops propagation to the greenWidget
    MultitouchTestWidget testWidget;
    testWidget.testNameLabel->setText("Touch Event Propagation Test: Accepting in Blue Blocks Green");
    testWidget.testDescriptionLabel->setText("Touch, hold, and release your finger on the blue widget.");
    testWidget.redWidget->hide();
    testWidget.blueWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    testWidget.greenWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    testWidget.blueWidget->acceptTouchBegin = true;
    testWidget.blueWidget->acceptTouchUpdate = true;
    testWidget.blueWidget->acceptTouchEnd = true;
    testWidget.blueWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.blueWidget->seenTouchBegin);
    // QVERIFY(testWidget.blueWidget->seenTouchUpdate);
    QVERIFY(testWidget.blueWidget->seenTouchEnd);
    QVERIFY(!testWidget.blueWidget->seenMousePress);
    QVERIFY(!testWidget.blueWidget->seenMouseMove);
    QVERIFY(!testWidget.blueWidget->seenMouseRelease);
    QVERIFY(!testWidget.greenWidget->seenTouchBegin);
    QVERIFY(!testWidget.greenWidget->seenTouchUpdate);
    QVERIFY(!testWidget.greenWidget->seenTouchEnd);
    QVERIFY(!testWidget.greenWidget->seenMousePress);
    QVERIFY(!testWidget.greenWidget->seenMouseMove);
    QVERIFY(!testWidget.greenWidget->seenMouseRelease);

    // ignoring TouchEnd
    testWidget.blueWidget->reset();
    testWidget.greenWidget->reset();
    testWidget.blueWidget->acceptTouchBegin = true;
    testWidget.blueWidget->acceptTouchUpdate = true;
    // testWidget.blueWidget->acceptTouchEnd = true;
    testWidget.blueWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.blueWidget->seenTouchBegin);
    // QVERIFY(testWidget.blueWidget->seenTouchUpdate);
    QVERIFY(testWidget.blueWidget->seenTouchEnd);
    QVERIFY(!testWidget.blueWidget->seenMousePress);
    QVERIFY(!testWidget.blueWidget->seenMouseMove);
    QVERIFY(!testWidget.blueWidget->seenMouseRelease);
    QVERIFY(!testWidget.greenWidget->seenTouchBegin);
    QVERIFY(!testWidget.greenWidget->seenTouchUpdate);
    QVERIFY(!testWidget.greenWidget->seenTouchEnd);
    QVERIFY(!testWidget.greenWidget->seenMousePress);
    QVERIFY(!testWidget.greenWidget->seenMouseMove);
    QVERIFY(!testWidget.greenWidget->seenMouseRelease);

    // ignoring TouchUpdate
    testWidget.blueWidget->reset();
    testWidget.greenWidget->reset();
    testWidget.blueWidget->acceptTouchBegin = true;
    // testWidget.blueWidget->acceptTouchUpdate = true;
    testWidget.blueWidget->acceptTouchEnd = true;
    testWidget.blueWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.blueWidget->seenTouchBegin);
    // QVERIFY(testWidget.blueWidget->seenTouchUpdate);
    QVERIFY(testWidget.blueWidget->seenTouchEnd);
    QVERIFY(!testWidget.blueWidget->seenMousePress);
    QVERIFY(!testWidget.blueWidget->seenMouseMove);
    QVERIFY(!testWidget.blueWidget->seenMouseRelease);
    QVERIFY(!testWidget.greenWidget->seenTouchBegin);
    QVERIFY(!testWidget.greenWidget->seenTouchUpdate);
    QVERIFY(!testWidget.greenWidget->seenTouchEnd);
    QVERIFY(!testWidget.greenWidget->seenMousePress);
    QVERIFY(!testWidget.greenWidget->seenMouseMove);
    QVERIFY(!testWidget.greenWidget->seenMouseRelease);

    // ignoring TouchUpdate and TouchEnd
    testWidget.blueWidget->reset();
    testWidget.greenWidget->reset();
    testWidget.blueWidget->acceptTouchBegin = true;
    // testWidget.blueWidget->acceptTouchUpdate = true;
    // testWidget.blueWidget->acceptTouchEnd = true;
    testWidget.blueWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.blueWidget->seenTouchBegin);
    // QVERIFY(testWidget.blueWidget->seenTouchUpdate);
    QVERIFY(testWidget.blueWidget->seenTouchEnd);
    QVERIFY(!testWidget.blueWidget->seenMousePress);
    QVERIFY(!testWidget.blueWidget->seenMouseMove);
    QVERIFY(!testWidget.blueWidget->seenMouseRelease);
    QVERIFY(!testWidget.greenWidget->seenTouchBegin);
    QVERIFY(!testWidget.greenWidget->seenTouchUpdate);
    QVERIFY(!testWidget.greenWidget->seenTouchEnd);
    QVERIFY(!testWidget.greenWidget->seenMousePress);
    QVERIFY(!testWidget.greenWidget->seenMouseMove);
    QVERIFY(!testWidget.greenWidget->seenMouseRelease);
}

void tst_ManualMultitouch::ignoringTouchBeginPropagatesToParent()
{
    // repeat the test above, now ignoring touch events in the
    // greyWidget, they should be propagated to the redWidget
    MultitouchTestWidget testWidget;
    testWidget.testNameLabel->setText("Touch Event Propagation Test: Ignoring in Grey Propagates to Red");
    testWidget.testDescriptionLabel->setText("Touch, hold, and release your finger on the grey widget.");
    testWidget.greenWidget->hide();
    testWidget.greyWidget->setAttribute(Qt::WA_AcceptTouchEvents, false);
    testWidget.redWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    testWidget.greyWidget->reset();
    testWidget.redWidget->reset();
    testWidget.redWidget->acceptTouchBegin = true;
    testWidget.redWidget->acceptTouchUpdate = true;
    testWidget.redWidget->acceptTouchEnd = true;
    testWidget.redWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(!testWidget.greyWidget->seenTouchBegin);
    QVERIFY(!testWidget.greyWidget->seenTouchUpdate);
    QVERIFY(!testWidget.greyWidget->seenTouchEnd);
    QVERIFY(!testWidget.greyWidget->seenMousePress);
    QVERIFY(!testWidget.greyWidget->seenMouseMove);
    QVERIFY(!testWidget.greyWidget->seenMouseRelease);
    QVERIFY(testWidget.redWidget->seenTouchBegin);
    // QVERIFY(testWidget.redWidget->seenTouchUpdate);
    QVERIFY(testWidget.redWidget->seenTouchEnd);
    QVERIFY(!testWidget.redWidget->seenMousePress);
    QVERIFY(!testWidget.redWidget->seenMouseMove);
    QVERIFY(!testWidget.redWidget->seenMouseRelease);

    // again, but this time greyWidget should see the TouchBegin
    testWidget.greyWidget->reset();
    testWidget.redWidget->reset();
    testWidget.greyWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    testWidget.redWidget->acceptTouchBegin = true;
    testWidget.redWidget->acceptTouchUpdate = true;
    testWidget.redWidget->acceptTouchEnd = true;
    testWidget.redWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.greyWidget->seenTouchBegin);
    QVERIFY(!testWidget.greyWidget->seenTouchUpdate);
    QVERIFY(!testWidget.greyWidget->seenTouchEnd);
    QVERIFY(!testWidget.greyWidget->seenMousePress);
    QVERIFY(!testWidget.greyWidget->seenMouseMove);
    QVERIFY(!testWidget.greyWidget->seenMouseRelease);
    QVERIFY(testWidget.redWidget->seenTouchBegin);
    // QVERIFY(testWidget.redWidget->seenTouchUpdate);
    QVERIFY(testWidget.redWidget->seenTouchEnd);
    QVERIFY(!testWidget.redWidget->seenMousePress);
    QVERIFY(!testWidget.redWidget->seenMouseMove);
    QVERIFY(!testWidget.redWidget->seenMouseRelease);

    // again, ignoring the TouchEnd
    testWidget.greyWidget->reset();
    testWidget.redWidget->reset();
    testWidget.redWidget->acceptTouchBegin = true;
    testWidget.redWidget->acceptTouchUpdate = true;
    // testWidget.redWidget->acceptTouchEnd = true;
    testWidget.redWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.greyWidget->seenTouchBegin);
    QVERIFY(!testWidget.greyWidget->seenTouchUpdate);
    QVERIFY(!testWidget.greyWidget->seenTouchEnd);
    QVERIFY(!testWidget.greyWidget->seenMousePress);
    QVERIFY(!testWidget.greyWidget->seenMouseMove);
    QVERIFY(!testWidget.greyWidget->seenMouseRelease);
    QVERIFY(testWidget.redWidget->seenTouchBegin);
    // QVERIFY(testWidget.redWidget->seenTouchUpdate);
    QVERIFY(testWidget.redWidget->seenTouchEnd);
    QVERIFY(!testWidget.redWidget->seenMousePress);
    QVERIFY(!testWidget.redWidget->seenMouseMove);
    QVERIFY(!testWidget.redWidget->seenMouseRelease);

    // again, ignoring TouchUpdates
    testWidget.greyWidget->reset();
    testWidget.redWidget->reset();
    testWidget.redWidget->acceptTouchBegin = true;
    // testWidget.redWidget->acceptTouchUpdate = true;
    testWidget.redWidget->acceptTouchEnd = true;
    testWidget.redWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.greyWidget->seenTouchBegin);
    QVERIFY(!testWidget.greyWidget->seenTouchUpdate);
    QVERIFY(!testWidget.greyWidget->seenTouchEnd);
    QVERIFY(!testWidget.greyWidget->seenMousePress);
    QVERIFY(!testWidget.greyWidget->seenMouseMove);
    QVERIFY(!testWidget.greyWidget->seenMouseRelease);
    QVERIFY(testWidget.redWidget->seenTouchBegin);
    // QVERIFY(testWidget.redWidget->seenTouchUpdate);
    QVERIFY(testWidget.redWidget->seenTouchEnd);
    QVERIFY(!testWidget.redWidget->seenMousePress);
    QVERIFY(!testWidget.redWidget->seenMouseMove);
    QVERIFY(!testWidget.redWidget->seenMouseRelease);

    // last time, ignoring TouchUpdates and TouchEnd
    testWidget.greyWidget->reset();
    testWidget.redWidget->reset();
    testWidget.redWidget->acceptTouchBegin = true;
    // testWidget.redWidget->acceptTouchUpdate = true;
    // testWidget.redWidget->acceptTouchEnd = true;
    testWidget.redWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.greyWidget->seenTouchBegin);
    QVERIFY(!testWidget.greyWidget->seenTouchUpdate);
    QVERIFY(!testWidget.greyWidget->seenTouchEnd);
    QVERIFY(!testWidget.greyWidget->seenMousePress);
    QVERIFY(!testWidget.greyWidget->seenMouseMove);
    QVERIFY(!testWidget.blueWidget->seenMouseRelease);
    QVERIFY(testWidget.redWidget->seenTouchBegin);
    // QVERIFY(testWidget.redWidget->seenTouchUpdate);
    QVERIFY(testWidget.redWidget->seenTouchEnd);
    QVERIFY(!testWidget.redWidget->seenMousePress);
    QVERIFY(!testWidget.redWidget->seenMouseMove);
    QVERIFY(!testWidget.redWidget->seenMouseRelease);
}

void tst_ManualMultitouch::secondTouchPointOnParentGoesToChild()
{
    MultitouchTestWidget testWidget;
    testWidget.testNameLabel->setText("Additional Touch-Points Outside Child's Rect Go to Child");
    testWidget.testDescriptionLabel->setText("Press and hold a finger on the blue widget, then on the green one, and release.");
    testWidget.redWidget->hide();
    testWidget.greenWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    testWidget.blueWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    testWidget.blueWidget->acceptTouchBegin = true;
    testWidget.greenWidget->acceptTouchBegin = true;
    testWidget.blueWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.blueWidget->seenTouchBegin);
    // QVERIFY(testWidget.blueWidget->seenTouchUpdate);
    QVERIFY(testWidget.blueWidget->seenTouchEnd);
    QVERIFY(!testWidget.blueWidget->seenMousePress);
    QVERIFY(!testWidget.blueWidget->seenMouseMove);
    QVERIFY(!testWidget.blueWidget->seenMouseRelease);
    QVERIFY(testWidget.blueWidget->touchPointCount > 1);
    QVERIFY(!testWidget.greenWidget->seenTouchBegin);
    QVERIFY(!testWidget.greenWidget->seenTouchUpdate);
    QVERIFY(!testWidget.greenWidget->seenTouchEnd);
    QVERIFY(!testWidget.greenWidget->seenMousePress);
    QVERIFY(!testWidget.greenWidget->seenMouseMove);
    QVERIFY(!testWidget.greenWidget->seenMouseRelease);
}

void tst_ManualMultitouch::secondTouchPointOnChildGoesToParent()
{
    MultitouchTestWidget testWidget;
    testWidget.testNameLabel->setText("Additional Touch-Points Over Child's Rect Go to Parent");
    testWidget.testDescriptionLabel->setText("Press and hold a finger on the red widget, then on the grey one, and release.");
    testWidget.greenWidget->hide();
    testWidget.redWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    testWidget.greyWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    testWidget.greyWidget->acceptTouchBegin = true;
    testWidget.redWidget->acceptTouchBegin = true;
    testWidget.redWidget->closeWindowOnTouchEnd = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.redWidget->seenTouchBegin);
    // QVERIFY(testWidget.redWidget->seenTouchUpdate);
    QVERIFY(testWidget.redWidget->seenTouchEnd);
    QVERIFY(!testWidget.redWidget->seenMousePress);
    QVERIFY(!testWidget.redWidget->seenMouseMove);
    QVERIFY(!testWidget.redWidget->seenMouseRelease);
    QVERIFY(testWidget.redWidget->touchPointCount > 1);
    QVERIFY(!testWidget.greyWidget->seenTouchBegin);
    QVERIFY(!testWidget.greyWidget->seenTouchUpdate);
    QVERIFY(!testWidget.greyWidget->seenTouchEnd);
    QVERIFY(!testWidget.greyWidget->seenMousePress);
    QVERIFY(!testWidget.greyWidget->seenMouseMove);
    QVERIFY(!testWidget.greyWidget->seenMouseRelease);
}

void tst_ManualMultitouch::secondTouchPointOnSiblingGoesToSibling()
{
    MultitouchTestWidget testWidget;
    testWidget.testNameLabel->setText("Multi-Touch Interaction Test, Unrelated Widgets Get Separate Events");
    testWidget.testDescriptionLabel->setText("Press and hold a finger on the green widget, then the red one, and release.");
    testWidget.blueWidget->hide();
    testWidget.greenWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    testWidget.greenWidget->acceptTouchBegin = true;
    testWidget.greenWidget->closeWindowOnTouchEnd = true;
    testWidget.greyWidget->hide();
    testWidget.redWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    testWidget.redWidget->acceptTouchBegin = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.greenWidget->seenTouchBegin);
    // QVERIFY(testWidget.greenWidget->seenTouchUpdate);
    QVERIFY(testWidget.greenWidget->seenTouchEnd);
    QVERIFY(!testWidget.greenWidget->seenMousePress);
    QVERIFY(!testWidget.greenWidget->seenMouseMove);
    QVERIFY(!testWidget.greenWidget->seenMouseRelease);
    QVERIFY(testWidget.redWidget->seenTouchBegin);
    // QVERIFY(testWidget.redWidget->seenTouchUpdate);
    QVERIFY(testWidget.redWidget->seenTouchEnd);
    QVERIFY(!testWidget.redWidget->seenMousePress);
    QVERIFY(!testWidget.redWidget->seenMouseMove);
    QVERIFY(!testWidget.redWidget->seenMouseRelease);
    QVERIFY(testWidget.greenWidget->touchPointCount == 1);
    QVERIFY(testWidget.redWidget->touchPointCount == 1);
}

void tst_ManualMultitouch::secondTouchPointOnCousinGoesToCousin()
{
    MultitouchTestWidget testWidget;
    testWidget.testNameLabel->setText("Multi-Touch Interaction Test, Unrelated Widgets Get Separate Events");
    testWidget.testDescriptionLabel->setText("Press and hold a finger on the blue widget, then the grey one, and release.");
    testWidget.blueWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    testWidget.blueWidget->acceptTouchBegin = true;
    testWidget.blueWidget->closeWindowOnTouchEnd = true;
    testWidget.greyWidget->setAttribute(Qt::WA_AcceptTouchEvents);
    testWidget.greyWidget->acceptTouchBegin = true;
    testWidget.showMaximized();
    (void) qApp->exec();
    QVERIFY(testWidget.blueWidget->seenTouchBegin);
    // QVERIFY(testWidget.blueWidget->seenTouchUpdate);
    QVERIFY(testWidget.blueWidget->seenTouchEnd);
    QVERIFY(!testWidget.blueWidget->seenMousePress);
    QVERIFY(!testWidget.blueWidget->seenMouseMove);
    QVERIFY(!testWidget.blueWidget->seenMouseRelease);
    QVERIFY(testWidget.greyWidget->seenTouchBegin);
    // QVERIFY(testWidget.greyWidget->seenTouchUpdate);
    QVERIFY(testWidget.greyWidget->seenTouchEnd);
    QVERIFY(!testWidget.greyWidget->seenMousePress);
    QVERIFY(!testWidget.greyWidget->seenMouseMove);
    QVERIFY(!testWidget.greyWidget->seenMouseRelease);
    QVERIFY(testWidget.blueWidget->touchPointCount == 1);
    QVERIFY(testWidget.greyWidget->touchPointCount == 1);
}

QTEST_MAIN(tst_ManualMultitouch)

#include "main.moc"
