// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mockcompositor.h"
#include <QtGui/QRasterWindow>
#include <QtGui/QEventPoint>

using namespace MockCompositor;

class SeatCompositor : public DefaultCompositor {
public:
    explicit SeatCompositor()
    {
        exec([this] {
            m_config.autoConfigure = true;

            removeAll<Seat>();

            uint capabilities = MockCompositor::Seat::capability_pointer | MockCompositor::Seat::capability_touch;
            int version = 9;
            add<Seat>(capabilities, version);
        });
    }
};

class tst_seat : public QObject, private SeatCompositor
{
    Q_OBJECT
private slots:
    void cleanup() { QTRY_VERIFY2(isClean(), qPrintable(dirtyMessage())); }
    void bindsToSeat();

    // Pointer tests
    void createsPointer();
    void setsCursorOnEnter();
    void usesEnterSerial();
    void simpleAxis_data();
    void simpleAxis();
    void fingerScroll();
    void fingerScrollSlow();
    void continuousScroll();
    void highResolutionScroll();

    // Touch tests
    void createsTouch();
    void singleTap();
    void singleTapFloat();
    void multiTouch();
    void multiTouchUpAndMotionFrame();
    void tapAndMoveInSameFrame();
    void cancelTouch();
};

void tst_seat::bindsToSeat()
{
    QCOMPOSITOR_COMPARE(get<Seat>()->resourceMap().size(), 1);
    QCOMPOSITOR_COMPARE(get<Seat>()->resourceMap().first()->version(), 9);
}

void tst_seat::createsPointer()
{
    QCOMPOSITOR_TRY_COMPARE(pointer()->resourceMap().size(), 1);
    QCOMPOSITOR_TRY_COMPARE(pointer()->resourceMap().first()->version(), 9);
}

void tst_seat::setsCursorOnEnter()
{
    QRasterWindow window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] {
        auto *surface = xdgSurface()->m_surface;
        pointer()->sendEnter(surface, {0, 0});
        pointer()->sendFrame(surface->resource()->client());
    });

    QCOMPOSITOR_TRY_VERIFY(pointer()->cursorSurface());
}

void tst_seat::usesEnterSerial()
{
    QSignalSpy setCursorSpy(exec([&] { return pointer(); }), &Pointer::setCursor);
    QRasterWindow window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    uint enterSerial = exec([&] {
        return pointer()->sendEnter(xdgSurface()->m_surface, {0, 0});
    });
    QCOMPOSITOR_TRY_VERIFY(pointer()->cursorSurface());

    QTRY_COMPARE(setCursorSpy.size(), 1);
    QCOMPARE(setCursorSpy.takeFirst().at(0).toUInt(), enterSerial);
}

class WheelWindow : QRasterWindow {
public:
    WheelWindow()
    {
        resize(64, 64);
        show();
    }
    void wheelEvent(QWheelEvent *event) override
    {
        QRasterWindow::wheelEvent(event);
//        qDebug() << event << "angleDelta" << event->angleDelta() << "pixelDelta" << event->pixelDelta();

        if (event->phase() != Qt::ScrollUpdate && event->phase() != Qt::NoScrollPhase) {
            // Shouldn't have deltas in the these phases
            QCOMPARE(event->angleDelta(), QPoint(0, 0));
            QCOMPARE(event->pixelDelta(), QPoint(0, 0));
        }

        // We didn't press any buttons
        QCOMPARE(event->buttons(), Qt::NoButton);

        m_events.append(Event{event});
    }
    struct Event // Because I didn't find a convenient way to copy it entirely
    {
        explicit Event() = default;
        explicit Event(const QWheelEvent *event)
            : phase(event->phase())
            , pixelDelta(event->pixelDelta())
            , angleDelta(event->angleDelta())
            , source(event->source())
            , inverted(event->inverted())
        {
        }
        Qt::ScrollPhase phase{};
        QPoint pixelDelta;
        QPoint angleDelta; // eights of a degree, positive is upwards, left
        Qt::MouseEventSource source{};
        bool inverted = false;
    };
    QList<Event> m_events;
};

void tst_seat::simpleAxis_data()
{
    QTest::addColumn<uint>("axis");
    QTest::addColumn<qreal>("value");
    QTest::addColumn<QPoint>("angleDelta");
    QTest::addColumn<bool>("inverted");

    // Directions in regular windows/linux terms (no "natural" scrolling)
    QTest::newRow("down") << uint(Pointer::axis_vertical_scroll) << 1.0 << QPoint{0, -12} << false;
    QTest::newRow("up") << uint(Pointer::axis_vertical_scroll) << -1.0 << QPoint{0, 12} << false;
    QTest::newRow("left") << uint(Pointer::axis_horizontal_scroll) << 1.0 << QPoint{-12, 0} << false;
    QTest::newRow("right") << uint(Pointer::axis_horizontal_scroll) << -1.0 << QPoint{12, 0} << false;
    QTest::newRow("up big") << uint(Pointer::axis_vertical_scroll) << -10.0 << QPoint{0, 120} << false;

    // (natural) scrolling
    QTest::newRow("down inverted") << uint(Pointer::axis_vertical_scroll) << 1.0 << QPoint{0, -12} << true;
    QTest::newRow("up inverted") << uint(Pointer::axis_vertical_scroll) << -1.0 << QPoint{0, 12} << true;
    QTest::newRow("left inverted") << uint(Pointer::axis_horizontal_scroll) << 1.0 << QPoint{-12, 0} << true;
    QTest::newRow("right inverted") << uint(Pointer::axis_horizontal_scroll) << -1.0 << QPoint{12, 0} << true;
    QTest::newRow("up big inverted") << uint(Pointer::axis_vertical_scroll) << -10.0 << QPoint{0, 120} << true;
}

void tst_seat::simpleAxis()
{
    QFETCH(uint, axis);
    QFETCH(qreal, value);
    QFETCH(QPoint, angleDelta);
    QFETCH(bool, inverted);

    WheelWindow window;
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] {
        auto *p = pointer();
        p->sendEnter(xdgToplevel()->surface(), {32, 32});
        p->sendFrame(client());
        p->sendAxis(
            client(),
            Pointer::axis(axis),
            value // Length of vector in surface-local space. i.e. positive is downwards
        );
        auto direction = inverted ? Pointer::axis_relative_direction_inverted : Pointer::axis_relative_direction_identical;
        p->sendAxisRelativeDirection(client(), Pointer::axis(axis), direction);
        p->sendFrame(client());
    });

    QTRY_VERIFY(!window.m_events.empty());
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.phase, Qt::NoScrollPhase);
        QCOMPARE(e.pixelDelta, axis == Pointer::axis_vertical_scroll ? QPoint(0, -value) : QPoint(-value, 0));
        // There has been no information about what created the event.
        // Documentation says not synthesized is appropriate in such cases
        QCOMPARE(e.source, Qt::MouseEventNotSynthesized);
        QCOMPARE(e.angleDelta, angleDelta);

        QCOMPARE(e.inverted, inverted);
    }

    // Sending axis_stop is not mandatory when axis source != finger
}

void tst_seat::fingerScroll()
{
    WheelWindow window;
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] {
        auto *p = pointer();
        auto *c = client();
        p->sendEnter(xdgToplevel()->surface(), {32, 32});
        p->sendFrame(c);
        p->sendAxisSource(c, Pointer::axis_source_finger);
        p->sendAxis(c, Pointer::axis_vertical_scroll, 10);
        p->sendFrame(c);
    });

    QTRY_VERIFY(!window.m_events.empty());
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.phase, Qt::ScrollBegin);
        QCOMPARE(e.angleDelta, QPoint());
        QCOMPARE(e.pixelDelta, QPoint());
    }

    QTRY_VERIFY(!window.m_events.empty());
    // For some reason we send two ScrollBegins, one for each direction, not sure if this is really
    // necessary, (could be removed from QtBase, hence the conditional below.
    if (window.m_events.first().phase == Qt::ScrollBegin) {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.angleDelta, QPoint());
        QCOMPARE(e.pixelDelta, QPoint());
    }

    QTRY_VERIFY(!window.m_events.empty());
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.phase, Qt::ScrollUpdate);
        QVERIFY(qAbs(e.angleDelta.x()) <= qAbs(e.angleDelta.y())); // Vertical scroll
//        QCOMPARE(e.angleDelta, angleDelta); // TODO: what should this be?
        QCOMPARE(e.pixelDelta, QPoint(0, -10));
        QCOMPARE(e.source, Qt::MouseEventSynthesizedBySystem); // A finger is not a wheel
    }

    QTRY_VERIFY(window.m_events.empty());

    // Scroll horizontally as well
    exec([&] {
        pointer()->sendAxisSource(client(), Pointer::axis_source_finger);
        pointer()->sendAxis(client(), Pointer::axis_horizontal_scroll, 10);
        pointer()->sendFrame(client());
    });
    QTRY_VERIFY(!window.m_events.empty());
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.phase, Qt::ScrollUpdate);
        QVERIFY(qAbs(e.angleDelta.x()) > qAbs(e.angleDelta.y())); // Horizontal scroll
        QCOMPARE(e.pixelDelta, QPoint(-10, 0));
        QCOMPARE(e.source, Qt::MouseEventSynthesizedBySystem); // A finger is not a wheel
    }

    // Scroll diagonally
    exec([&] {
        pointer()->sendAxisSource(client(), Pointer::axis_source_finger);
        pointer()->sendAxis(client(), Pointer::axis_horizontal_scroll, 10);
        pointer()->sendAxis(client(), Pointer::axis_vertical_scroll, 10);
        pointer()->sendFrame(client());
    });
    QTRY_VERIFY(!window.m_events.empty());
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.phase, Qt::ScrollUpdate);
        QCOMPARE(e.pixelDelta, QPoint(-10, -10));
        QCOMPARE(e.source, Qt::MouseEventSynthesizedBySystem); // A finger is not a wheel
    }

    // For diagonal events, Qt sends an additional compatibility ScrollUpdate event
    if (window.m_events.first().phase == Qt::ScrollUpdate) {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.angleDelta, QPoint());
        QCOMPARE(e.pixelDelta, QPoint());
    }

    QVERIFY(window.m_events.empty());

    // Sending axis_stop is mandatory when axis source == finger
    exec([&] {
        pointer()->sendAxisStop(client(), Pointer::axis_vertical_scroll);
        pointer()->sendFrame(client());
    });

    QTRY_VERIFY(!window.m_events.empty());
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.phase, Qt::ScrollEnd);
    }
}


void tst_seat::fingerScrollSlow()
{
    WheelWindow window;
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] {
        auto *p = pointer();
        auto *c = client();
        p->sendEnter(xdgToplevel()->surface(), {32, 32});
        p->sendFrame(c);
        // Send 10 really small updates
        for (int i = 0; i < 10; ++i) {
            p->sendAxisSource(c, Pointer::axis_source_finger);
            p->sendAxis(c, Pointer::axis_vertical_scroll, 0.1);
            p->sendFrame(c);
        }
        p->sendAxisStop(c, Pointer::axis_vertical_scroll);
        p->sendFrame(c);
    });

    QTRY_VERIFY(!window.m_events.empty());
    QPoint accumulated;
    while (window.m_events.first().phase != Qt::ScrollEnd) {
        auto e = window.m_events.takeFirst();
        accumulated += e.pixelDelta;
        QTRY_VERIFY(!window.m_events.empty());
    }
    QCOMPARE(accumulated.y(), -1);
}

void tst_seat::highResolutionScroll()
{
    WheelWindow window;
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] {
        auto *p = pointer();
        auto *c = client();
        p->sendEnter(xdgToplevel()->surface(), {32, 32});
        p->sendFrame(c);
        p->sendAxisSource(c, Pointer::axis_source_wheel);
        p->sendAxisValue120(c, Pointer::axis_vertical_scroll, 30); // quarter of a click
        p->sendAxis(c, Pointer::axis_vertical_scroll, 3.75);
        p->sendFrame(c);
    });

    QTRY_VERIFY(!window.m_events.empty());
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.phase, Qt::NoScrollPhase);
        QVERIFY(qAbs(e.angleDelta.x()) <= qAbs(e.angleDelta.y())); // Vertical scroll
        QCOMPARE(e.angleDelta, QPoint(0, -30));
        QCOMPARE(e.pixelDelta, QPoint(0, -4));
    }

    exec([&] {
        auto *p = pointer();
        auto *c = client();
        p->sendAxisSource(c, Pointer::axis_source_wheel);
        p->sendAxisValue120(c, Pointer::axis_vertical_scroll, 90); // complete the click
        p->sendAxis(c, Pointer::axis_vertical_scroll, 11.25);
        p->sendFrame(c);
    });

    QTRY_VERIFY(!window.m_events.empty());
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.phase, Qt::NoScrollPhase);
        QVERIFY(qAbs(e.angleDelta.x()) <= qAbs(e.angleDelta.y())); // Vertical scroll
        QCOMPARE(e.angleDelta, QPoint(0, -90));
        // Click scrolls are not continuous and should not have a pixel delta
        QCOMPARE(e.pixelDelta, QPoint(0, -11));
    }
}

void tst_seat::continuousScroll()
{
    WheelWindow window;
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] {
        auto *p = pointer();
        auto *c = client();
        p->sendEnter(xdgToplevel()->surface(), {32, 32});
        p->sendFrame(c);
        p->sendAxisSource(c, Pointer::axis_source_continuous);
        p->sendAxis(c, Pointer::axis_vertical_scroll, 10);
        p->sendAxis(c, Pointer::axis_horizontal_scroll, -5);
        p->sendFrame(c);
    });

    QTRY_VERIFY(!window.m_events.empty());
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.phase, Qt::NoScrollPhase);
        QCOMPARE(e.pixelDelta, QPoint(5, -10));
        QCOMPARE(e.source, Qt::MouseEventSynthesizedBySystem); // touchpads are not wheels
        QCOMPARE(e.inverted, false);

    }
    // Sending axis_stop is not mandatory when axis source != finger
}

void tst_seat::createsTouch()
{
    QCOMPOSITOR_TRY_COMPARE(touch()->resourceMap().size(), 1);
    QCOMPOSITOR_TRY_COMPARE(touch()->resourceMap().first()->version(), 9);
}

class TouchWindow : public QRasterWindow {
public:
    TouchWindow()
    {
        resize(64, 64);
        show();
    }
    void touchEvent(QTouchEvent *event) override
    {
        QRasterWindow::touchEvent(event);
        m_events.append(Event{event});
    }
    struct Event // Because I didn't find a convenient way to copy it entirely
    {
        explicit Event() = default;
        explicit Event(const QTouchEvent *event)
            : type(event->type())
            , touchPointStates(event->touchPointStates())
            , touchPoints(event->points())
        {
        }
        QEvent::Type type{};
        QEventPoint::States touchPointStates{};
        QList<QTouchEvent::TouchPoint> touchPoints;
    };
    QList<Event> m_events;
};

void tst_seat::singleTap()
{
    TouchWindow window;
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] {
        auto *t = touch();
        auto *c = client();
        t->sendDown(xdgToplevel()->surface(), {32, 32}, 1);
        t->sendFrame(c);
        t->sendUp(c, 1);
        t->sendFrame(c);
    });

    QTRY_VERIFY(!window.m_events.empty());
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.type, QEvent::TouchBegin);
        QCOMPARE(e.touchPointStates, QEventPoint::State::Pressed);
        QCOMPARE(e.touchPoints.size(), 1);
        QCOMPARE(e.touchPoints.first().position(), QPointF(32-window.frameMargins().left(), 32-window.frameMargins().top()));
    }
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.type, QEvent::TouchEnd);
        QCOMPARE(e.touchPointStates, QEventPoint::State::Released);
        QCOMPARE(e.touchPoints.size(), 1);
        QCOMPARE(e.touchPoints.first().position(), QPointF(32-window.frameMargins().left(), 32-window.frameMargins().top()));
    }
}

void tst_seat::singleTapFloat()
{
    TouchWindow window;
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] {
        auto *t = touch();
        auto *c = client();
        t->sendDown(xdgToplevel()->surface(), {32.75, 32.25}, 1);
        t->sendFrame(c);
        t->sendUp(c, 1);
        t->sendFrame(c);
    });

    QTRY_VERIFY(!window.m_events.empty());
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.type, QEvent::TouchBegin);
        QCOMPARE(e.touchPointStates, QEventPoint::State::Pressed);
        QCOMPARE(e.touchPoints.size(), 1);
        QCOMPARE(e.touchPoints.first().position(), QPointF(32.75-window.frameMargins().left(), 32.25-window.frameMargins().top()));
    }
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.type, QEvent::TouchEnd);
        QCOMPARE(e.touchPointStates, QEventPoint::State::Released);
        QCOMPARE(e.touchPoints.size(), 1);
        QCOMPARE(e.touchPoints.first().position(), QPointF(32.75-window.frameMargins().left(), 32.25-window.frameMargins().top()));
    }
}

void tst_seat::multiTouch()
{
    TouchWindow window;
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] {
        auto *t = touch();
        auto *c = client();

        t->sendDown(xdgToplevel()->surface(), {32, 32}, 0);
        t->sendDown(xdgToplevel()->surface(), {48, 48}, 1);
        t->sendFrame(c);

        // Compositor event order should not change the order of the QTouchEvent::points()
        // See QTBUG-77014
        t->sendMotion(c, {49, 48}, 1);
        t->sendMotion(c, {33, 32}, 0);
        t->sendFrame(c);

        t->sendUp(c, 0);
        t->sendFrame(c);

        t->sendUp(c, 1);
        t->sendFrame(c);
    });

    QTRY_VERIFY(!window.m_events.empty());
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.type, QEvent::TouchBegin);
        QCOMPARE(e.touchPointStates, QEventPoint::State::Pressed);
        QCOMPARE(e.touchPoints.size(), 2);

        QCOMPARE(e.touchPoints[0].state(), QEventPoint::State::Pressed);
        QCOMPARE(e.touchPoints[0].position(), QPointF(32-window.frameMargins().left(), 32-window.frameMargins().top()));

        QCOMPARE(e.touchPoints[1].state(), QEventPoint::State::Pressed);
        QCOMPARE(e.touchPoints[1].position(), QPointF(48-window.frameMargins().left(), 48-window.frameMargins().top()));
    }
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.type, QEvent::TouchUpdate);
        QCOMPARE(e.touchPoints.size(), 2);

        QCOMPARE(e.touchPoints[0].state(), QEventPoint::State::Updated);
        QCOMPARE(e.touchPoints[0].position(), QPointF(33-window.frameMargins().left(), 32-window.frameMargins().top()));

        QCOMPARE(e.touchPoints[1].state(), QEventPoint::State::Updated);
        QCOMPARE(e.touchPoints[1].position(), QPointF(49-window.frameMargins().left(), 48-window.frameMargins().top()));
    }
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.type, QEvent::TouchUpdate);
        QCOMPARE(e.touchPointStates, QEventPoint::State::Released | QEventPoint::State::Stationary);
        QCOMPARE(e.touchPoints.size(), 2);

        QCOMPARE(e.touchPoints[0].state(), QEventPoint::State::Released);
        QCOMPARE(e.touchPoints[0].position(), QPointF(33-window.frameMargins().left(), 32-window.frameMargins().top()));

        QCOMPARE(e.touchPoints[1].state(), QEventPoint::State::Stationary);
        QCOMPARE(e.touchPoints[1].position(), QPointF(49-window.frameMargins().left(), 48-window.frameMargins().top()));
    }
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.type, QEvent::TouchEnd);
        QCOMPARE(e.touchPointStates, QEventPoint::State::Released);
        QCOMPARE(e.touchPoints.size(), 1);
        QCOMPARE(e.touchPoints[0].state(), QEventPoint::State::Released);
        QCOMPARE(e.touchPoints[0].position(), QPointF(49-window.frameMargins().left(), 48-window.frameMargins().top()));
    }
}

void tst_seat::multiTouchUpAndMotionFrame()
{
    TouchWindow window;
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] {
        auto *t = touch();
        auto *c = client();

        t->sendDown(xdgToplevel()->surface(), {32, 32}, 0);
        t->sendDown(xdgToplevel()->surface(), {48, 48}, 1);
        t->sendFrame(c);

        // Sending an up event after a frame event, before any motion or down events used to
        // unnecessarily trigger a workaround for a bug in an old version of Weston. The workaround
        // would prematurely insert a fake frame event splitting the touch event up into two events.
        // However, this should only be needed on the up event for the very last touch point. So in
        // this test we verify that it doesn't unncecessarily break up the events.
        t->sendUp(c, 0);
        t->sendMotion(c, {49, 48}, 1);
        t->sendFrame(c);

        t->sendUp(c, 1);
        t->sendFrame(c);
    });

    QTRY_VERIFY(!window.m_events.empty());
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.type, QEvent::TouchBegin);
        QCOMPARE(e.touchPoints[0].state(), QEventPoint::State::Pressed);
        QCOMPARE(e.touchPoints[1].state(), QEventPoint::State::Pressed);
    }
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.type, QEvent::TouchUpdate);
        QCOMPARE(e.touchPoints.size(), 2);
        QCOMPARE(e.touchPoints[0].state(), QEventPoint::State::Released);
        QCOMPARE(e.touchPoints[1].state(), QEventPoint::State::Updated);
    }
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.type, QEvent::TouchEnd);
        QCOMPARE(e.touchPoints.size(), 1);
        QCOMPARE(e.touchPoints[0].state(), QEventPoint::State::Released);
    }
    QVERIFY(window.m_events.empty());
}

void tst_seat::tapAndMoveInSameFrame()
{
    TouchWindow window;
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] {
        auto *t = touch();
        auto *c = client();

        t->sendDown(xdgToplevel()->surface(), {32, 32}, 0);
        t->sendMotion(c, {33, 33}, 0);
        t->sendFrame(c);

        // Don't leave touch in a weird state
        t->sendUp(c, 0);
        t->sendFrame(c);
    });

    QTRY_VERIFY(!window.m_events.empty());
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.type, QEvent::TouchBegin);
        QCOMPARE(e.touchPoints.size(), 1);
        QCOMPARE(e.touchPoints[0].state(), QEventPoint::State::Pressed);
        // Position isn't that important, we just want to make sure we actually get the pressed event
    }

    // Make sure we eventually release
    QTRY_VERIFY(!window.m_events.empty());
    QTRY_COMPARE(window.m_events.last().touchPoints.first().state(), QEventPoint::State::Released);
}

void tst_seat::cancelTouch()
{
    TouchWindow window;
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] {
        auto *t = touch();
        auto *c = client();
        t->sendDown(xdgToplevel()->surface(), {32, 32}, 1);
        t->sendFrame(c);
        t->sendCancel(c);
        t->sendFrame(c);
    });

    QTRY_VERIFY(!window.m_events.empty());
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.type, QEvent::TouchBegin);
        QCOMPARE(e.touchPointStates, QEventPoint::State::Pressed);
        QCOMPARE(e.touchPoints.size(), 1);
        QCOMPARE(e.touchPoints.first().position(), QPointF(32-window.frameMargins().left(), 32-window.frameMargins().top()));
    }
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.type, QEvent::TouchCancel);
        QCOMPARE(e.touchPoints.size(), 0);
    }
}

QCOMPOSITOR_TEST_MAIN(tst_seat)
#include "tst_seat.moc"
