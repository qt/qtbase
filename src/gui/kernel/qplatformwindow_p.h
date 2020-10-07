/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
    QT_DECLARE_NATIVE_INTERFACE(QCocoaWindow)
    virtual void setContentBorderEnabled(bool enable) = 0;
    virtual QPoint bottomLeftClippedByNSWindowOffset() const = 0;
};
#endif

#if QT_CONFIG(xcb) || defined(Q_CLANG_QDOC)
struct Q_GUI_EXPORT QXcbWindow
{
    QT_DECLARE_NATIVE_INTERFACE(QXcbWindow)

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
    QT_DECLARE_NATIVE_INTERFACE(QWindowsWindow)

    virtual void setHasBorderInFullScreen(bool border) = 0;
    virtual bool hasBorderInFullScreen() const = 0;

    virtual QMargins customMargins() const = 0;
    virtual void setCustomMargins(const QMargins &margins) = 0;
};
#endif // Q_OS_WIN

} // QNativeInterface::Private

QT_END_NAMESPACE

#endif // QPLATFORMWINDOW_P_H
