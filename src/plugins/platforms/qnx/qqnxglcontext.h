/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
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

#ifndef QQNXGLCONTEXT_H
#define QQNXGLCONTEXT_H

#include <qpa/qplatformopenglcontext.h>
#include <QtGui/QSurfaceFormat>
#include <QtCore/QSize>

#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

class QQnxWindow;

class QQnxGLContext : public QPlatformOpenGLContext
{
public:
    QQnxGLContext(QOpenGLContext *glContext);
    virtual ~QQnxGLContext();

    static void initialize();
    static void shutdown();

    bool makeCurrent(QPlatformSurface *surface);
    void doneCurrent();
    void swapBuffers(QPlatformSurface *surface);
    QFunctionPointer getProcAddress(const QByteArray &procName);

    virtual QSurfaceFormat format() const { return m_windowFormat; }

    bool isCurrent() const;

    void createSurface(QPlatformSurface *surface);
    void destroySurface();

private:
    /** \todo Should this be non-static so we can use additional displays? */
    static EGLDisplay ms_eglDisplay;

    QSurfaceFormat m_windowFormat;
    QOpenGLContext *m_glContext;

    EGLConfig m_eglConfig;
    EGLContext m_eglContext;
    EGLSurface m_eglSurface;
    QSize m_surfaceSize;

    static EGLint *contextAttrs();
};

QT_END_NAMESPACE

#endif // QQNXGLCONTEXT_H
