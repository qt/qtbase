// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qxcbvulkanwindow.h"

QT_BEGIN_NAMESPACE

QXcbVulkanWindow::QXcbVulkanWindow(QWindow *window)
    : QXcbWindow(window),
      m_surface(VK_NULL_HANDLE)
{
}

QXcbVulkanWindow::~QXcbVulkanWindow()
{
    if (m_surface) {
        QVulkanInstance *inst = window()->vulkanInstance();
        if (inst)
            static_cast<QXcbVulkanInstance *>(inst->handle())->destroySurface(m_surface);
    }
}

void QXcbVulkanWindow::resolveFormat(const QSurfaceFormat &format)
{
    m_format = format;
    if (m_format.redBufferSize() <= 0)
        m_format.setRedBufferSize(8);
    if (m_format.greenBufferSize() <= 0)
        m_format.setGreenBufferSize(8);
    if (m_format.blueBufferSize() <= 0)
        m_format.setBlueBufferSize(8);
}

// No createVisual() needed, use the default that picks one based on the R/G/B/A size.

VkSurfaceKHR *QXcbVulkanWindow::surface()
{
    if (m_surface)
        return &m_surface;

    QVulkanInstance *inst = window()->vulkanInstance();
    if (!inst) {
        qWarning("Attempted to create Vulkan surface without an instance; was QWindow::setVulkanInstance() called?");
        return nullptr;
    }
    QXcbVulkanInstance *xcbinst = static_cast<QXcbVulkanInstance *>(inst->handle());
    m_surface = xcbinst->createSurface(this);

    return &m_surface;
}

QT_END_NAMESPACE
