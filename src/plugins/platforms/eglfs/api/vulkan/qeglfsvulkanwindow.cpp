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

#include "qeglfsvulkanwindow_p.h"

QT_BEGIN_NAMESPACE

QEglFSVulkanWindow::QEglFSVulkanWindow(QWindow *window)
    : QEglFSWindow(window),
      m_surface(VK_NULL_HANDLE)
{
}

QEglFSVulkanWindow::~QEglFSVulkanWindow()
{
    if (m_surface) {
        QVulkanInstance *inst = window()->vulkanInstance();
        if (inst)
            static_cast<QEglFSVulkanInstance *>(inst->handle())->destroySurface(m_surface);
    }
}

void *QEglFSVulkanWindow::vulkanSurfacePtr()
{
    if (m_surface)
        return &m_surface;

    QVulkanInstance *inst = window()->vulkanInstance();
    if (!inst) {
        qWarning("Attempted to create Vulkan surface without an instance; was QWindow::setVulkanInstance() called?");
        return nullptr;
    }
    QEglFSVulkanInstance *eglfsInst = static_cast<QEglFSVulkanInstance *>(inst->handle());
    m_surface = eglfsInst->createSurface(this);

    return &m_surface;
}

QT_END_NAMESPACE
