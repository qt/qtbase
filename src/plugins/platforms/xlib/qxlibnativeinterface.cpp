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

#include "qxlibnativeinterface.h"

#include "qxlibdisplay.h"
#include <QtGui/private/qapplication_p.h>

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


void * QXlibNativeInterface::nativeResourceForWidget(const QByteArray &resourceString, QWidget *widget)
{
    QByteArray lowerCaseResource = resourceString.toLower();
    ResourceType resource = qXlibResourceMap()->value(lowerCaseResource);
    void *result = 0;
    switch(resource) {
    case Display:
        result = displayForWidget(widget);
        break;
    case EglDisplay:
        result = eglDisplayForWidget(widget);
        break;
    case Connection:
        result = connectionForWidget(widget);
        break;
    case Screen:
        result = reinterpret_cast<void *>(qPlatformScreenForWidget(widget)->xScreenNumber());
        break;
    case GraphicsDevice:
        result = graphicsDeviceForWidget(widget);
        break;
    case EglContext:
        result = eglContextForWidget(widget);
        break;
    default:
        result = 0;
    }
    return result;
}

void * QXlibNativeInterface::displayForWidget(QWidget *widget)
{
    return qPlatformScreenForWidget(widget)->display()->nativeDisplay();
}

void * QXlibNativeInterface::eglDisplayForWidget(QWidget *widget)
{
    Q_UNUSED(widget);
    return 0;
}

void * QXlibNativeInterface::screenForWidget(QWidget *widget)
{
    Q_UNUSED(widget);
    return 0;
}

void * QXlibNativeInterface::graphicsDeviceForWidget(QWidget *widget)
{
    Q_UNUSED(widget);
    return 0;
}

void * QXlibNativeInterface::eglContextForWidget(QWidget *widget)
{
    Q_UNUSED(widget);
    return 0;
}

QXlibScreen * QXlibNativeInterface::qPlatformScreenForWidget(QWidget *widget)
{
    QXlibScreen *screen;
    if (widget) {
        screen = static_cast<QXlibScreen *>(QPlatformScreen::platformScreenForWidget(widget));
    }else {
        screen = static_cast<QXlibScreen *>(QApplicationPrivate::platformIntegration()->screens()[0]);
    }
    return screen;
}
