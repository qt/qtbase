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

#ifndef QLINUXFBFUNCTIONS_H
#define QLINUXFBFUNCTIONS_H

#include <QtCore/QByteArray>
#include <QtGui/QGuiApplication>

QT_BEGIN_NAMESPACE

class QLinuxFbFunctions
{
public:
    typedef void (*LoadKeymapType)(const QString &filename);
    typedef void (*SwitchLangType)();
    static QByteArray loadKeymapTypeIdentifier() { return QByteArrayLiteral("LinuxFbLoadKeymap"); }
    static QByteArray switchLangTypeIdentifier() { return QByteArrayLiteral("LinuxFbSwitchLang"); }

    static void loadKeymap(const QString &filename)
    {
        LoadKeymapType func = reinterpret_cast<LoadKeymapType>(QGuiApplication::platformFunction(loadKeymapTypeIdentifier()));
        if (func)
            func(filename);
    }

    static void switchLang()
    {
        SwitchLangType func = reinterpret_cast<SwitchLangType>(QGuiApplication::platformFunction(switchLangTypeIdentifier()));
        if (func)
            func();
    }
};


QT_END_NAMESPACE

#endif // QLINUXFBFUNCTIONS_H
