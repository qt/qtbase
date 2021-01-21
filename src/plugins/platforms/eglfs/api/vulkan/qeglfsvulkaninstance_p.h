/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QEGLFSVULKANINSTANCE_H
#define QEGLFSVULKANINSTANCE_H

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

#include "qeglfsglobal_p.h"
#include <QtVulkanSupport/private/qbasicvulkanplatforminstance_p.h>

QT_BEGIN_NAMESPACE

class QEglFSWindow;

class Q_EGLFS_EXPORT QEglFSVulkanInstance : public QBasicPlatformVulkanInstance
{
public:
    QEglFSVulkanInstance(QVulkanInstance *instance);

    void createOrAdoptInstance() override;
    bool supportsPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, QWindow *window) override;
    void presentAboutToBeQueued(QWindow *window) override;

    VkSurfaceKHR createSurface(QEglFSWindow *window);

private:
    QVulkanInstance *m_instance;
    VkPhysicalDevice m_physDev = VK_NULL_HANDLE;
    PFN_vkEnumeratePhysicalDevices m_enumeratePhysicalDevices = nullptr;
#if VK_KHR_display
    PFN_vkGetPhysicalDeviceDisplayPropertiesKHR m_getPhysicalDeviceDisplayPropertiesKHR = nullptr;
    PFN_vkGetDisplayModePropertiesKHR m_getDisplayModePropertiesKHR = nullptr;
    PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR m_getPhysicalDeviceDisplayPlanePropertiesKHR = nullptr;
    PFN_vkGetDisplayPlaneSupportedDisplaysKHR m_getDisplayPlaneSupportedDisplaysKHR = nullptr;
    PFN_vkGetDisplayPlaneCapabilitiesKHR m_getDisplayPlaneCapabilitiesKHR = nullptr;
    PFN_vkCreateDisplayPlaneSurfaceKHR m_createDisplayPlaneSurfaceKHR = nullptr;
#endif
};

QT_END_NAMESPACE

#endif
