/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
