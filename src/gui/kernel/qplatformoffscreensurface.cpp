// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformoffscreensurface.h"

#include "qoffscreensurface.h"
#include "qscreen.h"

QT_BEGIN_NAMESPACE

class QPlatformOffscreenSurfacePrivate
{
public:
};

QPlatformOffscreenSurface::QPlatformOffscreenSurface(QOffscreenSurface *offscreenSurface)
    : QPlatformSurface(offscreenSurface)
    , d_ptr(new QPlatformOffscreenSurfacePrivate)
{
}

QPlatformOffscreenSurface::~QPlatformOffscreenSurface()
{
}

QOffscreenSurface *QPlatformOffscreenSurface::offscreenSurface() const
{
    return static_cast<QOffscreenSurface*>(m_surface);
}

/*!
    Returns the platform screen handle corresponding to this QPlatformOffscreenSurface.
*/
QPlatformScreen *QPlatformOffscreenSurface::screen() const
{
    return offscreenSurface()->screen()->handle();
}

/*!
    Returns the actual surface format of the offscreen surface.
*/
QSurfaceFormat QPlatformOffscreenSurface::format() const
{
    return QSurfaceFormat();
}

/*!
    Returns \c true if the platform offscreen surface has been allocated.
*/
bool QPlatformOffscreenSurface::isValid() const
{
    return false;
}

QT_END_NAMESPACE
