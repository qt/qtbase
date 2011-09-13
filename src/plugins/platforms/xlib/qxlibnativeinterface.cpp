/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <private/qguiapplication_p.h>
#include "qxlibnativeinterface.h"

#include "qxlibdisplay.h"
#include "qscreen.h"

class QXlibResourceMap : public QMap<QByteArray, QXlibNativeInterface::ResourceType>
{
public:
    QXlibResourceMap()
        :QMap<QByteArray, QXlibNativeInterface::ResourceType>()
    {
        insert("display",QXlibNativeInterface::Display);
        insert("egldisplay",QXlibNativeInterface::EglDisplay);
        insert("connection",QXlibNativeInterface::Connection);
        insert("screen",QXlibNativeInterface::Screen);
        insert("graphicsdevice",QXlibNativeInterface::GraphicsDevice);
        insert("eglcontext",QXlibNativeInterface::EglContext);
    }
};

Q_GLOBAL_STATIC(QXlibResourceMap, qXlibResourceMap)


void * QXlibNativeInterface::nativeResourceForWindow(const QByteArray &resourceString, QWindow *window)
{
    QByteArray lowerCaseResource = resourceString.toLower();
    ResourceType resource = qXlibResourceMap()->value(lowerCaseResource);
    void *result = 0;
    switch(resource) {
    case Display:
        result = displayForWindow(window);
        break;
    case EglDisplay:
        result = eglDisplayForWindow(window);
        break;
    case Connection:
        result = connectionForWindow(window);
        break;
    case Screen:
        result = reinterpret_cast<void *>(qPlatformScreenForWindow(window)->xScreenNumber());
        break;
    case GraphicsDevice:
        result = graphicsDeviceForWindow(window);
        break;
    case EglContext:
        result = eglContextForWindow(window);
        break;
    default:
        result = 0;
    }
    return result;
}

void * QXlibNativeInterface::displayForWindow(QWindow *window)
{
    return qPlatformScreenForWindow(window)->display()->nativeDisplay();
}

void * QXlibNativeInterface::eglDisplayForWindow(QWindow *window)
{
    Q_UNUSED(window);
    return 0;
}

void * QXlibNativeInterface::screenForWindow(QWindow *window)
{
    Q_UNUSED(window);
    return 0;
}

void * QXlibNativeInterface::graphicsDeviceForWindow(QWindow *window)
{
    Q_UNUSED(window);
    return 0;
}

void * QXlibNativeInterface::eglContextForWindow(QWindow *window)
{
    Q_UNUSED(window);
    return 0;
}

QXlibScreen * QXlibNativeInterface::qPlatformScreenForWindow(QWindow *window)
{
    QScreen *screen = window ? window->screen() : QGuiApplication::primaryScreen();
    return static_cast<QXlibScreen *>(screen->handle());
}
