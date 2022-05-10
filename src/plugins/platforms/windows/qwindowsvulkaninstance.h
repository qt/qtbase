// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSVULKANINSTANCE_H
#define QWINDOWSVULKANINSTANCE_H

#if defined(VULKAN_H_) && !defined(VK_USE_PLATFORM_WIN32_KHR)
#error "vulkan.h included without Win32 WSI"
#endif

#define VK_USE_PLATFORM_WIN32_KHR

#include <QtGui/private/qbasicvulkanplatforminstance_p.h>
#include <QtCore/qlibrary.h>

QT_BEGIN_NAMESPACE

class QWindowsVulkanInstance : public QBasicPlatformVulkanInstance
{
    Q_DISABLE_COPY_MOVE(QWindowsVulkanInstance)
public:
    QWindowsVulkanInstance(QVulkanInstance *instance);

    void createOrAdoptInstance() override;
    bool supportsPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, QWindow *window) override;

    VkSurfaceKHR createSurface(HWND win);

private:
    QVulkanInstance *m_instance;
    PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR m_getPhysDevPresSupport;
    PFN_vkCreateWin32SurfaceKHR m_createSurface;
};

QT_END_NAMESPACE

#endif // QWINDOWSVULKANINSTANCE_H
