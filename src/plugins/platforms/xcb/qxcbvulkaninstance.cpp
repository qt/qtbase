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

#include "qxcbvulkaninstance.h"
#include "qxcbwindow.h"
#include "qxcbscreen.h"

QT_BEGIN_NAMESPACE

QXcbVulkanInstance::QXcbVulkanInstance(QVulkanInstance *instance)
    : m_instance(instance),
      m_getPhysDevPresSupport(nullptr),
      m_createSurface(nullptr)
{
    loadVulkanLibrary(QStringLiteral("vulkan"));
}

QXcbVulkanInstance::~QXcbVulkanInstance()
{
}

void QXcbVulkanInstance::createOrAdoptInstance()
{
    initInstance(m_instance, QByteArrayList() << QByteArrayLiteral("VK_KHR_xcb_surface"));

    if (!m_vkInst)
        return;

    m_getPhysDevPresSupport = reinterpret_cast<PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR>(
                m_vkGetInstanceProcAddr(m_vkInst, "vkGetPhysicalDeviceXcbPresentationSupportKHR"));
    if (!m_getPhysDevPresSupport)
        qWarning("Failed to find vkGetPhysicalDeviceXcbPresentationSupportKHR");
}

bool QXcbVulkanInstance::supportsPresent(VkPhysicalDevice physicalDevice,
                                         uint32_t queueFamilyIndex,
                                         QWindow *window)
{
    if (!m_getPhysDevPresSupport || !m_getPhysDevSurfaceSupport)
        return true;

    QXcbWindow *w = static_cast<QXcbWindow *>(window->handle());
    if (!w) {
        qWarning("Attempted to call supportsPresent() without a valid platform window");
        return false;
    }
    xcb_connection_t *connection = w->xcbScreen()->xcb_connection();
    bool ok = m_getPhysDevPresSupport(physicalDevice, queueFamilyIndex, connection, w->visualId());

    VkSurfaceKHR surface = QVulkanInstance::surfaceForWindow(window);
    VkBool32 supported = false;
    m_getPhysDevSurfaceSupport(physicalDevice, queueFamilyIndex, surface, &supported);
    ok &= bool(supported);

    return ok;
}

VkSurfaceKHR QXcbVulkanInstance::createSurface(QXcbWindow *window)
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    if (!m_createSurface) {
        m_createSurface = reinterpret_cast<PFN_vkCreateXcbSurfaceKHR>(
                    m_vkGetInstanceProcAddr(m_vkInst, "vkCreateXcbSurfaceKHR"));
    }
    if (!m_createSurface) {
        qWarning("Failed to find vkCreateXcbSurfaceKHR");
        return surface;
    }

    VkXcbSurfaceCreateInfoKHR surfaceInfo;
    memset(&surfaceInfo, 0, sizeof(surfaceInfo));
    surfaceInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.connection = window->xcbScreen()->xcb_connection();
    surfaceInfo.window = window->xcb_window();
    VkResult err = m_createSurface(m_vkInst, &surfaceInfo, nullptr, &surface);
    if (err != VK_SUCCESS)
        qWarning("Failed to create Vulkan surface: %d", err);

    return surface;
}

void QXcbVulkanInstance::presentQueued(QWindow *window)
{
    QXcbWindow *w = static_cast<QXcbWindow *>(window->handle());
    if (!w) {
        qWarning("Attempted to call presentQueued() without a valid platform window");
        return;
    }

    if (w->needsSync())
        w->postSyncWindowRequest();
}

QT_END_NAMESPACE
