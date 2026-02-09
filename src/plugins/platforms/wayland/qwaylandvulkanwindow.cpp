// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandvulkanwindow_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandVulkanWindow::QWaylandVulkanWindow(QWindow *window, QWaylandDisplay *display)
    : QWaylandWindow(window, display)
{
}

QWaylandVulkanWindow::~QWaylandVulkanWindow()
{
    invalidateSurface();
}

QWaylandWindow::WindowType QWaylandVulkanWindow::windowType() const
{
    return QWaylandWindow::Vulkan;
}

void QWaylandVulkanWindow::invalidateSurface()
{
    if (m_surface) {
        QVulkanInstance *inst = window()->vulkanInstance();
        if (inst)
            static_cast<QWaylandVulkanInstance *>(inst->handle())->destroySurface(m_surface);
    }
    m_surface = VK_NULL_HANDLE;
    QWaylandWindow::invalidateSurface();
}

VkSurfaceKHR *QWaylandVulkanWindow::vkSurface()
{
    if (m_surface)
        return &m_surface;

    QVulkanInstance *vulkanInstance = window()->vulkanInstance();
    if (!vulkanInstance) {
        qWarning() << "Attempted to create Vulkan surface without an instance; was QWindow::setVulkanInstance() called?";
        return nullptr;
    }

    auto *waylandVulkanInstance = static_cast<QWaylandVulkanInstance *>(vulkanInstance->handle());
    m_surface = waylandVulkanInstance->createSurface(this);

    return &m_surface;
}

} // namespace QtWaylandClient

QT_END_NAMESPACE
