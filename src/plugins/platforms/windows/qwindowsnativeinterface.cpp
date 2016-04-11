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
#include "qwindowscontext.h"
#include "qwindowscursor.h"
#include "qwindowsfontdatabase.h"
#include "qwindowsopenglcontext.h"
#include "qwindowsopengltester.h"
#include "qwindowsintegration.h"
#include "qwindowsmime.h"

#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

enum ResourceType {
    RenderingContextType,
    EglContextType,
    EglDisplayType,
    EglConfigType,
    HandleType,
    GlHandleType,
    GetDCType,
    ReleaseDCType
};

static int resourceType(const QByteArray &key)
{
    static const char *names[] = { // match ResourceType
        "renderingcontext",
        "eglcontext",
        "egldisplay",
        "eglconfig",
        "handle",
        "glhandle",
        "getdc",
        "releasedc"
    };
    const char ** const end = names + sizeof(names) / sizeof(names[0]);
    const char **result = std::find(names, end, key);
    if (result == end)
        result = std::find(names, end, key.toLower());
    return int(result - names);
}

QWindowsWindowFunctions::WindowActivationBehavior QWindowsNativeInterface::m_windowActivationBehavior =
    QWindowsWindowFunctions::DefaultActivateWindow;

void *QWindowsNativeInterface::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    if (!window || !window->handle()) {
        qWarning("%s: '%s' requested for null window or window without handle.", __FUNCTION__, resource.constData());
        return 0;
    }
    QWindowsWindow *bw = static_cast<QWindowsWindow *>(window->handle());
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
            return 0;
        }
        break;
    case QWindow::OpenGLSurface:
        break;
    }
    qWarning("%s: Invalid key '%s' requested.", __FUNCTION__, resource.constData());
    return 0;
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
    return Q_NULLPTR;
}
#endif // !QT_NO_CURSOR

static const char customMarginPropertyC[] = "WindowsCustomMargins";

QVariant QWindowsNativeInterface::windowProperty(QPlatformWindow *window, const QString &name) const
{
    QWindowsWindow *platformWindow = static_cast<QWindowsWindow *>(window);
    if (name == QLatin1String(customMarginPropertyC))
        return qVariantFromValue(platformWindow->customMargins());
    return QVariant();
}

QVariant QWindowsNativeInterface::windowProperty(QPlatformWindow *window, const QString &name, const QVariant &defaultValue) const
{
    const QVariant result = windowProperty(window, name);
    return result.isValid() ? result : defaultValue;
}

void QWindowsNativeInterface::setWindowProperty(QPlatformWindow *window, const QString &name, const QVariant &value)
{
    QWindowsWindow *platformWindow = static_cast<QWindowsWindow *>(window);
    if (name == QLatin1String(customMarginPropertyC))
        platformWindow->setCustomMargins(qvariant_cast<QMargins>(value));
}

QVariantMap QWindowsNativeInterface::windowProperties(QPlatformWindow *window) const
{
    QVariantMap result;
    const QString customMarginProperty = QLatin1String(customMarginPropertyC);
    result.insert(customMarginProperty, windowProperty(window, customMarginProperty));
    return result;
}

void *QWindowsNativeInterface::nativeResourceForIntegration(const QByteArray &resource)
{
#ifdef QT_NO_OPENGL
    Q_UNUSED(resource)
#else
    if (resourceType(resource) == GlHandleType) {
        if (const QWindowsStaticOpenGLContext *sc = QWindowsIntegration::staticOpenGLContext())
            return sc->moduleHandle();
    }
#endif

    return 0;
}

#ifndef QT_NO_OPENGL
void *QWindowsNativeInterface::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    if (!context || !context->handle()) {
        qWarning("%s: '%s' requested for null context or context without handle.", __FUNCTION__, resource.constData());
        return 0;
    }

    QWindowsOpenGLContext *glcontext = static_cast<QWindowsOpenGLContext *>(context->handle());
    switch (resourceType(resource)) {
    case RenderingContextType: // Fall through.
    case EglContextType:
        return glcontext->nativeContext();
    case EglDisplayType:
        return glcontext->nativeDisplay();
    case EglConfigType:
        return glcontext->nativeConfig();
    default:
        break;
    }

    qWarning("%s: Invalid key '%s' requested.", __FUNCTION__, resource.constData());
    return 0;
}
#endif // !QT_NO_OPENGL

/*!
    \brief Creates a non-visible window handle for filtering messages.
*/

void *QWindowsNativeInterface::createMessageWindow(const QString &classNameTemplate,
                                                   const QString &windowName,
                                                   void *eventProc) const
{
    QWindowsContext *ctx = QWindowsContext::instance();
    const HWND hwnd = ctx->createDummyWindow(classNameTemplate,
                                             (wchar_t*)windowName.utf16(),
                                             (WNDPROC)eventProc);
    return hwnd;
}

/*!
    \brief Registers a unique window class with a callback function based on \a classNameIn.
*/

QString QWindowsNativeInterface::registerWindowClass(const QString &classNameIn, void *eventProc) const
{
    return QWindowsContext::instance()->registerWindowClass(classNameIn, (WNDPROC)eventProc);
}

bool QWindowsNativeInterface::asyncExpose() const
{
    return QWindowsContext::instance()->asyncExpose();
}

void QWindowsNativeInterface::setAsyncExpose(bool value)
{
    QWindowsContext::instance()->setAsyncExpose(value);
}

void QWindowsNativeInterface::registerWindowsMime(void *mimeIn)
{
    QWindowsContext::instance()->mimeConverter().registerMime(reinterpret_cast<QWindowsMime *>(mimeIn));
}

void QWindowsNativeInterface::unregisterWindowsMime(void *mimeIn)
{
    QWindowsContext::instance()->mimeConverter().unregisterMime(reinterpret_cast<QWindowsMime *>(mimeIn));
}

int QWindowsNativeInterface::registerMimeType(const QString &mimeType)
{
    return QWindowsMime::registerMimeType(mimeType);
}

QFont QWindowsNativeInterface::logFontToQFont(const void *logFont, int verticalDpi)
{
    return QWindowsFontDatabase::LOGFONT_to_QFont(*reinterpret_cast<const LOGFONT *>(logFont), verticalDpi);
}

QFunctionPointer QWindowsNativeInterface::platformFunction(const QByteArray &function) const
{
    if (function == QWindowsWindowFunctions::setTouchWindowTouchTypeIdentifier())
        return QFunctionPointer(QWindowsWindow::setTouchWindowTouchTypeStatic);
    else if (function == QWindowsWindowFunctions::setHasBorderInFullScreenIdentifier())
        return QFunctionPointer(QWindowsWindow::setHasBorderInFullScreenStatic);
    else if (function == QWindowsWindowFunctions::setWindowActivationBehaviorIdentifier())
        return QFunctionPointer(QWindowsNativeInterface::setWindowActivationBehavior);
    return Q_NULLPTR;
}

QVariant QWindowsNativeInterface::gpu() const
{
    return GpuDescription::detect().toVariant();
}

QT_END_NAMESPACE
