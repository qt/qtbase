// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qrhivulkan_p.h"
#include <qpa/qplatformvulkaninstance.h>

#define VMA_IMPLEMENTATION
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_RECORDING_ENABLED 0
#define VMA_DEDICATED_ALLOCATION 0
#ifdef QT_DEBUG
#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#endif
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wsuggest-override")
#if defined(Q_CC_CLANG) && Q_CC_CLANG >= 1100
QT_WARNING_DISABLE_CLANG("-Wdeprecated-copy")
#endif
#include "vk_mem_alloc.h"
QT_WARNING_POP

#include <qmath.h>
#include <QVulkanFunctions>
#include <QtGui/qwindow.h>
#include <optional>

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
    \inmodule QtGui
    \since 6.6
    \brief Vulkan specific initialization parameters.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    A Vulkan-based QRhi needs at minimum a valid QVulkanInstance. It is up to
    the user to ensure this is available and initialized. This is typically
    done in main() similarly to the following:

    \badcode
    int main(int argc, char **argv)
    {
        ...

        QVulkanInstance inst;
        inst.setLayers({ "VK_LAYER_KHRONOS_validation" }); // for debugging only, not for release builds
        inst.setExtensions(QRhiVulkanInitParams::preferredInstanceExtensions());
        if (!inst.create())
            qFatal("Vulkan not available");

        ...
    }
    \endcode

    This example enables the
    \l{https://github.com/KhronosGroup/Vulkan-ValidationLayers}{Vulkan
    validation layers}, when they are available, and also enables the
    instance-level extensions QRhi reports as desirable (such as,
    VK_KHR_get_physical_device_properties2), as long as they are supported by
    the Vulkan implementation at run time.

    The former is optional, and is useful during the development phase
    QVulkanInstance conveniently redirects messages and warnings to qDebug.
    Avoid enabling it in production builds, however. The latter is strongly
    recommended, and is important in order to make certain features functional
    (for example, QRhi::CustomInstanceStepRate).

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

    It is expected that the backend's desired list of instance extensions will
    be queried by calling the static function preferredInstanceExtensions()
    before initializing a QVulkanInstance. The returned list can be safely
    passed to QVulkanInstance::setExtensions() as-is, because unsupported
    extensions are filtered out automatically. If this is not done, certain
    features, such as QRhi::CustomInstanceStepRate may be reported as
    unsupported even when the Vulkan implementation on the system has support
    for the relevant functionality.

    For full functionality the QVulkanInstance needs to have API 1.1 enabled,
    when available. This means calling QVulkanInstance::setApiVersion() with
    1.1 or higher whenever QVulkanInstance::supportedApiVersion() reports that
    at least Vulkan 1.1 is supported. If this is not done, certain features,
    such as QRhi::RenderTo3DTextureSlice may be reported as unsupported even
    when the Vulkan implementation on the system supports Vulkan 1.1 or newer.

    \section2 Working with existing Vulkan devices

    When interoperating with another graphics engine, it may be necessary to
    get a QRhi instance that uses the same Vulkan device. This can be achieved
    by passing a pointer to a QRhiVulkanNativeHandles to QRhi::create().

    The physical device must always be set to a non-null value. If the
    intention is to just specify a physical device, but leave the rest of the
    VkDevice and queue creation to QRhi, then no other members need to be
    filled out in the struct. For example, this is the case when working with
    OpenXR.

    To adopt an existing \c VkDevice, the device field must be set to a
    non-null value as well. In addition, the graphics queue family index is
    required. The queue index is optional, as the default of 0 is often
    suitable.

    Optionally, an existing command pool object can be specified as well. Also
    optionally, vmemAllocator can be used to share the same
    \l{https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator}{Vulkan
    memory allocator} between two QRhi instances.

    The QRhi does not take ownership of any of the external objects.

    Applications are encouraged to query the list of desired device extensions
    by calling the static function preferredExtensionsForImportedDevice(), and
    enable them on the VkDevice. Otherwise certain QRhi features may not be
    available.
 */

/*!
    \variable QRhiVulkanInitParams::inst

    The QVulkanInstance that has already been successfully
    \l{QVulkanInstance::create()}{created}, required.
*/

/*!
    \variable QRhiVulkanInitParams::window

    Optional, but recommended when targeting a QWindow.
*/

/*!
    \variable QRhiVulkanInitParams::deviceExtensions

    Optional, empty by default. The list of Vulkan device extensions to enable.
    Unsupported extensions are ignored gracefully.
*/

/*!
    \class QRhiVulkanNativeHandles
    \inmodule QtGui
    \since 6.6
    \brief Collects device, queue, and other Vulkan objects that are used by the QRhi.

    \note Ownership of the Vulkan objects is never transferred.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \variable QRhiVulkanNativeHandles::physDev

    When different from \nullptr, specifies the Vulkan physical device to use.
*/

/*!
    \variable QRhiVulkanNativeHandles::dev

    When wanting to import not just a physical device, but also use an already
    existing VkDevice, set this and the graphics queue index and family index.
*/

/*!
    \variable QRhiVulkanNativeHandles::gfxQueueFamilyIdx

    Graphics queue family index.
*/

/*!
    \variable QRhiVulkanNativeHandles::gfxQueueIdx

    Graphics queue index.
*/

/*!
    \variable QRhiVulkanNativeHandles::vmemAllocator

    Relevant only when importing an existing memory allocator object,
    leave it set to \nullptr otherwise.
*/

/*!
    \variable QRhiVulkanNativeHandles::gfxQueue

    Output only, not used by QRhi::create(), only set by the
    QRhi::nativeHandles() accessor. The graphics VkQueue used by the QRhi.
*/

/*!
    \variable QRhiVulkanNativeHandles::inst

    Output only, not used by QRhi::create(), only set by the
    QRhi::nativeHandles() accessor. The QVulkanInstance used by the QRhi.
*/

/*!
    \class QRhiVulkanCommandBufferNativeHandles
    \inmodule QtGui
    \since 6.6
    \brief Holds the Vulkan command buffer object that is backing a QRhiCommandBuffer.

    \note The Vulkan command buffer object is only guaranteed to be valid, and
    in recording state, while recording a frame. That is, between a
    \l{QRhi::beginFrame()}{beginFrame()} - \l{QRhi::endFrame()}{endFrame()} or
    \l{QRhi::beginOffscreenFrame()}{beginOffscreenFrame()} -
    \l{QRhi::endOffscreenFrame()}{endOffscreenFrame()} pair.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \variable QRhiVulkanCommandBufferNativeHandles::commandBuffer

    The VkCommandBuffer object.
*/

/*!
    \class QRhiVulkanRenderPassNativeHandles
    \inmodule QtGui
    \since 6.6
    \brief Holds the Vulkan render pass object backing a QRhiRenderPassDescriptor.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

/*!
    \variable QRhiVulkanRenderPassNativeHandles::renderPass

    The VkRenderPass object.
*/

template <class Int>
inline Int aligned(Int v, Int byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

static QVulkanInstance *globalVulkanInstance;

static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL wrap_vkGetInstanceProcAddr(VkInstance, const char *pName)
{
    return globalVulkanInstance->getInstanceProcAddr(pName);
}

static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL wrap_vkGetDeviceProcAddr(VkDevice device, const char *pName)
{
    return globalVulkanInstance->functions()->vkGetDeviceProcAddr(device, pName);
}

static inline VmaAllocation toVmaAllocation(QVkAlloc a)
{
    return reinterpret_cast<VmaAllocation>(a);
}

static inline VmaAllocator toVmaAllocator(QVkAllocator a)
{
    return reinterpret_cast<VmaAllocator>(a);
}

/*!
    \return the list of instance extensions that are expected to be enabled on
    the QVulkanInstance that is used for the Vulkan-based QRhi.

    The returned list can be safely passed to QVulkanInstance::setExtensions()
    as-is, because unsupported extensions are filtered out automatically.
 */
QByteArrayList QRhiVulkanInitParams::preferredInstanceExtensions()
{
    return {
        QByteArrayLiteral("VK_KHR_get_physical_device_properties2")
    };
}

/*!
    \return the list of device extensions that are expected to be enabled on the
    \c VkDevice when creating a Vulkan-based QRhi with an externally created
    \c VkDevice object.
 */
QByteArrayList QRhiVulkanInitParams::preferredExtensionsForImportedDevice()
{
    return {
        QByteArrayLiteral("VK_KHR_swapchain"),
        QByteArrayLiteral("VK_EXT_vertex_attribute_divisor")
    };
}

QRhiVulkan::QRhiVulkan(QRhiVulkanInitParams *params, QRhiVulkanNativeHandles *importParams)
    : ofr(this)
{
    inst = params->inst;
    maybeWindow = params->window; // may be null
    requestedDeviceExtensions = params->deviceExtensions;

    if (importParams) {
        physDev = importParams->physDev;
        dev = importParams->dev;
        if (dev && physDev) {
            importedDevice = true;
            gfxQueueFamilyIdx = importParams->gfxQueueFamilyIdx;
            gfxQueueIdx = importParams->gfxQueueIdx;
            // gfxQueue is output only, no point in accepting it as input
            if (importParams->vmemAllocator) {
                importedAllocator = true;
                allocator = importParams->vmemAllocator;
            }
        }
    }
}

static bool qvk_debug_filter(QVulkanInstance::DebugMessageSeverityFlags severity,
                             QVulkanInstance::DebugMessageTypeFlags type,
                             const void *callbackData)
{
    Q_UNUSED(severity);
    Q_UNUSED(type);
#ifdef VK_EXT_debug_utils
    const VkDebugUtilsMessengerCallbackDataEXT *d = static_cast<const VkDebugUtilsMessengerCallbackDataEXT *>(callbackData);

    // Filter out certain misleading validation layer messages, as per
    // VulkanMemoryAllocator documentation.
    if (strstr(d->pMessage, "Mapping an image with layout")
        && strstr(d->pMessage, "can result in undefined behavior if this memory is used by the device"))
    {
        return true;
    }

    // In certain cases allocateDescriptorSet() will attempt to allocate from a
    // pool that does not have enough descriptors of a certain type. This makes
    // the validation layer shout. However, this is not an error since we will
    // then move on to another pool. If there is a real error, a qWarning
    // message is shown by allocateDescriptorSet(), so the validation warning
    // does not have any value and is just noise.
    if (strstr(d->pMessage, "VUID-VkDescriptorSetAllocateInfo-descriptorPool-00307"))
        return true;
#else
    Q_UNUSED(callbackData);
#endif
    return false;
}

static inline QRhiDriverInfo::DeviceType toRhiDeviceType(VkPhysicalDeviceType type)
{
    switch (type) {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        return QRhiDriverInfo::UnknownDevice;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return QRhiDriverInfo::IntegratedDevice;
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return QRhiDriverInfo::DiscreteDevice;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        return QRhiDriverInfo::VirtualDevice;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return QRhiDriverInfo::CpuDevice;
    default:
        return QRhiDriverInfo::UnknownDevice;
    }
}

bool QRhiVulkan::create(QRhi::Flags flags)
{
    Q_ASSERT(inst);
    if (!inst->isValid()) {
        qWarning("Vulkan instance is not valid");
        return false;
    }

    rhiFlags = flags;
    qCDebug(QRHI_LOG_INFO, "Initializing QRhi Vulkan backend %p with flags %d", this, int(rhiFlags));

    globalVulkanInstance = inst; // used for function resolving in vkmemalloc callbacks
    f = inst->functions();
    if (QRHI_LOG_INFO().isEnabled(QtDebugMsg)) {
        qCDebug(QRHI_LOG_INFO, "Enabled instance extensions:");
        for (const char *ext : inst->extensions())
            qCDebug(QRHI_LOG_INFO, "  %s", ext);
    }
    caps.debugUtils = inst->extensions().contains(QByteArrayLiteral("VK_EXT_debug_utils"));

    QList<VkQueueFamilyProperties> queueFamilyProps;
    auto queryQueueFamilyProps = [this, &queueFamilyProps] {
        uint32_t queueCount = 0;
        f->vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueCount, nullptr);
        queueFamilyProps.resize(int(queueCount));
        f->vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueCount, queueFamilyProps.data());
    };

    // Choose a physical device, unless one was provided in importParams.
    if (!physDev) {
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
        f->vkGetPhysicalDeviceProperties(physDev, &physDevProperties);
    } else {
        f->vkGetPhysicalDeviceProperties(physDev, &physDevProperties);
        qCDebug(QRHI_LOG_INFO, "Using imported physical device '%s' %d.%d.%d (api %d.%d.%d vendor 0x%X device 0x%X type %d)",
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
    }

    caps.apiVersion = inst->apiVersion();

    // Check the physical device API version against the instance API version,
    // they do not have to match, which means whatever version was set in the
    // QVulkanInstance may not be legally used with a given device if the
    // physical device has a lower version.
    const QVersionNumber physDevApiVersion(VK_VERSION_MAJOR(physDevProperties.apiVersion),
                                           VK_VERSION_MINOR(physDevProperties.apiVersion)); // patch version left out intentionally
    if (physDevApiVersion < caps.apiVersion) {
        qCDebug(QRHI_LOG_INFO) << "Instance has api version" << caps.apiVersion
                               << "whereas the chosen physical device has" << physDevApiVersion
                               << "- restricting to the latter";
        caps.apiVersion = physDevApiVersion;
    }

    driverInfoStruct.deviceName = QByteArray(physDevProperties.deviceName);
    driverInfoStruct.deviceId = physDevProperties.deviceID;
    driverInfoStruct.vendorId = physDevProperties.vendorID;
    driverInfoStruct.deviceType = toRhiDeviceType(physDevProperties.deviceType);

#ifdef VK_VERSION_1_2 // Vulkan11Features is only in Vulkan 1.2
    VkPhysicalDeviceFeatures2 physDevFeaturesChainable = {};
    physDevFeaturesChainable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    physDevFeatures11 = {};
    physDevFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    physDevFeatures12 = {};
    physDevFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
#ifdef VK_VERSION_1_3
    physDevFeatures13 = {};
    physDevFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
#endif
    if (caps.apiVersion >= QVersionNumber(1, 2)) {
        physDevFeaturesChainable.pNext = &physDevFeatures11;
        physDevFeatures11.pNext = &physDevFeatures12;
#ifdef VK_VERSION_1_3
        if (caps.apiVersion >= QVersionNumber(1, 3))
            physDevFeatures12.pNext = &physDevFeatures13;
#endif
        f->vkGetPhysicalDeviceFeatures2(physDev, &physDevFeaturesChainable);
        memcpy(&physDevFeatures, &physDevFeaturesChainable.features, sizeof(VkPhysicalDeviceFeatures));
    } else
#endif // VK_VERSION_1_2
    {
        f->vkGetPhysicalDeviceFeatures(physDev, &physDevFeatures);
    }

    // Choose queue and create device, unless the device was specified in importParams.
    if (!importedDevice) {
        // We only support combined graphics+present queues. When it comes to
        // compute, only combined graphics+compute queue is used, compute gets
        // disabled otherwise.
        std::optional<uint32_t> gfxQueueFamilyIdxOpt;
        std::optional<uint32_t> computelessGfxQueueCandidateIdxOpt;
        queryQueueFamilyProps();
        const uint32_t queueFamilyCount = uint32_t(queueFamilyProps.size());
        for (uint32_t i = 0; i < queueFamilyCount; ++i) {
            qCDebug(QRHI_LOG_INFO, "queue family %u: flags=0x%x count=%u",
                    i, queueFamilyProps[i].queueFlags, queueFamilyProps[i].queueCount);
            if (!gfxQueueFamilyIdxOpt.has_value()
                    && (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    && (!maybeWindow || inst->supportsPresent(physDev, i, maybeWindow)))
            {
                if (queueFamilyProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                    gfxQueueFamilyIdxOpt = i;
                else if (!computelessGfxQueueCandidateIdxOpt.has_value())
                    computelessGfxQueueCandidateIdxOpt = i;
            }
        }
        if (gfxQueueFamilyIdxOpt.has_value()) {
            gfxQueueFamilyIdx = gfxQueueFamilyIdxOpt.value();
        } else {
            if (computelessGfxQueueCandidateIdxOpt.has_value()) {
                gfxQueueFamilyIdx = computelessGfxQueueCandidateIdxOpt.value();
            } else {
                qWarning("No graphics (or no graphics+present) queue family found");
                return false;
            }
        }

        VkDeviceQueueCreateInfo queueInfo = {};
        const float prio[] = { 0 };
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = gfxQueueFamilyIdx;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = prio;

        QList<const char *> devLayers;
        if (inst->layers().contains("VK_LAYER_KHRONOS_validation"))
            devLayers.append("VK_LAYER_KHRONOS_validation");

        QVulkanInfoVector<QVulkanExtension> devExts;
        uint32_t devExtCount = 0;
        f->vkEnumerateDeviceExtensionProperties(physDev, nullptr, &devExtCount, nullptr);
        if (devExtCount) {
            QList<VkExtensionProperties> extProps(devExtCount);
            f->vkEnumerateDeviceExtensionProperties(physDev, nullptr, &devExtCount, extProps.data());
            for (const VkExtensionProperties &p : std::as_const(extProps))
                devExts.append({ p.extensionName, p.specVersion });
        }
        qCDebug(QRHI_LOG_INFO, "%d device extensions available", int(devExts.size()));

        QList<const char *> requestedDevExts;
        requestedDevExts.append("VK_KHR_swapchain");

        const bool hasPhysDevProp2 = inst->extensions().contains(QByteArrayLiteral("VK_KHR_get_physical_device_properties2"));

        if (devExts.contains(QByteArrayLiteral("VK_KHR_portability_subset"))) {
            if (hasPhysDevProp2) {
                requestedDevExts.append("VK_KHR_portability_subset");
            } else {
                qWarning("VK_KHR_portability_subset should be enabled on the device "
                         "but the instance does not have VK_KHR_get_physical_device_properties2 enabled. "
                         "Expect problems.");
            }
        }

        caps.vertexAttribDivisor = false;
#ifdef VK_EXT_vertex_attribute_divisor
        if (devExts.contains(VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME)) {
            if (hasPhysDevProp2) {
                requestedDevExts.append(VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME);
                caps.vertexAttribDivisor = true;
            }
        }
#endif

        for (const QByteArray &ext : requestedDeviceExtensions) {
            if (!ext.isEmpty() && !requestedDevExts.contains(ext)) {
                if (devExts.contains(ext)) {
                    requestedDevExts.append(ext.constData());
                } else {
                    qWarning("Device extension %s requested in QRhiVulkanInitParams is not supported",
                             ext.constData());
                }
            }
        }

        QByteArrayList envExtList = qgetenv("QT_VULKAN_DEVICE_EXTENSIONS").split(';');
        for (const QByteArray &ext : envExtList) {
            if (!ext.isEmpty() && !requestedDevExts.contains(ext)) {
                if (devExts.contains(ext)) {
                    requestedDevExts.append(ext.constData());
                } else {
                    qWarning("Device extension %s requested in QT_VULKAN_DEVICE_EXTENSIONS is not supported",
                             ext.constData());
                }
            }
        }

        if (QRHI_LOG_INFO().isEnabled(QtDebugMsg)) {
            qCDebug(QRHI_LOG_INFO, "Enabling device extensions:");
            for (const char *ext : requestedDevExts)
                qCDebug(QRHI_LOG_INFO, "  %s", ext);
        }

        VkDeviceCreateInfo devInfo = {};
        devInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        devInfo.queueCreateInfoCount = 1;
        devInfo.pQueueCreateInfos = &queueInfo;
        devInfo.enabledLayerCount = uint32_t(devLayers.size());
        devInfo.ppEnabledLayerNames = devLayers.constData();
        devInfo.enabledExtensionCount = uint32_t(requestedDevExts.size());
        devInfo.ppEnabledExtensionNames = requestedDevExts.constData();

        // Enable all features that are reported as supported, except
        // robustness because that potentially affects performance.
        //
        // Enabling all features mainly serves third-party renderers that may
        // use the VkDevice created here. For the record, the backend here
        // optionally relies on the following features, meaning just for our
        // (QRhi/Quick/Quick 3D) purposes it would be sufficient to
        // enable-if-supported only the following:
        //
        // wideLines, largePoints, fillModeNonSolid,
        // tessellationShader, geometryShader
        // textureCompressionETC2, textureCompressionASTC_LDR, textureCompressionBC

#ifdef VK_VERSION_1_2
        if (caps.apiVersion >= QVersionNumber(1, 2)) {
            physDevFeaturesChainable.features.robustBufferAccess = VK_FALSE;
#ifdef VK_VERSION_1_3
            physDevFeatures13.robustImageAccess = VK_FALSE;
#endif
            devInfo.pNext = &physDevFeaturesChainable;
        } else
#endif // VK_VERSION_1_2
        {
            physDevFeatures.robustBufferAccess = VK_FALSE;
            devInfo.pEnabledFeatures = &physDevFeatures;
        }

        VkResult err = f->vkCreateDevice(physDev, &devInfo, nullptr, &dev);
        if (err != VK_SUCCESS) {
            qWarning("Failed to create device: %d", err);
            return false;
        }
    } else {
        qCDebug(QRHI_LOG_INFO, "Using imported device %p", dev);
    }

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
                inst->getInstanceProcAddr("vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    vkGetPhysicalDeviceSurfaceFormatsKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
                inst->getInstanceProcAddr("vkGetPhysicalDeviceSurfaceFormatsKHR"));
    vkGetPhysicalDeviceSurfacePresentModesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
                inst->getInstanceProcAddr("vkGetPhysicalDeviceSurfacePresentModesKHR"));
    if (!vkGetPhysicalDeviceSurfaceCapabilitiesKHR
            || !vkGetPhysicalDeviceSurfaceFormatsKHR
            || !vkGetPhysicalDeviceSurfacePresentModesKHR)
    {
        qWarning("Physical device surface queries not available");
        return false;
    }

    df = inst->deviceFunctions(dev);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = gfxQueueFamilyIdx;
    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i) {
        VkResult err = df->vkCreateCommandPool(dev, &poolInfo, nullptr, &cmdPool[i]);
        if (err != VK_SUCCESS) {
            qWarning("Failed to create command pool: %d", err);
            return false;
        }
    }

    qCDebug(QRHI_LOG_INFO, "Using queue family index %u and queue index %u",
            gfxQueueFamilyIdx, gfxQueueIdx);

    df->vkGetDeviceQueue(dev, gfxQueueFamilyIdx, gfxQueueIdx, &gfxQueue);

    if (queueFamilyProps.isEmpty())
        queryQueueFamilyProps();

    caps.compute = (queueFamilyProps[gfxQueueFamilyIdx].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
    timestampValidBits = queueFamilyProps[gfxQueueFamilyIdx].timestampValidBits;

    ubufAlign = physDevProperties.limits.minUniformBufferOffsetAlignment;
    // helps little with an optimal offset of 1 (on some drivers) when the spec
    // elsewhere states that the minimum bufferOffset is 4...
    texbufAlign = qMax<VkDeviceSize>(4, physDevProperties.limits.optimalBufferCopyOffsetAlignment);

    caps.wideLines = physDevFeatures.wideLines;

    caps.texture3DSliceAs2D = caps.apiVersion >= QVersionNumber(1, 1);

    caps.tessellation = physDevFeatures.tessellationShader;
    caps.geometryShader = physDevFeatures.geometryShader;

    caps.nonFillPolygonMode = physDevFeatures.fillModeNonSolid;

    if (!importedAllocator) {
        VmaVulkanFunctions funcs = {};
        funcs.vkGetInstanceProcAddr = wrap_vkGetInstanceProcAddr;
        funcs.vkGetDeviceProcAddr = wrap_vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorInfo = {};
        // A QRhi is supposed to be used from one single thread only. Disable
        // the allocator's own mutexes. This gives a performance boost.
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
        allocatorInfo.physicalDevice = physDev;
        allocatorInfo.device = dev;
        allocatorInfo.pVulkanFunctions = &funcs;
        allocatorInfo.instance = inst->vkInstance();
        allocatorInfo.vulkanApiVersion = VK_MAKE_VERSION(caps.apiVersion.majorVersion(),
                                                         caps.apiVersion.minorVersion(),
                                                         caps.apiVersion.microVersion());
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

    VkQueryPoolCreateInfo timestampQueryPoolInfo = {};
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

#ifdef VK_EXT_debug_utils
    if (caps.debugUtils) {
        vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(f->vkGetDeviceProcAddr(dev, "vkSetDebugUtilsObjectNameEXT"));
        vkCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(f->vkGetDeviceProcAddr(dev, "vkCmdBeginDebugUtilsLabelEXT"));
        vkCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(f->vkGetDeviceProcAddr(dev, "vkCmdEndDebugUtilsLabelEXT"));
        vkCmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(f->vkGetDeviceProcAddr(dev, "vkCmdInsertDebugUtilsLabelEXT"));
    }
#endif

    deviceLost = false;

    nativeHandlesStruct.physDev = physDev;
    nativeHandlesStruct.dev = dev;
    nativeHandlesStruct.gfxQueueFamilyIdx = gfxQueueFamilyIdx;
    nativeHandlesStruct.gfxQueueIdx = gfxQueueIdx;
    nativeHandlesStruct.gfxQueue = gfxQueue;
    nativeHandlesStruct.vmemAllocator = allocator;
    nativeHandlesStruct.inst = inst;

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

    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i) {
        if (cmdPool[i]) {
            df->vkDestroyCommandPool(dev, cmdPool[i], nullptr);
            cmdPool[i] = VK_NULL_HANDLE;
        }
        freeSecondaryCbs[i].clear();
        ofr.cbWrapper[i]->cb = VK_NULL_HANDLE;
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
    VkDescriptorPoolCreateInfo descPoolInfo = {};
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

    int lastPoolIdx = descriptorPools.size() - 1;
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
        lastPoolIdx = descriptorPools.size() - 1;
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
    case QRhiTexture::RG8:
        return srgb ? VK_FORMAT_R8G8_SRGB : VK_FORMAT_R8G8_UNORM;
    case QRhiTexture::R16:
        return VK_FORMAT_R16_UNORM;
    case QRhiTexture::RG16:
        return VK_FORMAT_R16G16_UNORM;
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

    case QRhiTexture::RGB10A2:
        // intentionally A2B10G10R10, not A2R10G10B10
        return VK_FORMAT_A2B10G10R10_UNORM_PACK32;

    case QRhiTexture::D16:
        return VK_FORMAT_D16_UNORM;
    case QRhiTexture::D24:
        return VK_FORMAT_X8_D24_UNORM_PACK32;
    case QRhiTexture::D24S8:
        return VK_FORMAT_D24_UNORM_S8_UINT;
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
        Q_UNREACHABLE_RETURN(VK_FORMAT_R8G8B8A8_UNORM);
    }
}

static inline QRhiTexture::Format swapchainReadbackTextureFormat(VkFormat format, QRhiTexture::Flags *flags)
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
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return QRhiTexture::RGBA16F;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return QRhiTexture::RGBA32F;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        return QRhiTexture::RGB10A2;
    default:
        qWarning("VkFormat %d cannot be read back", format);
        break;
    }
    return QRhiTexture::UnknownFormat;
}

static constexpr inline bool isDepthTextureFormat(QRhiTexture::Format format)
{
    switch (format) {
    case QRhiTexture::Format::D16:
    case QRhiTexture::Format::D24:
    case QRhiTexture::Format::D24S8:
    case QRhiTexture::Format::D32F:
        return true;

    default:
        return false;
    }
}

static constexpr inline VkImageAspectFlags aspectMaskForTextureFormat(QRhiTexture::Format format)
{
    return isDepthTextureFormat(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
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
        VkImageCreateInfo imgInfo = {};
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

    VkMemoryAllocateInfo memInfo = {};
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

        VkImageViewCreateInfo imgViewInfo = {};
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

static void fillRenderPassCreateInfo(VkRenderPassCreateInfo *rpInfo,
                                     VkSubpassDescription *subpassDesc,
                                     QVkRenderPassDescriptor *rpD)
{
    memset(subpassDesc, 0, sizeof(VkSubpassDescription));
    subpassDesc->pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc->colorAttachmentCount = uint32_t(rpD->colorRefs.size());
    subpassDesc->pColorAttachments = !rpD->colorRefs.isEmpty() ? rpD->colorRefs.constData() : nullptr;
    subpassDesc->pDepthStencilAttachment = rpD->hasDepthStencil ? &rpD->dsRef : nullptr;
    subpassDesc->pResolveAttachments = !rpD->resolveRefs.isEmpty() ? rpD->resolveRefs.constData() : nullptr;

    memset(rpInfo, 0, sizeof(VkRenderPassCreateInfo));
    rpInfo->sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo->attachmentCount = uint32_t(rpD->attDescs.size());
    rpInfo->pAttachments = rpD->attDescs.constData();
    rpInfo->subpassCount = 1;
    rpInfo->pSubpasses = subpassDesc;
    rpInfo->dependencyCount = uint32_t(rpD->subpassDeps.size());
    rpInfo->pDependencies = !rpD->subpassDeps.isEmpty() ? rpD->subpassDeps.constData() : nullptr;
}

bool QRhiVulkan::createDefaultRenderPass(QVkRenderPassDescriptor *rpD, bool hasDepthStencil, VkSampleCountFlagBits samples, VkFormat colorFormat)
{
    // attachment list layout is color (1), ds (0-1), resolve (0-1)

    VkAttachmentDescription attDesc = {};
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

    rpD->hasDepthStencil = hasDepthStencil;

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

    // Replace the first implicit dep (TOP_OF_PIPE / ALL_COMMANDS) with our own.
    VkSubpassDependency subpassDep = {};
    subpassDep.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDep.dstSubpass = 0;
    subpassDep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDep.srcAccessMask = 0;
    subpassDep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    rpD->subpassDeps.append(subpassDep);
    if (hasDepthStencil) {
        memset(&subpassDep, 0, sizeof(subpassDep));
        subpassDep.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDep.dstSubpass = 0;
        subpassDep.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
            | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        subpassDep.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
            | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        subpassDep.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        subpassDep.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
            | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        rpD->subpassDeps.append(subpassDep);
    }

    VkRenderPassCreateInfo rpInfo;
    VkSubpassDescription subpassDesc;
    fillRenderPassCreateInfo(&rpInfo, &subpassDesc, rpD);

    VkResult err = df->vkCreateRenderPass(dev, &rpInfo, nullptr, &rpD->rp);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create renderpass: %d", err);
        return false;
    }

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

        VkAttachmentDescription attDesc = {};
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

        const VkAttachmentReference ref = { uint32_t(rpD->attDescs.size() - 1), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
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
        VkAttachmentDescription attDesc = {};
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
    rpD->dsRef = { uint32_t(rpD->attDescs.size() - 1), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    for (auto it = firstColorAttachment; it != lastColorAttachment; ++it) {
        if (it->resolveTexture()) {
            QVkTexture *rtexD = QRHI_RES(QVkTexture, it->resolveTexture());
            const VkFormat dstFormat = rtexD->vkformat;
            if (rtexD->samples > VK_SAMPLE_COUNT_1_BIT)
                qWarning("Resolving into a multisample texture is not supported");

            QVkTexture *texD = QRHI_RES(QVkTexture, it->texture());
            QVkRenderBuffer *rbD = QRHI_RES(QVkRenderBuffer, it->renderBuffer());
            const VkFormat srcFormat = texD ? texD->vkformat : rbD->vkformat;
            if (srcFormat != dstFormat) {
                // This is a validation error. But some implementations survive,
                // actually. Warn about it however, because it's an error with
                // some other backends (like D3D) as well.
                qWarning("Multisample resolve between different formats (%d and %d) is not supported.",
                         int(srcFormat), int(dstFormat));
            }

            VkAttachmentDescription attDesc = {};
            attDesc.format = dstFormat;
            attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
            attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // ignored
            attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            rpD->attDescs.append(attDesc);

            const VkAttachmentReference ref = { uint32_t(rpD->attDescs.size() - 1), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
            rpD->resolveRefs.append(ref);
        } else {
            const VkAttachmentReference ref = { VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
            rpD->resolveRefs.append(ref);
        }
    }
    Q_ASSERT(rpD->colorRefs.size() == rpD->resolveRefs.size());

    // rpD->subpassDeps stays empty: don't yet know the correct initial/final
    // access and stage stuff for the implicit deps at this point, so leave it
    // to the resource tracking and activateTextureRenderTarget() to generate
    // barriers.

    VkRenderPassCreateInfo rpInfo;
    VkSubpassDescription subpassDesc;
    fillRenderPassCreateInfo(&rpInfo, &subpassDesc, rpD);

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
    if (swapChainD->m_flags.testFlag(QRhiSwapChain::MinimalBufferCount) || surfaceCaps.maxImageCount == 0) {
        reqBufferCount = qMax<quint32>(2, surfaceCaps.minImageCount);
    } else {
        reqBufferCount = qMax(qMin<quint32>(surfaceCaps.maxImageCount, 3), surfaceCaps.minImageCount);
    }
    VkSurfaceTransformFlagBitsKHR preTransform =
        (surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
        : surfaceCaps.currentTransform;

    // This looks odd but matches how platforms work in practice.
    //
    // On Windows with NVIDIA for example, the only supportedCompositeAlpha
    // value reported is OPAQUE, nothing else. Yet transparency works
    // regardless, as long as the native window is set up correctly, so that's
    // not something we need to handle here.
    //
    // On Linux with Intel and Mesa and running on xcb reports, on one
    // particular system, INHERIT+PRE_MULTIPLIED. Tranparency works, regardless,
    // presumably due to setting INHERIT.
    //
    // On the same setup with Wayland instead of xcb we see
    // OPAQUE+PRE_MULTIPLIED reported. Here transparency won't work unless
    // PRE_MULTIPLIED is set.
    //
    // Therefore our rules are:
    //   - Prefer INHERIT over OPAQUE.
    //   - Then based on the request, try the requested alpha mode, but if
    //     that's not reported as supported, try also the other (PRE/POST,
    //     POST/PRE) as that is better than nothing. This is not different from
    //     some other backends, e.g. D3D11 with DirectComposition there is also
    //     no control over being straight or pre-multiplied. Whereas with
    //     WGL/GLX/EGL we never had that sort of control.

    VkCompositeAlphaFlagBitsKHR compositeAlpha =
        (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
        ? VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
        : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    if (swapChainD->m_flags.testFlag(QRhiSwapChain::SurfaceHasPreMulAlpha)) {
        if (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
            compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
        else if (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
            compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
    } else if (swapChainD->m_flags.testFlag(QRhiSwapChain::SurfaceHasNonPreMulAlpha)) {
        if (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
            compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
        else if (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
            compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
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

    VkSwapchainCreateInfoKHR swapChainInfo = {};
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

    if (actualSwapChainBufferCount != reqBufferCount)
        qCDebug(QRHI_LOG_INFO, "Actual swapchain buffer count is %u", actualSwapChainBufferCount);
    swapChainD->bufferCount = int(actualSwapChainBufferCount);

    QVarLengthArray<VkImage, QVkSwapChain::EXPECTED_MAX_BUFFER_COUNT> swapChainImages(actualSwapChainBufferCount);
    err = vkGetSwapchainImagesKHR(dev, swapChainD->sc, &actualSwapChainBufferCount, swapChainImages.data());
    if (err != VK_SUCCESS) {
        qWarning("Failed to get swapchain images: %d", err);
        return false;
    }

    QVarLengthArray<VkImage, QVkSwapChain::EXPECTED_MAX_BUFFER_COUNT> msaaImages(swapChainD->bufferCount);
    QVarLengthArray<VkImageView, QVkSwapChain::EXPECTED_MAX_BUFFER_COUNT> msaaViews(swapChainD->bufferCount);
    if (swapChainD->samples > VK_SAMPLE_COUNT_1_BIT) {
        if (!createTransientImage(swapChainD->colorFormat,
                                  swapChainD->pixelSize,
                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                  swapChainD->samples,
                                  &swapChainD->msaaImageMem,
                                  msaaImages.data(),
                                  msaaViews.data(),
                                  swapChainD->bufferCount))
        {
            qWarning("Failed to create transient image for MSAA color buffer");
            return false;
        }
    }

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    swapChainD->imageRes.resize(swapChainD->bufferCount);
    for (int i = 0; i < swapChainD->bufferCount; ++i) {
        QVkSwapChain::ImageResources &image(swapChainD->imageRes[i]);
        image.image = swapChainImages[i];
        if (swapChainD->samples > VK_SAMPLE_COUNT_1_BIT) {
            image.msaaImage = msaaImages[i];
            image.msaaImageView = msaaViews[i];
        }

        VkImageViewCreateInfo imgViewInfo = {};
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

    VkSemaphoreCreateInfo semInfo = {};
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

void QRhiVulkan::ensureCommandPoolForNewFrame()
{
    VkCommandPoolResetFlags flags = 0;

    // While not clear what "recycles all of the resources from the command
    // pool back to the system" really means in practice, set it when there was
    // a call to releaseCachedResources() recently.
    if (releaseCachedResourcesCalledBeforeFrameStart)
        flags |= VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT;

    // put all command buffers allocated from this slot's pool to initial state
    df->vkResetCommandPool(dev, cmdPool[currentFrameSlot], flags);
}

double QRhiVulkan::elapsedSecondsFromTimestamp(quint64 timestamp[2], bool *ok)
{
    quint64 mask = 0;
    for (quint64 i = 0; i < timestampValidBits; i += 8)
        mask |= 0xFFULL << i;
    const quint64 ts0 = timestamp[0] & mask;
    const quint64 ts1 = timestamp[1] & mask;
    const float nsecsPerTick = physDevProperties.limits.timestampPeriod;
    if (!qFuzzyIsNull(nsecsPerTick)) {
        const float elapsedMs = float(ts1 - ts0) * nsecsPerTick / 1000000.0f;
        const double elapsedSec = elapsedMs / 1000.0;
        *ok = true;
        return elapsedSec;
    }
    *ok = false;
    return 0;
}

QRhi::FrameOpResult QRhiVulkan::beginFrame(QRhiSwapChain *swapChain, QRhi::BeginFrameFlags)
{
    QVkSwapChain *swapChainD = QRHI_RES(QVkSwapChain, swapChain);
    const int frameResIndex = swapChainD->bufferCount > 1 ? swapChainD->currentFrameSlot : 0;
    QVkSwapChain::FrameResources &frame(swapChainD->frameRes[frameResIndex]);

    inst->handle()->beginFrame(swapChainD->window);

    if (!frame.imageAcquired) {
        // Wait if we are too far ahead, i.e. the thread gets throttled based on the presentation rate
        // (note that we are using FIFO mode -> vsync)
        if (frame.imageFenceWaitable) {
            df->vkWaitForFences(dev, 1, &frame.imageFence, VK_TRUE, UINT64_MAX);
            df->vkResetFences(dev, 1, &frame.imageFence);
            frame.imageFenceWaitable = false;
        }

        // move on to next swapchain image
        uint32_t imageIndex = 0;
        VkResult err = vkAcquireNextImageKHR(dev, swapChainD->sc, UINT64_MAX,
                                             frame.imageSem, frame.imageFence, &imageIndex);
        if (err == VK_SUCCESS || err == VK_SUBOPTIMAL_KHR) {
            swapChainD->currentImageIndex = imageIndex;
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

    currentFrameSlot = int(swapChainD->currentFrameSlot);
    currentSwapChain = swapChainD;
    if (swapChainD->ds)
        swapChainD->ds->lastActiveFrameSlot = currentFrameSlot;

    // reset the command pool
    ensureCommandPoolForNewFrame();

    // start recording to this frame's command buffer
    QRhi::FrameOpResult cbres = startPrimaryCommandBuffer(&frame.cmdBuf);
    if (cbres != QRhi::FrameOpSuccess)
        return cbres;

    swapChainD->cbWrapper.cb = frame.cmdBuf;

    QVkSwapChain::ImageResources &image(swapChainD->imageRes[swapChainD->currentImageIndex]);
    swapChainD->rtWrapper.d.fb = image.fb;

    prepareNewFrame(&swapChainD->cbWrapper);

    // Read the timestamps for the previous frame for this slot.
    if (frame.timestampQueryIndex >= 0) {
        quint64 timestamp[2] = { 0, 0 };
        VkResult err = df->vkGetQueryPoolResults(dev, timestampQueryPool, uint32_t(frame.timestampQueryIndex), 2,
                                                 2 * sizeof(quint64), timestamp, sizeof(quint64),
                                                 VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
        timestampQueryPoolMap.clearBit(frame.timestampQueryIndex / 2);
        frame.timestampQueryIndex = -1;
        if (err == VK_SUCCESS) {
            bool ok = false;
            const double elapsedSec = elapsedSecondsFromTimestamp(timestamp, &ok);
            if (ok)
                swapChainD->cbWrapper.lastGpuTime = elapsedSec;
        } else {
            qWarning("Failed to query timestamp: %d", err);
        }
    }

    // No timestamps if the client did not opt in, or when not having at least 2 frames in flight.
    if (rhiFlags.testFlag(QRhi::EnableTimestamps) && swapChainD->bufferCount > 1) {
        int timestampQueryIdx = -1;
        for (int i = 0; i < timestampQueryPoolMap.size(); ++i) {
            if (!timestampQueryPoolMap.testBit(i)) {
                timestampQueryPoolMap.setBit(i);
                timestampQueryIdx = i * 2;
                break;
            }
        }
        if (timestampQueryIdx >= 0) {
            df->vkCmdResetQueryPool(frame.cmdBuf, timestampQueryPool, uint32_t(timestampQueryIdx), 2);
            // record timestamp at the start of the command buffer
            df->vkCmdWriteTimestamp(frame.cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                    timestampQueryPool, uint32_t(timestampQueryIdx));
            frame.timestampQueryIndex = timestampQueryIdx;
        }
    }

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiVulkan::endFrame(QRhiSwapChain *swapChain, QRhi::EndFrameFlags flags)
{
    QVkSwapChain *swapChainD = QRHI_RES(QVkSwapChain, swapChain);
    Q_ASSERT(currentSwapChain == swapChainD);

    auto cleanup = qScopeGuard([this, swapChainD] {
        inst->handle()->endFrame(swapChainD->window);
    });

    recordPrimaryCommandBuffer(&swapChainD->cbWrapper);

    int frameResIndex = swapChainD->bufferCount > 1 ? swapChainD->currentFrameSlot : 0;
    QVkSwapChain::FrameResources &frame(swapChainD->frameRes[frameResIndex]);
    QVkSwapChain::ImageResources &image(swapChainD->imageRes[swapChainD->currentImageIndex]);

    if (image.lastUse != QVkSwapChain::ImageResources::ScImageUseRender) {
        VkImageMemoryBarrier presTrans = {};
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

    if (needsPresent) {
        // add the Present to the queue
        VkPresentInfoKHR presInfo = {};
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
    // begin(Offscreen)Frame() blocks anyway waiting for its current frame
    // slot's previous commands to complete so this here is safe regardless.

    executeDeferredReleases();

    QRHI_RES(QVkCommandBuffer, cb)->resetState();

    finishActiveReadbacks(); // last, in case the readback-completed callback issues rhi calls

    releaseCachedResourcesCalledBeforeFrameStart = false;
}

QRhi::FrameOpResult QRhiVulkan::startPrimaryCommandBuffer(VkCommandBuffer *cb)
{
    if (!*cb) {
        VkCommandBufferAllocateInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufInfo.commandPool = cmdPool[currentFrameSlot];
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
    }

    VkCommandBufferBeginInfo cmdBufBeginInfo = {};
    cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResult err = df->vkBeginCommandBuffer(*cb, &cmdBufBeginInfo);
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

    VkSubmitInfo submitInfo = {};
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
    for (QVkSwapChain *sc : std::as_const(swapchains)) {
        const int frameResIndex = sc->bufferCount > 1 ? frameSlot : 0;
        QVkSwapChain::FrameResources &frame(sc->frameRes[frameResIndex]);
        if (frame.cmdFenceWaitable) {
            df->vkWaitForFences(dev, 1, &frame.cmdFence, VK_TRUE, UINT64_MAX);
            df->vkResetFences(dev, 1, &frame.cmdFence);
            frame.cmdFenceWaitable = false;
        }
    }
}

QRhi::FrameOpResult QRhiVulkan::beginOffscreenFrame(QRhiCommandBuffer **cb, QRhi::BeginFrameFlags)
{
    // Switch to the next slot manually. Swapchains do not know about this
    // which is good. So for example an onscreen, onscreen, offscreen,
    // onscreen, onscreen, onscreen sequence of frames leads to 0, 1, 0, 0, 1,
    // 0. (no strict alternation anymore) But this is not different from what
    // happens when multiple swapchains are involved. Offscreen frames are
    // synchronous anyway in the sense that they wait for execution to complete
    // in endOffscreenFrame, so no resources used in that frame are busy
    // anymore in the next frame.

    currentFrameSlot = (currentFrameSlot + 1) % QVK_FRAMES_IN_FLIGHT;

    waitCommandCompletion(currentFrameSlot);

    ensureCommandPoolForNewFrame();

    QVkCommandBuffer *cbWrapper = ofr.cbWrapper[currentFrameSlot];
    QRhi::FrameOpResult cbres = startPrimaryCommandBuffer(&cbWrapper->cb);
    if (cbres != QRhi::FrameOpSuccess)
        return cbres;

    prepareNewFrame(cbWrapper);
    ofr.active = true;

    if (rhiFlags.testFlag(QRhi::EnableTimestamps)) {
        int timestampQueryIdx = -1;
        for (int i = 0; i < timestampQueryPoolMap.size(); ++i) {
            if (!timestampQueryPoolMap.testBit(i)) {
                timestampQueryPoolMap.setBit(i);
                timestampQueryIdx = i * 2;
                break;
            }
        }
        if (timestampQueryIdx >= 0) {
            df->vkCmdResetQueryPool(cbWrapper->cb, timestampQueryPool, uint32_t(timestampQueryIdx), 2);
            // record timestamp at the start of the command buffer
            df->vkCmdWriteTimestamp(cbWrapper->cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                    timestampQueryPool, uint32_t(timestampQueryIdx));
            ofr.timestampQueryIndex = timestampQueryIdx;
        }
    }

    *cb = cbWrapper;
    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiVulkan::endOffscreenFrame(QRhi::EndFrameFlags flags)
{
    Q_UNUSED(flags);
    Q_ASSERT(ofr.active);
    ofr.active = false;

    QVkCommandBuffer *cbWrapper(ofr.cbWrapper[currentFrameSlot]);
    recordPrimaryCommandBuffer(cbWrapper);

    // record another timestamp, when enabled
    if (ofr.timestampQueryIndex >= 0) {
        df->vkCmdWriteTimestamp(cbWrapper->cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                timestampQueryPool, uint32_t(ofr.timestampQueryIndex + 1));
    }

    if (!ofr.cmdFence) {
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkResult err = df->vkCreateFence(dev, &fenceInfo, nullptr, &ofr.cmdFence);
        if (err != VK_SUCCESS) {
            qWarning("Failed to create command buffer fence: %d", err);
            return QRhi::FrameOpError;
        }
    }

    QRhi::FrameOpResult submitres = endAndSubmitPrimaryCommandBuffer(cbWrapper->cb, ofr.cmdFence, nullptr, nullptr);
    if (submitres != QRhi::FrameOpSuccess)
        return submitres;

    // wait for completion
    df->vkWaitForFences(dev, 1, &ofr.cmdFence, VK_TRUE, UINT64_MAX);
    df->vkResetFences(dev, 1, &ofr.cmdFence);

    // Here we know that executing the host-side reads for this (or any
    // previous) frame is safe since we waited for completion above.
    finishActiveReadbacks(true);

    // Read the timestamps, if we wrote them.
    if (ofr.timestampQueryIndex >= 0) {
        quint64 timestamp[2] = { 0, 0 };
        VkResult err = df->vkGetQueryPoolResults(dev, timestampQueryPool, uint32_t(ofr.timestampQueryIndex), 2,
                                                 2 * sizeof(quint64), timestamp, sizeof(quint64),
                                                 VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
        timestampQueryPoolMap.clearBit(ofr.timestampQueryIndex / 2);
        ofr.timestampQueryIndex = -1;
        if (err == VK_SUCCESS) {
            bool ok = false;
            const double elapsedSec = elapsedSecondsFromTimestamp(timestamp, &ok);
            if (ok)
                cbWrapper->lastGpuTime = elapsedSec;
        } else {
            qWarning("Failed to query timestamp: %d", err);
        }
    }

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
            QVkCommandBuffer *cbWrapper(ofr.cbWrapper[currentFrameSlot]);
            Q_ASSERT(cbWrapper->recordingPass == QVkCommandBuffer::NoPass);
            recordPrimaryCommandBuffer(cbWrapper);
            cbWrapper->resetCommands();
            cb = cbWrapper->cb;
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
        // The current frame slot's command pool needs to be reset.
        ensureCommandPoolForNewFrame();
        // Allocate and begin recording on a new command buffer.
        if (ofr.active) {
            startPrimaryCommandBuffer(&ofr.cbWrapper[currentFrameSlot]->cb);
        } else {
            QVkSwapChain::FrameResources &frame(swapChainD->frameRes[swapChainD->currentFrameSlot]);
            startPrimaryCommandBuffer(&frame.cmdBuf);
            swapChainD->cbWrapper.cb = frame.cmdBuf;
        }
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
    if (!QRhiRenderTargetAttachmentTracker::isUpToDate<QVkTexture, QVkRenderBuffer>(rtD->description(), rtD->d.currentResIdList))
        rtD->create();

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
    if (rtD->m_desc.depthStencilBuffer()) {
        QVkRenderBuffer *rbD = QRHI_RES(QVkRenderBuffer, rtD->m_desc.depthStencilBuffer());
        Q_ASSERT(rbD->m_type == QRhiRenderBuffer::DepthStencil);
        // We specify no explicit VkSubpassDependency for an offscreen render
        // target, meaning we need an explicit barrier for the depth-stencil
        // buffer to avoid a write-after-write hazard (as the implicit one is
        // not sufficient). Textures are taken care of by the resource tracking
        // but that excludes the (content-wise) throwaway depth-stencil buffer.
        depthStencilExplicitBarrier(cbD, rbD);
        rbD->lastActiveFrameSlot = currentFrameSlot;
    }
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

    if (!freeSecondaryCbs[currentFrameSlot].isEmpty()) {
        secondaryCb = freeSecondaryCbs[currentFrameSlot].last();
        freeSecondaryCbs[currentFrameSlot].removeLast();
    } else {
        VkCommandBufferAllocateInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufInfo.commandPool = cmdPool[currentFrameSlot];
        cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        cmdBufInfo.commandBufferCount = 1;

        VkResult err = df->vkAllocateCommandBuffers(dev, &cmdBufInfo, &secondaryCb);
        if (err != VK_SUCCESS) {
            qWarning("Failed to create secondary command buffer: %d", err);
            return VK_NULL_HANDLE;
        }
    }

    VkCommandBufferBeginInfo cmdBufBeginInfo = {};
    cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufBeginInfo.flags = rtD ? VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : 0;
    VkCommandBufferInheritanceInfo cmdBufInheritInfo = {};
    cmdBufInheritInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    cmdBufInheritInfo.subpass = 0;
    if (rtD) {
        cmdBufInheritInfo.renderPass = rtD->rp->rp;
        cmdBufInheritInfo.framebuffer = rtD->fb;
    }
    cmdBufBeginInfo.pInheritanceInfo = &cmdBufInheritInfo;

    VkResult err = df->vkBeginCommandBuffer(secondaryCb, &cmdBufBeginInfo);
    if (err != VK_SUCCESS) {
        qWarning("Failed to begin secondary command buffer: %d", err);
        return VK_NULL_HANDLE;
    }

    return secondaryCb;
}

void QRhiVulkan::endAndEnqueueSecondaryCommandBuffer(VkCommandBuffer cb, QVkCommandBuffer *cbD)
{
    VkResult err = df->vkEndCommandBuffer(cb);
    if (err != VK_SUCCESS)
        qWarning("Failed to end secondary command buffer: %d", err);

    QVkCommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QVkCommandBuffer::Command::ExecuteSecondary;
    cmd.args.executeSecondary.cb = cb;

    QRhiVulkan::DeferredReleaseEntry e;
    e.type = QRhiVulkan::DeferredReleaseEntry::SecondaryCommandBuffer;
    e.lastActiveFrameSlot = currentFrameSlot;
    e.secondaryCommandBuffer.cb = cb;
    releaseQueue.append(e);
}

void QRhiVulkan::beginPass(QRhiCommandBuffer *cb,
                           QRhiRenderTarget *rt,
                           const QColor &colorClearValue,
                           const QRhiDepthStencilClearValue &depthStencilClearValue,
                           QRhiResourceUpdateBatch *resourceUpdates,
                           QRhiCommandBuffer::BeginPassFlags flags)
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
    case QRhiResource::SwapChainRenderTarget:
        rtD = &QRHI_RES(QVkSwapChainRenderTarget, rt)->d;
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
    cbD->passUsesSecondaryCb = flags.testFlag(QRhiCommandBuffer::ExternalContent);
    cbD->currentTarget = rt;

    // No copy operations or image layout transitions allowed after this point
    // (up until endPass) as we are going to begin the renderpass.

    VkRenderPassBeginInfo rpBeginInfo = {};
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
    rpBeginInfo.clearValueCount = uint32_t(cvs.size());

    QVkCommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QVkCommandBuffer::Command::BeginRenderPass;
    cmd.args.beginRenderPass.desc = rpBeginInfo;
    cmd.args.beginRenderPass.clearValueIndex = cbD->pools.clearValue.size();
    cmd.args.beginRenderPass.useSecondaryCb = cbD->passUsesSecondaryCb;
    cbD->pools.clearValue.append(cvs.constData(), cvs.size());

    if (cbD->passUsesSecondaryCb)
        cbD->activeSecondaryCbStack.append(startSecondaryCommandBuffer(rtD));

    cbD->resetCachedState();
}

void QRhiVulkan::endPass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::RenderPass);

    if (cbD->passUsesSecondaryCb) {
        VkCommandBuffer secondaryCb = cbD->activeSecondaryCbStack.last();
        cbD->activeSecondaryCbStack.removeLast();
        endAndEnqueueSecondaryCommandBuffer(secondaryCb, cbD);
    }

    QVkCommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QVkCommandBuffer::Command::EndRenderPass;

    cbD->recordingPass = QVkCommandBuffer::NoPass;
    cbD->currentTarget = nullptr;

    if (resourceUpdates)
        enqueueResourceUpdates(cbD, resourceUpdates);
}

void QRhiVulkan::beginComputePass(QRhiCommandBuffer *cb,
                                  QRhiResourceUpdateBatch *resourceUpdates,
                                  QRhiCommandBuffer::BeginPassFlags flags)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::NoPass);

    if (resourceUpdates)
        enqueueResourceUpdates(cbD, resourceUpdates);

    enqueueTransitionPassResources(cbD);

    cbD->recordingPass = QVkCommandBuffer::ComputePass;
    cbD->passUsesSecondaryCb = flags.testFlag(QRhiCommandBuffer::ExternalContent);

    cbD->computePassState.reset();

    if (cbD->passUsesSecondaryCb)
        cbD->activeSecondaryCbStack.append(startSecondaryCommandBuffer());

    cbD->resetCachedState();
}

void QRhiVulkan::endComputePass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::ComputePass);

    if (cbD->passUsesSecondaryCb) {
        VkCommandBuffer secondaryCb = cbD->activeSecondaryCbStack.last();
        cbD->activeSecondaryCbStack.removeLast();
        endAndEnqueueSecondaryCommandBuffer(secondaryCb, cbD);
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
        if (cbD->passUsesSecondaryCb) {
            df->vkCmdBindPipeline(cbD->activeSecondaryCbStack.last(), VK_PIPELINE_BIND_POINT_COMPUTE, psD->pipeline);
        } else {
            QVkCommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QVkCommandBuffer::Command::BindPipeline;
            cmd.args.bindPipeline.bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
            cmd.args.bindPipeline.pipeline = psD->pipeline;
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
        const int bindingCount = srbD->m_bindings.size();
        for (int i = 0; i < bindingCount; ++i) {
            const QRhiShaderResourceBinding::Data *b = shaderResourceBindingData(srbD->m_bindings.at(i));
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
                    VkImageMemoryBarrier barrier = {};
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
                    VkBufferMemoryBarrier barrier = {};
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

    if (cbD->passUsesSecondaryCb) {
        VkCommandBuffer secondaryCb = cbD->activeSecondaryCbStack.last();
        if (!imageBarriers.isEmpty()) {
            df->vkCmdPipelineBarrier(secondaryCb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     0, 0, nullptr,
                                     0, nullptr,
                                     imageBarriers.size(), imageBarriers.constData());
        }
        if (!bufferBarriers.isEmpty()) {
            df->vkCmdPipelineBarrier(secondaryCb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     0, 0, nullptr,
                                     bufferBarriers.size(), bufferBarriers.constData(),
                                     0, nullptr);
        }
        df->vkCmdDispatch(secondaryCb, uint32_t(x), uint32_t(y), uint32_t(z));
    } else {
        if (!imageBarriers.isEmpty()) {
            QVkCommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QVkCommandBuffer::Command::ImageBarrier;
            cmd.args.imageBarrier.srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            cmd.args.imageBarrier.dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            cmd.args.imageBarrier.count = imageBarriers.size();
            cmd.args.imageBarrier.index = cbD->pools.imageBarrier.size();
            cbD->pools.imageBarrier.append(imageBarriers.constData(), imageBarriers.size());
        }
        if (!bufferBarriers.isEmpty()) {
            QVkCommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QVkCommandBuffer::Command::BufferBarrier;
            cmd.args.bufferBarrier.srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            cmd.args.bufferBarrier.dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            cmd.args.bufferBarrier.count = bufferBarriers.size();
            cmd.args.bufferBarrier.index = cbD->pools.bufferBarrier.size();
            cbD->pools.bufferBarrier.append(bufferBarriers.constData(), bufferBarriers.size());
        }
        QVkCommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QVkCommandBuffer::Command::Dispatch;
        cmd.args.dispatch.x = x;
        cmd.args.dispatch.y = y;
        cmd.args.dispatch.z = z;
    }
}

VkShaderModule QRhiVulkan::createShader(const QByteArray &spirv)
{
    VkShaderModuleCreateInfo shaderInfo = {};
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

bool QRhiVulkan::ensurePipelineCache(const void *initialData, size_t initialDataSize)
{
    if (pipelineCache)
        return true;

    VkPipelineCacheCreateInfo pipelineCacheInfo = {};
    pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipelineCacheInfo.initialDataSize = initialDataSize;
    pipelineCacheInfo.pInitialData = initialData;
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
        for (int i = 0, ie = srbD->sortedBindings.size(); i != ie; ++i) {
            const QRhiShaderResourceBinding::Data *b = shaderResourceBindingData(srbD->sortedBindings.at(i));
            QVkShaderResourceBindings::BoundResourceData &bd(srbD->boundResourceData[frameSlot][i]);

            VkWriteDescriptorSet writeInfo = {};
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
                bufInfo.offset = b->u.ubuf.offset;
                bufInfo.range = b->u.ubuf.maybeSize ? b->u.ubuf.maybeSize : bufD->m_size;
                // be nice and assert when we know the vulkan device would die a horrible death due to non-aligned reads
                Q_ASSERT(aligned(bufInfo.offset, ubufAlign) == bufInfo.offset);
                bufferInfoIndex = bufferInfos.size();
                bufferInfos.append(bufInfo);
            }
                break;
            case QRhiShaderResourceBinding::SampledTexture:
            {
                const QRhiShaderResourceBinding::Data::TextureAndOrSamplerData *data = &b->u.stex;
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
                imageInfoIndex = imageInfos.size();
                imageInfos.append(imageInfo);
            }
                break;
            case QRhiShaderResourceBinding::Texture:
            {
                const QRhiShaderResourceBinding::Data::TextureAndOrSamplerData *data = &b->u.stex;
                writeInfo.descriptorCount = data->count; // arrays of (separate) images are supported
                writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                ArrayOfImageDesc imageInfo(data->count);
                for (int elem = 0; elem < data->count; ++elem) {
                    QVkTexture *texD = QRHI_RES(QVkTexture, data->texSamplers[elem].tex);
                    bd.stex.d[elem].texId = texD->m_id;
                    bd.stex.d[elem].texGeneration = texD->generation;
                    bd.stex.d[elem].samplerId = 0;
                    bd.stex.d[elem].samplerGeneration = 0;
                    imageInfo[elem].sampler = VK_NULL_HANDLE;
                    imageInfo[elem].imageView = texD->imageView;
                    imageInfo[elem].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }
                bd.stex.count = data->count;
                imageInfoIndex = imageInfos.size();
                imageInfos.append(imageInfo);
            }
                break;
            case QRhiShaderResourceBinding::Sampler:
            {
                QVkSampler *samplerD = QRHI_RES(QVkSampler, b->u.stex.texSamplers[0].sampler);
                writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                bd.stex.d[0].texId = 0;
                bd.stex.d[0].texGeneration = 0;
                bd.stex.d[0].samplerId = samplerD->m_id;
                bd.stex.d[0].samplerGeneration = samplerD->generation;
                ArrayOfImageDesc imageInfo(1);
                imageInfo[0].sampler = samplerD->sampler;
                imageInfo[0].imageView = VK_NULL_HANDLE;
                imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageInfoIndex = imageInfos.size();
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
                    imageInfoIndex = imageInfos.size();
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
                bufInfo.offset = b->u.ubuf.offset;
                bufInfo.range = b->u.ubuf.maybeSize ? b->u.ubuf.maybeSize : bufD->m_size;
                bufferInfoIndex = bufferInfos.size();
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

    for (int i = 0, writeInfoCount = writeInfos.size(); i < writeInfoCount; ++i) {
        const int bufferInfoIndex = infoIndices[i].first;
        const int imageInfoIndex = infoIndices[i].second;
        if (bufferInfoIndex >= 0)
            writeInfos[i].pBufferInfo = &bufferInfos[bufferInfoIndex];
        else if (imageInfoIndex >= 0)
            writeInfos[i].pImageInfo = imageInfos[imageInfoIndex].constData();
    }

    df->vkUpdateDescriptorSets(dev, uint32_t(writeInfos.size()), writeInfos.constData(), 0, nullptr);
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

    VkBufferMemoryBarrier bufMemBarrier = {};
    bufMemBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufMemBarrier.srcAccessMask = s.access;
    bufMemBarrier.dstAccessMask = access;
    bufMemBarrier.buffer = bufD->buffers[slot];
    bufMemBarrier.size = VK_WHOLE_SIZE;

    QVkCommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QVkCommandBuffer::Command::BufferBarrier;
    cmd.args.bufferBarrier.srcStageMask = s.stage;
    cmd.args.bufferBarrier.dstStageMask = stage;
    cmd.args.bufferBarrier.count = 1;
    cmd.args.bufferBarrier.index = cbD->pools.bufferBarrier.size();
    cbD->pools.bufferBarrier.append(bufMemBarrier);

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

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.subresourceRange.aspectMask = aspectMaskForTextureFormat(texD->m_format);
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

    QVkCommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QVkCommandBuffer::Command::ImageBarrier;
    cmd.args.imageBarrier.srcStageMask = srcStage;
    cmd.args.imageBarrier.dstStageMask = stage;
    cmd.args.imageBarrier.count = 1;
    cmd.args.imageBarrier.index = cbD->pools.imageBarrier.size();
    cbD->pools.imageBarrier.append(barrier);

    s.layout = layout;
    s.access = access;
    s.stage = stage;
}

void QRhiVulkan::depthStencilExplicitBarrier(QVkCommandBuffer *cbD, QVkRenderBuffer *rbD)
{
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::NoPass);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
        | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barrier.image = rbD->image;

    const VkPipelineStageFlags stages = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
        | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

    QVkCommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QVkCommandBuffer::Command::ImageBarrier;
    cmd.args.imageBarrier.srcStageMask = stages;
    cmd.args.imageBarrier.dstStageMask = stages;
    cmd.args.imageBarrier.count = 1;
    cmd.args.imageBarrier.index = cbD->pools.imageBarrier.size();
    cbD->pools.imageBarrier.append(barrier);
}

void QRhiVulkan::subresourceBarrier(QVkCommandBuffer *cbD, VkImage image,
                                    VkImageLayout oldLayout, VkImageLayout newLayout,
                                    VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                                    VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                                    int startLayer, int layerCount,
                                    int startLevel, int levelCount)
{
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::NoPass);
    VkImageMemoryBarrier barrier = {};
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

    QVkCommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QVkCommandBuffer::Command::ImageBarrier;
    cmd.args.imageBarrier.srcStageMask = srcStage;
    cmd.args.imageBarrier.dstStageMask = dstStage;
    cmd.args.imageBarrier.count = 1;
    cmd.args.imageBarrier.index = cbD->pools.imageBarrier.size();
    cbD->pools.imageBarrier.append(barrier);
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
    const bool is3D = texD->m_flags.testFlag(QRhiTexture::ThreeDimensional);
    const bool is1D = texD->m_flags.testFlag(QRhiTexture::OneDimensional);

    VkBufferImageCopy copyInfo = {};
    copyInfo.bufferOffset = *curOfs;
    copyInfo.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyInfo.imageSubresource.mipLevel = uint32_t(level);
    copyInfo.imageSubresource.baseArrayLayer = is3D ? 0 : uint32_t(layer);
    copyInfo.imageSubresource.layerCount = 1;
    copyInfo.imageExtent.depth = 1;
    if (is3D)
        copyInfo.imageOffset.z = uint32_t(layer);
    if (is1D)
        copyInfo.imageOffset.y = uint32_t(layer);

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
        if (subresDesc.dataStride()) {
            quint32 bytesPerPixel = 0;
            textureFormatInfo(texD->m_format, size, nullptr, nullptr, &bytesPerPixel);
            if (bytesPerPixel)
                copyInfo.bufferRowLength = subresDesc.dataStride() / bytesPerPixel;
        }
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

    if (src) {
        memcpy(reinterpret_cast<char *>(mp) + *curOfs, src, size_t(copySizeBytes));
        *curOfs += aligned(VkDeviceSize(imageSizeBytes), texbufAlign);
    }
}

void QRhiVulkan::printExtraErrorInfo(VkResult err)
{
    if (err == VK_ERROR_OUT_OF_DEVICE_MEMORY)
        qWarning() << "Out of device memory, current allocator statistics are" << statistics();
}

void QRhiVulkan::enqueueResourceUpdates(QVkCommandBuffer *cbD, QRhiResourceUpdateBatch *resourceUpdates)
{
    QRhiResourceUpdateBatchPrivate *ud = QRhiResourceUpdateBatchPrivate::get(resourceUpdates);

    for (int opIdx = 0; opIdx < ud->activeBufferOpCount; ++opIdx) {
        const QRhiResourceUpdateBatchPrivate::BufferOp &u(ud->bufferOps[opIdx]);
        if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::DynamicUpdate) {
            QVkBuffer *bufD = QRHI_RES(QVkBuffer, u.buf);
            Q_ASSERT(bufD->m_type == QRhiBuffer::Dynamic);
            for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i) {
                if (u.offset == 0 && u.data.size() == bufD->m_size)
                    bufD->pendingDynamicUpdates[i].clear();
                bufD->pendingDynamicUpdates[i].append({ u.offset, u.data });
            }
        } else if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::StaticUpload) {
            QVkBuffer *bufD = QRHI_RES(QVkBuffer, u.buf);
            Q_ASSERT(bufD->m_type != QRhiBuffer::Dynamic);
            Q_ASSERT(u.offset + u.data.size() <= bufD->m_size);

            if (!bufD->stagingBuffers[currentFrameSlot]) {
                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                // must cover the entire buffer - this way multiple, partial updates per frame
                // are supported even when the staging buffer is reused (Static)
                bufferInfo.size = bufD->m_size;
                bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

                VmaAllocationCreateInfo allocInfo = {};
                allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

                VmaAllocation allocation;
                VkResult err = vmaCreateBuffer(toVmaAllocator(allocator), &bufferInfo, &allocInfo,
                                               &bufD->stagingBuffers[currentFrameSlot], &allocation, nullptr);
                if (err == VK_SUCCESS) {
                    bufD->stagingAllocations[currentFrameSlot] = allocation;
                } else {
                    qWarning("Failed to create staging buffer of size %u: %d", bufD->m_size, err);
                    printExtraErrorInfo(err);
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
            memcpy(static_cast<uchar *>(p) + u.offset, u.data.constData(), u.data.size());
            vmaFlushAllocation(toVmaAllocator(allocator), a, u.offset, u.data.size());
            vmaUnmapMemory(toVmaAllocator(allocator), a);

            trackedBufferBarrier(cbD, bufD, 0,
                                 VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            VkBufferCopy copyInfo = {};
            copyInfo.srcOffset = u.offset;
            copyInfo.dstOffset = u.offset;
            copyInfo.size = u.data.size();

            QVkCommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QVkCommandBuffer::Command::CopyBuffer;
            cmd.args.copyBuffer.src = bufD->stagingBuffers[currentFrameSlot];
            cmd.args.copyBuffer.dst = bufD->buffers[0];
            cmd.args.copyBuffer.desc = copyInfo;

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
                    memcpy(u.result->data.data(), reinterpret_cast<char *>(p) + u.offset, u.readSize);
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

                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = readback.byteSize;
                bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

                VmaAllocationCreateInfo allocInfo = {};
                allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;

                VmaAllocation allocation;
                VkResult err = vmaCreateBuffer(toVmaAllocator(allocator), &bufferInfo, &allocInfo, &readback.stagingBuf, &allocation, nullptr);
                if (err == VK_SUCCESS) {
                    readback.stagingAlloc = allocation;
                } else {
                    qWarning("Failed to create readback buffer of size %u: %d", readback.byteSize, err);
                    printExtraErrorInfo(err);
                    continue;
                }

                trackedBufferBarrier(cbD, bufD, 0, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

                VkBufferCopy copyInfo = {};
                copyInfo.srcOffset = u.offset;
                copyInfo.size = u.readSize;

                QVkCommandBuffer::Command &cmd(cbD->commands.get());
                cmd.cmd = QVkCommandBuffer::Command::CopyBuffer;
                cmd.args.copyBuffer.src = bufD->buffers[0];
                cmd.args.copyBuffer.dst = readback.stagingBuf;
                cmd.args.copyBuffer.desc = copyInfo;

                bufD->lastActiveFrameSlot = currentFrameSlot;

                activeBufferReadbacks.append(readback);
            }
        }
    }

    for (int opIdx = 0; opIdx < ud->activeTextureOpCount; ++opIdx) {
        const QRhiResourceUpdateBatchPrivate::TextureOp &u(ud->textureOps[opIdx]);
        if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Upload) {
            QVkTexture *utexD = QRHI_RES(QVkTexture, u.dst);
            // batch into a single staging buffer and a single CopyBufferToImage with multiple copyInfos
            VkDeviceSize stagingSize = 0;
            for (int layer = 0, maxLayer = u.subresDesc.size(); layer < maxLayer; ++layer) {
                for (int level = 0; level < QRhi::MAX_MIP_LEVELS; ++level) {
                    for (const QRhiTextureSubresourceUploadDescription &subresDesc : std::as_const(u.subresDesc[layer][level]))
                        stagingSize += subresUploadByteSize(subresDesc);
                }
            }

            Q_ASSERT(!utexD->stagingBuffers[currentFrameSlot]);
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = stagingSize;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

            VmaAllocation allocation;
            VkResult err = vmaCreateBuffer(toVmaAllocator(allocator), &bufferInfo, &allocInfo,
                                           &utexD->stagingBuffers[currentFrameSlot], &allocation, nullptr);
            if (err != VK_SUCCESS) {
                qWarning("Failed to create image staging buffer of size %d: %d", int(stagingSize), err);
                printExtraErrorInfo(err);
                continue;
            }
            utexD->stagingAllocations[currentFrameSlot] = allocation;

            BufferImageCopyList copyInfos;
            size_t curOfs = 0;
            void *mp = nullptr;
            VmaAllocation a = toVmaAllocation(utexD->stagingAllocations[currentFrameSlot]);
            err = vmaMapMemory(toVmaAllocator(allocator), a, &mp);
            if (err != VK_SUCCESS) {
                qWarning("Failed to map image data: %d", err);
                continue;
            }

            for (int layer = 0, maxLayer = u.subresDesc.size(); layer < maxLayer; ++layer) {
                for (int level = 0; level < QRhi::MAX_MIP_LEVELS; ++level) {
                    const QList<QRhiTextureSubresourceUploadDescription> &srd(u.subresDesc[layer][level]);
                    if (srd.isEmpty())
                        continue;
                    for (const QRhiTextureSubresourceUploadDescription &subresDesc : std::as_const(srd)) {
                        prepareUploadSubres(utexD, layer, level,
                                            subresDesc, &curOfs, mp, &copyInfos);
                    }
                }
            }
            vmaFlushAllocation(toVmaAllocator(allocator), a, 0, stagingSize);
            vmaUnmapMemory(toVmaAllocator(allocator), a);

            trackedImageBarrier(cbD, utexD, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            QVkCommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QVkCommandBuffer::Command::CopyBufferToImage;
            cmd.args.copyBufferToImage.src = utexD->stagingBuffers[currentFrameSlot];
            cmd.args.copyBufferToImage.dst = utexD->image;
            cmd.args.copyBufferToImage.dstLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            cmd.args.copyBufferToImage.count = copyInfos.size();
            cmd.args.copyBufferToImage.bufferImageCopyIndex = cbD->pools.bufferImageCopy.size();
            cbD->pools.bufferImageCopy.append(copyInfos.constData(), copyInfos.size());

            // no reuse of staging, this is intentional
            QRhiVulkan::DeferredReleaseEntry e;
            e.type = QRhiVulkan::DeferredReleaseEntry::StagingBuffer;
            e.lastActiveFrameSlot = currentFrameSlot;
            e.stagingBuffer.stagingBuffer = utexD->stagingBuffers[currentFrameSlot];
            e.stagingBuffer.stagingAllocation = utexD->stagingAllocations[currentFrameSlot];
            utexD->stagingBuffers[currentFrameSlot] = VK_NULL_HANDLE;
            utexD->stagingAllocations[currentFrameSlot] = nullptr;
            releaseQueue.append(e);

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
            const bool srcIs3D = srcD->m_flags.testFlag(QRhiTexture::ThreeDimensional);
            const bool dstIs3D = dstD->m_flags.testFlag(QRhiTexture::ThreeDimensional);

            VkImageCopy region = {};
            region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.srcSubresource.mipLevel = uint32_t(u.desc.sourceLevel());
            region.srcSubresource.baseArrayLayer = srcIs3D ? 0 : uint32_t(u.desc.sourceLayer());
            region.srcSubresource.layerCount = 1;

            region.srcOffset.x = u.desc.sourceTopLeft().x();
            region.srcOffset.y = u.desc.sourceTopLeft().y();
            if (srcIs3D)
                region.srcOffset.z = uint32_t(u.desc.sourceLayer());

            region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.dstSubresource.mipLevel = uint32_t(u.desc.destinationLevel());
            region.dstSubresource.baseArrayLayer = dstIs3D ? 0 : uint32_t(u.desc.destinationLayer());
            region.dstSubresource.layerCount = 1;

            region.dstOffset.x = u.desc.destinationTopLeft().x();
            region.dstOffset.y = u.desc.destinationTopLeft().y();
            if (dstIs3D)
                region.dstOffset.z = uint32_t(u.desc.destinationLayer());

            const QSize mipSize = q->sizeForMipLevel(u.desc.sourceLevel(), srcD->m_pixelSize);
            const QSize copySize = u.desc.pixelSize().isEmpty() ? mipSize : u.desc.pixelSize();
            region.extent.width = uint32_t(copySize.width());
            region.extent.height = uint32_t(copySize.height());
            region.extent.depth = 1;

            trackedImageBarrier(cbD, srcD, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
            trackedImageBarrier(cbD, dstD, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            QVkCommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QVkCommandBuffer::Command::CopyImage;
            cmd.args.copyImage.src = srcD->image;
            cmd.args.copyImage.srcLayout = srcD->usageState.layout;
            cmd.args.copyImage.dst = dstD->image;
            cmd.args.copyImage.dstLayout = dstD->usageState.layout;
            cmd.args.copyImage.desc = region;

            srcD->lastActiveFrameSlot = dstD->lastActiveFrameSlot = currentFrameSlot;
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Read) {
            TextureReadback readback;
            readback.activeFrameSlot = currentFrameSlot;
            readback.desc = u.rb;
            readback.result = u.result;

            QVkTexture *texD = QRHI_RES(QVkTexture, u.rb.texture());
            QVkSwapChain *swapChainD = nullptr;
            bool is3D = false;
            if (texD) {
                if (texD->samples > VK_SAMPLE_COUNT_1_BIT) {
                    qWarning("Multisample texture cannot be read back");
                    continue;
                }
                is3D = texD->m_flags.testFlag(QRhiTexture::ThreeDimensional);
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
                readback.format = swapchainReadbackTextureFormat(swapChainD->colorFormat, nullptr);
                if (readback.format == QRhiTexture::UnknownFormat)
                    continue;

                // Multisample swapchains need nothing special since resolving
                // happens when ending a renderpass.
            }
            textureFormatInfo(readback.format, readback.pixelSize, nullptr, &readback.byteSize, nullptr);

            // Create a host visible readback buffer.
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = readback.byteSize;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;

            VmaAllocation allocation;
            VkResult err = vmaCreateBuffer(toVmaAllocator(allocator), &bufferInfo, &allocInfo, &readback.stagingBuf, &allocation, nullptr);
            if (err == VK_SUCCESS) {
                readback.stagingAlloc = allocation;
            } else {
                qWarning("Failed to create readback buffer of size %u: %d", readback.byteSize, err);
                printExtraErrorInfo(err);
                continue;
            }

            // Copy from the (optimal and not host visible) image into the buffer.
            VkBufferImageCopy copyDesc = {};
            copyDesc.bufferOffset = 0;
            copyDesc.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyDesc.imageSubresource.mipLevel = uint32_t(u.rb.level());
            copyDesc.imageSubresource.baseArrayLayer = is3D ? 0 : uint32_t(u.rb.layer());
            copyDesc.imageSubresource.layerCount = 1;
            if (is3D)
                copyDesc.imageOffset.z = u.rb.layer();
            copyDesc.imageExtent.width = uint32_t(readback.pixelSize.width());
            copyDesc.imageExtent.height = uint32_t(readback.pixelSize.height());
            copyDesc.imageExtent.depth = 1;

            if (texD) {
                trackedImageBarrier(cbD, texD, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                    VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
                QVkCommandBuffer::Command &cmd(cbD->commands.get());
                cmd.cmd = QVkCommandBuffer::Command::CopyImageToBuffer;
                cmd.args.copyImageToBuffer.src = texD->image;
                cmd.args.copyImageToBuffer.srcLayout = texD->usageState.layout;
                cmd.args.copyImageToBuffer.dst = readback.stagingBuf;
                cmd.args.copyImageToBuffer.desc = copyDesc;
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

                QVkCommandBuffer::Command &cmd(cbD->commands.get());
                cmd.cmd = QVkCommandBuffer::Command::CopyImageToBuffer;
                cmd.args.copyImageToBuffer.src = image;
                cmd.args.copyImageToBuffer.srcLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                cmd.args.copyImageToBuffer.dst = readback.stagingBuf;
                cmd.args.copyImageToBuffer.desc = copyDesc;
            }

            activeTextureReadbacks.append(readback);
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::GenMips) {
            QVkTexture *utexD = QRHI_RES(QVkTexture, u.dst);
            Q_ASSERT(utexD->m_flags.testFlag(QRhiTexture::UsedWithGenerateMips));
            const bool isCube = utexD->m_flags.testFlag(QRhiTexture::CubeMap);
            const bool isArray = utexD->m_flags.testFlag(QRhiTexture::TextureArray);
            const bool is3D = utexD->m_flags.testFlag(QRhiTexture::ThreeDimensional);

            VkImageLayout origLayout = utexD->usageState.layout;
            VkAccessFlags origAccess = utexD->usageState.access;
            VkPipelineStageFlags origStage = utexD->usageState.stage;
            if (!origStage)
                origStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

            for (int layer = 0; layer < (isCube ? 6 : (isArray ? qMax(0, utexD->m_arraySize) : 1)); ++layer) {
                int w = utexD->m_pixelSize.width();
                int h = utexD->m_pixelSize.height();
                int depth = is3D ? qMax(1, utexD->m_depth) : 1;
                for (int level = 1; level < int(utexD->mipLevelCount); ++level) {
                    if (level == 1) {
                        subresourceBarrier(cbD, utexD->image,
                                           origLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                           origAccess, VK_ACCESS_TRANSFER_READ_BIT,
                                           origStage, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                           layer, 1,
                                           level - 1, 1);
                    } else {
                        subresourceBarrier(cbD, utexD->image,
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                           VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                                           VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                           layer, 1,
                                           level - 1, 1);
                    }

                    subresourceBarrier(cbD, utexD->image,
                                       origLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       origAccess, VK_ACCESS_TRANSFER_WRITE_BIT,
                                       origStage, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                       layer, 1,
                                       level, 1);

                    VkImageBlit region = {};
                    region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    region.srcSubresource.mipLevel = uint32_t(level) - 1;
                    region.srcSubresource.baseArrayLayer = uint32_t(layer);
                    region.srcSubresource.layerCount = 1;

                    region.srcOffsets[1].x = qMax(1, w);
                    region.srcOffsets[1].y = qMax(1, h);
                    region.srcOffsets[1].z = qMax(1, depth);

                    region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    region.dstSubresource.mipLevel = uint32_t(level);
                    region.dstSubresource.baseArrayLayer = uint32_t(layer);
                    region.dstSubresource.layerCount = 1;

                    region.dstOffsets[1].x = qMax(1, w >> 1);
                    region.dstOffsets[1].y = qMax(1, h >> 1);
                    region.dstOffsets[1].z = qMax(1, depth >> 1);

                    QVkCommandBuffer::Command &cmd(cbD->commands.get());
                    cmd.cmd = QVkCommandBuffer::Command::BlitImage;
                    cmd.args.blitImage.src = utexD->image;
                    cmd.args.blitImage.srcLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    cmd.args.blitImage.dst = utexD->image;
                    cmd.args.blitImage.dstLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    cmd.args.blitImage.filter = VK_FILTER_LINEAR;
                    cmd.args.blitImage.desc = region;

                    w >>= 1;
                    h >>= 1;
                    depth >>= 1;
                }

                if (utexD->mipLevelCount > 1) {
                    subresourceBarrier(cbD, utexD->image,
                                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, origLayout,
                                       VK_ACCESS_TRANSFER_READ_BIT, origAccess,
                                       VK_PIPELINE_STAGE_TRANSFER_BIT, origStage,
                                       layer, 1,
                                       0, int(utexD->mipLevelCount) - 1);
                    subresourceBarrier(cbD, utexD->image,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, origLayout,
                                       VK_ACCESS_TRANSFER_WRITE_BIT, origAccess,
                                       VK_PIPELINE_STAGE_TRANSFER_BIT, origStage,
                                       layer, 1,
                                       int(utexD->mipLevelCount) - 1, 1);
                }
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
    quint32 changeBegin = UINT32_MAX;
    quint32 changeEnd = 0;
    for (const QVkBuffer::DynamicUpdate &u : std::as_const(bufD->pendingDynamicUpdates[slot])) {
        memcpy(static_cast<char *>(p) + u.offset, u.data.constData(), u.data.size());
        if (u.offset < changeBegin)
            changeBegin = u.offset;
        if (u.offset + u.data.size() > changeEnd)
            changeEnd = u.offset + u.data.size();
    }
    if (changeBegin < UINT32_MAX && changeBegin < changeEnd)
        vmaFlushAllocation(toVmaAllocator(allocator), a, changeBegin, changeEnd - changeBegin);
    vmaUnmapMemory(toVmaAllocator(allocator), a);

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
    for (int i = 0; i < QRhi::MAX_MIP_LEVELS; ++i) {
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
    for (int i = releaseQueue.size() - 1; i >= 0; --i) {
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
            case QRhiVulkan::DeferredReleaseEntry::SecondaryCommandBuffer:
                freeSecondaryCbs[e.lastActiveFrameSlot].append(e.secondaryCommandBuffer.cb);
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

    for (int i = activeTextureReadbacks.size() - 1; i >= 0; --i) {
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

            if (readback.result->completed)
                completedCallbacks.append(readback.result->completed);

            activeTextureReadbacks.removeLast();
        }
    }

    for (int i = activeBufferReadbacks.size() - 1; i >= 0; --i) {
        const QRhiVulkan::BufferReadback &readback(activeBufferReadbacks[i]);
        if (forced || currentFrameSlot == readback.activeFrameSlot || readback.activeFrameSlot < 0) {
            VmaAllocation a = toVmaAllocation(readback.stagingAlloc);
            void *p = nullptr;
            VkResult err = vmaMapMemory(toVmaAllocator(allocator), a, &p);
            if (err == VK_SUCCESS && p) {
                readback.result->data.resize(readback.byteSize);
                memcpy(readback.result->data.data(), p, readback.byteSize);
                vmaUnmapMemory(toVmaAllocator(allocator), a);
            } else {
                qWarning("Failed to map buffer readback buffer of size %d: %d", readback.byteSize, err);
            }

            vmaDestroyBuffer(toVmaAllocator(allocator), readback.stagingBuf, a);

            if (readback.result->completed)
                completedCallbacks.append(readback.result->completed);

            activeBufferReadbacks.removeLast();
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

QList<int> QRhiVulkan::supportedSampleCounts() const
{
    const VkPhysicalDeviceLimits *limits = &physDevProperties.limits;
    VkSampleCountFlags color = limits->framebufferColorSampleCounts;
    VkSampleCountFlags depth = limits->framebufferDepthSampleCounts;
    VkSampleCountFlags stencil = limits->framebufferStencilSampleCounts;
    QList<int> result;

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

    Q_UNREACHABLE_RETURN(VK_SAMPLE_COUNT_1_BIT);
}

void QRhiVulkan::enqueueTransitionPassResources(QVkCommandBuffer *cbD)
{
    cbD->passResTrackers.append(QRhiPassResourceTracker());
    cbD->currentPassResTrackerIndex = cbD->passResTrackers.size() - 1;

    QVkCommandBuffer::Command &cmd(cbD->commands.get());
    cmd.cmd = QVkCommandBuffer::Command::TransitionPassResources;
    cmd.args.transitionResources.trackerIndex = cbD->passResTrackers.size() - 1;
}

void QRhiVulkan::recordPrimaryCommandBuffer(QVkCommandBuffer *cbD)
{
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::NoPass);

    for (auto it = cbD->commands.begin(), end = cbD->commands.end(); it != end; ++it) {
        QVkCommandBuffer::Command &cmd(*it);
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
                                     cmd.args.beginRenderPass.useSecondaryCb ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
                                                                             : VK_SUBPASS_CONTENTS_INLINE);
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
#ifdef VK_EXT_debug_utils
            cmd.args.debugMarkerBegin.label.pLabelName =
                    cbD->pools.debugMarkerData[cmd.args.debugMarkerBegin.labelNameIndex].constData();
            vkCmdBeginDebugUtilsLabelEXT(cbD->cb, &cmd.args.debugMarkerBegin.label);
#endif
            break;
        case QVkCommandBuffer::Command::DebugMarkerEnd:
#ifdef VK_EXT_debug_utils
            vkCmdEndDebugUtilsLabelEXT(cbD->cb);
#endif
            break;
        case QVkCommandBuffer::Command::DebugMarkerInsert:
#ifdef VK_EXT_debug_utils
            cmd.args.debugMarkerInsert.label.pLabelName =
                    cbD->pools.debugMarkerData[cmd.args.debugMarkerInsert.labelNameIndex].constData();
            vkCmdInsertDebugUtilsLabelEXT(cbD->cb, &cmd.args.debugMarkerInsert.label);
#endif
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
    case QRhiPassResourceTracker::BufTCStage:
        return VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
    case QRhiPassResourceTracker::BufTEStage:
        return VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
    case QRhiPassResourceTracker::BufFragmentStage:
        return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    case QRhiPassResourceTracker::BufComputeStage:
        return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    case QRhiPassResourceTracker::BufGeometryStage:
        return VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
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
    case QRhiPassResourceTracker::TexTCStage:
        return VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
    case QRhiPassResourceTracker::TexTEStage:
        return VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
    case QRhiPassResourceTracker::TexFragmentStage:
        return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    case QRhiPassResourceTracker::TexColorOutputStage:
        return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    case QRhiPassResourceTracker::TexDepthOutputStage:
        return VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    case QRhiPassResourceTracker::TexComputeStage:
        return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    case QRhiPassResourceTracker::TexGeometryStage:
        return VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
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
    const VkAccessFlags newAccess = toVkAccess(access);
    const VkPipelineStageFlags newStage = toVkPipelineStage(stage);
    if (u.access == newAccess && u.stage == newStage) {
        if (!accessIsWrite(access))
            return;
    }
    passResTracker->registerBuffer(bufD, slot, &access, &stage, toPassTrackerUsageState(u));
    u.access = newAccess;
    u.stage = newStage;
}

void QRhiVulkan::trackedRegisterTexture(QRhiPassResourceTracker *passResTracker,
                                        QVkTexture *texD,
                                        QRhiPassResourceTracker::TextureAccess access,
                                        QRhiPassResourceTracker::TextureStage stage)
{
    QVkTexture::UsageState &u(texD->usageState);
    const VkAccessFlags newAccess = toVkAccess(access);
    const VkPipelineStageFlags newStage = toVkPipelineStage(stage);
    const VkImageLayout newLayout = toVkLayout(access);
    if (u.access == newAccess && u.stage == newStage && u.layout == newLayout) {
        if (!accessIsWrite(access))
            return;
    }
    passResTracker->registerTexture(texD, &access, &stage, toPassTrackerUsageState(u));
    u.layout = newLayout;
    u.access = newAccess;
    u.stage = newStage;
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
        VkBufferMemoryBarrier bufMemBarrier = {};
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
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.subresourceRange.aspectMask = aspectMaskForTextureFormat(texD->m_format);
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

QRhiBuffer *QRhiVulkan::createBuffer(QRhiBuffer::Type type, QRhiBuffer::UsageFlags usage, quint32 size)
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
        return caps.debugUtils;
    case QRhi::Timestamps:
        return timestampValidBits != 0;
    case QRhi::Instancing:
        return true;
    case QRhi::CustomInstanceStepRate:
        return caps.vertexAttribDivisor;
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
        return caps.compute;
    case QRhi::WideLines:
        return caps.wideLines;
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
    case QRhi::RenderToNonBaseMipLevel:
        return true;
    case QRhi::IntAttributes:
        return true;
    case QRhi::ScreenSpaceDerivatives:
        return true;
    case QRhi::ReadBackAnyTextureFormat:
        return true;
    case QRhi::PipelineCacheDataLoadSave:
        return true;
    case QRhi::ImageDataStride:
        return true;
    case QRhi::RenderBufferImport:
        return false;
    case QRhi::ThreeDimensionalTextures:
        return true;
    case QRhi::RenderTo3DTextureSlice:
        return caps.texture3DSliceAs2D;
    case QRhi::TextureArrays:
        return true;
    case QRhi::Tessellation:
        return caps.tessellation;
    case QRhi::GeometryShader:
        return caps.geometryShader;
    case QRhi::TextureArrayRange:
        return true;
    case QRhi::NonFillPolygonMode:
        return caps.nonFillPolygonMode;
    case QRhi::OneDimensionalTextures:
        return true;
    case QRhi::OneDimensionalTextureMipmaps:
        return true;
    case QRhi::HalfAttributes:
        return true;
    case QRhi::RenderToOneDimensionalTexture:
        return true;
    case QRhi::ThreeDimensionalTextureMipmaps:
        return true;
    default:
        Q_UNREACHABLE_RETURN(false);
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
    case QRhi::MaxThreadGroupsPerDimension:
        return int(qMin(physDevProperties.limits.maxComputeWorkGroupCount[0],
                   qMin(physDevProperties.limits.maxComputeWorkGroupCount[1],
                        physDevProperties.limits.maxComputeWorkGroupCount[2])));
    case QRhi::MaxThreadsPerThreadGroup:
        return int(physDevProperties.limits.maxComputeWorkGroupInvocations);
    case QRhi::MaxThreadGroupX:
        return int(physDevProperties.limits.maxComputeWorkGroupSize[0]);
    case QRhi::MaxThreadGroupY:
        return int(physDevProperties.limits.maxComputeWorkGroupSize[1]);
    case QRhi::MaxThreadGroupZ:
        return int(physDevProperties.limits.maxComputeWorkGroupSize[2]);
    case QRhi::TextureArraySizeMax:
        return int(physDevProperties.limits.maxImageArrayLayers);
    case QRhi::MaxUniformBufferRange:
        return int(qMin<uint32_t>(INT_MAX, physDevProperties.limits.maxUniformBufferRange));
    case QRhi::MaxVertexInputs:
        return physDevProperties.limits.maxVertexInputAttributes;
    case QRhi::MaxVertexOutputs:
        return physDevProperties.limits.maxVertexOutputComponents / 4;
    default:
        Q_UNREACHABLE_RETURN(0);
    }
}

const QRhiNativeHandles *QRhiVulkan::nativeHandles()
{
    return &nativeHandlesStruct;
}

QRhiDriverInfo QRhiVulkan::driverInfo() const
{
    return driverInfoStruct;
}

QRhiStats QRhiVulkan::statistics()
{
    QRhiStats result;
    result.totalPipelineCreationTime = totalPipelineCreationTime();

    VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
    vmaGetHeapBudgets(toVmaAllocator(allocator), budgets);

    uint32_t count = toVmaAllocator(allocator)->GetMemoryHeapCount();
    for (uint32_t i = 0; i < count; ++i) {
        const VmaStatistics &stats(budgets[i].statistics);
        result.blockCount += stats.blockCount;
        result.allocCount += stats.allocationCount;
        result.usedBytes += stats.allocationBytes;
        result.unusedBytes += stats.blockBytes - stats.allocationBytes;
    }

    return result;
}

bool QRhiVulkan::makeThreadLocalNativeContextCurrent()
{
    // not applicable
    return false;
}

void QRhiVulkan::releaseCachedResources()
{
    releaseCachedResourcesCalledBeforeFrameStart = true;
}

bool QRhiVulkan::isDeviceLost() const
{
    return deviceLost;
}

struct QVkPipelineCacheDataHeader
{
    quint32 rhiId;
    quint32 arch;
    quint32 driverVersion;
    quint32 vendorId;
    quint32 deviceId;
    quint32 dataSize;
    quint32 uuidSize;
    quint32 reserved;
};

QByteArray QRhiVulkan::pipelineCacheData()
{
    Q_STATIC_ASSERT(sizeof(QVkPipelineCacheDataHeader) == 32);

    QByteArray data;
    if (!pipelineCache || !rhiFlags.testFlag(QRhi::EnablePipelineCacheDataSave))
        return data;

    size_t dataSize = 0;
    VkResult err = df->vkGetPipelineCacheData(dev, pipelineCache, &dataSize, nullptr);
    if (err != VK_SUCCESS) {
        qCDebug(QRHI_LOG_INFO, "Failed to get pipeline cache data size: %d", err);
        return QByteArray();
    }
    const size_t headerSize = sizeof(QVkPipelineCacheDataHeader);
    const size_t dataOffset = headerSize + VK_UUID_SIZE;
    data.resize(dataOffset + dataSize);
    err = df->vkGetPipelineCacheData(dev, pipelineCache, &dataSize, data.data() + dataOffset);
    if (err != VK_SUCCESS) {
        qCDebug(QRHI_LOG_INFO, "Failed to get pipeline cache data of %d bytes: %d", int(dataSize), err);
        return QByteArray();
    }

    QVkPipelineCacheDataHeader header;
    header.rhiId = pipelineCacheRhiId();
    header.arch = quint32(sizeof(void*));
    header.driverVersion = physDevProperties.driverVersion;
    header.vendorId = physDevProperties.vendorID;
    header.deviceId = physDevProperties.deviceID;
    header.dataSize = quint32(dataSize);
    header.uuidSize = VK_UUID_SIZE;
    memcpy(data.data(), &header, headerSize);
    memcpy(data.data() + headerSize, physDevProperties.pipelineCacheUUID, VK_UUID_SIZE);

    return data;
}

void QRhiVulkan::setPipelineCacheData(const QByteArray &data)
{
    if (data.isEmpty())
        return;

    const size_t headerSize = sizeof(QVkPipelineCacheDataHeader);
    if (data.size() < qsizetype(headerSize)) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: Invalid blob size");
        return;
    }
    QVkPipelineCacheDataHeader header;
    memcpy(&header, data.constData(), headerSize);

    const quint32 rhiId = pipelineCacheRhiId();
    if (header.rhiId != rhiId) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: The data is for a different QRhi version or backend (%u, %u)",
                rhiId, header.rhiId);
        return;
    }
    const quint32 arch = quint32(sizeof(void*));
    if (header.arch != arch) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: Architecture does not match (%u, %u)",
                 arch, header.arch);
        return;
    }
    if (header.driverVersion != physDevProperties.driverVersion) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: driverVersion does not match (%u, %u)",
                physDevProperties.driverVersion, header.driverVersion);
        return;
    }
    if (header.vendorId != physDevProperties.vendorID) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: vendorID does not match (%u, %u)",
                physDevProperties.vendorID, header.vendorId);
        return;
    }
    if (header.deviceId != physDevProperties.deviceID) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: deviceID does not match (%u, %u)",
                physDevProperties.deviceID, header.deviceId);
        return;
    }
    if (header.uuidSize != VK_UUID_SIZE) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: VK_UUID_SIZE does not match (%u, %u)",
                quint32(VK_UUID_SIZE), header.uuidSize);
        return;
    }

    if (data.size() < qsizetype(headerSize + VK_UUID_SIZE)) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: Invalid blob, no uuid");
        return;
    }
    if (memcmp(data.constData() + headerSize, physDevProperties.pipelineCacheUUID, VK_UUID_SIZE)) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: pipelineCacheUUID does not match");
        return;
    }

    const size_t dataOffset = headerSize + VK_UUID_SIZE;
    if (data.size() < qsizetype(dataOffset + header.dataSize)) {
        qCDebug(QRHI_LOG_INFO, "setPipelineCacheData: Invalid blob, data missing");
        return;
    }

    if (pipelineCache) {
        df->vkDestroyPipelineCache(dev, pipelineCache, nullptr);
        pipelineCache = VK_NULL_HANDLE;
    }

    if (ensurePipelineCache(data.constData() + dataOffset, header.dataSize)) {
        qCDebug(QRHI_LOG_INFO, "Created pipeline cache with initial data of %d bytes",
                int(header.dataSize));
    } else {
        qCDebug(QRHI_LOG_INFO, "Failed to create pipeline cache with initial data specified");
    }
}

QRhiRenderBuffer *QRhiVulkan::createRenderBuffer(QRhiRenderBuffer::Type type, const QSize &pixelSize,
                                                 int sampleCount, QRhiRenderBuffer::Flags flags,
                                                 QRhiTexture::Format backingFormatHint)
{
    return new QVkRenderBuffer(this, type, pixelSize, sampleCount, flags, backingFormatHint);
}

QRhiTexture *QRhiVulkan::createTexture(QRhiTexture::Format format,
                                       const QSize &pixelSize, int depth, int arraySize,
                                       int sampleCount, QRhiTexture::Flags flags)
{
    return new QVkTexture(this, format, pixelSize, depth, arraySize, sampleCount, flags);
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
        if (cbD->passUsesSecondaryCb) {
            df->vkCmdBindPipeline(cbD->activeSecondaryCbStack.last(), VK_PIPELINE_BIND_POINT_GRAPHICS, psD->pipeline);
        } else {
            QVkCommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QVkCommandBuffer::Command::BindPipeline;
            cmd.args.bindPipeline.bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            cmd.args.bindPipeline.pipeline = psD->pipeline;
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
    QRhiPassResourceTracker &passResTracker(cbD->passResTrackers[cbD->currentPassResTrackerIndex]);
    QVkGraphicsPipeline *gfxPsD = QRHI_RES(QVkGraphicsPipeline, cbD->currentGraphicsPipeline);
    QVkComputePipeline *compPsD = QRHI_RES(QVkComputePipeline, cbD->currentComputePipeline);

    if (!srb) {
        if (gfxPsD)
            srb = gfxPsD->m_shaderResourceBindings;
        else
            srb = compPsD->m_shaderResourceBindings;
    }

    QVkShaderResourceBindings *srbD = QRHI_RES(QVkShaderResourceBindings, srb);
    const int descSetIdx = srbD->hasSlottedResource ? currentFrameSlot : 0;
    auto &descSetBd(srbD->boundResourceData[descSetIdx]);
    bool rewriteDescSet = false;

    // Do host writes and mark referenced shader resources as in-use.
    // Also prepare to ensure the descriptor set we are going to bind refers to up-to-date Vk objects.
    for (int i = 0, ie = srbD->sortedBindings.size(); i != ie; ++i) {
        const QRhiShaderResourceBinding::Data *b = shaderResourceBindingData(srbD->sortedBindings[i]);
        QVkShaderResourceBindings::BoundResourceData &bd(descSetBd[i]);
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
        case QRhiShaderResourceBinding::Texture:
        case QRhiShaderResourceBinding::Sampler:
        {
            const QRhiShaderResourceBinding::Data::TextureAndOrSamplerData *data = &b->u.stex;
            if (bd.stex.count != data->count) {
                bd.stex.count = data->count;
                rewriteDescSet = true;
            }
            for (int elem = 0; elem < data->count; ++elem) {
                QVkTexture *texD = QRHI_RES(QVkTexture, data->texSamplers[elem].tex);
                QVkSampler *samplerD = QRHI_RES(QVkSampler, data->texSamplers[elem].sampler);
                // We use the same code path for both combined and separate
                // images and samplers, so tex or sampler (but not both) can be
                // null here.
                Q_ASSERT(texD || samplerD);
                if (texD) {
                    texD->lastActiveFrameSlot = currentFrameSlot;
                    trackedRegisterTexture(&passResTracker, texD,
                                           QRhiPassResourceTracker::TexSample,
                                           QRhiPassResourceTracker::toPassTrackerTextureStage(b->stage));
                }
                if (samplerD)
                    samplerD->lastActiveFrameSlot = currentFrameSlot;
                const quint64 texId = texD ? texD->m_id : 0;
                const uint texGen = texD ? texD->generation : 0;
                const quint64 samplerId = samplerD ? samplerD->m_id : 0;
                const uint samplerGen = samplerD ? samplerD->generation : 0;
                if (texGen != bd.stex.d[elem].texGeneration
                        || texId != bd.stex.d[elem].texId
                        || samplerGen != bd.stex.d[elem].samplerGeneration
                        || samplerId != bd.stex.d[elem].samplerId)
                {
                    rewriteDescSet = true;
                    bd.stex.d[elem].texId = texId;
                    bd.stex.d[elem].texGeneration = texGen;
                    bd.stex.d[elem].samplerId = samplerId;
                    bd.stex.d[elem].samplerGeneration = samplerGen;
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
    const bool forceRebind = (srbD->hasSlottedResource && cbD->currentDescSetSlot != descSetIdx) || srbD->hasDynamicOffset;

    const bool srbChanged = gfxPsD ? (cbD->currentGraphicsSrb != srb) : (cbD->currentComputeSrb != srb);

    if (forceRebind || rewriteDescSet || srbChanged || cbD->currentSrbGeneration != srbD->generation) {
        QVarLengthArray<uint32_t, 4> dynOfs;
        if (srbD->hasDynamicOffset) {
            // Filling out dynOfs based on the sorted bindings is important
            // because dynOfs has to be ordered based on the binding numbers,
            // and neither srb nor dynamicOffsets has any such ordering
            // requirement.
            for (const QRhiShaderResourceBinding &binding : std::as_const(srbD->sortedBindings)) {
                const QRhiShaderResourceBinding::Data *b = shaderResourceBindingData(binding);
                if (b->type == QRhiShaderResourceBinding::UniformBuffer && b->u.ubuf.hasDynamicOffset) {
                    uint32_t offset = 0;
                    for (int i = 0; i < dynamicOffsetCount; ++i) {
                        const QRhiCommandBuffer::DynamicOffset &bindingOffsetPair(dynamicOffsets[i]);
                        if (bindingOffsetPair.first == b->binding) {
                            offset = bindingOffsetPair.second;
                            break;
                        }
                    }
                    dynOfs.append(offset); // use 0 if dynamicOffsets did not contain this binding
                }
            }
        }

        if (cbD->passUsesSecondaryCb) {
            df->vkCmdBindDescriptorSets(cbD->activeSecondaryCbStack.last(),
                                        gfxPsD ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE,
                                        gfxPsD ? gfxPsD->layout : compPsD->layout,
                                        0, 1, &srbD->descSets[descSetIdx],
                                        uint32_t(dynOfs.size()),
                                        dynOfs.size() ? dynOfs.constData() : nullptr);
        } else {
            QVkCommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QVkCommandBuffer::Command::BindDescriptorSet;
            cmd.args.bindDescriptorSet.bindPoint = gfxPsD ? VK_PIPELINE_BIND_POINT_GRAPHICS
                                                          : VK_PIPELINE_BIND_POINT_COMPUTE;
            cmd.args.bindDescriptorSet.pipelineLayout = gfxPsD ? gfxPsD->layout : compPsD->layout;
            cmd.args.bindDescriptorSet.descSet = srbD->descSets[descSetIdx];
            cmd.args.bindDescriptorSet.dynamicOffsetCount = dynOfs.size();
            cmd.args.bindDescriptorSet.dynamicOffsetIndex = cbD->pools.dynamicOffset.size();
            cbD->pools.dynamicOffset.append(dynOfs.constData(), dynOfs.size());
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

        if (cbD->passUsesSecondaryCb) {
            df->vkCmdBindVertexBuffers(cbD->activeSecondaryCbStack.last(), uint32_t(startBinding),
                                       uint32_t(bufs.size()), bufs.constData(), ofs.constData());
        } else {
            QVkCommandBuffer::Command &cmd(cbD->commands.get());
            cmd.cmd = QVkCommandBuffer::Command::BindVertexBuffer;
            cmd.args.bindVertexBuffer.startBinding = startBinding;
            cmd.args.bindVertexBuffer.count = bufs.size();
            cmd.args.bindVertexBuffer.vertexBufferIndex = cbD->pools.vertexBuffer.size();
            cbD->pools.vertexBuffer.append(bufs.constData(), bufs.size());
            cmd.args.bindVertexBuffer.vertexBufferOffsetIndex = cbD->pools.vertexBufferOffset.size();
            cbD->pools.vertexBufferOffset.append(ofs.constData(), ofs.size());
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

            if (cbD->passUsesSecondaryCb) {
                df->vkCmdBindIndexBuffer(cbD->activeSecondaryCbStack.last(), vkindexbuf, indexOffset, type);
            } else {
                QVkCommandBuffer::Command &cmd(cbD->commands.get());
                cmd.cmd = QVkCommandBuffer::Command::BindIndexBuffer;
                cmd.args.bindIndexBuffer.buf = vkindexbuf;
                cmd.args.bindIndexBuffer.ofs = indexOffset;
                cmd.args.bindIndexBuffer.type = type;
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
    if (!qrhi_toTopLeftRenderTargetRect<UnBounded>(outputSize, viewport.viewport(), &x, &y, &w, &h))
        return;

    QVkCommandBuffer::Command &cmd(cbD->commands.get());
    VkViewport *vp = &cmd.args.setViewport.viewport;
    vp->x = x;
    vp->y = y;
    vp->width = w;
    vp->height = h;
    vp->minDepth = viewport.minDepth();
    vp->maxDepth = viewport.maxDepth();

    if (cbD->passUsesSecondaryCb) {
        df->vkCmdSetViewport(cbD->activeSecondaryCbStack.last(), 0, 1, vp);
        cbD->commands.unget();
    } else {
        cmd.cmd = QVkCommandBuffer::Command::SetViewport;
    }

    if (cbD->currentGraphicsPipeline
        && !QRHI_RES(QVkGraphicsPipeline, cbD->currentGraphicsPipeline)
                    ->m_flags.testFlag(QRhiGraphicsPipeline::UsesScissor)) {
        QVkCommandBuffer::Command &cmd(cbD->commands.get());
        VkRect2D *s = &cmd.args.setScissor.scissor;
        qrhi_toTopLeftRenderTargetRect<Bounded>(outputSize, viewport.viewport(), &x, &y, &w, &h);
        s->offset.x = int32_t(x);
        s->offset.y = int32_t(y);
        s->extent.width = uint32_t(w);
        s->extent.height = uint32_t(h);
        if (cbD->passUsesSecondaryCb) {
            df->vkCmdSetScissor(cbD->activeSecondaryCbStack.last(), 0, 1, s);
            cbD->commands.unget();
        } else {
            cmd.cmd = QVkCommandBuffer::Command::SetScissor;
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
    if (!qrhi_toTopLeftRenderTargetRect<Bounded>(outputSize, scissor.scissor(), &x, &y, &w, &h))
        return;

    QVkCommandBuffer::Command &cmd(cbD->commands.get());
    VkRect2D *s = &cmd.args.setScissor.scissor;
    s->offset.x = x;
    s->offset.y = y;
    s->extent.width = uint32_t(w);
    s->extent.height = uint32_t(h);

    if (cbD->passUsesSecondaryCb) {
        df->vkCmdSetScissor(cbD->activeSecondaryCbStack.last(), 0, 1, s);
        cbD->commands.unget();
    } else {
        cmd.cmd = QVkCommandBuffer::Command::SetScissor;
    }
}

void QRhiVulkan::setBlendConstants(QRhiCommandBuffer *cb, const QColor &c)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::RenderPass);

    if (cbD->passUsesSecondaryCb) {
        float constants[] = { float(c.redF()), float(c.greenF()), float(c.blueF()), float(c.alphaF()) };
        df->vkCmdSetBlendConstants(cbD->activeSecondaryCbStack.last(), constants);
    } else {
        QVkCommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QVkCommandBuffer::Command::SetBlendConstants;
        cmd.args.setBlendConstants.c[0] = float(c.redF());
        cmd.args.setBlendConstants.c[1] = float(c.greenF());
        cmd.args.setBlendConstants.c[2] = float(c.blueF());
        cmd.args.setBlendConstants.c[3] = float(c.alphaF());
    }
}

void QRhiVulkan::setStencilRef(QRhiCommandBuffer *cb, quint32 refValue)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::RenderPass);

    if (cbD->passUsesSecondaryCb) {
        df->vkCmdSetStencilReference(cbD->activeSecondaryCbStack.last(), VK_STENCIL_FRONT_AND_BACK, refValue);
    } else {
        QVkCommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QVkCommandBuffer::Command::SetStencilRef;
        cmd.args.setStencilRef.ref = refValue;
    }
}

void QRhiVulkan::draw(QRhiCommandBuffer *cb, quint32 vertexCount,
                      quint32 instanceCount, quint32 firstVertex, quint32 firstInstance)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::RenderPass);

    if (cbD->passUsesSecondaryCb) {
        df->vkCmdDraw(cbD->activeSecondaryCbStack.last(), vertexCount, instanceCount, firstVertex, firstInstance);
    } else {
        QVkCommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QVkCommandBuffer::Command::Draw;
        cmd.args.draw.vertexCount = vertexCount;
        cmd.args.draw.instanceCount = instanceCount;
        cmd.args.draw.firstVertex = firstVertex;
        cmd.args.draw.firstInstance = firstInstance;
    }
}

void QRhiVulkan::drawIndexed(QRhiCommandBuffer *cb, quint32 indexCount,
                             quint32 instanceCount, quint32 firstIndex, qint32 vertexOffset, quint32 firstInstance)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QVkCommandBuffer::RenderPass);

    if (cbD->passUsesSecondaryCb) {
        df->vkCmdDrawIndexed(cbD->activeSecondaryCbStack.last(), indexCount, instanceCount,
                             firstIndex, vertexOffset, firstInstance);
    } else {
        QVkCommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QVkCommandBuffer::Command::DrawIndexed;
        cmd.args.drawIndexed.indexCount = indexCount;
        cmd.args.drawIndexed.instanceCount = instanceCount;
        cmd.args.drawIndexed.firstIndex = firstIndex;
        cmd.args.drawIndexed.vertexOffset = vertexOffset;
        cmd.args.drawIndexed.firstInstance = firstInstance;
    }
}

void QRhiVulkan::debugMarkBegin(QRhiCommandBuffer *cb, const QByteArray &name)
{
#ifdef VK_EXT_debug_utils
    if (!debugMarkers || !caps.debugUtils)
        return;

    VkDebugUtilsLabelEXT label = {};
    label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;

    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    if (cbD->recordingPass != QVkCommandBuffer::NoPass && cbD->passUsesSecondaryCb) {
        label.pLabelName = name.constData();
        vkCmdBeginDebugUtilsLabelEXT(cbD->activeSecondaryCbStack.last(), &label);
    } else {
        QVkCommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QVkCommandBuffer::Command::DebugMarkerBegin;
        cmd.args.debugMarkerBegin.label = label;
        cmd.args.debugMarkerBegin.labelNameIndex = cbD->pools.debugMarkerData.size();
        cbD->pools.debugMarkerData.append(name);
    }
#else
    Q_UNUSED(cb);
    Q_UNUSED(name);
#endif
}

void QRhiVulkan::debugMarkEnd(QRhiCommandBuffer *cb)
{
#ifdef VK_EXT_debug_utils
    if (!debugMarkers || !caps.debugUtils)
        return;

    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    if (cbD->recordingPass != QVkCommandBuffer::NoPass && cbD->passUsesSecondaryCb) {
        vkCmdEndDebugUtilsLabelEXT(cbD->activeSecondaryCbStack.last());
    } else {
        QVkCommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QVkCommandBuffer::Command::DebugMarkerEnd;
    }
#else
    Q_UNUSED(cb);
#endif
}

void QRhiVulkan::debugMarkMsg(QRhiCommandBuffer *cb, const QByteArray &msg)
{
#ifdef VK_EXT_debug_utils
    if (!debugMarkers || !caps.debugUtils)
        return;

    VkDebugUtilsLabelEXT label = {};
    label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;

    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    if (cbD->recordingPass != QVkCommandBuffer::NoPass && cbD->passUsesSecondaryCb) {
        label.pLabelName = msg.constData();
        vkCmdInsertDebugUtilsLabelEXT(cbD->activeSecondaryCbStack.last(), &label);
    } else {
        QVkCommandBuffer::Command &cmd(cbD->commands.get());
        cmd.cmd = QVkCommandBuffer::Command::DebugMarkerInsert;
        cmd.args.debugMarkerInsert.label = label;
        cmd.args.debugMarkerInsert.labelNameIndex = cbD->pools.debugMarkerData.size();
        cbD->pools.debugMarkerData.append(msg);
    }
#else
    Q_UNUSED(cb);
    Q_UNUSED(msg);
#endif
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
        case QRhiResource::SwapChainRenderTarget:
            rtD = &QRHI_RES(QVkSwapChainRenderTarget, cbD->currentTarget)->d;
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

    if (!cbD->passUsesSecondaryCb) {
        qWarning("beginExternal() within a pass is only supported with secondary command buffers. "
                 "This can be enabled by passing QRhiCommandBuffer::ExternalContent to beginPass().");
        return;
    }

    VkCommandBuffer secondaryCb = cbD->activeSecondaryCbStack.last();
    cbD->activeSecondaryCbStack.removeLast();
    endAndEnqueueSecondaryCommandBuffer(secondaryCb, cbD);

    VkCommandBuffer extCb = startSecondaryCommandBuffer(maybeRenderTargetData(cbD));
    if (extCb) {
        cbD->activeSecondaryCbStack.append(extCb);
        cbD->inExternal = true;
    }
}

void QRhiVulkan::endExternal(QRhiCommandBuffer *cb)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);

    if (cbD->recordingPass == QVkCommandBuffer::NoPass) {
        Q_ASSERT(cbD->commands.isEmpty() && cbD->currentPassResTrackerIndex == -1);
    } else if (cbD->inExternal) {
        VkCommandBuffer extCb = cbD->activeSecondaryCbStack.last();
        cbD->activeSecondaryCbStack.removeLast();
        endAndEnqueueSecondaryCommandBuffer(extCb, cbD);
        cbD->activeSecondaryCbStack.append(startSecondaryCommandBuffer(maybeRenderTargetData(cbD)));
    }

    cbD->resetCachedState();
}

double QRhiVulkan::lastCompletedGpuTime(QRhiCommandBuffer *cb)
{
    QVkCommandBuffer *cbD = QRHI_RES(QVkCommandBuffer, cb);
    return cbD->lastGpuTime;
}

void QRhiVulkan::setObjectName(uint64_t object, VkObjectType type, const QByteArray &name, int slot)
{
#ifdef VK_EXT_debug_utils
    if (!debugMarkers || !caps.debugUtils || name.isEmpty())
        return;

    VkDebugUtilsObjectNameInfoEXT nameInfo = {};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = type;
    nameInfo.objectHandle = object;
    QByteArray decoratedName = name;
    if (slot >= 0) {
        decoratedName += '/';
        decoratedName += QByteArray::number(slot);
    }
    nameInfo.pObjectName = decoratedName.constData();
    vkSetDebugUtilsObjectNameEXT(dev, &nameInfo);
#else
    Q_UNUSED(object);
    Q_UNUSED(type);
    Q_UNUSED(name);
    Q_UNUSED(slot);
#endif
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
        Q_UNREACHABLE_RETURN(VK_FILTER_NEAREST);
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
        Q_UNREACHABLE_RETURN(VK_SAMPLER_MIPMAP_MODE_NEAREST);
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
        Q_UNREACHABLE_RETURN(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    }
}

static inline VkShaderStageFlagBits toVkShaderStage(QRhiShaderStage::Type type)
{
    switch (type) {
    case QRhiShaderStage::Vertex:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case QRhiShaderStage::TessellationControl:
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case QRhiShaderStage::TessellationEvaluation:
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case QRhiShaderStage::Fragment:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case QRhiShaderStage::Compute:
        return VK_SHADER_STAGE_COMPUTE_BIT;
    case QRhiShaderStage::Geometry:
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    default:
        Q_UNREACHABLE_RETURN(VK_SHADER_STAGE_VERTEX_BIT);
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
    case QRhiVertexInputAttribute::UInt4:
        return VK_FORMAT_R32G32B32A32_UINT;
    case QRhiVertexInputAttribute::UInt3:
        return VK_FORMAT_R32G32B32_UINT;
    case QRhiVertexInputAttribute::UInt2:
        return VK_FORMAT_R32G32_UINT;
    case QRhiVertexInputAttribute::UInt:
        return VK_FORMAT_R32_UINT;
    case QRhiVertexInputAttribute::SInt4:
        return VK_FORMAT_R32G32B32A32_SINT;
    case QRhiVertexInputAttribute::SInt3:
        return VK_FORMAT_R32G32B32_SINT;
    case QRhiVertexInputAttribute::SInt2:
        return VK_FORMAT_R32G32_SINT;
    case QRhiVertexInputAttribute::SInt:
        return VK_FORMAT_R32_SINT;
    case QRhiVertexInputAttribute::Half4:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case QRhiVertexInputAttribute::Half3:
        return VK_FORMAT_R16G16B16_SFLOAT;
    case QRhiVertexInputAttribute::Half2:
        return VK_FORMAT_R16G16_SFLOAT;
    case QRhiVertexInputAttribute::Half:
        return VK_FORMAT_R16_SFLOAT;
    default:
        Q_UNREACHABLE_RETURN(VK_FORMAT_R32G32B32A32_SFLOAT);
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
    case QRhiGraphicsPipeline::Patches:
        return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
    default:
        Q_UNREACHABLE_RETURN(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
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
        Q_UNREACHABLE_RETURN(VK_CULL_MODE_NONE);
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
        Q_UNREACHABLE_RETURN(VK_FRONT_FACE_COUNTER_CLOCKWISE);
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
        Q_UNREACHABLE_RETURN(VK_BLEND_FACTOR_ZERO);
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
        Q_UNREACHABLE_RETURN(VK_BLEND_OP_ADD);
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
        Q_UNREACHABLE_RETURN(VK_COMPARE_OP_ALWAYS);
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
        Q_UNREACHABLE_RETURN(VK_STENCIL_OP_KEEP);
    }
}

static inline VkPolygonMode toVkPolygonMode(QRhiGraphicsPipeline::PolygonMode mode)
{
    switch (mode) {
    case QRhiGraphicsPipeline::Fill:
        return VK_POLYGON_MODE_FILL;
    case QRhiGraphicsPipeline::Line:
        return VK_POLYGON_MODE_LINE;
    default:
        Q_UNREACHABLE_RETURN(VK_POLYGON_MODE_FILL);
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

    case QRhiShaderResourceBinding::Texture:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

    case QRhiShaderResourceBinding::Sampler:
        return VK_DESCRIPTOR_TYPE_SAMPLER;

    case QRhiShaderResourceBinding::ImageLoad:
    case QRhiShaderResourceBinding::ImageStore:
    case QRhiShaderResourceBinding::ImageLoadStore:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

    case QRhiShaderResourceBinding::BufferLoad:
    case QRhiShaderResourceBinding::BufferStore:
    case QRhiShaderResourceBinding::BufferLoadStore:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    default:
        Q_UNREACHABLE_RETURN(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
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
    if (stage.testFlag(QRhiShaderResourceBinding::TessellationControlStage))
        s |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    if (stage.testFlag(QRhiShaderResourceBinding::TessellationEvaluationStage))
        s |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    if (stage.testFlag(QRhiShaderResourceBinding::GeometryStage))
        s |= VK_SHADER_STAGE_GEOMETRY_BIT;
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
        Q_UNREACHABLE_RETURN(VK_COMPARE_OP_NEVER);
    }
}

QVkBuffer::QVkBuffer(QRhiImplementation *rhi, Type type, UsageFlags usage, quint32 size)
    : QRhiBuffer(rhi, type, usage, size)
{
    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i) {
        buffers[i] = stagingBuffers[i] = VK_NULL_HANDLE;
        allocations[i] = stagingAllocations[i] = nullptr;
    }
}

QVkBuffer::~QVkBuffer()
{
    destroy();
}

void QVkBuffer::destroy()
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
    // destroy() implementations, unlike other functions, are expected to test
    // for m_rhi being null, to allow surviving in case one attempts to destroy
    // a (leaked) resource after the QRhi.
    if (rhiD) {
        rhiD->releaseQueue.append(e);
        rhiD->unregisterResource(this);
    }
}

bool QVkBuffer::create()
{
    if (buffers[0])
        destroy();

    if (m_usage.testFlag(QRhiBuffer::StorageBuffer) && m_type == Dynamic) {
        qWarning("StorageBuffer cannot be combined with Dynamic");
        return false;
    }

    const quint32 nonZeroSize = m_size <= 0 ? 256 : m_size;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = nonZeroSize;
    bufferInfo.usage = toVkBufferUsage(m_usage);

    VmaAllocationCreateInfo allocInfo = {};

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
            rhiD->setObjectName(uint64_t(buffers[i]), VK_OBJECT_TYPE_BUFFER, m_objectName,
                                m_type == Dynamic ? i : -1);
        }
    }

    if (err != VK_SUCCESS) {
        qWarning("Failed to create buffer of size %u: %d", nonZeroSize, err);
        rhiD->printExtraErrorInfo(err);
        return false;
    }

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

char *QVkBuffer::beginFullDynamicBufferUpdateForCurrentFrame()
{
    // Shortcut the entire buffer update mechanism and allow the client to do
    // the host writes directly to the buffer. This will lead to unexpected
    // results when combined with QRhiResourceUpdateBatch-based updates for the
    // buffer, but provides a fast path for dynamic buffers that have all their
    // content changed in every frame.
    Q_ASSERT(m_type == Dynamic);
    QRHI_RES_RHI(QRhiVulkan);
    Q_ASSERT(rhiD->inFrame);
    const int slot = rhiD->currentFrameSlot;
    void *p = nullptr;
    VmaAllocation a = toVmaAllocation(allocations[slot]);
    VkResult err = vmaMapMemory(toVmaAllocator(rhiD->allocator), a, &p);
    if (err != VK_SUCCESS) {
        qWarning("Failed to map buffer: %d", err);
        return nullptr;
    }
    return static_cast<char *>(p);
}

void QVkBuffer::endFullDynamicBufferUpdateForCurrentFrame()
{
    QRHI_RES_RHI(QRhiVulkan);
    const int slot = rhiD->currentFrameSlot;
    VmaAllocation a = toVmaAllocation(allocations[slot]);
    vmaFlushAllocation(toVmaAllocator(rhiD->allocator), a, 0, m_size);
    vmaUnmapMemory(toVmaAllocator(rhiD->allocator), a);
}

QVkRenderBuffer::QVkRenderBuffer(QRhiImplementation *rhi, Type type, const QSize &pixelSize,
                                 int sampleCount, Flags flags,
                                 QRhiTexture::Format backingFormatHint)
    : QRhiRenderBuffer(rhi, type, pixelSize, sampleCount, flags, backingFormatHint)
{
}

QVkRenderBuffer::~QVkRenderBuffer()
{
    destroy();
    delete backingTexture;
}

void QVkRenderBuffer::destroy()
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
        backingTexture->destroy();
    }

    QRHI_RES_RHI(QRhiVulkan);
    if (rhiD) {
        rhiD->releaseQueue.append(e);
        rhiD->unregisterResource(this);
    }
}

bool QVkRenderBuffer::create()
{
    if (memory || backingTexture)
        destroy();

    if (m_pixelSize.isEmpty())
        return false;

    QRHI_RES_RHI(QRhiVulkan);
    samples = rhiD->effectiveSampleCount(m_sampleCount);

    switch (m_type) {
    case QRhiRenderBuffer::Color:
    {
        if (!backingTexture) {
            backingTexture = QRHI_RES(QVkTexture, rhiD->createTexture(backingFormat(),
                                                                      m_pixelSize,
                                                                      1,
                                                                      0,
                                                                      m_sampleCount,
                                                                      QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
        } else {
            backingTexture->setPixelSize(m_pixelSize);
            backingTexture->setSampleCount(m_sampleCount);
        }
        backingTexture->setName(m_objectName);
        if (!backingTexture->create())
            return false;
        vkformat = backingTexture->vkformat;
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
        rhiD->setObjectName(uint64_t(image), VK_OBJECT_TYPE_IMAGE, m_objectName);
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    lastActiveFrameSlot = -1;
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QRhiTexture::Format QVkRenderBuffer::backingFormat() const
{
    if (m_backingFormatHint != QRhiTexture::UnknownFormat)
        return m_backingFormatHint;
    else
        return m_type == Color ? QRhiTexture::RGBA8 : QRhiTexture::UnknownFormat;
}

QVkTexture::QVkTexture(QRhiImplementation *rhi, Format format, const QSize &pixelSize, int depth,
                       int arraySize, int sampleCount, Flags flags)
    : QRhiTexture(rhi, format, pixelSize, depth, arraySize, sampleCount, flags)
{
    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i) {
        stagingBuffers[i] = VK_NULL_HANDLE;
        stagingAllocations[i] = nullptr;
    }
    for (int i = 0; i < QRhi::MAX_MIP_LEVELS; ++i)
        perLevelImageViews[i] = VK_NULL_HANDLE;
}

QVkTexture::~QVkTexture()
{
    destroy();
}

void QVkTexture::destroy()
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

    for (int i = 0; i < QRhi::MAX_MIP_LEVELS; ++i) {
        e.texture.extraImageViews[i] = perLevelImageViews[i];
        perLevelImageViews[i] = VK_NULL_HANDLE;
    }

    image = VK_NULL_HANDLE;
    imageView = VK_NULL_HANDLE;
    imageAlloc = nullptr;

    QRHI_RES_RHI(QRhiVulkan);
    if (rhiD) {
        rhiD->releaseQueue.append(e);
        rhiD->unregisterResource(this);
    }
}

bool QVkTexture::prepareCreate(QSize *adjustedSize)
{
    if (image)
        destroy();

    QRHI_RES_RHI(QRhiVulkan);
    vkformat = toVkTextureFormat(m_format, m_flags);
    VkFormatProperties props;
    rhiD->f->vkGetPhysicalDeviceFormatProperties(rhiD->physDev, vkformat, &props);
    const bool canSampleOptimal = (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
    if (!canSampleOptimal) {
        qWarning("Texture sampling with optimal tiling for format %d not supported", vkformat);
        return false;
    }

    const bool isCube = m_flags.testFlag(CubeMap);
    const bool isArray = m_flags.testFlag(TextureArray);
    const bool is3D = m_flags.testFlag(ThreeDimensional);
    const bool is1D = m_flags.testFlag(OneDimensional);
    const bool hasMipMaps = m_flags.testFlag(MipMapped);

    const QSize size = is1D ? QSize(qMax(1, m_pixelSize.width()), 1)
                            : (m_pixelSize.isEmpty() ? QSize(1, 1) : m_pixelSize);

    mipLevelCount = uint(hasMipMaps ? rhiD->q->mipLevelsForSize(size) : 1);
    const int maxLevels = QRhi::MAX_MIP_LEVELS;
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
        if (is3D) {
            qWarning("3D texture cannot be multisample");
            return false;
        }
        if (hasMipMaps) {
            qWarning("Multisample texture cannot have mipmaps");
            return false;
        }
    }
    if (isCube && is3D) {
        qWarning("Texture cannot be both cube and 3D");
        return false;
    }
    if (isArray && is3D) {
        qWarning("Texture cannot be both array and 3D");
        return false;
    }
    if (isCube && is1D) {
        qWarning("Texture cannot be both cube and 1D");
        return false;
    }
    if (is1D && is3D) {
        qWarning("Texture cannot be both 1D and 3D");
        return false;
    }
    if (m_depth > 1 && !is3D) {
        qWarning("Texture cannot have a depth of %d when it is not 3D", m_depth);
        return false;
    }
    if (m_arraySize > 0 && !isArray) {
        qWarning("Texture cannot have an array size of %d when it is not an array", m_arraySize);
        return false;
    }
    if (m_arraySize < 1 && isArray) {
        qWarning("Texture is an array but array size is %d", m_arraySize);
        return false;
    }

    usageState.layout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    usageState.access = 0;
    usageState.stage = 0;

    if (adjustedSize)
        *adjustedSize = size;

    return true;
}

bool QVkTexture::finishCreate()
{
    QRHI_RES_RHI(QRhiVulkan);

    const auto aspectMask = aspectMaskForTextureFormat(m_format);
    const bool isCube = m_flags.testFlag(CubeMap);
    const bool isArray = m_flags.testFlag(TextureArray);
    const bool is3D = m_flags.testFlag(ThreeDimensional);
    const bool is1D = m_flags.testFlag(OneDimensional);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = isCube
            ? VK_IMAGE_VIEW_TYPE_CUBE
            : (is3D ? VK_IMAGE_VIEW_TYPE_3D
                    : (is1D ? (isArray ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D)
                            : (isArray ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D)));
    viewInfo.format = vkformat;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.levelCount = mipLevelCount;
    if (isArray && m_arrayRangeStart >= 0 && m_arrayRangeLength >= 0) {
        viewInfo.subresourceRange.baseArrayLayer = uint32_t(m_arrayRangeStart);
        viewInfo.subresourceRange.layerCount = uint32_t(m_arrayRangeLength);
    } else {
        viewInfo.subresourceRange.layerCount = isCube ? 6 : (isArray ? qMax(0, m_arraySize) : 1);
    }

    VkResult err = rhiD->df->vkCreateImageView(rhiD->dev, &viewInfo, nullptr, &imageView);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create image view: %d", err);
        return false;
    }

    lastActiveFrameSlot = -1;
    generation += 1;

    return true;
}

bool QVkTexture::create()
{
    QSize size;
    if (!prepareCreate(&size))
        return false;

    QRHI_RES_RHI(QRhiVulkan);
    const bool isRenderTarget = m_flags.testFlag(QRhiTexture::RenderTarget);
    const bool isDepth = isDepthTextureFormat(m_format);
    const bool isCube = m_flags.testFlag(CubeMap);
    const bool isArray = m_flags.testFlag(TextureArray);
    const bool is3D = m_flags.testFlag(ThreeDimensional);
    const bool is1D = m_flags.testFlag(OneDimensional);

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = 0;
    if (isCube)
        imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    if (is3D && isRenderTarget) {
        // This relies on a Vulkan 1.1 constant. For guaranteed proper behavior
        // this also requires that at run time the VkInstance has at least API 1.1
        // enabled. (though it works as expected with some Vulkan (1.2)
        // implementations regardless of the requested API version, but f.ex. the
        // validation layer complains when using this without enabling >=1.1)
        if (!rhiD->caps.texture3DSliceAs2D)
            qWarning("QRhiVulkan: Rendering to 3D texture slice may not be functional without API 1.1 on the VkInstance");
#ifdef VK_VERSION_1_1
        imageInfo.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
#else
        imageInfo.flags |= 0x00000020;
#endif
    }

    imageInfo.imageType = is1D ? VK_IMAGE_TYPE_1D : is3D ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    imageInfo.format = vkformat;
    imageInfo.extent.width = uint32_t(size.width());
    imageInfo.extent.height = uint32_t(size.height());
    imageInfo.extent.depth = is3D ? qMax(1, m_depth) : 1;
    imageInfo.mipLevels = mipLevelCount;
    imageInfo.arrayLayers = isCube ? 6 : (isArray ? qMax(0, m_arraySize) : 1);
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

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VmaAllocation allocation;
    VkResult err = vmaCreateImage(toVmaAllocator(rhiD->allocator), &imageInfo, &allocInfo, &image, &allocation, nullptr);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create image (with VkImageCreateInfo %ux%u depth %u vkformat 0x%X mips %u layers %u vksamples 0x%X): %d",
                 imageInfo.extent.width, imageInfo.extent.height, imageInfo.extent.depth,
                 int(imageInfo.format),
                 imageInfo.mipLevels,
                 imageInfo.arrayLayers,
                 int(imageInfo.samples),
                 err);
        rhiD->printExtraErrorInfo(err);
        return false;
    }
    imageAlloc = allocation;

    if (!finishCreate())
        return false;

    rhiD->setObjectName(uint64_t(image), VK_OBJECT_TYPE_IMAGE, m_objectName);

    owns = true;
    rhiD->registerResource(this);
    return true;
}

bool QVkTexture::createFrom(QRhiTexture::NativeTexture src)
{
    VkImage img = VkImage(src.object);
    if (img == 0)
        return false;

    if (!prepareCreate())
        return false;

    image = img;

    if (!finishCreate())
        return false;

    usageState.layout = VkImageLayout(src.layout);

    owns = false;
    QRHI_RES_RHI(QRhiVulkan);
    rhiD->registerResource(this);
    return true;
}

QRhiTexture::NativeTexture QVkTexture::nativeTexture()
{
    return {quint64(image), usageState.layout};
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

    const VkImageAspectFlags aspectMask = aspectMaskForTextureFormat(m_format);
    const bool isCube = m_flags.testFlag(CubeMap);
    const bool isArray = m_flags.testFlag(TextureArray);
    const bool is3D = m_flags.testFlag(ThreeDimensional);
    const bool is1D = m_flags.testFlag(OneDimensional);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = isCube
            ? VK_IMAGE_VIEW_TYPE_CUBE
            : (is3D ? VK_IMAGE_VIEW_TYPE_3D
                    : (is1D ? (isArray ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D)
                            : (isArray ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D)));
    viewInfo.format = vkformat;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = uint32_t(level);
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = isCube ? 6 : (isArray ? qMax(0, m_arraySize) : 1);

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
    destroy();
}

void QVkSampler::destroy()
{
    if (!sampler)
        return;

    QRhiVulkan::DeferredReleaseEntry e;
    e.type = QRhiVulkan::DeferredReleaseEntry::Sampler;
    e.lastActiveFrameSlot = lastActiveFrameSlot;

    e.sampler.sampler = sampler;
    sampler = VK_NULL_HANDLE;

    QRHI_RES_RHI(QRhiVulkan);
    if (rhiD) {
        rhiD->releaseQueue.append(e);
        rhiD->unregisterResource(this);
    }
}

bool QVkSampler::create()
{
    if (sampler)
        destroy();

    VkSamplerCreateInfo samplerInfo = {};
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
    serializedFormatData.reserve(32);
}

QVkRenderPassDescriptor::~QVkRenderPassDescriptor()
{
    destroy();
}

void QVkRenderPassDescriptor::destroy()
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
    if (rhiD) {
        rhiD->releaseQueue.append(e);
        rhiD->unregisterResource(this);
    }
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
    if (other == this)
        return true;

    if (!other)
        return false;

    const QVkRenderPassDescriptor *o = QRHI_RES(const QVkRenderPassDescriptor, other);

    if (attDescs.size() != o->attDescs.size())
        return false;
    if (colorRefs.size() != o->colorRefs.size())
        return false;
    if (resolveRefs.size() != o->resolveRefs.size())
        return false;
    if (hasDepthStencil != o->hasDepthStencil)
        return false;

    for (int i = 0, ie = colorRefs.size(); i != ie; ++i) {
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

    for (int i = 0, ie = resolveRefs.size(); i != ie; ++i) {
        const uint32_t attIdx = resolveRefs[i].attachment;
        if (attIdx != o->resolveRefs[i].attachment)
            return false;
        if (attIdx != VK_ATTACHMENT_UNUSED && !attachmentDescriptionEquals(attDescs[attIdx], o->attDescs[attIdx]))
            return false;
    }

    // subpassDeps is not included

    return true;
}

void QVkRenderPassDescriptor::updateSerializedFormat()
{
    serializedFormatData.clear();
    auto p = std::back_inserter(serializedFormatData);

    *p++ = attDescs.size();
    *p++ = colorRefs.size();
    *p++ = resolveRefs.size();
    *p++ = hasDepthStencil;

    auto serializeAttachmentData = [this, &p](uint32_t attIdx) {
        const bool used = attIdx != VK_ATTACHMENT_UNUSED;
        const VkAttachmentDescription *a = used ? &attDescs[attIdx] : nullptr;
        *p++ = used ? a->format : 0;
        *p++ = used ? a->samples : 0;
        *p++ = used ? a->loadOp : 0;
        *p++ = used ? a->storeOp : 0;
        *p++ = used ? a->stencilLoadOp : 0;
        *p++ = used ? a->stencilStoreOp : 0;
        *p++ = used ? a->initialLayout : 0;
        *p++ = used ? a->finalLayout : 0;
    };

    for (int i = 0, ie = colorRefs.size(); i != ie; ++i) {
        const uint32_t attIdx = colorRefs[i].attachment;
        *p++ = attIdx;
        serializeAttachmentData(attIdx);
    }

    if (hasDepthStencil) {
        const uint32_t attIdx = dsRef.attachment;
        *p++ = attIdx;
        serializeAttachmentData(attIdx);
    }

    for (int i = 0, ie = resolveRefs.size(); i != ie; ++i) {
        const uint32_t attIdx = resolveRefs[i].attachment;
        *p++ = attIdx;
        serializeAttachmentData(attIdx);
    }
}

QRhiRenderPassDescriptor *QVkRenderPassDescriptor::newCompatibleRenderPassDescriptor() const
{
    QVkRenderPassDescriptor *rpD = new QVkRenderPassDescriptor(m_rhi);

    rpD->ownsRp = true;
    rpD->attDescs = attDescs;
    rpD->colorRefs = colorRefs;
    rpD->resolveRefs = resolveRefs;
    rpD->subpassDeps = subpassDeps;
    rpD->hasDepthStencil = hasDepthStencil;
    rpD->dsRef = dsRef;

    VkRenderPassCreateInfo rpInfo;
    VkSubpassDescription subpassDesc;
    fillRenderPassCreateInfo(&rpInfo, &subpassDesc, rpD);

    QRHI_RES_RHI(QRhiVulkan);
    VkResult err = rhiD->df->vkCreateRenderPass(rhiD->dev, &rpInfo, nullptr, &rpD->rp);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create renderpass: %d", err);
        delete rpD;
        return nullptr;
    }

    rpD->updateSerializedFormat();
    rhiD->registerResource(rpD);
    return rpD;
}

QVector<quint32> QVkRenderPassDescriptor::serializedFormat() const
{
    return serializedFormatData;
}

const QRhiNativeHandles *QVkRenderPassDescriptor::nativeHandles()
{
    nativeHandlesStruct.renderPass = rp;
    return &nativeHandlesStruct;
}

QVkSwapChainRenderTarget::QVkSwapChainRenderTarget(QRhiImplementation *rhi, QRhiSwapChain *swapchain)
    : QRhiSwapChainRenderTarget(rhi, swapchain)
{
}

QVkSwapChainRenderTarget::~QVkSwapChainRenderTarget()
{
    destroy();
}

void QVkSwapChainRenderTarget::destroy()
{
    // nothing to do here
}

QSize QVkSwapChainRenderTarget::pixelSize() const
{
    return d.pixelSize;
}

float QVkSwapChainRenderTarget::devicePixelRatio() const
{
    return d.dpr;
}

int QVkSwapChainRenderTarget::sampleCount() const
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
    destroy();
}

void QVkTextureRenderTarget::destroy()
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
    if (rhiD) {
        rhiD->releaseQueue.append(e);
        rhiD->unregisterResource(this);
    }
}

QRhiRenderPassDescriptor *QVkTextureRenderTarget::newCompatibleRenderPassDescriptor()
{
    // not yet built so cannot rely on data computed in create()

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
    rp->updateSerializedFormat();
    rhiD->registerResource(rp);
    return rp;
}

bool QVkTextureRenderTarget::create()
{
    if (d.fb)
        destroy();

    Q_ASSERT(m_desc.colorAttachmentCount() > 0 || m_desc.depthTexture());
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
            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = texD->image;
            viewInfo.viewType = texD->flags().testFlag(QRhiTexture::OneDimensional)
                    ? VK_IMAGE_VIEW_TYPE_1D
                    : VK_IMAGE_VIEW_TYPE_2D;
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
                d.pixelSize = rhiD->q->sizeForMipLevel(it->level(), texD->pixelSize());
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

            VkImageViewCreateInfo viewInfo = {};
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

    VkFramebufferCreateInfo fbInfo = {};
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

    QRhiRenderTargetAttachmentTracker::updateResIdList<QVkTexture, QVkRenderBuffer>(m_desc, &d.currentResIdList);

    lastActiveFrameSlot = -1;
    rhiD->registerResource(this);
    return true;
}

QSize QVkTextureRenderTarget::pixelSize() const
{
    if (!QRhiRenderTargetAttachmentTracker::isUpToDate<QVkTexture, QVkRenderBuffer>(m_desc, d.currentResIdList))
        const_cast<QVkTextureRenderTarget *>(this)->create();

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
    destroy();
}

void QVkShaderResourceBindings::destroy()
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
    if (rhiD) {
        rhiD->releaseQueue.append(e);
        rhiD->unregisterResource(this);
    }
}

bool QVkShaderResourceBindings::create()
{
    if (layout)
        destroy();

    QRHI_RES_RHI(QRhiVulkan);
    if (!rhiD->sanityCheckShaderResourceBindings(this))
        return false;

    rhiD->updateLayoutDesc(this);

    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i)
        descSets[i] = VK_NULL_HANDLE;

    sortedBindings.clear();
    std::copy(m_bindings.cbegin(), m_bindings.cend(), std::back_inserter(sortedBindings));
    std::sort(sortedBindings.begin(), sortedBindings.end(), QRhiImplementation::sortedBindingLessThan);

    hasSlottedResource = false;
    hasDynamicOffset = false;
    for (const QRhiShaderResourceBinding &binding : std::as_const(sortedBindings)) {
        const QRhiShaderResourceBinding::Data *b = QRhiImplementation::shaderResourceBindingData(binding);
        if (b->type == QRhiShaderResourceBinding::UniformBuffer && b->u.ubuf.buf) {
            if (QRHI_RES(QVkBuffer, b->u.ubuf.buf)->type() == QRhiBuffer::Dynamic)
                hasSlottedResource = true;
            if (b->u.ubuf.hasDynamicOffset)
                hasDynamicOffset = true;
        }
    }

    QVarLengthArray<VkDescriptorSetLayoutBinding, 4> vkbindings;
    for (const QRhiShaderResourceBinding &binding : std::as_const(sortedBindings)) {
        const QRhiShaderResourceBinding::Data *b = QRhiImplementation::shaderResourceBindingData(binding);
        VkDescriptorSetLayoutBinding vkbinding = {};
        vkbinding.binding = uint32_t(b->binding);
        vkbinding.descriptorType = toVkDescriptorType(b);
        if (b->type == QRhiShaderResourceBinding::SampledTexture || b->type == QRhiShaderResourceBinding::Texture)
            vkbinding.descriptorCount = b->u.stex.count;
        else
            vkbinding.descriptorCount = 1;
        vkbinding.stageFlags = toVkShaderStageFlags(b->stage);
        vkbindings.append(vkbinding);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = uint32_t(vkbindings.size());
    layoutInfo.pBindings = vkbindings.constData();

    VkResult err = rhiD->df->vkCreateDescriptorSetLayout(rhiD->dev, &layoutInfo, nullptr, &layout);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create descriptor set layout: %d", err);
        return false;
    }

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorSetCount = QVK_FRAMES_IN_FLIGHT;
    VkDescriptorSetLayout layouts[QVK_FRAMES_IN_FLIGHT];
    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i)
        layouts[i] = layout;
    allocInfo.pSetLayouts = layouts;
    if (!rhiD->allocateDescriptorSet(&allocInfo, descSets, &poolIndex))
        return false;

    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i) {
        boundResourceData[i].resize(sortedBindings.size());
        for (BoundResourceData &bd : boundResourceData[i])
            memset(&bd, 0, sizeof(BoundResourceData));
    }

    lastActiveFrameSlot = -1;
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

void QVkShaderResourceBindings::updateResources(UpdateFlags flags)
{
    sortedBindings.clear();
    std::copy(m_bindings.cbegin(), m_bindings.cend(), std::back_inserter(sortedBindings));
    if (!flags.testFlag(BindingsAreSorted))
        std::sort(sortedBindings.begin(), sortedBindings.end(), QRhiImplementation::sortedBindingLessThan);

    // Reset the state tracking table too - it can deal with assigning a
    // different QRhiBuffer/Texture/Sampler for a binding point, but it cannot
    // detect changes in the associated data, such as the buffer offset. And
    // just like after a create(), a call to updateResources() may lead to now
    // specifying a different offset for the same QRhiBuffer for a given binding
    // point. The same applies to other type of associated data that is not part
    // of the layout, such as the mip level for a StorageImage. Instead of
    // complicating the checks in setShaderResources(), reset the table here
    // just like we do in create().
    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i) {
        Q_ASSERT(boundResourceData[i].size() == sortedBindings.size());
        for (BoundResourceData &bd : boundResourceData[i])
            memset(&bd, 0, sizeof(BoundResourceData));
    }

    generation += 1;
}

QVkGraphicsPipeline::QVkGraphicsPipeline(QRhiImplementation *rhi)
    : QRhiGraphicsPipeline(rhi)
{
}

QVkGraphicsPipeline::~QVkGraphicsPipeline()
{
    destroy();
}

void QVkGraphicsPipeline::destroy()
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
    if (rhiD) {
        rhiD->releaseQueue.append(e);
        rhiD->unregisterResource(this);
    }
}

bool QVkGraphicsPipeline::create()
{
    if (pipeline)
        destroy();

    QRHI_RES_RHI(QRhiVulkan);
    rhiD->pipelineCreationStart();
    if (!rhiD->sanityCheckGraphicsPipeline(this))
        return false;

    if (!rhiD->ensurePipelineCache())
        return false;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
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

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
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
            VkPipelineShaderStageCreateInfo shaderInfo = {};
            shaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderInfo.stage = toVkShaderStage(shaderStage.type());
            shaderInfo.module = shader;
            shaderInfo.pName = spirv.entryPoint().constData();
            shaderStageCreateInfos.append(shaderInfo);
        }
    }
    pipelineInfo.stageCount = uint32_t(shaderStageCreateInfos.size());
    pipelineInfo.pStages = shaderStageCreateInfos.constData();

    QVarLengthArray<VkVertexInputBindingDescription, 4> vertexBindings;
#ifdef VK_EXT_vertex_attribute_divisor
    QVarLengthArray<VkVertexInputBindingDivisorDescriptionEXT> nonOneStepRates;
#endif
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
#ifdef VK_EXT_vertex_attribute_divisor
            if (rhiD->caps.vertexAttribDivisor) {
                nonOneStepRates.append({ uint32_t(bindingIndex), it->instanceStepRate() });
            } else
#endif
            {
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
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = uint32_t(vertexBindings.size());
    vertexInputInfo.pVertexBindingDescriptions = vertexBindings.constData();
    vertexInputInfo.vertexAttributeDescriptionCount = uint32_t(vertexAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.constData();
#ifdef VK_EXT_vertex_attribute_divisor
    VkPipelineVertexInputDivisorStateCreateInfoEXT divisorInfo = {};
    if (!nonOneStepRates.isEmpty()) {
        divisorInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT;
        divisorInfo.vertexBindingDivisorCount = uint32_t(nonOneStepRates.size());
        divisorInfo.pVertexBindingDivisors = nonOneStepRates.constData();
        vertexInputInfo.pNext = &divisorInfo;
    }
#endif
    pipelineInfo.pVertexInputState = &vertexInputInfo;

    QVarLengthArray<VkDynamicState, 8> dynEnable;
    dynEnable << VK_DYNAMIC_STATE_VIEWPORT;
    dynEnable << VK_DYNAMIC_STATE_SCISSOR; // ignore UsesScissor - Vulkan requires a scissor for the viewport always
    if (m_flags.testFlag(QRhiGraphicsPipeline::UsesBlendConstants))
        dynEnable << VK_DYNAMIC_STATE_BLEND_CONSTANTS;
    if (m_flags.testFlag(QRhiGraphicsPipeline::UsesStencilRef))
        dynEnable << VK_DYNAMIC_STATE_STENCIL_REFERENCE;

    VkPipelineDynamicStateCreateInfo dynamicInfo = {};
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount = uint32_t(dynEnable.size());
    dynamicInfo.pDynamicStates = dynEnable.constData();
    pipelineInfo.pDynamicState = &dynamicInfo;

    VkPipelineViewportStateCreateInfo viewportInfo = {};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = viewportInfo.scissorCount = 1;
    pipelineInfo.pViewportState = &viewportInfo;

    VkPipelineInputAssemblyStateCreateInfo inputAsmInfo = {};
    inputAsmInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAsmInfo.topology = toVkTopology(m_topology);
    inputAsmInfo.primitiveRestartEnable = (m_topology == TriangleStrip || m_topology == LineStrip);
    pipelineInfo.pInputAssemblyState = &inputAsmInfo;

    VkPipelineTessellationStateCreateInfo tessInfo = {};
#ifdef VK_VERSION_1_1
    VkPipelineTessellationDomainOriginStateCreateInfo originInfo = {};
#endif
    if (m_topology == Patches) {
        tessInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        tessInfo.patchControlPoints = uint32_t(qMax(1, m_patchControlPointCount));

        // To be able to use the same tess.evaluation shader with both OpenGL
        // and Vulkan, flip the tessellation domain origin to be lower left.
        // This allows declaring the winding order in the shader to be CCW and
        // still have it working with both APIs. This requires Vulkan 1.1 (or
        // VK_KHR_maintenance2 but don't bother with that).
#ifdef VK_VERSION_1_1
        if (rhiD->caps.apiVersion >= QVersionNumber(1, 1)) {
            originInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO;
            originInfo.domainOrigin = VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT;
            tessInfo.pNext = &originInfo;
        } else {
            qWarning("Proper tessellation support requires Vulkan 1.1 or newer, leaving domain origin unset");
        }
#else
        qWarning("QRhi was built without Vulkan 1.1 headers, this is not sufficient for proper tessellation support");
#endif

        pipelineInfo.pTessellationState = &tessInfo;
    }

    VkPipelineRasterizationStateCreateInfo rastInfo = {};
    rastInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rastInfo.cullMode = toVkCullMode(m_cullMode);
    rastInfo.frontFace = toVkFrontFace(m_frontFace);
    if (m_depthBias != 0 || !qFuzzyIsNull(m_slopeScaledDepthBias)) {
        rastInfo.depthBiasEnable = true;
        rastInfo.depthBiasConstantFactor = float(m_depthBias);
        rastInfo.depthBiasSlopeFactor = m_slopeScaledDepthBias;
    }
    rastInfo.lineWidth = rhiD->caps.wideLines ? m_lineWidth : 1.0f;
    rastInfo.polygonMode = toVkPolygonMode(m_polygonMode);
    pipelineInfo.pRasterizationState = &rastInfo;

    VkPipelineMultisampleStateCreateInfo msInfo = {};
    msInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msInfo.rasterizationSamples = rhiD->effectiveSampleCount(m_sampleCount);
    pipelineInfo.pMultisampleState = &msInfo;

    VkPipelineDepthStencilStateCreateInfo dsInfo = {};
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

    VkPipelineColorBlendStateCreateInfo blendInfo = {};
    blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    QVarLengthArray<VkPipelineColorBlendAttachmentState, 4> vktargetBlends;
    for (const QRhiGraphicsPipeline::TargetBlend &b : std::as_const(m_targetBlends)) {
        VkPipelineColorBlendAttachmentState blend = {};
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
        VkPipelineColorBlendAttachmentState blend = {};
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        vktargetBlends.append(blend);
    }
    blendInfo.attachmentCount = uint32_t(vktargetBlends.size());
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

    rhiD->pipelineCreationEnd();
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
    destroy();
}

void QVkComputePipeline::destroy()
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
    if (rhiD) {
        rhiD->releaseQueue.append(e);
        rhiD->unregisterResource(this);
    }
}

bool QVkComputePipeline::create()
{
    if (pipeline)
        destroy();

    QRHI_RES_RHI(QRhiVulkan);
    rhiD->pipelineCreationStart();
    if (!rhiD->ensurePipelineCache())
        return false;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
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

    VkComputePipelineCreateInfo pipelineInfo = {};
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
    VkPipelineShaderStageCreateInfo shaderInfo = {};
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

    rhiD->pipelineCreationEnd();
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
    destroy();
}

void QVkCommandBuffer::destroy()
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

    if (recordingPass == QVkCommandBuffer::NoPass) {
        nativeHandlesStruct.commandBuffer = cb;
    } else {
        if (passUsesSecondaryCb && !activeSecondaryCbStack.isEmpty())
            nativeHandlesStruct.commandBuffer = activeSecondaryCbStack.last();
        else
            nativeHandlesStruct.commandBuffer = cb;
    }

    return &nativeHandlesStruct;
}

QVkSwapChain::QVkSwapChain(QRhiImplementation *rhi)
    : QRhiSwapChain(rhi),
      rtWrapper(rhi, this),
      cbWrapper(rhi)
{
}

QVkSwapChain::~QVkSwapChain()
{
    destroy();
}

void QVkSwapChain::destroy()
{
    if (sc == VK_NULL_HANDLE)
        return;

    QRHI_RES_RHI(QRhiVulkan);
    if (rhiD) {
        rhiD->swapchains.remove(this);
        rhiD->releaseSwapChainResources(this);
    }

    for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i) {
        QVkSwapChain::FrameResources &frame(frameRes[i]);
        frame.cmdBuf = VK_NULL_HANDLE;
        frame.timestampQueryIndex = -1;
    }

    surface = lastConnectedSurface = VK_NULL_HANDLE;

    if (rhiD)
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
    VkSurfaceCapabilitiesKHR surfaceCaps = {};
    QRHI_RES_RHI(QRhiVulkan);
    rhiD->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(rhiD->physDev, surface, &surfaceCaps);
    VkExtent2D bufferSize = surfaceCaps.currentExtent;
    if (bufferSize.width == uint32_t(-1)) {
        Q_ASSERT(bufferSize.height == uint32_t(-1));
        return m_window->size() * m_window->devicePixelRatio();
    }
    return QSize(int(bufferSize.width), int(bufferSize.height));
}

static inline bool hdrFormatMatchesVkSurfaceFormat(QRhiSwapChain::Format f, const VkSurfaceFormatKHR &s)
{
    switch (f) {
    case QRhiSwapChain::HDRExtendedSrgbLinear:
        return s.format == VK_FORMAT_R16G16B16A16_SFLOAT
                && s.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;
    case QRhiSwapChain::HDR10:
        return (s.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 || s.format == VK_FORMAT_A2R10G10B10_UNORM_PACK32)
                && s.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT;
    default:
        break;
    }
    return false;
}

bool QVkSwapChain::isFormatSupported(Format f)
{
    if (f == SDR)
        return true;

    if (!m_window) {
        qWarning("Attempted to call isFormatSupported() without a window set");
        return false;
    }

    // we may be called before create so query the surface
    VkSurfaceKHR surf = QVulkanInstance::surfaceForWindow(m_window);

    QRHI_RES_RHI(QRhiVulkan);
    uint32_t formatCount = 0;
    rhiD->vkGetPhysicalDeviceSurfaceFormatsKHR(rhiD->physDev, surf, &formatCount, nullptr);
    QVarLengthArray<VkSurfaceFormatKHR, 8> formats(formatCount);
    if (formatCount) {
        rhiD->vkGetPhysicalDeviceSurfaceFormatsKHR(rhiD->physDev, surf, &formatCount, formats.data());
        for (uint32_t i = 0; i < formatCount; ++i) {
            if (hdrFormatMatchesVkSurfaceFormat(f, formats[i]))
                return true;
        }
    }

    return false;
}

QRhiRenderPassDescriptor *QVkSwapChain::newCompatibleRenderPassDescriptor()
{
    // not yet built so cannot rely on data computed in createOrResize()

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
    rp->updateSerializedFormat();
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
    if (!rhiD->inst->supportsPresent(rhiD->physDev, rhiD->gfxQueueFamilyIdx, m_window)) {
        qWarning("Presenting not supported on this window");
        return false;
    }

    quint32 formatCount = 0;
    rhiD->vkGetPhysicalDeviceSurfaceFormatsKHR(rhiD->physDev, surface, &formatCount, nullptr);
    QList<VkSurfaceFormatKHR> formats(formatCount);
    if (formatCount)
        rhiD->vkGetPhysicalDeviceSurfaceFormatsKHR(rhiD->physDev, surface, &formatCount, formats.data());

    // See if there is a better match than the default BGRA8 format. (but if
    // not, we will stick to the default)
    const bool srgbRequested = m_flags.testFlag(sRGB);
    for (int i = 0; i < int(formatCount); ++i) {
        if (formats[i].format != VK_FORMAT_UNDEFINED) {
            bool ok = srgbRequested == isSrgbFormat(formats[i].format);
            if (m_format != SDR)
                ok &= hdrFormatMatchesVkSurfaceFormat(m_format, formats[i]);
            if (ok) {
                colorFormat = formats[i].format;
                colorSpace = formats[i].colorSpace;
                break;
            }
        }
    }

    samples = rhiD->effectiveSampleCount(m_sampleCount);

    quint32 presModeCount = 0;
    rhiD->vkGetPhysicalDeviceSurfacePresentModesKHR(rhiD->physDev, surface, &presModeCount, nullptr);
    supportedPresentationModes.resize(presModeCount);
    rhiD->vkGetPhysicalDeviceSurfacePresentModesKHR(rhiD->physDev, surface, &presModeCount,
                                                    supportedPresentationModes.data());

    return true;
}

bool QVkSwapChain::createOrResize()
{
    QRHI_RES_RHI(QRhiVulkan);
    const bool needsRegistration = !window || window != m_window;

    // Can be called multiple times due to window resizes - that is not the
    // same as a simple destroy+create (as with other resources). Thus no
    // destroy() here. See recreateSwapChain().

    // except if the window actually changes
    if (window && window != m_window)
        destroy();

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
            if (!m_depthStencil->create())
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

    rtWrapper.setRenderPassDescriptor(m_renderPassDesc); // for the public getter in QRhiRenderTarget
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

        VkFramebufferCreateInfo fbInfo = {};
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

    if (needsRegistration)
        rhiD->registerResource(this);

    return true;
}

QT_END_NAMESPACE
