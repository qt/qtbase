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

#ifndef QXCBEGLINTEGRATION_H
#define QXCBEGLINTEGRATION_H

#include "qxcbglintegration.h"

#include "qxcbeglwindow.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/qpa/qplatformscreen.h>
#include <QtGui/QScreen>

#include "qxcbscreen.h"

#include "qxcbeglinclude.h"

QT_BEGIN_NAMESPACE

class QXcbEglNativeInterfaceHandler;

class QXcbEglIntegration : public QXcbGlIntegration
{
public:
    QXcbEglIntegration();
    ~QXcbEglIntegration();

    bool initialize(QXcbConnection *connection) override;

    QXcbWindow *createWindow(QWindow *window) const override;
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
    QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const override;

    bool supportsThreadedOpenGL() const override { return true; }

    EGLDisplay eglDisplay() const { return m_egl_display; }
    void *xlib_display() const;
private:
    QXcbConnection *m_connection;
    EGLDisplay m_egl_display;

    QScopedPointer<QXcbEglNativeInterfaceHandler> m_native_interface_handler;
};

QT_END_NAMESPACE
#endif //QXCBEGLINTEGRATION_H
