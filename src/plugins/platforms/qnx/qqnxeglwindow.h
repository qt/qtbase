// Copyright (C) 2013 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXEGLWINDOW_H
#define QQNXEGLWINDOW_H

#include "qqnxwindow.h"
#include <QtCore/QMutex>
#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaWindowEgl);

class QQnxGLContext;

class QQnxEglWindow : public QQnxWindow
{
public:
    QQnxEglWindow(QWindow *window, screen_context_t context, bool needRootWindow);
    ~QQnxEglWindow();

    EGLSurface surface() const;

    bool isInitialized() const;
    void ensureInitialized(QQnxGLContext *context);

    void setGeometry(const QRect &rect) override;

    QSurfaceFormat format() const override { return m_format; }

protected:
    int pixelFormat() const override;
    void resetBuffers() override;

private:
    void createEGLSurface(QQnxGLContext *context);
    void destroyEGLSurface();

    QSize m_requestedBufferSize;

    // This mutex is used to protect access to the m_requestedBufferSize
    // member. This member is used in conjunction with QQnxGLContext::requestNewSurface()
    // to coordinate recreating the EGL surface which involves destroying any
    // existing EGL surface; resizing the native window buffers; and creating a new
    // EGL surface. All of this has to be done from the thread that is calling
    // QQnxGLContext::makeCurrent()
    mutable QMutex m_mutex;

    QAtomicInt m_newSurfaceRequested;
    EGLDisplay m_eglDisplay;
    EGLConfig m_eglConfig;
    EGLSurface m_eglSurface;
    QSurfaceFormat m_format;
};

QT_END_NAMESPACE

#endif // QQNXEGLWINDOW_H
