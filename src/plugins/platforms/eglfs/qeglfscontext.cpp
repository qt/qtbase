/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qeglfscontext.h"
#include "qeglfswindow.h"
#include "qeglfscursor.h"
#include "qeglfshooks.h"

#include <QtDebug>

QT_BEGIN_NAMESPACE

QEglFSContext::QEglFSContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share,
                             EGLDisplay display, EGLenum eglApi)
    : QEGLPlatformContext(format, share, display, eglApi)
{
}

bool QEglFSContext::makeCurrent(QPlatformSurface *surface)
{
    // create the native window surface. this makes sure that
    // we create surfaces only for painted widgets (unlike QDesktopWidget)
    (static_cast<QEglFSWindow *>(surface))->create();
    return QEGLPlatformContext::makeCurrent(surface);
}

EGLSurface QEglFSContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    QEglFSWindow *window = static_cast<QEglFSWindow *>(surface);
    return window->surface();
}

void QEglFSContext::swapBuffers(QPlatformSurface *surface)
{
    QEglFSWindow *window = static_cast<QEglFSWindow *>(surface);
    // draw the cursor
    if (QEglFSCursor *cursor = static_cast<QEglFSCursor *>(window->screen()->cursor()))
        cursor->paintOnScreen();

    QEGLPlatformContext::swapBuffers(surface);
}

QT_END_NAMESPACE

