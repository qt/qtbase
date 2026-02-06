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
            int version = 7;
            add<Seat>(capabilities, version);
        });
    }
};

class tst_seatv7 : public QObject, private SeatCompositor
{
    Q_OBJECT
private slots:
    void cleanup() { QTRY_VERIFY2(isClean(), qPrintable(dirtyMessage())); }
    void bindsToSeat();

    // Pointer tests
    void wheelDiscreteScroll_data();
    void wheelDiscreteScroll();
};

void tst_seatv7::bindsToSeat()
{
    QCOMPOSITOR_COMPARE(get<Seat>()->resourceMap().size(), 1);
    QCOMPOSITOR_COMPARE(get<Seat>()->resourceMap().first()->version(), 7);
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

        // The axis vector of the event is already in surface space, so there is now way to tell
        // whether it is inverted or not.
        QCOMPARE(event->inverted(), false);

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
        {
        }
        Qt::ScrollPhase phase{};
        QPoint pixelDelta;
        QPoint angleDelta; // eights of a degree, positive is upwards, left
        Qt::MouseEventSource source{};
    };
    QList<Event> m_events;
};

void tst_seatv7::wheelDiscreteScroll_data()
{
    QTest::addColumn<uint>("source");
    QTest::newRow("wheel") << uint(Pointer::axis_source_wheel);
    QTest::newRow("wheel tilt") << uint(Pointer::axis_source_wheel_tilt);
}

void tst_seatv7::wheelDiscreteScroll()
{
    WheelWindow window;
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    QFETCH(uint, source);

    exec([&] {
        auto *p = pointer();
        auto *c = client();
        p->sendEnter(xdgToplevel()->surface(), {32, 32});
        p->sendFrame(c);
        p->sendAxisSource(c, Pointer::axis_source(source));
        p->sendAxisDiscrete(c, Pointer::axis_vertical_scroll, 1); // 1 click downwards
        p->sendAxis(c, Pointer::axis_vertical_scroll, 1.0);
        p->sendFrame(c);
    });

    QTRY_VERIFY(!window.m_events.empty());
    {
        auto e = window.m_events.takeFirst();
        QCOMPARE(e.phase, Qt::NoScrollPhase);
        QVERIFY(qAbs(e.angleDelta.x()) <= qAbs(e.angleDelta.y())); // Vertical scroll
        // According to the docs the angle delta is in eights of a degree and most mice have
        // 1 click = 15 degrees. The angle delta should therefore be:
        // 15 degrees / (1/8 eights per degrees) = 120 eights of degrees.
        QCOMPARE(e.angleDelta, QPoint(0, -120));
        QCOMPARE(e.pixelDelta, QPoint(0, -1));
    }
}

QCOMPOSITOR_TEST_MAIN(tst_seatv7)
#include "tst_seatv7.moc"
