// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWINDOW_PLATFORM_H
#define QWINDOW_PLATFORM_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the native interface APIs. Usage of
// this API may make your code source and binary incompatible
// with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/qnativeinterface.h>
#include <QtGui/qwindow.h>

#if defined(Q_OS_HARMONY) || defined(Q_QDOC)
#include <QtCore/qharmonyjsobjectref.h>
#endif

QT_BEGIN_NAMESPACE

namespace QNativeInterface {

#if defined(Q_OS_HARMONY) || defined(Q_QDOC)
struct Q_GUI_EXPORT QHarmonyWindow
{
    QT_DECLARE_NATIVE_INTERFACE(QHarmonyWindow, 1, QWindow)
    virtual QHarmonyJsObjectRef ability() const = 0;
    virtual QHarmonyJsObjectRef jsWindow() const = 0;
};
#endif

} // QNativeInterface

QT_END_NAMESPACE

#endif // QWINDOW_PLATFORM_H
