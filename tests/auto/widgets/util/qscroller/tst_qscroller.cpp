/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the $MODULE$ of the Qt Toolkit.
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

#include <QtGui>
#include <QtWidgets>
#include <QtTest>
#include <qpa/qwindowsysteminterface.h>

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

    bool event(QEvent *e)
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
                if (!qFuzzyCompare( overshoot.x() + 1.0, 1.0 ) ||
                    !qFuzzyCompare( overshoot.y() + 1.0, 1.0 ))
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
    void kineticScroll( tst_QScrollerWidget *sw, QPointF from, QPoint touchStart, QPoint touchUpdate, QPoint touchEnd);
    void kineticScrollNoTest( tst_QScrollerWidget *sw, QPointF from, QPoint touchStart, QPoint touchUpdate, QPoint touchEnd);

private slots:
    void staticScrollers();
    void scrollerProperties();
    void scrollTo();
    void scroll();
    void overshoot();
};

/*! \internal
    Generates touchBegin, touchUpdate and touchEnd events to trigger scrolling.
    Tests some in between states but does not wait until scrolling is finished.
*/
void tst_QScroller::kineticScroll( tst_QScrollerWidget *sw, QPointF from, QPoint touchStart, QPoint touchUpdate, QPoint touchEnd)
{
    sw->scrollPosition = from;
    sw->currentPos= from;

    QScroller *s1 = QScroller::scroller(sw);
    QCOMPARE( s1->state(), QScroller::Inactive );

    QScrollerProperties sp1 = QScroller::scroller(sw)->scrollerProperties();

    QTouchEvent::TouchPoint rawTouchPoint;
    rawTouchPoint.setId(0);

    // send the touch begin event
    QTouchEvent::TouchPoint touchPoint(0);
    touchPoint.setState(Qt::TouchPointPressed);
    touchPoint.setPos(touchStart);
    touchPoint.setScenePos(touchStart);
    touchPoint.setScreenPos(touchStart);
    QTouchDevice *device = new QTouchDevice;
    device->setType(QTouchDevice::TouchScreen);
    QWindowSystemInterface::registerTouchDevice(device);
    QTouchEvent touchEvent1(QEvent::TouchBegin,
                            device,
                            Qt::NoModifier,
                            Qt::TouchPointPressed,
                            (QList<QTouchEvent::TouchPoint>() << touchPoint));
    QApplication::sendEvent(sw, &touchEvent1);

    QCOMPARE( s1->state(), QScroller::Pressed );

    // send the touch update far enough to trigger a scroll
    QTest::qWait(200); // we need to wait a little or else the speed would be infinite. now we have around 500 pixel per second.
    touchPoint.setPos(touchUpdate);
    touchPoint.setScenePos(touchUpdate);
    touchPoint.setScreenPos(touchUpdate);
    QTouchEvent touchEvent2(QEvent::TouchUpdate,
                            device,
                            Qt::NoModifier,
                            Qt::TouchPointMoved,
                            (QList<QTouchEvent::TouchPoint>() << touchPoint));
    QApplication::sendEvent(sw, &touchEvent2);

    QCOMPARE( s1->state(), QScroller::Dragging );
    QCOMPARE( sw->receivedPrepare, true );


    QTRY_COMPARE( sw->receivedFirst, true );
    QCOMPARE( sw->receivedScroll, true );
    QCOMPARE( sw->receivedOvershoot, false );

    // note that the scrolling goes in a different direction than the mouse move
    QPoint calculatedPos = from.toPoint() - touchUpdate - touchStart;
    QVERIFY(qAbs(sw->currentPos.x() - calculatedPos.x()) < 1.0);
    QVERIFY(qAbs(sw->currentPos.y() - calculatedPos.y()) < 1.0);

    // send the touch end
    touchPoint.setPos(touchEnd);
    touchPoint.setScenePos(touchEnd);
    touchPoint.setScreenPos(touchEnd);
    QTouchEvent touchEvent5(QEvent::TouchEnd,
                            device,
                            Qt::NoModifier,
                            Qt::TouchPointReleased,
                            (QList<QTouchEvent::TouchPoint>() << touchPoint));
    QApplication::sendEvent(sw, &touchEvent5);
}

/*! \internal
    Generates touchBegin, touchUpdate and touchEnd events to trigger scrolling.
    This function does not have any in between tests, it does not expect the scroller to actually scroll.
*/
void tst_QScroller::kineticScrollNoTest( tst_QScrollerWidget *sw, QPointF from, QPoint touchStart, QPoint touchUpdate, QPoint touchEnd)
{
    sw->scrollPosition = from;
    sw->currentPos = from;

    QScroller *s1 = QScroller::scroller(sw);
    QCOMPARE( s1->state(), QScroller::Inactive );

    QScrollerProperties sp1 = s1->scrollerProperties();
    int fps = 60;

    QTouchEvent::TouchPoint rawTouchPoint;
    rawTouchPoint.setId(0);

    // send the touch begin event
    QTouchEvent::TouchPoint touchPoint(0);
    touchPoint.setState(Qt::TouchPointPressed);
    touchPoint.setPos(touchStart);
    touchPoint.setScenePos(touchStart);
    touchPoint.setScreenPos(touchStart);
    QTouchDevice *device = new QTouchDevice;
    device->setType(QTouchDevice::TouchScreen);
    QWindowSystemInterface::registerTouchDevice(device);
    QTouchEvent touchEvent1(QEvent::TouchBegin,
                            device,
                            Qt::NoModifier,
                            Qt::TouchPointPressed,
                            (QList<QTouchEvent::TouchPoint>() << touchPoint));
    QApplication::sendEvent(sw, &touchEvent1);

    // send the touch update far enough to trigger a scroll
    QTest::qWait(200); // we need to wait a little or else the speed would be infinite. now we have around 500 pixel per second.
    touchPoint.setPos(touchUpdate);
    touchPoint.setScenePos(touchUpdate);
    touchPoint.setScreenPos(touchUpdate);
    QTouchEvent touchEvent2(QEvent::TouchUpdate,
                            device,
                            Qt::NoModifier,
                            Qt::TouchPointMoved,
                            (QList<QTouchEvent::TouchPoint>() << touchPoint));
    QApplication::sendEvent(sw, &touchEvent2);

    QTest::qWait(1000 / fps * 2); // wait until the first scroll move

    // send the touch end
    touchPoint.setPos(touchEnd);
    touchPoint.setScenePos(touchEnd);
    touchPoint.setScreenPos(touchEnd);
    QTouchEvent touchEvent5(QEvent::TouchEnd,
                            device,
                            Qt::NoModifier,
                            Qt::TouchPointReleased,
                            (QList<QTouchEvent::TouchPoint>() << touchPoint));
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
        QVERIFY(sp1 == sp2);

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
#ifdef Q_OS_MAC
    QSKIP("Flakey test - https://bugreports.qt-project.org/browse/QTBUG-29950");
#endif
    {
        tst_QScrollerWidget *sw = new tst_QScrollerWidget();
        sw->scrollArea = QRectF( 0, 0, 1000, 1000 );
        sw->scrollPosition = QPointF( 500, 500 );

        QScroller *s1 = QScroller::scroller(sw);
        QCOMPARE( s1->state(), QScroller::Inactive );

        // a normal scroll
        s1->scrollTo(QPointF(100,100), 100);
        QTest::qWait(200);

        QCOMPARE( sw->receivedPrepare, true );
        QCOMPARE( sw->receivedScroll, true );
        QCOMPARE( sw->receivedFirst, true );
        QCOMPARE( sw->receivedLast, true );
        QCOMPARE( sw->receivedOvershoot, false );
        QVERIFY(qFuzzyCompare( sw->currentPos.x(), 100 ));
        QVERIFY(qFuzzyCompare( sw->currentPos.y(), 100 ));

        delete sw;
    }
}

void tst_QScroller::scroll()
{
#ifdef Q_OS_MAC
    QSKIP("Flakey test - https://bugreports.qt-project.org/browse/QTBUG-30133");
#endif
#ifndef QT_NO_GESTURES
    // -- good case. normal scroll
    tst_QScrollerWidget *sw = new tst_QScrollerWidget();
    sw->scrollArea = QRectF(0, 0, 1000, 1000);
    QScroller::grabGesture(sw, QScroller::TouchGesture);
    sw->setGeometry(100, 100, 400, 300);

    QScroller *s1 = QScroller::scroller(sw);
    kineticScroll(sw, QPointF(500, 500), QPoint(0, 0), QPoint(100, 100), QPoint(200, 200));
    // now we should be scrolling
    QTRY_COMPARE( s1->state(), QScroller::Scrolling );

    // wait until finished, check that no further first scroll is send
    sw->receivedFirst = false;
    sw->receivedScroll = false;
    while (s1->state() == QScroller::Scrolling)
        QTest::qWait(100);

    QCOMPARE( sw->receivedFirst, false );
    QCOMPARE( sw->receivedScroll, true );
    QCOMPARE( sw->receivedLast, true );
    QVERIFY(sw->currentPos.x() < 400);
    QVERIFY(sw->currentPos.y() < 400);

    // -- try to scroll when nothing to scroll

    sw->reset();
    sw->scrollArea = QRectF(0, 0, 0, 1000);
    kineticScrollNoTest(sw, QPointF(0, 500), QPoint(0, 0), QPoint(100, 0), QPoint(200, 0));

    while (s1->state() != QScroller::Inactive)
        QTest::qWait(20);

    QCOMPARE(sw->currentPos.x(), 0.0);
    QCOMPARE(sw->currentPos.y(), 500.0);

    delete sw;
#endif
}

void tst_QScroller::overshoot()
{
#ifdef Q_OS_MAC
    QSKIP("Flakey test - https://bugreports.qt-project.org/browse/QTBUG-29950");
#endif
#ifndef QT_NO_GESTURES
    tst_QScrollerWidget *sw = new tst_QScrollerWidget();
    sw->scrollArea = QRectF(0, 0, 1000, 1000);
    QScroller::grabGesture(sw, QScroller::TouchGesture);
    sw->setGeometry(100, 100, 400, 300);

    QScroller *s1 = QScroller::scroller(sw);
    QScrollerProperties sp1 = s1->scrollerProperties();

    sp1.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 0.5);
    sp1.setScrollMetric(QScrollerProperties::OvershootDragDistanceFactor, 0.2);
    sp1.setScrollMetric(QScrollerProperties::OvershootScrollDistanceFactor, 0.2);

    // -- try to scroll with overshoot (when scrollable good case)

    sp1.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QVariant::fromValue(QScrollerProperties::OvershootWhenScrollable));
    s1->setScrollerProperties(sp1);
    kineticScrollNoTest(sw, QPointF(500, 500), QPoint(0, 0), QPoint(400, 0), QPoint(490, 0));

    while (s1->state() != QScroller::Inactive)
        QTest::qWait(20);

    //qDebug() << "Overshoot fuzzy: "<<sw->currentPos;
    QVERIFY(qFuzzyCompare( sw->currentPos.x(), 0 ));
    QVERIFY(qFuzzyCompare( sw->currentPos.y(), 500 ));
    QCOMPARE( sw->receivedOvershoot, true );

    // -- try to scroll with overshoot (when scrollable bad case)
    sw->reset();
    sw->scrollArea = QRectF(0, 0, 0, 1000);

    sp1.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QVariant::fromValue(QScrollerProperties::OvershootWhenScrollable));
    s1->setScrollerProperties(sp1);
    kineticScrollNoTest(sw, QPointF(0, 500), QPoint(0, 0), QPoint(400, 0), QPoint(490, 0));

    while (s1->state() != QScroller::Inactive)
        QTest::qWait(20);

    //qDebug() << "Overshoot fuzzy: "<<sw->currentPos;
    QVERIFY(qFuzzyCompare( sw->currentPos.x(), 0 ));
    QVERIFY(qFuzzyCompare( sw->currentPos.y(), 500 ));
    QCOMPARE( sw->receivedOvershoot, false );

    // -- try to scroll with overshoot (always on)
    sw->reset();
    sw->scrollArea = QRectF(0, 0, 0, 1000);

    sp1.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QVariant::fromValue(QScrollerProperties::OvershootAlwaysOn));
    s1->setScrollerProperties(sp1);
    kineticScrollNoTest(sw, QPointF(0, 500), QPoint(0, 0), QPoint(400, 0), QPoint(490, 0));

    while (s1->state() != QScroller::Inactive)
        QTest::qWait(20);

    //qDebug() << "Overshoot fuzzy: "<<sw->currentPos;

    QVERIFY(qFuzzyCompare( sw->currentPos.x(), 0 ));
    QVERIFY(qFuzzyCompare( sw->currentPos.y(), 500 ));
    QCOMPARE( sw->receivedOvershoot, true );

    // -- try to scroll with overshoot (always off)
    sw->reset();
    sw->scrollArea = QRectF(0, 0, 1000, 1000);

    sp1.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QVariant::fromValue(QScrollerProperties::OvershootAlwaysOff));
    s1->setScrollerProperties(sp1);
    kineticScrollNoTest(sw, QPointF(500, 500), QPoint(0, 0), QPoint(400, 0), QPoint(490, 0));

    while (s1->state() != QScroller::Inactive)
        QTest::qWait(20);

    QVERIFY(qFuzzyCompare( sw->currentPos.x(), 0 ));
    QVERIFY(qFuzzyCompare( sw->currentPos.y(), 500 ));
    QCOMPARE( sw->receivedOvershoot, false );

    // -- try to scroll with overshoot (always on but max overshoot = 0)
    sp1.setScrollMetric(QScrollerProperties::OvershootDragDistanceFactor, 0.0);
    sp1.setScrollMetric(QScrollerProperties::OvershootScrollDistanceFactor, 0.0);
    sw->reset();
    sw->scrollArea = QRectF(0, 0, 1000, 1000);

    sp1.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QVariant::fromValue(QScrollerProperties::OvershootAlwaysOn));
    s1->setScrollerProperties(sp1);
    kineticScrollNoTest(sw, QPointF(500, 500), QPoint(0, 0), QPoint(400, 0), QPoint(490, 0));

    while (s1->state() != QScroller::Inactive)
        QTest::qWait(20);

    QVERIFY(qFuzzyCompare( sw->currentPos.x(), 0 ));
    QVERIFY(qFuzzyCompare( sw->currentPos.y(), 500 ));
    QCOMPARE( sw->receivedOvershoot, false );

    delete sw;
#endif
}

QTEST_MAIN(tst_QScroller)

#include "tst_qscroller.moc"
