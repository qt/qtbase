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

#include "qxcbnativeinterface.h"

#include "qxcbscreen.h"

#include <QtGui/private/qapplication_p.h>
#include <QtCore/QMap>

#include <QtCore/QDebug>

#if defined(XCB_USE_EGL)
#include "../eglconvenience/qeglplatformcontext.h"
#elif defined (XCB_USE_DRI2)
#include "qdri2context.h"
#endif

class QXcbResourceMap : public QMap<QByteArray, QXcbNativeInterface::ResourceType>
{
public:
    QXcbResourceMap()
        :QMap<QByteArray, QXcbNativeInterface::ResourceType>()
    {
        insert("display",QXcbNativeInterface::Display);
        insert("egldisplay",QXcbNativeInterface::EglDisplay);
        insert("connection",QXcbNativeInterface::Connection);
        insert("screen",QXcbNativeInterface::Screen);
        insert("graphicsdevice",QXcbNativeInterface::GraphicsDevice);
        insert("eglcontext",QXcbNativeInterface::EglContext);
    }
};

Q_GLOBAL_STATIC(QXcbResourceMap, qXcbResourceMap)

void *QXcbNativeInterface::nativeResourceForWidget(const QByteArray &resourceString, QWidget *widget)
{
    QByteArray lowerCaseResource = resourceString.toLower();
    ResourceType resource = qXcbResourceMap()->value(lowerCaseResource);
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
        result = qPlatformScreenForWidget(widget);
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

QXcbScreen *QXcbNativeInterface::qPlatformScreenForWidget(QWidget *widget)
{
    QXcbScreen *screen;
    if (widget) {
        screen = static_cast<QXcbScreen *>(QPlatformScreen::platformScreenForWidget(widget));
    }else {
        screen = static_cast<QXcbScreen *>(QApplicationPrivate::platformIntegration()->screens()[0]);
    }
    return screen;
}

void *QXcbNativeInterface::displayForWidget(QWidget *widget)
{
#if defined(XCB_USE_XLIB)
    QXcbScreen *screen = qPlatformScreenForWidget(widget);
    return screen->connection()->xlib_display();
#else
    Q_UNUSED(widget);
    return 0;
#endif
}

void *QXcbNativeInterface::eglDisplayForWidget(QWidget *widget)
{
#if defined(XCB_USE_DRI2) || defined(XCB_USE_EGL)
    QXcbScreen *screen = qPlatformScreenForWidget(widget);
    return screen->connection()->egl_display();
#else
    Q_UNUSED(widget)
    return 0;
#endif
}

void *QXcbNativeInterface::connectionForWidget(QWidget *widget)
{
    QXcbScreen *screen = qPlatformScreenForWidget(widget);
    return screen->xcb_connection();
}

void *QXcbNativeInterface::screenForWidget(QWidget *widget)
{
    QXcbScreen *screen = qPlatformScreenForWidget(widget);
    return screen->screen();
}

void *QXcbNativeInterface::graphicsDeviceForWidget(QWidget *widget)
{
#if defined(XCB_USE_DRI2)
    QXcbScreen *screen = qPlatformScreenForWidget(widget);
    QByteArray deviceName = screen->connection()->dri2DeviceName();
    return deviceName.data();
#else
    Q_UNUSED(widget);
    return 0;
#endif

}

void * QXcbNativeInterface::eglContextForWidget(QWidget *widget)
{
    Q_ASSERT(widget);
    if (!widget->platformWindow()) {
        qDebug() << "QPlatformWindow does not exist for widget" << widget
                 << "cannot return EGLContext";
        return 0;
    }
    QPlatformGLContext *platformContext = widget->platformWindow()->glContext();
    if (!platformContext) {
        qDebug() << "QPlatformWindow" << widget->platformWindow() << "does not have a glContext"
                 << "cannot return EGLContext";
        return 0;
    }
#if defined(XCB_USE_EGL)
    QEGLPlatformContext *eglPlatformContext = static_cast<QEGLPlatformContext *>(platformContext);
    return eglPlatformContext->eglContext();
#elif defined (XCB_USE_DRI2)
    QDri2Context *dri2Context = static_cast<QDri2Context *>(platformContext);
    return dri2Context->eglContext();
#else
    return 0;
#endif
}
