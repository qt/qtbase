// Copyright (C) 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxnativeinterface.h"

#if !defined(QT_NO_OPENGL)
#include "qqnxglcontext.h"
#endif

#include "qqnxscreen.h"
#include "qqnxwindow.h"
#if defined(QQNX_IMF)
#include "qqnxinputcontext_imf.h"
#endif

#include "qqnxintegration.h"

#if !defined(QT_NO_OPENGL)
#include <QtGui/QOpenGLContext>
#endif

#include <QtGui/QScreen>
#include <QtGui/QWindow>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QQnxNativeInterface::QQnxNativeInterface(QQnxIntegration *integration)
    : m_integration(integration)
{
}

void *QQnxNativeInterface::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    if (resource == "windowGroup" && window && window->screen()) {
        QQnxScreen * const screen = static_cast<QQnxScreen *>(window->screen()->handle());
        if (screen) {
            screen_window_t screenWindow = reinterpret_cast<screen_window_t>(window->winId());
            QQnxWindow *qnxWindow = screen->findWindow(screenWindow);
            // We can't just call data() instead of constData() here, since that would detach
            // and the lifetime of the char * would not be long enough. Therefore the const_cast.
            return qnxWindow ? const_cast<char *>(qnxWindow->groupName().constData()) : 0;
        }
    }

    return 0;
}

void *QQnxNativeInterface::nativeResourceForScreen(const QByteArray &resource, QScreen *screen)
{
    if (resource == "QObject*" && screen)
        return static_cast<QObject*>(static_cast<QQnxScreen*>(screen->handle()));

    return 0;
}

void *QQnxNativeInterface::nativeResourceForIntegration(const QByteArray &resource)
{
    if (resource == "screenContext")
        return m_integration->screenContext();

#if QT_CONFIG(opengl)
    if (resource.toLower() == "egldisplay")
        return m_integration->eglDisplay();
#endif

    return 0;
}

#if !defined(QT_NO_OPENGL)
void *QQnxNativeInterface::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    if (resource == "eglcontext" && context)
        return static_cast<QQnxGLContext*>(context->handle())->eglContext();

    return 0;
}
#endif

void QQnxNativeInterface::setWindowProperty(QPlatformWindow *window, const QString &name, const QVariant &value)
{
    QQnxWindow *qnxWindow = static_cast<QQnxWindow*>(window);

    if (name == "qnxWindowGroup"_L1) {
        if (value.isNull())
            qnxWindow->joinWindowGroup(QByteArray());
        else if (value.canConvert<QByteArray>())
            qnxWindow->joinWindowGroup(value.toByteArray());
    }
}

QPlatformNativeInterface::NativeResourceForIntegrationFunction QQnxNativeInterface::nativeResourceFunctionForIntegration(const QByteArray &resource)
{
#if defined(QQNX_IMF)
    if (resource == "blackberryIMFSetHighlightColor")
        return reinterpret_cast<NativeResourceForIntegrationFunction>(QQnxInputContext::setHighlightColor);
    if (resource == "blackberryIMFCheckSpelling")
        return reinterpret_cast<NativeResourceForIntegrationFunction>(QQnxInputContext::checkSpelling);
#else
    Q_UNUSED(resource);
#endif
    return 0;
}

QT_END_NAMESPACE
