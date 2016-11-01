/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
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

#include "qandroidplatformopenglcontext.h"
#include "qandroidplatformopenglwindow.h"
#include "qandroidplatformintegration.h"

#include <QtEglSupport/private/qeglpbuffer_p.h>

#include <QSurface>
#include <QtGui/private/qopenglcontext_p.h>

QT_BEGIN_NAMESPACE

QAndroidPlatformOpenGLContext::QAndroidPlatformOpenGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display)
    :QEGLPlatformContext(format, share, display)
{
}

void QAndroidPlatformOpenGLContext::swapBuffers(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() == QSurface::Window &&
            static_cast<QAndroidPlatformOpenGLWindow *>(surface)->checkNativeSurface(eglConfig())) {
        QEGLPlatformContext::makeCurrent(surface);
    }

    QEGLPlatformContext::swapBuffers(surface);
}

bool QAndroidPlatformOpenGLContext::needsFBOReadBackWorkaround()
{
    static bool set = false;
    static bool needsWorkaround = false;

    if (!set) {
        QByteArray env = qgetenv("QT_ANDROID_DISABLE_GLYPH_CACHE_WORKAROUND");
        needsWorkaround = env.isEmpty() || env == "0" || env == "false";

        if (!needsWorkaround) {
            const char *rendererString = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
            needsWorkaround =
                    qstrncmp(rendererString, "Mali-4xx", 6) == 0 // Mali-400, Mali-450
                    || qstrncmp(rendererString, "Adreno (TM) 2xx", 13) == 0 // Adreno 200, 203, 205
                    || qstrncmp(rendererString, "Adreno 2xx", 8) == 0 // Same as above but without the '(TM)'
                    || qstrncmp(rendererString, "Adreno (TM) 30x", 14) == 0 // Adreno 302, 305
                    || qstrncmp(rendererString, "Adreno 30x", 9) == 0 // Same as above but without the '(TM)'
                    || qstrcmp(rendererString, "GC800 core") == 0
                    || qstrcmp(rendererString, "GC1000 core") == 0
                    || qstrcmp(rendererString, "Immersion.16") == 0;
        }

        set = true;
    }

    return needsWorkaround;
}

bool QAndroidPlatformOpenGLContext::makeCurrent(QPlatformSurface *surface)
{
    bool ret = QEGLPlatformContext::makeCurrent(surface);
    QOpenGLContextPrivate *ctx_d = QOpenGLContextPrivate::get(context());

    const char *rendererString = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    if (rendererString != 0 && qstrncmp(rendererString, "Android Emulator", 16) == 0)
        ctx_d->workaround_missingPrecisionQualifiers = true;

    if (!ctx_d->workaround_brokenFBOReadBack && needsFBOReadBackWorkaround())
        ctx_d->workaround_brokenFBOReadBack = true;

    return ret;
}

EGLSurface QAndroidPlatformOpenGLContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() == QSurface::Window)
        return static_cast<QAndroidPlatformOpenGLWindow *>(surface)->eglSurface(eglConfig());
    else
        return static_cast<QEGLPbuffer *>(surface)->pbuffer();
}

QT_END_NAMESPACE
