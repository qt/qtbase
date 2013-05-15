/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfshooks.h"
#include "androidjnimain.h"
#include "qandroidplatformintegration.h"

#include <android/native_window.h>
#include <jni.h>

QT_BEGIN_NAMESPACE

class QEglFSAndroidHooks: public QEglFSHooks
{
public:
    void platformInit();
    void platformDestroy();
    EGLNativeDisplayType platformDisplay() const;
    QSize screenSize() const;
    QSizeF physicalScreenSize() const;
    QDpi logicalDpi() const;
    int screenDepth() const;
    QSurfaceFormat surfaceFormatFor(const QSurfaceFormat &inputFormat) const;
    EGLNativeWindowType createNativeWindow(const QSize &size, const QSurfaceFormat &format);
    void destroyNativeWindow(EGLNativeWindowType window);
    bool hasCapability(QPlatformIntegration::Capability cap) const;
    QEglFSCursor *createCursor(QEglFSScreen *screen) const;
};

void QEglFSAndroidHooks::platformInit()
{
}

void QEglFSAndroidHooks::platformDestroy()
{
}

EGLNativeDisplayType QEglFSAndroidHooks::platformDisplay() const
{
    return EGL_DEFAULT_DISPLAY;
}

QSize QEglFSAndroidHooks::screenSize() const
{
    return QtAndroid::nativeWindowSize();
}

QSizeF QEglFSAndroidHooks::physicalScreenSize() const
{
    return QSizeF(QAndroidPlatformIntegration::m_defaultPhysicalSizeWidth, QAndroidPlatformIntegration::m_defaultPhysicalSizeHeight);
}

QDpi QEglFSAndroidHooks::logicalDpi() const
{
    qreal lDpi = QtAndroid::scaledDensity() * 100;
    return QDpi(lDpi, lDpi);
}


EGLNativeWindowType QEglFSAndroidHooks::createNativeWindow(const QSize &size, const QSurfaceFormat &format)
{
    Q_UNUSED(size);
    ANativeWindow *window = QtAndroid::nativeWindow();
    if (window != 0)
        ANativeWindow_acquire(window);

    return window;
}

void QEglFSAndroidHooks::destroyNativeWindow(EGLNativeWindowType window)
{
    ANativeWindow_release(window);
}

bool QEglFSAndroidHooks::hasCapability(QPlatformIntegration::Capability capability) const
{
    switch (capability) {
    case QPlatformIntegration::OpenGL: return true;
    case QPlatformIntegration::ThreadedOpenGL: return true;
    default: return false;
    };
}

int QEglFSAndroidHooks::screenDepth() const
{
    // ### Hardcoded
    return 32;
}

QSurfaceFormat QEglFSAndroidHooks::surfaceFormatFor(const QSurfaceFormat &inputFormat) const
{
    QSurfaceFormat ret(inputFormat);
    ret.setAlphaBufferSize(8);
    ret.setRedBufferSize(8);
    ret.setGreenBufferSize(8);
    ret.setBlueBufferSize(8);
    return ret;
}

QEglFSCursor *QEglFSAndroidHooks::createCursor(QEglFSScreen *screen) const
{
    Q_UNUSED(screen);
    return 0;
}

static QEglFSAndroidHooks eglFSAndroidHooks;
QEglFSHooks *platformHooks = &eglFSAndroidHooks;

QT_END_NAMESPACE
