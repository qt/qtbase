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

#ifndef QMINIMALEGLSCREEN_H
#define QMINIMALEGLSCREEN_H

#include <qpa/qplatformscreen.h>

#include <QtCore/QTextStream>

#include <QtEglSupport/private/qt_egl_p.h>

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
