// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVKKHRDISPLAYVULKANINSTANCE_H
#define QVKKHRDISPLAYVULKANINSTANCE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qbasicvulkanplatforminstance_p.h>
#include <QtCore/qsize.h>

#include <climits> // For UINT_MAX

QT_BEGIN_NAMESPACE

class QVkKhrDisplayVulkanInstance : public QBasicPlatformVulkanInstance
{
public:
    QVkKhrDisplayVulkanInstance(QVulkanInstance *instance);

    void createOrAdoptInstance() override;
    bool supportsPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, QWindow *window) override;
    void presentAboutToBeQueued(QWindow *window) override;

    VkSurfaceKHR createSurface(QWindow *window);

    QSize displaySize() const { return QSize(int(m_width), int(m_height)); }

    using CreatedCallback = void (*)(QVkKhrDisplayVulkanInstance *, void *);
    void setCreatedCallback(CreatedCallback callback, void *userData) {
        m_createdCallback = callback;
        m_createdCallbackUserData = userData;
    }

private:
    bool chooseDisplay();

    QVulkanInstance *m_instance;
    VkPhysicalDevice m_physDev = VK_NULL_HANDLE;
    PFN_vkEnumeratePhysicalDevices m_enumeratePhysicalDevices = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR m_getPhysicalDeviceSurfaceSupportKHR = nullptr;
#if VK_KHR_display
    PFN_vkGetPhysicalDeviceDisplayPropertiesKHR m_getPhysicalDeviceDisplayPropertiesKHR = nullptr;
    PFN_vkGetDisplayModePropertiesKHR m_getDisplayModePropertiesKHR = nullptr;
    PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR m_getPhysicalDeviceDisplayPlanePropertiesKHR = nullptr;
    PFN_vkGetDisplayPlaneSupportedDisplaysKHR m_getDisplayPlaneSupportedDisplaysKHR = nullptr;
    PFN_vkGetDisplayPlaneCapabilitiesKHR m_getDisplayPlaneCapabilitiesKHR = nullptr;
    PFN_vkCreateDisplayPlaneSurfaceKHR m_createDisplayPlaneSurfaceKHR = nullptr;
#endif

    CreatedCallback m_createdCallback = nullptr;
    void *m_createdCallbackUserData = nullptr;
    VkDisplayKHR m_display = VK_NULL_HANDLE;
    VkDisplayModeKHR m_displayMode = VK_NULL_HANDLE;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_planeIndex = UINT_MAX;
    uint32_t m_planeStackIndex = UINT_MAX;
};

QT_END_NAMESPACE

#endif
