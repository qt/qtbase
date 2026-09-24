// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QANDROIDPLATFORMVULKANWINDOW_H
#define QANDROIDPLATFORMVULKANWINDOW_H

#if defined(VULKAN_H_) && !defined(VK_USE_PLATFORM_ANDROID_KHR)
#error "vulkan.h included without Android WSI"
#endif

#define VK_USE_PLATFORM_ANDROID_KHR

#include "qandroidplatformvulkaninstance.h"
#include "qandroidplatformwindow.h"

#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>

QT_BEGIN_NAMESPACE

class QAndroidPlatformVulkanWindow : public QAndroidPlatformWindow
{
public:
    explicit QAndroidPlatformVulkanWindow(QWindow *window);
    ~QAndroidPlatformVulkanWindow();

    void setGeometry(const QRect &rect) override;
    QSurfaceFormat format() const override;

    // Render thread. Creates the VkSurfaceKHR on first use. The window isn't
    // exposed before a Surface exists, so QRhi always finds one here.
    VkSurfaceKHR *vkSurface();

private:
    void destroyWindowSurface() override;
    QSurfaceFormat m_format;
    // Render-thread-only state.
    VkSurfaceKHR m_vkSurface;
    PFN_vkCreateAndroidSurfaceKHR m_createVkSurface;
    PFN_vkDestroySurfaceKHR m_destroyVkSurface;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMVULKANWINDOW_H
