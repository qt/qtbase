/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qxcbnativeinterface.h"

#include "qxcbscreen.h"

#include <private/qguiapplication_p.h>
#include <QtCore/QMap>

#include <QtCore/QDebug>

#include <QtGui/qopenglcontext.h>
#include <QtGui/qscreen.h>

#if defined(XCB_USE_EGL)
#include "QtPlatformSupport/private/qeglplatformcontext_p.h"
#elif defined (XCB_USE_DRI2)
#include "qdri2context.h"
#endif

QT_BEGIN_NAMESPACE

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

QXcbNativeInterface::QXcbNativeInterface() :
    m_genericEventFilterType(QByteArrayLiteral("xcb_generic_event_t"))

{
}

void *QXcbNativeInterface::nativeResourceForContext(const QByteArray &resourceString, QOpenGLContext *context)
{
    QByteArray lowerCaseResource = resourceString.toLower();
    if (!qXcbResourceMap()->contains(lowerCaseResource))
        return 0;

    ResourceType resource = qXcbResourceMap()->value(lowerCaseResource);
    void *result = 0;
    switch(resource) {
    case EglContext:
        result = eglContextForContext(context);
        break;
    default:
        break;
    }

    return result;
}

void *QXcbNativeInterface::nativeResourceForWindow(const QByteArray &resourceString, QWindow *window)
{
    QByteArray lowerCaseResource = resourceString.toLower();
    if (!qXcbResourceMap()->contains(lowerCaseResource))
        return 0;

    ResourceType resource = qXcbResourceMap()->value(lowerCaseResource);
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
        result = qPlatformScreenForWindow(window);
        break;
    case GraphicsDevice:
        result = graphicsDeviceForWindow(window);
        break;
    default:
        break;
    }

    return result;
}

QPlatformNativeInterface::NativeResourceForContextFunction QXcbNativeInterface::nativeResourceFunctionForContext(const QByteArray &resource)
{
    QByteArray lowerCaseResource = resource.toLower();
    if (lowerCaseResource == "get_egl_context") {
        return eglContextForContext;
    }
    return 0;
}

QXcbScreen *QXcbNativeInterface::qPlatformScreenForWindow(QWindow *window)
{
    QXcbScreen *screen;
    if (window) {
        screen = static_cast<QXcbScreen *>(window->screen()->handle());
    } else {
        screen = static_cast<QXcbScreen *>(QGuiApplication::primaryScreen()->handle());
    }
    return screen;
}

void *QXcbNativeInterface::displayForWindow(QWindow *window)
{
#if defined(XCB_USE_XLIB)
    QXcbScreen *screen = qPlatformScreenForWindow(window);
    return screen->connection()->xlib_display();
#else
    Q_UNUSED(window);
    return 0;
#endif
}

void *QXcbNativeInterface::eglDisplayForWindow(QWindow *window)
{
#if defined(XCB_USE_DRI2) || defined(XCB_USE_EGL)
    QXcbScreen *screen = qPlatformScreenForWindow(window);
    return screen->connection()->egl_display();
#else
    Q_UNUSED(window)
    return 0;
#endif
}

void *QXcbNativeInterface::connectionForWindow(QWindow *window)
{
    QXcbScreen *screen = qPlatformScreenForWindow(window);
    return screen->xcb_connection();
}

void *QXcbNativeInterface::screenForWindow(QWindow *window)
{
    QXcbScreen *screen = qPlatformScreenForWindow(window);
    return screen->screen();
}

void *QXcbNativeInterface::graphicsDeviceForWindow(QWindow *window)
{
#if defined(XCB_USE_DRI2)
    QXcbScreen *screen = qPlatformScreenForWindow(window);
    QByteArray deviceName = screen->connection()->dri2DeviceName();
    return deviceName.data();
#else
    Q_UNUSED(window);
    return 0;
#endif

}

void * QXcbNativeInterface::eglContextForContext(QOpenGLContext *context)
{
    Q_ASSERT(context);
#if defined(XCB_USE_EGL)
    QEGLPlatformContext *eglPlatformContext = static_cast<QEGLPlatformContext *>(context->handle());
    return eglPlatformContext->eglContext();
#endif
#if 0
    Q_ASSERT(window);
    QPlatformOpenGLContext *platformContext = window->glContext()->handle();
    if (!platformContext) {
        qDebug() << "QWindow" << window << "does not have a glContext"
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
#else
    Q_UNUSED(context)
    return 0;
#endif
}

QT_END_NAMESPACE
