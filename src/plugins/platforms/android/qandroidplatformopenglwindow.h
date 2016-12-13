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

#ifndef QANDROIDPLATFORMOPENGLWINDOW_H
#define QANDROIDPLATFORMOPENGLWINDOW_H

#include <EGL/egl.h>
#include <QWaitCondition>
#include <QtCore/private/qjni_p.h>

#include "androidsurfaceclient.h"
#include "qandroidplatformwindow.h"

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
    QJNIObjectPrivate m_androidSurfaceObject;
    QWaitCondition m_surfaceWaitCondition;
    QSurfaceFormat m_format;
    QRect m_oldGeometry;
};

QT_END_NAMESPACE
#endif // QANDROIDPLATFORMOPENGLWINDOW_H
