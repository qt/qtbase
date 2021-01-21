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

#ifndef QWINDOWSWINDOWFUNCTIONS_H
#define QWINDOWSWINDOWFUNCTIONS_H

#include <QtCore/QByteArray>
#include <QtGui/QGuiApplication>

QT_BEGIN_NAMESPACE

class QWindow;

class QWindowsWindowFunctions {
public:
    enum TouchWindowTouchType {
        NormalTouch   = 0x00000000,
        FineTouch     = 0x00000001,
        WantPalmTouch = 0x00000002
    };

    Q_DECLARE_FLAGS(TouchWindowTouchTypes, TouchWindowTouchType)

    enum WindowActivationBehavior {
        DefaultActivateWindow,
        AlwaysActivateWindow
    };

    typedef void (*SetTouchWindowTouchType)(QWindow *window, QWindowsWindowFunctions::TouchWindowTouchTypes touchType);
    static const QByteArray setTouchWindowTouchTypeIdentifier() { return QByteArrayLiteral("WindowsSetTouchWindowTouchType"); }

    static void setTouchWindowTouchType(QWindow *window, TouchWindowTouchTypes type)
    {
        SetTouchWindowTouchType func = reinterpret_cast<SetTouchWindowTouchType>(QGuiApplication::platformFunction(setTouchWindowTouchTypeIdentifier()));
        if (func)
            func(window, type);
    }

    typedef void (*SetHasBorderInFullScreen)(QWindow *window, bool border);
    static const QByteArray setHasBorderInFullScreenIdentifier() { return QByteArrayLiteral("WindowsSetHasBorderInFullScreen"); }
    static void setHasBorderInFullScreen(QWindow *window, bool border)
    {
        SetHasBorderInFullScreen func = reinterpret_cast<SetHasBorderInFullScreen>(QGuiApplication::platformFunction(setHasBorderInFullScreenIdentifier()));
        if (func)
            func(window, border);
    }

    typedef void (*SetHasBorderInFullScreenDefault)(bool border);
    static const QByteArray setHasBorderInFullScreenDefaultIdentifier() { return QByteArrayLiteral("WindowsSetHasBorderInFullScreenDefault"); }
    static void setHasBorderInFullScreenDefault(bool border)
    {
        auto func = reinterpret_cast<SetHasBorderInFullScreenDefault>(QGuiApplication::platformFunction(setHasBorderInFullScreenDefaultIdentifier()));
        if (func)
            func(border);
    }

    typedef void (*SetWindowActivationBehaviorType)(WindowActivationBehavior);
    static const QByteArray setWindowActivationBehaviorIdentifier() { return QByteArrayLiteral("WindowsSetWindowActivationBehavior"); }

    static void setWindowActivationBehavior(WindowActivationBehavior behavior)
    {
        SetWindowActivationBehaviorType func = reinterpret_cast<SetWindowActivationBehaviorType>(QGuiApplication::platformFunction(setWindowActivationBehaviorIdentifier()));
        if (func)
            func(behavior);
    }

    typedef bool (*IsTabletModeType)();
    static const QByteArray isTabletModeIdentifier() { return QByteArrayLiteral("WindowsIsTabletMode"); }

    static bool isTabletMode()
    {
        IsTabletModeType func = reinterpret_cast<IsTabletModeType>(QGuiApplication::platformFunction(isTabletModeIdentifier()));
        return func && func();
    }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QWindowsWindowFunctions::TouchWindowTouchTypes)

QT_END_NAMESPACE

#endif // QWINDOWSWINDOWFUNCTIONS_H
