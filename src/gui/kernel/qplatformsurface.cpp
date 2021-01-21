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

#include "qplatformsurface.h"
#ifndef QT_NO_DEBUG_STREAM
#include <QtCore/qdebug.h>
#include <QtGui/qwindow.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformSurface
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformSurface class provides an abstraction for a surface.
 */
QPlatformSurface::~QPlatformSurface()
{

}

QSurface *QPlatformSurface::surface() const
{
    return m_surface;
}

QPlatformSurface::QPlatformSurface(QSurface *surface) : m_surface(surface)
{
}

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug debug, const QPlatformSurface *surface)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "QPlatformSurface(" << (const void *)surface;
    if (surface) {
        QSurface *s = surface->surface();
        auto surfaceClass = s->surfaceClass();
        debug << ", class=" << surfaceClass;
        debug << ", type=" << s->surfaceType();
        if (surfaceClass == QSurface::Window)
            debug << ", window=" << static_cast<QWindow *>(s);
        else
            debug << ", surface=" << s;
    }
    debug << ')';
    return debug;
}
#endif // !QT_NO_DEBUG_STREAM

QT_END_NAMESPACE

