// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSVULKANSURFACE_H
#define QOHOSVULKANSURFACE_H

#include <QtCore/qglobal.h>
#include <native_window/external_window.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_ohos.h>
#include <optional>

QT_BEGIN_NAMESPACE

class QOhosVulkanSurface
{
    Q_DISABLE_COPY_MOVE(QOhosVulkanSurface)
public:
    QOhosVulkanSurface() = default;

    void setNativeWindowSurface(::OHNativeWindow *nativeWindow);
    VkSurfaceKHR *tryGetOrCreateVulkanWindowSurface(VkInstance instance,
                                                    PFN_vkCreateSurfaceOHOS createFn,
                                                    PFN_vkDestroySurfaceKHR destroyFn);

private:
    struct VulkanSurface {
        Q_DISABLE_COPY_MOVE(VulkanSurface)
        VkInstance instance;
        VkSurfaceKHR surface;
        PFN_vkDestroySurfaceKHR destroyFunc;
        VulkanSurface(VkInstance inst, VkSurfaceKHR surf, PFN_vkDestroySurfaceKHR fn)
            : instance(inst), surface(surf), destroyFunc(fn)
        {
        }
        ~VulkanSurface() {
            if (destroyFunc)
                destroyFunc(instance, surface, nullptr);
        }
    };

    ::OHNativeWindow *m_nativeWindow = nullptr;
    ::OHNativeWindow *m_surfaceNativeWindow = nullptr;
    std::optional<VulkanSurface> m_vulkanSurface = std::nullopt;
};

QT_END_NAMESPACE

#endif // QOHOSVULKANSURFACE_H
