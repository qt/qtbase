// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosvulkansurface.h"

#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

void QOhosVulkanSurface::setNativeWindowSurface(::OHNativeWindow *nativeWindow)
{
    m_nativeWindow = nativeWindow;
}

VkSurfaceKHR *QOhosVulkanSurface::tryGetOrCreateVulkanWindowSurface(VkInstance instance,
                                                                    PFN_vkCreateSurfaceOHOS createFn,
                                                                    PFN_vkDestroySurfaceKHR destroyFn)
{
    const bool surfaceValid = m_vulkanSurface.has_value() && m_surfaceNativeWindow == m_nativeWindow;
    if (surfaceValid)
        return &m_vulkanSurface->surface;

    // safe to destroy: the RHI guarantees all swapchains are destroyed before requesting a new surface
    m_vulkanSurface.reset();

    if (!createFn) {
        qCWarning(QtForOhos, "%s: vkCreateSurfaceOHOS not available", Q_FUNC_INFO);
        return nullptr;
    }

    VkSurfaceCreateInfoOHOS surfaceInfo{};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_SURFACE_CREATE_INFO_OHOS;
    surfaceInfo.pNext = nullptr;
    surfaceInfo.flags = 0;
    surfaceInfo.window = m_nativeWindow;

    VkSurfaceKHR vkSurface = VK_NULL_HANDLE;
    VkResult err = createFn(instance, &surfaceInfo, nullptr, &vkSurface);
    if (err != VK_SUCCESS) {
        qCWarning(QtForOhos, "%s: Failed to create OHOS VkSurface: %d", Q_FUNC_INFO, err);
        return nullptr;
    }

    m_vulkanSurface.emplace(instance, vkSurface, destroyFn);
    m_surfaceNativeWindow = m_nativeWindow;

    return &m_vulkanSurface->surface;
}

QT_END_NAMESPACE
