// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpa/qwindowsysteminterface.h>

#include "qminimaleglwindow.h"

QT_BEGIN_NAMESPACE

QMinimalEglWindow::QMinimalEglWindow(QWindow *w)
    : QPlatformWindow(w)
{
    static int serialNo = 0;
    m_winid  = ++serialNo;
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglWindow %p: %p 0x%x\n", this, w, uint(m_winid));
#endif

    QRect screenGeometry(screen()->availableGeometry());
    if (w->geometry() != screenGeometry) {
        QWindowSystemInterface::handleGeometryChange(w, screenGeometry);
    }
    w->setSurfaceType(QSurface::OpenGLSurface);
}

void QMinimalEglWindow::setGeometry(const QRect &)
{
    // We only support full-screen windows
    QRect rect(screen()->availableGeometry());
    QWindowSystemInterface::handleGeometryChange(window(), rect);

    QPlatformWindow::setGeometry(rect);
}

WId QMinimalEglWindow::winId() const
{
    return m_winid;
}

QT_END_NAMESPACE
