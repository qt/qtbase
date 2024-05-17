// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
#include <qevent.h>
#include <qwindow.h>
#include <QtGui/private/qpointingdevice_p.h>

#include <QtCore/qpointer.h>

Q_LOGGING_CATEGORY(lcTests, "qt.gui.tests")

class MouseEventWidget : public QWindow
{
public:
    MouseEventWidget(QWindow *parent = nullptr) : QWindow(parent)
    {
        setGeometry(100, 100, 100, 100);
    }
    bool grabExclusive = false;
    bool grabPassive = false;
    bool mousePressEventRecieved;
    bool mouseReleaseEventRecieved;
    int mousePressButton;
    int mousePressButtons;
    int mousePressModifiers;
    int mouseReleaseButton;
    int mouseReleaseButtons;
    int mouseReleaseModifiers;
    ulong timestamp;
    ulong pressTimestamp;
    ulong lastTimestamp;
    QVector2D velocity;
protected:
    void mousePressEvent(QMouseEvent *e) override
    {
        const auto &firstPoint = e->point(0);
        qCDebug(lcTests) << e << firstPoint;
        timestamp = firstPoint.timestamp();
        lastTimestamp = firstPoint.lastTimestamp();
        if (e->type() == QEvent::MouseButtonPress) {
            auto firstPoint = e->points().first();
            QCOMPARE(e->exclusiveGrabber(firstPoint), nullptr);
            QVERIFY(e->passiveGrabbers(firstPoint).isEmpty());
            QCOMPARE(firstPoint.timeHeld(), 0);
            QCOMPARE(firstPoint.pressTimestamp(), e->timestamp());
            pressTimestamp = e->timestamp();
        }
        QWindow::mousePressEvent(e);
        mousePressButton = e->button();
        mousePressButtons = e->buttons();
        mousePressModifiers = e->modifiers();
        mousePressEventRecieved = true;
        e->accept();
        // It's not normal for QWindow to be the grabber, but that's easier to test
        // without needing to create child ojects.
        if (grabExclusive)
            e->setExclusiveGrabber(firstPoint, this);
        if (grabPassive)
            e->addPassiveGrabber(firstPoint, this);
    }
    void mouseMoveEvent(QMouseEvent *e) override
    {
        qCDebug(lcTests) << e << e->points().first();
        timestamp = e->points().first().timestamp();
        lastTimestamp = e->points().first().lastTimestamp();
        velocity = e->points().first().velocity();
    }
    void mouseReleaseEvent(QMouseEvent *e) override
    {
        qCDebug(lcTests) << e << e->points().first();
        QWindow::mouseReleaseEvent(e);
        mouseReleaseButton = e->button();
        mouseReleaseButtons = e->buttons();
        mouseReleaseModifiers = e->modifiers();
        timestamp = e->points().first().timestamp();
        lastTimestamp = e->points().first().lastTimestamp();
        velocity = e->points().first().velocity();
        mouseReleaseEventRecieved = true;
        e->accept();
    }
};

class tst_QMouseEvent : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
private slots:
    void mouseEventBasic();
    void checkMousePressEvent_data();
    void checkMousePressEvent();
    void checkMouseReleaseEvent_data();
    void checkMouseReleaseEvent();
    void grabbers_data();
    void grabbers();
    void velocity();
    void clone();

private:
    MouseEventWidget* testMouseWidget;
};

void tst_QMouseEvent::initTestCase()
{
    testMouseWidget = new MouseEventWidget;
    testMouseWidget->show();
}

void tst_QMouseEvent::cleanupTestCase()
{
    delete testMouseWidget;
}

void tst_QMouseEvent::init()
{
    testMouseWidget->mousePressEventRecieved = false;
    testMouseWidget->mouseReleaseEventRecieved = false;
    testMouseWidget->mousePressButton = 0;
    testMouseWidget->mousePressButtons = 0;
    testMouseWidget->mousePressModifiers = 0;
    testMouseWidget->mouseReleaseButton = 0;
    testMouseWidget->mouseReleaseButtons = 0;
    testMouseWidget->mouseReleaseModifiers = 0;
}

void tst_QMouseEvent::mouseEventBasic()
{
    QPointF local(100, 100);
    QPointF scene(200, 200);
    QPointF screen(300, 300);
    // Press left button
    QMouseEvent me(QEvent::MouseButtonPress, local, scene, screen, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QVERIFY(me.isInputEvent());
    QVERIFY(me.isPointerEvent());
    QVERIFY(me.isSinglePointEvent());
    QCOMPARE(me.isAccepted(), true);
    QCOMPARE(me.button(), Qt::LeftButton);
    QCOMPARE(me.buttons(), Qt::LeftButton);
    QVERIFY(me.isBeginEvent());
    QVERIFY(!me.isEndEvent());
    QCOMPARE(me.position(), local);
    QCOMPARE(me.scenePosition(), scene);
    QCOMPARE(me.globalPosition(), screen);
    // Press right button while left is already pressed
    QMouseEvent me2(QEvent::MouseButtonPress, local, scene, screen, Qt::RightButton, Qt::LeftButton | Qt::RightButton, Qt::NoModifier);
    QVERIFY(me2.isBeginEvent());
    QVERIFY(!me2.isEndEvent());
    // Release right button while left is still pressed
    QMouseEvent me3 = QMouseEvent(QEvent::MouseButtonRelease, local, scene, screen, Qt::RightButton, Qt::LeftButton, Qt::NoModifier);
    QVERIFY(!me3.isBeginEvent());
    QVERIFY(me3.isEndEvent());
    // Release left button in the usual way
    QMouseEvent me4 = QMouseEvent(QEvent::MouseButtonRelease, local, scene, screen, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QVERIFY(!me4.isBeginEvent());
    QVERIFY(me4.isEndEvent());
}

void tst_QMouseEvent::checkMousePressEvent_data()
{
    QTest::addColumn<int>("buttonPressed");
    QTest::addColumn<int>("keyPressed");

    QTest::newRow("leftButton-nokey") << int(Qt::LeftButton) << int(Qt::NoButton);
    QTest::newRow("leftButton-shiftkey") << int(Qt::LeftButton) << int(Qt::ShiftModifier);
    QTest::newRow("leftButton-controlkey") << int(Qt::LeftButton) << int(Qt::ControlModifier);
    QTest::newRow("leftButton-altkey") << int(Qt::LeftButton) << int(Qt::AltModifier);
    QTest::newRow("leftButton-metakey") << int(Qt::LeftButton) << int(Qt::MetaModifier);
    QTest::newRow("rightButton-nokey") << int(Qt::RightButton) << int(Qt::NoButton);
    QTest::newRow("rightButton-shiftkey") << int(Qt::RightButton) << int(Qt::ShiftModifier);
    QTest::newRow("rightButton-controlkey") << int(Qt::RightButton) << int(Qt::ControlModifier);
    QTest::newRow("rightButton-altkey") << int(Qt::RightButton) << int(Qt::AltModifier);
    QTest::newRow("rightButton-metakey") << int(Qt::RightButton) << int(Qt::MetaModifier);
    QTest::newRow("middleButton-nokey") << int(Qt::MiddleButton) << int(Qt::NoButton);
    QTest::newRow("middleButton-shiftkey") << int(Qt::MiddleButton) << int(Qt::ShiftModifier);
    QTest::newRow("middleButton-controlkey") << int(Qt::MiddleButton) << int(Qt::ControlModifier);
    QTest::newRow("middleButton-altkey") << int(Qt::MiddleButton) << int(Qt::AltModifier);
    QTest::newRow("middleButton-metakey") << int(Qt::MiddleButton) << int(Qt::MetaModifier);
}

void tst_QMouseEvent::checkMousePressEvent()
{
    QFETCH(int,buttonPressed);
    QFETCH(int,keyPressed);
    int button = buttonPressed;
    int buttons = button;
    int modifiers = keyPressed;

    QTest::mousePress(testMouseWidget, Qt::MouseButton(buttonPressed), Qt::KeyboardModifiers(keyPressed));
    qApp->processEvents();
    QVERIFY(testMouseWidget->mousePressEventRecieved);
    QCOMPARE(testMouseWidget->mousePressButton, button);
    QCOMPARE(testMouseWidget->mousePressButtons, buttons);
    QCOMPARE(testMouseWidget->mousePressModifiers, modifiers);

    QTest::mouseRelease(testMouseWidget, Qt::MouseButton(buttonPressed), Qt::KeyboardModifiers(keyPressed));
    qApp->processEvents();
}

void tst_QMouseEvent::checkMouseReleaseEvent_data()
{
    QTest::addColumn<int>("buttonReleased");
    QTest::addColumn<int>("keyPressed");

    QTest::newRow("leftButton-nokey") << int(Qt::LeftButton) << int(Qt::NoButton);
    QTest::newRow("leftButton-shiftkey") << int(Qt::LeftButton) << int(Qt::ShiftModifier);
    QTest::newRow("leftButton-controlkey") << int(Qt::LeftButton) << int(Qt::ControlModifier);
    QTest::newRow("leftButton-altkey") << int(Qt::LeftButton) << int(Qt::AltModifier);
    QTest::newRow("leftButton-metakey") << int(Qt::LeftButton) << int(Qt::MetaModifier);
    QTest::newRow("rightButton-nokey") << int(Qt::RightButton) << int(Qt::NoButton);
    QTest::newRow("rightButton-shiftkey") << int(Qt::RightButton) << int(Qt::ShiftModifier);
    QTest::newRow("rightButton-controlkey") << int(Qt::RightButton) << int(Qt::ControlModifier);
    QTest::newRow("rightButton-altkey") << int(Qt::RightButton) << int(Qt::AltModifier);
    QTest::newRow("rightButton-metakey") << int(Qt::RightButton) << int(Qt::MetaModifier);
    QTest::newRow("middleButton-nokey") << int(Qt::MiddleButton) << int(Qt::NoButton);
    QTest::newRow("middleButton-shiftkey") << int(Qt::MiddleButton) << int(Qt::ShiftModifier);
    QTest::newRow("middleButton-controlkey") << int(Qt::MiddleButton) << int(Qt::ControlModifier);
    QTest::newRow("middleButton-altkey") << int(Qt::MiddleButton) << int(Qt::AltModifier);
    QTest::newRow("middleButton-metakey") << int(Qt::MiddleButton) << int(Qt::MetaModifier);
}

void tst_QMouseEvent::checkMouseReleaseEvent()
{
    QFETCH(int,buttonReleased);
    QFETCH(int,keyPressed);
    int button = buttonReleased;
    int buttons = 0;
    int modifiers = keyPressed;

    QTest::mouseClick(testMouseWidget, Qt::MouseButton(buttonReleased), Qt::KeyboardModifiers(keyPressed));
    qApp->processEvents();
    QVERIFY(testMouseWidget->mouseReleaseEventRecieved);
    QCOMPARE(testMouseWidget->mouseReleaseButton, button);
    QCOMPARE(testMouseWidget->mouseReleaseButtons, buttons);
    QCOMPARE(testMouseWidget->mouseReleaseModifiers, modifiers);
}

void tst_QMouseEvent::grabbers_data()
{
    QTest::addColumn<bool>("grabExclusive");
    QTest::addColumn<bool>("grabPassive");

    QTest::newRow("no grab") << false << false;
    QTest::newRow("exclusive") << true << false;
    QTest::newRow("passive") << false << true;
}

void tst_QMouseEvent::grabbers()
{
    QFETCH(bool, grabExclusive);
    QFETCH(bool, grabPassive);

    testMouseWidget->grabExclusive = grabExclusive;
    testMouseWidget->grabPassive = grabPassive;

    QTest::mousePress(testMouseWidget, Qt::LeftButton, Qt::KeyboardModifiers(), {10, 10});

    auto devPriv = QPointingDevicePrivate::get(QPointingDevice::primaryPointingDevice());
    QCOMPARE(devPriv->activePoints.count(), 1);

    // Ensure that grabbers are persistent between events, within the stored touchpoints
    auto firstEPD = devPriv->pointById(0);
    QCOMPARE(firstEPD->eventPoint.pressTimestamp(), testMouseWidget->pressTimestamp);
    QCOMPARE(firstEPD->exclusiveGrabber, grabExclusive ? testMouseWidget : nullptr);
    QCOMPARE(firstEPD->passiveGrabbers.size(), grabPassive ? 1 : 0);
    if (grabPassive)
        QCOMPARE(firstEPD->passiveGrabbers.first(), testMouseWidget);

    // Ensure that grabbers are forgotten after release delivery
    QTest::mouseRelease(testMouseWidget, Qt::LeftButton, Qt::KeyboardModifiers(), {10, 10});
    QTRY_COMPARE(firstEPD->exclusiveGrabber, nullptr);
    QCOMPARE(firstEPD->passiveGrabbers.size(), 0);
}

void tst_QMouseEvent::velocity()
{
    testMouseWidget->grabExclusive = true;
    auto devPriv = QPointingDevicePrivate::get(const_cast<QPointingDevice *>(QPointingDevice::primaryPointingDevice()));
    devPriv->activePoints.clear();

    qCDebug(lcTests) << "sending mouse press event";
    QPoint pos(10, 10);
    QTest::mousePress(testMouseWidget, Qt::LeftButton, Qt::KeyboardModifiers(), pos);
    QCOMPARE(devPriv->activePoints.count(), 1);
    QVERIFY(devPriv->activePoints.count() <= 2);
    const auto &firstPoint = devPriv->pointById(0)->eventPoint;
    QVERIFY(firstPoint.timestamp() > 0);
    QCOMPARE(firstPoint.state(), QEventPoint::State::Pressed);

    ulong timestamp = firstPoint.timestamp();
    for (int i = 1; i < 4; ++i) {
        qCDebug(lcTests) << "sending mouse move event" << i;
        pos += {10, 10};
        QTest::mouseMove(testMouseWidget, pos, 1);
        qApp->processEvents();
        qCDebug(lcTests) << firstPoint;
        // currently we expect it to be updated in-place in devPriv->activePoints
        QVERIFY(firstPoint.timestamp() > timestamp);
        QVERIFY(testMouseWidget->timestamp > testMouseWidget->lastTimestamp);
        QCOMPARE(testMouseWidget->timestamp, firstPoint.timestamp());
        timestamp = firstPoint.timestamp();
        QVERIFY(testMouseWidget->velocity.x() > 0);
        QVERIFY(testMouseWidget->velocity.y() > 0);
    }
    QTest::mouseRelease(testMouseWidget, Qt::LeftButton, Qt::KeyboardModifiers(), pos, 1);
    qCDebug(lcTests) << firstPoint;
    QVERIFY(testMouseWidget->velocity.x() > 0);
    QVERIFY(testMouseWidget->velocity.y() > 0);
}

void tst_QMouseEvent::clone()
{
    const QPointF pos(10.0f, 10.0f);

    QMouseEvent originalMe(QEvent::MouseButtonPress, pos, pos, pos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QVERIFY(!originalMe.allPointsAccepted());
    QVERIFY(!originalMe.points().first().isAccepted());

    // create a clone of the original
    std::unique_ptr<QMouseEvent> clonedMe(originalMe.clone());
    QVERIFY(!clonedMe->allPointsAccepted());
    QVERIFY(!clonedMe->points().first().isAccepted());

    // now we alter originalMe, which should *not* change clonedMe
    originalMe.setAccepted(true);
    QVERIFY(!clonedMe->allPointsAccepted());
    QVERIFY(!clonedMe->points().first().isAccepted());
}

QTEST_MAIN(tst_QMouseEvent)
#include "tst_qmouseevent.moc"
