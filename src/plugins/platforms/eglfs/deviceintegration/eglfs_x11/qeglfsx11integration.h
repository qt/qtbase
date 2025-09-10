// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEGLFSX11INTEGRATION_H
#define QEGLFSX11INTEGRATION_H

#include "private/qeglfsdeviceintegration_p.h"

#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformwindow.h>

#include <xcb/xcb.h>

QT_BEGIN_NAMESPACE

namespace Atoms {
    enum {
        _NET_WM_NAME = 0,
        UTF8_STRING,
        WM_PROTOCOLS,
        WM_DELETE_WINDOW,
        _NET_WM_STATE,
        _NET_WM_STATE_FULLSCREEN,

        N_ATOMS
    };
}

class EventReader;

class QEglFSX11Integration : public QEglFSDeviceIntegration
{
public:
    QEglFSX11Integration() : m_connection(nullptr), m_window(0), m_eventReader(nullptr) {}

    void platformInit() override;
    void platformDestroy() override;
    EGLNativeDisplayType platformDisplay() const override;
    QSize screenSize() const override;
    EGLNativeWindowType createNativeWindow(QPlatformWindow *window,
                                           const QSize &size,
                                           const QSurfaceFormat &format) override;
    void destroyNativeWindow(EGLNativeWindowType window) override;
    bool hasCapability(QPlatformIntegration::Capability cap) const override;

    xcb_connection_t *connection() { return m_connection; }
    const xcb_atom_t *atoms() const { return m_atoms; }
    QPlatformWindow *platformWindow() { return m_platformWindow; }

private:
    void sendConnectionEvent(xcb_atom_t a);

    void *m_display;
    xcb_connection_t *m_connection;
    xcb_atom_t m_atoms[Atoms::N_ATOMS];
    xcb_window_t m_window;
    EventReader *m_eventReader;
    xcb_window_t m_connectionEventListener;
    QPlatformWindow *m_platformWindow;
    mutable QSize m_screenSize;
};

QT_END_NAMESPACE

#endif
