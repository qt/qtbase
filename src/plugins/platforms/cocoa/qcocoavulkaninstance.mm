// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <AppKit/AppKit.h>
#import <QuartzCore/CAMetalLayer.h>

#include "qcocoavulkaninstance.h"
#include "qcocoawindow.h"

QT_BEGIN_NAMESPACE

QCocoaVulkanInstance::QCocoaVulkanInstance(QVulkanInstance *instance)
    : m_instance(instance)
{
    loadVulkanLibrary(QStringLiteral("vulkan"));
}

QCocoaVulkanInstance::~QCocoaVulkanInstance()
{
}

void QCocoaVulkanInstance::createOrAdoptInstance()
{
    initInstance(m_instance, QByteArrayList() << QByteArrayLiteral("VK_EXT_metal_surface") << QByteArrayLiteral("VK_MVK_macos_surface"));
}

VkSurfaceKHR *QCocoaVulkanInstance::surface(QWindow *window)
{
    QCocoaWindow *cocoaWindow = static_cast<QCocoaWindow *>(window->handle());
    if (!cocoaWindow->m_vulkanSurface)
        cocoaWindow->m_vulkanSurface = createSurface(cocoaWindow->contentLayer());
    return &cocoaWindow->m_vulkanSurface;
}

VkSurfaceKHR QCocoaVulkanInstance::createSurface(CALayer *layer)
{
    VkSurfaceKHR surface = nullptr;
    if (!m_createMetalSurface) {
        m_createMetalSurface = reinterpret_cast<PFN_vkCreateMetalSurfaceEXT>(
                    m_vkGetInstanceProcAddr(m_vkInst, "vkCreateMetalSurfaceEXT"));
    }
    if (m_createMetalSurface) {
        VkMetalSurfaceCreateInfoEXT info = {};
        info.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        info.pLayer = qt_objc_cast<CAMetalLayer*>(layer);
        VkResult err = m_createMetalSurface(m_vkInst, &info, nullptr, &surface);
        if (err == VK_SUCCESS)
            return surface;
        qWarning("Failed to create Vulkan surface from metal layer: %d", err);
    }
    if (!m_createSurface) {
        m_createSurface = reinterpret_cast<PFN_vkCreateMacOSSurfaceMVK>(
                    m_vkGetInstanceProcAddr(m_vkInst, "vkCreateMacOSSurfaceMVK"));
    }
    if (!m_createSurface) {
        qWarning("Failed to find vkCreateMacOSSurfaceMVK");
        return m_nullSurface;
    }

    VkMacOSSurfaceCreateInfoMVK surfaceInfo;
    surfaceInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
    surfaceInfo.pNext = nullptr;
    surfaceInfo.flags = 0;
    surfaceInfo.pView = layer;

    VkResult err = m_createSurface(m_vkInst, &surfaceInfo, nullptr, &surface);
    if (err != VK_SUCCESS)
        qWarning("Failed to create Vulkan surface: %d", err);

    return surface;
}


QT_END_NAMESPACE
