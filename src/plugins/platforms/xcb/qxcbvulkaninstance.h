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

#ifndef QXCBVULKANINSTANCE_H
#define QXCBVULKANINSTANCE_H

#if defined(VULKAN_H_) && !defined(VK_USE_PLATFORM_XCB_KHR)
#error "vulkan.h included without xcb WSI"
#endif

#define VK_USE_PLATFORM_XCB_KHR

#include <QtVulkanSupport/private/qbasicvulkanplatforminstance_p.h>
#include <QLibrary>

QT_BEGIN_NAMESPACE

class QXcbWindow;

class QXcbVulkanInstance : public QBasicPlatformVulkanInstance
{
public:
    QXcbVulkanInstance(QVulkanInstance *instance);
    ~QXcbVulkanInstance();

    void createOrAdoptInstance() override;
    bool supportsPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, QWindow *window) override;
    void presentQueued(QWindow *window) override;

    VkSurfaceKHR createSurface(QXcbWindow *window);
    void destroySurface(VkSurfaceKHR surface);

private:
    QVulkanInstance *m_instance;
    QLibrary m_lib;
    PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR m_getPhysDevPresSupport;
    PFN_vkCreateXcbSurfaceKHR m_createSurface;
    PFN_vkDestroySurfaceKHR m_destroySurface;
};

QT_END_NAMESPACE

#endif // QXCBVULKANINSTANCE_H
