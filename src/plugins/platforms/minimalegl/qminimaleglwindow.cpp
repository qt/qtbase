/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
