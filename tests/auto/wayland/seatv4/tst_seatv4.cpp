// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mockcompositor.h"

#include <QtGui/QRasterWindow>
#if QT_CONFIG(cursor)
#include <wayland-cursor.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtWaylandClient/private/qwaylandintegration_p.h>
#endif

using namespace MockCompositor;

// wl_seat version 5 was introduced in wayland 1.10, and although that's pretty old,
// there are still compositors that have yet to update their implementation to support
// the new version (most importantly our own QtWaylandCompositor).
// As long as that's the case, this test makes sure input events still works on version 4.
class SeatV4Compositor : public DefaultCompositor {
public:
    explicit SeatV4Compositor()
    {
        exec([this] {
            m_config.autoConfigure = true;
            m_config.autoFrameCallback = false; // for cursor animation test

            removeAll<Seat>();

            uint capabilities = Seat::capability_pointer | Seat::capability_keyboard;
            int version = 4;
            add<Seat>(capabilities, version);
        });
    }
};

class tst_seatv4 : public QObject, private SeatV4Compositor
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();
    void bindsToSeat();
    void keyboardKeyPress();
#if QT_CONFIG(cursor)
    void createsPointer();
    void setsCursorOnEnter();
    void usesEnterSerial();
    void focusDestruction();
    void mousePress();
    void mousePressFloat();
    void simpleAxis_data();
    void simpleAxis();
    void invalidPointerEvents();
    void scaledCursor();
    void unscaledFallbackCursor();
    void bitmapCursor();
    void hidpiBitmapCursor();
    void hidpiBitmapCursorNonInt();
    void animatedCursor();
#endif
};

void tst_seatv4::init()
{
    // Remove the extra outputs to clean up for the next test
    exec([&] { while (auto *o = output(1)) remove(o); });
}

void tst_seatv4::cleanup()
{
    QTRY_VERIFY2(isClean(), qPrintable(dirtyMessage()));
    QCOMPOSITOR_COMPARE(getAll<Output>().size(), 1); // No extra outputs left
}

void tst_seatv4::bindsToSeat()
{
    QCOMPOSITOR_COMPARE(get<Seat>()->resourceMap().size(), 1);
    QCOMPOSITOR_COMPARE(get<Seat>()->resourceMap().first()->version(), 4);
}

void tst_seatv4::keyboardKeyPress()
{
    class Window : public QRasterWindow {
    public:
        void keyPressEvent(QKeyEvent *) override { m_pressed = true; }
        bool m_pressed = false;
    };

    Window window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    uint keyCode = 80; // arbitrarily chosen
    exec([&] {
        auto *surface = xdgSurface()->m_surface;
        keyboard()->sendEnter(surface);
        keyboard()->sendKey(client(), keyCode, Keyboard::key_state_pressed);
        keyboard()->sendKey(client(), keyCode, Keyboard::key_state_released);
    });
    QTRY_VERIFY(window.m_pressed);
}

#if QT_CONFIG(cursor)

void tst_seatv4::createsPointer()
{
    QCOMPOSITOR_TRY_COMPARE(pointer()->resourceMap().size(), 1);
    QCOMPOSITOR_TRY_COMPARE(pointer()->resourceMap().first()->version(), 4);
}

void tst_seatv4::setsCursorOnEnter()
{
    QRasterWindow window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] { pointer()->sendEnter(xdgSurface()->m_surface, {24, 24}); });
    QCOMPOSITOR_TRY_VERIFY(cursorSurface());
}

void tst_seatv4::usesEnterSerial()
{
    QSignalSpy setCursorSpy(exec([&] { return pointer(); }), &Pointer::setCursor);
    QRasterWindow window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    uint enterSerial = exec([&] {
        return pointer()->sendEnter(xdgSurface()->m_surface, {32, 32});
    });
    QCOMPOSITOR_TRY_VERIFY(cursorSurface());

    QTRY_COMPARE(setCursorSpy.size(), 1);
    QCOMPARE(setCursorSpy.takeFirst().at(0).toUInt(), enterSerial);
}

void tst_seatv4::focusDestruction()
{
    QSignalSpy setCursorSpy(exec([&] { return pointer(); }), &Pointer::setCursor);
    QRasterWindow window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);
    // Setting a cursor now is not allowed since there has been no enter event
    QCOMPARE(setCursorSpy.size(), 0);

    uint enterSerial = exec([&] {
        return pointer()->sendEnter(xdgSurface()->m_surface, {32, 32});
    });
    QCOMPOSITOR_TRY_VERIFY(cursorSurface());
    QTRY_COMPARE(setCursorSpy.size(), 1);
    QCOMPARE(setCursorSpy.takeFirst().at(0).toUInt(), enterSerial);

    // Destroy the focus
    window.close();

    QRasterWindow window2;
    window2.resize(64, 64);
    window2.show();
    window2.setCursor(Qt::WaitCursor);
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    // Setting a cursor now is not allowed since there has been no enter event
    xdgPingAndWaitForPong();
    QCOMPARE(setCursorSpy.size(), 0);
}

void tst_seatv4::mousePress()
{
    class Window : public QRasterWindow {
    public:
        void mousePressEvent(QMouseEvent *) override { m_pressed = true; }
        bool m_pressed = false;
    };

    Window window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] {
        auto *surface = xdgSurface()->m_surface;
        pointer()->sendEnter(surface, {32, 32});
        pointer()->sendButton(client(), BTN_LEFT, 1);
        pointer()->sendButton(client(), BTN_LEFT, 0);
    });
    QTRY_VERIFY(window.m_pressed);
}

void tst_seatv4::mousePressFloat()
{
    class Window : public QRasterWindow {
    public:
        void mousePressEvent(QMouseEvent *e) override { m_position = e->position(); }
        QPointF m_position;
    };

    Window window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] {
        auto *surface = xdgSurface()->m_surface;
        pointer()->sendEnter(surface, {32.75, 32.25});
        pointer()->sendButton(client(), BTN_LEFT, 1);
        pointer()->sendButton(client(), BTN_LEFT, 0);
    });
    QMargins m = window.frameMargins();
    QPointF pressedPosition(32.75 -m.left(), 32.25 - m.top());
    QTRY_COMPARE(window.m_position, pressedPosition);
}

void tst_seatv4::simpleAxis_data()
{
    QTest::addColumn<uint>("axis");
    QTest::addColumn<qreal>("value");
    QTest::addColumn<QPoint>("angleDelta");

    // Directions in regular windows/linux terms (no "natural" scrolling)
    QTest::newRow("down") << uint(Pointer::axis_vertical_scroll) << 1.0  << QPoint{0, -12};
    QTest::newRow("up") << uint(Pointer::axis_vertical_scroll) << -1.0 << QPoint{0, 12};
    QTest::newRow("left") << uint(Pointer::axis_horizontal_scroll) << 1.0 << QPoint{-12, 0};
    QTest::newRow("right") << uint(Pointer::axis_horizontal_scroll) << -1.0 << QPoint{12, 0};
    QTest::newRow("up big") << uint(Pointer::axis_vertical_scroll) << -10.0 << QPoint{0, 120};
}

void tst_seatv4::simpleAxis()
{
    QFETCH(uint, axis);
    QFETCH(qreal, value);
    QFETCH(QPoint, angleDelta);

    class WheelWindow : QRasterWindow {
    public:
        explicit WheelWindow()
        {
            resize(64, 64);
            show();
        }
        void wheelEvent(QWheelEvent *event) override
        {
            QRasterWindow::wheelEvent(event);
            // Angle delta should always be provided (says docs)
            QVERIFY(!event->angleDelta().isNull());

            // There are now scroll phases on Wayland prior to v5
            QCOMPARE(event->phase(), Qt::NoScrollPhase);

            // Pixel delta should only be set if we know it's a high-res input device (which we don't)
            QCOMPARE(event->pixelDelta(), QPoint(0, 0));

            // The axis vector of the event is already in surface space, so there is now way to tell
            // whether it is inverted or not.
            QCOMPARE(event->inverted(), false);

            // We didn't press any buttons
            QCOMPARE(event->buttons(), Qt::NoButton);

            // There has been no information about what created the event.
            // Documentation says not synthesized is appropriate in such cases
            QCOMPARE(event->source(), Qt::MouseEventNotSynthesized);

            m_events.append(Event{event->pixelDelta(), event->angleDelta()});
        }
        struct Event // Because I didn't find a convenient way to copy it entirely
        {
            QPoint pixelDelta;
            QPoint angleDelta; // eights of a degree, positive is upwards, left
        };
        QList<Event> m_events;
    };

    WheelWindow window;
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] {
        Surface *surface = xdgSurface()->m_surface;
        pointer()->sendEnter(surface, {32, 32});
        wl_client *client = surface->resource()->client();
        // Length of vector in surface-local space. i.e. positive is downwards
        pointer()->sendAxis(
            client,
            Pointer::axis(axis),
            value // Length of vector in surface-local space. i.e. positive is downwards
        );
    });

    QTRY_COMPARE(window.m_events.size(), 1);
    auto event = window.m_events.takeFirst();
    QCOMPARE(event.angleDelta, angleDelta);
}

void tst_seatv4::invalidPointerEvents()
{
    QRasterWindow window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] {
        auto *p = pointer();
        auto *c = client();
        // Purposefully send events without a wl_pointer.enter
        p->sendMotion(c, {32, 32});
        p->sendButton(c, BTN_LEFT, Pointer::button_state_pressed);
        p->sendAxis(c, Pointer::axis_vertical_scroll, 1.0);
    });

    // Make sure we get here without crashing
    xdgPingAndWaitForPong();
}

static bool supportsCursorSize(uint size, wl_shm *shm)
{
    auto *theme = wl_cursor_theme_load(qgetenv("XCURSOR_THEME"), size, shm);
    if (!theme)
        return false;

    constexpr std::array<const char *, 4> names{"left_ptr", "default", "left_arrow", "top_left_arrow"};
    for (const char *name : names) {
        if (auto *cursor = wl_cursor_theme_get_cursor(theme, name)) {
            auto *image = cursor->images[0];
            return image->width == image->height && image->width == size;
        }
    }
    return false;
}

static bool supportsCursorSizes(const QList<uint> &sizes)
{
    auto *waylandIntegration = static_cast<QtWaylandClient::QWaylandIntegration *>(QGuiApplicationPrivate::platformIntegration());
    wl_shm *shm = waylandIntegration->display()->shm()->object();
    return std::all_of(sizes.begin(), sizes.end(), [&](uint size) {
        return supportsCursorSize(size, shm);
    });
}

static uint defaultCursorSize() {
    const int xCursorSize = qEnvironmentVariableIntValue("XCURSOR_SIZE");
    return xCursorSize > 0 ? uint(xCursorSize) : 24;
}

void tst_seatv4::scaledCursor()
{
    const uint defaultSize = defaultCursorSize();
    if (!supportsCursorSizes({defaultSize, defaultSize * 2}))
        QSKIP("Cursor themes with default size and 2x default size not found.");

    // Add a highdpi output
    exec([&] {
        OutputData d;
        d.scale = 2;
        d.position = {1920, 0};
        add<Output>(d);
    });

    QRasterWindow window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] { pointer()->sendEnter(xdgSurface()->m_surface, {32, 32}); });
    QCOMPOSITOR_TRY_VERIFY(cursorSurface());
    QCOMPOSITOR_TRY_VERIFY(cursorSurface()->m_committed.buffer);
    QCOMPOSITOR_TRY_COMPARE(cursorSurface()->m_committed.bufferScale, 1);
    QSize unscaledPixelSize = exec([&] {
        return cursorSurface()->m_committed.buffer->size();
    });

    exec([&] {
        auto *surface = cursorSurface();
        surface->sendEnter(getAll<Output>()[1]);
        surface->sendLeave(getAll<Output>()[0]);
    });

    QCOMPOSITOR_TRY_COMPARE(cursorSurface()->m_committed.buffer->size(), unscaledPixelSize * 2);

    // Remove the extra output to clean up for the next test
    exec([&] { remove(output(1)); });
}

void tst_seatv4::unscaledFallbackCursor()
{
    const uint defaultSize = defaultCursorSize();
    if (!supportsCursorSizes({defaultSize}))
        QSKIP("Default cursor size not supported");

    const int screens = 4; // with scales 1, 2, 4, 8

    exec([&] {
        for (int i = 1; i < screens; ++i) {
            OutputData d;
            d.scale = int(qPow(2, i));
            d.position = {1920 * i, 0};
            add<Output>(d);
        }
    });

    QRasterWindow window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);
    exec([&] { pointer()->sendEnter(xdgSurface()->m_surface, {32, 32}); });
    QCOMPOSITOR_TRY_VERIFY(cursorSurface());
    QCOMPOSITOR_TRY_VERIFY(cursorSurface()->m_committed.buffer);
    QCOMPOSITOR_TRY_COMPARE(cursorSurface()->m_committed.bufferScale, 1);
    QSize unscaledPixelSize = exec([&] {
        return cursorSurface()->m_committed.buffer->size();
    });

    QCOMPARE(unscaledPixelSize.width(), int(defaultSize));
    QCOMPARE(unscaledPixelSize.height(), int(defaultSize));

    for (int i = 1; i < screens; ++i) {
        exec([&] {
            auto *surface = cursorSurface();
            surface->sendEnter(getAll<Output>()[i]);
            surface->sendLeave(getAll<Output>()[i-1]);
        });

        xdgPingAndWaitForPong(); // Give the client a chance to mess up

        // Surface size (buffer size / scale) should stay constant
        QCOMPOSITOR_TRY_COMPARE(cursorSurface()->m_committed.buffer->size() / cursorSurface()->m_committed.bufferScale, unscaledPixelSize);
    }

    // Remove the extra outputs to clean up for the next test
    exec([&] { while (auto *o = output(1)) remove(o); });
}

void tst_seatv4::bitmapCursor()
{
    // Add a highdpi output
    exec([&] {
        OutputData d;
        d.scale = 2;
        d.position = {1920, 0};
        add<Output>(d);
    });

    QRasterWindow window;
    window.resize(64, 64);

    QPixmap pixmap(24, 24);
    pixmap.setDevicePixelRatio(1);
    QPoint hotspot(12, 12); // In device pixel coordinates
    QCursor cursor(pixmap, hotspot.x(), hotspot.y());
    window.setCursor(cursor);

    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] { pointer()->sendEnter(xdgSurface()->m_surface, {32, 32}); });
    QCOMPOSITOR_TRY_VERIFY(cursorSurface());
    QCOMPOSITOR_TRY_VERIFY(cursorSurface()->m_committed.buffer);
    QCOMPOSITOR_COMPARE(cursorSurface()->m_committed.buffer->size(), QSize(24, 24));
    QCOMPOSITOR_COMPARE(cursorSurface()->m_committed.bufferScale, 1);
    QCOMPOSITOR_COMPARE(pointer()->m_hotspot, QPoint(12, 12));

    exec([&] {
        auto *surface = cursorSurface();
        surface->sendEnter(getAll<Output>()[1]);
        surface->sendLeave(getAll<Output>()[0]);
    });

    xdgPingAndWaitForPong();

    // Everything should remain the same, the cursor still has dpr 1
    QCOMPOSITOR_COMPARE(cursorSurface()->m_committed.bufferScale, 1);
    QCOMPOSITOR_COMPARE(cursorSurface()->m_committed.buffer->size(), QSize(24, 24));
    QCOMPOSITOR_COMPARE(pointer()->m_hotspot, QPoint(12, 12));

    // Remove the extra output to clean up for the next test
    exec([&] { remove(getAll<Output>()[1]); });
}

void tst_seatv4::hidpiBitmapCursor()
{
    // Add a highdpi output
    exec([&] {
        OutputData d;
        d.scale = 2;
        d.position = {1920, 0};
        add<Output>(d);
    });

    QRasterWindow window;
    window.resize(64, 64);

    QPixmap pixmap(48, 48);
    pixmap.setDevicePixelRatio(2);
    QPoint hotspot(12, 12); // In device pixel coordinates
    QCursor cursor(pixmap, hotspot.x(), hotspot.y());
    window.setCursor(cursor);

    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] { pointer()->sendEnter(xdgSurface()->m_surface, {32, 32}); });
    QCOMPOSITOR_TRY_VERIFY(cursorSurface());
    QCOMPOSITOR_TRY_VERIFY(cursorSurface()->m_committed.buffer);
    QCOMPOSITOR_COMPARE(cursorSurface()->m_committed.buffer->size(), QSize(48, 48));
    QCOMPOSITOR_COMPARE(cursorSurface()->m_committed.bufferScale, 2);
    QCOMPOSITOR_COMPARE(pointer()->m_hotspot, QPoint(12, 12));

    exec([&] {
        auto *surface = cursorSurface();
        surface->sendEnter(getAll<Output>()[1]);
        surface->sendLeave(getAll<Output>()[0]);
    });

    xdgPingAndWaitForPong();

    QCOMPOSITOR_COMPARE(cursorSurface()->m_committed.bufferScale, 2);
    QCOMPOSITOR_COMPARE(cursorSurface()->m_committed.buffer->size(), QSize(48, 48));
    QCOMPOSITOR_COMPARE(pointer()->m_hotspot, QPoint(12, 12));

    // Remove the extra output to clean up for the next test
    exec([&] { remove(getAll<Output>()[1]); });
}

void tst_seatv4::hidpiBitmapCursorNonInt()
{
    QRasterWindow window;
    window.resize(64, 64);

    QPixmap pixmap(100, 100);
    pixmap.setDevicePixelRatio(2.5); // dpr width is now 100 / 2.5 = 40
    QPoint hotspot(20, 20); // In device pixel coordinates (middle of buffer)
    QCursor cursor(pixmap, hotspot.x(), hotspot.y());
    window.setCursor(cursor);

    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] { pointer()->sendEnter(xdgSurface()->m_surface, {32, 32}); });
    QCOMPOSITOR_TRY_VERIFY(cursorSurface());
    QCOMPOSITOR_TRY_VERIFY(cursorSurface()->m_committed.buffer);
    QCOMPOSITOR_COMPARE(cursorSurface()->m_committed.buffer->size(), QSize(100, 100));
    QCOMPOSITOR_COMPARE(cursorSurface()->m_committed.bufferScale, 2);
    // Verify that the hotspot was scaled correctly
    // Surface size is now 100 / 2 = 50, so the middle should be at 25 in surface coordinates
    QCOMPOSITOR_COMPARE(pointer()->m_hotspot, QPoint(25, 25));
}

void tst_seatv4::animatedCursor()
{
    QRasterWindow window;
    window.resize(64, 64);
    window.setCursor(Qt::WaitCursor); // TODO: verify that the theme has an animated wait cursor or skip test
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    exec([&] { pointer()->sendEnter(xdgSurface()->m_surface, {32, 32}); });
    QCOMPOSITOR_TRY_VERIFY(cursorSurface());

    // We should get the first buffer without waiting for a frame callback
    QCOMPOSITOR_TRY_VERIFY(cursorSurface()->m_committed.buffer);
    QSignalSpy bufferSpy(exec([&] { return cursorSurface(); }), &Surface::bufferCommitted);

    exec([&] {
        // Make sure no extra buffers have arrived
        QVERIFY(bufferSpy.empty());

        if (QSysInfo::productType() == "opensuse-leap" && QSysInfo::productVersion() == QLatin1String("16.0"))
            QEXPECT_FAIL("", "QTBUG-141773: opensuse-leap 16.0 fails", Continue);

        // The client should send a frame request in order to time animations correctly
        QVERIFY(!cursorSurface()->m_waitingFrameCallbacks.empty());

        // Tell the client it's time to animate
        cursorSurface()->sendFrameCallbacks();
    });

    if (QSysInfo::productType() == "opensuse-leap" && QSysInfo::productVersion() == QLatin1String("16.0"))
        QEXPECT_FAIL("", "QTBUG-141773: opensuse-leap 16.0 fails", Continue);

    // Verify that we get a new cursor buffer
    QTRY_COMPARE(bufferSpy.size(), 1);
}

#endif // QT_CONFIG(cursor)

QCOMPOSITOR_TEST_MAIN(tst_seatv4)
#include "tst_seatv4.moc"
