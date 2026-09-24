// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "androidjnimain.h"
#include "qandroideventdispatcher.h"
#include "qandroidplatformscreen.h"
#include "qandroidplatformvulkanwindow.h"

#include <QSurfaceFormat>
#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QAndroidPlatformVulkanWindow::QAndroidPlatformVulkanWindow(QWindow *window)
    : QAndroidPlatformWindow(window),
      m_vkSurface(0),
      m_createVkSurface(nullptr),
      m_destroyVkSurface(nullptr)
{
}

QAndroidPlatformVulkanWindow::~QAndroidPlatformVulkanWindow()
{
    // The base class destructor can't reach destroyWindowSurface().
    destroySurface();
}

void QAndroidPlatformVulkanWindow::setGeometry(const QRect &rect)
{
    QAndroidPlatformWindow::setGeometry(rect);

    QRect availableGeometry = screen()->availableGeometry();
    if (rect.width() > 0
            && rect.height() > 0
            && availableGeometry.width() > 0
            && availableGeometry.height() > 0) {
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), rect.size()));
    }
}

QSurfaceFormat QAndroidPlatformVulkanWindow::format() const
{
    return window()->requestedFormat();
}

VkSurfaceKHR *QAndroidPlatformVulkanWindow::vkSurface()
{
    if (Q_LIKELY(m_vkSurface != VK_NULL_HANDLE))
        return &m_vkSurface;
    ensureNativeWindow();
    if (!m_nativeWindow)
        return &m_vkSurface;

    QVulkanInstance *inst = window()->vulkanInstance();
    if (!inst) {
        qWarning("Attempted to create Vulkan surface without an instance; "
                 "was QWindow::setVulkanInstance() called?");
        return &m_vkSurface;
    }
    if (!m_createVkSurface) {
        m_createVkSurface = reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(
                    inst->getInstanceProcAddr("vkCreateAndroidSurfaceKHR"));
    }
    if (!m_destroyVkSurface) {
        m_destroyVkSurface = reinterpret_cast<PFN_vkDestroySurfaceKHR>(
                    inst->getInstanceProcAddr("vkDestroySurfaceKHR"));
    }
    VkAndroidSurfaceCreateInfoKHR surfaceInfo {};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.window = m_nativeWindow;
    const VkResult err = m_createVkSurface(inst->vkInstance(), &surfaceInfo, nullptr, &m_vkSurface);
    if (err != VK_SUCCESS)
        qWarning("Failed to create Android VkSurface: %d", err);
    return &m_vkSurface;
}

void QAndroidPlatformVulkanWindow::destroyWindowSurface()
{
    if (m_vkSurface && m_destroyVkSurface) {
        m_destroyVkSurface(window()->vulkanInstance()->vkInstance(), m_vkSurface, nullptr);
        m_vkSurface = VK_NULL_HANDLE;
    }
}

QT_END_NAMESPACE
