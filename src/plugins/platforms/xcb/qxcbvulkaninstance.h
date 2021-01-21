/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of plugins of the Qt Toolkit.
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

#ifndef QXCBVULKANINSTANCE_H
#define QXCBVULKANINSTANCE_H

#if defined(VULKAN_H_) && !defined(VK_USE_PLATFORM_XCB_KHR)
#error "vulkan.h included without xcb WSI"
#endif

#define VK_USE_PLATFORM_XCB_KHR

#include <QtVulkanSupport/private/qbasicvulkanplatforminstance_p.h>
#include <QLibrary>

QT_BEGIN_NAMESPACE

class QXcbWindow;

class QXcbVulkanInstance : public QBasicPlatformVulkanInstance
{
public:
    QXcbVulkanInstance(QVulkanInstance *instance);
    ~QXcbVulkanInstance();

    void createOrAdoptInstance() override;
    bool supportsPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, QWindow *window) override;
    void presentQueued(QWindow *window) override;

    VkSurfaceKHR createSurface(QXcbWindow *window);

private:
    QVulkanInstance *m_instance;
    PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR m_getPhysDevPresSupport;
    PFN_vkCreateXcbSurfaceKHR m_createSurface;
};

QT_END_NAMESPACE

#endif // QXCBVULKANINSTANCE_H
