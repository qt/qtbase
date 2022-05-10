// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
