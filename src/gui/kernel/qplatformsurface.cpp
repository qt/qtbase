/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

