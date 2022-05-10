// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDPLATFORMOPENGLWINDOW_H
#define QANDROIDPLATFORMOPENGLWINDOW_H

#include "androidsurfaceclient.h"
#include "qandroidplatformwindow.h"

#include <QWaitCondition>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjniobject.h>

#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

class QAndroidPlatformOpenGLWindow : public QAndroidPlatformWindow, public AndroidSurfaceClient
{
public:
    explicit QAndroidPlatformOpenGLWindow(QWindow *window, EGLDisplay display);
    ~QAndroidPlatformOpenGLWindow();

    void setGeometry(const QRect &rect) override;
    EGLSurface eglSurface(EGLConfig config);
    QSurfaceFormat format() const override;

    bool checkNativeSurface(EGLConfig config);

    void applicationStateChanged(Qt::ApplicationState) override;

    void repaint(const QRegion &region) override;

protected:
    void surfaceChanged(JNIEnv *jniEnv, jobject surface, int w, int h) override;
    void createEgl(EGLConfig config);
    void clearEgl();

private:
    EGLDisplay m_eglDisplay = EGL_NO_DISPLAY;
    EGLSurface m_eglSurface = EGL_NO_SURFACE;
    EGLNativeWindowType m_nativeWindow = nullptr;

    int m_nativeSurfaceId = -1;
    QJniObject m_androidSurfaceObject;
    QWaitCondition m_surfaceWaitCondition;
    QSurfaceFormat m_format;
    QRect m_oldGeometry;
};

QT_END_NAMESPACE
#endif // QANDROIDPLATFORMOPENGLWINDOW_H
