/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Gui module
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qrhivulkan_p_p.h"
#include "qrhivulkanext_p.h"

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_RECORDING_ENABLED 0
#define VMA_DEDICATED_ALLOCATION 0
#ifdef QT_DEBUG
#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#endif
#include "vk_mem_alloc.h"

#include <qmath.h>
#include <QVulkanFunctions>
#include <QtGui/qwindow.h>

QT_BEGIN_NAMESPACE

/*
  Vulkan 1.0 backend. Provides a double-buffered swapchain that throttles the
  rendering thread to vsync. Textures and "static" buffers are device local,
  and a separate, host visible staging buffer is used to upload data to them.
  "Dynamic" buffers are in host visible memory and are duplicated (since there
  can be 2 frames in flight). This is handled transparently to the application.

  Barriers are generated automatically for each render or compute pass, based
  on the resources that are used in that pass (in QRhiShaderResourceBindings,
  vertex inputs, etc.). This implies deferring the recording of the command
  buffer since the barriers have to be placed at the right place (before the
  pass), and that can only be done once we know all the things the pass does.

  This in turn has implications for integrating external commands
  (beginExternal() - direct Vulkan calls - endExternal()) because that is
  incompatible with this approach by nature. Therefore we support another mode
  of operation, where each render or compute pass uses one or more secondary
  command buffers (recorded right away), with each beginExternal() leading to
  closing the current secondary cb, creating a new secondary cb for the
  external content, and then starting yet another one in endExternal() for
  whatever comes afterwards in the pass. This way the primary command buffer
  only has vkCmdExecuteCommand(s) within a renderpass instance
  (Begin-EndRenderPass). (i.e. our only subpass is then
  VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS instead of
  VK_SUBPASS_CONTENTS_INLINE)

  The command buffer management mode is decided on a per frame basis,
  controlled by the ExternalContentsInPass flag of beginFrame().
*/

/*!
    \class QRhiVulkanInitParams
    \internal
    \inmodule QtGui
    \brief Vulkan specific initialization parameters.

    A Vulkan-based QRhi needs at minimum a valid QVulkanInstance. It is up to
    the user to ensure this is available and initialized. This is typically
    done in main() similarly to the following:

    \badcode
    int main(int argc, char **argv)
    {
        ...

        QVulkanInstance inst;
    #ifndef Q_OS_ANDROID
        inst.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");
    #else
        inst.setLayers(QByteArrayList()
                       << "VK_LAYER_GOOGLE_threading"
                       << "VK_LAYER_LUNARG_parameter_validation"
                       << "VK_LAYER_LUNARG_object_tracker"
                       << "VK_LAYER_LUNARG_core_validation"
                       << "VK_LAYER_LUNARG_image"
                       << "VK_LAYER_LUNARG_swapchain"
                       << "VK_LAYER_GOOGLE_unique_objects");
    #endif
        inst.setExtensions(QByteArrayList()
                           << "VK_KHR_get_physical_device_properties2");
        if (!inst.create())
            qFatal("Vulkan not available");

        ...
    }
    \endcode

    The example here has two optional aspects: it enables the
    \l{https://github.com/KhronosGroup/Vulkan-ValidationLayers}{Vulkan
    validation layers}, when they are available, and also enables the
    VK_KHR_get_physical_device_properties2 extension (part of Vulkan 1.1), when
    available. The former is useful during the development phase (remember that
    QVulkanInstance conveniently redirects messages and warnings to qDebug).
    Avoid enabling it in production builds, however. The latter is important in
    order to make QRhi::CustomInstanceStepRate available with Vulkan since
    VK_EXT_vertex_attribute_divisor (part of Vulkan 1.1) depends on it. It can
    be omitted when instanced drawing with a non-one step rate is not used.

    Once this is done, a Vulkan-based QRhi can be created by passing the
    instance and a QWindow with its surface type set to
    QSurface::VulkanSurface:

    \badcode
        QRhiVulkanInitParams params;
        params.inst = vulkanInstance;
        params.window = window;
        rhi = QRhi::create(QRhi::Vulkan, &params);
    \endcode

    The window is optional and can be omitted. This is not recommended however
    because there is then no way to ensure presenting is supported while
    choosing a graphics queue.

    \note Even when a window is specified, QRhiSwapChain objects can be created
    for other windows as well, as long as they all have their
    QWindow::surfaceType() set to QSurface::VulkanSurface.

    To request additional extensions to be enabled on the Vulkan device, list them
    in deviceExtensions. This can be relevant when integrating with native Vulkan
    rendering code.

    \section2 Working with existing Vulkan devices

    When interoperating with another graphics engine, it may be necessary to
    get a QRhi instance that uses the same Vulkan device. This can be achieved
    by passing a pointer to a QRhiVulkanNativeHandles to QRhi::create().

    The physical device and device object must then be set to a non-null value.
    In addition, either the graphics queue family index or the graphics queue
    object itself is required. Prefer the former, whenever possible since
    deducing the index is not possible afterwards. Optionally, an existing
    command pool object can be specified as well, and, also optionally,
    vmemAllocator can be used to share the same
    \l{https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator}{Vulkan
    memory allocator} between two QRhi instances.

    The QRhi does not take ownership of any of the external objects.
 */

/*!
    \class QRhiVulkanNativeHandles
    \internal
    \inmodule QtGui
    \brief Collects device, queue, and other Vulkan objects that are used by the QRhi.

    \note Ownership of the Vulkan objects is never transferred.
 */

/*!
    \class QRhiVulkanCommandBufferNativeHandles
    \internal
    \inmodule QtGui
    \brief Holds the Vulkan command buffer object that is backing a QRhiCommandBuffer.

    \note The Vulkan command buffer object is only guaranteed to be valid, and
    in recording state, while recording a frame. That is, between a
    \l{QRhi::beginFrame()}{beginFrame()} - \l{QRhi::endFrame()}{endFrame()} or
    \l{QRhi::beginOffscreenFrame()}{beginOffscreenFrame()} -
    \l{QRhi::endOffsrceenFrame()}{endOffscreenFrame()} pair.
 */

/*!
    \class QRhiVulkanRenderPassNativeHandles
    \internal
    \inmodule QtGui
    \brief Holds the Vulkan render pass object backing a QRhiRenderPassDescriptor.
 */

template <class Int>
inline Int aligned(Int v, Int byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

static QVulkanInstance *globalVulkanInstance;

static void VKAPI_PTR wrap_vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties)
{
    globalVulkanInstance->functions()->vkGetPhysicalDeviceProperties(physicalDevice, pProperties);
}

static void VKAPI_PTR wrap_vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties)
{
    globalVulkanInstance->functions()->vkGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}

static VkResult VKAPI_PTR wrap_vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo* pAllocateInfo, const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory)
{
    return globalVulkanInstance->deviceFunctions(device)->vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
}

void VKAPI_PTR wrap_vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks* pAllocator)
{
    globalVulkanInstance->deviceFunctions(device)->vkFreeMemory(device, memory, pAllocator);
}

VkResult VKAPI_PTR wrap_vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData)
{
    return globalVulkanInstance->deviceFunctions(device)->vkMapMemory(device, memory, offset, size, flags, ppData);
}

void VKAPI_PTR wrap_vkUnmapMemory(VkDevice device, VkDeviceMemory memory)
{
    globalVulkanInstance->deviceFunctions(device)->vkUnmapMemory(device, memory);
}

VkResult VKAPI_PTR wrap_vkFlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges)
{
    return globalVulkanInstance->deviceFunctions(device)->vkFlushMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

VkResult VKAPI_PTR wrap_vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges)
{
    return globalVulkanInstance->deviceFunctions(device)->vkInvalidateMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

VkResult VKAPI_PTR wrap_vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    return globalVulkanInstance->deviceFunctions(device)->vkBindBufferMemory(device, buffer, memory, memoryOffset);
}

VkResult VKAPI_PTR wrap_vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    return globalVulkanInstance->deviceFunctions(device)->vkBindImageMemory(device, image, memory, memoryOffset);
}

void VKAPI_PTR wrap_vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements)
{
    globalVulkanInstance->deviceFunctions(device)->vkGetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}

void VKAPI_PTR wrap_vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements* pMemoryRequirements)
{
    globalVulkanInstance->deviceFunctions(device)->vkGetImageMemoryRequirements(device, image, pMemoryRequirements);
}

VkResult VKAPI_PTR wrap_vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer)
{
    return globalVulkanInstance->deviceFunctions(device)->vkCreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
}

void VKAPI_PTR wrap_vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator)
{
    globalVulkanInstance->deviceFunctions(device)->vkDestroyBuffer(device, buffer, pAllocator);
}

VkResult VKAPI_PTR wrap_vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage)
{
    return globalVulkanInstance->deviceFunctions(device)->vkCreateImage(device, pCreateInfo, pAllocator, pImage);
}

void VKAPI_PTR wrap_vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator)
{
    globalVulkanInstance->deviceFunctions(device)->vkDestroyImage(device, image, pAllocator);
}

static inline VmaAllocation toVmaAllocation(QVkAlloc a)
{
    return reinterpret_cast<VmaAllocation>(a);
}

static inline VmaAllocator toVmaAllocator(QVkAllocator a)
{
    return reinterpret_cast<VmaAllocator>(a);
}

QRhiVulkan::QRhiVulkan(QRhiVulkanInitParams *params, QRhiVulkanNativeHandles *importDevice)
    : ofr(this)
{
    inst = params->inst;
    maybeWindow = params->window; // may be null
    requestedDeviceExtensions = params->deviceExtensions;

    importedDevice = importDevice != nullptr;
    if (importedDevice) {
        physDev = importDevice->physDev;
        dev = importDevice->dev;
        if (physDev && dev) {
            gfxQueueFamilyIdx = importDevice->gfxQueueFamilyIdx;
            gfxQueue = importDevice->gfxQueue;
            if (importDevice->cmdPool) {
                importedCmdPool = true;
                cmdPool = importDevice->cmdPool;
            }
            if (importDevice->vmemAllocator) {
                importedAllocator = true;
                allocator = importDevice->vmemAllocator;
            }
        } else {
            qWarning("No (physical) Vulkan device is given, cannot import");
            importedDevice = false;
        }
    }
}

static bool qvk_debug_filter(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object,
                             size_t location, int32_t messageCode, const char *pLayerPrefix, const char *pMessage)
{
    Q_UNUSED(flags);
    Q_UNUSED(objectType);
    Q_UNUSED(object);
    Q_UNUSED(location);
    Q_UNUSED(messageCode);
    Q_UNUSED(pLayerPrefix);

    // Filter out certain misleading validation layer messages, as per
    // VulkanMemoryAllocator documentation.
    if (strstr(pMessage, "Mapping an image with layout")
        && strstr(pMessage, "can result in undefined behavior if this memory is used by the device"))
    {
        return true;
    }

    // In certain cases allocateDescriptorSet() will attempt to allocate from a
    // pool that does not have enough descriptors of a certain type. This makes
    // the validation layer shout. However, this is not an error since we will
    // then move on to another pool. If there is a real error, a qWarning
    // message is shown by allocateDescriptorSet(), so the validation warning
    // does not have any value and is just noise.
    if (strstr(pMessage, "VUID-VkDescriptorSetAllocateInfo-descriptorPool-00307"))
        return true;

    return false;
}

bool QRhiVulkan::create(QRhi::Flags flags)
{
    Q_UNUSED(flags);
    Q_ASSERT(inst);

    if (!inst->isValid()) {
        qWarning("Vulkan instance is not valid");
        return false;
    }

    globalVulkanInstance = inst; // assume this will not change during the lifetime of the entire application

    f = inst->functions();

    QVector<VkQueueFamilyProperties> queueFamilyProps;
    auto queryQueueFamilyProps = [this, &queueFamilyProps] {
        uint32_t queueCount = 0;
        f->vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueCount, nullptr);
        queueFamilyProps.resize(int(queueCount));
        f->vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueCount, queueFamilyProps.data());
    };

    if (!importedDevice) {
        uint32_t physDevCount = 0;
        f->vkEnumeratePhysicalDevices(inst->vkInstance(), &physDevCount, nullptr);
        if (!physDevCount) {
            qWarning("No physical devices");
            return false;
        }
        QVarLengthArray<VkPhysicalDevice, 4> physDevs(physDevCount);
        VkResult err = f->vkEnumeratePhysicalDevices(inst->vkInstance(), &physDevCount, physDevs.data());
        if (err != VK_SUCCESS || !physDevCount) {
            qWarning("Failed to enumerate physical devices: %d", err);
            return false;
        }

        int physDevIndex = -1;
        int requestedPhysDevIndex = -1;
        if (qEnvironmentVariableIsSet("QT_VK_PHYSICAL_DEVICE_INDEX"))
            requestedPhysDevIndex = qEnvironmentVariableIntValue("QT_VK_PHYSICAL_DEVICE_INDEX");

        if (requestedPhysDevIndex < 0 && flags.testFlag(QRhi::PreferSoftwareRenderer)) {
            for (int i = 0; i < int(physDevCount); ++i) {
                f->vkGetPhysicalDeviceProperties(physDevs[i], &physDevProperties);
                if (physDevProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) {
                    requestedPhysDevIndex = i;
                    break;
                }
            }
        }

        for (int i = 0; i < int(physDevCount); ++i) {
            f->vkGetPhysicalDeviceProperties(physDevs[i], &physDevProperties);
            qCDebug(QRHI_LOG_INFO, "Physical device %d: '%s' %d.%d.%d (api %d.%d.%d vendor 0x%X device 0x%X type %d)",
                    i,
                    physDevProperties.deviceName,
                    VK_VERSION_MAJOR(physDevProperties.driverVersion),
                    VK_VERSION_MINOR(physDevProperties.driverVersion),
                    VK_VERSION_PATCH(physDevProperties.driverVersion),
                    VK_VERSION_MAJOR(physDevProperties.apiVersion),
                    VK_VERSION_MINOR(physDevProperties.apiVersion),
                    VK_VERSION_PATCH(physDevProperties.apiVersion),
                    physDevProperties.vendorID,
                    physDevProperties.deviceID,
                    physDevProperties.deviceType);
            if (physDevIndex < 0 && (requestedPhysDevIndex < 0 || requestedPhysDevIndex == int(i))) {
                physDevIndex = i;
                qCDebug(QRHI_LOG_INFO, "    using this physical device");
            }
        }

        if (physDevIndex < 0) {
            qWarning("No matching physical device");
            return false;
        }
        physDev = physDevs[physDevIndex];

        queryQueueFamilyProps();

        gfxQueue = VK_NULL_HANDLE;

        // We only support combined graphics+present queues. When it comes to
        // compute, only combined graphics+compute queue is used, compute gets
        // disabled otherwise.
        gfxQueueFamilyIdx = -1;
        int computelessGfxQueueCandidateIdx = -1;
        for (int i = 0; i < queueFamilyProps.count(); ++i) {
            qCDebug(QRHI_LOG_INFO, "queue family %d: flags=0x%x count=%d",
                    i, queueFamilyProps[i].queueFlags, queueFamilyProps[i].queueCount);
            if (gfxQueueFamilyIdx == -1
                    && (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    && (!maybeWindow || inst->supportsPresent(physDev, uint32_t(i), maybeWindow)))
            {
                if (queueFamilyProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                    gfxQueueFamilyIdx = i;
                else if (computelessGfxQueueCandidateIdx == -1)
                    computelessGfxQueueCandidateIdx = i;
            }
        }
        if (gfxQueueFamilyIdx == -1) {
            if (computelessGfxQueueCandidateIdx != -1) {
                gfxQueueFamilyIdx = computelessGfxQueueCandidateIdx;
            } else {
                qWarning("No graphics (or no graphics+present) queue family found");
                return false;
            }
        }

        VkDeviceQueueCreateInfo queueInfo[2];
        const float prio[] = { 0 };
        memset(queueInfo, 0, sizeof(queueInfo));
        queueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo[0].queueFamilyIndex = uint32_t(gfxQueueFamilyIdx);
        queueInfo[0].queueCount = 1;
        queueInfo[0].pQueuePriorities = prio;

        QVector<const char *> devLayers;
        if (inst->layers().contains("VK_LAYER_LUNARG_standard_validation"))
            devLayers.append("VK_LAYER_LUNARG_standard_validation");

        QVulkanInfoVector<QVulkanExtension> devExts;
        uint32_t devExtCount = 0;
        f->vkEnumerateDeviceExtensionProperties(physDev, nullptr, &devExtCount, nullptr);
        if (devExtCount) {
            QVector<VkExtensionProperties> extProps(devExtCount);
            f->vkEnumerateDeviceExtensionProperties(physDev, nullptr, &devExtCount, extProps.data());
            for (const VkExtensionProperties &p : qAsConst(extProps))
                devExts.append({ p.extensionName, p.specVersion });
        }
        qCDebug(QRHI_LOG_INFO, "%d device extensions available", devExts.count());

        QVector<const char *> requestedDevExts;
        requestedDevExts.append("VK_KHR_swapchain");

        debugMarkersAvailable = false;
        if (devExts.contains(VK_EXT_DEBUG_MARKER_EXTENSION_NAME)) {
            requestedDevExts.append(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
            debugMarkersAvailable = true;
        }

        vertexAttribDivisorAvailable = false;
        if (devExts.contains(VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME)) {
            if (inst->extensions().contains(QByteArrayLiteral("VK_KHR_get_physical_device_properties2"))) {
                requestedDevExts.append(VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME);
                vertexAttribDivisorAvailable = true;
            }
        }

        for (const QByteArray &ext : requestedDeviceExtensions) {
            if (!ext.isEmpty()) {
                if (devExts.contains(ext))
                    requestedDevExts.append(ext.constData());
                else
                    qWarning("Device extension %s is not supported", ext.constData());
            }
        }

        QByteArrayList envExtList = qgetenv("QT_VULKAN_DEVICE_EXTENSIONS").split(';');
        for (const QByteArray &ext : envExtList) {
            if (!ext.isEmpty() && !requestedDevExts.contains(ext)) {
                if (devExts.contains(ext))
                    requestedDevExts.append(ext.constData());
                else
                    qWarning("Device extension %s is not supported", ext.constData());
            }
        }

        if (QRHI_LOG_INFO().isEnabled(QtDebugMsg)) {
            qCDebug(QRHI_LOG_INFO, "Enabling device extensions:");
            for (const char *ext : requestedDevExts)
                qCDebug(QRHI_LOG_INFO, "  %s", ext);
        }

        VkDeviceCreateInfo devInfo;
        memset(&devInfo, 0, sizeof(devInfo));
        devInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        devInfo.queueCreateInfoCount = 1;
        devInfo.pQueueCreateInfos = queueInfo;
        devInfo.enabledLayerCount = uint32_t(devLayers.count());
        devInfo.ppEnabledLayerNames = devLayers.constData();
        devInfo.enabledExtensionCount = uint32_t(requestedDevExts.count());
        devInfo.ppEnabledExtensionNames = requestedDevExts.constData();

        err = f->vkCreateDevice(physDev, &devInfo, nullptr, &dev);
        if (err != VK_SUCCESS) {
            qWarning("Failed to create device: %d", err);
            return false;
        }
    }

    df = inst->deviceFunctions(dev);

    if (!importedCmdPool) {
        VkCommandPoolCreateInfo poolInfo;
        memset(&poolInfo, 0, sizeof(poolInfo));
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = uint32_t(gfxQueueFamilyIdx);
        VkResult err = df->vkCreateCommandPool(dev, &poolInfo, nullptr, &cmdPool);
        if (err != VK_SUCCESS) {
            qWarning("Failed to create command pool: %d", err);
            return false;
        }
    }

    if (gfxQueueFamilyIdx != -1) {
        if (!gfxQueue)
            df->vkGetDeviceQueue(dev, uint32_t(gfxQueueFamilyIdx), 0, &gfxQueue);

        if (queueFamilyProps.isEmpty())
            queryQueueFamilyProps();

        hasCompute = (queueFamilyProps[gfxQueueFamilyIdx].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
        timestampValidBits = queueFamilyProps[gfxQueueFamilyIdx].timestampValidBits;
    }

    f->vkGetPhysicalDeviceProperties(physDev, &physDevProperties);
    ubufAlign = physDevProperties.limits.minUniformBufferOffsetAlignment;
    // helps little with an optimal offset of 1 (on some drivers) when the spec
    // elsewhere states that the minimum bufferOffset is 4...
    texbufAlign = qMax<VkDeviceSize>(4, physDevProperties.limits.optimalBufferCopyOffsetAlignment);

    f->vkGetPhysicalDeviceFeatures(physDev, &physDevFeatures);
    hasWideLines = physDevFeatures.wideLines;

    if (!importedAllocator) {
        VmaVulkanFunctions afuncs;
        afuncs.vkGetPhysicalDeviceProperties = wrap_vkGetPhysicalDeviceProperties;
        afuncs.vkGetPhysicalDeviceMemoryProperties = wrap_vkGetPhysicalDeviceMemoryProperties;
        afuncs.vkAllocateMemory = wrap_vkAllocateMemory;
        afuncs.vkFreeMemory = wrap_vkFreeMemory;
        afuncs.vkMapMemory = wrap_vkMapMemory;
        afuncs.vkUnmapMemory = wrap_vkUnmapMemory;
        afuncs.vkFlushMappedMemoryRanges = wrap_vkFlushMappedMemoryRanges;
        afuncs.vkInvalidateMappedMemoryRanges = wrap_vkInvalidateMappedMemoryRanges;
        afuncs.vkBindBufferMemory = wrap_vkBindBufferMemory;
        afuncs.vkBindImageMemory = wrap_vkBindImageMemory;
        afuncs.vkGetBufferMemoryRequirements = wrap_vkGetBufferMemoryRequirements;
        afuncs.vkGetImageMemoryRequirements = wrap_vkGetImageMemoryRequirements;
        afuncs.vkCreateBuffer = wrap_vkCreateBuffer;
        afuncs.vkDestroyBuffer = wrap_vkDestroyBuffer;
        afuncs.vkCreateImage = wrap_vkCreateImage;
        afuncs.vkDestroyImage = wrap_vkDestroyImage;

        VmaAllocatorCreateInfo allocatorInfo;
        memset(&allocatorInfo, 0, sizeof(allocatorInfo));
        // A QRhi is supposed to be used from one single thread only. Disable
        // the allocator's own mutexes. This gives a performance boost.
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
        allocatorInfo.physicalDevice = physDev;
        allocatorInfo.device = dev;
        allocatorInfo.pVulkanFunctions = &afuncs;
        VmaAllocator vmaallocator;
        VkResult err = vmaCreateAllocator(&allocatorInfo, &vmaallocator);
        if (err != VK_SUCCESS) {
            qWarning("Failed to create allocator: %d", err);
            return false;
        }
        allocator = vmaallocator;
    }

    inst->installDebugOutputFilter(qvk_debug_filter);

    VkDescriptorPool pool;
    VkResult err = createDescriptorPool(&pool);
    if (err == VK_SUCCESS)
        descriptorPools.append(pool);
    else
        qWarning("Failed to create initial descriptor pool: %d", err);

    VkQueryPoolCreateInfo timestampQueryPoolInfo;
    memset(&timestampQueryPoolInfo, 0, sizeof(timestampQueryPoolInfo));
    timestampQueryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    timestampQueryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    timestampQueryPoolInfo.queryCount = QVK_MAX_ACTIVE_TIMESTAMP_PAIRS * 2;
    err = df->vkCreateQueryPool(dev, &timestampQueryPoolInfo, nullptr, &timestampQueryPool);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create timestamp query pool: %d", err);
        return false;
    }
    timestampQueryPoolMap.resize(QVK_MAX_ACTIVE_TIMESTAMP_PAIRS); // 1 bit per pair
    timestampQueryPoolMap.fill(false);

    if (debugMarkersAvailable) {
        vkCmdDebugMarkerBegin = reinterpret_cast<PFN_vkCmdDebugMarkerBeginEXT>(f->vkGetDeviceProcAddr(dev, "vkCmdDebugMarkerBeginEXT"));
        vkCmdDebugMarkerEnd = reinterpret_cast<PFN_vkCmdDebugMarkerEndEXT>(f->vkGetDeviceProcAddr(dev, "vkCmdDebugMarkerEndEXT"));
        vkCmdDebugMarkerInsert = reinterpret_cast<PFN_vkCmdDebugMarkerInsertEXT>(f->vkGetDeviceProcAddr(dev, "vkCmdDebugMarkerInsertEXT"));
        vkDebugMarkerSetObjectName = reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(f->vkGetDeviceProcAddr(dev, "vkDebugMarkerSetObjectNameEXT"));
    }

    deviceLost = false;

    nativeHandlesStruct.physDev = physDev;
    nativeHandlesStruct.dev = dev;
    nativeHandlesStruct.gfxQueueFamilyIdx = gfxQueueFamilyIdx;
    nativeHandlesStruct.gfxQueue = gfxQueue;
    nativeHandlesStruct.cmdPool = cmdPool;
    nativeHandlesStruct.vmemAllocator = allocator;

    return true;
}

void QRhiVulkan::destroy()
{
    if (!df)
        return;

    if (!deviceLost)
        df->vkDeviceWaitIdle(dev);

    executeDeferredReleases(true);
    finishActiveReadbacks(true);

    if (ofr.cmdFence) {
        df->vkDestroyFence(dev, ofr.cmdFence, nullptr);
        ofr.cmdFence = VK_NULL_HANDLE;
    }

    if (ofr.cbWrapper.cb) {
        df->vkFreeCommandBuffers(dev, cmdPool, 1, &ofr.cbWrapper.cb);
        ofr.cbWrapper.cb = VK_NULL_HANDLE;
    }

    if (pipelineCache) {
        df->vkDestroyPipelineCache(dev, pipelineCache, nullptr);
        pipelineCache = VK_NULL_HANDLE;
    }

    for (const DescriptorPoolData &pool : descriptorPools)
        df->vkDestroyDescriptorPool(dev, pool.pool, nullptr);

    descriptorPools.clear();

    if (timestampQueryPool) {
        df->vkDestroyQueryPool(dev, timestampQueryPool, nullptr);
        timestampQueryPool = VK_NULL_HANDLE;
    }

    if (!importedAllocator && allocator) {
        vmaDestroyAllocator(toVmaAllocator(allocator));
        allocator = nullptr;
    }

    if (!importedCmdPool && cmdPool) {
        df->vkDestroyCommandPool(dev, cmdPool, nullptr);
        cmdPool = VK_NULL_HANDLE;
    }

    if (!importedDevice && dev) {
        df->vkDestroyDevice(dev, nullptr);
        inst->resetDeviceFunctions(dev);
        dev = VK_NULL_HANDLE;
    }

    f = nullptr;
    df = nullptr;
}

VkResult QRhiVulkan::createDescriptorPool(VkDescriptorPool *pool)
{
    VkDescriptorPoolSize descPoolSizes[] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, QVK_UNIFORM_BUFFERS_PER_POOL },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, QVK_UNIFORM_BUFFERS_PER_POOL },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, QVK_COMBINED_IMAGE_SAMPLERS_PER_POOL },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, QVK_STORAGE_BUFFERS_PER_POOL },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, QVK_STORAGE_IMAGES_PER_POOL }
    };
    VkDescriptorPoolCreateInfo descPoolInfo;
    memset(&descPoolInfo, 0, sizeof(descPoolInfo));
    descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    // Do not enable vkFreeDescriptorSets - sets are never freed on their own
    // (good so no trouble with fragmentation), they just deref their pool
    // which is then reset at some point (or not).
    descPoolInfo.flags = 0;
    descPoolInfo.maxSets = QVK_DESC_SETS_PER_POOL;
    descPoolInfo.poolSizeCount = sizeof(descPoolSizes) / sizeof(descPoolSizes[0]);
    descPoolInfo.pPoolSizes = descPoolSizes;
    return df->vkCreateDescriptorPool(dev, &descPoolInfo, nullptr, pool);
}

bool QRhiVulkan::allocateDescriptorSet(VkDescriptorSetAllocateInfo *allocInfo, VkDescriptorSet *result, int *resultPoolIndex)
{
    auto tryAllocate = [this, allocInfo, result](int poolIndex) {
        allocInfo->descriptorPool = descriptorPools[poolIndex].pool;
        VkResult r = df->vkAllocateDescriptorSets(dev, allocInfo, result);
        if (r == VK_SUCCESS)
            descriptorPools[poolIndex].refCount += 1;
        return r;
    };

    int lastPoolIdx = descriptorPools.count() - 1;
    for (int i = lastPoolIdx; i >= 0; --i) {
        if (descriptorPools[i].refCount == 0) {
            df->vkResetDescriptorPool(dev, descriptorPools[i].pool, 0);
            descriptorPools[i].allocedDescSets = 0;
        }
        if (descriptorPools[i].allocedDescSets + int(allocInfo->descriptorSetCount) <= QVK_DESC_SETS_PER_POOL) {
            VkResult err = tryAllocate(i);
            if (err == VK_SUCCESS) {
                descriptorPools[i].allocedDescSets += allocInfo->descriptorSetCount;
                *resultPoolIndex = i;
                return true;
            }
        }
    }

    VkDescriptorPool newPool;
    VkResult poolErr = createDescriptorPool(&newPool);
    if (poolErr == VK_SUCCESS) {
        descriptorPools.append(newPool);
        lastPoolIdx = descriptorPools.count() - 1;
        VkResult err = tryAllocate(lastPoolIdx);
        if (err != VK_SUCCESS) {
            qWarning("Failed to allocate descriptor set from new pool too, giving up: %d", err);
            return false;
        }
        descriptorPools[lastPoolIdx].allocedDescSets += allocInfo->descriptorSetCount;
        *resultPoolIndex = lastPoolIdx;
        return true;
    } else {
        qWarning("Failed to allocate new descriptor pool: %d", poolErr);
        return false;
    }
}

static inline VkFormat toVkTextureFormat(QRhiTexture::Format format, QRhiTexture::Flags flags)
{
    const bool srgb = flags.testFlag(QRhiTexture::sRGB);
    switch (format) {
    case QRhiTexture::RGBA8:
        return srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
    case QRhiTexture::BGRA8:
        return srgb ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_B8G8R8A8_UNORM;
    case QRhiTexture::R8:
        return srgb ? VK_FORMAT_R8_SRGB : VK_FORMAT_R8_UNORM;
    case QRhiTexture::R16:
        return VK_FORMAT_R16_UNORM;
    case QRhiTexture::RED_OR_ALPHA8:
        return VK_FORMAT_R8_UNORM;

    case QRhiTexture::RGBA16F:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case QRhiTexture::RGBA32F:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case QRhiTexture::R16F:
        return VK_FORMAT_R16_SFLOAT;
    case QRhiTexture::R32F:
        return VK_FORMAT_R32_SFLOAT;

    case QRhiTexture::D16:
        return VK_FORMAT_D16_UNORM;
    case QRhiTexture::D32F:
        return VK_FORMAT_D32_SFLOAT;

    case QRhiTexture::BC1:
        return srgb ? VK_FORMAT_BC1_RGB_SRGB_BLOCK : VK_FORMAT_BC1_RGB_UNORM_BLOCK;
    case QRhiTexture::BC2:
        return srgb ? VK_FORMAT_BC2_SRGB_BLOCK : VK_FORMAT_BC2_UNORM_BLOCK;
    case QRhiTexture::BC3:
        return srgb ? VK_FORMAT_BC3_SRGB_BLOCK : VK_FORMAT_BC3_UNORM_BLOCK;
    case QRhiTexture::BC4:
        return VK_FORMAT_BC4_UNORM_BLOCK;
    case QRhiTexture::BC5:
        return VK_FORMAT_BC5_UNORM_BLOCK;
    case QRhiTexture::BC6H:
        return VK_FORMAT_BC6H_UFLOAT_BLOCK;
    case QRhiTexture::BC7:
        return srgb ? VK_FORMAT_BC7_SRGB_BLOCK : VK_FORMAT_BC7_UNORM_BLOCK;

    case QRhiTexture::ETC2_RGB8:
        return srgb ? VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK : VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
    case QRhiTexture::ETC2_RGB8A1:
        return srgb ? VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK : VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
    case QRhiTexture::ETC2_RGBA8:
        return srgb ? VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK : VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;

    case QRhiTexture::ASTC_4x4:
        return srgb ? VK_FORMAT_ASTC_4x4_SRGB_BLOCK : VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
    case QRhiTexture::ASTC_5x4:
        return srgb ? VK_FORMAT_ASTC_5x4_SRGB_BLOCK : VK_FORMAT_ASTC_5x4_UNORM_BLOCK;
    case QRhiTexture::ASTC_5x5:
        return srgb ? VK_FORMAT_ASTC_5x5_SRGB_BLOCK : VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
    case QRhiTexture::ASTC_6x5:
        return srgb ? VK_FORMAT_ASTC_6x5_SRGB_BLOCK : VK_FORMAT_ASTC_6x5_UNORM_BLOCK;
    case QRhiTexture::ASTC_6x6:
        return srgb ? VK_FORMAT_ASTC_6x6_SRGB_BLOCK : VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
    case QRhiTexture::ASTC_8x5:
        return srgb ? VK_FORMAT_ASTC_8x5_SRGB_BLOCK : VK_FORMAT_ASTC_8x5_UNORM_BLOCK;
    case QRhiTexture::ASTC_8x6:
        return srgb ? VK_FORMAT_ASTC_8x6_SRGB_BLOCK : VK_FORMAT_ASTC_8x6_UNORM_BLOCK;
    case QRhiTexture::ASTC_8x8:
        return srgb ? VK_FORMAT_ASTC_8x8_SRGB_BLOCK : VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
    case QRhiTexture::ASTC_10x5:
        return srgb ? VK_FORMAT_ASTC_10x5_SRGB_BLOCK : VK_FORMAT_ASTC_10x5_UNORM_BLOCK;
    case QRhiTexture::ASTC_10x6:
        return srgb ? VK_FORMAT_ASTC_10x6_SRGB_BLOCK : VK_FORMAT_ASTC_10x6_UNORM_BLOCK;
    case QRhiTexture::ASTC_10x8:
        return srgb ? VK_FORMAT_ASTC_10x8_SRGB_BLOCK : VK_FORMAT_ASTC_10x8_UNORM_BLOCK;
    case QRhiTexture::ASTC_10x10:
        return srgb ? VK_FORMAT_ASTC_10x10_SRGB_BLOCK : VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
    case QRhiTexture::ASTC_12x10:
        return srgb ? VK_FORMAT_ASTC_12x10_SRGB_BLOCK : VK_FORMAT_ASTC_12x10_UNORM_BLOCK;
    case QRhiTexture::ASTC_12x12:
        return srgb ? VK_FORMAT_ASTC_12x12_SRGB_BLOCK : VK_FORMAT_ASTC_12x12_UNORM_BLOCK;

    default:
        Q_UNREACHABLE();
        return VK_FORMAT_R8G8B8A8_UNORM;
    }
}

static inline QRhiTexture::Format colorTextureFormatFromVkFormat(VkFormat format, QRhiTexture::Flags *flags)
{
    switch (format) {
    case VK_FORMAT_R8G8B8A8_UNORM:
        return QRhiTexture::RGBA8;
    case VK_FORMAT_R8G8B8A8_SRGB:
        if (flags)
            (*flags) |= QRhiTexture::sRGB;
        return QRhiTexture::RGBA8;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return QRhiTexture::BGRA8;
    case VK_FORMAT_B8G8R8A8_SRGB:
        if (flags)
            (*flags) |= QRhiTexture::sRGB;
        return QRhiTexture::BGRA8;
    case VK_FORMAT_R8_UNORM:
        return QRhiTexture::R8;
    case VK_FORMAT_R8_SRGB:
        if (flags)
            (*flags) |= QRhiTexture::sRGB;
        return QRhiTexture::R8;
    case VK_FORMAT_R16_UNORM:
        return QRhiTexture::R16;
    default: // this cannot assert, must warn and return unknown
        qWarning("VkFormat %d is not a recognized uncompressed color format", format);
        break;
    }
    return QRhiTexture::UnknownFormat;
}

static inline bool isDepthTextureFormat(QRhiTexture::Format format)
{
    switch (format) {
    case QRhiTexture::Format::D16:
    case QRhiTexture::Format::D32F:
        return true;

    default:
        return false;
    }
}

// Transient images ("render buffers") backed by lazily allocated memory are
// managed manually without going through vk_mem_alloc since it does not offer
// any support for such images. This should be ok since in practice there
// should be very few of such images.

uint32_t QRhiVulkan::chooseTransientImageMemType(VkImage img, uint32_t startIndex)
{
    VkPhysicalDeviceMemoryProperties physDevMemProps;
    f->vkGetPhysicalDeviceMemoryProperties(physDev, &physDevMemProps);

    VkMemoryRequirements memReq;
    df->vkGetImageMemoryRequirements(dev, img, &memReq);
    uint32_t memTypeIndex = uint32_t(-1);

    if (memReq.memoryTypeBits) {
        // Find a device local + lazily allocated, or at least device local memtype.
        const VkMemoryType *memType = physDevMemProps.memoryTypes;
        bool foundDevLocal = false;
        for (uint32_t i = startIndex; i < physDevMemProps.memoryTypeCount; ++i) {
            if (memReq.memoryTypeBits & (1 << i)) {
                if (memType[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                    if (!foundDevLocal) {
                        foundDevLocal = true;
                        memTypeIndex = i;
                    }
                    if (memType[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
                        memTypeIndex = i;
                        break;
                    }
                }
            }
        }
    }

    return memTypeIndex;
}

bool QRhiVulkan::createTransientImage(VkFormat format,
                                      const QSize &pixelSize,
                                      VkImageUsageFlags usage,
                                      VkImageAspectFlags aspectMask,
                                      VkSampleCountFlagBits samples,
                                      VkDeviceMemory *mem,
                                      VkImage *images,
                                      VkImageView *views,
                                      int count)
{
    VkMemoryRequirements memReq;
    VkResult err;

    for (int i = 0; i < count; ++i) {
        VkImageCreateInfo imgInfo;
        memset(&imgInfo, 0, sizeof(imgInfo));
        imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imgInfo.imageType = VK_IMAGE_TYPE_2D;
        imgInfo.format = format;
        imgInfo.extent.width = uint32_t(pixelSize.width());
        imgInfo.extent.height = uint32_t(pixelSize.height());
        imgInfo.extent.depth = 1;
        imgInfo.mipLevels = imgInfo.arrayLayers = 1;
        imgInfo.samples = samples;
        imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.usage = usage | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        err = df->vkCreateImage(dev, &imgInfo, nullptr, images + i);
        if (err != VK_SUCCESS) {
            qWarning("Failed to create image: %d", err);
            return false;
        }

        // Assume the reqs are the same since the images are same in every way.
        // Still, call GetImageMemReq for every image, in order to prevent the
        // validation layer from complaining.
        df->vkGetImageMemoryRequirements(dev, images[i], &memReq);
    }

    VkMemoryAllocateInfo memInfo;
    memset(&memInfo, 0, sizeof(memInfo));
    memInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memInfo.allocationSize = aligned(memReq.size, memReq.alignment) * VkDeviceSize(count);

    uint32_t startIndex = 0;
    do {
        memInfo.memoryTypeIndex = chooseTransientImageMemType(images[0], startIndex);
        if (memInfo.memoryTypeIndex == uint32_t(-1)) {
            qWarning("No suitable memory type found");
            return false;
        }
        startIndex = memInfo.memoryTypeIndex + 1;
        err = df->vkAllocateMemory(dev, &memInfo, nullptr, mem);
        if (err != VK_SUCCESS && err != VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            qWarning("Failed to allocate image memory: %d", err);
            return false;
        }
    } while (err != VK_SUCCESS);

    VkDeviceSize ofs = 0;
    for (int i = 0; i < count; ++i) {
        err = df->vkBindImageMemory(dev, images[i], *mem, ofs);
        if (err != VK_SUCCESS) {
            qWarning("Failed to bind image memory: %d", err);
            return false;
        }
        ofs += aligned(memReq.size, memReq.alignment);

        VkImageViewCreateInfo imgViewInfo;
        memset(&imgViewInfo, 0, sizeof(imgViewInfo));
        imgViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imgViewInfo.image = images[i];
        imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imgViewInfo.format = format;
        imgViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        imgViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        imgViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        imgViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        imgViewInfo.subresourceRange.aspectMask = aspectMask;
        imgViewInfo.subresourceRange.levelCount = imgViewInfo.subresourceRange.layerCount = 1;

        err = df->vkCreateImageView(dev, &imgViewInfo, nullptr, views + i);
        if (err != VK_SUCCESS) {
            qWarning("Failed to create image view: %d", err);
            return false;
        }
    }

    return true;
}

VkFormat QRhiVulkan::optimalDepthStencilFormat()
{
    if (optimalDsFormat != VK_FORMAT_UNDEFINED)
        return optimalDsFormat;

    const VkFormat dsFormatCandidates[] = {
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT
    };
    const int dsFormatCandidateCount = sizeof(dsFormatCandidates) / sizeof(VkFormat);
    int dsFormatIdx = 0;
    while (dsFormatIdx < dsFormatCandidateCount) {
        optimalDsFormat = dsFormatCandidates[dsFormatIdx];
        VkFormatProperties fmtProp;
        f->vkGetPhysicalDeviceFormatProperties(physDev, optimalDsFormat, &fmtProp);
        if (fmtProp.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            break;
        ++dsFormatIdx;
    }
    if (dsFormatIdx == dsFormatCandidateCount)
        qWarning("Failed to find an optimal depth-stencil format");

    return optimalDsFormat;
}

bool QRhiVulkan::createDefaultRenderPass(QVkRenderPassDescriptor *rpD, bool hasDepthStencil, VkSampleCountFlagBits samples, VkFormat colorFormat)
{
    // attachment list layout is color (1), ds (0-1), resolve (0-1)

    VkAttachmentDescription attDesc;
    memset(&attDesc, 0, sizeof(attDesc));
    attDesc.format = colorFormat;
    attDesc.samples = samples;
    attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attDesc.storeOp = samples > VK_SAMPLE_COUNT_1_BIT ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
    attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attDesc.finalLayout = samples > VK_SAMPLE_COUNT_1_BIT ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    rpD->attDescs.append(attDesc);

    rpD->colorRefs.append({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

    if (hasDepthStencil) {
        // clear on load + no store + lazy alloc + transient image should play
        // nicely with tiled GPUs (no physical backing necessary for ds buffer)
        memset(&attDesc, 0, sizeof(attDesc));
        attDesc.format = optimalDepthStencilFormat();
        attDesc.samples = samples;
        attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        rpD->attDescs.append(attDesc);

        rpD->dsRef = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
    }

    if (samples > VK_SAMPLE_COUNT_1_BIT) {
        memset(&attDesc, 0, sizeof(attDesc));
        attDesc.format = colorFormat;
        attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
        attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        rpD->attDescs.append(attDesc);

        rpD->resolveRefs.append({ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
    }

    VkSubpassDescription subpassDesc;
    memset(&subpassDesc, 0, sizeof(subpassDesc));
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = rpD->colorRefs.constData();
    subpassDesc.pDepthStencilAttachment = hasDepthStencil ? &rpD->dsRef : nullptr;

    // Replace the first implicit dep (TOP_OF_PIPE / ALL_COMMANDS) with our own.
    VkSubpassDependency subpassDep;
    memset(&subpassDep, 0, sizeof(subpassDep));
    subpassDep.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDep.dstSubpass = 0;
    subpassDep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDep.srcAccessMask = 0;
    subpassDep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rpInfo;
    memset(&rpInfo, 0, sizeof(rpInfo));
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments = rpD->attDescs.constData();
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpassDesc;
    rpInfo.dependencyCount = 1;
    rpInfo.pDependencies = &subpassDep;

    if (hasDepthStencil)
        rpInfo.attachmentCount += 1;

    if (samples > VK_SAMPLE_COUNT_1_BIT) {
        rpInfo.attachmentCount += 1;
        subpassDesc.pResolveAttachments = rpD->resolveRefs.constData();
    }

    VkResult err = df->vkCreateRenderPass(dev, &rpInfo, nullptr, &rpD->rp);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create renderpass: %d", err);
        return false;
    }

    rpD->hasDepthStencil = hasDepthStencil;

    return true;
}

bool QRhiVulkan::createOffscreenRenderPass(QVkRenderPassDescriptor *rpD,
                                           const QRhiColorAttachment *firstColorAttachment,
                                           const QRhiColorAttachment *lastColorAttachment,
                                           bool preserveColor,
                                           bool preserveDs,
                                           QRhiRenderBuffer *depthStencilBuffer,
                                           QRhiTexture *depthTexture)
{
    // attachment list layout is color (0-8), ds (0-1), resolve (0-8)

    for (auto it = firstColorAttachment; it != lastColorAttachment; ++it) {
        QVkTexture *texD = QRHI_RES(QVkTexture, it->texture());
        QVkRenderBuffer *rbD = QRHI_RES(QVkRenderBuffer, it->renderBuffer());
        Q_ASSERT(texD || rbD);
        const VkFormat vkformat = texD ? texD->vkformat : rbD->vkformat;
        const VkSampleCountFlagBits samples = texD ? texD->samples : rbD->samples;

        VkAttachmentDescription attDesc;
        memset(&attDesc, 0, sizeof(attDesc));
        attDesc.format = vkformat;
        attDesc.samples = samples;
        attDesc.loadOp = preserveColor ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
        attDesc.storeOp = it->resolveTexture() ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
        attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        // this has to interact correctly with activateTextureRenderTarget(), hence leaving in COLOR_ATT
        attDesc.initialLayout = preserveColor ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
        attDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        rpD->attDescs.append(attDesc);

        const VkAttachmentReference ref = { uint32_t(rpD->attDescs.count() - 1), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        rpD->colorRefs.append(ref);
    }

    rpD->hasDepthStencil = depthStencilBuffer || depthTexture;
    if (rpD->hasDepthStencil) {
        const VkFormat dsFormat = depthTexture ? QRHI_RES(QVkTexture, depthTexture)->vkformat
                                               : QRHI_RES(QVkRenderBuffer, depthStencilBuffer)->vkformat;
        const VkSampleCountFlagBits samples = depthTexture ? QRHI_RES(QVkTexture, depthTexture)->samples
                                                           : QRHI_RES(QVkRenderBuffer, depthStencilBuffer)->samples;
        const VkAttachmentLoadOp loadOp = preserveDs ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
        const VkAttachmentStoreOp storeOp = depthTexture ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        VkAttachmentDescription attDesc;
        memset(&attDesc, 0, sizeof(attDesc));
        attDesc.format = dsFormat;
        attDesc.samples = samples;
        attDesc.loadOp = loadOp;
        attDesc.storeOp = storeOp;
        attDesc.stencilLoadOp = loadOp;
        attDesc.stencilStoreOp = storeOp;
        attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        rpD->attDescs.append(attDesc);
    }
    rpD->dsRef = { uint32_t(rpD->attDescs.count() - 1), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    for (auto it = firstColorAttachment; it != lastColorAttachment; ++it) {
        if (it->resolveTexture()) {
            QVkTexture *rtexD = QRHI_RES(QVkTexture, it->resolveTexture());
            if (rtexD->samples > VK_SAMPLE_COUNT_1_BIT)
                qWarning("Resolving into a multisample texture is not supported");

            VkAttachmentDescription attDesc;
            memset(&attDesc, 0, sizeof(attDesc));
            attDesc.format = rtexD->vkformat;
            attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
            attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // ignored
            attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            rpD->attDescs.append(attDesc);

            const VkAttachmentReference ref = { uint32_t(rpD->attDescs.count() - 1), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
            rpD->resolveRefs.append(ref);
        } else {
            const VkAttachmentReference ref = { VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
            rpD->resolveRefs.append(ref);
        }
    }

    VkSubpassDescription subpassDesc;
    memset(&subpassDesc, 0, sizeof(subpassDesc));
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = uint32_t(rpD->colorRefs.count());
    Q_ASSERT(rpD->colorRefs.count() == rpD->resolveRefs.count());
    subpassDesc.pColorAttachments = !rpD->colorRefs.isEmpty() ? rpD->colorRefs.constData() : nullptr;
    subpassDesc.pDepthStencilAttachment = rpD->hasDepthStencil ? &rpD->dsRef : nullptr;
    subpassDesc.pResolveAttachments = !rpD->resolveRefs.isEmpty() ? rpD->resolveRefs.constData() : nullptr;

    VkRenderPassCreateInfo rpInfo;
    memset(&rpInfo, 0, sizeof(rpInfo));
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = uint32_t(rpD->attDescs.count());
    rpInfo.pAttachments = rpD->attDescs.constData();
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpassDesc;
    // don't yet know the correct initial/final access and stage stuff for the
    // implicit deps at this point, so leave it to the resource tracking to
    // generate barriers

    VkResult err = df->vkCreateRenderPass(dev, &rpInfo, nullptr, &rpD->rp);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create renderpass: %d", err);
        return false;
    }

    return true;
}

bool QRhiVulkan::recreateSwapChain(QRhiSwapChain *swapChain)
{
    QVkSwapChain *swapChainD = QRHI_RES(QVkSwapChain, swapChain);
    if (swapChainD->pixelSize.isEmpty()) {
        qWarning("Surface size is 0, cannot create swapchain");
        return false;
    }

    df->vkDeviceWaitIdle(dev);

    if (!vkCreateSwapchainKHR) {
        vkCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(f->vkGetDeviceProcAddr(dev, "vkCreateSwapchainKHR"));
        vkDestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(f->vkGetDeviceProcAddr(dev, "vkDestroySwapchainKHR"));
        vkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(f->vkGetDeviceProcAddr(dev, "vkGetSwapchainImagesKHR"));
        vkAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(f->vkGetDeviceProcAddr(dev, "vkAcquireNextImageKHR"));
        vkQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(f->vkGetDeviceProcAddr(dev, "vkQueuePresentKHR"));
        if (!vkCreateSwapchainKHR || !vkDestroySwapchainKHR || !vkGetSwapchainImagesKHR || !vkAcquireNextImageKHR || !vkQueuePresentKHR) {
            qWarning("Swapchain functions not available");
            return false;
        }
    }

    VkSurfaceCapabilitiesKHR surfaceCaps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDev, swapChainD->surface, &surfaceCaps);
    quint32 reqBufferCount;
    if (swapChainD->m_flags.testFlag(QRhiSwapChain::MinimalBufferCount)) {
        reqBufferCount = qMax<quint32>(2, surfaceCaps.minImageCount);
    } else {
        const quint32 maxBuffers = QVkSwapChain::MAX_BUFFER_COUNT;
        if (surfaceCaps.maxImageCount)
            reqBufferCount = qMax(qMin(surfaceCaps.maxImageCount, maxBuffers), surfaceCaps.minImageCount);
        else
            reqBufferCount = qMax<quint32>(2, surfaceCaps.minImageCount);
    }

    VkSurfaceTransformFlagBitsKHR preTransform =
        (surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
        : surfaceCaps.currentTransform;

    VkCompositeAlphaFlagBitsKHR compositeAlpha =
        (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
        ? VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
        : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    if (swapChainD->m_flags.testFlag(QRhiSwapChain::SurfaceHasPreMulAlpha)
            && (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR))
    {
        compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
    }

    if (swapChainD->m_flags.testFlag(QRhiSwapChain::SurfaceHasNonPreMulAlpha)
            && (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR))
    {
        compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
    }

    VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainD->supportsReadback = (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    if (swapChainD->supportsReadback && swapChainD->m_flags.testFlag(QRhiSwapChain::UsedAsTransferSource))
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (swapChainD->m_flags.testFlag(QRhiSwapChain::NoVSync)) {
        if (swapChainD->supportedPresentationModes.contains(VK_PRESENT_MODE_MAILBOX_KHR))
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        else if (swapChainD->supportedPresentationModes.contains(VK_PRESENT_MODE_IMMEDIATE_KHR))
            presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    // If the surface is different than before, then passing in the old
    // swapchain associated with the old surface can fail the swapchain
    // creation. (for example, Android loses the surface when backgrounding and
    // restoring applications, and it also enforces failing swapchain creation
    // with VK_ERROR_NATIVE_WINDOW_IN_USE_KHR if the old swapchain is provided)
    const bool reuseExisting = swapChainD->sc && swapChainD->lastConnectedSurface == swapChainD->surface;

    qCDebug(QRHI_LOG_INFO, "Creating %s swapchain of %u buffers, size %dx%d, presentation mode %d",
            reuseExisting ? "recycled" : "new",
            reqBufferCount, swapChainD->pixelSize.width(), swapChainD->pixelSize.height(), presentMode);

    VkSwapchainCreateInfoKHR swapChainInfo;
    memset(&swapChainInfo, 0, sizeof(swapChainInfo));
    swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainInfo.surface = swapChainD->surface;
    swapChainInfo.minImageCount = reqBufferCount;
    swapChainInfo.imageFormat = swapChainD->colorFormat;
    swapChainInfo.imageColorSpace = swapChainD->colorSpace;
    swapChainInfo.imageExtent = VkExtent2D { uint32_t(swapChainD->pixelSize.width()), uint32_t(swapChainD->pixelSize.height()) };
    swapChainInfo.imageArrayLayers = 1;
    swapChainInfo.imageUsage = usage;
    swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainInfo.preTransform = preTransform;
    swapChainInfo.compositeAlpha = compositeAlpha;
    swapChainInfo.presentMode = presentMode;
    swapChainInfo.clipped = true;
    swapChainInfo.oldSwapchain = reuseExisting ? swapChainD->sc : VK_NULL_HANDLE;

    VkSwapchainKHR newSwapChain;
    VkResult err = vkCreateSwapchainKHR(dev, &swapChainInfo, nullptr, &newSwapChain);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create swapchain: %d", err);
        return false;
    }

    if (swapChainD->sc)
        releaseSwapChainResources(swapChain);

    swapChainD->sc = newSwapChain;
    swapChainD->lastConnectedSurface = swapChainD->surface;

    quint32 actualSwapChainBufferCount = 0;
    err = vkGetSwapchainImagesKHR(dev, swapChainD->sc, &actualSwapChainBufferCount, nullptr);
    if (err != VK_SUCCESS || actualSwapChainBufferCount == 0) {
        qWarning("Failed to get swapchain images: %d", err);
        return false;
    }

    if (actualSwapChainBufferCount > QVkSwapChain::MAX_BUFFER_COUNT) {
        qWarning("Too many swapchain buffers (%u)", actualSwapChainBufferCount);
        return false;
    }
    if (actualSwapChainBufferCount != reqBufferCount)
        qCDebug(QRHI_LOG_INFO, "Actual swapchain buffer count is %u", actualSwapChainBufferCount);
    swapChainD->bufferCount = int(actualSwapChainBufferCount);

    VkImage swapChainImages[QVkSwapChain::MAX_BUFFER_COUNT];
    err = vkGetSwapchainImagesKHR(dev, swapChainD->sc, &actualSwapChainBufferCount, swapChainImages);
    if (err != VK_SUCCESS) {
        qWarning("Failed to get swapchain images: %d", err);
        return false;
    }

    VkImage msaaImages[QVkSwapChain::MAX_BUFFER_COUNT];
    VkImageView msaaViews[QVkSwapChain::MAX_BUFFER_COUNT];
    if (swapChainD->samples > VK_SAMPLE_COUNT_1_BIT) {
        if (!createTransientImage(swapChainD->colorFormat,
                                  swapChainD->pixelSize,
                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                  swapChainD->samples,
                                  &swapChainD->msaaImageMem,
                                  msaaImages,
                                  msaaViews,
                                  swapChainD->bufferCount))
        {
            qWarning("Failed to create transient image for MSAA color buffer");
            return false;
        }
    }

    VkFenceCreateInfo fenceInfo;
    memset(&fenceInfo, 0, sizeof(fenceInfo));
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < swapChainD->bufferCount; ++i) {
        QVkSwapChain::ImageResources &image(swapChainD->imageRes[i]);
        image.image = swapChainImages[i];
        if (swapChainD->samples > VK_SAMPLE_COUNT_1_BIT) {
            image.msaaImage = msaaImages[i];
            image.msaaImageView = msaaViews[i];
        }

        VkImageViewCreateInfo imgViewInfo;
        memset(&imgViewInfo, 0, sizeof(imgViewInfo));
        imgViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imgViewInfo.image = swapChainImages[i];
        imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imgViewInfo.format = swapChainD->colorFormat;
        imgViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        imgViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        imgViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        imgViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        imgViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imgViewInfo.subresourceRange.levelCount = imgViewInfo.subresourceRange.layerCount = 1;
        err = df->vkCreateImageView(dev, &imgViewInfo, nullptr, &image.imageView);
        if (err != VK_SUCCESS) {
            qWarning("Failed to create swapchain image view %d: %d", i, err);
            return false;
        }

        image.lastUse = QVkSwapChain::ImageResources::ScImageUseNone;
    }

    swapChainD->currentImageIndex = 0;

    VkSemaphoreCreateInfo semInfo;
    memset(&semInfo, 0, sizeof(semInfo));
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i) {
        QVkSwapChain::FrameResources &frame(swapChainD->frameRes[i]);

        frame.imageAcquired = false;
        frame.imageSemWaitable = false;

        df->vkCreateFence(dev, &fenceInfo, nullptr, &frame.imageFence);
        frame.imageFenceWaitable = true; // fence was created in signaled state

        df->vkCreateSemaphore(dev, &semInfo, nullptr, &frame.imageSem);
        df->vkCreateSemaphore(dev, &semInfo, nullptr, &frame.drawSem);

        err = df->vkCreateFence(dev, &fenceInfo, nullptr, &frame.cmdFence);
        if (err != VK_SUCCESS) {
            qWarning("Failed to create command buffer fence: %d", err);
            return false;
        }
        frame.cmdFenceWaitable = true; // fence was created in signaled state
    }

    swapChainD->currentFrameSlot = 0;

    return true;
}

void QRhiVulkan::releaseSwapChainResources(QRhiSwapChain *swapChain)
{
    QVkSwapChain *swapChainD = QRHI_RES(QVkSwapChain, swapChain);

    if (swapChainD->sc == VK_NULL_HANDLE)
        return;

    if (!deviceLost)
        df->vkDeviceWaitIdle(dev);

    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i) {
        QVkSwapChain::FrameResources &frame(swapChainD->frameRes[i]);
        if (frame.cmdFence) {
            if (frame.cmdFenceWaitable)
                df->vkWaitForFences(dev, 1, &frame.cmdFence, VK_TRUE, UINT64_MAX);
            df->vkDestroyFence(dev, frame.cmdFence, nullptr);
            frame.cmdFence = VK_NULL_HANDLE;
            frame.cmdFenceWaitable = false;
        }
        if (frame.imageFence) {
            if (frame.imageFenceWaitable)
                df->vkWaitForFences(dev, 1, &frame.imageFence, VK_TRUE, UINT64_MAX);
            df->vkDestroyFence(dev, frame.imageFence, nullptr);
            frame.imageFence = VK_NULL_HANDLE;
            frame.imageFenceWaitable = false;
        }
        if (frame.imageSem) {
            df->vkDestroySemaphore(dev, frame.imageSem, nullptr);
            frame.imageSem = VK_NULL_HANDLE;
        }
        if (frame.drawSem) {
            df->vkDestroySemaphore(dev, frame.drawSem, nullptr);
            frame.drawSem = VK_NULL_HANDLE;
        }
        if (frame.cmdBuf) {
            df->vkFreeCommandBuffers(dev, cmdPool, 1, &frame.cmdBuf);
            frame.cmdBuf = VK_NULL_HANDLE;
        }
    }

    for (int i = 0; i < swapChainD->bufferCount; ++i) {
        QVkSwapChain::ImageResources &image(swapChainD->imageRes[i]);
        if (image.fb) {
            df->vkDestroyFramebuffer(dev, image.fb, nullptr);
            image.fb = VK_NULL_HANDLE;
        }
        if (image.imageView) {
            df->vkDestroyImageView(dev, image.imageView, nullptr);
            image.imageView = VK_NULL_HANDLE;
        }
        if (image.msaaImageView) {
            df->vkDestroyImageView(dev, image.msaaImageView, nullptr);
            image.msaaImageView = VK_NULL_HANDLE;
        }
        if (image.msaaImage) {
            df->vkDestroyImage(dev, image.msaaImage, nullptr);
            image.msaaImage = VK_NULL_HANDLE;
        }
    }

    if (swapChainD->msaaImageMem) {
        df->vkFreeMemory(dev, swapChainD->msaaImageMem, nullptr);
        swapChainD->msaaImageMem = VK_NULL_HANDLE;
    }

    vkDestroySwapchainKHR(dev, swapChainD->sc, nullptr);
    swapChainD->sc = VK_NULL_HANDLE;

    // NB! surface and similar must remain intact
}

QRhi::FrameOpResult QRhiVulkan::beginFrame(QRhiSwapChain *swapChain, QRhi::BeginFrameFlags flags)
{
    QVkSwapChain *swapChainD = QRHI_RES(QVkSwapChain, swapChain);
    const int frameResIndex = swapChainD->bufferCount > 1 ? swapChainD->currentFrameSlot : 0;
    QVkSwapChain::FrameResources &frame(swapChainD->frameRes[frameResIndex]);
    QRhiProfilerPrivate *rhiP = profilerPrivateOrNull();

    if (!frame.imageAcquired) {
        // Wait if we are too far ahead, i.e. the thread gets throttled based on the presentation rate
        // (note that we are using FIFO mode -> vsync)
        if (frame.imageFenceWaitable) {
            df->vkWaitForFences(dev, 1, &frame.imageFence, VK_TRUE, UINT64_MAX);
            df->vkResetFences(dev, 1, &frame.imageFence);
            frame.imageFenceWaitable = false;
        }

        // move on to next swapchain image
        VkResult err = vkAcquireNextImageKHR(dev, swapChainD->sc, UINT64_MAX,
                                             frame.imageSem, frame.imageFence, &frame.imageIndex);
        if (err == VK_SUCCESS || err == VK_SUBOPTIMAL_KHR) {
            swapChainD->currentImageIndex = frame.imageIndex;
            frame.imageSemWaitable = true;
            frame.imageAcquired = true;
            frame.imageFenceWaitable = true;
        } else if (err == VK_ERROR_OUT_OF_DATE_KHR) {
            return QRhi::FrameOpSwapChainOutOfDate;
        } else {
            if (err == VK_ERROR_DEVICE_LOST) {
                qWarning("Device loss detected in vkAcquireNextImageKHR()");
                deviceLost = true;
                return QRhi::FrameOpDeviceLost;
            }
            qWarning("Failed to acquire next swapchain image: %d", err);
            return QRhi::FrameOpError;
        }
    }

    // Make sure the previous commands for the same image have finished. (note
    // that this is based on the fence from the command buffer submit, nothing
    // to do with the Present)
    //
    // Do this also for any other swapchain's commands with the same frame slot
    // While this reduces concurrency, it keeps resource usage safe: swapchain
    // A starting its frame 0, followed by swapchain B starting its own frame 0
    // will make B wait for A's frame 0 commands, so if a resource is written
    // in B's frame or when B checks for pending resource releases, that won't
    // mess up A's in-flight commands (as they are not in flight anymore).
    waitCommandCompletion(frameResIndex);

    // Now is the time to read the timestamps for the previous frame for this slot.
    if (frame.timestampQueryIndex >= 0) {
        quint64 timestamp[2] = { 0, 0 };
        VkResult err = df->vkGetQueryPoolResults(dev, timestampQueryPool, uint32_t(frame.timestampQueryIndex), 2,
                                                 2 * sizeof(quint64), timestamp, sizeof(quint64),
                                                 VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
        timestampQueryPoolMap.clearBit(frame.timestampQueryIndex / 2);
        frame.timestampQueryIndex = -1;
        if (err == VK_SUCCESS) {
            quint64 mask = 0;
            for (quint64 i = 0; i < timestampValidBits; i += 8)
                mask |= 0xFFULL << i;
            const quint64 ts0 = timestamp[0] & mask;
            const quint64 ts1 = timestamp[1] & mask;
            const float nsecsPerTick = physDevProperties.limits.timestampPeriod;
            if (!qFuzzyIsNull(nsecsPerTick)) {
                const float elapsedMs = float(ts1 - ts0) * nsecsPerTick / 1000000.0f;
                // now we have the gpu time for the previous frame for this slot, report it
                // (does not matter that it is not for this frame)
                QRHI_PROF_F(swapChainFrameGpuTime(swapChain, elapsedMs));
            }
        } else {
            qWarning("Failed to query timestamp: %d", err);
        }
    }

    // build new draw command buffer
    QRhi::FrameOpResult cbres = startPrimaryCommandBuffer(&frame.cmdBuf);
    if (cbres != QRhi::FrameOpSuccess)
        return cbres;

    // when profiling is enabled, pick a free query (pair) from the pool
    int timestampQueryIdx = -1;
    if (profilerPrivateOrNull() && swapChainD->bufferCount > 1) { // no timestamps if not having at least 2 frames in flight
        for (int i = 0; i < timestampQueryPoolMap.count(); ++i) {
            if (!timestampQueryPoolMap.testBit(i)) {
                timestampQueryPoolMap.setBit(i);
                timestampQueryIdx = i * 2;
                break;
            }
        }
    }
    if (timestampQueryIdx >= 0) {
        df->vkCmdResetQueryPool(frame.cmdBuf, timestampQueryPool, uint32_t(timestampQueryIdx), 2);
        // record timestamp at the start of the command buffer
        df->vkCmdWriteTimestamp(frame.cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                timestampQueryPool, uint32_t(timestampQueryIdx));
        frame.timestampQueryIndex = timestampQueryIdx;
    }

    swapChainD->cbWrapper.cb = frame.cmdBuf;
    swapChainD->cbWrapper.useSecondaryCb = flags.testFlag(QRhi::ExternalContentsInPass);

    QVkSwapChain::ImageResources &image(swapChainD->imageRes[swapChainD->currentImageIndex]);
    swapChainD->rtWrapper.d.fb = image.fb;

    currentFrameSlot = int(swapChainD->currentFrameSlot);
    currentSwapChain = swapChainD;
    if (swapChainD->ds)
        swapChainD->ds->lastActiveFrameSlot = currentFrameSlot;

    QRHI_PROF_F(beginSwapChainFrame(swapChain));

    prepareNewFrame(&swapChainD->cbWrapper);

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiVulkan::endFrame(QRhiSwapChain *swapChain, QRhi::EndFrameFlags flags)
{
    QVkSwapChain *swapChainD = QRHI_RES(QVkSwapChain, swapChain);
    Q_ASSERT(currentSwapChain == swapChainD);

    recordPrimaryCommandBuffer(&swapChainD->cbWrapper);

    int frameResIndex = swapChainD->bufferCount > 1 ? swapChainD->currentFrameSlot : 0;
    QVkSwapChain::FrameResources &frame(swapChainD->frameRes[frameResIndex]);
    QVkSwapChain::ImageResources &image(swapChainD->imageRes[swapChainD->currentImageIndex]);

    if (image.lastUse != QVkSwapChain::ImageResources::ScImageUseRender) {
        VkImageMemoryBarrier presTrans;
        memset(&presTrans, 0, sizeof(presTrans));
        presTrans.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        presTrans.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        presTrans.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        presTrans.image = image.image;
        presTrans.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        presTrans.subresourceRange.levelCount = presTrans.subresourceRange.layerCount = 1;

        if (image.lastUse == QVkSwapChain::ImageResources::ScImageUseNone) {
            // was not used at all (no render pass), just transition from undefined to presentable
            presTrans.srcAccessMask = 0;
            presTrans.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            df->vkCmdPipelineBarrier(frame.cmdBuf,
                                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                     0, 0, nullptr, 0, nullptr,
                                     1, &presTrans);
        } else if (image.lastUse == QVkSwapChain::ImageResources::ScImageUseTransferSource) {
            // was used in a readback as transfer source, go back to presentable layout
            presTrans.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            presTrans.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            df->vkCmdPipelineBarrier(frame.cmdBuf,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                     0, 0, nullptr, 0, nullptr,
                                     1, &presTrans);
        }
        image.lastUse = QVkSwapChain::ImageResources::ScImageUseRender;
    }

    // record another timestamp, when enabled
    if (frame.timestampQueryIndex >= 0) {
        df->vkCmdWriteTimestamp(frame.cmdBuf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                timestampQueryPool, uint32_t(frame.timestampQueryIndex + 1));
    }

    // stop recording and submit to the queue
    Q_ASSERT(!frame.cmdFenceWaitable);
    const bool needsPresent = !flags.testFlag(QRhi::SkipPresent);
    QRhi::FrameOpResult submitres = endAndSubmitPrimaryCommandBuffer(frame.cmdBuf,
                                                                     frame.cmdFence,
                                                                     frame.imageSemWaitable ? &frame.imageSem : nullptr,
                                                                     needsPresent ? &frame.drawSem : nullptr);
    if (submitres != QRhi::FrameOpSuccess)
        return submitres;

    frame.imageSemWaitable = false;
    frame.cmdFenceWaitable = true;

    QRhiProfilerPrivate *rhiP = profilerPrivateOrNull();
    // this must be done before the Present
    QRHI_PROF_F(endSwapChainFrame(swapChain, swapChainD->frameCount + 1));

    if (needsPresent) {
        // add the Present to the queue
        VkPresentInfoKHR presInfo;
        memset(&presInfo, 0, sizeof(presInfo));
        presInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presInfo.swapchainCount = 1;
        presInfo.pSwapchains = &swapChainD->sc;
        presInfo.pImageIndices = &swapChainD->currentImageIndex;
        presInfo.waitSemaphoreCount = 1;
        presInfo.pWaitSemaphores = &frame.drawSem; // gfxQueueFamilyIdx == presQueueFamilyIdx ? &frame.drawSem : &frame.presTransSem;

        // Do platform-specific WM notification. F.ex. essential on Wayland in
        // order to circumvent driver frame callbacks
        inst->presentAboutToBeQueued(swapChainD->window);

        VkResult err = vkQueuePresentKHR(gfxQueue, &presInfo);
        if (err != VK_SUCCESS) {
            if (err == VK_ERROR_OUT_OF_DATE_KHR) {
                return QRhi::FrameOpSwapChainOutOfDate;
            } else if (err != VK_SUBOPTIMAL_KHR) {
                if (err == VK_ERROR_DEVICE_LOST) {
                    qWarning("Device loss detected in vkQueuePresentKHR()");
                    deviceLost = true;
                    return QRhi::FrameOpDeviceLost;
                }
                qWarning("Failed to present: %d", err);
                return QRhi::FrameOpError;
            }
        }

        // Do platform-specific WM notification. F.ex. essential on X11 in
        // order to prevent glitches on resizing the window.
        inst->presentQueued(swapChainD->window);

        // mark the current swapchain buffer as unused from our side
        frame.imageAcquired = false;
        // and move on to the next buffer
        swapChainD->currentFrameSlot = (swapChainD->currentFrameSlot + 1) % QVK_FRAMES_IN_FLIGHT;
    }

    swapChainD->frameCount += 1;
    currentSwapChain = nullptr;
    return QRhi::FrameOpSuccess;
}

void QRhiVulkan::prepareNewFrame(QRhiCommandBuffer *cb)
{
    // Now is the time to do things for frame N-F, where N is the current one,
    // F is QVK_FRAMES_IN_FLIGHT, because only here it is guaranteed that that
    // frame has completed on the GPU (due to the fence wait in beginFrame). To
    // decide if something is safe to handle now a simple "lastActiveFrameSlot
    // == currentFrameSlot" is sufficient (remember that e.g. with F==2
    // currentFrameSlot goes 0, 1, 0, 1, 0, ...)
    //
    // With multiple swapchains on the same QRhi things get more convoluted
    // (and currentFrameSlot strictly alternating is not true anymore) but
    // beginNonWrapperFrame() solves that by blocking as necessary so the rest
    // here is safe regardless.

    executeDeferredReleases();

    QRHI_RES(QVkCommandBuffer, cb)->resetState();

    finishActiveReadbacks(); // last, in case the readback-completed callback issues rhi calls
}

QRhi::FrameOpResult QRhiVulkan::startPrimaryCommandBuffer(VkCommandBuffer *cb)
{
    if (*cb) {
        df->vkFreeCommandBuffers(dev, cmdPool, 1, cb);
        *cb = VK_NULL_HANDLE;
    }

    VkCommandBufferAllocateInfo cmdBufInfo;
    memset(&cmdBufInfo, 0, sizeof(cmdBufInfo));
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufInfo.commandPool = cmdPool;
    cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufInfo.commandBufferCount = 1;

    VkResult err = df->vkAllocateCommandBuffers(dev, &cmdBufInfo, cb);
    if (err != VK_SUCCESS) {
        if (err == VK_ERROR_DEVICE_LOST) {
            qWarning("Device loss detected in vkAllocateCommandBuffers()");
            deviceLost = true;
            return QRhi::FrameOpDeviceLost;
        }
        qWarning("Failed to allocate frame command buffer: %d", err);
        return QRhi::FrameOpError;
    }

    VkCommandBufferBeginInfo cmdBufBeginInfo;
    memset(&cmdBufBeginInfo, 0, sizeof(cmdBufBeginInfo));
    cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    err = df->vkBeginCommandBuffer(*cb, &cmdBufBeginInfo);
    if (err != VK_SUCCESS) {
        if (err == VK_ERROR_DEVICE_LOST) {
            qWarning("Device loss detected in vkBeginCommandBuffer()");
            deviceLost = true;
            return QRhi::FrameOpDeviceLost;
        }
        qWarning("Failed to begin frame command buffer: %d", err);
        return QRhi::FrameOpError;
    }

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiVulkan::endAndSubmitPrimaryCommandBuffer(VkCommandBuffer cb, VkFence cmdFence,
                                                                 VkSemaphore *waitSem, VkSemaphore *signalSem)
{
    VkResult err = df->vkEndCommandBuffer(cb);
    if (err != VK_SUCCESS) {
        if (err == VK_ERROR_DEVICE_LOST) {
            qWarning("Device loss detected in vkEndCommandBuffer()");
            deviceLost = true;
            return QRhi::FrameOpDeviceLost;
        }
        qWarning("Failed to end frame command buffer: %d", err);
        return QRhi::FrameOpError;
    }

    VkSubmitInfo submitInfo;
    memset(&submitInfo, 0, sizeof(submitInfo));
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cb;
    if (waitSem) {
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSem;
    }
    if (signalSem) {
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSem;
    }
    VkPipelineStageFlags psf = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &psf;

    err = df->vkQueueSubmit(gfxQueue, 1, &submitInfo, cmdFence);
    if (err != VK_SUCCESS) {
        if (err == VK_ERROR_DEVICE_LOST) {
            qWarning("Device loss detected in vkQueueSubmit()");
            deviceLost = true;
            return QRhi::FrameOpDeviceLost;
        }
        qWarning("Failed to submit to graphics queue: %d", err);
        return QRhi::FrameOpError;
    }

    return QRhi::FrameOpSuccess;
}

void QRhiVulkan::waitCommandCompletion(int frameSlot)
{
    for (QVkSwapChain *sc : qAsConst(swapchains)) {
        const int frameResIndex = sc->bufferCount > 1 ? frameSlot : 0;
        QVkSwapChain::FrameResources &frame(sc->frameRes[frameResIndex]);
        if (frame.cmdFenceWaitable) {
            df->vkWaitForFences(dev, 1, &frame.cmdFence, VK_TRUE, UINT64_MAX);
            df->vkResetFences(dev, 1, &frame.cmdFence);
            frame.cmdFenceWaitable = false;
        }
    }
}

QRhi::FrameOpResult QRhiVulkan::beginOffscreenFrame(QRhiCommandBuffer **cb, QRhi::BeginFrameFlags flags)
{
    QRhi::FrameOpResult cbres = startPrimaryCommandBuffer(&ofr.cbWrapper.cb);
    if (cbres != QRhi::FrameOpSuccess)
        return cbres;

    // Switch to the next slot manually. Swapchains do not know about this
    // which is good. So for example a - unusual but possible - onscreen,
    // onscreen, offscreen, onscreen, onscreen, onscreen sequence of
    // begin/endFrame leads to 0, 1, 0, 0, 1, 0. This works because the
    // offscreen frame is synchronous in the sense that we wait for execution
    // to complete in endFrame, and so no resources used in that frame are busy
    // anymore in the next frame.
    currentFrameSlot = (currentFrameSlot + 1) % QVK_FRAMES_IN_FLIGHT;
    // except that this gets complicated with multiple swapchains so make sure
    // any pending commands have finished for the frame slot we are going to use
    if (swapchains.count() > 1)
        waitCommandCompletion(currentFrameSlot);

    ofr.cbWrapper.useSecondaryCb = flags.testFlag(QRhi::ExternalContentsInPass);

    prepareNewFrame(&ofr.cbWrapper);
    ofr.active = true;

    *cb = &ofr.cbWrapper;
    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiVulkan::endOffscreenFrame(QRhi::EndFrameFlags flags)
{
    Q_UNUSED(flags);
    Q_ASSERT(ofr.active);
    ofr.active = false;

    recordPrimaryCommandBuffer(&ofr.cbWrapper);

    if (!ofr.cmdFence) {
        VkFenceCreateInfo fenceInfo;
        memset(&fenceInfo, 0, sizeof(fenceInfo));
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkResult err = df->vkCreateFence(dev, &fenceInfo, nullptr, &ofr.cmdFence);
        if (err != VK_SUCCESS) {
            qWarning("Failed to create command buffer fence: %d", err);
            return QRhi::FrameOpError;
        }
    }

    QRhi::FrameOpResult submitres = endAndSubmitPrimaryCommandBuffer(ofr.cbWrapper.cb, ofr.cmdFence, nullptr, nullptr);
    if (submitres != QRhi::FrameOpSuccess)
        return submitres;

    // wait for completion
    df->vkWaitForFences(dev, 1, &ofr.cmdFence, VK_TRUE, UINT64_MAX);
    df->vkResetFences(dev, 1, &ofr.cmdFence);

    // Here we know that executing the host-side reads for this (or any
    // previous) frame is safe since we waited for completion above.
    finishActiveReadbacks(true);

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiVulkan::finish()
{
    QVkSwapChain *swapChainD = nullptr;
    if (inFrame) {
        // There is either a swapchain or an offscreen frame on-going.
        // End command recording and submit what we have.
        VkCommandBuffer cb;
        if (ofr.active) {
            Q_ASSERT(!currentSwapChain);
            Q_ASSERT(ofr.cbWrapper.recordingPass == QVkCommandBuffer::NoPass);
            recordPrimaryCommandBuffer(&ofr.cbWrapper);
            ofr.cbWrapper.resetCommands();
            cb = ofr.cbWrapper.cb;
        } else {
            Q_ASSERT(currentSwapChain);
            Q_ASSERT(currentSwapChain->cbWrapper.recordingPass == QVkCommandBuffer::NoPass);
            swapChainD = currentSwapChain;
            recordPrimaryCommandBuffer(&swapChainD->cbWrapper);
            swapChainD->cbWrapper.resetCommands();
            cb = swapChainD->cbWrapper.cb;
        }
        QRhi::FrameOpResult submitres = endAndSubmitPrimaryCommandBuffer(cb, VK_NULL_HANDLE, nullptr, nullptr);
        if (submitres != QRhi::FrameOpSuccess)
            return submitres;
    }

    df->vkQueueWaitIdle(gfxQueue);

    if (inFrame) {
        // Allocate and begin recording on a new command buffer.
        if (ofr.active)
            startPrimaryCommandBuffer(&ofr.cbWrapper.cb);
        else
            startPrimaryCommandBuffer(&swapChainD->frameRes[swapChainD->currentFrameSlot].cmdBuf);
    }

    executeDeferredReleases(true);
    finishActiveReadbacks(true);

    return QRhi::FrameOpSuccess;
}

static inline QRhiPassResourceTracker::UsageState toPassTrackerUsageState(const QVkBuffer::UsageState &bufUsage)
{
    QRhiPassResourceTracker::UsageState u;
    u.layout = 0; // unused with buffers
    u.access = int(bufUsage.access);
    u.stage = int(bufUsage.stage);
    return u;
}

static inline QRhiPassResourceTracker::UsageState toPassTrackerUsageState(const QVkTexture::UsageState &texUsage)
{
    QRhiPassResourceTracker::UsageState u;
    u.layout = texUsage.layout;
    u.access = int(texUsage.access);
    u.stage = int(texUsage.stage);
    return u;
}

void QRhiVulkan::activateTextureRenderTarget(QVkCommandBuffer *cbD, QVkTextureRenderTarget *rtD)
{
    rtD->lastActiveFrameSlot = currentFrameSlot;
    rtD->d.rp->lastActiveFrameSlot = currentFrameSlot;
    QRhiPassResourceTracker &passResTracker(cbD->passResTrackers[cbD->currentPassResTrackerIndex]);
    for (auto it = rtD->m_desc.cbeginColorAttachments(), itEnd = rtD->m_desc.cendColorAttachments(); it != itEnd; ++it) {
        QVkTexture *texD = QRHI_RES(QVkTexture, it->texture());
        QVkTexture *resolveTexD = QRHI_RES(QVkTexture, it->resolveTexture());
        QVkRenderBuffer *rbD = QRHI_RES(QVkRenderBuffer, it->renderBuffer());
        if (texD) {
            trackedRegisterTexture(&passResTracker, texD,
                                   QRhiPassResourceTracker::TexColorOutput,
                                   QRhiPassResourceTracker::TexColorOutputStage);
            texD->lastActiveFrameSlot = currentFrameSlot;
        } else if (rbD) {
            // Won't register rbD->backingTexture because it cannot be used for
            // anything in a renderpass, its use makes only sense in
            // combination with a resolveTexture.
            rbD->lastActiveFrameSlot = currentFrameSlot;
        }
        if (resolveTexD) {
            trackedRegisterTexture(&passResTracker, resolveTexD,
                                   QRhiPassResourceTracker::TexColorOutput,
                                   QRhiPassResourceTracker::TexColorOutputStage);
            resolveTexD->lastActiveFrameSlot = currentFrameSlot;
        }
    }
    if (rtD->m_desc.depthStencilBuffer())
        QRHI_RES(QVkRenderBuffer, rtD->m_desc.depthStencilBuffer())->lastActiveFrameSlot = currentFrameSlot;
    if (rtD->m_desc.depthTexture()) {
        QVkTexture *depthTexD = QRHI_RES(QVkTexture, rtD->m_desc.depthTexture());
        trackedRegisterTexture(&passResTracker, depthTexD,
                               QRhiPassResourceTracker::TexDepthOutput,
                               QRhiPassResourceTracker::TexDepthOutputStage);
        depthTexD->lastActiveFrameSlot = currentFrameSlot;
    }
}

void QRhiVulkan::resourceUpdate(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::NoPass);

    enqueueResourceUpdates(cbD, resourceUpdates);
}

VkCommandBuffer QRhiVulkan::startSecondaryCommandBuffer(QVkRenderTargetData *rtD)
{
    VkCommandBuffer secondaryCb;

    VkCommandBufferAllocateInfo cmdBufInfo;
    memset(&cmdBufInfo, 0, sizeof(cmdBufInfo));
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufInfo.commandPool = cmdPool;
    cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    cmdBufInfo.commandBufferCount = 1;
    VkResult err = df->vkAllocateCommandBuffers(dev, &cmdBufInfo, &secondaryCb);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create secondary command buffer: %d", err);
        return VK_NULL_HANDLE;
    }

    VkCommandBufferBeginInfo cmdBufBeginInfo;
    memset(&cmdBufBeginInfo, 0, sizeof(cmdBufBeginInfo));
    cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufBeginInfo.flags = rtD ? VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : 0;
    VkCommandBufferInheritanceInfo cmdBufInheritInfo;
    memset(&cmdBufInheritInfo, 0, sizeof(cmdBufInheritInfo));
    cmdBufInheritInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    cmdBufInheritInfo.subpass = 0;
    if (rtD) {
        cmdBufInheritInfo.renderPass = rtD->rp->rp;
        cmdBufInheritInfo.framebuffer = rtD->fb;
    }
    cmdBufBeginInfo.pInheritanceInfo = &cmdBufInheritInfo;

    err = df->vkBeginCommandBuffer(secondaryCb, &cmdBufBeginInfo);
    if (err != VK_SUCCESS) {
        qWarning("Failed to begin secondary command buffer: %d", err);
        df->vkFreeCommandBuffers(dev, cmdPool, 1, &secondaryCb);
        return VK_NULL_HANDLE;
    }

    return secondaryCb;
}

void QRhiVulkan::endAndEnqueueSecondaryCommandBuffer(VkCommandBuffer cb, QVkCommandBuffer *cbD)
{
    VkResult err = df->vkEndCommandBuffer(cb);
    if (err != VK_SUCCESS)
        qWarning("Failed to end secondary command buffer: %d", err);

    QVkCommandBuffer::Command cmd;
    cmd.cmd = QVkCommandBuffer::Command::ExecuteSecondary;
    cmd.args.executeSecondary.cb = cb;
    cbD->commands.append(cmd);

    deferredReleaseSecondaryCommandBuffer(cb);
}

void QRhiVulkan::deferredReleaseSecondaryCommandBuffer(VkCommandBuffer cb)
{
    QRhiVulkan::DeferredReleaseEntry e;
    e.type = QRhiVulkan::DeferredReleaseEntry::CommandBuffer;
    e.lastActiveFrameSlot = currentFrameSlot;
    e.commandBuffer.cb = cb;
    releaseQueue.append(e);
}

void QRhiVulkan::beginPass(QRhiCommandBuffer *cb,
                           QRhiRenderTarget *rt,
                           const QColor &colorClearValue,
                           const QRhiDepthStencilClearValue &depthStencilClearValue,
                           QRhiResourceUpdateBatch *resourceUpdates)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::NoPass);

    if (resourceUpdates)
        enqueueResourceUpdates(cbD, resourceUpdates);

    // Insert a TransitionPassResources into the command stream, pointing to
    // the tracker this pass is going to use. That's how we generate the
    // barriers later during recording the real VkCommandBuffer, right before
    // the vkCmdBeginRenderPass.
    enqueueTransitionPassResources(cbD);

    QVkRenderTargetData *rtD = nullptr;
    switch (rt->resourceType()) {
    case QRhiResource::RenderTarget:
        rtD = &QRHI_RES(QVkReferenceRenderTarget, rt)->d;
        rtD->rp->lastActiveFrameSlot = currentFrameSlot;
        Q_ASSERT(currentSwapChain);
        currentSwapChain->imageRes[currentSwapChain->currentImageIndex].lastUse =
                QVkSwapChain::ImageResources::ScImageUseRender;
        break;
    case QRhiResource::TextureRenderTarget:
    {
        QVkTextureRenderTarget *rtTex = QRHI_RES(QVkTextureRenderTarget, rt);
        rtD = &rtTex->d;
        activateTextureRenderTarget(cbD, rtTex);
    }
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    cbD->recordingPass = QVkCommandBuffer::RenderPass;
    cbD->currentTarget = rt;

    // No copy operations or image layout transitions allowed after this point
    // (up until endPass) as we are going to begin the renderpass.

    VkRenderPassBeginInfo rpBeginInfo;
    memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = rtD->rp->rp;
    rpBeginInfo.framebuffer = rtD->fb;
    rpBeginInfo.renderArea.extent.width = uint32_t(rtD->pixelSize.width());
    rpBeginInfo.renderArea.extent.height = uint32_t(rtD->pixelSize.height());

    QVarLengthArray<VkClearValue, 4> cvs;
    for (int i = 0; i < rtD->colorAttCount; ++i) {
        VkClearValue cv;
        cv.color = { { float(colorClearValue.redF()), float(colorClearValue.greenF()), float(colorClearValue.blueF()),
                       float(colorClearValue.alphaF()) } };
        cvs.append(cv);
    }
    for (int i = 0; i < rtD->dsAttCount; ++i) {
        VkClearValue cv;
        cv.depthStencil = { depthStencilClearValue.depthClearValue(), depthStencilClearValue.stencilClearValue() };
        cvs.append(cv);
    }
    for (int i = 0; i < rtD->resolveAttCount; ++i) {
        VkClearValue cv;
        cv.color = { { float(colorClearValue.redF()), float(colorClearValue.greenF()), float(colorClearValue.blueF()),
                       float(colorClearValue.alphaF()) } };
        cvs.append(cv);
    }
    rpBeginInfo.clearValueCount = uint32_t(cvs.count());

    QVkCommandBuffer::Command cmd;
    cmd.cmd = QVkCommandBuffer::Command::BeginRenderPass;
    cmd.args.beginRenderPass.desc = rpBeginInfo;
    cmd.args.beginRenderPass.clearValueIndex = cbD->pools.clearValue.count();
    cbD->pools.clearValue.append(cvs.constData(), cvs.count());
    cbD->commands.append(cmd);

    if (cbD->useSecondaryCb)
        cbD->secondaryCbs.append(startSecondaryCommandBuffer(rtD));
}

void QRhiVulkan::endPass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::RenderPass);

    if (cbD->useSecondaryCb) {
        VkCommandBuffer secondaryCb = cbD->secondaryCbs.last();
        cbD->secondaryCbs.removeLast();
        endAndEnqueueSecondaryCommandBuffer(secondaryCb, cbD);
        cbD->resetCachedState();
    }

    QVkCommandBuffer::Command cmd;
    cmd.cmd = QVkCommandBuffer::Command::EndRenderPass;
    cbD->commands.append(cmd);

    cbD->recordingPass = QVkCommandBuffer::NoPass;
    cbD->currentTarget = nullptr;

    if (resourceUpdates)
        enqueueResourceUpdates(cbD, resourceUpdates);
}

void QRhiVulkan::beginComputePass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::NoPass);

    if (resourceUpdates)
        enqueueResourceUpdates(cbD, resourceUpdates);

    enqueueTransitionPassResources(cbD);

    cbD->recordingPass = QVkCommandBuffer::ComputePass;

    cbD->computePassState.reset();

    if (cbD->useSecondaryCb)
        cbD->secondaryCbs.append(startSecondaryCommandBuffer());
}

void QRhiVulkan::endComputePass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::ComputePass);

    if (cbD->useSecondaryCb) {
        VkCommandBuffer secondaryCb = cbD->secondaryCbs.last();
        cbD->secondaryCbs.removeLast();
        endAndEnqueueSecondaryCommandBuffer(secondaryCb, cbD);
        cbD->resetCachedState();
    }

    cbD->recordingPass = QVkCommandBuffer::NoPass;

    if (resourceUpdates)
        enqueueResourceUpdates(cbD, resourceUpdates);
}

void QRhiVulkan::setComputePipeline(QRhiCommandBuffer *cb, QRhiComputePipeline *ps)
{
    QVkComputePipeline *psD = QRHI_RES(QVkComputePipeline, ps);
    Q_ASSERT(psD->pipeline);
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::ComputePass);

    if (cbD->currentComputePipeline != ps || cbD->currentPipelineGeneration != psD->generation) {
        if (cbD->useSecondaryCb) {
            df->vkCmdBindPipeline(cbD->secondaryCbs.last(), VK_PIPELINE_BIND_POINT_COMPUTE, psD->pipeline);
        } else {
            QVkCommandBuffer::Command cmd;
            cmd.cmd = QVkCommandBuffer::Command::BindPipeline;
            cmd.args.bindPipeline.bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
            cmd.args.bindPipeline.pipeline = psD->pipeline;
            cbD->commands.append(cmd);
        }

        cbD->currentGraphicsPipeline = nullptr;
        cbD->currentComputePipeline = ps;
        cbD->currentPipelineGeneration = psD->generation;
    }

    psD->lastActiveFrameSlot = currentFrameSlot;
}

template<typename T>
inline void qrhivk_accumulateComputeResource(T *writtenResources, QRhiResource *resource,
                                             QRhiShaderResourceBinding::Type bindingType,
                                             int loadTypeVal, int storeTypeVal, int loadStoreTypeVal)
{
    VkAccessFlags access = 0;
    if (bindingType == loadTypeVal) {
        access = VK_ACCESS_SHADER_READ_BIT;
    } else {
        access = VK_ACCESS_SHADER_WRITE_BIT;
        if (bindingType == loadStoreTypeVal)
            access |= VK_ACCESS_SHADER_READ_BIT;
    }
    auto it = writtenResources->find(resource);
    if (it != writtenResources->end())
        it->first |= access;
    else if (bindingType == storeTypeVal || bindingType == loadStoreTypeVal)
        writtenResources->insert(resource, { access, true });
}

void QRhiVulkan::dispatch(QRhiCommandBuffer *cb, int x, int y, int z)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::ComputePass);

    // When there are multiple dispatches, read-after-write and
    // write-after-write need a barrier.
    QVarLengthArray<VkImageMemoryBarrier, 8> imageBarriers;
    QVarLengthArray<VkBufferMemoryBarrier, 8> bufferBarriers;
    if (cbD->currentComputeSrb) {
        // The key in the writtenResources map indicates that the resource was
        // written in a previous dispatch, whereas the value accumulates the
        // access mask in the current one.
        for (auto &accessAndIsNewFlag : cbD->computePassState.writtenResources)
            accessAndIsNewFlag = { 0, false };

        QVkShaderResourceBindings *srbD = QRHI_RES(QVkShaderResourceBindings, cbD->currentComputeSrb);
        const int bindingCount = srbD->m_bindings.count();
        for (int i = 0; i < bindingCount; ++i) {
            const QRhiShaderResourceBinding::Data *b = srbD->m_bindings.at(i).data();
            switch (b->type) {
            case QRhiShaderResourceBinding::ImageLoad:
            case QRhiShaderResourceBinding::ImageStore:
            case QRhiShaderResourceBinding::ImageLoadStore:
                qrhivk_accumulateComputeResource(&cbD->computePassState.writtenResources,
                                                 b->u.simage.tex,
                                                 b->type,
                                                 QRhiShaderResourceBinding::ImageLoad,
                                                 QRhiShaderResourceBinding::ImageStore,
                                                 QRhiShaderResourceBinding::ImageLoadStore);
                break;
            case QRhiShaderResourceBinding::BufferLoad:
            case QRhiShaderResourceBinding::BufferStore:
            case QRhiShaderResourceBinding::BufferLoadStore:
                qrhivk_accumulateComputeResource(&cbD->computePassState.writtenResources,
                                                 b->u.sbuf.buf,
                                                 b->type,
                                                 QRhiShaderResourceBinding::BufferLoad,
                                                 QRhiShaderResourceBinding::BufferStore,
                                                 QRhiShaderResourceBinding::BufferLoadStore);
                break;
            default:
                break;
            }
        }

        for (auto it = cbD->computePassState.writtenResources.begin(); it != cbD->computePassState.writtenResources.end(); ) {
            const int accessInThisDispatch = it->first;
            const bool isNewInThisDispatch = it->second;
            if (accessInThisDispatch && !isNewInThisDispatch) {
                if (it.key()->resourceType() == QRhiResource::Texture) {
                    QVkTexture *texD = QRHI_RES(QVkTexture, it.key());
                    VkImageMemoryBarrier barrier;
                    memset(&barrier, 0, sizeof(barrier));
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    // won't care about subresources, pretend the whole resource was written
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
                    barrier.oldLayout = texD->usageState.layout;
                    barrier.newLayout = texD->usageState.layout;
                    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                    barrier.dstAccessMask = accessInThisDispatch;
                    barrier.image = texD->image;
                    imageBarriers.append(barrier);
                } else {
                    QVkBuffer *bufD = QRHI_RES(QVkBuffer, it.key());
                    VkBufferMemoryBarrier barrier;
                    memset(&barrier, 0, sizeof(barrier));
                    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                    barrier.dstAccessMask = accessInThisDispatch;
                    barrier.buffer = bufD->buffers[bufD->m_type == QRhiBuffer::Dynamic ? currentFrameSlot : 0];
                    barrier.size = VK_WHOLE_SIZE;
                    bufferBarriers.append(barrier);
                }
            }
            // Anything that was previously written, but is only read now, can be
            // removed from the written list (because that previous write got a
            // corresponding barrier now).
            if (accessInThisDispatch == VK_ACCESS_SHADER_READ_BIT)
                it = cbD->computePassState.writtenResources.erase(it);
            else
                ++it;
        }
    }

    if (cbD->useSecondaryCb) {
        VkCommandBuffer secondaryCb = cbD->secondaryCbs.last();
        if (!imageBarriers.isEmpty()) {
            df->vkCmdPipelineBarrier(secondaryCb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     0, 0, nullptr,
                                     0, nullptr,
                                     imageBarriers.count(), imageBarriers.constData());
        }
        if (!bufferBarriers.isEmpty()) {
            df->vkCmdPipelineBarrier(secondaryCb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     0, 0, nullptr,
                                     bufferBarriers.count(), bufferBarriers.constData(),
                                     0, nullptr);
        }
        df->vkCmdDispatch(secondaryCb, uint32_t(x), uint32_t(y), uint32_t(z));
    } else {
        QVkCommandBuffer::Command cmd;
        if (!imageBarriers.isEmpty()) {
            cmd.cmd = QVkCommandBuffer::Command::ImageBarrier;
            cmd.args.imageBarrier.srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            cmd.args.imageBarrier.dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            cmd.args.imageBarrier.count = imageBarriers.count();
            cmd.args.imageBarrier.index = cbD->pools.imageBarrier.count();
            cbD->pools.imageBarrier.append(imageBarriers.constData(), imageBarriers.count());
            cbD->commands.append(cmd);
        }
        if (!bufferBarriers.isEmpty()) {
            cmd.cmd = QVkCommandBuffer::Command::BufferBarrier;
            cmd.args.bufferBarrier.srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            cmd.args.bufferBarrier.dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            cmd.args.bufferBarrier.count = bufferBarriers.count();
            cmd.args.bufferBarrier.index = cbD->pools.bufferBarrier.count();
            cbD->pools.bufferBarrier.append(bufferBarriers.constData(), bufferBarriers.count());
            cbD->commands.append(cmd);
        }
        cmd.cmd = QVkCommandBuffer::Command::Dispatch;
        cmd.args.dispatch.x = x;
        cmd.args.dispatch.y = y;
        cmd.args.dispatch.z = z;
        cbD->commands.append(cmd);
    }
}

VkShaderModule QRhiVulkan::createShader(const QByteArray &spirv)
{
    VkShaderModuleCreateInfo shaderInfo;
    memset(&shaderInfo, 0, sizeof(shaderInfo));
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = size_t(spirv.size());
    shaderInfo.pCode = reinterpret_cast<const quint32 *>(spirv.constData());
    VkShaderModule shaderModule;
    VkResult err = df->vkCreateShaderModule(dev, &shaderInfo, nullptr, &shaderModule);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create shader module: %d", err);
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}

bool QRhiVulkan::ensurePipelineCache()
{
    if (pipelineCache)
        return true;

    VkPipelineCacheCreateInfo pipelineCacheInfo;
    memset(&pipelineCacheInfo, 0, sizeof(pipelineCacheInfo));
    pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VkResult err = df->vkCreatePipelineCache(dev, &pipelineCacheInfo, nullptr, &pipelineCache);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create pipeline cache: %d", err);
        return false;
    }
    return true;
}

void QRhiVulkan::updateShaderResourceBindings(QRhiShaderResourceBindings *srb, int descSetIdx)
{
    QVkShaderResourceBindings *srbD = QRHI_RES(QVkShaderResourceBindings, srb);

    QVarLengthArray<VkDescriptorBufferInfo, 8> bufferInfos;
    using ArrayOfImageDesc = QVarLengthArray<VkDescriptorImageInfo, 8>;
    QVarLengthArray<ArrayOfImageDesc, 8> imageInfos;
    QVarLengthArray<VkWriteDescriptorSet, 12> writeInfos;
    QVarLengthArray<QPair<int, int>, 12> infoIndices;

    const bool updateAll = descSetIdx < 0;
    int frameSlot = updateAll ? 0 : descSetIdx;
    while (frameSlot < (updateAll ? QVK_FRAMES_IN_FLIGHT : descSetIdx + 1)) {
        srbD->boundResourceData[frameSlot].resize(srbD->sortedBindings.count());
        for (int i = 0, ie = srbD->sortedBindings.count(); i != ie; ++i) {
            const QRhiShaderResourceBinding::Data *b = srbD->sortedBindings.at(i).data();
            QVkShaderResourceBindings::BoundResourceData &bd(srbD->boundResourceData[frameSlot][i]);

            VkWriteDescriptorSet writeInfo;
            memset(&writeInfo, 0, sizeof(writeInfo));
            writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeInfo.dstSet = srbD->descSets[frameSlot];
            writeInfo.dstBinding = uint32_t(b->binding);
            writeInfo.descriptorCount = 1;

            int bufferInfoIndex = -1;
            int imageInfoIndex = -1;

            switch (b->type) {
            case QRhiShaderResourceBinding::UniformBuffer:
            {
                writeInfo.descriptorType = b->u.ubuf.hasDynamicOffset ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
                                                                      : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                QRhiBuffer *buf = b->u.ubuf.buf;
                QVkBuffer *bufD = QRHI_RES(QVkBuffer, buf);
                bd.ubuf.id = bufD->m_id;
                bd.ubuf.generation = bufD->generation;
                VkDescriptorBufferInfo bufInfo;
                bufInfo.buffer = bufD->m_type == QRhiBuffer::Dynamic ? bufD->buffers[frameSlot] : bufD->buffers[0];
                bufInfo.offset = VkDeviceSize(b->u.ubuf.offset);
                bufInfo.range = VkDeviceSize(b->u.ubuf.maybeSize ? b->u.ubuf.maybeSize : bufD->m_size);
                // be nice and assert when we know the vulkan device would die a horrible death due to non-aligned reads
                Q_ASSERT(aligned(bufInfo.offset, ubufAlign) == bufInfo.offset);
                bufferInfoIndex = bufferInfos.count();
                bufferInfos.append(bufInfo);
            }
                break;
            case QRhiShaderResourceBinding::SampledTexture:
            {
                const QRhiShaderResourceBinding::Data::SampledTextureData *data = &b->u.stex;
                writeInfo.descriptorCount = data->count; // arrays of combined image samplers are supported
                writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                ArrayOfImageDesc imageInfo(data->count);
                for (int elem = 0; elem < data->count; ++elem) {
                    QVkTexture *texD = QRHI_RES(QVkTexture, data->texSamplers[elem].tex);
                    QVkSampler *samplerD = QRHI_RES(QVkSampler, data->texSamplers[elem].sampler);
                    bd.stex.d[elem].texId = texD->m_id;
                    bd.stex.d[elem].texGeneration = texD->generation;
                    bd.stex.d[elem].samplerId = samplerD->m_id;
                    bd.stex.d[elem].samplerGeneration = samplerD->generation;
                    imageInfo[elem].sampler = samplerD->sampler;
                    imageInfo[elem].imageView = texD->imageView;
                    imageInfo[elem].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }
                bd.stex.count = data->count;
                imageInfoIndex = imageInfos.count();
                imageInfos.append(imageInfo);
            }
                break;
            case QRhiShaderResourceBinding::ImageLoad:
            case QRhiShaderResourceBinding::ImageStore:
            case QRhiShaderResourceBinding::ImageLoadStore:
            {
                QVkTexture *texD = QRHI_RES(QVkTexture, b->u.simage.tex);
                VkImageView view = texD->imageViewForLevel(b->u.simage.level);
                if (view) {
                    writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    bd.simage.id = texD->m_id;
                    bd.simage.generation = texD->generation;
                    ArrayOfImageDesc imageInfo(1);
                    imageInfo[0].sampler = VK_NULL_HANDLE;
                    imageInfo[0].imageView = view;
                    imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                    imageInfoIndex = imageInfos.count();
                    imageInfos.append(imageInfo);
                }
            }
                break;
            case QRhiShaderResourceBinding::BufferLoad:
            case QRhiShaderResourceBinding::BufferStore:
            case QRhiShaderResourceBinding::BufferLoadStore:
            {
                QVkBuffer *bufD = QRHI_RES(QVkBuffer, b->u.sbuf.buf);
                writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                bd.sbuf.id = bufD->m_id;
                bd.sbuf.generation = bufD->generation;
                VkDescriptorBufferInfo bufInfo;
                bufInfo.buffer = bufD->m_type == QRhiBuffer::Dynamic ? bufD->buffers[frameSlot] : bufD->buffers[0];
                bufInfo.offset = VkDeviceSize(b->u.ubuf.offset);
                bufInfo.range = VkDeviceSize(b->u.ubuf.maybeSize ? b->u.ubuf.maybeSize : bufD->m_size);
                bufferInfoIndex = bufferInfos.count();
                bufferInfos.append(bufInfo);
            }
                break;
            default:
                continue;
            }

            writeInfos.append(writeInfo);
            infoIndices.append({ bufferInfoIndex, imageInfoIndex });
        }
        ++frameSlot;
    }

    for (int i = 0, writeInfoCount = writeInfos.count(); i < writeInfoCount; ++i) {
        const int bufferInfoIndex = infoIndices[i].first;
        const int imageInfoIndex = infoIndices[i].second;
        if (bufferInfoIndex >= 0)
            writeInfos[i].pBufferInfo = &bufferInfos[bufferInfoIndex];
        else if (imageInfoIndex >= 0)
            writeInfos[i].pImageInfo = imageInfos[imageInfoIndex].constData();
    }

    df->vkUpdateDescriptorSets(dev, uint32_t(writeInfos.count()), writeInfos.constData(), 0, nullptr);
}

static inline bool accessIsWrite(VkAccessFlags access)
{
    return (access & VK_ACCESS_SHADER_WRITE_BIT) != 0
            || (access & VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT) != 0
            || (access & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT) != 0
            || (access & VK_ACCESS_TRANSFER_WRITE_BIT) != 0
            || (access & VK_ACCESS_HOST_WRITE_BIT) != 0
            || (access & VK_ACCESS_MEMORY_WRITE_BIT) != 0;
}

void QRhiVulkan::trackedBufferBarrier(QVkCommandBuffer *cbD, QVkBuffer *bufD, int slot,
                                      VkAccessFlags access, VkPipelineStageFlags stage)
{
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::NoPass);
    Q_ASSERT(access && stage);
    QVkBuffer::UsageState &s(bufD->usageState[slot]);
    if (!s.stage) {
        s.access = access;
        s.stage = stage;
        return;
    }

    if (s.access == access && s.stage == stage) {
        // No need to flood with unnecessary read-after-read barriers.
        // Write-after-write is a different matter, however.
        if (!accessIsWrite(access))
            return;
    }

    VkBufferMemoryBarrier bufMemBarrier;
    memset(&bufMemBarrier, 0, sizeof(bufMemBarrier));
    bufMemBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufMemBarrier.srcAccessMask = s.access;
    bufMemBarrier.dstAccessMask = access;
    bufMemBarrier.buffer = bufD->buffers[slot];
    bufMemBarrier.size = VK_WHOLE_SIZE;

    QVkCommandBuffer::Command cmd;
    cmd.cmd = QVkCommandBuffer::Command::BufferBarrier;
    cmd.args.bufferBarrier.srcStageMask = s.stage;
    cmd.args.bufferBarrier.dstStageMask = stage;
    cmd.args.bufferBarrier.count = 1;
    cmd.args.bufferBarrier.index = cbD->pools.bufferBarrier.count();
    cbD->pools.bufferBarrier.append(bufMemBarrier);
    cbD->commands.append(cmd);

    s.access = access;
    s.stage = stage;
}

void QRhiVulkan::trackedImageBarrier(QVkCommandBuffer *cbD, QVkTexture *texD,
                                     VkImageLayout layout, VkAccessFlags access, VkPipelineStageFlags stage)
{
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::NoPass);
    Q_ASSERT(layout && access && stage);
    QVkTexture::UsageState &s(texD->usageState);
    if (s.access == access && s.stage == stage && s.layout == layout) {
        if (!accessIsWrite(access))
            return;
    }

    VkImageMemoryBarrier barrier;
    memset(&barrier, 0, sizeof(barrier));
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.subresourceRange.aspectMask = !isDepthTextureFormat(texD->m_format)
            ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    barrier.oldLayout = s.layout; // new textures have this set to PREINITIALIZED
    barrier.newLayout = layout;
    barrier.srcAccessMask = s.access; // may be 0 but that's fine
    barrier.dstAccessMask = access;
    barrier.image = texD->image;

    VkPipelineStageFlags srcStage = s.stage;
    // stage mask cannot be 0
    if (!srcStage)
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    QVkCommandBuffer::Command cmd;
    cmd.cmd = QVkCommandBuffer::Command::ImageBarrier;
    cmd.args.imageBarrier.srcStageMask = srcStage;
    cmd.args.imageBarrier.dstStageMask = stage;
    cmd.args.imageBarrier.count = 1;
    cmd.args.imageBarrier.index = cbD->pools.imageBarrier.count();
    cbD->pools.imageBarrier.append(barrier);
    cbD->commands.append(cmd);

    s.layout = layout;
    s.access = access;
    s.stage = stage;
}

void QRhiVulkan::subresourceBarrier(QVkCommandBuffer *cbD, VkImage image,
                                    VkImageLayout oldLayout, VkImageLayout newLayout,
                                    VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                                    VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                                    int startLayer, int layerCount,
                                    int startLevel, int levelCount)
{
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::NoPass);
    VkImageMemoryBarrier barrier;
    memset(&barrier, 0, sizeof(barrier));
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = uint32_t(startLevel);
    barrier.subresourceRange.levelCount = uint32_t(levelCount);
    barrier.subresourceRange.baseArrayLayer = uint32_t(startLayer);
    barrier.subresourceRange.layerCount = uint32_t(layerCount);
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;
    barrier.image = image;

    QVkCommandBuffer::Command cmd;
    cmd.cmd = QVkCommandBuffer::Command::ImageBarrier;
    cmd.args.imageBarrier.srcStageMask = srcStage;
    cmd.args.imageBarrier.dstStageMask = dstStage;
    cmd.args.imageBarrier.count = 1;
    cmd.args.imageBarrier.index = cbD->pools.imageBarrier.count();
    cbD->pools.imageBarrier.append(barrier);
    cbD->commands.append(cmd);
}

VkDeviceSize QRhiVulkan::subresUploadByteSize(const QRhiTextureSubresourceUploadDescription &subresDesc) const
{
    VkDeviceSize size = 0;
    const qsizetype imageSizeBytes = subresDesc.image().isNull() ?
                subresDesc.data().size() : subresDesc.image().sizeInBytes();
    if (imageSizeBytes > 0)
        size += aligned(VkDeviceSize(imageSizeBytes), texbufAlign);
    return size;
}

void QRhiVulkan::prepareUploadSubres(QVkTexture *texD, int layer, int level,
                                     const QRhiTextureSubresourceUploadDescription &subresDesc,
                                     size_t *curOfs, void *mp,
                                     BufferImageCopyList *copyInfos)
{
    qsizetype copySizeBytes = 0;
    qsizetype imageSizeBytes = 0;
    const void *src = nullptr;

    VkBufferImageCopy copyInfo;
    memset(&copyInfo, 0, sizeof(copyInfo));
    copyInfo.bufferOffset = *curOfs;
    copyInfo.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyInfo.imageSubresource.mipLevel = uint32_t(level);
    copyInfo.imageSubresource.baseArrayLayer = uint32_t(layer);
    copyInfo.imageSubresource.layerCount = 1;
    copyInfo.imageExtent.depth = 1;

    const QByteArray rawData = subresDesc.data();
    const QPoint dp = subresDesc.destinationTopLeft();
    QImage image = subresDesc.image();
    if (!image.isNull()) {
        copySizeBytes = imageSizeBytes = image.sizeInBytes();
        QSize size = image.size();
        src = image.constBits();
        // Scanlines in QImage are 4 byte aligned so bpl must
        // be taken into account for bufferRowLength.
        int bpc = qMax(1, image.depth() / 8);
        // this is in pixels, not bytes, to make it more complicated...
        copyInfo.bufferRowLength = uint32_t(image.bytesPerLine() / bpc);
        if (!subresDesc.sourceSize().isEmpty() || !subresDesc.sourceTopLeft().isNull()) {
            const int sx = subresDesc.sourceTopLeft().x();
            const int sy = subresDesc.sourceTopLeft().y();
            if (!subresDesc.sourceSize().isEmpty())
                size = subresDesc.sourceSize();
            if (image.depth() == 32) {
                // The staging buffer will get the full image
                // regardless, just adjust the vk
                // buffer-to-image copy start offset.
                copyInfo.bufferOffset += VkDeviceSize(sy * image.bytesPerLine() + sx * 4);
                // bufferRowLength remains set to the original image's width
            } else {
                image = image.copy(sx, sy, size.width(), size.height());
                src = image.constBits();
                // The staging buffer gets the slice only. The rest of the
                // space reserved for this mip will be unused.
                copySizeBytes = image.sizeInBytes();
                bpc = qMax(1, image.depth() / 8);
                copyInfo.bufferRowLength = uint32_t(image.bytesPerLine() / bpc);
            }
        }
        copyInfo.imageOffset.x = dp.x();
        copyInfo.imageOffset.y = dp.y();
        copyInfo.imageExtent.width = uint32_t(size.width());
        copyInfo.imageExtent.height = uint32_t(size.height());
        copyInfos->append(copyInfo);
    } else if (!rawData.isEmpty() && isCompressedFormat(texD->m_format)) {
        copySizeBytes = imageSizeBytes = rawData.size();
        src = rawData.constData();
        QSize size = q->sizeForMipLevel(level, texD->m_pixelSize);
        const int subresw = size.width();
        const int subresh = size.height();
        if (!subresDesc.sourceSize().isEmpty())
            size = subresDesc.sourceSize();
        const int w = size.width();
        const int h = size.height();
        QSize blockDim;
        compressedFormatInfo(texD->m_format, QSize(w, h), nullptr, nullptr, &blockDim);
        // x and y must be multiples of the block width and height
        copyInfo.imageOffset.x = aligned(dp.x(), blockDim.width());
        copyInfo.imageOffset.y = aligned(dp.y(), blockDim.height());
        // width and height must be multiples of the block width and height
        // or x + width and y + height must equal the subresource width and height
        copyInfo.imageExtent.width = uint32_t(dp.x() + w == subresw ? w : aligned(w, blockDim.width()));
        copyInfo.imageExtent.height = uint32_t(dp.y() + h == subresh ? h : aligned(h, blockDim.height()));
        copyInfos->append(copyInfo);
    } else if (!rawData.isEmpty()) {
        copySizeBytes = imageSizeBytes = rawData.size();
        src = rawData.constData();
        QSize size = q->sizeForMipLevel(level, texD->m_pixelSize);
        if (!subresDesc.sourceSize().isEmpty())
            size = subresDesc.sourceSize();
        copyInfo.imageOffset.x = dp.x();
        copyInfo.imageOffset.y = dp.y();
        copyInfo.imageExtent.width = uint32_t(size.width());
        copyInfo.imageExtent.height = uint32_t(size.height());
        copyInfos->append(copyInfo);
    } else {
        qWarning("Invalid texture upload for %p layer=%d mip=%d", texD, layer, level);
    }

    memcpy(reinterpret_cast<char *>(mp) + *curOfs, src, size_t(copySizeBytes));
    *curOfs += aligned(VkDeviceSize(imageSizeBytes), texbufAlign);
}

void QRhiVulkan::enqueueResourceUpdates(QVkCommandBuffer *cbD, QRhiResourceUpdateBatch *resourceUpdates)
{
    QRhiResourceUpdateBatchPrivate *ud = QRhiResourceUpdateBatchPrivate::get(resourceUpdates);
    QRhiProfilerPrivate *rhiP = profilerPrivateOrNull();

    for (const QRhiResourceUpdateBatchPrivate::BufferOp &u : ud->bufferOps) {
        if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::DynamicUpdate) {
            QVkBuffer *bufD = QRHI_RES(QVkBuffer, u.buf);
            Q_ASSERT(bufD->m_type == QRhiBuffer::Dynamic);
            for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i)
                bufD->pendingDynamicUpdates[i].append(u);
        } else if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::StaticUpload) {
            QVkBuffer *bufD = QRHI_RES(QVkBuffer, u.buf);
            Q_ASSERT(bufD->m_type != QRhiBuffer::Dynamic);
            Q_ASSERT(u.offset + u.data.size() <= bufD->m_size);

            if (!bufD->stagingBuffers[currentFrameSlot]) {
                VkBufferCreateInfo bufferInfo;
                memset(&bufferInfo, 0, sizeof(bufferInfo));
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                // must cover the entire buffer - this way multiple, partial updates per frame
                // are supported even when the staging buffer is reused (Static)
                bufferInfo.size = VkDeviceSize(bufD->m_size);
                bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

                VmaAllocationCreateInfo allocInfo;
                memset(&allocInfo, 0, sizeof(allocInfo));
                allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

                VmaAllocation allocation;
                VkResult err = vmaCreateBuffer(toVmaAllocator(allocator), &bufferInfo, &allocInfo,
                                               &bufD->stagingBuffers[currentFrameSlot], &allocation, nullptr);
                if (err == VK_SUCCESS) {
                    bufD->stagingAllocations[currentFrameSlot] = allocation;
                    QRHI_PROF_F(newBufferStagingArea(bufD, currentFrameSlot, quint32(bufD->m_size)));
                } else {
                    qWarning("Failed to create staging buffer of size %d: %d", bufD->m_size, err);
                    continue;
                }
            }

            void *p = nullptr;
            VmaAllocation a = toVmaAllocation(bufD->stagingAllocations[currentFrameSlot]);
            VkResult err = vmaMapMemory(toVmaAllocator(allocator), a, &p);
            if (err != VK_SUCCESS) {
                qWarning("Failed to map buffer: %d", err);
                continue;
            }
            memcpy(static_cast<uchar *>(p) + u.offset, u.data.constData(), size_t(u.data.size()));
            vmaUnmapMemory(toVmaAllocator(allocator), a);
            vmaFlushAllocation(toVmaAllocator(allocator), a, VkDeviceSize(u.offset), VkDeviceSize(u.data.size()));

            trackedBufferBarrier(cbD, bufD, 0,
                                 VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            VkBufferCopy copyInfo;
            memset(&copyInfo, 0, sizeof(copyInfo));
            copyInfo.srcOffset = VkDeviceSize(u.offset);
            copyInfo.dstOffset = VkDeviceSize(u.offset);
            copyInfo.size = VkDeviceSize(u.data.size());

            QVkCommandBuffer::Command cmd;
            cmd.cmd = QVkCommandBuffer::Command::CopyBuffer;
            cmd.args.copyBuffer.src = bufD->stagingBuffers[currentFrameSlot];
            cmd.args.copyBuffer.dst = bufD->buffers[0];
            cmd.args.copyBuffer.desc = copyInfo;
            cbD->commands.append(cmd);

            // Where's the barrier for read-after-write? (assuming the common case
            // of binding this buffer as vertex/index, or, less likely, as uniform
            // buffer, in a renderpass later on) That is handled by the pass
            // resource tracking: the appropriate pipeline barrier will be
            // generated and recorded right before the renderpass, that binds this
            // buffer in one of its commands, gets its BeginRenderPass recorded.

            bufD->lastActiveFrameSlot = currentFrameSlot;

            if (bufD->m_type == QRhiBuffer::Immutable) {
                QRhiVulkan::DeferredReleaseEntry e;
                e.type = QRhiVulkan::DeferredReleaseEntry::StagingBuffer;
                e.lastActiveFrameSlot = currentFrameSlot;
                e.stagingBuffer.stagingBuffer = bufD->stagingBuffers[currentFrameSlot];
                e.stagingBuffer.stagingAllocation = bufD->stagingAllocations[currentFrameSlot];
                bufD->stagingBuffers[currentFrameSlot] = VK_NULL_HANDLE;
                bufD->stagingAllocations[currentFrameSlot] = nullptr;
                releaseQueue.append(e);
                QRHI_PROF_F(releaseBufferStagingArea(bufD, currentFrameSlot));
            }
        } else if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::Read) {
            QVkBuffer *bufD = QRHI_RES(QVkBuffer, u.buf);
            if (bufD->m_type == QRhiBuffer::Dynamic) {
                executeBufferHostWritesForSlot(bufD, currentFrameSlot);
                void *p = nullptr;
                VmaAllocation a = toVmaAllocation(bufD->allocations[currentFrameSlot]);
                VkResult err = vmaMapMemory(toVmaAllocator(allocator), a, &p);
                if (err == VK_SUCCESS) {
                    u.result->data.resize(u.readSize);
                    memcpy(u.result->data.data(), reinterpret_cast<char *>(p) + u.offset, size_t(u.readSize));
                    vmaUnmapMemory(toVmaAllocator(allocator), a);
                }
                if (u.result->completed)
                    u.result->completed();
            } else {
                // Non-Dynamic buffers may not be host visible, so have to
                // create a readback buffer, enqueue a copy from
                // bufD->buffers[0] to this buffer, and then once the command
                // buffer completes, copy the data out of the host visible
                // readback buffer. Quite similar to what we do for texture
                // readbacks.
                BufferReadback readback;
                readback.activeFrameSlot = currentFrameSlot;
                readback.result = u.result;
                readback.byteSize = u.readSize;

                VkBufferCreateInfo bufferInfo;
                memset(&bufferInfo, 0, sizeof(bufferInfo));
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = VkDeviceSize(readback.byteSize);
                bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

                VmaAllocationCreateInfo allocInfo;
                memset(&allocInfo, 0, sizeof(allocInfo));
                allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;

                VmaAllocation allocation;
                VkResult err = vmaCreateBuffer(toVmaAllocator(allocator), &bufferInfo, &allocInfo, &readback.stagingBuf, &allocation, nullptr);
                if (err == VK_SUCCESS) {
                    readback.stagingAlloc = allocation;
                    QRHI_PROF_F(newReadbackBuffer(qint64(readback.stagingBuf), bufD, uint(readback.byteSize)));
                } else {
                    qWarning("Failed to create readback buffer of size %u: %d", readback.byteSize, err);
                    continue;
                }

                trackedBufferBarrier(cbD, bufD, 0, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

                VkBufferCopy copyInfo;
                memset(&copyInfo, 0, sizeof(copyInfo));
                copyInfo.srcOffset = VkDeviceSize(u.offset);
                copyInfo.size = VkDeviceSize(u.readSize);

                QVkCommandBuffer::Command cmd;
                cmd.cmd = QVkCommandBuffer::Command::CopyBuffer;
                cmd.args.copyBuffer.src = bufD->buffers[0];
                cmd.args.copyBuffer.dst = readback.stagingBuf;
                cmd.args.copyBuffer.desc = copyInfo;
                cbD->commands.append(cmd);

                bufD->lastActiveFrameSlot = currentFrameSlot;

                activeBufferReadbacks.append(readback);
            }
        }
    }

    for (const QRhiResourceUpdateBatchPrivate::TextureOp &u : ud->textureOps) {
        if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Upload) {
            QVkTexture *utexD = QRHI_RES(QVkTexture, u.dst);
            // batch into a single staging buffer and a single CopyBufferToImage with multiple copyInfos
            VkDeviceSize stagingSize = 0;
            for (int layer = 0; layer < QRhi::MAX_LAYERS; ++layer) {
                for (int level = 0; level < QRhi::MAX_LEVELS; ++level) {
                    for (const QRhiTextureSubresourceUploadDescription &subresDesc : qAsConst(u.subresDesc[layer][level]))
                        stagingSize += subresUploadByteSize(subresDesc);
                }
            }

            Q_ASSERT(!utexD->stagingBuffers[currentFrameSlot]);
            VkBufferCreateInfo bufferInfo;
            memset(&bufferInfo, 0, sizeof(bufferInfo));
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = stagingSize;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            VmaAllocationCreateInfo allocInfo;
            memset(&allocInfo, 0, sizeof(allocInfo));
            allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

            VmaAllocation allocation;
            VkResult err = vmaCreateBuffer(toVmaAllocator(allocator), &bufferInfo, &allocInfo,
                                           &utexD->stagingBuffers[currentFrameSlot], &allocation, nullptr);
            if (err != VK_SUCCESS) {
                qWarning("Failed to create image staging buffer of size %d: %d", int(stagingSize), err);
                continue;
            }
            utexD->stagingAllocations[currentFrameSlot] = allocation;
            QRHI_PROF_F(newTextureStagingArea(utexD, currentFrameSlot, quint32(stagingSize)));

            BufferImageCopyList copyInfos;
            size_t curOfs = 0;
            void *mp = nullptr;
            VmaAllocation a = toVmaAllocation(utexD->stagingAllocations[currentFrameSlot]);
            err = vmaMapMemory(toVmaAllocator(allocator), a, &mp);
            if (err != VK_SUCCESS) {
                qWarning("Failed to map image data: %d", err);
                continue;
            }

            for (int layer = 0; layer < QRhi::MAX_LAYERS; ++layer) {
                for (int level = 0; level < QRhi::MAX_LEVELS; ++level) {
                    const QVector<QRhiTextureSubresourceUploadDescription> &srd(u.subresDesc[layer][level]);
                    if (srd.isEmpty())
                        continue;
                    for (const QRhiTextureSubresourceUploadDescription &subresDesc : qAsConst(srd)) {
                        prepareUploadSubres(utexD, layer, level,
                                            subresDesc, &curOfs, mp, &copyInfos);
                    }
                }
            }
            vmaUnmapMemory(toVmaAllocator(allocator), a);
            vmaFlushAllocation(toVmaAllocator(allocator), a, 0, stagingSize);

            trackedImageBarrier(cbD, utexD, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            QVkCommandBuffer::Command cmd;
            cmd.cmd = QVkCommandBuffer::Command::CopyBufferToImage;
            cmd.args.copyBufferToImage.src = utexD->stagingBuffers[currentFrameSlot];
            cmd.args.copyBufferToImage.dst = utexD->image;
            cmd.args.copyBufferToImage.dstLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            cmd.args.copyBufferToImage.count = copyInfos.count();
            cmd.args.copyBufferToImage.bufferImageCopyIndex = cbD->pools.bufferImageCopy.count();
            cbD->pools.bufferImageCopy.append(copyInfos.constData(), copyInfos.count());
            cbD->commands.append(cmd);

            // no reuse of staging, this is intentional
            QRhiVulkan::DeferredReleaseEntry e;
            e.type = QRhiVulkan::DeferredReleaseEntry::StagingBuffer;
            e.lastActiveFrameSlot = currentFrameSlot;
            e.stagingBuffer.stagingBuffer = utexD->stagingBuffers[currentFrameSlot];
            e.stagingBuffer.stagingAllocation = utexD->stagingAllocations[currentFrameSlot];
            utexD->stagingBuffers[currentFrameSlot] = VK_NULL_HANDLE;
            utexD->stagingAllocations[currentFrameSlot] = nullptr;
            releaseQueue.append(e);
            QRHI_PROF_F(releaseTextureStagingArea(utexD, currentFrameSlot));

            // Similarly to buffers, transitioning away from DST is done later,
            // when a renderpass using the texture is encountered.

            utexD->lastActiveFrameSlot = currentFrameSlot;
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Copy) {
            Q_ASSERT(u.src && u.dst);
            if (u.src == u.dst) {
                qWarning("Texture copy with matching source and destination is not supported");
                continue;
            }
            QVkTexture *srcD = QRHI_RES(QVkTexture, u.src);
            QVkTexture *dstD = QRHI_RES(QVkTexture, u.dst);

            VkImageCopy region;
            memset(&region, 0, sizeof(region));

            region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.srcSubresource.mipLevel = uint32_t(u.desc.sourceLevel());
            region.srcSubresource.baseArrayLayer = uint32_t(u.desc.sourceLayer());
            region.srcSubresource.layerCount = 1;

            region.srcOffset.x = u.desc.sourceTopLeft().x();
            region.srcOffset.y = u.desc.sourceTopLeft().y();

            region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.dstSubresource.mipLevel = uint32_t(u.desc.destinationLevel());
            region.dstSubresource.baseArrayLayer = uint32_t(u.desc.destinationLayer());
            region.dstSubresource.layerCount = 1;

            region.dstOffset.x = u.desc.destinationTopLeft().x();
            region.dstOffset.y = u.desc.destinationTopLeft().y();

            const QSize mipSize = q->sizeForMipLevel(u.desc.sourceLevel(), srcD->m_pixelSize);
            const QSize copySize = u.desc.pixelSize().isEmpty() ? mipSize : u.desc.pixelSize();
            region.extent.width = uint32_t(copySize.width());
            region.extent.height = uint32_t(copySize.height());
            region.extent.depth = 1;

            trackedImageBarrier(cbD, srcD, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
            trackedImageBarrier(cbD, dstD, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            QVkCommandBuffer::Command cmd;
            cmd.cmd = QVkCommandBuffer::Command::CopyImage;
            cmd.args.copyImage.src = srcD->image;
            cmd.args.copyImage.srcLayout = srcD->usageState.layout;
            cmd.args.copyImage.dst = dstD->image;
            cmd.args.copyImage.dstLayout = dstD->usageState.layout;
            cmd.args.copyImage.desc = region;
            cbD->commands.append(cmd);

            srcD->lastActiveFrameSlot = dstD->lastActiveFrameSlot = currentFrameSlot;
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Read) {
            TextureReadback readback;
            readback.activeFrameSlot = currentFrameSlot;
            readback.desc = u.rb;
            readback.result = u.result;

            QVkTexture *texD = QRHI_RES(QVkTexture, u.rb.texture());
            QVkSwapChain *swapChainD = nullptr;
            if (texD) {
                if (texD->samples > VK_SAMPLE_COUNT_1_BIT) {
                    qWarning("Multisample texture cannot be read back");
                    continue;
                }
                readback.pixelSize = q->sizeForMipLevel(u.rb.level(), texD->m_pixelSize);
                readback.format = texD->m_format;
                texD->lastActiveFrameSlot = currentFrameSlot;
            } else {
                Q_ASSERT(currentSwapChain);
                swapChainD = QRHI_RES(QVkSwapChain, currentSwapChain);
                if (!swapChainD->supportsReadback) {
                    qWarning("Swapchain does not support readback");
                    continue;
                }
                readback.pixelSize = swapChainD->pixelSize;
                readback.format = colorTextureFormatFromVkFormat(swapChainD->colorFormat, nullptr);
                if (readback.format == QRhiTexture::UnknownFormat)
                    continue;

                // Multisample swapchains need nothing special since resolving
                // happens when ending a renderpass.
            }
            textureFormatInfo(readback.format, readback.pixelSize, nullptr, &readback.byteSize);

            // Create a host visible readback buffer.
            VkBufferCreateInfo bufferInfo;
            memset(&bufferInfo, 0, sizeof(bufferInfo));
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = readback.byteSize;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

            VmaAllocationCreateInfo allocInfo;
            memset(&allocInfo, 0, sizeof(allocInfo));
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;

            VmaAllocation allocation;
            VkResult err = vmaCreateBuffer(toVmaAllocator(allocator), &bufferInfo, &allocInfo, &readback.stagingBuf, &allocation, nullptr);
            if (err == VK_SUCCESS) {
                readback.stagingAlloc = allocation;
                QRHI_PROF_F(newReadbackBuffer(qint64(readback.stagingBuf),
                                              texD ? static_cast<QRhiResource *>(texD) : static_cast<QRhiResource *>(swapChainD),
                                              readback.byteSize));
            } else {
                qWarning("Failed to create readback buffer of size %u: %d", readback.byteSize, err);
                continue;
            }

            // Copy from the (optimal and not host visible) image into the buffer.
            VkBufferImageCopy copyDesc;
            memset(&copyDesc, 0, sizeof(copyDesc));
            copyDesc.bufferOffset = 0;
            copyDesc.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyDesc.imageSubresource.mipLevel = uint32_t(u.rb.level());
            copyDesc.imageSubresource.baseArrayLayer = uint32_t(u.rb.layer());
            copyDesc.imageSubresource.layerCount = 1;
            copyDesc.imageExtent.width = uint32_t(readback.pixelSize.width());
            copyDesc.imageExtent.height = uint32_t(readback.pixelSize.height());
            copyDesc.imageExtent.depth = 1;

            if (texD) {
                trackedImageBarrier(cbD, texD, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                    VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
                QVkCommandBuffer::Command cmd;
                cmd.cmd = QVkCommandBuffer::Command::CopyImageToBuffer;
                cmd.args.copyImageToBuffer.src = texD->image;
                cmd.args.copyImageToBuffer.srcLayout = texD->usageState.layout;
                cmd.args.copyImageToBuffer.dst = readback.stagingBuf;
                cmd.args.copyImageToBuffer.desc = copyDesc;
                cbD->commands.append(cmd);
            } else {
                // use the swapchain image
                QVkSwapChain::ImageResources &imageRes(swapChainD->imageRes[swapChainD->currentImageIndex]);
                VkImage image = imageRes.image;
                if (imageRes.lastUse != QVkSwapChain::ImageResources::ScImageUseTransferSource) {
                    if (imageRes.lastUse != QVkSwapChain::ImageResources::ScImageUseRender) {
                        qWarning("Attempted to read back undefined swapchain image content, "
                                 "results are undefined. (do a render pass first)");
                    }
                    subresourceBarrier(cbD, image,
                                       VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                       VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                       0, 1,
                                       0, 1);
                    imageRes.lastUse = QVkSwapChain::ImageResources::ScImageUseTransferSource;
                }

                QVkCommandBuffer::Command cmd;
                cmd.cmd = QVkCommandBuffer::Command::CopyImageToBuffer;
                cmd.args.copyImageToBuffer.src = image;
                cmd.args.copyImageToBuffer.srcLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                cmd.args.copyImageToBuffer.dst = readback.stagingBuf;
                cmd.args.copyImageToBuffer.desc = copyDesc;
                cbD->commands.append(cmd);
            }

            activeTextureReadbacks.append(readback);
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::GenMips) {
            QVkTexture *utexD = QRHI_RES(QVkTexture, u.dst);
            Q_ASSERT(utexD->m_flags.testFlag(QRhiTexture::UsedWithGenerateMips));
            int w = utexD->m_pixelSize.width();
            int h = utexD->m_pixelSize.height();

            VkImageLayout origLayout = utexD->usageState.layout;
            VkAccessFlags origAccess = utexD->usageState.access;
            VkPipelineStageFlags origStage = utexD->usageState.stage;
            if (!origStage)
                origStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

            for (int level = 1; level < int(utexD->mipLevelCount); ++level) {
                if (level == 1) {
                    subresourceBarrier(cbD, utexD->image,
                                       origLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                       origAccess, VK_ACCESS_TRANSFER_READ_BIT,
                                       origStage, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                       u.layer, 1,
                                       level - 1, 1);
                } else {
                    subresourceBarrier(cbD, utexD->image,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                       VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                                       VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                       u.layer, 1,
                                       level - 1, 1);
                }

                subresourceBarrier(cbD, utexD->image,
                                   origLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   origAccess, VK_ACCESS_TRANSFER_WRITE_BIT,
                                   origStage, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   u.layer, 1,
                                   level, 1);

                VkImageBlit region;
                memset(&region, 0, sizeof(region));

                region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.srcSubresource.mipLevel = uint32_t(level) - 1;
                region.srcSubresource.baseArrayLayer = uint32_t(u.layer);
                region.srcSubresource.layerCount = 1;

                region.srcOffsets[1].x = qMax(1, w);
                region.srcOffsets[1].y = qMax(1, h);
                region.srcOffsets[1].z = 1;

                region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.dstSubresource.mipLevel = uint32_t(level);
                region.dstSubresource.baseArrayLayer = uint32_t(u.layer);
                region.dstSubresource.layerCount = 1;

                region.dstOffsets[1].x = qMax(1, w >> 1);
                region.dstOffsets[1].y = qMax(1, h >> 1);
                region.dstOffsets[1].z = 1;

                QVkCommandBuffer::Command cmd;
                cmd.cmd = QVkCommandBuffer::Command::BlitImage;
                cmd.args.blitImage.src = utexD->image;
                cmd.args.blitImage.srcLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                cmd.args.blitImage.dst = utexD->image;
                cmd.args.blitImage.dstLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                cmd.args.blitImage.filter = VK_FILTER_LINEAR;
                cmd.args.blitImage.desc = region;
                cbD->commands.append(cmd);

                w >>= 1;
                h >>= 1;
            }

            if (utexD->mipLevelCount > 1) {
                subresourceBarrier(cbD, utexD->image,
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, origLayout,
                                   VK_ACCESS_TRANSFER_READ_BIT, origAccess,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT, origStage,
                                   u.layer, 1,
                                   0, int(utexD->mipLevelCount) - 1);
                subresourceBarrier(cbD, utexD->image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, origLayout,
                                   VK_ACCESS_TRANSFER_WRITE_BIT, origAccess,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT, origStage,
                                   u.layer, 1,
                                   int(utexD->mipLevelCount) - 1, 1);
            }

            utexD->lastActiveFrameSlot = currentFrameSlot;
        }
    }

    ud->free();
}

void QRhiVulkan::executeBufferHostWritesForSlot(QVkBuffer *bufD, int slot)
{
    if (bufD->pendingDynamicUpdates[slot].isEmpty())
        return;

    Q_ASSERT(bufD->m_type == QRhiBuffer::Dynamic);
    void *p = nullptr;
    VmaAllocation a = toVmaAllocation(bufD->allocations[slot]);
    // The vmaMap/Unmap are basically a no-op when persistently mapped since it
    // refcounts; this is great because we don't need to care if the allocation
    // was created as persistently mapped or not.
    VkResult err = vmaMapMemory(toVmaAllocator(allocator), a, &p);
    if (err != VK_SUCCESS) {
        qWarning("Failed to map buffer: %d", err);
        return;
    }
    int changeBegin = -1;
    int changeEnd = -1;
    for (const QRhiResourceUpdateBatchPrivate::BufferOp &u : qAsConst(bufD->pendingDynamicUpdates[slot])) {
        Q_ASSERT(bufD == QRHI_RES(QVkBuffer, u.buf));
        memcpy(static_cast<char *>(p) + u.offset, u.data.constData(), size_t(u.data.size()));
        if (changeBegin == -1 || u.offset < changeBegin)
            changeBegin = u.offset;
        if (changeEnd == -1 || u.offset + u.data.size() > changeEnd)
            changeEnd = u.offset + u.data.size();
    }
    vmaUnmapMemory(toVmaAllocator(allocator), a);
    if (changeBegin >= 0)
        vmaFlushAllocation(toVmaAllocator(allocator), a, VkDeviceSize(changeBegin), VkDeviceSize(changeEnd - changeBegin));

    bufD->pendingDynamicUpdates[slot].clear();
}

static void qrhivk_releaseBuffer(const QRhiVulkan::DeferredReleaseEntry &e, void *allocator)
{
    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i) {
        vmaDestroyBuffer(toVmaAllocator(allocator), e.buffer.buffers[i], toVmaAllocation(e.buffer.allocations[i]));
        vmaDestroyBuffer(toVmaAllocator(allocator), e.buffer.stagingBuffers[i], toVmaAllocation(e.buffer.stagingAllocations[i]));
    }
}

static void qrhivk_releaseRenderBuffer(const QRhiVulkan::DeferredReleaseEntry &e, VkDevice dev, QVulkanDeviceFunctions *df)
{
    df->vkDestroyImageView(dev, e.renderBuffer.imageView, nullptr);
    df->vkDestroyImage(dev, e.renderBuffer.image, nullptr);
    df->vkFreeMemory(dev, e.renderBuffer.memory, nullptr);
}

static void qrhivk_releaseTexture(const QRhiVulkan::DeferredReleaseEntry &e, VkDevice dev, QVulkanDeviceFunctions *df, void *allocator)
{
    df->vkDestroyImageView(dev, e.texture.imageView, nullptr);
    vmaDestroyImage(toVmaAllocator(allocator), e.texture.image, toVmaAllocation(e.texture.allocation));
    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i)
        vmaDestroyBuffer(toVmaAllocator(allocator), e.texture.stagingBuffers[i], toVmaAllocation(e.texture.stagingAllocations[i]));
    for (int i = 0; i < QRhi::MAX_LEVELS; ++i) {
        if (e.texture.extraImageViews[i])
            df->vkDestroyImageView(dev, e.texture.extraImageViews[i], nullptr);
    }
}

static void qrhivk_releaseSampler(const QRhiVulkan::DeferredReleaseEntry &e, VkDevice dev, QVulkanDeviceFunctions *df)
{
    df->vkDestroySampler(dev, e.sampler.sampler, nullptr);
}

void QRhiVulkan::executeDeferredReleases(bool forced)
{
    for (int i = releaseQueue.count() - 1; i >= 0; --i) {
        const QRhiVulkan::DeferredReleaseEntry &e(releaseQueue[i]);
        if (forced || currentFrameSlot == e.lastActiveFrameSlot || e.lastActiveFrameSlot < 0) {
            switch (e.type) {
            case QRhiVulkan::DeferredReleaseEntry::Pipeline:
                df->vkDestroyPipeline(dev, e.pipelineState.pipeline, nullptr);
                df->vkDestroyPipelineLayout(dev, e.pipelineState.layout, nullptr);
                break;
            case QRhiVulkan::DeferredReleaseEntry::ShaderResourceBindings:
                df->vkDestroyDescriptorSetLayout(dev, e.shaderResourceBindings.layout, nullptr);
                if (e.shaderResourceBindings.poolIndex >= 0) {
                    descriptorPools[e.shaderResourceBindings.poolIndex].refCount -= 1;
                    Q_ASSERT(descriptorPools[e.shaderResourceBindings.poolIndex].refCount >= 0);
                }
                break;
            case QRhiVulkan::DeferredReleaseEntry::Buffer:
                qrhivk_releaseBuffer(e, allocator);
                break;
            case QRhiVulkan::DeferredReleaseEntry::RenderBuffer:
                qrhivk_releaseRenderBuffer(e, dev, df);
                break;
            case QRhiVulkan::DeferredReleaseEntry::Texture:
                qrhivk_releaseTexture(e, dev, df, allocator);
                break;
            case QRhiVulkan::DeferredReleaseEntry::Sampler:
                qrhivk_releaseSampler(e, dev, df);
                break;
            case QRhiVulkan::DeferredReleaseEntry::TextureRenderTarget:
                df->vkDestroyFramebuffer(dev, e.textureRenderTarget.fb, nullptr);
                for (int att = 0; att < QVkRenderTargetData::MAX_COLOR_ATTACHMENTS; ++att) {
                    df->vkDestroyImageView(dev, e.textureRenderTarget.rtv[att], nullptr);
                    df->vkDestroyImageView(dev, e.textureRenderTarget.resrtv[att], nullptr);
                }
                break;
            case QRhiVulkan::DeferredReleaseEntry::RenderPass:
                df->vkDestroyRenderPass(dev, e.renderPass.rp, nullptr);
                break;
            case QRhiVulkan::DeferredReleaseEntry::StagingBuffer:
                vmaDestroyBuffer(toVmaAllocator(allocator), e.stagingBuffer.stagingBuffer, toVmaAllocation(e.stagingBuffer.stagingAllocation));
                break;
            case QRhiVulkan::DeferredReleaseEntry::CommandBuffer:
                df->vkFreeCommandBuffers(dev, cmdPool, 1, &e.commandBuffer.cb);
                break;
            default:
                Q_UNREACHABLE();
                break;
            }
            releaseQueue.removeAt(i);
        }
    }
}

void QRhiVulkan::finishActiveReadbacks(bool forced)
{
    QVarLengthArray<std::function<void()>, 4> completedCallbacks;
    QRhiProfilerPrivate *rhiP = profilerPrivateOrNull();

    for (int i = activeTextureReadbacks.count() - 1; i >= 0; --i) {
        const QRhiVulkan::TextureReadback &readback(activeTextureReadbacks[i]);
        if (forced || currentFrameSlot == readback.activeFrameSlot || readback.activeFrameSlot < 0) {
            readback.result->format = readback.format;
            readback.result->pixelSize = readback.pixelSize;
            VmaAllocation a = toVmaAllocation(readback.stagingAlloc);
            void *p = nullptr;
            VkResult err = vmaMapMemory(toVmaAllocator(allocator), a, &p);
            if (err == VK_SUCCESS && p) {
                readback.result->data.resize(int(readback.byteSize));
                memcpy(readback.result->data.data(), p, readback.byteSize);
                vmaUnmapMemory(toVmaAllocator(allocator), a);
            } else {
                qWarning("Failed to map texture readback buffer of size %u: %d", readback.byteSize, err);
            }

            vmaDestroyBuffer(toVmaAllocator(allocator), readback.stagingBuf, a);
            QRHI_PROF_F(releaseReadbackBuffer(qint64(readback.stagingBuf)));

            if (readback.result->completed)
                completedCallbacks.append(readback.result->completed);

            activeTextureReadbacks.removeAt(i);
        }
    }

    for (int i = activeBufferReadbacks.count() - 1; i >= 0; --i) {
        const QRhiVulkan::BufferReadback &readback(activeBufferReadbacks[i]);
        if (forced || currentFrameSlot == readback.activeFrameSlot || readback.activeFrameSlot < 0) {
            VmaAllocation a = toVmaAllocation(readback.stagingAlloc);
            void *p = nullptr;
            VkResult err = vmaMapMemory(toVmaAllocator(allocator), a, &p);
            if (err == VK_SUCCESS && p) {
                readback.result->data.resize(readback.byteSize);
                memcpy(readback.result->data.data(), p, size_t(readback.byteSize));
                vmaUnmapMemory(toVmaAllocator(allocator), a);
            } else {
                qWarning("Failed to map buffer readback buffer of size %d: %d", readback.byteSize, err);
            }

            vmaDestroyBuffer(toVmaAllocator(allocator), readback.stagingBuf, a);
            QRHI_PROF_F(releaseReadbackBuffer(qint64(readback.stagingBuf)));

            if (readback.result->completed)
                completedCallbacks.append(readback.result->completed);

            activeBufferReadbacks.removeAt(i);
        }
    }

    for (auto f : completedCallbacks)
        f();
}

static struct {
    VkSampleCountFlagBits mask;
    int count;
} qvk_sampleCounts[] = {
    // keep this sorted by 'count'
    { VK_SAMPLE_COUNT_1_BIT, 1 },
    { VK_SAMPLE_COUNT_2_BIT, 2 },
    { VK_SAMPLE_COUNT_4_BIT, 4 },
    { VK_SAMPLE_COUNT_8_BIT, 8 },
    { VK_SAMPLE_COUNT_16_BIT, 16 },
    { VK_SAMPLE_COUNT_32_BIT, 32 },
    { VK_SAMPLE_COUNT_64_BIT, 64 }
};

QVector<int> QRhiVulkan::supportedSampleCounts() const
{
    const VkPhysicalDeviceLimits *limits = &physDevProperties.limits;
    VkSampleCountFlags color = limits->framebufferColorSampleCounts;
    VkSampleCountFlags depth = limits->framebufferDepthSampleCounts;
    VkSampleCountFlags stencil = limits->framebufferStencilSampleCounts;
    QVector<int> result;

    for (const auto &qvk_sampleCount : qvk_sampleCounts) {
        if ((color & qvk_sampleCount.mask)
                && (depth & qvk_sampleCount.mask)
                && (stencil & qvk_sampleCount.mask))
        {
            result.append(qvk_sampleCount.count);
        }
    }

    return result;
}

VkSampleCountFlagBits QRhiVulkan::effectiveSampleCount(int sampleCount)
{
    // Stay compatible with QSurfaceFormat and friends where samples == 0 means the same as 1.
    sampleCount = qBound(1, sampleCount, 64);

    if (!supportedSampleCounts().contains(sampleCount)) {
        qWarning("Attempted to set unsupported sample count %d", sampleCount);
        return VK_SAMPLE_COUNT_1_BIT;
    }

    for (const auto &qvk_sampleCount : qvk_sampleCounts) {
        if (qvk_sampleCount.count == sampleCount)
            return qvk_sampleCount.mask;
    }

    Q_UNREACHABLE();
    return VK_SAMPLE_COUNT_1_BIT;
}

void QRhiVulkan::enqueueTransitionPassResources(QVkCommandBuffer *cbD)
{
    cbD->passResTrackers.append(QRhiPassResourceTracker());
    cbD->currentPassResTrackerIndex = cbD->passResTrackers.count() - 1;

    QVkCommandBuffer::Command cmd;
    cmd.cmd = QVkCommandBuffer::Command::TransitionPassResources;
    cmd.args.transitionResources.trackerIndex = cbD->passResTrackers.count() - 1;
    cbD->commands.append(cmd);
}

void QRhiVulkan::recordPrimaryCommandBuffer(QVkCommandBuffer *cbD)
{
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::NoPass);

    for (QVkCommandBuffer::Command &cmd : cbD->commands) {
        switch (cmd.cmd) {
        case QVkCommandBuffer::Command::CopyBuffer:
            df->vkCmdCopyBuffer(cbD->cb, cmd.args.copyBuffer.src, cmd.args.copyBuffer.dst,
                                1, &cmd.args.copyBuffer.desc);
            break;
        case QVkCommandBuffer::Command::CopyBufferToImage:
            df->vkCmdCopyBufferToImage(cbD->cb, cmd.args.copyBufferToImage.src, cmd.args.copyBufferToImage.dst,
                                       cmd.args.copyBufferToImage.dstLayout,
                                       uint32_t(cmd.args.copyBufferToImage.count),
                                       cbD->pools.bufferImageCopy.constData() + cmd.args.copyBufferToImage.bufferImageCopyIndex);
            break;
        case QVkCommandBuffer::Command::CopyImage:
            df->vkCmdCopyImage(cbD->cb, cmd.args.copyImage.src, cmd.args.copyImage.srcLayout,
                               cmd.args.copyImage.dst, cmd.args.copyImage.dstLayout,
                               1, &cmd.args.copyImage.desc);
            break;
        case QVkCommandBuffer::Command::CopyImageToBuffer:
            df->vkCmdCopyImageToBuffer(cbD->cb, cmd.args.copyImageToBuffer.src, cmd.args.copyImageToBuffer.srcLayout,
                                       cmd.args.copyImageToBuffer.dst,
                                       1, &cmd.args.copyImageToBuffer.desc);
            break;
        case QVkCommandBuffer::Command::ImageBarrier:
            df->vkCmdPipelineBarrier(cbD->cb, cmd.args.imageBarrier.srcStageMask, cmd.args.imageBarrier.dstStageMask,
                                     0, 0, nullptr, 0, nullptr,
                                     cmd.args.imageBarrier.count, cbD->pools.imageBarrier.constData() + cmd.args.imageBarrier.index);
            break;
        case QVkCommandBuffer::Command::BufferBarrier:
            df->vkCmdPipelineBarrier(cbD->cb, cmd.args.bufferBarrier.srcStageMask, cmd.args.bufferBarrier.dstStageMask,
                                     0, 0, nullptr,
                                     cmd.args.bufferBarrier.count, cbD->pools.bufferBarrier.constData() + cmd.args.bufferBarrier.index,
                                     0, nullptr);
            break;
        case QVkCommandBuffer::Command::BlitImage:
            df->vkCmdBlitImage(cbD->cb, cmd.args.blitImage.src, cmd.args.blitImage.srcLayout,
                               cmd.args.blitImage.dst, cmd.args.blitImage.dstLayout,
                               1, &cmd.args.blitImage.desc,
                               cmd.args.blitImage.filter);
            break;
        case QVkCommandBuffer::Command::BeginRenderPass:
            cmd.args.beginRenderPass.desc.pClearValues = cbD->pools.clearValue.constData() + cmd.args.beginRenderPass.clearValueIndex;
            df->vkCmdBeginRenderPass(cbD->cb, &cmd.args.beginRenderPass.desc,
                                     cbD->useSecondaryCb ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : VK_SUBPASS_CONTENTS_INLINE);
            break;
        case QVkCommandBuffer::Command::EndRenderPass:
            df->vkCmdEndRenderPass(cbD->cb);
            break;
        case QVkCommandBuffer::Command::BindPipeline:
            df->vkCmdBindPipeline(cbD->cb, cmd.args.bindPipeline.bindPoint, cmd.args.bindPipeline.pipeline);
            break;
        case QVkCommandBuffer::Command::BindDescriptorSet:
        {
            const uint32_t *offsets = nullptr;
            if (cmd.args.bindDescriptorSet.dynamicOffsetCount > 0)
                offsets = cbD->pools.dynamicOffset.constData() + cmd.args.bindDescriptorSet.dynamicOffsetIndex;
            df->vkCmdBindDescriptorSets(cbD->cb, cmd.args.bindDescriptorSet.bindPoint,
                                        cmd.args.bindDescriptorSet.pipelineLayout,
                                        0, 1, &cmd.args.bindDescriptorSet.descSet,
                                        uint32_t(cmd.args.bindDescriptorSet.dynamicOffsetCount),
                                        offsets);
        }
            break;
        case QVkCommandBuffer::Command::BindVertexBuffer:
            df->vkCmdBindVertexBuffers(cbD->cb, uint32_t(cmd.args.bindVertexBuffer.startBinding),
                                       uint32_t(cmd.args.bindVertexBuffer.count),
                                       cbD->pools.vertexBuffer.constData() + cmd.args.bindVertexBuffer.vertexBufferIndex,
                                       cbD->pools.vertexBufferOffset.constData() + cmd.args.bindVertexBuffer.vertexBufferOffsetIndex);
            break;
        case QVkCommandBuffer::Command::BindIndexBuffer:
            df->vkCmdBindIndexBuffer(cbD->cb, cmd.args.bindIndexBuffer.buf,
                                     cmd.args.bindIndexBuffer.ofs, cmd.args.bindIndexBuffer.type);
            break;
        case QVkCommandBuffer::Command::SetViewport:
            df->vkCmdSetViewport(cbD->cb, 0, 1, &cmd.args.setViewport.viewport);
            break;
        case QVkCommandBuffer::Command::SetScissor:
            df->vkCmdSetScissor(cbD->cb, 0, 1, &cmd.args.setScissor.scissor);
            break;
        case QVkCommandBuffer::Command::SetBlendConstants:
            df->vkCmdSetBlendConstants(cbD->cb, cmd.args.setBlendConstants.c);
            break;
        case QVkCommandBuffer::Command::SetStencilRef:
            df->vkCmdSetStencilReference(cbD->cb, VK_STENCIL_FRONT_AND_BACK, cmd.args.setStencilRef.ref);
            break;
        case QVkCommandBuffer::Command::Draw:
            df->vkCmdDraw(cbD->cb, cmd.args.draw.vertexCount, cmd.args.draw.instanceCount,
                          cmd.args.draw.firstVertex, cmd.args.draw.firstInstance);
            break;
        case QVkCommandBuffer::Command::DrawIndexed:
            df->vkCmdDrawIndexed(cbD->cb, cmd.args.drawIndexed.indexCount, cmd.args.drawIndexed.instanceCount,
                                 cmd.args.drawIndexed.firstIndex, cmd.args.drawIndexed.vertexOffset,
                                 cmd.args.drawIndexed.firstInstance);
            break;
        case QVkCommandBuffer::Command::DebugMarkerBegin:
            cmd.args.debugMarkerBegin.marker.pMarkerName =
                    cbD->pools.debugMarkerData[cmd.args.debugMarkerBegin.markerNameIndex].constData();
            vkCmdDebugMarkerBegin(cbD->cb, &cmd.args.debugMarkerBegin.marker);
            break;
        case QVkCommandBuffer::Command::DebugMarkerEnd:
            vkCmdDebugMarkerEnd(cbD->cb);
            break;
        case QVkCommandBuffer::Command::DebugMarkerInsert:
            cmd.args.debugMarkerInsert.marker.pMarkerName =
                    cbD->pools.debugMarkerData[cmd.args.debugMarkerInsert.markerNameIndex].constData();
            vkCmdDebugMarkerInsert(cbD->cb, &cmd.args.debugMarkerInsert.marker);
            break;
        case QVkCommandBuffer::Command::TransitionPassResources:
            recordTransitionPassResources(cbD, cbD->passResTrackers[cmd.args.transitionResources.trackerIndex]);
            break;
        case QVkCommandBuffer::Command::Dispatch:
            df->vkCmdDispatch(cbD->cb, uint32_t(cmd.args.dispatch.x), uint32_t(cmd.args.dispatch.y), uint32_t(cmd.args.dispatch.z));
            break;
        case QVkCommandBuffer::Command::ExecuteSecondary:
            df->vkCmdExecuteCommands(cbD->cb, 1, &cmd.args.executeSecondary.cb);
            break;
        default:
            break;
        }
    }
}

static inline VkAccessFlags toVkAccess(QRhiPassResourceTracker::BufferAccess access)
{
    switch (access) {
    case QRhiPassResourceTracker::BufVertexInput:
        return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    case QRhiPassResourceTracker::BufIndexRead:
        return VK_ACCESS_INDEX_READ_BIT;
    case QRhiPassResourceTracker::BufUniformRead:
        return VK_ACCESS_UNIFORM_READ_BIT;
    case QRhiPassResourceTracker::BufStorageLoad:
        return VK_ACCESS_SHADER_READ_BIT;
    case QRhiPassResourceTracker::BufStorageStore:
        return VK_ACCESS_SHADER_WRITE_BIT;
    case QRhiPassResourceTracker::BufStorageLoadStore:
        return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    default:
        Q_UNREACHABLE();
        break;
    }
    return 0;
}

static inline VkPipelineStageFlags toVkPipelineStage(QRhiPassResourceTracker::BufferStage stage)
{
    switch (stage) {
    case QRhiPassResourceTracker::BufVertexInputStage:
        return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
    case QRhiPassResourceTracker::BufVertexStage:
        return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    case QRhiPassResourceTracker::BufFragmentStage:
        return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    case QRhiPassResourceTracker::BufComputeStage:
        return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    default:
        Q_UNREACHABLE();
        break;
    }
    return 0;
}

static inline QVkBuffer::UsageState toVkBufferUsageState(QRhiPassResourceTracker::UsageState usage)
{
    QVkBuffer::UsageState u;
    u.access = VkAccessFlags(usage.access);
    u.stage = VkPipelineStageFlags(usage.stage);
    return u;
}

static inline VkImageLayout toVkLayout(QRhiPassResourceTracker::TextureAccess access)
{
    switch (access) {
    case QRhiPassResourceTracker::TexSample:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case QRhiPassResourceTracker::TexColorOutput:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case QRhiPassResourceTracker::TexDepthOutput:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case QRhiPassResourceTracker::TexStorageLoad:
    case QRhiPassResourceTracker::TexStorageStore:
    case QRhiPassResourceTracker::TexStorageLoadStore:
        return VK_IMAGE_LAYOUT_GENERAL;
    default:
        Q_UNREACHABLE();
        break;
    }
    return VK_IMAGE_LAYOUT_GENERAL;
}

static inline VkAccessFlags toVkAccess(QRhiPassResourceTracker::TextureAccess access)
{
    switch (access) {
    case QRhiPassResourceTracker::TexSample:
        return VK_ACCESS_SHADER_READ_BIT;
    case QRhiPassResourceTracker::TexColorOutput:
        return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    case QRhiPassResourceTracker::TexDepthOutput:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    case QRhiPassResourceTracker::TexStorageLoad:
        return VK_ACCESS_SHADER_READ_BIT;
    case QRhiPassResourceTracker::TexStorageStore:
        return VK_ACCESS_SHADER_WRITE_BIT;
    case QRhiPassResourceTracker::TexStorageLoadStore:
        return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    default:
        Q_UNREACHABLE();
        break;
    }
    return 0;
}

static inline VkPipelineStageFlags toVkPipelineStage(QRhiPassResourceTracker::TextureStage stage)
{
    switch (stage) {
    case QRhiPassResourceTracker::TexVertexStage:
        return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    case QRhiPassResourceTracker::TexFragmentStage:
        return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    case QRhiPassResourceTracker::TexColorOutputStage:
        return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    case QRhiPassResourceTracker::TexDepthOutputStage:
        return VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    case QRhiPassResourceTracker::TexComputeStage:
        return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    default:
        Q_UNREACHABLE();
        break;
    }
    return 0;
}

static inline QVkTexture::UsageState toVkTextureUsageState(QRhiPassResourceTracker::UsageState usage)
{
    QVkTexture::UsageState u;
    u.layout = VkImageLayout(usage.layout);
    u.access = VkAccessFlags(usage.access);
    u.stage = VkPipelineStageFlags(usage.stage);
    return u;
}

void QRhiVulkan::trackedRegisterBuffer(QRhiPassResourceTracker *passResTracker,
                                       QVkBuffer *bufD,
                                       int slot,
                                       QRhiPassResourceTracker::BufferAccess access,
                                       QRhiPassResourceTracker::BufferStage stage)
{
    QVkBuffer::UsageState &u(bufD->usageState[slot]);
    passResTracker->registerBuffer(bufD, slot, &access, &stage, toPassTrackerUsageState(u));
    u.access = toVkAccess(access);
    u.stage = toVkPipelineStage(stage);
}

void QRhiVulkan::trackedRegisterTexture(QRhiPassResourceTracker *passResTracker,
                                        QVkTexture *texD,
                                        QRhiPassResourceTracker::TextureAccess access,
                                        QRhiPassResourceTracker::TextureStage stage)
{
    QVkTexture::UsageState &u(texD->usageState);
    passResTracker->registerTexture(texD, &access, &stage, toPassTrackerUsageState(u));
    u.layout = toVkLayout(access);
    u.access = toVkAccess(access);
    u.stage = toVkPipelineStage(stage);
}

void QRhiVulkan::recordTransitionPassResources(QVkCommandBuffer *cbD, const QRhiPassResourceTracker &tracker)
{
    if (tracker.isEmpty())
        return;

    for (auto it = tracker.cbeginBuffers(), itEnd = tracker.cendBuffers(); it != itEnd; ++it) {
        QVkBuffer *bufD = QRHI_RES(QVkBuffer, it.key());
        VkAccessFlags access = toVkAccess(it->access);
        VkPipelineStageFlags stage = toVkPipelineStage(it->stage);
        QVkBuffer::UsageState s = toVkBufferUsageState(it->stateAtPassBegin);
        if (!s.stage)
            continue;
        if (s.access == access && s.stage == stage) {
            if (!accessIsWrite(access))
                continue;
        }
        VkBufferMemoryBarrier bufMemBarrier;
        memset(&bufMemBarrier, 0, sizeof(bufMemBarrier));
        bufMemBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufMemBarrier.srcAccessMask = s.access;
        bufMemBarrier.dstAccessMask = access;
        bufMemBarrier.buffer = bufD->buffers[it->slot];
        bufMemBarrier.size = VK_WHOLE_SIZE;
        df->vkCmdPipelineBarrier(cbD->cb, s.stage, stage, 0,
                                 0, nullptr,
                                 1, &bufMemBarrier,
                                 0, nullptr);
    }

    for (auto it = tracker.cbeginTextures(), itEnd = tracker.cendTextures(); it != itEnd; ++it) {
        QVkTexture *texD = QRHI_RES(QVkTexture, it.key());
        VkImageLayout layout = toVkLayout(it->access);
        VkAccessFlags access = toVkAccess(it->access);
        VkPipelineStageFlags stage = toVkPipelineStage(it->stage);
        QVkTexture::UsageState s = toVkTextureUsageState(it->stateAtPassBegin);
        if (s.access == access && s.stage == stage && s.layout == layout) {
            if (!accessIsWrite(access))
                continue;
        }
        VkImageMemoryBarrier barrier;
        memset(&barrier, 0, sizeof(barrier));
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.subresourceRange.aspectMask = !isDepthTextureFormat(texD->m_format)
                ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        barrier.oldLayout = s.layout; // new textures have this set to PREINITIALIZED
        barrier.newLayout = layout;
        barrier.srcAccessMask = s.access; // may be 0 but that's fine
        barrier.dstAccessMask = access;
        barrier.image = texD->image;
        VkPipelineStageFlags srcStage = s.stage;
        // stage mask cannot be 0
        if (!srcStage)
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        df->vkCmdPipelineBarrier(cbD->cb, srcStage, stage, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
    }
}

QRhiSwapChain *QRhiVulkan::createSwapChain()
{
    return new QVkSwapChain(this);
}

QRhiBuffer *QRhiVulkan::createBuffer(QRhiBuffer::Type type, QRhiBuffer::UsageFlags usage, int size)
{
    return new QVkBuffer(this, type, usage, size);
}

int QRhiVulkan::ubufAlignment() const
{
    return int(ubufAlign); // typically 256 (bytes)
}

bool QRhiVulkan::isYUpInFramebuffer() const
{
    return false;
}

bool QRhiVulkan::isYUpInNDC() const
{
    return false;
}

bool QRhiVulkan::isClipDepthZeroToOne() const
{
    return true;
}

QMatrix4x4 QRhiVulkan::clipSpaceCorrMatrix() const
{
    // See https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/

    static QMatrix4x4 m;
    if (m.isIdentity()) {
        // NB the ctor takes row-major
        m = QMatrix4x4(1.0f, 0.0f, 0.0f, 0.0f,
                       0.0f, -1.0f, 0.0f, 0.0f,
                       0.0f, 0.0f, 0.5f, 0.5f,
                       0.0f, 0.0f, 0.0f, 1.0f);
    }
    return m;
}

bool QRhiVulkan::isTextureFormatSupported(QRhiTexture::Format format, QRhiTexture::Flags flags) const
{
    // Note that with some SDKs the validation layer gives an odd warning about
    // BC not being supported, even when our check here succeeds. Not much we
    // can do about that.
    if (format >= QRhiTexture::BC1 && format <= QRhiTexture::BC7) {
        if (!physDevFeatures.textureCompressionBC)
            return false;
    }

    if (format >= QRhiTexture::ETC2_RGB8 && format <= QRhiTexture::ETC2_RGBA8) {
        if (!physDevFeatures.textureCompressionETC2)
            return false;
    }

    if (format >= QRhiTexture::ASTC_4x4 && format <= QRhiTexture::ASTC_12x12) {
        if (!physDevFeatures.textureCompressionASTC_LDR)
            return false;
    }

    VkFormat vkformat = toVkTextureFormat(format, flags);
    VkFormatProperties props;
    f->vkGetPhysicalDeviceFormatProperties(physDev, vkformat, &props);
    return (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;
}

bool QRhiVulkan::isFeatureSupported(QRhi::Feature feature) const
{
    switch (feature) {
    case QRhi::MultisampleTexture:
        return true;
    case QRhi::MultisampleRenderBuffer:
        return true;
    case QRhi::DebugMarkers:
        return debugMarkersAvailable;
    case QRhi::Timestamps:
        return timestampValidBits != 0;
    case QRhi::Instancing:
        return true;
    case QRhi::CustomInstanceStepRate:
        return vertexAttribDivisorAvailable;
    case QRhi::PrimitiveRestart:
        return true;
    case QRhi::NonDynamicUniformBuffers:
        return true;
    case QRhi::NonFourAlignedEffectiveIndexBufferOffset:
        return true;
    case QRhi::NPOTTextureRepeat:
        return true;
    case QRhi::RedOrAlpha8IsRed:
        return true;
    case QRhi::ElementIndexUint:
        return true;
    case QRhi::Compute:
        return hasCompute;
    case QRhi::WideLines:
        return hasWideLines;
    case QRhi::VertexShaderPointSize:
        return true;
    case QRhi::BaseVertex:
        return true;
    case QRhi::BaseInstance:
        return true;
    case QRhi::TriangleFanTopology:
        return true;
    case QRhi::ReadBackNonUniformBuffer:
        return true;
    case QRhi::ReadBackNonBaseMipLevel:
        return true;
    case QRhi::TexelFetch:
        return true;
    default:
        Q_UNREACHABLE();
        return false;
    }
}

int QRhiVulkan::resourceLimit(QRhi::ResourceLimit limit) const
{
    switch (limit) {
    case QRhi::TextureSizeMin:
        return 1;
    case QRhi::TextureSizeMax:
        return int(physDevProperties.limits.maxImageDimension2D);
    case QRhi::MaxColorAttachments:
        return int(physDevProperties.limits.maxColorAttachments);
    case QRhi::FramesInFlight:
        return QVK_FRAMES_IN_FLIGHT;
    case QRhi::MaxAsyncReadbackFrames:
        return QVK_FRAMES_IN_FLIGHT;
    default:
        Q_UNREACHABLE();
        return 0;
    }
}

const QRhiNativeHandles *QRhiVulkan::nativeHandles()
{
    return &nativeHandlesStruct;
}

void QRhiVulkan::sendVMemStatsToProfiler()
{
    QRhiProfilerPrivate *rhiP = profilerPrivateOrNull();
    if (!rhiP)
        return;

    VmaStats stats;
    vmaCalculateStats(toVmaAllocator(allocator), &stats);
    QRHI_PROF_F(vmemStat(stats.total.blockCount, stats.total.allocationCount,
                         quint32(stats.total.usedBytes), quint32(stats.total.unusedBytes)));
}

bool QRhiVulkan::makeThreadLocalNativeContextCurrent()
{
    // not applicable
    return false;
}

void QRhiVulkan::releaseCachedResources()
{
    // nothing to do here
}

bool QRhiVulkan::isDeviceLost() const
{
    return deviceLost;
}

QRhiRenderBuffer *QRhiVulkan::createRenderBuffer(QRhiRenderBuffer::Type type, const QSize &pixelSize,
                                                 int sampleCount, QRhiRenderBuffer::Flags flags)
{
    return new QVkRenderBuffer(this, type, pixelSize, sampleCount, flags);
}

QRhiTexture *QRhiVulkan::createTexture(QRhiTexture::Format format, const QSize &pixelSize,
                                       int sampleCount, QRhiTexture::Flags flags)
{
    return new QVkTexture(this, format, pixelSize, sampleCount, flags);
}

QRhiSampler *QRhiVulkan::createSampler(QRhiSampler::Filter magFilter, QRhiSampler::Filter minFilter,
                                       QRhiSampler::Filter mipmapMode,
                                       QRhiSampler::AddressMode u, QRhiSampler::AddressMode v, QRhiSampler::AddressMode w)
{
    return new QVkSampler(this, magFilter, minFilter, mipmapMode, u, v, w);
}

QRhiTextureRenderTarget *QRhiVulkan::createTextureRenderTarget(const QRhiTextureRenderTargetDescription &desc,
                                                               QRhiTextureRenderTarget::Flags flags)
{
    return new QVkTextureRenderTarget(this, desc, flags);
}

QRhiGraphicsPipeline *QRhiVulkan::createGraphicsPipeline()
{
    return new QVkGraphicsPipeline(this);
}

QRhiComputePipeline *QRhiVulkan::createComputePipeline()
{
    return new QVkComputePipeline(this);
}

QRhiShaderResourceBindings *QRhiVulkan::createShaderResourceBindings()
{
    return new QVkShaderResourceBindings(this);
}

void QRhiVulkan::setGraphicsPipeline(QRhiCommandBuffer *cb, QRhiGraphicsPipeline *ps)
{
    QVkGraphicsPipeline *psD = QRHI_RES(QVkGraphicsPipeline, ps);
    Q_ASSERT(psD->pipeline);
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::RenderPass);

    if (cbD->currentGraphicsPipeline != ps || cbD->currentPipelineGeneration != psD->generation) {
        if (cbD->useSecondaryCb) {
            df->vkCmdBindPipeline(cbD->secondaryCbs.last(), VK_PIPELINE_BIND_POINT_GRAPHICS, psD->pipeline);
        } else {
            QVkCommandBuffer::Command cmd;
            cmd.cmd = QVkCommandBuffer::Command::BindPipeline;
            cmd.args.bindPipeline.bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            cmd.args.bindPipeline.pipeline = psD->pipeline;
            cbD->commands.append(cmd);
        }

        cbD->currentGraphicsPipeline = ps;
        cbD->currentComputePipeline = nullptr;
        cbD->currentPipelineGeneration = psD->generation;
    }

    psD->lastActiveFrameSlot = currentFrameSlot;
}

void QRhiVulkan::setShaderResources(QRhiCommandBuffer *cb, QRhiShaderResourceBindings *srb,
                                    int dynamicOffsetCount,
                                    const QRhiCommandBuffer::DynamicOffset *dynamicOffsets)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass != QVkCommandBuffer::NoPass);
    QVkGraphicsPipeline *gfxPsD = QRHI_RES(QVkGraphicsPipeline, cbD->currentGraphicsPipeline);
    QVkComputePipeline *compPsD = QRHI_RES(QVkComputePipeline, cbD->currentComputePipeline);

    if (!srb) {
        if (gfxPsD)
            srb = gfxPsD->m_shaderResourceBindings;
        else
            srb = compPsD->m_shaderResourceBindings;
    }

    QVkShaderResourceBindings *srbD = QRHI_RES(QVkShaderResourceBindings, srb);
    bool hasSlottedResourceInSrb = false;
    bool hasDynamicOffsetInSrb = false;

    for (const QRhiShaderResourceBinding &binding : qAsConst(srbD->sortedBindings)) {
        const QRhiShaderResourceBinding::Data *b = binding.data();
        switch (b->type) {
        case QRhiShaderResourceBinding::UniformBuffer:
            if (QRHI_RES(QVkBuffer, b->u.ubuf.buf)->m_type == QRhiBuffer::Dynamic)
                hasSlottedResourceInSrb = true;
            if (b->u.ubuf.hasDynamicOffset)
                hasDynamicOffsetInSrb = true;
            break;
        default:
            break;
        }
    }

    const int descSetIdx = hasSlottedResourceInSrb ? currentFrameSlot : 0;
    bool rewriteDescSet = false;

    // Do host writes and mark referenced shader resources as in-use.
    // Also prepare to ensure the descriptor set we are going to bind refers to up-to-date Vk objects.
    for (int i = 0, ie = srbD->sortedBindings.count(); i != ie; ++i) {
        const QRhiShaderResourceBinding::Data *b = srbD->sortedBindings.at(i).data();
        QVkShaderResourceBindings::BoundResourceData &bd(srbD->boundResourceData[descSetIdx][i]);
        QRhiPassResourceTracker &passResTracker(cbD->passResTrackers[cbD->currentPassResTrackerIndex]);
        switch (b->type) {
        case QRhiShaderResourceBinding::UniformBuffer:
        {
            QVkBuffer *bufD = QRHI_RES(QVkBuffer, b->u.ubuf.buf);
            Q_ASSERT(bufD->m_usage.testFlag(QRhiBuffer::UniformBuffer));

            if (bufD->m_type == QRhiBuffer::Dynamic)
                executeBufferHostWritesForSlot(bufD, currentFrameSlot);

            bufD->lastActiveFrameSlot = currentFrameSlot;
            trackedRegisterBuffer(&passResTracker, bufD, bufD->m_type == QRhiBuffer::Dynamic ? currentFrameSlot : 0,
                                  QRhiPassResourceTracker::BufUniformRead,
                                  QRhiPassResourceTracker::toPassTrackerBufferStage(b->stage));

            // Check both the "local" id (the generation counter) and the
            // global id. The latter is relevant when a newly allocated
            // QRhiResource ends up with the same pointer as a previous one.
            // (and that previous one could have been in an srb...)
            if (bufD->generation != bd.ubuf.generation || bufD->m_id != bd.ubuf.id) {
                rewriteDescSet = true;
                bd.ubuf.id = bufD->m_id;
                bd.ubuf.generation = bufD->generation;
            }
        }
            break;
        case QRhiShaderResourceBinding::SampledTexture:
        {
            const QRhiShaderResourceBinding::Data::SampledTextureData *data = &b->u.stex;
            if (bd.stex.count != data->count) {
                bd.stex.count = data->count;
                rewriteDescSet = true;
            }
            for (int elem = 0; elem < data->count; ++elem) {
                QVkTexture *texD = QRHI_RES(QVkTexture, data->texSamplers[elem].tex);
                QVkSampler *samplerD = QRHI_RES(QVkSampler, data->texSamplers[elem].sampler);
                texD->lastActiveFrameSlot = currentFrameSlot;
                samplerD->lastActiveFrameSlot = currentFrameSlot;
                trackedRegisterTexture(&passResTracker, texD,
                                       QRhiPassResourceTracker::TexSample,
                                       QRhiPassResourceTracker::toPassTrackerTextureStage(b->stage));
                if (texD->generation != bd.stex.d[elem].texGeneration
                        || texD->m_id != bd.stex.d[elem].texId
                        || samplerD->generation != bd.stex.d[elem].samplerGeneration
                        || samplerD->m_id != bd.stex.d[elem].samplerId)
                {
                    rewriteDescSet = true;
                    bd.stex.d[elem].texId = texD->m_id;
                    bd.stex.d[elem].texGeneration = texD->generation;
                    bd.stex.d[elem].samplerId = samplerD->m_id;
                    bd.stex.d[elem].samplerGeneration = samplerD->generation;
                }
            }
        }
            break;
        case QRhiShaderResourceBinding::ImageLoad:
        case QRhiShaderResourceBinding::ImageStore:
        case QRhiShaderResourceBinding::ImageLoadStore:
        {
            QVkTexture *texD = QRHI_RES(QVkTexture, b->u.simage.tex);
            Q_ASSERT(texD->m_flags.testFlag(QRhiTexture::UsedWithLoadStore));
            texD->lastActiveFrameSlot = currentFrameSlot;
            QRhiPassResourceTracker::TextureAccess access;
            if (b->type == QRhiShaderResourceBinding::ImageLoad)
                access = QRhiPassResourceTracker::TexStorageLoad;
            else if (b->type == QRhiShaderResourceBinding::ImageStore)
                access = QRhiPassResourceTracker::TexStorageStore;
            else
                access = QRhiPassResourceTracker::TexStorageLoadStore;
            trackedRegisterTexture(&passResTracker, texD,
                                   access,
                                   QRhiPassResourceTracker::toPassTrackerTextureStage(b->stage));

            if (texD->generation != bd.simage.generation || texD->m_id != bd.simage.id) {
                rewriteDescSet = true;
                bd.simage.id = texD->m_id;
                bd.simage.generation = texD->generation;
            }
        }
            break;
        case QRhiShaderResourceBinding::BufferLoad:
        case QRhiShaderResourceBinding::BufferStore:
        case QRhiShaderResourceBinding::BufferLoadStore:
        {
            QVkBuffer *bufD = QRHI_RES(QVkBuffer, b->u.sbuf.buf);
            Q_ASSERT(bufD->m_usage.testFlag(QRhiBuffer::StorageBuffer));

            if (bufD->m_type == QRhiBuffer::Dynamic)
                executeBufferHostWritesForSlot(bufD, currentFrameSlot);

            bufD->lastActiveFrameSlot = currentFrameSlot;
            QRhiPassResourceTracker::BufferAccess access;
            if (b->type == QRhiShaderResourceBinding::BufferLoad)
                access = QRhiPassResourceTracker::BufStorageLoad;
            else if (b->type == QRhiShaderResourceBinding::BufferStore)
                access = QRhiPassResourceTracker::BufStorageStore;
            else
                access = QRhiPassResourceTracker::BufStorageLoadStore;
            trackedRegisterBuffer(&passResTracker, bufD, bufD->m_type == QRhiBuffer::Dynamic ? currentFrameSlot : 0,
                                  access,
                                  QRhiPassResourceTracker::toPassTrackerBufferStage(b->stage));

            if (bufD->generation != bd.sbuf.generation || bufD->m_id != bd.sbuf.id) {
                rewriteDescSet = true;
                bd.sbuf.id = bufD->m_id;
                bd.sbuf.generation = bufD->generation;
            }
        }
            break;
        default:
            Q_UNREACHABLE();
            break;
        }
    }

    // write descriptor sets, if needed
    if (rewriteDescSet)
        updateShaderResourceBindings(srb, descSetIdx);

    // make sure the descriptors for the correct slot will get bound.
    // also, dynamic offsets always need a bind.
    const bool forceRebind = (hasSlottedResourceInSrb && cbD->currentDescSetSlot != descSetIdx) || hasDynamicOffsetInSrb;

    const bool srbChanged = gfxPsD ? (cbD->currentGraphicsSrb != srb) : (cbD->currentComputeSrb != srb);

    if (forceRebind || rewriteDescSet || srbChanged || cbD->currentSrbGeneration != srbD->generation) {
        QVarLengthArray<uint32_t, 4> dynOfs;
        if (hasDynamicOffsetInSrb) {
            // Filling out dynOfs based on the sorted bindings is important
            // because dynOfs has to be ordered based on the binding numbers,
            // and neither srb nor dynamicOffsets has any such ordering
            // requirement.
            for (const QRhiShaderResourceBinding &binding : qAsConst(srbD->sortedBindings)) {
                const QRhiShaderResourceBinding::Data *b = binding.data();
                if (b->type == QRhiShaderResourceBinding::UniformBuffer && b->u.ubuf.hasDynamicOffset) {
                    uint32_t offset = 0;
                    for (int i = 0; i < dynamicOffsetCount; ++i) {
                        const QRhiCommandBuffer::DynamicOffset &dynOfs(dynamicOffsets[i]);
                        if (dynOfs.first == b->binding) {
                            offset = dynOfs.second;
                            break;
                        }
                    }
                    dynOfs.append(offset); // use 0 if dynamicOffsets did not contain this binding
                }
            }
        }

        if (cbD->useSecondaryCb) {
            df->vkCmdBindDescriptorSets(cbD->secondaryCbs.last(),
                                        gfxPsD ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE,
                                        gfxPsD ? gfxPsD->layout : compPsD->layout,
                                        0, 1, &srbD->descSets[descSetIdx],
                                        uint32_t(dynOfs.count()),
                                        dynOfs.count() ? dynOfs.constData() : nullptr);
        } else {
            QVkCommandBuffer::Command cmd;
            cmd.cmd = QVkCommandBuffer::Command::BindDescriptorSet;
            cmd.args.bindDescriptorSet.bindPoint = gfxPsD ? VK_PIPELINE_BIND_POINT_GRAPHICS
                                                          : VK_PIPELINE_BIND_POINT_COMPUTE;
            cmd.args.bindDescriptorSet.pipelineLayout = gfxPsD ? gfxPsD->layout : compPsD->layout;
            cmd.args.bindDescriptorSet.descSet = srbD->descSets[descSetIdx];
            cmd.args.bindDescriptorSet.dynamicOffsetCount = dynOfs.count();
            cmd.args.bindDescriptorSet.dynamicOffsetIndex = cbD->pools.dynamicOffset.count();
            cbD->pools.dynamicOffset.append(dynOfs.constData(), dynOfs.count());
            cbD->commands.append(cmd);
        }

        if (gfxPsD) {
            cbD->currentGraphicsSrb = srb;
            cbD->currentComputeSrb = nullptr;
        } else {
            cbD->currentGraphicsSrb = nullptr;
            cbD->currentComputeSrb = srb;
        }
        cbD->currentSrbGeneration = srbD->generation;
        cbD->currentDescSetSlot = descSetIdx;
    }

    srbD->lastActiveFrameSlot = currentFrameSlot;
}

void QRhiVulkan::setVertexInput(QRhiCommandBuffer *cb,
                                int startBinding, int bindingCount, const QRhiCommandBuffer::VertexInput *bindings,
                                QRhiBuffer *indexBuf, quint32 indexOffset, QRhiCommandBuffer::IndexFormat indexFormat)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::RenderPass);
    QRhiPassResourceTracker &passResTracker(cbD->passResTrackers[cbD->currentPassResTrackerIndex]);

    bool needsBindVBuf = false;
    for (int i = 0; i < bindingCount; ++i) {
        const int inputSlot = startBinding + i;
        QVkBuffer *bufD = QRHI_RES(QVkBuffer, bindings[i].first);
        Q_ASSERT(bufD->m_usage.testFlag(QRhiBuffer::VertexBuffer));
        bufD->lastActiveFrameSlot = currentFrameSlot;
        if (bufD->m_type == QRhiBuffer::Dynamic)
            executeBufferHostWritesForSlot(bufD, currentFrameSlot);

        const VkBuffer vkvertexbuf = bufD->buffers[bufD->m_type == QRhiBuffer::Dynamic ? currentFrameSlot : 0];
        if (cbD->currentVertexBuffers[inputSlot] != vkvertexbuf
                || cbD->currentVertexOffsets[inputSlot] != bindings[i].second)
        {
            needsBindVBuf = true;
            cbD->currentVertexBuffers[inputSlot] = vkvertexbuf;
            cbD->currentVertexOffsets[inputSlot] = bindings[i].second;
        }
    }

    if (needsBindVBuf) {
        QVarLengthArray<VkBuffer, 4> bufs;
        QVarLengthArray<VkDeviceSize, 4> ofs;
        for (int i = 0; i < bindingCount; ++i) {
            QVkBuffer *bufD = QRHI_RES(QVkBuffer, bindings[i].first);
            const int slot = bufD->m_type == QRhiBuffer::Dynamic ? currentFrameSlot : 0;
            bufs.append(bufD->buffers[slot]);
            ofs.append(bindings[i].second);
            trackedRegisterBuffer(&passResTracker, bufD, slot,
                                  QRhiPassResourceTracker::BufVertexInput,
                                  QRhiPassResourceTracker::BufVertexInputStage);
        }

        if (cbD->useSecondaryCb) {
            df->vkCmdBindVertexBuffers(cbD->secondaryCbs.last(), uint32_t(startBinding),
                                       uint32_t(bufs.count()), bufs.constData(), ofs.constData());
        } else {
            QVkCommandBuffer::Command cmd;
            cmd.cmd = QVkCommandBuffer::Command::BindVertexBuffer;
            cmd.args.bindVertexBuffer.startBinding = startBinding;
            cmd.args.bindVertexBuffer.count = bufs.count();
            cmd.args.bindVertexBuffer.vertexBufferIndex = cbD->pools.vertexBuffer.count();
            cbD->pools.vertexBuffer.append(bufs.constData(), bufs.count());
            cmd.args.bindVertexBuffer.vertexBufferOffsetIndex = cbD->pools.vertexBufferOffset.count();
            cbD->pools.vertexBufferOffset.append(ofs.constData(), ofs.count());
            cbD->commands.append(cmd);
        }
    }

    if (indexBuf) {
        QVkBuffer *ibufD = QRHI_RES(QVkBuffer, indexBuf);
        Q_ASSERT(ibufD->m_usage.testFlag(QRhiBuffer::IndexBuffer));
        ibufD->lastActiveFrameSlot = currentFrameSlot;
        if (ibufD->m_type == QRhiBuffer::Dynamic)
            executeBufferHostWritesForSlot(ibufD, currentFrameSlot);

        const int slot = ibufD->m_type == QRhiBuffer::Dynamic ? currentFrameSlot : 0;
        const VkBuffer vkindexbuf = ibufD->buffers[slot];
        const VkIndexType type = indexFormat == QRhiCommandBuffer::IndexUInt16 ? VK_INDEX_TYPE_UINT16
                                                                               : VK_INDEX_TYPE_UINT32;

        if (cbD->currentIndexBuffer != vkindexbuf
                || cbD->currentIndexOffset != indexOffset
                || cbD->currentIndexFormat != type)
        {
            cbD->currentIndexBuffer = vkindexbuf;
            cbD->currentIndexOffset = indexOffset;
            cbD->currentIndexFormat = type;

            if (cbD->useSecondaryCb) {
                df->vkCmdBindIndexBuffer(cbD->secondaryCbs.last(), vkindexbuf, indexOffset, type);
            } else {
                QVkCommandBuffer::Command cmd;
                cmd.cmd = QVkCommandBuffer::Command::BindIndexBuffer;
                cmd.args.bindIndexBuffer.buf = vkindexbuf;
                cmd.args.bindIndexBuffer.ofs = indexOffset;
                cmd.args.bindIndexBuffer.type = type;
                cbD->commands.append(cmd);
            }

            trackedRegisterBuffer(&passResTracker, ibufD, slot,
                                  QRhiPassResourceTracker::BufIndexRead,
                                  QRhiPassResourceTracker::BufVertexInputStage);
        }
    }
}

void QRhiVulkan::setViewport(QRhiCommandBuffer *cb, const QRhiViewport &viewport)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::RenderPass);
    const QSize outputSize = cbD->currentTarget->pixelSize();

    // x,y is top-left in VkViewport but bottom-left in QRhiViewport
    float x, y, w, h;
    if (!qrhi_toTopLeftRenderTargetRect(outputSize, viewport.viewport(), &x, &y, &w, &h))
        return;

    QVkCommandBuffer::Command cmd;
    VkViewport *vp = &cmd.args.setViewport.viewport;
    vp->x = x;
    vp->y = y;
    vp->width = w;
    vp->height = h;
    vp->minDepth = viewport.minDepth();
    vp->maxDepth = viewport.maxDepth();

    if (cbD->useSecondaryCb) {
        df->vkCmdSetViewport(cbD->secondaryCbs.last(), 0, 1, vp);
    } else {
        cmd.cmd = QVkCommandBuffer::Command::SetViewport;
        cbD->commands.append(cmd);
    }

    if (!QRHI_RES(QVkGraphicsPipeline, cbD->currentGraphicsPipeline)->m_flags.testFlag(QRhiGraphicsPipeline::UsesScissor)) {
        VkRect2D *s = &cmd.args.setScissor.scissor;
        s->offset.x = int32_t(x);
        s->offset.y = int32_t(y);
        s->extent.width = uint32_t(w);
        s->extent.height = uint32_t(h);
        if (cbD->useSecondaryCb) {
            df->vkCmdSetScissor(cbD->secondaryCbs.last(), 0, 1, s);
        } else {
            cmd.cmd = QVkCommandBuffer::Command::SetScissor;
            cbD->commands.append(cmd);
        }
    }
}

void QRhiVulkan::setScissor(QRhiCommandBuffer *cb, const QRhiScissor &scissor)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::RenderPass);
    Q_ASSERT(QRHI_RES(QVkGraphicsPipeline, cbD->currentGraphicsPipeline)->m_flags.testFlag(QRhiGraphicsPipeline::UsesScissor));
    const QSize outputSize = cbD->currentTarget->pixelSize();

    // x,y is top-left in VkRect2D but bottom-left in QRhiScissor
    int x, y, w, h;
    if (!qrhi_toTopLeftRenderTargetRect(outputSize, scissor.scissor(), &x, &y, &w, &h))
        return;

    QVkCommandBuffer::Command cmd;
    VkRect2D *s = &cmd.args.setScissor.scissor;
    s->offset.x = x;
    s->offset.y = y;
    s->extent.width = uint32_t(w);
    s->extent.height = uint32_t(h);

    if (cbD->useSecondaryCb) {
        df->vkCmdSetScissor(cbD->secondaryCbs.last(), 0, 1, s);
    } else {
        cmd.cmd = QVkCommandBuffer::Command::SetScissor;
        cbD->commands.append(cmd);
    }
}

void QRhiVulkan::setBlendConstants(QRhiCommandBuffer *cb, const QColor &c)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::RenderPass);

    if (cbD->useSecondaryCb) {
        float constants[] = { float(c.redF()), float(c.greenF()), float(c.blueF()), float(c.alphaF()) };
        df->vkCmdSetBlendConstants(cbD->secondaryCbs.last(), constants);
    } else {
        QVkCommandBuffer::Command cmd;
        cmd.cmd = QVkCommandBuffer::Command::SetBlendConstants;
        cmd.args.setBlendConstants.c[0] = float(c.redF());
        cmd.args.setBlendConstants.c[1] = float(c.greenF());
        cmd.args.setBlendConstants.c[2] = float(c.blueF());
        cmd.args.setBlendConstants.c[3] = float(c.alphaF());
        cbD->commands.append(cmd);
    }
}

void QRhiVulkan::setStencilRef(QRhiCommandBuffer *cb, quint32 refValue)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::RenderPass);

    if (cbD->useSecondaryCb) {
        df->vkCmdSetStencilReference(cbD->secondaryCbs.last(), VK_STENCIL_FRONT_AND_BACK, refValue);
    } else {
        QVkCommandBuffer::Command cmd;
        cmd.cmd = QVkCommandBuffer::Command::SetStencilRef;
        cmd.args.setStencilRef.ref = refValue;
        cbD->commands.append(cmd);
    }
}

void QRhiVulkan::draw(QRhiCommandBuffer *cb, quint32 vertexCount,
                      quint32 instanceCount, quint32 firstVertex, quint32 firstInstance)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::RenderPass);

    if (cbD->useSecondaryCb) {
        df->vkCmdDraw(cbD->secondaryCbs.last(), vertexCount, instanceCount, firstVertex, firstInstance);
    } else {
        QVkCommandBuffer::Command cmd;
        cmd.cmd = QVkCommandBuffer::Command::Draw;
        cmd.args.draw.vertexCount = vertexCount;
        cmd.args.draw.instanceCount = instanceCount;
        cmd.args.draw.firstVertex = firstVertex;
        cmd.args.draw.firstInstance = firstInstance;
        cbD->commands.append(cmd);
    }
}

void QRhiVulkan::drawIndexed(QRhiCommandBuffer *cb, quint32 indexCount,
                             quint32 instanceCount, quint32 firstIndex, qint32 vertexOffset, quint32 firstInstance)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::RenderPass);

    if (cbD->useSecondaryCb) {
        df->vkCmdDrawIndexed(cbD->secondaryCbs.last(), indexCount, instanceCount,
                             firstIndex, vertexOffset, firstInstance);
    } else {
        QVkCommandBuffer::Command cmd;
        cmd.cmd = QVkCommandBuffer::Command::DrawIndexed;
        cmd.args.drawIndexed.indexCount = indexCount;
        cmd.args.drawIndexed.instanceCount = instanceCount;
        cmd.args.drawIndexed.firstIndex = firstIndex;
        cmd.args.drawIndexed.vertexOffset = vertexOffset;
        cmd.args.drawIndexed.firstInstance = firstInstance;
        cbD->commands.append(cmd);
    }
}

void QRhiVulkan::debugMarkBegin(QRhiCommandBuffer *cb, const QByteArray &name)
{
    if (!debugMarkers || !debugMarkersAvailable)
        return;

    VkDebugMarkerMarkerInfoEXT marker;
    memset(&marker, 0, sizeof(marker));
    marker.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;

    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    if (cbD->recordingPass != QVkCommandBuffer::NoPass && cbD->useSecondaryCb) {
        marker.pMarkerName = name.constData();
        vkCmdDebugMarkerBegin(cbD->secondaryCbs.last(), &marker);
    } else {
        QVkCommandBuffer::Command cmd;
        cmd.cmd = QVkCommandBuffer::Command::DebugMarkerBegin;
        cmd.args.debugMarkerBegin.marker = marker;
        cmd.args.debugMarkerBegin.markerNameIndex = cbD->pools.debugMarkerData.count();
        cbD->pools.debugMarkerData.append(name);
        cbD->commands.append(cmd);
    }
}

void QRhiVulkan::debugMarkEnd(QRhiCommandBuffer *cb)
{
    if (!debugMarkers || !debugMarkersAvailable)
        return;

    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    if (cbD->recordingPass != QVkCommandBuffer::NoPass && cbD->useSecondaryCb) {
        vkCmdDebugMarkerEnd(cbD->secondaryCbs.last());
    } else {
        QVkCommandBuffer::Command cmd;
        cmd.cmd = QVkCommandBuffer::Command::DebugMarkerEnd;
        cbD->commands.append(cmd);
    }
}

void QRhiVulkan::debugMarkMsg(QRhiCommandBuffer *cb, const QByteArray &msg)
{
    if (!debugMarkers || !debugMarkersAvailable)
        return;

    VkDebugMarkerMarkerInfoEXT marker;
    memset(&marker, 0, sizeof(marker));
    marker.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;

    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    if (cbD->recordingPass != QVkCommandBuffer::NoPass && cbD->useSecondaryCb) {
        marker.pMarkerName = msg.constData();
        vkCmdDebugMarkerInsert(cbD->secondaryCbs.last(), &marker);
    } else {
        QVkCommandBuffer::Command cmd;
        cmd.cmd = QVkCommandBuffer::Command::DebugMarkerInsert;
        cmd.args.debugMarkerInsert.marker = marker;
        cmd.args.debugMarkerInsert.markerNameIndex = cbD->pools.debugMarkerData.count();
        cbD->pools.debugMarkerData.append(msg);
        cbD->commands.append(cmd);
    }
}

const QRhiNativeHandles *QRhiVulkan::nativeHandles(QRhiCommandBuffer *cb)
{
    return QRHI_RES(QVkCommandBuffer, cb)->nativeHandles();
}

static inline QVkRenderTargetData *maybeRenderTargetData(QVkCommandBuffer *cbD)
{
    Q_ASSERT(cbD->currentTarget);
    QVkRenderTargetData *rtD = nullptr;
    if (cbD->recordingPass == QVkCommandBuffer::RenderPass) {
        switch (cbD->currentTarget->resourceType()) {
        case QRhiResource::RenderTarget:
            rtD = &QRHI_RES(QVkReferenceRenderTarget, cbD->currentTarget)->d;
            break;
        case QRhiResource::TextureRenderTarget:
            rtD = &QRHI_RES(QVkTextureRenderTarget, cbD->currentTarget)->d;
            break;
        default:
            Q_UNREACHABLE();
            break;
        }
    }
    return rtD;
}

void QRhiVulkan::beginExternal(QRhiCommandBuffer *cb)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);

    // When not in a pass, it is simple: record what we have (but do not
    // submit), the cb can then be used to record more external commands.
    if (cbD->recordingPass == QVkCommandBuffer::NoPass) {
        recordPrimaryCommandBuffer(cbD);
        cbD->resetCommands();
        return;
    }

    // Otherwise, inside a pass, have a secondary command buffer (with
    // RENDER_PASS_CONTINUE). Using the main one is not acceptable since we
    // cannot just record at this stage, that would mess up the resource
    // tracking and commands like TransitionPassResources.

    if (cbD->inExternal)
        return;

    if (!cbD->useSecondaryCb) {
        qWarning("beginExternal() within a pass is only supported with secondary command buffers. "
                 "This can be enabled by passing QRhi::ExternalContentsInPass to beginFrame().");
        return;
    }

    VkCommandBuffer secondaryCb = cbD->secondaryCbs.last();
    cbD->secondaryCbs.removeLast();
    endAndEnqueueSecondaryCommandBuffer(secondaryCb, cbD);

    VkCommandBuffer extCb = startSecondaryCommandBuffer(maybeRenderTargetData(cbD));
    if (extCb) {
        cbD->secondaryCbs.append(extCb);
        cbD->inExternal = true;
    }
}

void QRhiVulkan::endExternal(QRhiCommandBuffer *cb)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);

    if (cbD->recordingPass == QVkCommandBuffer::NoPass) {
        Q_ASSERT(cbD->commands.isEmpty() && cbD->currentPassResTrackerIndex == -1);
    } else if (cbD->inExternal) {
        VkCommandBuffer extCb = cbD->secondaryCbs.last();
        cbD->secondaryCbs.removeLast();
        endAndEnqueueSecondaryCommandBuffer(extCb, cbD);
        cbD->secondaryCbs.append(startSecondaryCommandBuffer(maybeRenderTargetData(cbD)));
    }

    cbD->resetCachedState();
}

void QRhiVulkan::setObjectName(uint64_t object, VkDebugReportObjectTypeEXT type, const QByteArray &name, int slot)
{
    if (!debugMarkers || !debugMarkersAvailable || name.isEmpty())
        return;

    VkDebugMarkerObjectNameInfoEXT nameInfo;
    memset(&nameInfo, 0, sizeof(nameInfo));
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = type;
    nameInfo.object = object;
    QByteArray decoratedName = name;
    if (slot >= 0) {
        decoratedName += '/';
        decoratedName += QByteArray::number(slot);
    }
    nameInfo.pObjectName = decoratedName.constData();
    vkDebugMarkerSetObjectName(dev, &nameInfo);
}

static inline VkBufferUsageFlagBits toVkBufferUsage(QRhiBuffer::UsageFlags usage)
{
    int u = 0;
    if (usage.testFlag(QRhiBuffer::VertexBuffer))
        u |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (usage.testFlag(QRhiBuffer::IndexBuffer))
        u |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (usage.testFlag(QRhiBuffer::UniformBuffer))
        u |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (usage.testFlag(QRhiBuffer::StorageBuffer))
        u |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    return VkBufferUsageFlagBits(u);
}

static inline VkFilter toVkFilter(QRhiSampler::Filter f)
{
    switch (f) {
    case QRhiSampler::Nearest:
        return VK_FILTER_NEAREST;
    case QRhiSampler::Linear:
        return VK_FILTER_LINEAR;
    default:
        Q_UNREACHABLE();
        return VK_FILTER_NEAREST;
    }
}

static inline VkSamplerMipmapMode toVkMipmapMode(QRhiSampler::Filter f)
{
    switch (f) {
    case QRhiSampler::None:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    case QRhiSampler::Nearest:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    case QRhiSampler::Linear:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    default:
        Q_UNREACHABLE();
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    }
}

static inline VkSamplerAddressMode toVkAddressMode(QRhiSampler::AddressMode m)
{
    switch (m) {
    case QRhiSampler::Repeat:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case QRhiSampler::ClampToEdge:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case QRhiSampler::Mirror:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    default:
        Q_UNREACHABLE();
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }
}

static inline VkShaderStageFlagBits toVkShaderStage(QRhiShaderStage::Type type)
{
    switch (type) {
    case QRhiShaderStage::Vertex:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case QRhiShaderStage::Fragment:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case QRhiShaderStage::Compute:
        return VK_SHADER_STAGE_COMPUTE_BIT;
    default:
        Q_UNREACHABLE();
        return VK_SHADER_STAGE_VERTEX_BIT;
    }
}

static inline VkFormat toVkAttributeFormat(QRhiVertexInputAttribute::Format format)
{
    switch (format) {
    case QRhiVertexInputAttribute::Float4:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case QRhiVertexInputAttribute::Float3:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case QRhiVertexInputAttribute::Float2:
        return VK_FORMAT_R32G32_SFLOAT;
    case QRhiVertexInputAttribute::Float:
        return VK_FORMAT_R32_SFLOAT;
    case QRhiVertexInputAttribute::UNormByte4:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case QRhiVertexInputAttribute::UNormByte2:
        return VK_FORMAT_R8G8_UNORM;
    case QRhiVertexInputAttribute::UNormByte:
        return VK_FORMAT_R8_UNORM;
    default:
        Q_UNREACHABLE();
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    }
}

static inline VkPrimitiveTopology toVkTopology(QRhiGraphicsPipeline::Topology t)
{
    switch (t) {
    case QRhiGraphicsPipeline::Triangles:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case QRhiGraphicsPipeline::TriangleStrip:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case QRhiGraphicsPipeline::TriangleFan:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    case QRhiGraphicsPipeline::Lines:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case QRhiGraphicsPipeline::LineStrip:
        return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case QRhiGraphicsPipeline::Points:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    default:
        Q_UNREACHABLE();
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

static inline VkCullModeFlags toVkCullMode(QRhiGraphicsPipeline::CullMode c)
{
    switch (c) {
    case QRhiGraphicsPipeline::None:
        return VK_CULL_MODE_NONE;
    case QRhiGraphicsPipeline::Front:
        return VK_CULL_MODE_FRONT_BIT;
    case QRhiGraphicsPipeline::Back:
        return VK_CULL_MODE_BACK_BIT;
    default:
        Q_UNREACHABLE();
        return VK_CULL_MODE_NONE;
    }
}

static inline VkFrontFace toVkFrontFace(QRhiGraphicsPipeline::FrontFace f)
{
    switch (f) {
    case QRhiGraphicsPipeline::CCW:
        return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    case QRhiGraphicsPipeline::CW:
        return VK_FRONT_FACE_CLOCKWISE;
    default:
        Q_UNREACHABLE();
        return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }
}

static inline VkColorComponentFlags toVkColorComponents(QRhiGraphicsPipeline::ColorMask c)
{
    int f = 0;
    if (c.testFlag(QRhiGraphicsPipeline::R))
        f |= VK_COLOR_COMPONENT_R_BIT;
    if (c.testFlag(QRhiGraphicsPipeline::G))
        f |= VK_COLOR_COMPONENT_G_BIT;
    if (c.testFlag(QRhiGraphicsPipeline::B))
        f |= VK_COLOR_COMPONENT_B_BIT;
    if (c.testFlag(QRhiGraphicsPipeline::A))
        f |= VK_COLOR_COMPONENT_A_BIT;
    return VkColorComponentFlags(f);
}

static inline VkBlendFactor toVkBlendFactor(QRhiGraphicsPipeline::BlendFactor f)
{
    switch (f) {
    case QRhiGraphicsPipeline::Zero:
        return VK_BLEND_FACTOR_ZERO;
    case QRhiGraphicsPipeline::One:
        return VK_BLEND_FACTOR_ONE;
    case QRhiGraphicsPipeline::SrcColor:
        return VK_BLEND_FACTOR_SRC_COLOR;
    case QRhiGraphicsPipeline::OneMinusSrcColor:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case QRhiGraphicsPipeline::DstColor:
        return VK_BLEND_FACTOR_DST_COLOR;
    case QRhiGraphicsPipeline::OneMinusDstColor:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case QRhiGraphicsPipeline::SrcAlpha:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case QRhiGraphicsPipeline::OneMinusSrcAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case QRhiGraphicsPipeline::DstAlpha:
        return VK_BLEND_FACTOR_DST_ALPHA;
    case QRhiGraphicsPipeline::OneMinusDstAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case QRhiGraphicsPipeline::ConstantColor:
        return VK_BLEND_FACTOR_CONSTANT_COLOR;
    case QRhiGraphicsPipeline::OneMinusConstantColor:
        return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
    case QRhiGraphicsPipeline::ConstantAlpha:
        return VK_BLEND_FACTOR_CONSTANT_ALPHA;
    case QRhiGraphicsPipeline::OneMinusConstantAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
    case QRhiGraphicsPipeline::SrcAlphaSaturate:
        return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
    case QRhiGraphicsPipeline::Src1Color:
        return VK_BLEND_FACTOR_SRC1_COLOR;
    case QRhiGraphicsPipeline::OneMinusSrc1Color:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
    case QRhiGraphicsPipeline::Src1Alpha:
        return VK_BLEND_FACTOR_SRC1_ALPHA;
    case QRhiGraphicsPipeline::OneMinusSrc1Alpha:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
    default:
        Q_UNREACHABLE();
        return VK_BLEND_FACTOR_ZERO;
    }
}

static inline VkBlendOp toVkBlendOp(QRhiGraphicsPipeline::BlendOp op)
{
    switch (op) {
    case QRhiGraphicsPipeline::Add:
        return VK_BLEND_OP_ADD;
    case QRhiGraphicsPipeline::Subtract:
        return VK_BLEND_OP_SUBTRACT;
    case QRhiGraphicsPipeline::ReverseSubtract:
        return VK_BLEND_OP_REVERSE_SUBTRACT;
    case QRhiGraphicsPipeline::Min:
        return VK_BLEND_OP_MIN;
    case QRhiGraphicsPipeline::Max:
        return VK_BLEND_OP_MAX;
    default:
        Q_UNREACHABLE();
        return VK_BLEND_OP_ADD;
    }
}

static inline VkCompareOp toVkCompareOp(QRhiGraphicsPipeline::CompareOp op)
{
    switch (op) {
    case QRhiGraphicsPipeline::Never:
        return VK_COMPARE_OP_NEVER;
    case QRhiGraphicsPipeline::Less:
        return VK_COMPARE_OP_LESS;
    case QRhiGraphicsPipeline::Equal:
        return VK_COMPARE_OP_EQUAL;
    case QRhiGraphicsPipeline::LessOrEqual:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case QRhiGraphicsPipeline::Greater:
        return VK_COMPARE_OP_GREATER;
    case QRhiGraphicsPipeline::NotEqual:
        return VK_COMPARE_OP_NOT_EQUAL;
    case QRhiGraphicsPipeline::GreaterOrEqual:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case QRhiGraphicsPipeline::Always:
        return VK_COMPARE_OP_ALWAYS;
    default:
        Q_UNREACHABLE();
        return VK_COMPARE_OP_ALWAYS;
    }
}

static inline VkStencilOp toVkStencilOp(QRhiGraphicsPipeline::StencilOp op)
{
    switch (op) {
    case QRhiGraphicsPipeline::StencilZero:
        return VK_STENCIL_OP_ZERO;
    case QRhiGraphicsPipeline::Keep:
        return VK_STENCIL_OP_KEEP;
    case QRhiGraphicsPipeline::Replace:
        return VK_STENCIL_OP_REPLACE;
    case QRhiGraphicsPipeline::IncrementAndClamp:
        return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    case QRhiGraphicsPipeline::DecrementAndClamp:
        return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    case QRhiGraphicsPipeline::Invert:
        return VK_STENCIL_OP_INVERT;
    case QRhiGraphicsPipeline::IncrementAndWrap:
        return VK_STENCIL_OP_INCREMENT_AND_WRAP;
    case QRhiGraphicsPipeline::DecrementAndWrap:
        return VK_STENCIL_OP_DECREMENT_AND_WRAP;
    default:
        Q_UNREACHABLE();
        return VK_STENCIL_OP_KEEP;
    }
}

static inline void fillVkStencilOpState(VkStencilOpState *dst, const QRhiGraphicsPipeline::StencilOpState &src)
{
    dst->failOp = toVkStencilOp(src.failOp);
    dst->passOp = toVkStencilOp(src.passOp);
    dst->depthFailOp = toVkStencilOp(src.depthFailOp);
    dst->compareOp = toVkCompareOp(src.compareOp);
}

static inline VkDescriptorType toVkDescriptorType(const QRhiShaderResourceBinding::Data *b)
{
    switch (b->type) {
    case QRhiShaderResourceBinding::UniformBuffer:
        return b->u.ubuf.hasDynamicOffset ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
                                          : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    case QRhiShaderResourceBinding::SampledTexture:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    case QRhiShaderResourceBinding::ImageLoad:
    case QRhiShaderResourceBinding::ImageStore:
    case QRhiShaderResourceBinding::ImageLoadStore:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

    case QRhiShaderResourceBinding::BufferLoad:
    case QRhiShaderResourceBinding::BufferStore:
    case QRhiShaderResourceBinding::BufferLoadStore:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    default:
        Q_UNREACHABLE();
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
}

static inline VkShaderStageFlags toVkShaderStageFlags(QRhiShaderResourceBinding::StageFlags stage)
{
    int s = 0;
    if (stage.testFlag(QRhiShaderResourceBinding::VertexStage))
        s |= VK_SHADER_STAGE_VERTEX_BIT;
    if (stage.testFlag(QRhiShaderResourceBinding::FragmentStage))
        s |= VK_SHADER_STAGE_FRAGMENT_BIT;
    if (stage.testFlag(QRhiShaderResourceBinding::ComputeStage))
        s |= VK_SHADER_STAGE_COMPUTE_BIT;
    return VkShaderStageFlags(s);
}

static inline VkCompareOp toVkTextureCompareOp(QRhiSampler::CompareOp op)
{
    switch (op) {
    case QRhiSampler::Never:
        return VK_COMPARE_OP_NEVER;
    case QRhiSampler::Less:
        return VK_COMPARE_OP_LESS;
    case QRhiSampler::Equal:
        return VK_COMPARE_OP_EQUAL;
    case QRhiSampler::LessOrEqual:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case QRhiSampler::Greater:
        return VK_COMPARE_OP_GREATER;
    case QRhiSampler::NotEqual:
        return VK_COMPARE_OP_NOT_EQUAL;
    case QRhiSampler::GreaterOrEqual:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case QRhiSampler::Always:
        return VK_COMPARE_OP_ALWAYS;
    default:
        Q_UNREACHABLE();
        return VK_COMPARE_OP_NEVER;
    }
}

QVkBuffer::QVkBuffer(QRhiImplementation *rhi, Type type, UsageFlags usage, int size)
    : QRhiBuffer(rhi, type, usage, size)
{
    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i) {
        buffers[i] = stagingBuffers[i] = VK_NULL_HANDLE;
        allocations[i] = stagingAllocations[i] = nullptr;
    }
}

QVkBuffer::~QVkBuffer()
{
    release();
}

void QVkBuffer::release()
{
    if (!buffers[0])
        return;

    QRhiVulkan::DeferredReleaseEntry e;
    e.type = QRhiVulkan::DeferredReleaseEntry::Buffer;
    e.lastActiveFrameSlot = lastActiveFrameSlot;

    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i) {
        e.buffer.buffers[i] = buffers[i];
        e.buffer.allocations[i] = allocations[i];
        e.buffer.stagingBuffers[i] = stagingBuffers[i];
        e.buffer.stagingAllocations[i] = stagingAllocations[i];

        buffers[i] = VK_NULL_HANDLE;
        allocations[i] = nullptr;
        stagingBuffers[i] = VK_NULL_HANDLE;
        stagingAllocations[i] = nullptr;
        pendingDynamicUpdates[i].clear();
    }

    QRHI_RES_RHI(QRhiVulkan);
    rhiD->releaseQueue.append(e);

    QRHI_PROF;
    QRHI_PROF_F(releaseBuffer(this));

    rhiD->unregisterResource(this);
}

bool QVkBuffer::build()
{
    if (buffers[0])
        release();

    if (m_usage.testFlag(QRhiBuffer::StorageBuffer) && m_type == Dynamic) {
        qWarning("StorageBuffer cannot be combined with Dynamic");
        return false;
    }

    const int nonZeroSize = m_size <= 0 ? 256 : m_size;

    VkBufferCreateInfo bufferInfo;
    memset(&bufferInfo, 0, sizeof(bufferInfo));
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = uint32_t(nonZeroSize);
    bufferInfo.usage = toVkBufferUsage(m_usage);

    VmaAllocationCreateInfo allocInfo;
    memset(&allocInfo, 0, sizeof(allocInfo));

    if (m_type == Dynamic) {
#ifndef Q_OS_DARWIN // not for MoltenVK
        // Keep mapped all the time. Essential f.ex. with some mobile GPUs,
        // where mapping and unmapping an entire allocation every time updating
        // a suballocated buffer presents a significant perf. hit.
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
#endif
        // host visible, frequent changes
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    } else {
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    QRHI_RES_RHI(QRhiVulkan);
    VkResult err = VK_SUCCESS;
    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i) {
        buffers[i] = VK_NULL_HANDLE;
        allocations[i] = nullptr;
        usageState[i].access = usageState[i].stage = 0;
        if (i == 0 || m_type == Dynamic) {
            VmaAllocation allocation;
            err = vmaCreateBuffer(toVmaAllocator(rhiD->allocator), &bufferInfo, &allocInfo, &buffers[i], &allocation, nullptr);
            if (err != VK_SUCCESS)
                break;
            allocations[i] = allocation;
            rhiD->setObjectName(uint64_t(buffers[i]), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, m_objectName,
                                m_type == Dynamic ? i : -1);
        }
    }

    if (err != VK_SUCCESS) {
        qWarning("Failed to create buffer: %d", err);
        return false;
    }

    QRHI_PROF;
    QRHI_PROF_F(newBuffer(this, uint(nonZeroSize), m_type != Dynamic ? 1 : QVK_FRAMES_IN_FLIGHT, 0));

    lastActiveFrameSlot = -1;
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QRhiBuffer::NativeBuffer QVkBuffer::nativeBuffer()
{
    if (m_type == Dynamic) {
        QRHI_RES_RHI(QRhiVulkan);
        NativeBuffer b;
        Q_ASSERT(sizeof(b.objects) / sizeof(b.objects[0]) >= size_t(QVK_FRAMES_IN_FLIGHT));
        for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i) {
            rhiD->executeBufferHostWritesForSlot(this, i);
            b.objects[i] = &buffers[i];
        }
        b.slotCount = QVK_FRAMES_IN_FLIGHT;
        return b;
    }
    return { { &buffers[0] }, 1 };
}

QVkRenderBuffer::QVkRenderBuffer(QRhiImplementation *rhi, Type type, const QSize &pixelSize,
                                 int sampleCount, Flags flags)
    : QRhiRenderBuffer(rhi, type, pixelSize, sampleCount, flags)
{
}

QVkRenderBuffer::~QVkRenderBuffer()
{
    release();
    delete backingTexture;
}

void QVkRenderBuffer::release()
{
    if (!memory && !backingTexture)
        return;

    QRhiVulkan::DeferredReleaseEntry e;
    e.type = QRhiVulkan::DeferredReleaseEntry::RenderBuffer;
    e.lastActiveFrameSlot = lastActiveFrameSlot;

    e.renderBuffer.memory = memory;
    e.renderBuffer.image = image;
    e.renderBuffer.imageView = imageView;

    memory = VK_NULL_HANDLE;
    image = VK_NULL_HANDLE;
    imageView = VK_NULL_HANDLE;

    if (backingTexture) {
        Q_ASSERT(backingTexture->lastActiveFrameSlot == -1);
        backingTexture->lastActiveFrameSlot = e.lastActiveFrameSlot;
        backingTexture->release();
    }

    QRHI_RES_RHI(QRhiVulkan);
    rhiD->releaseQueue.append(e);

    QRHI_PROF;
    QRHI_PROF_F(releaseRenderBuffer(this));

    rhiD->unregisterResource(this);
}

bool QVkRenderBuffer::build()
{
    if (memory || backingTexture)
        release();

    if (m_pixelSize.isEmpty())
        return false;

    QRHI_RES_RHI(QRhiVulkan);
    QRHI_PROF;
    samples = rhiD->effectiveSampleCount(m_sampleCount);

    switch (m_type) {
    case QRhiRenderBuffer::Color:
    {
        if (!backingTexture) {
            backingTexture = QRHI_RES(QVkTexture, rhiD->createTexture(QRhiTexture::RGBA8,
                                                                      m_pixelSize,
                                                                      m_sampleCount,
                                                                      QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
        } else {
            backingTexture->setPixelSize(m_pixelSize);
            backingTexture->setSampleCount(m_sampleCount);
        }
        backingTexture->setName(m_objectName);
        if (!backingTexture->build())
            return false;
        vkformat = backingTexture->vkformat;
        QRHI_PROF_F(newRenderBuffer(this, false, false, samples));
    }
        break;
    case QRhiRenderBuffer::DepthStencil:
        vkformat = rhiD->optimalDepthStencilFormat();
        if (!rhiD->createTransientImage(vkformat,
                                        m_pixelSize,
                                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                                        samples,
                                        &memory,
                                        &image,
                                        &imageView,
                                        1))
        {
            return false;
        }
        rhiD->setObjectName(uint64_t(image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, m_objectName);
        QRHI_PROF_F(newRenderBuffer(this, true, false, samples));
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    lastActiveFrameSlot = -1;
    rhiD->registerResource(this);
    return true;
}

QRhiTexture::Format QVkRenderBuffer::backingFormat() const
{
    return m_type == Color ? QRhiTexture::RGBA8 : QRhiTexture::UnknownFormat;
}

QVkTexture::QVkTexture(QRhiImplementation *rhi, Format format, const QSize &pixelSize,
                       int sampleCount, Flags flags)
    : QRhiTexture(rhi, format, pixelSize, sampleCount, flags)
{
    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i) {
        stagingBuffers[i] = VK_NULL_HANDLE;
        stagingAllocations[i] = nullptr;
    }
    for (int i = 0; i < QRhi::MAX_LEVELS; ++i)
        perLevelImageViews[i] = VK_NULL_HANDLE;
}

QVkTexture::~QVkTexture()
{
    release();
}

void QVkTexture::release()
{
    if (!image)
        return;

    QRhiVulkan::DeferredReleaseEntry e;
    e.type = QRhiVulkan::DeferredReleaseEntry::Texture;
    e.lastActiveFrameSlot = lastActiveFrameSlot;

    e.texture.image = owns ? image : VK_NULL_HANDLE;
    e.texture.imageView = imageView;
    e.texture.allocation = owns ? imageAlloc : nullptr;

    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i) {
        e.texture.stagingBuffers[i] = stagingBuffers[i];
        e.texture.stagingAllocations[i] = stagingAllocations[i];

        stagingBuffers[i] = VK_NULL_HANDLE;
        stagingAllocations[i] = nullptr;
    }

    for (int i = 0; i < QRhi::MAX_LEVELS; ++i) {
        e.texture.extraImageViews[i] = perLevelImageViews[i];
        perLevelImageViews[i] = VK_NULL_HANDLE;
    }

    image = VK_NULL_HANDLE;
    imageView = VK_NULL_HANDLE;
    imageAlloc = nullptr;

    QRHI_RES_RHI(QRhiVulkan);
    rhiD->releaseQueue.append(e);

    QRHI_PROF;
    QRHI_PROF_F(releaseTexture(this));

    rhiD->unregisterResource(this);
}

bool QVkTexture::prepareBuild(QSize *adjustedSize)
{
    if (image)
        release();

    QRHI_RES_RHI(QRhiVulkan);
    vkformat = toVkTextureFormat(m_format, m_flags);
    VkFormatProperties props;
    rhiD->f->vkGetPhysicalDeviceFormatProperties(rhiD->physDev, vkformat, &props);
    const bool canSampleOptimal = (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
    if (!canSampleOptimal) {
        qWarning("Texture sampling with optimal tiling for format %d not supported", vkformat);
        return false;
    }

    const QSize size = m_pixelSize.isEmpty() ? QSize(1, 1) : m_pixelSize;
    const bool isCube = m_flags.testFlag(CubeMap);
    const bool hasMipMaps = m_flags.testFlag(MipMapped);

    mipLevelCount = uint(hasMipMaps ? rhiD->q->mipLevelsForSize(size) : 1);
    const int maxLevels = QRhi::MAX_LEVELS;
    if (mipLevelCount > maxLevels) {
        qWarning("Too many mip levels (%d, max is %d), truncating mip chain", mipLevelCount, maxLevels);
        mipLevelCount = maxLevels;
    }
    samples = rhiD->effectiveSampleCount(m_sampleCount);
    if (samples > VK_SAMPLE_COUNT_1_BIT) {
        if (isCube) {
            qWarning("Cubemap texture cannot be multisample");
            return false;
        }
        if (hasMipMaps) {
            qWarning("Multisample texture cannot have mipmaps");
            return false;
        }
    }

    usageState.layout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    usageState.access = 0;
    usageState.stage = 0;

    if (adjustedSize)
        *adjustedSize = size;

    return true;
}

bool QVkTexture::finishBuild()
{
    QRHI_RES_RHI(QRhiVulkan);

    const bool isDepth = isDepthTextureFormat(m_format);
    const bool isCube = m_flags.testFlag(CubeMap);

    VkImageViewCreateInfo viewInfo;
    memset(&viewInfo, 0, sizeof(viewInfo));
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = isCube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = vkformat;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    viewInfo.subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = mipLevelCount;
    viewInfo.subresourceRange.layerCount = isCube ? 6 : 1;

    VkResult err = rhiD->df->vkCreateImageView(rhiD->dev, &viewInfo, nullptr, &imageView);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create image view: %d", err);
        return false;
    }

    lastActiveFrameSlot = -1;
    generation += 1;

    return true;
}

bool QVkTexture::build()
{
    QSize size;
    if (!prepareBuild(&size))
        return false;

    const bool isRenderTarget = m_flags.testFlag(QRhiTexture::RenderTarget);
    const bool isDepth = isDepthTextureFormat(m_format);
    const bool isCube = m_flags.testFlag(CubeMap);

    VkImageCreateInfo imageInfo;
    memset(&imageInfo, 0, sizeof(imageInfo));
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = isCube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = vkformat;
    imageInfo.extent.width = uint32_t(size.width());
    imageInfo.extent.height = uint32_t(size.height());
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevelCount;
    imageInfo.arrayLayers = isCube ? 6 : 1;
    imageInfo.samples = samples;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (isRenderTarget) {
        if (isDepth)
            imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        else
            imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (m_flags.testFlag(QRhiTexture::UsedAsTransferSource))
        imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (m_flags.testFlag(QRhiTexture::UsedWithGenerateMips))
        imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (m_flags.testFlag(QRhiTexture::UsedWithLoadStore))
        imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

    VmaAllocationCreateInfo allocInfo;
    memset(&allocInfo, 0, sizeof(allocInfo));
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    QRHI_RES_RHI(QRhiVulkan);
    VmaAllocation allocation;
    VkResult err = vmaCreateImage(toVmaAllocator(rhiD->allocator), &imageInfo, &allocInfo, &image, &allocation, nullptr);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create image: %d", err);
        return false;
    }
    imageAlloc = allocation;

    if (!finishBuild())
        return false;

    rhiD->setObjectName(uint64_t(image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, m_objectName);

    QRHI_PROF;
    QRHI_PROF_F(newTexture(this, true, int(mipLevelCount), isCube ? 6 : 1, samples));

    owns = true;
    rhiD->registerResource(this);
    return true;
}

bool QVkTexture::buildFrom(QRhiTexture::NativeTexture src)
{
    auto *img = static_cast<const VkImage*>(src.object);
    if (!img || !*img)
        return false;

    if (!prepareBuild())
        return false;

    image = *img;

    if (!finishBuild())
        return false;

    QRHI_PROF;
    QRHI_PROF_F(newTexture(this, false, int(mipLevelCount), m_flags.testFlag(CubeMap) ? 6 : 1, samples));

    usageState.layout = VkImageLayout(src.layout);

    owns = false;
    QRHI_RES_RHI(QRhiVulkan);
    rhiD->registerResource(this);
    return true;
}

QRhiTexture::NativeTexture QVkTexture::nativeTexture()
{
    return {&image, usageState.layout};
}

void QVkTexture::setNativeLayout(int layout)
{
    usageState.layout = VkImageLayout(layout);
}

VkImageView QVkTexture::imageViewForLevel(int level)
{
    Q_ASSERT(level >= 0 && level < int(mipLevelCount));
    if (perLevelImageViews[level] != VK_NULL_HANDLE)
        return perLevelImageViews[level];

    const bool isDepth = isDepthTextureFormat(m_format);
    const bool isCube = m_flags.testFlag(CubeMap);

    VkImageViewCreateInfo viewInfo;
    memset(&viewInfo, 0, sizeof(viewInfo));
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = isCube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = vkformat;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    viewInfo.subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = uint32_t(level);
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = isCube ? 6 : 1;

    VkImageView v = VK_NULL_HANDLE;
    QRHI_RES_RHI(QRhiVulkan);
    VkResult err = rhiD->df->vkCreateImageView(rhiD->dev, &viewInfo, nullptr, &v);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create image view: %d", err);
        return VK_NULL_HANDLE;
    }

    perLevelImageViews[level] = v;
    return v;
}

QVkSampler::QVkSampler(QRhiImplementation *rhi, Filter magFilter, Filter minFilter, Filter mipmapMode,
                       AddressMode u, AddressMode v, AddressMode w)
    : QRhiSampler(rhi, magFilter, minFilter, mipmapMode, u, v, w)
{
}

QVkSampler::~QVkSampler()
{
    release();
}

void QVkSampler::release()
{
    if (!sampler)
        return;

    QRhiVulkan::DeferredReleaseEntry e;
    e.type = QRhiVulkan::DeferredReleaseEntry::Sampler;
    e.lastActiveFrameSlot = lastActiveFrameSlot;

    e.sampler.sampler = sampler;
    sampler = VK_NULL_HANDLE;

    QRHI_RES_RHI(QRhiVulkan);
    rhiD->releaseQueue.append(e);
    rhiD->unregisterResource(this);
}

bool QVkSampler::build()
{
    if (sampler)
        release();

    VkSamplerCreateInfo samplerInfo;
    memset(&samplerInfo, 0, sizeof(samplerInfo));
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = toVkFilter(m_magFilter);
    samplerInfo.minFilter = toVkFilter(m_minFilter);
    samplerInfo.mipmapMode = toVkMipmapMode(m_mipmapMode);
    samplerInfo.addressModeU = toVkAddressMode(m_addressU);
    samplerInfo.addressModeV = toVkAddressMode(m_addressV);
    samplerInfo.addressModeW = toVkAddressMode(m_addressW);
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.compareEnable = m_compareOp != Never;
    samplerInfo.compareOp = toVkTextureCompareOp(m_compareOp);
    samplerInfo.maxLod = m_mipmapMode == None ? 0.25f : 1000.0f;

    QRHI_RES_RHI(QRhiVulkan);
    VkResult err = rhiD->df->vkCreateSampler(rhiD->dev, &samplerInfo, nullptr, &sampler);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create sampler: %d", err);
        return false;
    }

    lastActiveFrameSlot = -1;
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QVkRenderPassDescriptor::QVkRenderPassDescriptor(QRhiImplementation *rhi)
    : QRhiRenderPassDescriptor(rhi)
{
}

QVkRenderPassDescriptor::~QVkRenderPassDescriptor()
{
    release();
}

void QVkRenderPassDescriptor::release()
{
    if (!rp)
        return;

    if (!ownsRp) {
        rp = VK_NULL_HANDLE;
        return;
    }

    QRhiVulkan::DeferredReleaseEntry e;
    e.type = QRhiVulkan::DeferredReleaseEntry::RenderPass;
    e.lastActiveFrameSlot = lastActiveFrameSlot;

    e.renderPass.rp = rp;

    rp = VK_NULL_HANDLE;

    QRHI_RES_RHI(QRhiVulkan);
    rhiD->releaseQueue.append(e);

    rhiD->unregisterResource(this);
}

static inline bool attachmentDescriptionEquals(const VkAttachmentDescription &a, const VkAttachmentDescription &b)
{
    return a.format == b.format
            && a.samples == b.samples
            && a.loadOp == b.loadOp
            && a.storeOp == b.storeOp
            && a.stencilLoadOp == b.stencilLoadOp
            && a.stencilStoreOp == b.stencilStoreOp
            && a.initialLayout == b.initialLayout
            && a.finalLayout == b.finalLayout;
}

bool QVkRenderPassDescriptor::isCompatible(const QRhiRenderPassDescriptor *other) const
{
    if (!other)
        return false;

    const QVkRenderPassDescriptor *o = QRHI_RES(const QVkRenderPassDescriptor, other);

    if (attDescs.count() != o->attDescs.count())
        return false;
    if (colorRefs.count() != o->colorRefs.count())
        return false;
    if (resolveRefs.count() != o->resolveRefs.count())
        return false;
    if (hasDepthStencil != o->hasDepthStencil)
        return false;

    for (int i = 0, ie = colorRefs.count(); i != ie; ++i) {
        const uint32_t attIdx = colorRefs[i].attachment;
        if (attIdx != o->colorRefs[i].attachment)
            return false;
        if (attIdx != VK_ATTACHMENT_UNUSED && !attachmentDescriptionEquals(attDescs[attIdx], o->attDescs[attIdx]))
            return false;
    }

    if (hasDepthStencil) {
        const uint32_t attIdx = dsRef.attachment;
        if (attIdx != o->dsRef.attachment)
            return false;
        if (attIdx != VK_ATTACHMENT_UNUSED && !attachmentDescriptionEquals(attDescs[attIdx], o->attDescs[attIdx]))
            return false;
    }

    for (int i = 0, ie = resolveRefs.count(); i != ie; ++i) {
        const uint32_t attIdx = resolveRefs[i].attachment;
        if (attIdx != o->resolveRefs[i].attachment)
            return false;
        if (attIdx != VK_ATTACHMENT_UNUSED && !attachmentDescriptionEquals(attDescs[attIdx], o->attDescs[attIdx]))
            return false;
    }

    return true;
}

const QRhiNativeHandles *QVkRenderPassDescriptor::nativeHandles()
{
    nativeHandlesStruct.renderPass = rp;
    return &nativeHandlesStruct;
}

QVkReferenceRenderTarget::QVkReferenceRenderTarget(QRhiImplementation *rhi)
    : QRhiRenderTarget(rhi)
{
}

QVkReferenceRenderTarget::~QVkReferenceRenderTarget()
{
    release();
}

void QVkReferenceRenderTarget::release()
{
    // nothing to do here
}

QSize QVkReferenceRenderTarget::pixelSize() const
{
    return d.pixelSize;
}

float QVkReferenceRenderTarget::devicePixelRatio() const
{
    return d.dpr;
}

int QVkReferenceRenderTarget::sampleCount() const
{
    return d.sampleCount;
}

QVkTextureRenderTarget::QVkTextureRenderTarget(QRhiImplementation *rhi,
                                               const QRhiTextureRenderTargetDescription &desc,
                                               Flags flags)
    : QRhiTextureRenderTarget(rhi, desc, flags)
{
    for (int att = 0; att < QVkRenderTargetData::MAX_COLOR_ATTACHMENTS; ++att) {
        rtv[att] = VK_NULL_HANDLE;
        resrtv[att] = VK_NULL_HANDLE;
    }
}

QVkTextureRenderTarget::~QVkTextureRenderTarget()
{
    release();
}

void QVkTextureRenderTarget::release()
{
    if (!d.fb)
        return;

    QRhiVulkan::DeferredReleaseEntry e;
    e.type = QRhiVulkan::DeferredReleaseEntry::TextureRenderTarget;
    e.lastActiveFrameSlot = lastActiveFrameSlot;

    e.textureRenderTarget.fb = d.fb;
    d.fb = VK_NULL_HANDLE;

    for (int att = 0; att < QVkRenderTargetData::MAX_COLOR_ATTACHMENTS; ++att) {
        e.textureRenderTarget.rtv[att] = rtv[att];
        e.textureRenderTarget.resrtv[att] = resrtv[att];
        rtv[att] = VK_NULL_HANDLE;
        resrtv[att] = VK_NULL_HANDLE;
    }

    QRHI_RES_RHI(QRhiVulkan);
    rhiD->releaseQueue.append(e);

    rhiD->unregisterResource(this);
}

QRhiRenderPassDescriptor *QVkTextureRenderTarget::newCompatibleRenderPassDescriptor()
{
    // not yet built so cannot rely on data computed in build()

    QRHI_RES_RHI(QRhiVulkan);
    QVkRenderPassDescriptor *rp = new QVkRenderPassDescriptor(m_rhi);
    if (!rhiD->createOffscreenRenderPass(rp,
                                         m_desc.cbeginColorAttachments(),
                                         m_desc.cendColorAttachments(),
                                         m_flags.testFlag(QRhiTextureRenderTarget::PreserveColorContents),
                                         m_flags.testFlag(QRhiTextureRenderTarget::PreserveDepthStencilContents),
                                         m_desc.depthStencilBuffer(),
                                         m_desc.depthTexture()))
    {
        delete rp;
        return nullptr;
    }

    rp->ownsRp = true;
    rhiD->registerResource(rp);
    return rp;
}

bool QVkTextureRenderTarget::build()
{
    if (d.fb)
        release();

    const bool hasColorAttachments = m_desc.cbeginColorAttachments() != m_desc.cendColorAttachments();
    Q_ASSERT(hasColorAttachments || m_desc.depthTexture());
    Q_ASSERT(!m_desc.depthStencilBuffer() || !m_desc.depthTexture());
    const bool hasDepthStencil = m_desc.depthStencilBuffer() || m_desc.depthTexture();

    QRHI_RES_RHI(QRhiVulkan);
    QVarLengthArray<VkImageView, 8> views;

    d.colorAttCount = 0;
    int attIndex = 0;
    for (auto it = m_desc.cbeginColorAttachments(), itEnd = m_desc.cendColorAttachments(); it != itEnd; ++it, ++attIndex) {
        d.colorAttCount += 1;
        QVkTexture *texD = QRHI_RES(QVkTexture, it->texture());
        QVkRenderBuffer *rbD = QRHI_RES(QVkRenderBuffer, it->renderBuffer());
        Q_ASSERT(texD || rbD);
        if (texD) {
            Q_ASSERT(texD->flags().testFlag(QRhiTexture::RenderTarget));
            VkImageViewCreateInfo viewInfo;
            memset(&viewInfo, 0, sizeof(viewInfo));
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = texD->image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = texD->vkformat;
            viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = uint32_t(it->level());
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = uint32_t(it->layer());
            viewInfo.subresourceRange.layerCount = 1;
            VkResult err = rhiD->df->vkCreateImageView(rhiD->dev, &viewInfo, nullptr, &rtv[attIndex]);
            if (err != VK_SUCCESS) {
                qWarning("Failed to create render target image view: %d", err);
                return false;
            }
            views.append(rtv[attIndex]);
            if (attIndex == 0) {
                d.pixelSize = texD->pixelSize();
                d.sampleCount = texD->samples;
            }
        } else if (rbD) {
            Q_ASSERT(rbD->backingTexture);
            views.append(rbD->backingTexture->imageView);
            if (attIndex == 0) {
                d.pixelSize = rbD->pixelSize();
                d.sampleCount = rbD->samples;
            }
        }
    }
    d.dpr = 1;

    if (hasDepthStencil) {
        if (m_desc.depthTexture()) {
            QVkTexture *depthTexD = QRHI_RES(QVkTexture, m_desc.depthTexture());
            views.append(depthTexD->imageView);
            if (d.colorAttCount == 0) {
                d.pixelSize = depthTexD->pixelSize();
                d.sampleCount = depthTexD->samples;
            }
        } else {
            QVkRenderBuffer *depthRbD = QRHI_RES(QVkRenderBuffer, m_desc.depthStencilBuffer());
            views.append(depthRbD->imageView);
            if (d.colorAttCount == 0) {
                d.pixelSize = depthRbD->pixelSize();
                d.sampleCount = depthRbD->samples;
            }
        }
        d.dsAttCount = 1;
    } else {
        d.dsAttCount = 0;
    }

    d.resolveAttCount = 0;
    attIndex = 0;
    for (auto it = m_desc.cbeginColorAttachments(), itEnd = m_desc.cendColorAttachments(); it != itEnd; ++it, ++attIndex) {
        if (it->resolveTexture()) {
            QVkTexture *resTexD = QRHI_RES(QVkTexture, it->resolveTexture());
            Q_ASSERT(resTexD->flags().testFlag(QRhiTexture::RenderTarget));
            d.resolveAttCount += 1;

            VkImageViewCreateInfo viewInfo;
            memset(&viewInfo, 0, sizeof(viewInfo));
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = resTexD->image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = resTexD->vkformat;
            viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = uint32_t(it->resolveLevel());
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = uint32_t(it->resolveLayer());
            viewInfo.subresourceRange.layerCount = 1;
            VkResult err = rhiD->df->vkCreateImageView(rhiD->dev, &viewInfo, nullptr, &resrtv[attIndex]);
            if (err != VK_SUCCESS) {
                qWarning("Failed to create render target resolve image view: %d", err);
                return false;
            }
            views.append(resrtv[attIndex]);
        }
    }

    if (!m_renderPassDesc)
        qWarning("QVkTextureRenderTarget: No renderpass descriptor set. See newCompatibleRenderPassDescriptor() and setRenderPassDescriptor().");

    d.rp = QRHI_RES(QVkRenderPassDescriptor, m_renderPassDesc);
    Q_ASSERT(d.rp && d.rp->rp);

    VkFramebufferCreateInfo fbInfo;
    memset(&fbInfo, 0, sizeof(fbInfo));
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = d.rp->rp;
    fbInfo.attachmentCount = uint32_t(d.colorAttCount + d.dsAttCount + d.resolveAttCount);
    fbInfo.pAttachments = views.constData();
    fbInfo.width = uint32_t(d.pixelSize.width());
    fbInfo.height = uint32_t(d.pixelSize.height());
    fbInfo.layers = 1;

    VkResult err = rhiD->df->vkCreateFramebuffer(rhiD->dev, &fbInfo, nullptr, &d.fb);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create framebuffer: %d", err);
        return false;
    }

    lastActiveFrameSlot = -1;
    rhiD->registerResource(this);
    return true;
}

QSize QVkTextureRenderTarget::pixelSize() const
{
    return d.pixelSize;
}

float QVkTextureRenderTarget::devicePixelRatio() const
{
    return d.dpr;
}

int QVkTextureRenderTarget::sampleCount() const
{
    return d.sampleCount;
}

QVkShaderResourceBindings::QVkShaderResourceBindings(QRhiImplementation *rhi)
    : QRhiShaderResourceBindings(rhi)
{
}

QVkShaderResourceBindings::~QVkShaderResourceBindings()
{
    release();
}

void QVkShaderResourceBindings::release()
{
    if (!layout)
        return;

    sortedBindings.clear();

    QRhiVulkan::DeferredReleaseEntry e;
    e.type = QRhiVulkan::DeferredReleaseEntry::ShaderResourceBindings;
    e.lastActiveFrameSlot = lastActiveFrameSlot;

    e.shaderResourceBindings.poolIndex = poolIndex;
    e.shaderResourceBindings.layout = layout;

    poolIndex = -1;
    layout = VK_NULL_HANDLE;
    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i)
        descSets[i] = VK_NULL_HANDLE;

    QRHI_RES_RHI(QRhiVulkan);
    rhiD->releaseQueue.append(e);

    rhiD->unregisterResource(this);
}

bool QVkShaderResourceBindings::build()
{
    if (layout)
        release();

    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i)
        descSets[i] = VK_NULL_HANDLE;

    sortedBindings.clear();
    std::copy(m_bindings.cbegin(), m_bindings.cend(), std::back_inserter(sortedBindings));
    std::sort(sortedBindings.begin(), sortedBindings.end(),
              [](const QRhiShaderResourceBinding &a, const QRhiShaderResourceBinding &b)
    {
        return a.data()->binding < b.data()->binding;
    });

    QVarLengthArray<VkDescriptorSetLayoutBinding, 4> vkbindings;
    for (const QRhiShaderResourceBinding &binding : qAsConst(sortedBindings)) {
        const QRhiShaderResourceBinding::Data *b = binding.data();
        VkDescriptorSetLayoutBinding vkbinding;
        memset(&vkbinding, 0, sizeof(vkbinding));
        vkbinding.binding = uint32_t(b->binding);
        vkbinding.descriptorType = toVkDescriptorType(b);
        if (b->type == QRhiShaderResourceBinding::SampledTexture)
            vkbinding.descriptorCount = b->u.stex.count;
        else
            vkbinding.descriptorCount = 1;
        vkbinding.stageFlags = toVkShaderStageFlags(b->stage);
        vkbindings.append(vkbinding);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo;
    memset(&layoutInfo, 0, sizeof(layoutInfo));
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = uint32_t(vkbindings.count());
    layoutInfo.pBindings = vkbindings.constData();

    QRHI_RES_RHI(QRhiVulkan);
    VkResult err = rhiD->df->vkCreateDescriptorSetLayout(rhiD->dev, &layoutInfo, nullptr, &layout);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create descriptor set layout: %d", err);
        return false;
    }

    VkDescriptorSetAllocateInfo allocInfo;
    memset(&allocInfo, 0, sizeof(allocInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorSetCount = QVK_FRAMES_IN_FLIGHT;
    VkDescriptorSetLayout layouts[QVK_FRAMES_IN_FLIGHT];
    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i)
        layouts[i] = layout;
    allocInfo.pSetLayouts = layouts;
    if (!rhiD->allocateDescriptorSet(&allocInfo, descSets, &poolIndex))
        return false;

    rhiD->updateShaderResourceBindings(this);

    lastActiveFrameSlot = -1;
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QVkGraphicsPipeline::QVkGraphicsPipeline(QRhiImplementation *rhi)
    : QRhiGraphicsPipeline(rhi)
{
}

QVkGraphicsPipeline::~QVkGraphicsPipeline()
{
    release();
}

void QVkGraphicsPipeline::release()
{
    if (!pipeline && !layout)
        return;

    QRhiVulkan::DeferredReleaseEntry e;
    e.type = QRhiVulkan::DeferredReleaseEntry::Pipeline;
    e.lastActiveFrameSlot = lastActiveFrameSlot;

    e.pipelineState.pipeline = pipeline;
    e.pipelineState.layout = layout;

    pipeline = VK_NULL_HANDLE;
    layout = VK_NULL_HANDLE;

    QRHI_RES_RHI(QRhiVulkan);
    rhiD->releaseQueue.append(e);

    rhiD->unregisterResource(this);
}

bool QVkGraphicsPipeline::build()
{
    if (pipeline)
        release();

    QRHI_RES_RHI(QRhiVulkan);
    if (!rhiD->sanityCheckGraphicsPipeline(this))
        return false;

    if (!rhiD->ensurePipelineCache())
        return false;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    QVkShaderResourceBindings *srbD = QRHI_RES(QVkShaderResourceBindings, m_shaderResourceBindings);
    Q_ASSERT(m_shaderResourceBindings && srbD->layout);
    pipelineLayoutInfo.pSetLayouts = &srbD->layout;
    VkResult err = rhiD->df->vkCreatePipelineLayout(rhiD->dev, &pipelineLayoutInfo, nullptr, &layout);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create pipeline layout: %d", err);
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo;
    memset(&pipelineInfo, 0, sizeof(pipelineInfo));
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    QVarLengthArray<VkShaderModule, 4> shaders;
    QVarLengthArray<VkPipelineShaderStageCreateInfo, 4> shaderStageCreateInfos;
    for (const QRhiShaderStage &shaderStage : m_shaderStages) {
        const QShader bakedShader = shaderStage.shader();
        const QShaderCode spirv = bakedShader.shader({ QShader::SpirvShader, 100, shaderStage.shaderVariant() });
        if (spirv.shader().isEmpty()) {
            qWarning() << "No SPIR-V 1.0 shader code found in baked shader" << bakedShader;
            return false;
        }
        VkShaderModule shader = rhiD->createShader(spirv.shader());
        if (shader) {
            shaders.append(shader);
            VkPipelineShaderStageCreateInfo shaderInfo;
            memset(&shaderInfo, 0, sizeof(shaderInfo));
            shaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderInfo.stage = toVkShaderStage(shaderStage.type());
            shaderInfo.module = shader;
            shaderInfo.pName = spirv.entryPoint().constData();
            shaderStageCreateInfos.append(shaderInfo);
        }
    }
    pipelineInfo.stageCount = uint32_t(shaderStageCreateInfos.count());
    pipelineInfo.pStages = shaderStageCreateInfos.constData();

    QVarLengthArray<VkVertexInputBindingDescription, 4> vertexBindings;
    QVarLengthArray<VkVertexInputBindingDivisorDescriptionEXT> nonOneStepRates;
    int bindingIndex = 0;
    for (auto it = m_vertexInputLayout.cbeginBindings(), itEnd = m_vertexInputLayout.cendBindings();
         it != itEnd; ++it, ++bindingIndex)
    {
        VkVertexInputBindingDescription bindingInfo = {
            uint32_t(bindingIndex),
            it->stride(),
            it->classification() == QRhiVertexInputBinding::PerVertex
                ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE
        };
        if (it->classification() == QRhiVertexInputBinding::PerInstance && it->instanceStepRate() != 1) {
            if (rhiD->vertexAttribDivisorAvailable) {
                nonOneStepRates.append({ uint32_t(bindingIndex), uint32_t(it->instanceStepRate()) });
            } else {
                qWarning("QRhiVulkan: Instance step rates other than 1 not supported without "
                         "VK_EXT_vertex_attribute_divisor on the device and "
                         "VK_KHR_get_physical_device_properties2 on the instance");
            }
        }
        vertexBindings.append(bindingInfo);
    }
    QVarLengthArray<VkVertexInputAttributeDescription, 4> vertexAttributes;
    for (auto it = m_vertexInputLayout.cbeginAttributes(), itEnd = m_vertexInputLayout.cendAttributes();
         it != itEnd; ++it)
    {
        VkVertexInputAttributeDescription attributeInfo = {
            uint32_t(it->location()),
            uint32_t(it->binding()),
            toVkAttributeFormat(it->format()),
            it->offset()
        };
        vertexAttributes.append(attributeInfo);
    }
    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    memset(&vertexInputInfo, 0, sizeof(vertexInputInfo));
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = uint32_t(vertexBindings.count());
    vertexInputInfo.pVertexBindingDescriptions = vertexBindings.constData();
    vertexInputInfo.vertexAttributeDescriptionCount = uint32_t(vertexAttributes.count());
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.constData();
    VkPipelineVertexInputDivisorStateCreateInfoEXT divisorInfo;
    if (!nonOneStepRates.isEmpty()) {
        memset(&divisorInfo, 0, sizeof(divisorInfo));
        divisorInfo.sType = VkStructureType(1000190001); // VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT
        divisorInfo.vertexBindingDivisorCount = uint32_t(nonOneStepRates.count());
        divisorInfo.pVertexBindingDivisors = nonOneStepRates.constData();
        vertexInputInfo.pNext = &divisorInfo;
    }
    pipelineInfo.pVertexInputState = &vertexInputInfo;

    QVarLengthArray<VkDynamicState, 8> dynEnable;
    dynEnable << VK_DYNAMIC_STATE_VIEWPORT;
    dynEnable << VK_DYNAMIC_STATE_SCISSOR; // ignore UsesScissor - Vulkan requires a scissor for the viewport always
    if (m_flags.testFlag(QRhiGraphicsPipeline::UsesBlendConstants))
        dynEnable << VK_DYNAMIC_STATE_BLEND_CONSTANTS;
    if (m_flags.testFlag(QRhiGraphicsPipeline::UsesStencilRef))
        dynEnable << VK_DYNAMIC_STATE_STENCIL_REFERENCE;

    VkPipelineDynamicStateCreateInfo dynamicInfo;
    memset(&dynamicInfo, 0, sizeof(dynamicInfo));
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount = uint32_t(dynEnable.count());
    dynamicInfo.pDynamicStates = dynEnable.constData();
    pipelineInfo.pDynamicState = &dynamicInfo;

    VkPipelineViewportStateCreateInfo viewportInfo;
    memset(&viewportInfo, 0, sizeof(viewportInfo));
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = viewportInfo.scissorCount = 1;
    pipelineInfo.pViewportState = &viewportInfo;

    VkPipelineInputAssemblyStateCreateInfo inputAsmInfo;
    memset(&inputAsmInfo, 0, sizeof(inputAsmInfo));
    inputAsmInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAsmInfo.topology = toVkTopology(m_topology);
    inputAsmInfo.primitiveRestartEnable = (m_topology == TriangleStrip || m_topology == LineStrip);
    pipelineInfo.pInputAssemblyState = &inputAsmInfo;

    VkPipelineRasterizationStateCreateInfo rastInfo;
    memset(&rastInfo, 0, sizeof(rastInfo));
    rastInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rastInfo.cullMode = toVkCullMode(m_cullMode);
    rastInfo.frontFace = toVkFrontFace(m_frontFace);
    if (m_depthBias != 0 || !qFuzzyIsNull(m_slopeScaledDepthBias)) {
        rastInfo.depthBiasEnable = true;
        rastInfo.depthBiasConstantFactor = float(m_depthBias);
        rastInfo.depthBiasSlopeFactor = m_slopeScaledDepthBias;
    }
    rastInfo.lineWidth = rhiD->hasWideLines ? m_lineWidth : 1.0f;
    pipelineInfo.pRasterizationState = &rastInfo;

    VkPipelineMultisampleStateCreateInfo msInfo;
    memset(&msInfo, 0, sizeof(msInfo));
    msInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msInfo.rasterizationSamples = rhiD->effectiveSampleCount(m_sampleCount);
    pipelineInfo.pMultisampleState = &msInfo;

    VkPipelineDepthStencilStateCreateInfo dsInfo;
    memset(&dsInfo, 0, sizeof(dsInfo));
    dsInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    dsInfo.depthTestEnable = m_depthTest;
    dsInfo.depthWriteEnable = m_depthWrite;
    dsInfo.depthCompareOp = toVkCompareOp(m_depthOp);
    dsInfo.stencilTestEnable = m_stencilTest;
    if (m_stencilTest) {
        fillVkStencilOpState(&dsInfo.front, m_stencilFront);
        dsInfo.front.compareMask = m_stencilReadMask;
        dsInfo.front.writeMask = m_stencilWriteMask;
        fillVkStencilOpState(&dsInfo.back, m_stencilBack);
        dsInfo.back.compareMask = m_stencilReadMask;
        dsInfo.back.writeMask = m_stencilWriteMask;
    }
    pipelineInfo.pDepthStencilState = &dsInfo;

    VkPipelineColorBlendStateCreateInfo blendInfo;
    memset(&blendInfo, 0, sizeof(blendInfo));
    blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    QVarLengthArray<VkPipelineColorBlendAttachmentState, 4> vktargetBlends;
    for (const QRhiGraphicsPipeline::TargetBlend &b : qAsConst(m_targetBlends)) {
        VkPipelineColorBlendAttachmentState blend;
        memset(&blend, 0, sizeof(blend));
        blend.blendEnable = b.enable;
        blend.srcColorBlendFactor = toVkBlendFactor(b.srcColor);
        blend.dstColorBlendFactor = toVkBlendFactor(b.dstColor);
        blend.colorBlendOp = toVkBlendOp(b.opColor);
        blend.srcAlphaBlendFactor = toVkBlendFactor(b.srcAlpha);
        blend.dstAlphaBlendFactor = toVkBlendFactor(b.dstAlpha);
        blend.alphaBlendOp = toVkBlendOp(b.opAlpha);
        blend.colorWriteMask = toVkColorComponents(b.colorWrite);
        vktargetBlends.append(blend);
    }
    if (vktargetBlends.isEmpty()) {
        VkPipelineColorBlendAttachmentState blend;
        memset(&blend, 0, sizeof(blend));
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        vktargetBlends.append(blend);
    }
    blendInfo.attachmentCount = uint32_t(vktargetBlends.count());
    blendInfo.pAttachments = vktargetBlends.constData();
    pipelineInfo.pColorBlendState = &blendInfo;

    pipelineInfo.layout = layout;

    Q_ASSERT(m_renderPassDesc && QRHI_RES(const QVkRenderPassDescriptor, m_renderPassDesc)->rp);
    pipelineInfo.renderPass = QRHI_RES(const QVkRenderPassDescriptor, m_renderPassDesc)->rp;

    err = rhiD->df->vkCreateGraphicsPipelines(rhiD->dev, rhiD->pipelineCache, 1, &pipelineInfo, nullptr, &pipeline);

    for (VkShaderModule shader : shaders)
        rhiD->df->vkDestroyShaderModule(rhiD->dev, shader, nullptr);

    if (err != VK_SUCCESS) {
        qWarning("Failed to create graphics pipeline: %d", err);
        return false;
    }

    lastActiveFrameSlot = -1;
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QVkComputePipeline::QVkComputePipeline(QRhiImplementation *rhi)
    : QRhiComputePipeline(rhi)
{
}

QVkComputePipeline::~QVkComputePipeline()
{
    release();
}

void QVkComputePipeline::release()
{
    if (!pipeline && !layout)
        return;

    QRhiVulkan::DeferredReleaseEntry e;
    e.type = QRhiVulkan::DeferredReleaseEntry::Pipeline;
    e.lastActiveFrameSlot = lastActiveFrameSlot;

    e.pipelineState.pipeline = pipeline;
    e.pipelineState.layout = layout;

    pipeline = VK_NULL_HANDLE;
    layout = VK_NULL_HANDLE;

    QRHI_RES_RHI(QRhiVulkan);
    rhiD->releaseQueue.append(e);

    rhiD->unregisterResource(this);
}

bool QVkComputePipeline::build()
{
    if (pipeline)
        release();

    QRHI_RES_RHI(QRhiVulkan);
    if (!rhiD->ensurePipelineCache())
        return false;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    QVkShaderResourceBindings *srbD = QRHI_RES(QVkShaderResourceBindings, m_shaderResourceBindings);
    Q_ASSERT(m_shaderResourceBindings && srbD->layout);
    pipelineLayoutInfo.pSetLayouts = &srbD->layout;
    VkResult err = rhiD->df->vkCreatePipelineLayout(rhiD->dev, &pipelineLayoutInfo, nullptr, &layout);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create pipeline layout: %d", err);
        return false;
    }

    VkComputePipelineCreateInfo pipelineInfo;
    memset(&pipelineInfo, 0, sizeof(pipelineInfo));
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = layout;

    if (m_shaderStage.type() != QRhiShaderStage::Compute) {
        qWarning("Compute pipeline requires a compute shader stage");
        return false;
    }
    const QShader bakedShader = m_shaderStage.shader();
    const QShaderCode spirv = bakedShader.shader({ QShader::SpirvShader, 100, m_shaderStage.shaderVariant() });
    if (spirv.shader().isEmpty()) {
        qWarning() << "No SPIR-V 1.0 shader code found in baked shader" << bakedShader;
        return false;
    }
    if (bakedShader.stage() != QShader::ComputeStage) {
        qWarning() << bakedShader << "is not a compute shader";
        return false;
    }
    VkShaderModule shader = rhiD->createShader(spirv.shader());
    VkPipelineShaderStageCreateInfo shaderInfo;
    memset(&shaderInfo, 0, sizeof(shaderInfo));
    shaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderInfo.module = shader;
    shaderInfo.pName = spirv.entryPoint().constData();
    pipelineInfo.stage = shaderInfo;

    err = rhiD->df->vkCreateComputePipelines(rhiD->dev, rhiD->pipelineCache, 1, &pipelineInfo, nullptr, &pipeline);
    rhiD->df->vkDestroyShaderModule(rhiD->dev, shader, nullptr);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create graphics pipeline: %d", err);
        return false;
    }

    lastActiveFrameSlot = -1;
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QVkCommandBuffer::QVkCommandBuffer(QRhiImplementation *rhi)
    : QRhiCommandBuffer(rhi)
{
    resetState();
}

QVkCommandBuffer::~QVkCommandBuffer()
{
    release();
}

void QVkCommandBuffer::release()
{
    // nothing to do here, cb is not owned by us
}

const QRhiNativeHandles *QVkCommandBuffer::nativeHandles()
{
    // Ok this is messy but no other way has been devised yet. Outside
    // begin(Compute)Pass - end(Compute)Pass it is simple - just return the
    // primary VkCommandBuffer. Inside, however, we need to provide the current
    // secondary command buffer (typically the one started by beginExternal(),
    // in case we are between beginExternal - endExternal inside a pass).

    if (useSecondaryCb && !secondaryCbs.isEmpty())
        nativeHandlesStruct.commandBuffer = secondaryCbs.last();
    else
        nativeHandlesStruct.commandBuffer = cb;

    return &nativeHandlesStruct;
}

QVkSwapChain::QVkSwapChain(QRhiImplementation *rhi)
    : QRhiSwapChain(rhi),
      rtWrapper(rhi),
      cbWrapper(rhi)
{
}

QVkSwapChain::~QVkSwapChain()
{
    release();
}

void QVkSwapChain::release()
{
    if (sc == VK_NULL_HANDLE)
        return;

    QRHI_RES_RHI(QRhiVulkan);
    rhiD->swapchains.remove(this);
    rhiD->releaseSwapChainResources(this);
    surface = lastConnectedSurface = VK_NULL_HANDLE;

    QRHI_PROF;
    QRHI_PROF_F(releaseSwapChain(this));

    rhiD->unregisterResource(this);
}

QRhiCommandBuffer *QVkSwapChain::currentFrameCommandBuffer()
{
    return &cbWrapper;
}

QRhiRenderTarget *QVkSwapChain::currentFrameRenderTarget()
{
    return &rtWrapper;
}

QSize QVkSwapChain::surfacePixelSize()
{
    if (!ensureSurface())
        return QSize();

    // The size from the QWindow may not exactly match the surface... so if a
    // size is reported from the surface, use that.
    VkSurfaceCapabilitiesKHR surfaceCaps;
    memset(&surfaceCaps, 0, sizeof(surfaceCaps));
    QRHI_RES_RHI(QRhiVulkan);
    rhiD->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(rhiD->physDev, surface, &surfaceCaps);
    VkExtent2D bufferSize = surfaceCaps.currentExtent;
    if (bufferSize.width == uint32_t(-1)) {
        Q_ASSERT(bufferSize.height == uint32_t(-1));
        return m_window->size() * m_window->devicePixelRatio();
    }
    return QSize(int(bufferSize.width), int(bufferSize.height));
}

QRhiRenderPassDescriptor *QVkSwapChain::newCompatibleRenderPassDescriptor()
{
    // not yet built so cannot rely on data computed in buildOrResize()

    if (!ensureSurface()) // make sure sampleCount and colorFormat reflect what was requested
        return nullptr;

    QRHI_RES_RHI(QRhiVulkan);
    QVkRenderPassDescriptor *rp = new QVkRenderPassDescriptor(m_rhi);
    if (!rhiD->createDefaultRenderPass(rp,
                                       m_depthStencil != nullptr,
                                       samples,
                                       colorFormat))
    {
        delete rp;
        return nullptr;
    }

    rp->ownsRp = true;
    rhiD->registerResource(rp);
    return rp;
}

static inline bool isSrgbFormat(VkFormat format)
{
    switch (format) {
    case VK_FORMAT_R8_SRGB:
    case VK_FORMAT_R8G8_SRGB:
    case VK_FORMAT_R8G8B8_SRGB:
    case VK_FORMAT_B8G8R8_SRGB:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_SRGB:
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        return true;
    default:
        return false;
    }
}

bool QVkSwapChain::ensureSurface()
{
    // Do nothing when already done, however window may change so check the
    // surface is still the same. Some of the queries below are very expensive
    // with some implementations so it is important to do the rest only once
    // per surface.

    Q_ASSERT(m_window);
    VkSurfaceKHR surf = QVulkanInstance::surfaceForWindow(m_window);
    if (!surf) {
        qWarning("Failed to get surface for window");
        return false;
    }
    if (surface == surf)
        return true;

    surface = surf;

    QRHI_RES_RHI(QRhiVulkan);
    if (rhiD->gfxQueueFamilyIdx != -1) {
        if (!rhiD->inst->supportsPresent(rhiD->physDev, uint32_t(rhiD->gfxQueueFamilyIdx), m_window)) {
            qWarning("Presenting not supported on this window");
            return false;
        }
    }

    if (!rhiD->vkGetPhysicalDeviceSurfaceCapabilitiesKHR) {
        rhiD->vkGetPhysicalDeviceSurfaceCapabilitiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
                    rhiD->inst->getInstanceProcAddr("vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
        rhiD->vkGetPhysicalDeviceSurfaceFormatsKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
                    rhiD->inst->getInstanceProcAddr("vkGetPhysicalDeviceSurfaceFormatsKHR"));
        rhiD->vkGetPhysicalDeviceSurfacePresentModesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
                    rhiD->inst->getInstanceProcAddr("vkGetPhysicalDeviceSurfacePresentModesKHR"));
        if (!rhiD->vkGetPhysicalDeviceSurfaceCapabilitiesKHR
                || !rhiD->vkGetPhysicalDeviceSurfaceFormatsKHR
                || !rhiD->vkGetPhysicalDeviceSurfacePresentModesKHR)
        {
            qWarning("Physical device surface queries not available");
            return false;
        }
    }

    quint32 formatCount = 0;
    rhiD->vkGetPhysicalDeviceSurfaceFormatsKHR(rhiD->physDev, surface, &formatCount, nullptr);
    QVector<VkSurfaceFormatKHR> formats(formatCount);
    if (formatCount)
        rhiD->vkGetPhysicalDeviceSurfaceFormatsKHR(rhiD->physDev, surface, &formatCount, formats.data());

    const bool srgbRequested = m_flags.testFlag(sRGB);
    for (int i = 0; i < int(formatCount); ++i) {
        if (formats[i].format != VK_FORMAT_UNDEFINED && srgbRequested == isSrgbFormat(formats[i].format)) {
            colorFormat = formats[i].format;
            colorSpace = formats[i].colorSpace;
            break;
        }
    }

    samples = rhiD->effectiveSampleCount(m_sampleCount);

    quint32 presModeCount = 0;
    rhiD->vkGetPhysicalDeviceSurfacePresentModesKHR(rhiD->physDev, surface, &presModeCount, nullptr);
    QVector<VkPresentModeKHR> presModes(presModeCount);
    rhiD->vkGetPhysicalDeviceSurfacePresentModesKHR(rhiD->physDev, surface, &presModeCount, presModes.data());
    supportedPresentationModes = presModes;

    return true;
}

bool QVkSwapChain::buildOrResize()
{
    QRHI_RES_RHI(QRhiVulkan);
    const bool needsRegistration = !window || window != m_window;

    // Can be called multiple times due to window resizes - that is not the
    // same as a simple release+build (as with other resources). Thus no
    // release() here. See recreateSwapChain().

    // except if the window actually changes
    if (window && window != m_window)
        release();

    window = m_window;
    m_currentPixelSize = surfacePixelSize();
    pixelSize = m_currentPixelSize;

    if (!rhiD->recreateSwapChain(this)) {
        qWarning("Failed to create new swapchain");
        return false;
    }

    if (needsRegistration)
        rhiD->swapchains.insert(this);

    if (m_depthStencil && m_depthStencil->sampleCount() != m_sampleCount) {
        qWarning("Depth-stencil buffer's sampleCount (%d) does not match color buffers' sample count (%d). Expect problems.",
                 m_depthStencil->sampleCount(), m_sampleCount);
    }
    if (m_depthStencil && m_depthStencil->pixelSize() != pixelSize) {
        if (m_depthStencil->flags().testFlag(QRhiRenderBuffer::UsedWithSwapChainOnly)) {
            m_depthStencil->setPixelSize(pixelSize);
            if (!m_depthStencil->build())
                qWarning("Failed to rebuild swapchain's associated depth-stencil buffer for size %dx%d",
                         pixelSize.width(), pixelSize.height());
        } else {
            qWarning("Depth-stencil buffer's size (%dx%d) does not match the surface size (%dx%d). Expect problems.",
                     m_depthStencil->pixelSize().width(), m_depthStencil->pixelSize().height(),
                     pixelSize.width(), pixelSize.height());
        }
    }

    if (!m_renderPassDesc)
        qWarning("QVkSwapChain: No renderpass descriptor set. See newCompatibleRenderPassDescriptor() and setRenderPassDescriptor().");

    rtWrapper.d.rp = QRHI_RES(QVkRenderPassDescriptor, m_renderPassDesc);
    Q_ASSERT(rtWrapper.d.rp && rtWrapper.d.rp->rp);

    rtWrapper.d.pixelSize = pixelSize;
    rtWrapper.d.dpr = float(window->devicePixelRatio());
    rtWrapper.d.sampleCount = samples;
    rtWrapper.d.colorAttCount = 1;
    if (m_depthStencil) {
        rtWrapper.d.dsAttCount = 1;
        ds = QRHI_RES(QVkRenderBuffer, m_depthStencil);
    } else {
        rtWrapper.d.dsAttCount = 0;
        ds = nullptr;
    }
    if (samples > VK_SAMPLE_COUNT_1_BIT)
        rtWrapper.d.resolveAttCount = 1;
    else
        rtWrapper.d.resolveAttCount = 0;

    for (int i = 0; i < bufferCount; ++i) {
        QVkSwapChain::ImageResources &image(imageRes[i]);
        VkImageView views[3] = { // color, ds, resolve
            samples > VK_SAMPLE_COUNT_1_BIT ? image.msaaImageView : image.imageView,
            ds ? ds->imageView : VK_NULL_HANDLE,
            samples > VK_SAMPLE_COUNT_1_BIT ? image.imageView : VK_NULL_HANDLE
        };

        VkFramebufferCreateInfo fbInfo;
        memset(&fbInfo, 0, sizeof(fbInfo));
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = rtWrapper.d.rp->rp;
        fbInfo.attachmentCount = uint32_t(rtWrapper.d.colorAttCount + rtWrapper.d.dsAttCount + rtWrapper.d.resolveAttCount);
        fbInfo.pAttachments = views;
        fbInfo.width = uint32_t(pixelSize.width());
        fbInfo.height = uint32_t(pixelSize.height());
        fbInfo.layers = 1;

        VkResult err = rhiD->df->vkCreateFramebuffer(rhiD->dev, &fbInfo, nullptr, &image.fb);
        if (err != VK_SUCCESS) {
            qWarning("Failed to create framebuffer: %d", err);
            return false;
        }
    }

    frameCount = 0;

    QRHI_PROF;
    QRHI_PROF_F(resizeSwapChain(this, QVK_FRAMES_IN_FLIGHT, samples > VK_SAMPLE_COUNT_1_BIT ? QVK_FRAMES_IN_FLIGHT : 0, samples));

    if (needsRegistration)
        rhiD->registerResource(this);

    return true;
}

QT_END_NAMESPACE
