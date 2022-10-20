/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowsnativeinterface.h"
#include "qwindowswindow.h"
#include "qwindowsscreen.h"
#include "qwindowscontext.h"
#include "qwindowscursor.h"
#include "qwindowsopenglcontext.h"
#include "qwindowsintegration.h"
#include "qwindowstheme.h"
#include "qwin10helpers.h"

#include <QtGui/qwindow.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qscreen.h>
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

enum ResourceType {
    RenderingContextType,
    HandleType,
    GlHandleType,
    GetDCType,
    ReleaseDCType,
    VkSurface
};

static int resourceType(const QByteArray &key)
{
    static const char *names[] = { // match ResourceType
        "renderingcontext",
        "handle",
        "glhandle",
        "getdc",
        "releasedc",
        "vkSurface"
    };
    const char ** const end = names + sizeof(names) / sizeof(names[0]);
    const char **result = std::find(names, end, key);
    if (result == end)
        result = std::find(names, end, key.toLower());
    return int(result - names);
}

void *QWindowsNativeInterface::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    if (!window || !window->handle()) {
        qWarning("%s: '%s' requested for null window or window without handle.", __FUNCTION__, resource.constData());
        return nullptr;
    }
    auto *bw = static_cast<QWindowsWindow *>(window->handle());
    int type = resourceType(resource);
    if (type == HandleType)
        return bw->handle();
    switch (window->surfaceType()) {
    case QWindow::RasterSurface:
    case QWindow::RasterGLSurface:
        if (type == GetDCType)
            return bw->getDC();
        if (type == ReleaseDCType) {
            bw->releaseDC();
            return nullptr;
        }
        break;
    case QWindow::VulkanSurface:
#if QT_CONFIG(vulkan)
        if (type == VkSurface)
            return bw->surface(nullptr, nullptr); // returns the address of the VkSurfaceKHR, not the value, as expected
#endif
        break;
    default:
        break;
    }
    qWarning("%s: Invalid key '%s' requested.", __FUNCTION__, resource.constData());
    return nullptr;
}

void *QWindowsNativeInterface::nativeResourceForScreen(const QByteArray &resource, QScreen *screen)
{
    if (!screen || !screen->handle()) {
        qWarning("%s: '%s' requested for null screen or screen without handle.", __FUNCTION__, resource.constData());
        return nullptr;
    }
    auto *bs = static_cast<QWindowsScreen *>(screen->handle());
    int type = resourceType(resource);
    if (type == HandleType)
        return bs->handle();

    qWarning("%s: Invalid key '%s' requested.", __FUNCTION__, resource.constData());
    return nullptr;
}

#ifndef QT_NO_CURSOR
void *QWindowsNativeInterface::nativeResourceForCursor(const QByteArray &resource, const QCursor &cursor)
{
    if (resource == QByteArrayLiteral("hcursor")) {
        if (const QScreen *primaryScreen = QGuiApplication::primaryScreen()) {
            if (const QPlatformCursor *pCursor= primaryScreen->handle()->cursor())
                return static_cast<const QWindowsCursor *>(pCursor)->hCursor(cursor);
        }
    }
    return nullptr;
}
#endif // !QT_NO_CURSOR

void *QWindowsNativeInterface::nativeResourceForIntegration(const QByteArray &resource)
{
#ifdef QT_NO_OPENGL
    Q_UNUSED(resource);
#else
    if (resourceType(resource) == GlHandleType) {
        if (const QWindowsStaticOpenGLContext *sc = QWindowsIntegration::staticOpenGLContext())
            return sc->moduleHandle();
    }
#endif

    return nullptr;
}

#ifndef QT_NO_OPENGL
void *QWindowsNativeInterface::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    if (!context || !context->handle()) {
        qWarning("%s: '%s' requested for null context or context without handle.", __FUNCTION__, resource.constData());
        return nullptr;
    }

    qWarning("%s: Invalid key '%s' requested.", __FUNCTION__, resource.constData());
    return nullptr;
}
#endif // !QT_NO_OPENGL

QT_END_NAMESPACE
