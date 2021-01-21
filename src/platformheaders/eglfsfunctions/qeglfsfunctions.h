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

#ifndef QEGLFSFUNCTIONS_H
#define QEGLFSFUNCTIONS_H

#include <QtCore/QByteArray>
#include <QtGui/QGuiApplication>

QT_BEGIN_NAMESPACE

class QEglFSFunctions
{
public:
    typedef void (*LoadKeymapType)(const QString &filename);
    typedef void (*SwitchLangType)();
    static QByteArray loadKeymapTypeIdentifier() { return QByteArrayLiteral("EglFSLoadKeymap"); }
    static QByteArray switchLangTypeIdentifier() { return QByteArrayLiteral("EglFSSwitchLang"); }

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

    typedef int (*Vsp2AddLayerType)(const QScreen *screen, int dmabufFd, const QSize &size, const QPoint &position, uint drmPixelFormat, uint bytesPerLine);
    static QByteArray vsp2AddLayerTypeIdentifier() { return QByteArrayLiteral("EglFSVsp2AddLayer"); }

    //vsp2 functions are currently internal and preliminary (see qdoc file)
    static int vsp2AddLayer(const QScreen *screen, int dmabufFd, const QSize &size, const QPoint &position, uint drmPixelFormat, uint bytesPerLine)
    {
        auto func = reinterpret_cast<Vsp2AddLayerType>(QGuiApplication::platformFunction(vsp2AddLayerTypeIdentifier()));
        if (func)
            return func(screen, dmabufFd, size, position, drmPixelFormat, bytesPerLine);
        return 0;
    }

    typedef bool (*Vsp2RemoveLayerType)(const QScreen *screen, int id);
    static QByteArray vsp2RemoveLayerTypeIdentifier() { return QByteArrayLiteral("EglFSVsp2RemoveLayer"); }

    static bool vsp2RemoveLayer(const QScreen *screen, int id)
    {
        auto func = reinterpret_cast<Vsp2RemoveLayerType>(QGuiApplication::platformFunction(vsp2RemoveLayerTypeIdentifier()));
        return func && func(screen, id);
    }

    typedef void (*Vsp2SetLayerBufferType)(const QScreen *screen, int id, int dmabufFd);
    static QByteArray vsp2SetLayerBufferTypeIdentifier() { return QByteArrayLiteral("EglFSVsp2SetLayerBuffer"); }

    static void vsp2SetLayerBuffer(const QScreen *screen, int id, int dmabufFd)
    {
        auto func = reinterpret_cast<Vsp2SetLayerBufferType>(QGuiApplication::platformFunction(vsp2SetLayerBufferTypeIdentifier()));
        if (func)
            func(screen, id, dmabufFd);
    }

    typedef bool (*Vsp2SetLayerPositionType)(const QScreen *screen, int id, const QPoint &position);
    static QByteArray vsp2SetLayerPositionTypeIdentifier() { return QByteArrayLiteral("EglFSVsp2SetLayerPosition"); }

    static bool vsp2SetLayerPosition(const QScreen *screen, int id, const QPoint &position)
    {
        auto func = reinterpret_cast<Vsp2SetLayerPositionType>(QGuiApplication::platformFunction(vsp2SetLayerPositionTypeIdentifier()));
        return func && func(screen, id, position);
    }

    typedef bool (*Vsp2SetLayerAlphaType)(const QScreen *screen, int id, qreal alpha);
    static QByteArray vsp2SetLayerAlphaTypeIdentifier() { return QByteArrayLiteral("EglFSVsp2SetLayerAlpha"); }

    static bool vsp2SetLayerAlpha(const QScreen *screen, int id, qreal alpha)
    {
        auto func = reinterpret_cast<Vsp2SetLayerAlphaType>(QGuiApplication::platformFunction(vsp2SetLayerAlphaTypeIdentifier()));
        return func && func(screen, id, alpha);
    }

    typedef void (*Vsp2AddBlendListenerType)(const QScreen *screen, void(*callback)());
    static QByteArray vsp2AddBlendListenerTypeIdentifier() { return QByteArrayLiteral("EglFSVsp2AddBlendListener"); }

    static void vsp2AddBlendListener(const QScreen *screen, void(*callback)())
    {
        auto func = reinterpret_cast<Vsp2AddBlendListenerType>(QGuiApplication::platformFunction(vsp2AddBlendListenerTypeIdentifier()));
        if (func)
            func(screen, callback);
    }
};


QT_END_NAMESPACE

#endif // QEGLFSFUNCTIONS_H
