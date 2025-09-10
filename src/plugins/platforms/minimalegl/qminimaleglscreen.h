// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMINIMALEGLSCREEN_H
#define QMINIMALEGLSCREEN_H

#include <qpa/qplatformscreen.h>

#include <QtCore/QTextStream>

#include <QtGui/private/qt_egl_p.h>

QT_BEGIN_NAMESPACE

class QPlatformOpenGLContext;

class QMinimalEglScreen : public QPlatformScreen
{
public:
    QMinimalEglScreen(EGLNativeDisplayType display);
    ~QMinimalEglScreen();

    QRect geometry() const override;
    int depth() const override;
    QImage::Format format() const override;
#ifndef QT_NO_OPENGL
    QPlatformOpenGLContext *platformContext() const;
#endif
    EGLSurface surface() const { return m_surface; }

private:
    void createAndSetPlatformContext() const;
    void createAndSetPlatformContext();

    QRect m_geometry;
    int m_depth;
    QImage::Format m_format;
    QPlatformOpenGLContext *m_platformContext;
    EGLDisplay m_dpy;
    EGLSurface m_surface;
};

QT_END_NAMESPACE
#endif // QMINIMALEGLSCREEN_H
