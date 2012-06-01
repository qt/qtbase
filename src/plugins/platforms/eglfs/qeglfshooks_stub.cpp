/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the qmake spec of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfshooks.h"

QT_BEGIN_NAMESPACE

void QEglFSHooks::platformInit()
{
}

void QEglFSHooks::platformDestroy()
{
}

EGLNativeDisplayType QEglFSHooks::platformDisplay() const
{
    return EGL_DEFAULT_DISPLAY;
}

QSize QEglFSHooks::screenSize() const
{
    return QSize();
}

int QEglFSHooks::screenDepth() const
{
    static int depth = qgetenv("QT_QPA_EGLFS_DEPTH").toInt();
    return depth == 16 ? 16 : 32;
}

QImage::Format QEglFSHooks::screenFormat() const
{
    return screenDepth() == 16 ? QImage::Format_RGB16 : QImage::Format_RGB32;
}

QSurfaceFormat QEglFSHooks::defaultSurfaceFormat() const
{
    QSurfaceFormat format;
    if (screenDepth() == 16) {
        format.setDepthBufferSize(16);
        format.setRedBufferSize(5);
        format.setGreenBufferSize(6);
        format.setBlueBufferSize(5);
    } else {
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        format.setRedBufferSize(8);
        format.setGreenBufferSize(8);
        format.setBlueBufferSize(8);
    }

    static int samples = qgetenv("QT_QPA_EGLFS_MULTISAMPLE").toInt();
    format.setSamples(samples);

    return format;
}

EGLNativeWindowType QEglFSHooks::createNativeWindow(const QSize &size)
{
    Q_UNUSED(size);
    return 0;
}

void QEglFSHooks::destroyNativeWindow(EGLNativeWindowType window)
{
    Q_UNUSED(window);
}

bool QEglFSHooks::hasCapability(QPlatformIntegration::Capability cap) const
{
    Q_UNUSED(cap);
    return false;
}

QEglFSCursor *QEglFSHooks::createCursor(QEglFSScreen *screen) const
{
    Q_UNUSED(screen);
    return 0;
}

#ifndef EGLFS_PLATFORM_HOOKS
QEglFSHooks stubHooks;
#endif

QT_END_NAMESPACE
