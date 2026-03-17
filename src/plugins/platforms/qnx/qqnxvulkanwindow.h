// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXVULKANWINDOW_H
#define QQNXVULKANWINDOW_H

#include "qqnxwindow.h"
#include "qqnxvulkaninstance.h"

QT_BEGIN_NAMESPACE

class QQnxVulkanWindow : public QQnxWindow
{
public:
    QQnxVulkanWindow(QWindow *window, screen_context_t context, bool needRootWindow);
    ~QQnxVulkanWindow();

    VkSurfaceKHR *surface();

protected:
    int pixelFormat() const override;
    void resetBuffers() override {}

private:
    VkSurfaceKHR m_surface;
};

QT_END_NAMESPACE

#endif // QQNXVULKANWINDOW_H
