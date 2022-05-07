/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QOFFSCREENSURFACE_PLATFORM_H
#define QOFFSCREENSURFACE_PLATFORM_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qoffscreensurface.h>
#include <QtCore/qnativeinterface.h>

#if defined(Q_OS_ANDROID)
struct ANativeWindow;
#endif

QT_BEGIN_NAMESPACE

namespace QNativeInterface {

#if defined(Q_OS_ANDROID) || defined(Q_CLANG_QDOC)
struct Q_GUI_EXPORT QAndroidOffscreenSurface
{
    QT_DECLARE_NATIVE_INTERFACE(QAndroidOffscreenSurface, 1, QOffscreenSurface)
    static QOffscreenSurface *fromNative(ANativeWindow *nativeSurface);
    virtual ANativeWindow *nativeSurface() const = 0;
};
#endif

} // QNativeInterface

QT_END_NAMESPACE

#endif // QOFFSCREENSURFACE_PLATFORM_H
