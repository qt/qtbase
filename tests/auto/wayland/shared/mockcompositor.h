// Copyright (C) 2021 David Edmundson <davidedmundson@kde.org>
// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MOCKCOMPOSITOR_H
#define MOCKCOMPOSITOR_H

#include "corecompositor.h"
#include "coreprotocol.h"
#include "datadevice.h"
#include "fullscreenshellv1.h"
//#include "iviapplication.h"
#include "xdgshell.h"
#include "viewport.h"
#include "fractionalscalev1.h"
#include "xdgdialog.h"

#include <QtGui/QGuiApplication>

// As defined in linux/input-event-codes.h
#ifndef BTN_LEFT
#define BTN_LEFT 0x110
#endif
#ifndef BTN_RIGHT
#define BTN_RIGHT 0x111
#endif
#ifndef BTN_MIDDLE
#define BTN_MIDDLE 0x112
#endif

namespace MockCompositor {

class DefaultCompositor : public CoreCompositor
{
public:
    explicit DefaultCompositor(CompositorType t = CompositorType::Default, int socketFd = -1);
    // Convenience functions
    Output *output(int i = 0) { return getAll<Output>().value(i, nullptr); }
    Surface *surface(int i = 0);
    Subsurface *subSurface(int i = 0) { return get<SubCompositor>()->m_subsurfaces.value(i, nullptr); }
    WlShellSurface *wlShellSurface(int i = 0) { return get<WlShell>()->m_wlShellSurfaces.value(i, nullptr); }
    Surface *wlSurface(int i = 0);
    XdgSurface *xdgSurface(int i = 0) { return get<XdgWmBase>()->m_xdgSurfaces.value(i, nullptr); }
    XdgToplevel *xdgToplevel(int i = 0) { return get<XdgWmBase>()->toplevel(i); }
    XdgPopup *xdgPopup(int i = 0) { return get<XdgWmBase>()->popup(i); }
    Pointer *pointer() { auto *seat = get<Seat>(); Q_ASSERT(seat); return seat->m_pointer; }
    Touch *touch() { auto *seat = get<Seat>(); Q_ASSERT(seat); return seat->m_touch; }
    Surface *cursorSurface() { auto *p = pointer(); return p ? p->cursorSurface() : nullptr; }
    Keyboard *keyboard() { auto *seat = get<Seat>(); Q_ASSERT(seat); return seat->m_keyboard; }
    FullScreenShellV1 *fullScreenShellV1() {return get<FullScreenShellV1>();};
    //IviSurface *iviSurface(int i = 0) { return get<IviApplication>()->m_iviSurfaces.value(i, nullptr); }
    FractionalScale *fractionalScale(int i = 0) {return get<FractionalScaleManager>()->m_fractionalScales.value(i, nullptr); }
    Viewport *viewport(int i = 0) {return get<Viewporter>()->m_viewports.value(i, nullptr); }
    XdgDialog *xdgDialog(int i = 0) { return get<XdgWmDialog>()->m_dialogs.value(i, nullptr); }

    uint sendXdgShellPing();
    void xdgPingAndWaitForPong();

    void sendShellSurfaceConfigure(Surface *surface);

    // Things that can be changed run-time without confusing the client (i.e. don't require separate tests)
    struct Config {
        bool autoEnter = true;
        bool autoRelease = true;
        bool autoConfigure = false;
        bool autoFrameCallback = true;
    } m_config;
    void resetConfig() { exec([&] { m_config = Config{}; }); }
};

class WlShellCompositor : public DefaultCompositor
{
public:
    explicit WlShellCompositor(CompositorType t = CompositorType::Legacy);
};

} // namespace MockCompositor

#define QCOMPOSITOR_VERIFY(expr) QVERIFY(exec([&]{ return expr; }))
#define QCOMPOSITOR_TRY_VERIFY(expr) QTRY_VERIFY(exec([&]{ return expr; }))
#define QCOMPOSITOR_COMPARE(expr, expr2) QCOMPARE(exec([&]{ return expr; }), expr2)
#define QCOMPOSITOR_TRY_COMPARE(expr, expr2) QTRY_COMPARE(exec([&]{ return expr; }), expr2)

#define QCOMPOSITOR_TEST_MAIN(test) \
int main(int argc, char **argv) \
{ \
    QTemporaryDir tmpRuntimeDir; \
    qputenv("XDG_RUNTIME_DIR", tmpRuntimeDir.path().toLocal8Bit()); \
    qputenv("XDG_CURRENT_DESKTOP", "qtwaylandtests"); \
    qputenv("QT_QPA_PLATFORM", "wayland"); \
    test tc; \
    QGuiApplication app(argc, argv); \
    QTEST_SET_MAIN_SOURCE_PATH \
    return QTest::qExec(&tc, argc, argv); \
} \

#endif
