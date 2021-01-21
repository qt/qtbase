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

#ifndef QPLATFORMHEADERHELPER_H
#define QPLATFORMHEADERHELPER_H

#include <QtCore/QByteArray>
#include <QtGui/QGuiApplication>

#if 0
#pragma qt_class(QPlatformHeaderHelper)
#endif

QT_BEGIN_NAMESPACE

namespace QPlatformHeaderHelper {

template<typename ReturnT, typename FunctionT>
ReturnT callPlatformFunction(const QByteArray &functionName)
{
    FunctionT func = reinterpret_cast<FunctionT>(QGuiApplication::platformFunction(functionName));
    return func ? func() : ReturnT();
}

template<typename ReturnT, typename FunctionT, typename Arg1>
ReturnT callPlatformFunction(const QByteArray &functionName, Arg1 a1)
{
    FunctionT func = reinterpret_cast<FunctionT>(QGuiApplication::platformFunction(functionName));
    return func ? func(a1) : ReturnT();
}

template<typename ReturnT, typename FunctionT, typename Arg1, typename Arg2>
ReturnT callPlatformFunction(const QByteArray &functionName, Arg1 a1, Arg2 a2)
{
    FunctionT func = reinterpret_cast<FunctionT>(QGuiApplication::platformFunction(functionName));
    return func ? func(a1, a2) : ReturnT();
}

template<typename ReturnT, typename FunctionT, typename Arg1, typename Arg2, typename Arg3>
ReturnT callPlatformFunction(const QByteArray &functionName, Arg1 a1, Arg2 a2, Arg3 a3)
{
    FunctionT func = reinterpret_cast<FunctionT>(QGuiApplication::platformFunction(functionName));
    return func ? func(a1, a2, a3) : ReturnT();
}

template<typename ReturnT, typename FunctionT, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
ReturnT callPlatformFunction(const QByteArray &functionName, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4)
{
    FunctionT func = reinterpret_cast<FunctionT>(QGuiApplication::platformFunction(functionName));
    return func ? func(a1, a2, a3, a4) : ReturnT();
}

}

QT_END_NAMESPACE

#endif  /*QPLATFORMHEADERHELPER_H*/
