// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtGui>
#include <QtWidgets>
#include <QTest>
#include <QtGui/private/qevent_p.h>
#include <QtGui/private/qeventpoint_p.h>
#include <qpa/qwindowsysteminterface.h>

#include <QtWidgets/private/qapplication_p.h>

// #include <QDebug>

class tst_QScrollerWidget : public QWidget
{
public:
    tst_QScrollerWidget()
        : QWidget()
    {
        reset();
    }

    void reset()
    {
        receivedPrepare = false;
        receivedScroll = false;
        receivedFirst = false;
        receivedLast = false;
        receivedOvershoot = false;
    }

    bool event(QEvent *e) override
    {
        switch (e->type()) {
        case QEvent::Gesture:
            e->setAccepted(false); // better reject the event or QGestureManager will make trouble
            return false;

        case QEvent::ScrollPrepare:
            {
                receivedPrepare = true;
                QScrollPrepareEvent *se = static_cast<QScrollPrepareEvent *>(e);
                se->setViewportSize(QSizeF(100,100));
                se->setContentPosRange(scrollArea);
                se->setContentPos(scrollPosition);
                se->accept();
                return true;
            }

        case QEvent::Scroll:
            {
                receivedScroll = true;
                QScrollEvent *se = static_cast<QScrollEvent *>(e);
                // qDebug() << "Scroll for"<<this<<"pos"<<se->scrollPos()<<"ov"<<se->overshoot()<<"first"<<se->isFirst()<<"last"<<se->isLast();

                if (se->scrollState() == QScrollEvent::ScrollStarted)
                    receivedFirst = true;
                if (se->scrollState() == QScrollEvent::ScrollFinished)
                    receivedLast = true;

                currentPos = se->contentPos();
                overshoot = se->overshootDistance();
                if (!qFuzzyCompare(overshoot.x() + 1.0, 1.0) ||
                    !qFuzzyCompare(overshoot.y() + 1.0, 1.0))
                    receivedOvershoot = true;
                return true;
            }

        default:
            return QObject::event(e);
        }
    }


    QRectF scrollArea;
    QPointF scrollPosition;

    bool receivedPrepare;
    bool receivedScroll;
    bool receivedFirst;
    bool receivedLast;
    bool receivedOvershoot;

    QPointF currentPos;
    QPointF overshoot;
};


class tst_QScroller : public QObject
{
    Q_OBJECT
public:
    tst_QScroller() { }
    ~tst_QScroller() { }

private:
    void kineticScroll(tst_QScrollerWidget *sw, QPointF from, QPoint touchStart, QPoint touchUpdate, QPoint touchEnd);
    void kineticScrollNoTest(tst_QScrollerWidget *sw, QPointF from, QPoint touchStart, QPoint touchUpdate, QPoint touchEnd);

private slots:
    void staticScrollers();
    void scrollerProperties();
    void scrollTo();
    void scroll();
    void overshoot();
    void multipleWindows();
    void mouseEventTimestamp();

private:
    QPointingDevice *m_touchScreen = QTest::createTouchDevice();
};

/*! \internal
    Generates touchBegin, touchUpdate and touchEnd events to trigger scrolling.
    Tests some in between states but does not wait until scrolling is finished.
*/
void tst_QScroller::kineticScroll(tst_QScrollerWidget *sw, QPointF from, QPoint touchStart, QPoint touchUpdate, QPoint touchEnd)
{
    sw->scrollPosition = from;
    sw->currentPos= from;

    QScroller *s1 = QScroller::scroller(sw);
    QCOMPARE(s1->state(), QScroller::Inactive);

    QScrollerProperties sp1 = QScroller::scroller(sw)->scrollerProperties();

    // send the touch begin event
    QEventPoint touchPoint(0);
    QMutableEventPoint::setState(touchPoint, QEventPoint::State::Pressed);
    QMutableEventPoint::setPosition(touchPoint, touchStart);
    QMutableEventPoint::setScenePosition(touchPoint, touchStart);
    QMutableEventPoint::setGlobalPosition(touchPoint, touchStart);

    QTouchEvent touchEvent1(QEvent::TouchBegin,
                            m_touchScreen,
                            Qt::NoModifier,
                            (QList<QTouchEvent::TouchPoint>() << touchPoint));

    QApplication::sendEvent(sw, &touchEvent1);

    QCOMPARE(s1->state(), QScroller::Pressed);

    // send the touch update far enough to trigger a scroll
    QTest::qWait(200); // we need to wait a little or else the speed would be infinite. now we have around 500 pixel per second.
    QMutableEventPoint::setPosition(touchPoint, touchUpdate);
    QMutableEventPoint::setScenePosition(touchPoint, touchUpdate);
    QMutableEventPoint::setGlobalPosition(touchPoint, touchUpdate);
    QMutableEventPoint::setState(touchPoint, QEventPoint::State::Updated);
    QTouchEvent touchEvent2(QEvent::TouchUpdate,
                            m_touchScreen,
                            Qt::NoModifier,
                            (QList<QEventPoint>() << touchPoint));
    QApplication::sendEvent(sw, &touchEvent2);

    QCOMPARE(s1->state(), QScroller::Dragging);
    QCOMPARE(sw->receivedPrepare, true);


    QTRY_COMPARE(sw->receivedFirst, true);
    QCOMPARE(sw->receivedScroll, true);
    QCOMPARE(sw->receivedOvershoot, false);

    // note that the scrolling goes in a different direction than the mouse move
    QPoint calculatedPos = from.toPoint() - touchUpdate - touchStart;
    QVERIFY(qAbs(sw->currentPos.x() - calculatedPos.x()) < 1.0);
    QVERIFY(qAbs(sw->currentPos.y() - calculatedPos.y()) < 1.0);

    // send the touch end
    QMutableEventPoint::setPosition(touchPoint, touchEnd);
    QMutableEventPoint::setScenePosition(touchPoint, touchEnd);
    QMutableEventPoint::setGlobalPosition(touchPoint, touchEnd);
    QMutableEventPoint::setState(touchPoint, QEventPoint::State::Released);
    QTouchEvent touchEvent5(QEvent::TouchEnd,
                            m_touchScreen,
                            Qt::NoModifier,
                            (QList<QEventPoint>() << touchPoint));
    QApplication::sendEvent(sw, &touchEvent5);
}

/*! \internal
    Generates touchBegin, touchUpdate and touchEnd events to trigger scrolling.
    This function does not have any in between tests, it does not expect the scroller to actually scroll.
*/
void tst_QScroller::kineticScrollNoTest(tst_QScrollerWidget *sw, QPointF from, QPoint touchStart, QPoint touchUpdate, QPoint touchEnd)
{
    sw->scrollPosition = from;
    sw->currentPos = from;

    QScroller *s1 = QScroller::scroller(sw);
    QCOMPARE(s1->state(), QScroller::Inactive);

    QScrollerProperties sp1 = s1->scrollerProperties();
    int fps = 60;

    // send the touch begin event
    QEventPoint touchPoint(0);
    QMutableEventPoint::setState(touchPoint, QEventPoint::State::Pressed);
    QMutableEventPoint::setPosition(touchPoint, touchStart);
    QMutableEventPoint::setScenePosition(touchPoint, touchStart);
    QMutableEventPoint::setGlobalPosition(touchPoint, touchStart);
    QTouchEvent touchEvent1(QEvent::TouchBegin,
                            m_touchScreen,
                            Qt::NoModifier,
                            (QList<QEventPoint>() << touchPoint));
    QApplication::sendEvent(sw, &touchEvent1);

    // send the touch update far enough to trigger a scroll
    QTest::qWait(200); // we need to wait a little or else the speed would be infinite. now we have around 500 pixel per second.
    QMutableEventPoint::setState(touchPoint, QEventPoint::State::Updated);
    QMutableEventPoint::setPosition(touchPoint, touchUpdate);
    QMutableEventPoint::setScenePosition(touchPoint, touchUpdate);
    QMutableEventPoint::setGlobalPosition(touchPoint, touchUpdate);
    QTouchEvent touchEvent2(QEvent::TouchUpdate,
                            m_touchScreen,
                            Qt::NoModifier,
                            (QList<QEventPoint>() << touchPoint));
    QApplication::sendEvent(sw, &touchEvent2);

    QTest::qWait(1000 / fps * 2); // wait until the first scroll move

    // send the touch end
    QMutableEventPoint::setState(touchPoint, QEventPoint::State::Released);
    QMutableEventPoint::setPosition(touchPoint, touchEnd);
    QMutableEventPoint::setScenePosition(touchPoint, touchEnd);
    QMutableEventPoint::setGlobalPosition(touchPoint, touchEnd);
    QTouchEvent touchEvent5(QEvent::TouchEnd,
                            m_touchScreen,
                            Qt::NoModifier,
                            (QList<QEventPoint>() << touchPoint));
    QApplication::sendEvent(sw, &touchEvent5);
}


void tst_QScroller::staticScrollers()
{
    // scrollers
    {
        QObject *o1 = new QObject(this);
        QObject *o2 = new QObject(this);

        // get scroller for object
        QScroller *s1 = QScroller::scroller(o1);
        QScroller *s2 = QScroller::scroller(o2);

        QVERIFY(s1);
        QVERIFY(s2);
        QVERIFY(s1 != s2);

        QVERIFY(!QScroller::scroller(static_cast<const QObject*>(0)));
        QCOMPARE(QScroller::scroller(o1), s1);

        delete o1;
        delete o2;
    }

    // the same for properties
    {
        QObject *o1 = new QObject(this);
        QObject *o2 = new QObject(this);

        // get scroller for object
        QScrollerProperties sp1 = QScroller::scroller(o1)->scrollerProperties();
        QScrollerProperties sp2 = QScroller::scroller(o2)->scrollerProperties();

        // default properties should be the same
        QCOMPARE(sp1, sp2);

        QCOMPARE(QScroller::scroller(o1)->scrollerProperties(), sp1);

        delete o1;
        delete o2;
    }
}

void tst_QScroller::scrollerProperties()
{
    QObject *o1 = new QObject(this);
    QScrollerProperties sp1 = QScroller::scroller(o1)->scrollerProperties();

    QScrollerProperties::ScrollMetric metrics[] =
    {
        QScrollerProperties::MousePressEventDelay,                    // qreal [s]
        QScrollerProperties::DragStartDistance,                       // qreal [m]
        QScrollerProperties::DragVelocitySmoothingFactor,             // qreal [0..1/s]  (complex calculation involving time) v = v_new* DASF + v_old * (1-DASF)
        QScrollerProperties::AxisLockThreshold,                       // qreal [0..1] atan(|min(dx,dy)|/|max(dx,dy)|)

        QScrollerProperties::DecelerationFactor,                      // slope of the curve

        QScrollerProperties::MinimumVelocity,                         // qreal [m/s]
        QScrollerProperties::MaximumVelocity,                         // qreal [m/s]
        QScrollerProperties::MaximumClickThroughVelocity,             // qreal [m/s]

        QScrollerProperties::AcceleratingFlickMaximumTime,            // qreal [s]
        QScrollerProperties::AcceleratingFlickSpeedupFactor,          // qreal [1..]

        QScrollerProperties::SnapPositionRatio,                       // qreal [0..1]
        QScrollerProperties::SnapTime,                                // qreal [s]

        QScrollerProperties::OvershootDragResistanceFactor,           // qreal [0..1]
        QScrollerProperties::OvershootDragDistanceFactor,             // qreal [0..1]
        QScrollerProperties::OvershootScrollDistanceFactor,           // qreal [0..1]
        QScrollerProperties::OvershootScrollTime,                     // qreal [s]
    };

    for (unsigned int i = 0; i < sizeof(metrics) / sizeof(metrics[0]); i++) {
        sp1.setScrollMetric(metrics[i], 0.9);
        QCOMPARE(sp1.scrollMetric(metrics[i]).toDouble(), 0.9);
    }
    sp1.setScrollMetric(QScrollerProperties::ScrollingCurve, QEasingCurve(QEasingCurve::OutQuart));
    QCOMPARE(sp1.scrollMetric(QScrollerProperties::ScrollingCurve).toEasingCurve().type(), QEasingCurve::OutQuart);

    sp1.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QVariant::fromValue(QScrollerProperties::OvershootAlwaysOff));
    QCOMPARE(sp1.scrollMetric(QScrollerProperties::HorizontalOvershootPolicy).value<QScrollerProperties::OvershootPolicy>(), QScrollerProperties::OvershootAlwaysOff);

    sp1.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QVariant::fromValue(QScrollerProperties::OvershootAlwaysOn));
    QCOMPARE(sp1.scrollMetric(QScrollerProperties::VerticalOvershootPolicy).value<QScrollerProperties::OvershootPolicy>(), QScrollerProperties::OvershootAlwaysOn);

    sp1.setScrollMetric(QScrollerProperties::FrameRate, QVariant::fromValue(QScrollerProperties::Fps20));
    QCOMPARE(sp1.scrollMetric(QScrollerProperties::FrameRate).value<QScrollerProperties::FrameRates>(), QScrollerProperties::Fps20);
}

void tst_QScroller::scrollTo()
{
    QScopedPointer<tst_QScrollerWidget> sw(new tst_QScrollerWidget);
    sw->show();
    QApplicationPrivate::setActiveWindow(sw.data());
    if (!QTest::qWaitForWindowExposed(sw.data()) || !QTest::qWaitForWindowActive(sw.data()))
        QSKIP("Failed to show and activate window");

    sw->scrollArea = QRectF(0, 0, 1000, 1000);
    sw->scrollPosition = QPointF(500, 500);

    QScroller *s1 = QScroller::scroller(sw.data());
    QCOMPARE(s1->state(), QScroller::Inactive);

    // a normal scroll
    s1->scrollTo(QPointF(100,100), 100);
    QTest::qWait(200);

    QTRY_COMPARE(sw->receivedPrepare, true);
    QCOMPARE(sw->receivedScroll, true);
    QCOMPARE(sw->receivedFirst, true);
    QCOMPARE(sw->receivedLast, true);
    QCOMPARE(sw->receivedOvershoot, false);
    QTRY_VERIFY(qFuzzyCompare(sw->currentPos.x(), 100));
    QVERIFY(qFuzzyCompare(sw->currentPos.y(), 100));
}

void tst_QScroller::scroll()
{
#if QT_CONFIG(gestures) && QT_CONFIG(scroller)
    // -- good case. normal scroll
    QScopedPointer<tst_QScrollerWidget> sw(new tst_QScrollerWidget());
    sw->scrollArea = QRectF(0, 0, 1000, 1000);
    QScroller::grabGesture(sw.data(), QScroller::TouchGesture);
    sw->setGeometry(100, 100, 400, 300);
    sw->show();
    QApplicationPrivate::setActiveWindow(sw.data());
    if (!QTest::qWaitForWindowExposed(sw.data()) || !QTest::qWaitForWindowActive(sw.data()))
        QSKIP("Failed to show and activate window");

    QScroller *s1 = QScroller::scroller(sw.data());
    kineticScroll(sw.data(), QPointF(500, 500), QPoint(0, 0), QPoint(100, 100), QPoint(200, 200));
    // now we should be scrolling
    QTRY_COMPARE(s1->state(), QScroller::Scrolling);

    // wait until finished, check that no further first scroll is sent
    sw->receivedFirst = false;
    sw->receivedScroll = false;
    QTRY_VERIFY(s1->state() != QScroller::Scrolling);

    QCOMPARE(sw->receivedFirst, false);
    QCOMPARE(sw->receivedScroll, true);
    QCOMPARE(sw->receivedLast, true);
    QVERIFY(sw->currentPos.x() < 400);
    QVERIFY(sw->currentPos.y() < 400);

    // -- try to scroll when nothing to scroll

    sw->reset();
    sw->scrollArea = QRectF(0, 0, 0, 1000);
    kineticScrollNoTest(sw.data(), QPointF(0, 500), QPoint(0, 0), QPoint(100, 0), QPoint(200, 0));

    QTRY_COMPARE(s1->state(), QScroller::Inactive);

    QCOMPARE(sw->currentPos.x(), 0.0);
    QCOMPARE(sw->currentPos.y(), 500.0);
#endif
}

void tst_QScroller::overshoot()
{
#if QT_CONFIG(gestures) && QT_CONFIG(scroller)
    QScopedPointer<tst_QScrollerWidget> sw(new tst_QScrollerWidget);
    sw->scrollArea = QRectF(0, 0, 1000, 1000);
    QScroller::grabGesture(sw.data(), QScroller::TouchGesture);
    sw->setGeometry(100, 100, 400, 300);
    sw->show();
    QApplicationPrivate::setActiveWindow(sw.data());
    if (!QTest::qWaitForWindowExposed(sw.data()) || !QTest::qWaitForWindowActive(sw.data()))
        QSKIP("Failed to show and activate window");

    QScroller *s1 = QScroller::scroller(sw.data());
    QScrollerProperties sp1 = s1->scrollerProperties();

    sp1.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 0.5);
    sp1.setScrollMetric(QScrollerProperties::OvershootDragDistanceFactor, 0.2);
    sp1.setScrollMetric(QScrollerProperties::OvershootScrollDistanceFactor, 0.2);

    // -- try to scroll with overshoot (when scrollable good case)

    sp1.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QVariant::fromValue(QScrollerProperties::OvershootWhenScrollable));
    s1->setScrollerProperties(sp1);
    kineticScrollNoTest(sw.data(), QPointF(500, 500), QPoint(0, 0), QPoint(400, 0), QPoint(490, 0));

    QTRY_COMPARE(s1->state(), QScroller::Inactive);

    //qDebug() << "Overshoot fuzzy: "<<sw->currentPos;
    QVERIFY(qFuzzyCompare(sw->currentPos.x(), 0));
    QVERIFY(qFuzzyCompare(sw->currentPos.y(), 500));
    QCOMPARE(sw->receivedOvershoot, true);

    // -- try to scroll with overshoot (when scrollable bad case)
    sw->reset();
    sw->scrollArea = QRectF(0, 0, 0, 1000);

    sp1.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QVariant::fromValue(QScrollerProperties::OvershootWhenScrollable));
    s1->setScrollerProperties(sp1);
    kineticScrollNoTest(sw.data(), QPointF(0, 500), QPoint(0, 0), QPoint(400, 0), QPoint(490, 0));

    QTRY_COMPARE(s1->state(), QScroller::Inactive);

    //qDebug() << "Overshoot fuzzy: "<<sw->currentPos;
    QVERIFY(qFuzzyCompare(sw->currentPos.x(), 0));
    QVERIFY(qFuzzyCompare(sw->currentPos.y(), 500));
    QCOMPARE(sw->receivedOvershoot, false);

    // -- try to scroll with overshoot (always on)
    sw->reset();
    sw->scrollArea = QRectF(0, 0, 0, 1000);

    sp1.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QVariant::fromValue(QScrollerProperties::OvershootAlwaysOn));
    s1->setScrollerProperties(sp1);
    kineticScrollNoTest(sw.data(), QPointF(0, 500), QPoint(0, 0), QPoint(400, 0), QPoint(490, 0));

    QTRY_COMPARE(s1->state(), QScroller::Inactive);

    //qDebug() << "Overshoot fuzzy: "<<sw->currentPos;

    QVERIFY(qFuzzyCompare(sw->currentPos.x(), 0));
    QVERIFY(qFuzzyCompare(sw->currentPos.y(), 500));
    QCOMPARE(sw->receivedOvershoot, true);

    // -- try to scroll with overshoot (always off)
    sw->reset();
    sw->scrollArea = QRectF(0, 0, 1000, 1000);

    sp1.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QVariant::fromValue(QScrollerProperties::OvershootAlwaysOff));
    s1->setScrollerProperties(sp1);
    kineticScrollNoTest(sw.data(), QPointF(500, 500), QPoint(0, 0), QPoint(400, 0), QPoint(490, 0));

    QTRY_COMPARE(s1->state(), QScroller::Inactive);

    QVERIFY(qFuzzyCompare(sw->currentPos.x(), 0));
    QVERIFY(qFuzzyCompare(sw->currentPos.y(), 500));
    QCOMPARE(sw->receivedOvershoot, false);

    // -- try to scroll with overshoot (always on but max overshoot = 0)
    sp1.setScrollMetric(QScrollerProperties::OvershootDragDistanceFactor, 0.0);
    sp1.setScrollMetric(QScrollerProperties::OvershootScrollDistanceFactor, 0.0);
    sw->reset();
    sw->scrollArea = QRectF(0, 0, 1000, 1000);

    sp1.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QVariant::fromValue(QScrollerProperties::OvershootAlwaysOn));
    s1->setScrollerProperties(sp1);
    kineticScrollNoTest(sw.data(), QPointF(500, 500), QPoint(0, 0), QPoint(400, 0), QPoint(490, 0));

    QTRY_COMPARE(s1->state(), QScroller::Inactive);

    QVERIFY(qFuzzyCompare(sw->currentPos.x(), 0));
    QVERIFY(qFuzzyCompare(sw->currentPos.y(), 500));
    QCOMPARE(sw->receivedOvershoot, false);
#endif
}

void tst_QScroller::multipleWindows()
{
#if QT_CONFIG(gestures) && QT_CONFIG(scroller)
    QScopedPointer<tst_QScrollerWidget> sw1(new tst_QScrollerWidget);
    sw1->scrollArea = QRectF(0, 0, 1000, 1000);
    QScroller::grabGesture(sw1.data(), QScroller::TouchGesture);
    sw1->setGeometry(100, 100, 400, 300);

    QScroller *s1 = QScroller::scroller(sw1.data());
    kineticScroll(sw1.data(), QPointF(500, 500), QPoint(0, 0), QPoint(100, 100), QPoint(200, 200));
    // now we should be scrolling
    QTRY_COMPARE(s1->state(), QScroller::Scrolling);

    // That was fun! Do it again!
    QScopedPointer<tst_QScrollerWidget> sw2(new tst_QScrollerWidget());
    sw2->scrollArea = QRectF(0, 0, 1000, 1000);
    QScroller::grabGesture(sw2.data(), QScroller::TouchGesture);
    sw2->setGeometry(100, 100, 400, 300);

    QScroller *s2 = QScroller::scroller(sw2.data());
    kineticScroll(sw2.data(), QPointF(500, 500), QPoint(0, 0), QPoint(100, 100), QPoint(200, 200));
    // now we should be scrolling
    QTRY_COMPARE(s2->state(), QScroller::Scrolling);

    // wait for both to stop
    QTRY_VERIFY(s1->state() != QScroller::Scrolling);
    QTRY_VERIFY(s2->state() != QScroller::Scrolling);

    sw1.reset(); // destroy one window
    sw2->reset(); // reset the other scroller's internal state
    // make sure we can still scroll the remaining one without crashing (QTBUG-71232)
    kineticScroll(sw2.data(), QPointF(500, 500), QPoint(0, 0), QPoint(100, 100), QPoint(200, 200));
#endif
}

/*!
    This test verifies that mouse events arrive at the target widget
    with valid timestamp, even if there is a gesture filtering (and then
    replaying a copy of) the event. QTBUG-102010

    We cannot truly simulate the double click here, as simulated events don't
    go through the exact same event machinery as real events, so double clicks
    don't get generated by Qt here. But we can verify that the timestamps of
    the eventually delivered events are maintained.
*/
void tst_QScroller::mouseEventTimestamp()
{
#if QT_CONFIG(gestures) && QT_CONFIG(scroller)
    QScopedPointer<tst_QScrollerWidget> sw(new tst_QScrollerWidget());
    sw->scrollArea = QRectF(0, 0, 1000, 1000);
    QScroller::grabGesture(sw.data(), QScroller::LeftMouseButtonGesture);
    sw->setGeometry(100, 100, 400, 300);
    sw->show();
    QApplicationPrivate::setActiveWindow(sw.data());
    if (!QTest::qWaitForWindowExposed(sw.data()) || !QTest::qWaitForWindowActive(sw.data()))
        QSKIP("Failed to show and activate window");

    QScroller *s1 = QScroller::scroller(sw.data());

    struct EventFilter : QObject
    {
        QList<int> timestamps;
    protected:
        bool eventFilter(QObject *o, QEvent *e) override
        {
            if (e->isInputEvent())
                timestamps << static_cast<QInputEvent *>(e)->timestamp();
            return QObject::eventFilter(o, e);
        }

    } eventFilter;
    sw->installEventFilter(&eventFilter);

    const int interval = QGuiApplication::styleHints()->mouseDoubleClickInterval() / 10;
    const QPoint point = sw->geometry().center();
    // Simulate double by pressing twice within the double click interval.
    // Presses are filtered and then delayed by the scroller/gesture machinery,
    // so we first record all events, and then make sure that the relative timestamps
    // are maintained also for the replayed or synthesized events.
    QTest::mousePress(sw->windowHandle(), Qt::LeftButton, {}, point);
    QCOMPARE(s1->state(), QScroller::Pressed);
    QTest::mouseRelease(sw->windowHandle(), Qt::LeftButton, {}, point, interval);
    QCOMPARE(s1->state(), QScroller::Inactive);
    QTest::mousePress(sw->windowHandle(), Qt::LeftButton, {}, point, interval);
    QCOMPARE(s1->state(), QScroller::Pressed);
    // also filtered and delayed by the scroller
    QTest::mouseRelease(sw->windowHandle(), Qt::LeftButton, {}, point, interval);
    QCOMPARE(s1->state(), QScroller::Inactive);
    int lastTimestamp = -1;
    for (int timestamp : std::as_const(eventFilter.timestamps)) {
        QCOMPARE_GE(timestamp, lastTimestamp);
        lastTimestamp = timestamp + interval;
    }
#endif
}

QTEST_MAIN(tst_QScroller)

#include "tst_qscroller.moc"
