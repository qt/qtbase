/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QXCBEGLCONTEXT_H
#define QXCBEGLCONTEXT_H

#include "qxcbeglwindow.h"
#include <QtPlatformSupport/private/qeglplatformcontext_p.h>
#include <QtPlatformSupport/private/qeglpbuffer_p.h>
#include <QtPlatformHeaders/QEGLNativeContext>

QT_BEGIN_NAMESPACE

//####todo remove the noops (looks like their where there in the initial commit)
class QXcbEglContext : public QEGLPlatformContext
{
public:
    QXcbEglContext(const QSurfaceFormat &glFormat, QPlatformOpenGLContext *share,
                           EGLDisplay display, QXcbConnection *c, const QVariant &nativeHandle)
        : QEGLPlatformContext(glFormat, share, display, 0, nativeHandle)
        , m_connection(c)
    {
        Q_XCB_NOOP(m_connection);
    }

    void swapBuffers(QPlatformSurface *surface)
    {
        Q_XCB_NOOP(m_connection);
        QEGLPlatformContext::swapBuffers(surface);
        Q_XCB_NOOP(m_connection);
    }

    bool makeCurrent(QPlatformSurface *surface)
    {
        Q_XCB_NOOP(m_connection);
        bool ret = QEGLPlatformContext::makeCurrent(surface);
        Q_XCB_NOOP(m_connection);
        return ret;
    }

    void doneCurrent()
    {
        Q_XCB_NOOP(m_connection);
        QEGLPlatformContext::doneCurrent();
        Q_XCB_NOOP(m_connection);
    }

    EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface)
    {
        if (surface->surface()->surfaceClass() == QSurface::Window)
            return static_cast<QXcbEglWindow *>(surface)->eglSurface();
        else
            return static_cast<QEGLPbuffer *>(surface)->pbuffer();
    }

    QVariant nativeHandle() const {
        return QVariant::fromValue<QEGLNativeContext>(QEGLNativeContext(eglContext(), eglDisplay()));
    }

private:
    QXcbConnection *m_connection;
};

QT_END_NAMESPACE
#endif //QXCBEGLCONTEXT_H

