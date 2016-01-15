/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QWINDOWSOPENGLCONTEXT_H
#define QWINDOWSOPENGLCONTEXT_H

#include <QtGui/QOpenGLContext>
#include <qpa/qplatformopenglcontext.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_OPENGL

class QWindowsOpenGLContext;

class QWindowsStaticOpenGLContext
{
public:
    static QWindowsStaticOpenGLContext *create();
    virtual ~QWindowsStaticOpenGLContext() { }

    virtual QWindowsOpenGLContext *createContext(QOpenGLContext *context) = 0;
    virtual void *moduleHandle() const = 0;
    virtual QOpenGLContext::OpenGLModuleType moduleType() const = 0;
    virtual bool supportsThreadedOpenGL() const { return false; }

    // If the windowing system interface needs explicitly created window surfaces (like EGL),
    // reimplement these.
    virtual void *createWindowSurface(void * /*nativeWindow*/, void * /*nativeConfig*/, int * /*err*/) { return 0; }
    virtual void destroyWindowSurface(void * /*nativeSurface*/) { }

private:
    static QWindowsStaticOpenGLContext *doCreate();
};

class QWindowsOpenGLContext : public QPlatformOpenGLContext
{
public:
    virtual ~QWindowsOpenGLContext() { }

    // Returns the native context handle (e.g. HGLRC for WGL, EGLContext for EGL).
    virtual void *nativeContext() const = 0;

    // These should be implemented only for some winsys interfaces, for example EGL.
    // For others, like WGL, they are not relevant.
    virtual void *nativeDisplay() const { return 0; }
    virtual void *nativeConfig() const { return 0; }
};

#endif // QT_NO_OPENGL

QT_END_NAMESPACE

#endif // QWINDOWSOPENGLCONTEXT_H
