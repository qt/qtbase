// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef NATIVEWINDOW_H
#define NATIVEWINDOW_H

#if defined(HAVE_NATIVE_WINDOW)

#include <qglobal.h>
#include <QtCore/qrect.h>
#include <QtGui/qwindowdefs.h>

#if defined(Q_OS_MACOS)
Q_FORWARD_DECLARE_OBJC_CLASS(NSView);
#elif defined(Q_OS_IOS)
Q_FORWARD_DECLARE_OBJC_CLASS(UIView);
#elif defined(Q_OS_WIN)
#  include <QtCore/qt_windows.h>
#elif QT_CONFIG(xcb)
#  include <xcb/xcb.h>
#  include <xcb/xcb_icccm.h>
#elif defined(ANDROID)
#  include <QtCore/qjniobject.h>
#  include <QtCore/qjnitypes.h>
#  include <QtCore/qnativeinterface.h>
Q_DECLARE_JNI_CLASS(View, "android/view/View")
Q_DECLARE_JNI_CLASS(ViewParent, "android/view/ViewParent")
#endif

class NativeWindow
{
    Q_DISABLE_COPY(NativeWindow)
public:
#if defined(Q_OS_MACOS)
    using Handle = NSView*;
#elif defined(QT_PLATFORM_UIKIT)
    using Handle = UIView*;
#elif defined(Q_OS_WIN)
    using Handle = HWND;
#elif QT_CONFIG(xcb)
    using Handle = xcb_window_t;
#elif defined(ANDROID)
    using Handle = QtJniTypes::View;
#endif

    NativeWindow();
    ~NativeWindow();

    void setVisible(bool visible);

    operator WId() const;
    WId parentWinId() const;
    bool isParentOf(WId childWinId);
    void setParent(WId parent);

    void setGeometry(const QRect &rect);
    QRect geometry() const;

private:
    Handle m_handle = {};
};

#endif // HAVE_NATIVE_WINDOW

#endif // NATIVEWINDOW_H
