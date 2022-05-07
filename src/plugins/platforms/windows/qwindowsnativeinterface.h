/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QWINDOWSNATIVEINTERFACE_H
#define QWINDOWSNATIVEINTERFACE_H

#include <QtGui/qfont.h>
#include <QtGui/qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsNativeInterface
    \brief Provides access to native handles.

    Currently implemented keys
    \list
    \li handle (HWND)
    \li getDC (DC)
    \li releaseDC Releases the previously acquired DC and returns 0.
    \endlist

    \internal
*/

class QWindowsNativeInterface : public QPlatformNativeInterface
{
    Q_OBJECT

public:
    void *nativeResourceForIntegration(const QByteArray &resource) override;
#ifndef QT_NO_OPENGL
    void *nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context) override;
#endif
    void *nativeResourceForWindow(const QByteArray &resource, QWindow *window) override;
    void *nativeResourceForScreen(const QByteArray &resource, QScreen *screen) override;
#ifndef QT_NO_CURSOR
    void *nativeResourceForCursor(const QByteArray &resource, const QCursor &cursor) override;
#endif
};

QT_END_NAMESPACE

#endif // QWINDOWSNATIVEINTERFACE_H
