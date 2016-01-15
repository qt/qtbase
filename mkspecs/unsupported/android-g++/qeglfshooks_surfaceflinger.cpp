/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the qmake spec of the Qt Toolkit.
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

#include "qeglfshooks.h"

#include <ui/DisplayInfo.h>
#include <ui/FramebufferNativeWindow.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>

#if Q_ANDROID_VERSION_MAJOR > 4 || (Q_ANDROID_VERSION_MAJOR == 4 && Q_ANDROID_VERSION_MINOR >= 1)
#include <gui/SurfaceComposerClient.h>
#else
#include <surfaceflinger/SurfaceComposerClient.h>
#endif

using namespace android;

QT_BEGIN_NAMESPACE

class QEglFSPandaHooks : public QEglFSHooks
{
public:
    QEglFSPandaHooks();
    virtual EGLNativeWindowType createNativeWindow(QPlatformWindow *window, const QSize &size, const QSurfaceFormat &format);
    virtual bool filterConfig(EGLDisplay display, EGLConfig config) const;
    virtual const char *fbDeviceName() const { return "/dev/graphics/fb0"; }

private:
    EGLNativeWindowType createNativeWindowSurfaceFlinger(const QSize &size, const QSurfaceFormat &format);
    EGLNativeWindowType createNativeWindowFramebuffer(const QSize &size, const QSurfaceFormat &format);

    void ensureFramebufferNativeWindowCreated();

    // androidy things
    sp<android::SurfaceComposerClient> mSession;
    sp<android::SurfaceControl> mControl;
    sp<android::Surface> mAndroidSurface;

    sp<android::FramebufferNativeWindow> mFramebufferNativeWindow;
    EGLint mFramebufferVisualId;

    bool mUseFramebuffer;
};

QEglFSPandaHooks::QEglFSPandaHooks()
    : mFramebufferVisualId(EGL_DONT_CARE)
{
    mUseFramebuffer = qgetenv("QT_QPA_EGLFS_NO_SURFACEFLINGER").toInt();
}

void QEglFSPandaHooks::ensureFramebufferNativeWindowCreated()
{
    if (mFramebufferNativeWindow.get())
        return;
    mFramebufferNativeWindow = new FramebufferNativeWindow();
    framebuffer_device_t const *fbDev = mFramebufferNativeWindow->getDevice();
    if (!fbDev)
        qFatal("Failed to get valid FramebufferNativeWindow, no way to create EGL surfaces");

    ANativeWindow *window = mFramebufferNativeWindow.get();

    window->query(window, NATIVE_WINDOW_FORMAT, &mFramebufferVisualId);
}

EGLNativeWindowType QEglFSPandaHooks::createNativeWindow(QPlatformWindow *window, const QSize &size, const QSurfaceFormat &format)
{
    Q_UNUSED(window)
    return mUseFramebuffer ? createNativeWindowFramebuffer(size, format) : createNativeWindowSurfaceFlinger(size, format);
}

EGLNativeWindowType QEglFSPandaHooks::createNativeWindowFramebuffer(const QSize &size, const QSurfaceFormat &)
{
    Q_UNUSED(size);
    ensureFramebufferNativeWindowCreated();
    return mFramebufferNativeWindow.get();
}

EGLNativeWindowType QEglFSPandaHooks::createNativeWindowSurfaceFlinger(const QSize &size, const QSurfaceFormat &)
{
    Q_UNUSED(size);

    mSession = new SurfaceComposerClient();
    DisplayInfo dinfo;
    int status=0;
    status = mSession->getDisplayInfo(0, &dinfo);
    mControl = mSession->createSurface(
            0, dinfo.w, dinfo.h, PIXEL_FORMAT_RGB_888);
    SurfaceComposerClient::openGlobalTransaction();
    mControl->setLayer(0x40000000);
//    mControl->setAlpha(1);
    SurfaceComposerClient::closeGlobalTransaction();
    mAndroidSurface = mControl->getSurface();

    EGLNativeWindowType eglWindow = mAndroidSurface.get();
    return eglWindow;
}

bool QEglFSPandaHooks::filterConfig(EGLDisplay display, EGLConfig config) const
{
    if (!mUseFramebuffer)
        return true;

    const_cast<QEglFSPandaHooks *>(this)->ensureFramebufferNativeWindowCreated();

    if (mFramebufferVisualId == EGL_DONT_CARE)
        return true;

    EGLint nativeVisualId = 0;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &nativeVisualId);

    return nativeVisualId == mFramebufferVisualId;
}

static QEglFSPandaHooks eglFSPandaHooks;
QEglFSHooks *platformHooks = &eglFSPandaHooks;

QT_END_NAMESPACE
