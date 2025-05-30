// Copyright (C) 2018 The Qt Company Ltd.
// Copyright (C) 2023 David Edmundson <davidedmundson@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mockcompositor.h"
#include <QtGui/QRasterWindow>
#include <QtGui/qpa/qplatformnativeinterface.h>
#include <QtWaylandClient/private/wayland-wayland-client-protocol.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

using namespace MockCompositor;

class tst_xdgshell : public QObject, private DefaultCompositor
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanup() { QTRY_VERIFY2(isClean(), qPrintable(dirtyMessage())); }
    void showMinimized();
    void basicConfigure();
    void configureSize();
    void configureStates();
    void configureBounds();
    void popup();
    void tooltipOnPopup();
    void tooltipAndSiblingPopup();
    void windowTypeChanges();
    void switchPopups();
    void hidePopupParent();
    void popupsWithoutParent();
    void pongs();
    void minMaxSize_data();
    void minMaxSize();
    void windowGeometry();
    void foreignSurface();
    void nativeResources();
    void suspended();
    void initiallySuspended();
    void modality();
    void modalityWithoutTransientParent();
};

void tst_xdgshell::initTestCase()
{
    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
}

void tst_xdgshell::showMinimized()
{
    // On xdg-shell there's really no way for the compositor to tell the window if it's minimized
    // There are wl_surface.enter events and so on, but there's really no way to differentiate
    // between a window preview and an unminimized window.
    QWindow window;
    window.showMinimized();
    QTRY_COMPARE(window.windowStates(), Qt::WindowNoState); // rejected by handleWindowStateChanged

    // Make sure the window on the compositor side is/was created here, and not after the test
    // finishes, as that may mess up for later tests.
    QCOMPOSITOR_TRY_VERIFY(xdgSurface());
    QVERIFY(!window.isExposed());
}

void tst_xdgshell::basicConfigure()
{
    QRasterWindow window;
    window.resize(64, 48);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());

    QSignalSpy configureSpy(exec([&] { return xdgSurface(); }), &XdgSurface::configureCommitted);

    QTRY_VERIFY(window.isVisible());
    // The window should not be exposed before the first xdg_surface configure event
    QTRY_VERIFY(!window.isExposed());

    exec([&] {
        xdgToplevel()->sendConfigure({0, 0}, {}); // Let the window decide the size
    });

    // Nothing should happen before the *xdg_surface* configure
    QTRY_VERIFY(!window.isExposed()); //Window should not be exposed before the first configure event
    QVERIFY(configureSpy.isEmpty());

    const uint serial = exec([&] { return nextSerial(); });

    exec([&] {
        xdgSurface()->sendConfigure(serial);
    });

    // Finally, we're exposed
    QTRY_VERIFY(window.isExposed());

    // The client is now going to ack the configure
    QTRY_COMPARE(configureSpy.size(), 1);
    QCOMPARE(configureSpy.takeFirst().at(0).toUInt(), serial);

    // And attach a buffer
    exec([&] {
        Buffer *buffer = xdgToplevel()->surface()->m_committed.buffer;
        QVERIFY(buffer);
        QCOMPARE(buffer->size(), window.frameGeometry().size());
    });
}

void tst_xdgshell::configureSize()
{
    QRasterWindow window;
    window.resize(64, 48);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());

    QSignalSpy configureSpy(exec([&] { return xdgSurface(); }), &XdgSurface::configureCommitted);

    const QSize configureSize(60, 40);

    int pendingSerial;
    exec([&] {
        xdgToplevel()->sendCompleteConfigure(configureSize);
        pendingSerial = xdgSurface()->m_pendingConfigureSerials.last();
    });

    QTRY_COMPARE(configureSpy.size(), 1);
    QCOMPARE(configureSpy.last()[0].toInt(), pendingSerial);

    exec([&] {
        Buffer *buffer = xdgToplevel()->surface()->m_committed.buffer;
        QVERIFY(buffer);
        QCOMPARE(buffer->size(), configureSize);
    });

    // clients should always respond with a new ack configure + commit
    // even if nothing changed
    exec([&] {
        xdgToplevel()->sendCompleteConfigure(configureSize);
        pendingSerial = xdgSurface()->m_pendingConfigureSerials.last();
    });

    QTRY_COMPARE(configureSpy.size(), 2);
    QCOMPARE(configureSpy.last()[0].toInt(), pendingSerial);

    exec([&] {
        Buffer *buffer = xdgToplevel()->surface()->m_committed.buffer;
        QVERIFY(buffer);
        QCOMPARE(buffer->size(), configureSize);
    });
}

void tst_xdgshell::configureStates()
{
    QVERIFY(qputenv("QT_WAYLAND_FRAME_CALLBACK_TIMEOUT", "0"));
    QRasterWindow window;
    window.resize(64, 48);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());

    const QSize windowedSize(320, 240);
    const uint windowedSerial = exec([&] {
        return xdgToplevel()->sendCompleteConfigure(windowedSize, { XdgToplevel::state_activated });
    });
    QCOMPOSITOR_TRY_COMPARE(xdgSurface()->m_committedConfigureSerial, windowedSerial);
    QCOMPARE(window.visibility(), QWindow::Windowed);
    QCOMPARE(window.windowStates(), Qt::WindowNoState);
    QCOMPARE(window.frameGeometry().size(), windowedSize);
    // Toplevel windows don't know their position on xdg-shell
//    QCOMPARE(window.frameGeometry().topLeft(), QPoint()); // TODO: this doesn't currently work when window decorations are enabled

    // window.windowstate() is driven by keyboard focus, however for decorations we want to follow
    // XDGShell this is internal to QtWayland so it is queried directly
    auto waylandWindow = static_cast<QtWaylandClient::QWaylandWindow *>(window.handle());
    Q_ASSERT(waylandWindow);
    QTRY_VERIFY(waylandWindow->windowStates().testFlag(
            Qt::WindowActive)); // Just make sure it eventually get's set correctly

    const QSize screenSize(640, 480);
    const uint maximizedSerial = exec([&] {
        return xdgToplevel()->sendCompleteConfigure(screenSize, { XdgToplevel::state_activated, XdgToplevel::state_maximized });
    });
    QCOMPOSITOR_TRY_COMPARE(xdgSurface()->m_committedConfigureSerial, maximizedSerial);
    QCOMPARE(window.visibility(), QWindow::Maximized);
    QCOMPARE(window.windowStates(), Qt::WindowMaximized);
    QCOMPARE(window.frameGeometry().size(), screenSize);
//    QCOMPARE(window.frameGeometry().topLeft(), QPoint()); // TODO: this doesn't currently work when window decorations are enabled

    const uint fullscreenSerial = exec([&] {
        return xdgToplevel()->sendCompleteConfigure(screenSize, { XdgToplevel::state_activated, XdgToplevel::state_fullscreen });
    });
    QCOMPOSITOR_TRY_COMPARE(xdgSurface()->m_committedConfigureSerial, fullscreenSerial);
    QCOMPARE(window.visibility(), QWindow::FullScreen);
    QCOMPARE(window.windowStates(), Qt::WindowFullScreen);
    QCOMPARE(window.frameGeometry().size(), screenSize);
//    QCOMPARE(window.frameGeometry().topLeft(), QPoint()); // TODO: this doesn't currently work when window decorations are enabled

    // The window should remember its original size
    const uint restoreSerial = exec([&] {
        return xdgToplevel()->sendCompleteConfigure({0, 0}, { XdgToplevel::state_activated });
    });
    QCOMPOSITOR_TRY_COMPARE(xdgSurface()->m_committedConfigureSerial, restoreSerial);
    QCOMPARE(window.visibility(), QWindow::Windowed);
    QCOMPARE(window.windowStates(), Qt::WindowNoState);
    QCOMPARE(window.frameGeometry().size(), windowedSize);
//    QCOMPARE(window.frameGeometry().topLeft(), QPoint()); // TODO: this doesn't currently work when window decorations are enabled
    QVERIFY(qunsetenv("QT_WAYLAND_FRAME_CALLBACK_TIMEOUT"));
}

void tst_xdgshell::configureBounds()
{
    QRasterWindow window;
    window.resize(1280, 1024);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());

    // Take xdg_toplevel.configure_bounds into account only if the configure event has 0x0 size.
    const uint serial1 = exec([&] {
        xdgToplevel()->sendConfigureBounds(QSize(800, 600));
        return xdgToplevel()->sendCompleteConfigure(QSize(0, 0), { XdgToplevel::state_activated });
    });
    QCOMPOSITOR_TRY_COMPARE(xdgSurface()->m_committedConfigureSerial, serial1);
    QCOMPARE(window.frameGeometry().size(), QSize(800, 600));

    // Window size in xdg_toplevel configure events takes precedence over the configure bounds.
    const uint serial2 = exec([&] {
        xdgToplevel()->sendConfigureBounds(QSize(800, 600));
        return xdgToplevel()->sendCompleteConfigure(QSize(1600, 900), { XdgToplevel::state_activated });
    });
    QCOMPOSITOR_TRY_COMPARE(xdgSurface()->m_committedConfigureSerial, serial2);
    QCOMPARE(window.frameGeometry().size(), QSize(1600, 900));
}

void tst_xdgshell::popup()
{
    class Window : public QRasterWindow {
    public:
        void mousePressEvent(QMouseEvent *event) override
        {
            QRasterWindow::mousePressEvent(event);
            m_popup.reset(new QRasterWindow);
            m_popup->setTransientParent(this);
            m_popup->setFlags(Qt::Popup);
            m_popup->resize(100, 100);
            m_popup->show();
        }
        QScopedPointer<QRasterWindow> m_popup;
    };
    Window window;
    window.resize(200, 200);
    window.show();

    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());
    QSignalSpy toplevelConfigureSpy(exec([&] { return xdgSurface(); }), &XdgSurface::configureCommitted);
    exec([&] { xdgToplevel()->sendCompleteConfigure(); });
    QTRY_COMPARE(toplevelConfigureSpy.size(), 1);

    uint clickSerial = exec([&] {
        auto *surface = xdgToplevel()->surface();
        auto *p = pointer();
        auto *c = client();
        p->sendEnter(surface, {100, 100});
        p->sendFrame(c);
        uint serial = p->sendButton(client(), BTN_LEFT, Pointer::button_state_pressed);
        p->sendButton(c, BTN_LEFT, Pointer::button_state_released);
        p->sendFrame(c);
        return serial;
    });

    QTRY_VERIFY(window.m_popup);
    QCOMPOSITOR_TRY_VERIFY(xdgPopup());
    QSignalSpy popupConfigureSpy(exec([&] { return xdgPopup()->m_xdgSurface; }), &XdgSurface::configureCommitted);
    QCOMPOSITOR_TRY_VERIFY(xdgPopup()->m_grabbed);
    QCOMPOSITOR_TRY_COMPARE(xdgPopup()->m_grabSerial, clickSerial);

    QRasterWindow *popup = window.m_popup.get();
    QVERIFY(!popup->isExposed()); // wait for configure

    QRect rect1 = QRect(100, 100, 100, 100);
    exec([&] { xdgPopup()->sendConfigure(rect1); });

    // Nothing should happen before the *xdg_surface* configure
    QTRY_VERIFY(!popup->isExposed()); // Popup shouldn't be exposed before the first configure event
    QVERIFY(popupConfigureSpy.isEmpty());

    const uint configureSerial = exec([&] {
        return xdgPopup()->m_xdgSurface->sendConfigure();
    });

    // Finally, we're exposed
    QTRY_VERIFY(popup->isExposed());

    // The client is now going to ack the configure
    QTRY_COMPARE(popupConfigureSpy.size(), 1);
    QCOMPARE(popupConfigureSpy.takeFirst().at(0).toUInt(), configureSerial);
    QCOMPARE(popup->geometry(), rect1);

    QRect rect2 = QRect(50, 50, 150, 150);
    exec([&] { xdgPopup()->sendConfigure(rect2); });

    const uint configureSerial2 = exec([&] {
        return xdgPopup()->m_xdgSurface->sendConfigure();
    });

    QTRY_COMPARE(popupConfigureSpy.size(), 1);
    QCOMPARE(popupConfigureSpy.takeFirst().at(0).toUInt(), configureSerial2);
    QCOMPARE(popup->geometry(), rect2);

    // And attach a buffer
    exec([&] {
        Buffer *buffer = xdgPopup()->surface()->m_committed.buffer;
        QVERIFY(buffer);
        QCOMPARE(buffer->size(), popup->frameGeometry().size());
    });
}

void tst_xdgshell::tooltipOnPopup()
{
    class Popup : public QRasterWindow {
    public:
        explicit Popup(QWindow *parent) {
            setTransientParent(parent);
            setFlags(Qt::Popup);
            resize(100, 100);
            show();
        }
        void mousePressEvent(QMouseEvent *event) override {
            QRasterWindow::mousePressEvent(event);
            m_tooltip = new QRasterWindow;
            m_tooltip->setTransientParent(this);
            m_tooltip->setFlags(Qt::ToolTip);
            m_tooltip->resize(100, 100);
            m_tooltip->show();
        }
        QWindow *m_tooltip = nullptr;
    };

    class Window : public QRasterWindow {
    public:
        void mousePressEvent(QMouseEvent *event) override {
            QRasterWindow::mousePressEvent(event);
            m_popup = new Popup(this);
        }
        Popup *m_popup = nullptr;
    };

    Window window;
    window.resize(200, 200);
    window.show();

    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());
    exec([&] { xdgToplevel()->sendCompleteConfigure(); });
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel()->m_xdgSurface->m_committedConfigureSerial);

    exec([&] {
        auto *surface = xdgToplevel()->surface();
        auto *p = pointer();
        auto *c = client();
        p->sendEnter(surface, {100, 100});
        p->sendFrame(c);
        p->sendButton(client(), BTN_LEFT, Pointer::button_state_pressed);
        p->sendButton(client(), BTN_LEFT, Pointer::button_state_released);
        p->sendFrame(c);
        p->sendLeave(surface);
        p->sendFrame(c);
    });

    QCOMPOSITOR_TRY_VERIFY(xdgPopup());
    exec([&] { xdgPopup()->sendCompleteConfigure(QRect(100, 100, 100, 100)); });
    QCOMPOSITOR_TRY_VERIFY(xdgPopup()->m_xdgSurface->m_committedConfigureSerial);
    QCOMPOSITOR_TRY_VERIFY(xdgPopup()->m_grabbed);

    exec([&] {
        auto *surface = xdgPopup()->surface();
        auto *p = pointer();
        auto *c = client();
        p->sendEnter(surface, {100, 100});
        p->sendFrame(c);
        p->sendButton(client(), BTN_LEFT, Pointer::button_state_pressed);
        p->sendButton(client(), BTN_LEFT, Pointer::button_state_released);
        p->sendFrame(c);
    });

    QCOMPOSITOR_TRY_VERIFY(xdgPopup(1));
    exec([&] { xdgPopup(1)->sendCompleteConfigure(QRect(100, 100, 100, 100)); });
    QCOMPOSITOR_TRY_VERIFY(xdgPopup(1)->m_xdgSurface->m_committedConfigureSerial);
    QCOMPOSITOR_TRY_VERIFY(!xdgPopup(1)->m_grabbed);

    // Close the middle popup (according protocol this should also destroy any nested popups)
    window.m_popup->close();

    // Close order is verified in XdgSurface::xdg_surface_destroy

    QCOMPOSITOR_TRY_COMPARE(xdgPopup(1), nullptr);
    QCOMPOSITOR_TRY_COMPARE(xdgPopup(0), nullptr);
}

void tst_xdgshell::tooltipAndSiblingPopup()
{
    class ToolTip : public QRasterWindow {
    public:
        explicit ToolTip(QWindow *parent) {
            setTransientParent(parent);
            setFlags(Qt::ToolTip);
            resize(100, 100);
            show();
        }
        void mousePressEvent(QMouseEvent *event) override {
            QRasterWindow::mousePressEvent(event);
            m_popup = new QRasterWindow;
            m_popup->setTransientParent(transientParent());
            m_popup->setFlags(Qt::Popup);
            m_popup->resize(100, 100);
            m_popup->show();
        }

        QRasterWindow *m_popup = nullptr;
    };

    class Window : public QRasterWindow {
    public:
        void mousePressEvent(QMouseEvent *event) override {
            QRasterWindow::mousePressEvent(event);
            m_tooltip = new ToolTip(this);
        }
        ToolTip *m_tooltip = nullptr;
    };

    Window window;
    window.resize(200, 200);
    window.show();

    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());
    exec([&] { xdgToplevel()->sendCompleteConfigure(); });
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel()->m_xdgSurface->m_committedConfigureSerial);

    exec([&] {
        auto *surface = xdgToplevel()->surface();
        auto *p = pointer();
        auto *c = client();
        p->sendEnter(surface, {100, 100});
        p->sendFrame(c);
        p->sendButton(client(), BTN_LEFT, Pointer::button_state_pressed);
        p->sendButton(client(), BTN_LEFT, Pointer::button_state_released);
        p->sendFrame(c);
        p->sendLeave(surface);
        p->sendFrame(c);
    });

    QCOMPOSITOR_TRY_VERIFY(xdgPopup());
    exec([&] { xdgPopup()->sendCompleteConfigure(QRect(100, 100, 100, 100)); });
    QCOMPOSITOR_TRY_VERIFY(xdgPopup()->m_xdgSurface->m_committedConfigureSerial);
    QCOMPOSITOR_TRY_VERIFY(!xdgPopup()->m_grabbed);

    exec([&] {
        auto *surface = xdgPopup()->surface();
        auto *p = pointer();
        auto *c = client();
        p->sendEnter(surface, {100, 100});
        p->sendFrame(c);
        p->sendButton(client(), BTN_LEFT, Pointer::button_state_pressed);
        p->sendButton(client(), BTN_LEFT, Pointer::button_state_released);
        p->sendFrame(c);
    });

    QCOMPOSITOR_TRY_VERIFY(xdgPopup(1));
    exec([&] { xdgPopup(1)->sendCompleteConfigure(QRect(100, 100, 100, 100)); });
    QCOMPOSITOR_TRY_VERIFY(xdgPopup(1)->m_xdgSurface->m_committedConfigureSerial);
    QCOMPOSITOR_TRY_VERIFY(xdgPopup(1)->m_grabbed);

    // Close the middle tooltip (it should not close the sibling popup)
    window.m_tooltip->close();

    QCOMPOSITOR_TRY_COMPARE(xdgPopup(1), nullptr);
    // Verify the remaining xdg surface is a grab popup..
    QCOMPOSITOR_TRY_VERIFY(xdgPopup(0));
    QCOMPOSITOR_TRY_VERIFY(xdgPopup(0)->m_grabbed);

    window.m_tooltip->m_popup->close();
    QCOMPOSITOR_TRY_COMPARE(xdgPopup(1), nullptr);
    QCOMPOSITOR_TRY_COMPARE(xdgPopup(0), nullptr);
}

void tst_xdgshell::windowTypeChanges()
{
    QRasterWindow parentWindow;
    parentWindow.resize(200, 200);
    parentWindow.show();

    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());
    exec([&] { xdgToplevel()->sendCompleteConfigure(); });
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel()->m_xdgSurface->m_committedConfigureSerial);

    // show a toplevel
    QRasterWindow window;
    window.resize(100, 100);
    window.setTransientParent(&parentWindow);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel(1));
    exec([&] { xdgToplevel(1)->sendCompleteConfigure(); });
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel(1)->m_xdgSurface->m_committedConfigureSerial);

    // now change it to a popup
    window.setFlag(Qt::ToolTip, true);
    QCOMPOSITOR_TRY_VERIFY(!xdgToplevel(1));
    QCOMPOSITOR_TRY_VERIFY(xdgPopup());
    exec([&] { xdgPopup()->sendCompleteConfigure(QRect(100, 100, 100, 100)); });
    QCOMPOSITOR_TRY_VERIFY(xdgPopup()->m_xdgSurface->m_committedConfigureSerial);

    window.hide();
    QCOMPOSITOR_TRY_VERIFY(!xdgPopup());

    // change to a toplevel again this time whilst hidden
    window.setFlag(Qt::ToolTip, false);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel(1));
    exec([&] { xdgToplevel(1)->sendCompleteConfigure(); });
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel(1)->m_xdgSurface->m_committedConfigureSerial);
}

// QTBUG-65680
void tst_xdgshell::switchPopups()
{
    class Popup : public QRasterWindow {
    public:
        explicit Popup(QWindow *parent) {
            setTransientParent(parent);
            setFlags(Qt::Popup);
            resize(10, 10);
            show();
        }
    };

    class Window : public QRasterWindow {
    public:
        void mousePressEvent(QMouseEvent *event) override {
            QRasterWindow::mousePressEvent(event);
            if (!m_popups.empty())
                m_popups.last()->setVisible(false);
        }
        // The bug this checks for, is the case where there is a flushWindowSystemEvents() call
        // somewhere within setVisible(false) before the grab has been released.
        // This leads to the the release event below—including its show() call—to be handled
        // At a time where there is still an active grab, making it illegal for the new popup to
        // grab.
        void mouseReleaseEvent(QMouseEvent *event) override {
            QRasterWindow::mouseReleaseEvent(event);
            m_popups << new Popup(this);
        }
        ~Window() override { qDeleteAll(m_popups); }
        QList<Popup *> m_popups;
    };

    Window window;
    window.resize(200, 200);
    window.show();

    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());
    exec([&] { xdgToplevel()->sendCompleteConfigure(); });
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel()->m_xdgSurface->m_committedConfigureSerial);

    exec([&] {
        auto *surface = xdgToplevel()->surface();
        auto *p = pointer();
        auto *c = client();
        p->sendEnter(surface, {100, 100});
        p->sendFrame(c);
        p->sendButton(c, BTN_LEFT, Pointer::button_state_pressed);
        p->sendButton(c, BTN_LEFT, Pointer::button_state_released);
        p->sendFrame(c);
        p->sendLeave(surface);
        p->sendFrame(c);
    });

    QCOMPOSITOR_TRY_VERIFY(xdgPopup());
    exec([&] { xdgPopup()->sendCompleteConfigure(QRect(100, 100, 100, 100)); });
    QCOMPOSITOR_TRY_VERIFY(xdgPopup()->m_xdgSurface->m_committedConfigureSerial);
    QCOMPOSITOR_TRY_VERIFY(xdgPopup()->m_grabbed);

    QSignalSpy firstDestroyed(exec([&] { return xdgPopup(); }), &XdgPopup::destroyRequested);

    exec([&] {
        auto *surface = xdgToplevel()->surface();
        auto *p = pointer();
        auto *c = client();
        p->sendEnter(surface, {100, 100});
        p->sendFrame(c);
        p->sendButton(c, BTN_LEFT, Pointer::button_state_pressed);
        p->sendButton(c, BTN_LEFT, Pointer::button_state_released);
        p->sendFrame(c);
    });

    // The client will now hide one popup and then show another
    firstDestroyed.wait();

    QCOMPOSITOR_TRY_VERIFY(xdgPopup());
    QCOMPOSITOR_TRY_VERIFY(!xdgPopup(1));

    // Verify we still grabbed in case the client has checks to avoid illegal grabs
    QCOMPOSITOR_TRY_VERIFY(xdgPopup()->m_grabbed);

    // Sometimes the popup will select another parent if one is illegal at the time it tries to
    // grab. So we verify we got the intended parent on the compositor side.
    QCOMPOSITOR_TRY_VERIFY(xdgPopup()->m_parentXdgSurface == xdgToplevel()->m_xdgSurface);

    // For good measure just check that configuring works as usual
    exec([&] { xdgPopup()->sendCompleteConfigure(QRect(100, 100, 100, 100)); });
    QCOMPOSITOR_TRY_VERIFY(xdgPopup()->m_xdgSurface->m_committedConfigureSerial);
}

void tst_xdgshell::hidePopupParent()
{
    class Window : public QRasterWindow {
    public:
        void mousePressEvent(QMouseEvent *event) override
        {
            QRasterWindow::mousePressEvent(event);
            m_popup.reset(new QRasterWindow);
            m_popup->setTransientParent(this);
            m_popup->setFlags(Qt::Popup);
            m_popup->resize(100, 100);
            m_popup->show();
        }
        QScopedPointer<QRasterWindow> m_popup;
    };
    Window window;
    window.resize(200, 200);
    window.show();

    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());
    exec([&] { xdgToplevel()->sendCompleteConfigure(); });
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel()->m_xdgSurface->m_committedConfigureSerial);

    exec([&] {
        auto *surface = xdgToplevel()->surface();
        auto *p = pointer();
        auto *c = client();
        p->sendEnter(surface, {100, 100});
        p->sendFrame(c);
        p->sendButton(c, BTN_LEFT, Pointer::button_state_pressed);
        p->sendButton(c, BTN_LEFT, Pointer::button_state_released);
        p->sendFrame(c);
    });
    QCOMPOSITOR_TRY_VERIFY(xdgPopup());
    exec([&] {
        xdgPopup()->sendConfigure(QRect(100, 100, 100, 100));
        xdgPopup()->m_xdgSurface->sendConfigure();
    });
    QCOMPOSITOR_TRY_VERIFY(xdgPopup()->m_xdgSurface->m_committedConfigureSerial);

    window.hide();
    QCOMPOSITOR_TRY_VERIFY(!xdgToplevel());
}

void tst_xdgshell::popupsWithoutParent()
{
    QRasterWindow popup;
    QSignalSpy popupDoneSpy(&popup, &QWindow::visibilityChanged);
    popup.setFlags(Qt::Popup);
    popup.resize(100, 100);
    popup.show();
    QVERIFY(popup.isVisible());

    // popup cannot be created within the spec, so it gets auto-dismissed
    QVERIFY(popupDoneSpy.wait());
    QVERIFY(!popup.isVisible());

    QCOMPOSITOR_VERIFY(!xdgToplevel());

    // now make a normal window with an input event
    QRasterWindow window;
    window.setTitle("main window");
    window.resize(200, 200);
    window.show();

    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());
    exec([&] { xdgToplevel()->sendCompleteConfigure(); });
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel()->m_xdgSurface->m_committedConfigureSerial);
    exec([&] {
        keyboard()->sendEnter(xdgToplevel()->surface());
        keyboard()->sendKey(client(), 72, Keyboard::key_state_pressed); // related with native scan code
        keyboard()->sendKey(client(), 72, Keyboard::key_state_released); // related with native scan code
    });
    QTRY_COMPARE(qGuiApp->focusWindow(), &window);

    // now re-show our popup, it should be able to guess a transient this time
    // and correctly show as a popup
    popup.show();
    QCOMPOSITOR_TRY_VERIFY(xdgPopup());
    exec([&] { xdgPopup()->sendCompleteConfigure(QRect(100, 100, 100, 100)); });
    QCOMPOSITOR_TRY_VERIFY(xdgPopup()->m_xdgSurface->m_committedConfigureSerial);
}

void tst_xdgshell::pongs()
{
    // Create and show a window to trigger shell integration initialzation,
    // otherwise we don't have anything to send ping events to.
    QRasterWindow window;
    window.resize(200, 200);
    window.show();

    // Verify that the client has bound to the global
    QCOMPOSITOR_TRY_COMPARE(get<XdgWmBase>()->resourceMap().size(), 1);

    QSignalSpy pongSpy(exec([&] { return get<XdgWmBase>(); }), &XdgWmBase::pong);
    const uint serial = exec([&] { return nextSerial(); });
    exec([&] {
        auto *base = get<XdgWmBase>();
        wl_resource *resource = base->resourceMap().first()->handle;
        base->send_ping(resource, serial);
    });
    QTRY_COMPARE(pongSpy.size(), 1);
    QCOMPARE(pongSpy.first().at(0).toUInt(), serial);
}

void tst_xdgshell::minMaxSize_data()
{
    QTest::addColumn<QSize>("initialMinSize");
    QTest::addColumn<QSize>("initialMaxSize");
    QTest::addColumn<QSize>("nextMinSize");
    QTest::addColumn<QSize>("nextMaxSize");
    QTest::addColumn<QSize>("expectedInitialMinSize");
    QTest::addColumn<QSize>("expectedInitialMaxSize");
    QTest::addColumn<QSize>("expectedNextMinSize");
    QTest::addColumn<QSize>("expectedNextMaxSize");

    QTest::newRow("onlyMinSize") << QSize(50, 60) << QSize() << QSize(500, 600) << QSize()
                                 << QSize(50, 60) << QSize(0, 0) << QSize(500, 600) << QSize(0, 0);

    QTest::newRow("onlyMaxSize") << QSize() << QSize(70, 80) << QSize() << QSize(700, 800)
                                 << QSize(0,0 ) << QSize(70, 80) << QSize(0, 0) << QSize(700, 800);

    QTest::newRow("maxIsSentAsZero") << QSize() << QSize(QWINDOWSIZE_MAX, QWINDOWSIZE_MAX) << QSize() << QSize()
                                 << QSize(0,0 ) << QSize(0, 0) << QSize(0, 0) << QSize(0, 0);


    QTest::newRow("fullHints") << QSize(50, 60) << QSize(700, 800) << QSize(500, 600) << QSize(710, 810)
                               << QSize(50, 60) << QSize(700, 800) << QSize(500, 600) << QSize(710, 810);

    // setting a minimum above the maximum is not allowed, we should no-op
    QTest::newRow("invalidResize") << QSize(50, 60) << QSize(100, 100) << QSize(500, 600) << QSize(100, 100)
                                   << QSize(50, 60) << QSize(100, 100) << QSize(50, 60) << QSize(100, 100);}

void tst_xdgshell::minMaxSize()
{
    QFETCH(QSize, initialMinSize);
    QFETCH(QSize, initialMaxSize);

    QFETCH(QSize, expectedInitialMinSize);
    QFETCH(QSize, expectedInitialMaxSize);

    QRasterWindow window;
    if (initialMinSize.isValid())
        window.setMinimumSize(initialMinSize);
    if (initialMaxSize.isValid())
        window.setMaximumSize(initialMaxSize);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());

    exec([&] { xdgToplevel()->sendCompleteConfigure(); });

    // we don't roundtrip with our configuration the initial commit should be correct

    QCOMPOSITOR_TRY_COMPARE(xdgToplevel()->m_committed.minSize, expectedInitialMinSize);
    QCOMPOSITOR_TRY_COMPARE(xdgToplevel()->m_committed.maxSize, expectedInitialMaxSize);

    QFETCH(QSize, nextMinSize);
    QFETCH(QSize, expectedNextMinSize);
    window.setMinimumSize(nextMinSize);
    window.update();
    QCOMPOSITOR_TRY_COMPARE(xdgToplevel()->m_committed.minSize, expectedNextMinSize);

    QFETCH(QSize, nextMaxSize);
    QFETCH(QSize, expectedNextMaxSize);

    window.setMaximumSize(nextMaxSize);
    window.update();
    QCOMPOSITOR_TRY_COMPARE(xdgToplevel()->m_committed.maxSize, expectedNextMaxSize);
}

void tst_xdgshell::windowGeometry()
{
    QRasterWindow window;
    window.resize(400, 320);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());

    exec([&] { xdgToplevel()->sendCompleteConfigure(); });

    QSize marginsSize;
    marginsSize.setWidth(window.frameMargins().left() + window.frameMargins().right());
    marginsSize.setHeight(window.frameMargins().top() + window.frameMargins().bottom());

    QCOMPOSITOR_TRY_COMPARE(xdgSurface()->m_committed.windowGeometry, QRect(QPoint(0, 0), QSize(400, 320) + marginsSize));

    window.resize(800, 600);
    QCOMPOSITOR_TRY_COMPARE(xdgSurface()->m_committed.windowGeometry, QRect(QPoint(0, 0), QSize(800, 600) + marginsSize));
}

void tst_xdgshell::foreignSurface()
{
    auto *ni = QGuiApplication::platformNativeInterface();
    auto *compositor = static_cast<::wl_compositor *>(ni->nativeResourceForIntegration("compositor"));
    ::wl_surface *foreignSurface = wl_compositor_create_surface(compositor);

    // There *could* be cursor surfaces lying around, we don't want to confuse those with
    // the foreign surface we will be creating.
    const int newSurfaceIndex = exec([&]{
        return get<WlCompositor>()->m_surfaces.size();
    });

    QCOMPOSITOR_TRY_VERIFY(surface(newSurfaceIndex));
    exec([&] {
        pointer()->sendEnter(surface(newSurfaceIndex), {32, 32});
        pointer()->sendLeave(surface(newSurfaceIndex));
    });

    // Just do something to make sure we don't destroy the surface before
    // the pointer events above are handled.
    QSignalSpy spy(exec([&] { return surface(newSurfaceIndex); }), &Surface::commit);
    wl_surface_commit(foreignSurface);
    QTRY_COMPARE(spy.size(), 1);

    wl_surface_destroy(foreignSurface);
}

void tst_xdgshell::nativeResources()
{
    QRasterWindow window;
    window.resize(400, 320);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());

    auto *ni = QGuiApplication::platformNativeInterface();
    auto *xdg_surface_proxy = static_cast<::wl_proxy *>(ni->nativeResourceForWindow("xdg_surface", &window));
    QCOMPARE(wl_proxy_get_class(xdg_surface_proxy), "xdg_surface");

    auto *xdg_toplevel_proxy = static_cast<::wl_proxy *>(ni->nativeResourceForWindow("xdg_toplevel", &window));
    QCOMPARE(wl_proxy_get_class(xdg_toplevel_proxy), "xdg_toplevel");

    auto *xdg_popup_proxy = static_cast<::wl_proxy *>(ni->nativeResourceForWindow("xdg_popup", &window));
    QCOMPARE(xdg_popup_proxy, nullptr);
}

void tst_xdgshell::suspended()
{
    QRasterWindow window;
    window.resize(400, 320);
    window.show();
    QVERIFY(!window.isExposed()); // not exposed until we're configured
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());

    uint serial = 0;
    exec([&] { serial = xdgToplevel()->sendCompleteConfigure(); });
    QTRY_VERIFY(window.isExposed());
    QCOMPOSITOR_TRY_COMPARE(xdgToplevel()->m_xdgSurface->m_committedConfigureSerial, serial);

    exec([&] { serial = xdgToplevel()->sendCompleteConfigure(QSize(), {XdgToplevel::state_suspended}); });
    QTRY_VERIFY(!window.isExposed());
    QCOMPOSITOR_TRY_COMPARE(xdgToplevel()->m_xdgSurface->m_committedConfigureSerial, serial);

    exec([&] { serial = xdgToplevel()->sendCompleteConfigure(QSize(), {}); });
    QTRY_VERIFY(window.isExposed());
    QCOMPOSITOR_TRY_COMPARE(xdgToplevel()->m_xdgSurface->m_committedConfigureSerial, serial);
}

void tst_xdgshell::initiallySuspended()
{
    QRasterWindow window;
    window.resize(400, 320);
    window.show();
    QVERIFY(!window.isExposed());
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());
    exec([&] { xdgToplevel()->sendCompleteConfigure(QSize(), {XdgToplevel::state_suspended}); });
    QVERIFY(!window.isExposed());
}

void tst_xdgshell::modality()
{
    QRasterWindow parent;
    parent.resize(400, 320);
    parent.show();

    QRasterWindow child;
    child.resize(400, 320);
    child.setTransientParent(&parent);
    child.show();
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel(1));
    QCOMPOSITOR_VERIFY(!xdgDialog());

    child.hide();
    child.setModality(Qt::WindowModal);
    child.show();
    QCOMPOSITOR_TRY_VERIFY(xdgDialog());
    QCOMPOSITOR_VERIFY(xdgDialog()->modal);

    child.hide();
    QCOMPOSITOR_TRY_VERIFY(!xdgDialog());

    child.setModality(Qt::ApplicationModal);
    child.show();
    QCOMPOSITOR_TRY_VERIFY(xdgDialog());
    QCOMPOSITOR_VERIFY(xdgDialog()->modal);

    child.hide();
    QCOMPOSITOR_TRY_VERIFY(!xdgDialog());

    child.show();
    child.setModality(Qt::NonModal);
    QCOMPOSITOR_TRY_VERIFY(!xdgDialog());
}

void tst_xdgshell::modalityWithoutTransientParent()
{
    QRasterWindow child;
    child.resize(400, 320);
    child.show();
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());
    QCOMPOSITOR_VERIFY(!xdgDialog());

    child.hide();
    child.setModality(Qt::WindowModal);
    child.show();
    QCOMPOSITOR_TRY_VERIFY(xdgDialog());
    QCOMPOSITOR_VERIFY(xdgDialog()->modal);

    child.hide();
    QCOMPOSITOR_TRY_VERIFY(!xdgDialog());

    child.setModality(Qt::ApplicationModal);
    child.show();
    QCOMPOSITOR_TRY_VERIFY(xdgDialog());
    QCOMPOSITOR_VERIFY(xdgDialog()->modal);

    child.hide();
    QCOMPOSITOR_TRY_VERIFY(!xdgDialog());

    child.show();
    child.setModality(Qt::NonModal);
    QCOMPOSITOR_TRY_VERIFY(!xdgDialog());
}

QCOMPOSITOR_TEST_MAIN(tst_xdgshell)
#include "tst_xdgshell.moc"
