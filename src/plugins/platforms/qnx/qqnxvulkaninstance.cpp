// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqnxvulkaninstance.h"
#include "qqnxwindow.h"
#include "qqnxintegration.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaQnxVulkan, "qt.qpa.qnx.vulkan")

QQnxVulkanInstance::QQnxVulkanInstance(QVulkanInstance *instance)
    : m_instance(instance),
      m_createSurface(nullptr),
      m_getPhysDevScreenPresSupport(nullptr)
{
    loadVulkanLibrary(QStringLiteral("vulkan"), 1);
}

QQnxVulkanInstance::~QQnxVulkanInstance()
{
}

void QQnxVulkanInstance::createOrAdoptInstance()
{
    initInstance(m_instance, QByteArrayList() << QByteArrayLiteral("VK_QNX_screen_surface"));

    if (!m_vkInst)
        return;

    m_getPhysDevScreenPresSupport =
            reinterpret_cast<PFN_vkGetPhysicalDeviceScreenPresentationSupportQNX>(
                    m_vkGetInstanceProcAddr(m_vkInst,
                                            "vkGetPhysicalDeviceScreenPresentationSupportQNX"));
    if (!m_getPhysDevScreenPresSupport)
        qCWarning(lcQpaQnxVulkan, "Failed to find vkGetPhysicalDeviceScreenPresentationSupportQNX");
}

bool QQnxVulkanInstance::supportsPresent(VkPhysicalDevice physicalDevice,
                                          uint32_t queueFamilyIndex,
                                          QWindow *window)
{
    if (window->surfaceType() != QSurface::VulkanSurface) {
        qCWarning(lcQpaQnxVulkan, "supportsPresent: called on non-Vulkan window");
        return false;
    }

    // Prefer the QNX-specific check: it doesn't require a VkSurfaceKHR to exist yet,
    // avoiding premature surface creation in the Vulkan driver.
    if (m_getPhysDevScreenPresSupport) {
        QQnxWindow *w = static_cast<QQnxWindow *>(window->handle());
        if (!w) {
            qCWarning(lcQpaQnxVulkan, "supportsPresent: window has no platform handle");
            return false;
        }
        return m_getPhysDevScreenPresSupport(physicalDevice, queueFamilyIndex, w->nativeHandle());
    }

    // Fallback: generic surface support check (requires the surface to already exist).
    if (m_getPhysDevSurfaceSupport) {
        VkSurfaceKHR surface = QVulkanInstance::surfaceForWindow(window);
        if (surface) {
            VkBool32 supported = false;
            VkResult err = m_getPhysDevSurfaceSupport(physicalDevice, queueFamilyIndex, surface, &supported);
            if (err != VK_SUCCESS) {
                qCWarning(lcQpaQnxVulkan, "vkGetPhysicalDeviceSurfaceSupportKHR failed: %d", err);
                return true;
            }
            return bool(supported);
        }
    }

    return true;
}

void QQnxVulkanInstance::presentAboutToBeQueued(QWindow *window)
{
    auto *w = static_cast<QQnxWindow *>(window->handle());
    if (!w) {
        qWarning("QQnxVulkanInstance: presentAboutToBeQueued() called without a valid platform window");
        return;
    }
    w->windowPosted();
}

VkSurfaceKHR QQnxVulkanInstance::createSurface(QQnxWindow *window)
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    if (!m_vkInst) {
        qCWarning(lcQpaQnxVulkan, "createSurface: Vulkan instance not available");
        return VK_NULL_HANDLE;
    }

    if (!m_createSurface) {
        m_createSurface = reinterpret_cast<PFN_vkCreateScreenSurfaceQNX>(
                m_vkGetInstanceProcAddr(m_vkInst, "vkCreateScreenSurfaceQNX"));
    }
    if (!m_createSurface) {
        qCWarning(lcQpaQnxVulkan, "Failed to find vkCreateScreenSurfaceQNX");
        return surface;
    }

    if (!window->nativeHandle()) {
        qCWarning(lcQpaQnxVulkan, "Window has no native screen_window_t handle");
        return surface;
    }

    VkScreenSurfaceCreateInfoQNX surfaceInfo;
    memset(&surfaceInfo, 0, sizeof(surfaceInfo));
    surfaceInfo.sType   = VK_STRUCTURE_TYPE_SCREEN_SURFACE_CREATE_INFO_QNX;
    surfaceInfo.context = window->screenContext();
    surfaceInfo.window  = window->nativeHandle();

    VkResult err = m_createSurface(m_vkInst, &surfaceInfo, nullptr, &surface);

    if (err != VK_SUCCESS) {
        qCWarning(lcQpaQnxVulkan, "Failed to create Vulkan screen surface: %d", err);
    } else {
        qCDebug(lcQpaQnxVulkan, "Created Vulkan surface %p for window %p", surface, window->nativeHandle());
    }

    return surface;
}

QT_END_NAMESPACE
