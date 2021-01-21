/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QANDROIDPLATFORMOFFSCREENSURFACETEXTURE_H
#define QANDROIDPLATFORMOFFSCREENSURFACETEXTURE_H

#include <qpa/qplatformoffscreensurface.h>
#include <QtEglSupport/private/qeglplatformcontext_p.h>

QT_BEGIN_NAMESPACE
class QOffscreenSurface;
class QAndroidPlatformOffscreenSurface : public QPlatformOffscreenSurface
{
public:
    QAndroidPlatformOffscreenSurface(EGLDisplay display, const QSurfaceFormat &format,
                                            QOffscreenSurface *offscreenSurface);
    ~QAndroidPlatformOffscreenSurface();

    QSurfaceFormat format() const override { return m_format; }
    bool isValid() const override { return m_surface != EGL_NO_SURFACE; }

    EGLSurface surface() const { return m_surface; }
private:
    QSurfaceFormat m_format;
    EGLDisplay m_display;
    EGLSurface m_surface;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMOFFSCREENSURFACETEXTURE_H
