// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMWINDOW_P_H
#define QPLATFORMWINDOW_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/qbasictimer.h>
#include <QtCore/qrect.h>
#include <QtCore/qnativeinterface.h>

QT_BEGIN_NAMESPACE

class QMargins;

class QPlatformWindowPrivate
{
public:
    QRect rect;
    QBasicTimer updateTimer;
};

// ----------------- QNativeInterface -----------------

namespace QNativeInterface::Private {

#if defined(Q_OS_MACOS) || defined(Q_CLANG_QDOC)
struct Q_GUI_EXPORT QCocoaWindow
{
    QT_DECLARE_NATIVE_INTERFACE(QCocoaWindow, 1, QWindow)
    virtual void setContentBorderEnabled(bool enable) = 0;
    virtual QPoint bottomLeftClippedByNSWindowOffset() const = 0;
};
#endif

#if QT_CONFIG(xcb) || defined(Q_CLANG_QDOC)
struct Q_GUI_EXPORT QXcbWindow
{
    QT_DECLARE_NATIVE_INTERFACE(QXcbWindow, 1, QWindow)

    enum WindowType {
        None         = 0x000000,
        Normal       = 0x000001,
        Desktop      = 0x000002,
        Dock         = 0x000004,
        Toolbar      = 0x000008,
        Menu         = 0x000010,
        Utility      = 0x000020,
        Splash       = 0x000040,
        Dialog       = 0x000080,
        DropDownMenu = 0x000100,
        PopupMenu    = 0x000200,
        Tooltip      = 0x000400,
        Notification = 0x000800,
        Combo        = 0x001000,
        Dnd          = 0x002000,
        KdeOverride  = 0x004000
    };
    Q_DECLARE_FLAGS(WindowTypes, WindowType)

    virtual void setWindowType(WindowTypes type) = 0;
    virtual void setWindowRole(const QString &role) = 0;
    virtual void setWindowIconText(const QString &text) = 0;
    virtual uint visualId() const = 0;
};
#endif // xcb

#if defined(Q_OS_WIN) || defined(Q_CLANG_QDOC)
struct Q_GUI_EXPORT QWindowsWindow
{
    QT_DECLARE_NATIVE_INTERFACE(QWindowsWindow, 1, QWindow)

    virtual void setHasBorderInFullScreen(bool border) = 0;
    virtual bool hasBorderInFullScreen() const = 0;

    virtual QMargins customMargins() const = 0;
    virtual void setCustomMargins(const QMargins &margins) = 0;
};
#endif // Q_OS_WIN

} // QNativeInterface::Private

QT_END_NAMESPACE

#endif // QPLATFORMWINDOW_P_H
