/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qwindowsvulkaninstance.h"

QT_BEGIN_NAMESPACE

QWindowsVulkanInstance::QWindowsVulkanInstance(QVulkanInstance *instance)
    : m_instance(instance),
      m_getPhysDevPresSupport(nullptr),
      m_createSurface(nullptr),
      m_destroySurface(nullptr)
{
    if (qEnvironmentVariableIsSet("QT_VULKAN_LIB"))
        m_lib.setFileName(QString::fromUtf8(qgetenv("QT_VULKAN_LIB")));
    else
        m_lib.setFileName(QStringLiteral("vulkan-1"));

    if (!m_lib.load()) {
        qWarning("Failed to load %s: %s", qPrintable(m_lib.fileName()), qPrintable(m_lib.errorString()));
        return;
    }

    init(&m_lib);
}

void QWindowsVulkanInstance::createOrAdoptInstance()
{
    initInstance(m_instance, QByteArrayList() << QByteArrayLiteral("VK_KHR_win32_surface"));

    if (!m_vkInst)
        return;

    m_getPhysDevPresSupport = reinterpret_cast<PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR>(
                m_vkGetInstanceProcAddr(m_vkInst, "vkGetPhysicalDeviceWin32PresentationSupportKHR"));
    if (!m_getPhysDevPresSupport)
        qWarning("Failed to find vkGetPhysicalDeviceWin32PresentationSupportKHR");
}

QWindowsVulkanInstance::~QWindowsVulkanInstance()
{
}

bool QWindowsVulkanInstance::supportsPresent(VkPhysicalDevice physicalDevice,
                                             uint32_t queueFamilyIndex,
                                             QWindow *window)
{
    if (!m_getPhysDevPresSupport || !m_getPhysDevSurfaceSupport)
        return true;

    bool ok = m_getPhysDevPresSupport(physicalDevice, queueFamilyIndex);

    VkSurfaceKHR surface = QVulkanInstance::surfaceForWindow(window);
    VkBool32 supported = false;
    m_getPhysDevSurfaceSupport(physicalDevice, queueFamilyIndex, surface, &supported);
    ok &= bool(supported);

    return ok;
}

VkSurfaceKHR QWindowsVulkanInstance::createSurface(HWND win)
{
    VkSurfaceKHR surface = 0;

    if (!m_createSurface) {
        m_createSurface = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
                    m_vkGetInstanceProcAddr(m_vkInst, "vkCreateWin32SurfaceKHR"));
    }
    if (!m_createSurface) {
        qWarning("Failed to find vkCreateWin32SurfaceKHR");
        return surface;
    }
    if (!m_destroySurface) {
        m_destroySurface = reinterpret_cast<PFN_vkDestroySurfaceKHR>(
                    m_vkGetInstanceProcAddr(m_vkInst, "vkDestroySurfaceKHR"));
    }
    if (!m_destroySurface) {
        qWarning("Failed to find vkDestroySurfaceKHR");
        return surface;
    }

    VkWin32SurfaceCreateInfoKHR surfaceInfo;
    memset(&surfaceInfo, 0, sizeof(surfaceInfo));
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hinstance = GetModuleHandle(nullptr);
    surfaceInfo.hwnd = win;
    VkResult err = m_createSurface(m_vkInst, &surfaceInfo, nullptr, &surface);
    if (err != VK_SUCCESS)
        qWarning("Failed to create Vulkan surface: %d", err);

    return surface;
}

void QWindowsVulkanInstance::destroySurface(VkSurfaceKHR surface)
{
    if (m_destroySurface && surface)
        m_destroySurface(m_vkInst, surface, nullptr);
}

QT_END_NAMESPACE
