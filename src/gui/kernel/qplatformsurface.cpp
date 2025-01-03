// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

bool QPlatformSurface::isRasterSurface(QSurface *surface)
{
    switch (surface->surfaceType()) {
    case QSurface::RasterSurface:
    case QSurface::RasterGLSurface:
        return true;
    default:
        return false;
    };
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

