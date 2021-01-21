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

#ifndef QCOCOAGLCONTEXT_H
#define QCOCOAGLCONTEXT_H

#include <QtCore/QPointer>
#include <QtCore/private/qcore_mac_p.h>
#include <qpa/qplatformopenglcontext.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QWindow>

#include <AppKit/AppKit.h>

QT_BEGIN_NAMESPACE

class QCocoaWindow;

class QCocoaGLContext : public QPlatformOpenGLContext
{
public:
    QCocoaGLContext(QOpenGLContext *context);
    ~QCocoaGLContext();

    void initialize() override;

    bool makeCurrent(QPlatformSurface *surface) override;
    void swapBuffers(QPlatformSurface *surface) override;
    void doneCurrent() override;

    void update();

    QSurfaceFormat format() const override;
    bool isSharing() const override;
    bool isValid() const override;

    NSOpenGLContext *nativeContext() const;

    QFunctionPointer getProcAddress(const char *procName) override;

private:
    static NSOpenGLPixelFormat *pixelFormatForSurfaceFormat(const QSurfaceFormat &format);

    bool setDrawable(QPlatformSurface *surface);
    void prepareDrawable(QCocoaWindow *platformWindow);
    void updateSurfaceFormat();

    NSOpenGLContext *m_context = nil;
    NSOpenGLContext *m_shareContext = nil;
    QSurfaceFormat m_format;
    QVarLengthArray<QMacNotificationObserver, 3> m_updateObservers;
    QAtomicInt m_needsUpdate = false;

#ifndef QT_NO_DEBUG_STREAM
    friend QDebug operator<<(QDebug debug, const QCocoaGLContext *screen);
#endif
};

QT_END_NAMESPACE

#endif // QCOCOAGLCONTEXT_H
