/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    initInstance(m_instance, QByteArrayList() << QByteArrayLiteral("VK_MVK_macos_surface"));
}

VkSurfaceKHR *QCocoaVulkanInstance::surface(QWindow *window)
{
    QCocoaWindow *cocoaWindow = static_cast<QCocoaWindow *>(window->handle());
    if (!cocoaWindow->m_vulkanSurface)
        cocoaWindow->m_vulkanSurface = createSurface(cocoaWindow->m_view);
    return &cocoaWindow->m_vulkanSurface;
}

VkSurfaceKHR QCocoaVulkanInstance::createSurface(NSView *view)
{
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
    surfaceInfo.pView = view.layer;

    VkSurfaceKHR surface = nullptr;
    VkResult err = m_createSurface(m_vkInst, &surfaceInfo, nullptr, &surface);
    if (err != VK_SUCCESS)
        qWarning("Failed to create Vulkan surface: %d", err);

    return surface;
}


QT_END_NAMESPACE
