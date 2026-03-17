// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqnxglobal.h"

#include "qqnxvulkanwindow.h"
#include "qqnxintegration.h"

#include <QtGui/QVulkanInstance>

QT_BEGIN_NAMESPACE

QQnxVulkanWindow::QQnxVulkanWindow(QWindow *window, screen_context_t context, bool needRootWindow)
    : QQnxWindow(window, context, needRootWindow),
      m_surface(VK_NULL_HANDLE)
{
    // Set Vulkan usage on the screen window before initializing.
    // This must be done before any swapchain/buffer creation.
    int usage = SCREEN_USAGE_VULKAN;

    Q_SCREEN_CHECKERROR(screen_set_window_property_iv(nativeHandle(), SCREEN_PROPERTY_USAGE, &usage),
                        "Failed to set vulkan usage");

    initWindow();

    qCDebug(lcQpaQnxVulkan, "Created Vulkan window %p", nativeHandle());
}

QQnxVulkanWindow::~QQnxVulkanWindow()
{
    if (m_surface != VK_NULL_HANDLE) {
        QVulkanInstance *inst = window()->vulkanInstance();
        if (inst)
            static_cast<QQnxVulkanInstance *>(inst->handle())->destroySurface(m_surface);
        m_surface = VK_NULL_HANDLE;
    }
}

int QQnxVulkanWindow::pixelFormat() const
{
    // Return -1 to prevent QQnxWindow::setBufferSize from creating screen buffers.
    // Vulkan manages its own swapchain buffers via the VkSwapchainKHR.
    return -1;
}

VkSurfaceKHR *QQnxVulkanWindow::surface()
{
    if (m_surface != VK_NULL_HANDLE)
        return &m_surface;

    QVulkanInstance *inst = window()->vulkanInstance();
    if (!inst) {
        qCWarning(lcQpaQnxVulkan, "Attempted to create Vulkan surface without a QVulkanInstance; "
                 "was QWindow::setVulkanInstance() called?");
        return nullptr;
    }

    m_surface = static_cast<QQnxVulkanInstance *>(inst->handle())->createSurface(this);
    return &m_surface;
}

QT_END_NAMESPACE
