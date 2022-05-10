// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDPLATFORMOFFSCREENSURFACETEXTURE_H
#define QANDROIDPLATFORMOFFSCREENSURFACETEXTURE_H

#include <qpa/qplatformoffscreensurface.h>
#include <QtGui/qoffscreensurface_platform.h>

#include <EGL/egl.h>

QT_BEGIN_NAMESPACE
class QOffscreenSurface;
class QAndroidPlatformOffscreenSurface : public QPlatformOffscreenSurface,
                                         public QNativeInterface::QAndroidOffscreenSurface
{
public:
    QAndroidPlatformOffscreenSurface(ANativeWindow *nativeSurface, EGLDisplay display, QOffscreenSurface *offscreenSurface);
    ~QAndroidPlatformOffscreenSurface();

    QSurfaceFormat format() const override { return m_format; }
    bool isValid() const override { return m_surface != EGL_NO_SURFACE; }

    EGLSurface surface() const { return m_surface; }

    ANativeWindow *nativeSurface() const override { return (ANativeWindow *)surface(); };

private:
    QSurfaceFormat m_format;
    EGLDisplay m_display;
    EGLSurface m_surface;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMOFFSCREENSURFACETEXTURE_H
