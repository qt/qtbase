/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Gui module
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
******************************************************************************/

#ifndef QRHIVULKAN_H
#define QRHIVULKAN_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qrhi_p.h>
#include <QtGui/qvulkaninstance.h> // this is where vulkan.h gets pulled in

QT_BEGIN_NAMESPACE

struct Q_GUI_EXPORT QRhiVulkanInitParams : public QRhiInitParams
{
    QVulkanInstance *inst = nullptr;
    QWindow *window = nullptr;
    QByteArrayList deviceExtensions;

    static QByteArrayList preferredInstanceExtensions();
    static QByteArrayList preferredExtensionsForImportedDevice();
};

struct Q_GUI_EXPORT QRhiVulkanNativeHandles : public QRhiNativeHandles
{
    // to import a physical device (always required)
    VkPhysicalDevice physDev = VK_NULL_HANDLE;
    // to import a device and queue
    VkDevice dev = VK_NULL_HANDLE;
    int gfxQueueFamilyIdx = -1;
    int gfxQueueIdx = 0;
    VkQueue gfxQueue = VK_NULL_HANDLE;
    // and optionally, the mem allocator
    void *vmemAllocator = nullptr;
};

struct Q_GUI_EXPORT QRhiVulkanCommandBufferNativeHandles : public QRhiNativeHandles
{
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
};

struct Q_GUI_EXPORT QRhiVulkanRenderPassNativeHandles : public QRhiNativeHandles
{
    VkRenderPass renderPass = VK_NULL_HANDLE;
};

QT_END_NAMESPACE

#endif
