// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosplatformnativeinterface.h"
#include "qohosplatformclipboard.h"
#include "qohosplatformintegration.h"
#include "qohosplatformwindow.h"
#include <qohosjsenv_p.h>

#if QT_CONFIG(vulkan)
#include "render/qohossurface.h"
#include <QtGui/qvulkaninstance.h>
#endif

#include <QtCore/qmutex.h>
#include <QtGui/private/qguiapplication_p.h>
QT_BEGIN_NAMESPACE

void *QOhosPlatformNativeInterface::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
#if QT_CONFIG(vulkan)
    if (resource == "vkSurface") {
        if (window && window->surfaceType() == QSurface::VulkanSurface) {
            auto *platformWindow = static_cast<QOhosPlatformWindow *>(window->handle());
            QOhosSurface *surface = platformWindow ? platformWindow->ownedSurfaceOrNull() : nullptr;
            if (!surface) {
                qWarning("QOhosPlatformNativeInterface: No QOhosSurface available for Vulkan window");
                return nullptr;
            }
            QVulkanInstance *vulkanInstance = window->vulkanInstance();
            if (!vulkanInstance) {
                qWarning("QOhosPlatformNativeInterface: No Vulkan instance; was QWindow::setVulkanInstance() called?");
                return nullptr;
            }
            return surface->tryGetOrCreateVulkanWindowSurface(vulkanInstance);
        }
    }
#else
    Q_UNUSED(resource);
    Q_UNUSED(window);
#endif
    return nullptr;
}

void QOhosPlatformNativeInterface::customEvent(QEvent *event)
{
    auto __dbg = make_QCScopedDebug("QOhosPlatformNativeInterface::customEvent");
    if (event->type() != QEvent::User)
        return;
}

QFunctionPointer QOhosPlatformNativeInterface::platformFunction(const QByteArray &functionName) const
{
    if (functionName == "tagWindowOrWidgetAsSubWindowOf") {
        return reinterpret_cast<QFunctionPointer>(&QOhosPlatformWindow::tagWindowOrWidgetAsSubWindowOf);
    } else if (functionName == "getWindowOrWidgetAsSubWindowOfTagValue") {
        return reinterpret_cast<QFunctionPointer>(&QOhosPlatformWindow::getWindowOrWidgetAsSubWindowOfTagValue);
    } else if (functionName == "tagWindowOrWidgetAsMainWindow") {
        return reinterpret_cast<QFunctionPointer>(&QOhosPlatformWindow::tagWindowOrWidgetAsMainWindow);
    } else if (functionName == "setInAppOnlyPasteboardShareOption") {
        return reinterpret_cast<QFunctionPointer>(
            &QOhosPlatformClipboard::setInAppOnlyPasteboardShareOption);
    } else if (functionName == "setSurfaceConsumer") {
        return reinterpret_cast<QFunctionPointer>(&QOhosPlatformWindow::setSurfaceConsumer);
    }

    return QPlatformNativeInterface::platformFunction(functionName);
}

QT_END_NAMESPACE
