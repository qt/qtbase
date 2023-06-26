// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsapplication.h"
#include "qwindowsclipboard.h"
#include "qwindowscontext.h"
#include "qwindowsmimeregistry.h"
#include "qwin10helpers.h"
#include "qwindowsopengltester.h"
#include "qwindowswindow.h"
#include "qwindowsintegration.h"
#include "qwindowstheme.h"

#include <QtCore/qvariant.h>
#include <QtCore/private/qfunctions_win_p.h>

#include <QtGui/qpalette.h>

QT_BEGIN_NAMESPACE

void QWindowsApplication::setTouchWindowTouchType(QWindowsApplication::TouchWindowTouchTypes type)
{
    if (m_touchWindowTouchTypes == type)
        return;
    m_touchWindowTouchTypes = type;
    if (auto ctx = QWindowsContext::instance())
        ctx->registerTouchWindows();
}

QWindowsApplication::TouchWindowTouchTypes QWindowsApplication::touchWindowTouchType() const
{
    return m_touchWindowTouchTypes;
}

QWindowsApplication::WindowActivationBehavior QWindowsApplication::windowActivationBehavior() const
{
   return m_windowActivationBehavior;
}

void QWindowsApplication::setWindowActivationBehavior(WindowActivationBehavior behavior)
{
    m_windowActivationBehavior = behavior;
}

void QWindowsApplication::setHasBorderInFullScreenDefault(bool border)
{
    QWindowsWindow::setHasBorderInFullScreenDefault(border);
}

bool QWindowsApplication::isTabletMode() const
{
#if QT_CONFIG(clipboard)
    if (const QWindowsClipboard *clipboard = QWindowsClipboard::instance())
        return qt_windowsIsTabletMode(clipboard->clipboardViewer());
#endif
    return false;
}

bool QWindowsApplication::isWinTabEnabled() const
{
    auto ctx = QWindowsContext::instance();
    return ctx != nullptr && ctx->tabletSupport() != nullptr;
}

bool QWindowsApplication::setWinTabEnabled(bool enabled)
{
    if (enabled == isWinTabEnabled())
        return true;
    auto ctx = QWindowsContext::instance();
    if (!ctx)
        return false;
    return enabled ? ctx->initTablet() : ctx->disposeTablet();
}

bool QWindowsApplication::isDarkMode() const
{
    return QWindowsContext::isDarkMode();
}

QWindowsApplication::DarkModeHandling QWindowsApplication::darkModeHandling() const
{
    return m_darkModeHandling;
}

void QWindowsApplication::setDarkModeHandling(QWindowsApplication::DarkModeHandling handling)
{
    m_darkModeHandling = handling;
}

void QWindowsApplication::registerMime(QWindowsMimeConverter *mime)
{
    if (auto ctx = QWindowsContext::instance())
        ctx->mimeConverter().registerMime(mime);
}

void QWindowsApplication::unregisterMime(QWindowsMimeConverter *mime)
{
    if (auto ctx = QWindowsContext::instance())
        ctx->mimeConverter().unregisterMime(mime);
}

int QWindowsApplication::registerMimeType(const QString &mime)
{
    return QWindowsMimeRegistry::registerMimeType(mime);
}

HWND QWindowsApplication::createMessageWindow(const QString &classNameTemplate,
                                              const QString &windowName,
                                              QFunctionPointer eventProc) const
{
    QWindowsContext *ctx = QWindowsContext::instance();
    if (!ctx)
        return nullptr;
    auto wndProc = eventProc ? reinterpret_cast<WNDPROC>(eventProc) : DefWindowProc;
    return ctx->createDummyWindow(classNameTemplate,
                                  reinterpret_cast<const wchar_t*>(windowName.utf16()),
                                  wndProc);
}

bool QWindowsApplication::asyncExpose() const
{
    QWindowsContext *ctx = QWindowsContext::instance();
    return ctx && ctx->asyncExpose();
}

void QWindowsApplication::setAsyncExpose(bool value)
{
    if (QWindowsContext *ctx = QWindowsContext::instance())
        ctx->setAsyncExpose(value);
}

QVariant QWindowsApplication::gpu() const
{
    return GpuDescription::detect().toVariant();
}

QVariant QWindowsApplication::gpuList() const
{
    QVariantList result;
    const auto gpus = GpuDescription::detectAll();
    for (const auto &gpu : gpus)
        result.append(gpu.toVariant());
    return result;
}

void QWindowsApplication::populateLightSystemPalette(QPalette &result) const
{
    result = QWindowsTheme::systemPalette(Qt::ColorScheme::Light);
}

QT_END_NAMESPACE
