/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
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
