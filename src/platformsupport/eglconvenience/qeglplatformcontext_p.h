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

#ifndef QEGLPLATFORMCONTEXT_H
#define QEGLPLATFORMCONTEXT_H

#include <qpa/qplatformwindow.h>
#include <qpa/qplatformopenglcontext.h>
#include <EGL/egl.h>

class QEGLPlatformContext : public QPlatformOpenGLContext
{
public:
    QEGLPlatformContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display,
                        EGLenum eglApi = EGL_OPENGL_ES_API);
    ~QEGLPlatformContext();

    bool makeCurrent(QPlatformSurface *surface);
    void doneCurrent();
    void swapBuffers(QPlatformSurface *surface);
    void (*getProcAddress(const QByteArray &procName)) ();

    QSurfaceFormat format() const;
    bool isSharing() const { return m_shareContext != EGL_NO_CONTEXT; }
    bool isValid() const { return m_eglContext != EGL_NO_CONTEXT; }

    EGLContext eglContext() const;

protected:
    virtual EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface) = 0;

private:
    EGLContext m_eglContext;
    EGLContext m_shareContext;
    EGLDisplay m_eglDisplay;
    EGLenum m_eglApi;

    QSurfaceFormat m_format;
};

#endif //QEGLPLATFORMCONTEXT_H
