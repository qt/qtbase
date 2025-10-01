// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandshmwindow_p.h"

#include "qwaylandbuffer_p.h"

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandShmWindow::QWaylandShmWindow(QWindow *window, QWaylandDisplay *display)
    : QWaylandWindow(window, display)
{
    mSurfaceFormat.setRedBufferSize(8);
    mSurfaceFormat.setGreenBufferSize(8);
    mSurfaceFormat.setBlueBufferSize(8);

    const QSurfaceFormat format = window->requestedFormat();
    mSurfaceFormat.setAlphaBufferSize(format.hasAlpha() ? 8 : 0);
}

QWaylandShmWindow::~QWaylandShmWindow()
{
}

QWaylandWindow::WindowType QWaylandShmWindow::windowType() const
{
    return QWaylandWindow::Shm;
}

bool QWaylandShmWindow::createDecoration()
{
    bool rc = QWaylandWindow::createDecoration();

    const QSurfaceFormat format = window()->requestedFormat();
    if (!format.hasAlpha())
        mSurfaceFormat.setAlphaBufferSize(mWindowDecorationEnabled ? 8 : 0);

    return rc;
}

}

QT_END_NAMESPACE
