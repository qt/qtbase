// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPLATFORMOFFSCREENSURFACETEXTURE_H
#define QOHOSPLATFORMOFFSCREENSURFACETEXTURE_H

#include <qpa/qplatformoffscreensurface.h>
#include <QtGui/private/qeglplatformcontext_p.h>

QT_BEGIN_NAMESPACE
class QOffscreenSurface;
class QOhosPlatformOffscreenSurface : public QPlatformOffscreenSurface
{
public:
    QOhosPlatformOffscreenSurface(EGLDisplay display, const QSurfaceFormat &format,
                                            QOffscreenSurface *offscreenSurface);
    ~QOhosPlatformOffscreenSurface();

    QSurfaceFormat format() const override { return m_format; }
    bool isValid() const override { return m_surface != EGL_NO_SURFACE; }

    EGLSurface surface() const { return m_surface; }
private:
    QSurfaceFormat m_format;
    EGLDisplay m_display;
    EGLSurface m_surface;
};

QT_END_NAMESPACE

#endif // QOHOSPLATFORMOFFSCREENSURFACETEXTURE_H
