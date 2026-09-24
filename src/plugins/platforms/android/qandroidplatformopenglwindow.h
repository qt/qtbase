// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QANDROIDPLATFORMOPENGLWINDOW_H
#define QANDROIDPLATFORMOPENGLWINDOW_H

#include "qandroidplatformwindow.h"

#include <QtCore/qjnienvironment.h>
#include <QtCore/qjniobject.h>

#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

class QAndroidPlatformOpenGLWindow : public QAndroidPlatformWindow
{
public:
    explicit QAndroidPlatformOpenGLWindow(QWindow *window, EGLDisplay display);
    ~QAndroidPlatformOpenGLWindow();

    void setGeometry(const QRect &rect) override;
    // Render thread. Creates the EGLSurface on first use, EGL_NO_SURFACE until
    // Android provides a Surface (the window isn't exposed before that).
    EGLSurface eglSurface(EGLConfig config);
    QSurfaceFormat format() const override;

private:
    void destroyWindowSurface() override;
    EGLDisplay m_eglDisplay = EGL_NO_DISPLAY;

    // Render-thread-only state.
    EGLSurface m_eglSurface = EGL_NO_SURFACE;
    QSurfaceFormat m_format;
};

QT_END_NAMESPACE
#endif // QANDROIDPLATFORMOPENGLWINDOW_H
