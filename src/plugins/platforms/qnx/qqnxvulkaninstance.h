// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXVULKANINSTANCE_H
#define QQNXVULKANINSTANCE_H

#if defined(VULKAN_H_) && !defined(VK_USE_PLATFORM_SCREEN_QNX)
#error "vulkan.h included without QNX Screen WSI"
#endif

#define VK_USE_PLATFORM_SCREEN_QNX

#include <QtGui/private/qbasicvulkanplatforminstance_p.h>
#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaQnxVulkan)

class QQnxWindow;

class QQnxVulkanInstance : public QBasicPlatformVulkanInstance
{
public:
    QQnxVulkanInstance(QVulkanInstance *instance);
    ~QQnxVulkanInstance() override;

    void createOrAdoptInstance() override;
    bool supportsPresent(VkPhysicalDevice physicalDevice,
                         uint32_t queueFamilyIndex,
                         QWindow *window) override;
    // presentQueued() is intentionally not overridden: the QNX Screen ICD calls
    // screen_post_window() internally during vkQueuePresentKHR, so no explicit
    // screen_flush_context() is needed (unlike the raster path).
    //
    // presentAboutToBeQueued() sets the frame-pacing gate on the window so that
    // requestUpdate() is suppressed until the SCREEN_NOTIFY_UPDATE pulse fires.
    // This mirrors QWaylandVulkanInstance::presentAboutToBeQueued().
    void presentAboutToBeQueued(QWindow *window) override;

    VkSurfaceKHR createSurface(QQnxWindow *window);

private:
    QVulkanInstance *m_instance;
    PFN_vkCreateScreenSurfaceQNX m_createSurface;
    PFN_vkGetPhysicalDeviceScreenPresentationSupportQNX m_getPhysDevScreenPresSupport;
};

QT_END_NAMESPACE

#endif // QQNXVULKANINSTANCE_H
