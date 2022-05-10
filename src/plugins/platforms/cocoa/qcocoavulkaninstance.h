// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOCOAVULKANINSTANCE_H
#define QCOCOAVULKANINSTANCE_H

// Include mvk_vulkan.h first. The order is important since
// mvk_vulkan.h just defines VK_USE_PLATFORM_MACOS_MVK (or the IOS
// variant) and includes vulkan.h. If something else included vulkan.h
// before this then we wouldn't get the MVK specifics...
#include <MoltenVK/mvk_vulkan.h>

#include <QtCore/QHash>
#include <QtGui/private/qbasicvulkanplatforminstance_p.h>

QT_BEGIN_NAMESPACE

class QCocoaVulkanInstance : public QBasicPlatformVulkanInstance
{
public:
    QCocoaVulkanInstance(QVulkanInstance *instance);
    ~QCocoaVulkanInstance();

    void createOrAdoptInstance() override;

    VkSurfaceKHR *surface(QWindow *window);

private:
    VkSurfaceKHR createSurface(NSView *view);

    QVulkanInstance *m_instance = nullptr;
    QLibrary m_lib;
    VkSurfaceKHR m_nullSurface = nullptr;
    PFN_vkCreateMacOSSurfaceMVK m_createSurface = nullptr;
};

QT_END_NAMESPACE

#endif // QXCBVULKANINSTANCE_H
