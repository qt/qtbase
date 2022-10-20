/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
