// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    quint32 gfxQueueFamilyIdx = 0;
    quint32 gfxQueueIdx = 0;
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
