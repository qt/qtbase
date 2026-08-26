// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Minimal example of driving Vulkan directly, with a QWindow and
// QVulkanInstance. Renders (clears) to the swapchain using Vulkan 1.3 dynamic
// rendering, and so requires a Vulkan 1.3 instance and device.

// NOTE: This application enables the Vulkan validation layer, if installed, and
// also some logging from Qt itself, see main(). These are for development and
// testing, not for production use.

#include <QGuiApplication>
#include <QVulkanInstance>
#include <QVulkanFunctions>
#include <QWindow>
#include <QLoggingCategory>
#include <QVarLengthArray>
#include <qevent.h>

static constexpr int FRAMES_IN_FLIGHT = 2;

class VulkanWindow : public QWindow
{
public:
    VulkanWindow() { setSurfaceType(VulkanSurface); }
    ~VulkanWindow() { releaseSwapChain(); releaseDevice(); }

private:
    void exposeEvent(QExposeEvent *) override;
    bool event(QEvent *) override;

    struct ImageResources {
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkSemaphore drawSem = VK_NULL_HANDLE;
    };

    void initDevice();
    void releaseDevice();
    void recreateSwapChain();
    void releaseSwapChain();
    QSize surfacePixelSize();
    void render();
    void recordFrame(VkCommandBuffer cb, const ImageResources &image);

    bool m_initialized = false;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_physDev = VK_NULL_HANDLE;
    uint32_t m_queueFamilyIdx = 0;
    VkDevice m_dev = VK_NULL_HANDLE;
    QVulkanDeviceFunctions *m_devFuncs = nullptr;
    VkQueue m_queue = VK_NULL_HANDLE;
    VkCommandPool m_cmdPool = VK_NULL_HANDLE;

    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR m_vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR m_vkGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkCreateSwapchainKHR m_vkCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR m_vkDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR m_vkGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR m_vkAcquireNextImageKHR;
    PFN_vkQueuePresentKHR m_vkQueuePresentKHR;

    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
    QSize m_swapChainImageSize;
    QVarLengthArray<ImageResources, 4> m_imageRes;

    struct FrameResources {
        VkCommandBuffer cmdBuf = VK_NULL_HANDLE;
        VkFence fence = VK_NULL_HANDLE;
        VkSemaphore imageSem = VK_NULL_HANDLE;
    } m_frameRes[FRAMES_IN_FLIGHT];
    int m_currentFrame = 0;
};

void VulkanWindow::exposeEvent(QExposeEvent *)
{
    if (isExposed()) {
        if (!m_initialized) {
            m_initialized = true;
            initDevice();
            recreateSwapChain();
        }
        requestUpdate();
    }
}

bool VulkanWindow::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::UpdateRequest:
        render();
        break;

    case QEvent::PlatformSurface:
        if (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
            releaseSwapChain();
        break;

    default:
        break;
    }

    return QWindow::event(e);
}

void VulkanWindow::initDevice()
{
    QVulkanInstance *inst = vulkanInstance();
    QVulkanFunctions *f = inst->functions();

    m_surface = QVulkanInstance::surfaceForWindow(this);
    if (!m_surface)
        qFatal("Failed to get surface for window");

    uint32_t devCount = 0;
    f->vkEnumeratePhysicalDevices(inst->vkInstance(), &devCount, nullptr);
    if (!devCount)
        qFatal("No physical devices");
    qDebug("%u physical devices, using the first one", devCount);
    devCount = 1;
    VkResult err = f->vkEnumeratePhysicalDevices(inst->vkInstance(), &devCount, &m_physDev);
    if (err != VK_SUCCESS && err != VK_INCOMPLETE)
        qFatal("Failed to enumerate physical devices: %d", err);

    VkPhysicalDeviceProperties physDevProps;
    f->vkGetPhysicalDeviceProperties(m_physDev, &physDevProps);
    qDebug("Device name: %s Driver version: %d.%d.%d", physDevProps.deviceName,
           VK_VERSION_MAJOR(physDevProps.driverVersion), VK_VERSION_MINOR(physDevProps.driverVersion),
           VK_VERSION_PATCH(physDevProps.driverVersion));
    if (physDevProps.apiVersion < VK_API_VERSION_1_3)
        qFatal("This example requires a Vulkan 1.3 device");

    // Keep it simple and require a single queue family with both graphics and
    // present support, which is what all common implementations offer.
    uint32_t queueCount = 0;
    f->vkGetPhysicalDeviceQueueFamilyProperties(m_physDev, &queueCount, nullptr);
    QVarLengthArray<VkQueueFamilyProperties, 8> queueFamilyProps(queueCount);
    f->vkGetPhysicalDeviceQueueFamilyProperties(m_physDev, &queueCount, queueFamilyProps.data());
    m_queueFamilyIdx = uint32_t(-1);
    for (uint32_t i = 0; i < queueCount; ++i) {
        if ((queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && inst->supportsPresent(m_physDev, i, this)) {
            m_queueFamilyIdx = i;
            break;
        }
    }
    if (m_queueFamilyIdx == uint32_t(-1))
        qFatal("No queue family with graphics and present support");

    VkPhysicalDeviceVulkan13Features features13 = {};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    VkPhysicalDeviceFeatures2 features2 = {};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &features13;
    f->vkGetPhysicalDeviceFeatures2(m_physDev, &features2);
    if (!features13.dynamicRendering)
        qFatal("dynamicRendering is not supported");

    VkPhysicalDeviceVulkan13Features enabledFeatures13 = {};
    enabledFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    enabledFeatures13.dynamicRendering = VK_TRUE;

    const float prio = 0.0f;
    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = m_queueFamilyIdx;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &prio;

    const char *devExts[] = { "VK_KHR_swapchain" };
    VkDeviceCreateInfo devInfo = {};
    devInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    devInfo.pNext = &enabledFeatures13;
    devInfo.queueCreateInfoCount = 1;
    devInfo.pQueueCreateInfos = &queueInfo;
    devInfo.enabledExtensionCount = 1;
    devInfo.ppEnabledExtensionNames = devExts;
    err = f->vkCreateDevice(m_physDev, &devInfo, nullptr, &m_dev);
    if (err != VK_SUCCESS)
        qFatal("Failed to create device: %d", err);

    m_devFuncs = inst->deviceFunctions(m_dev);
    m_devFuncs->vkGetDeviceQueue(m_dev, m_queueFamilyIdx, 0, &m_queue);

    m_vkGetPhysicalDeviceSurfaceCapabilitiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
                inst->getInstanceProcAddr("vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    m_vkGetPhysicalDeviceSurfaceFormatsKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
                inst->getInstanceProcAddr("vkGetPhysicalDeviceSurfaceFormatsKHR"));
    m_vkCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(f->vkGetDeviceProcAddr(m_dev, "vkCreateSwapchainKHR"));
    m_vkDestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(f->vkGetDeviceProcAddr(m_dev, "vkDestroySwapchainKHR"));
    m_vkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(f->vkGetDeviceProcAddr(m_dev, "vkGetSwapchainImagesKHR"));
    m_vkAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(f->vkGetDeviceProcAddr(m_dev, "vkAcquireNextImageKHR"));
    m_vkQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(f->vkGetDeviceProcAddr(m_dev, "vkQueuePresentKHR"));

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_queueFamilyIdx;
    err = m_devFuncs->vkCreateCommandPool(m_dev, &poolInfo, nullptr, &m_cmdPool);
    if (err != VK_SUCCESS)
        qFatal("Failed to create command pool: %d", err);

    VkCommandBuffer cmdBufs[FRAMES_IN_FLIGHT];
    VkCommandBufferAllocateInfo cmdBufInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,
                                               m_cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, FRAMES_IN_FLIGHT };
    err = m_devFuncs->vkAllocateCommandBuffers(m_dev, &cmdBufInfo, cmdBufs);
    if (err != VK_SUCCESS)
        qFatal("Failed to allocate command buffers: %d", err);

    const VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };
    const VkSemaphoreCreateInfo semInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
    for (int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        m_frameRes[i].cmdBuf = cmdBufs[i];
        m_devFuncs->vkCreateFence(m_dev, &fenceInfo, nullptr, &m_frameRes[i].fence);
        m_devFuncs->vkCreateSemaphore(m_dev, &semInfo, nullptr, &m_frameRes[i].imageSem);
    }
    m_currentFrame = 0;
}

void VulkanWindow::releaseDevice()
{
    if (!m_dev)
        return;

    m_devFuncs->vkDeviceWaitIdle(m_dev);

    for (FrameResources &frame : m_frameRes) {
        m_devFuncs->vkDestroyFence(m_dev, frame.fence, nullptr);
        m_devFuncs->vkDestroySemaphore(m_dev, frame.imageSem, nullptr);
        frame = {};
    }

    m_devFuncs->vkDestroyCommandPool(m_dev, m_cmdPool, nullptr);
    m_cmdPool = VK_NULL_HANDLE;

    m_devFuncs->vkDestroyDevice(m_dev, nullptr);
    vulkanInstance()->resetDeviceFunctions(m_dev);
    m_dev = VK_NULL_HANDLE;
    m_devFuncs = nullptr;
    m_surface = VK_NULL_HANDLE;
}

QSize VulkanWindow::surfacePixelSize()
{
    // The QWindow size is in device independent pixels and may not match the
    // surface exactly, so prefer what the surface reports.
    VkSurfaceCapabilitiesKHR surfaceCaps = {};
    m_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physDev, m_surface, &surfaceCaps);
    if (surfaceCaps.currentExtent.width == uint32_t(-1))
        return size() * devicePixelRatio();
    return QSize(int(surfaceCaps.currentExtent.width), int(surfaceCaps.currentExtent.height));
}

void VulkanWindow::recreateSwapChain()
{
    QVulkanInstance *inst = vulkanInstance();

    const VkSurfaceKHR surface = QVulkanInstance::surfaceForWindow(this);
    if (!surface)
        return;
    if (surface != m_surface) {
        releaseSwapChain();
        m_surface = surface;
        if (!inst->supportsPresent(m_physDev, m_queueFamilyIdx, this))
            qFatal("Present is not supported on the surface");
    }

    const QSize pixelSize = surfacePixelSize();
    if (pixelSize.isEmpty()) {
        releaseSwapChain();
        return;
    }

    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    uint32_t formatCount = 0;
    m_vkGetPhysicalDeviceSurfaceFormatsKHR(m_physDev, m_surface, &formatCount, nullptr);
    if (formatCount) {
        QVarLengthArray<VkSurfaceFormatKHR, 8> formats(formatCount);
        m_vkGetPhysicalDeviceSurfaceFormatsKHR(m_physDev, m_surface, &formatCount, formats.data());
        if (formats[0].format != VK_FORMAT_UNDEFINED) {
            colorFormat = formats[0].format;
            colorSpace = formats[0].colorSpace;
            for (const VkSurfaceFormatKHR &format : formats) {
                if (format.format == VK_FORMAT_B8G8R8A8_UNORM) {
                    colorFormat = format.format;
                    colorSpace = format.colorSpace;
                    break;
                }
            }
        }
    }

    VkSurfaceCapabilitiesKHR surfaceCaps;
    m_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physDev, m_surface, &surfaceCaps);
    uint32_t reqBufferCount = qMax(3u, surfaceCaps.minImageCount);
    if (surfaceCaps.maxImageCount)
        reqBufferCount = qMin(reqBufferCount, surfaceCaps.maxImageCount);

    VkSwapchainCreateInfoKHR swapChainInfo = {};
    swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainInfo.surface = m_surface;
    swapChainInfo.minImageCount = reqBufferCount;
    swapChainInfo.imageFormat = colorFormat;
    swapChainInfo.imageColorSpace = colorSpace;
    swapChainInfo.imageExtent = { uint32_t(pixelSize.width()), uint32_t(pixelSize.height()) };
    swapChainInfo.imageArrayLayers = 1;
    swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainInfo.preTransform = (surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
            ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : surfaceCaps.currentTransform;
    swapChainInfo.compositeAlpha = (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
            ? VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapChainInfo.clipped = true;
    swapChainInfo.oldSwapchain = m_swapChain;

    qDebug("creating new swapchain of %u buffers, size %dx%d", reqBufferCount, pixelSize.width(), pixelSize.height());

    VkSwapchainKHR newSwapChain;
    VkResult err = m_vkCreateSwapchainKHR(m_dev, &swapChainInfo, nullptr, &newSwapChain);
    if (err != VK_SUCCESS)
        qFatal("Failed to create swapchain: %d", err);

    releaseSwapChain();
    m_swapChain = newSwapChain;
    m_swapChainImageSize = pixelSize;

    uint32_t bufferCount = 0;
    err = m_vkGetSwapchainImagesKHR(m_dev, m_swapChain, &bufferCount, nullptr);
    if (err != VK_SUCCESS || !bufferCount)
        qFatal("Failed to get swapchain images: %d (count=%u)", err, bufferCount);
    QVarLengthArray<VkImage, 4> images(bufferCount);
    err = m_vkGetSwapchainImagesKHR(m_dev, m_swapChain, &bufferCount, images.data());
    if (err != VK_SUCCESS)
        qFatal("Failed to get swapchain images: %d", err);
    qDebug("actual swapchain buffer count: %u", bufferCount);

    const VkSemaphoreCreateInfo semInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
    m_imageRes.resize(bufferCount);
    for (uint32_t i = 0; i < bufferCount; ++i) {
        ImageResources &image(m_imageRes[i]);
        image.image = images[i];

        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = colorFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = viewInfo.subresourceRange.layerCount = 1;
        err = m_devFuncs->vkCreateImageView(m_dev, &viewInfo, nullptr, &image.view);
        if (err != VK_SUCCESS)
            qFatal("Failed to create swapchain image view %u: %d", i, err);

        m_devFuncs->vkCreateSemaphore(m_dev, &semInfo, nullptr, &image.drawSem);
    }
}

void VulkanWindow::releaseSwapChain()
{
    if (!m_dev || !m_swapChain)
        return;

    m_devFuncs->vkDeviceWaitIdle(m_dev);

    for (const ImageResources &image : m_imageRes) {
        m_devFuncs->vkDestroyImageView(m_dev, image.view, nullptr);
        m_devFuncs->vkDestroySemaphore(m_dev, image.drawSem, nullptr);
    }
    m_imageRes.clear();

    m_vkDestroySwapchainKHR(m_dev, m_swapChain, nullptr);
    m_swapChain = VK_NULL_HANDLE;
    m_swapChainImageSize = QSize();
}

void VulkanWindow::render()
{
    if (!m_initialized || !isExposed())
        return;

    if (!m_swapChain || surfacePixelSize() != m_swapChainImageSize) {
        recreateSwapChain();
        if (!m_swapChain)
            return;
    }

    FrameResources &frame(m_frameRes[m_currentFrame]);

    // Wait until the commands submitted in this frame slot have finished, which
    // throttles to FRAMES_IN_FLIGHT frames (so to the presentation rate, given
    // that FIFO means vsync).
    m_devFuncs->vkWaitForFences(m_dev, 1, &frame.fence, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult err = m_vkAcquireNextImageKHR(m_dev, m_swapChain, UINT64_MAX,
                                           frame.imageSem, VK_NULL_HANDLE, &imageIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        requestUpdate();
        return;
    }
    // VK_SUBOPTIMAL_KHR is a success: an image was acquired and imageSem was signaled.
    if (err != VK_SUCCESS && err != VK_SUBOPTIMAL_KHR)
        qFatal("Failed to acquire next swapchain image: %d", err);

    ImageResources &image(m_imageRes[imageIndex]);
    m_devFuncs->vkResetFences(m_dev, 1, &frame.fence);

    recordFrame(frame.cmdBuf, image);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &frame.imageSem;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frame.cmdBuf;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &image.drawSem;
    err = m_devFuncs->vkQueueSubmit(m_queue, 1, &submitInfo, frame.fence);
    if (err != VK_SUCCESS)
        qFatal("Failed to submit to command queue: %d", err);

    VkPresentInfoKHR presInfo = {};
    presInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presInfo.waitSemaphoreCount = 1;
    presInfo.pWaitSemaphores = &image.drawSem;
    presInfo.swapchainCount = 1;
    presInfo.pSwapchains = &m_swapChain;
    presInfo.pImageIndices = &imageIndex;

    // Platform-specific window manager notifications, essential on Wayland and X11.
    vulkanInstance()->presentAboutToBeQueued(this);

    err = m_vkQueuePresentKHR(m_queue, &presInfo);
    if (err == VK_SUCCESS || err == VK_SUBOPTIMAL_KHR)
        vulkanInstance()->presentQueued(this);

    m_currentFrame = (m_currentFrame + 1) % FRAMES_IN_FLIGHT;

    // The surface can go out of date without the size changing, e.g. when moving
    // to a screen with a different scale factor. VK_SUBOPTIMAL_KHR is not worth
    // acting on, some implementations report it permanently.
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
        recreateSwapChain();
    else if (err != VK_SUCCESS && err != VK_SUBOPTIMAL_KHR)
        qWarning("Failed to present: %d", err);

    requestUpdate();
}

void VulkanWindow::recordFrame(VkCommandBuffer cb, const ImageResources &image)
{
    const VkCommandBufferBeginInfo cmdBufBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr };
    VkResult err = m_devFuncs->vkBeginCommandBuffer(cb, &cmdBufBeginInfo);
    if (err != VK_SUCCESS)
        qFatal("Failed to begin command buffer: %d", err);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = barrier.subresourceRange.layerCount = 1;

    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    m_devFuncs->vkCmdPipelineBarrier(cb,
                                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                     0, 0, nullptr, 0, nullptr, 1, &barrier);

    static float g = 0;
    g += 0.005f;
    if (g > 1.0f)
        g = 0.0f;

    VkRenderingAttachmentInfo colorAtt = {};
    colorAtt.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAtt.imageView = image.view;
    colorAtt.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.clearValue.color = { { 0.0f, g, 0.0f, 1.0f } };

    VkRenderingInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.extent = { uint32_t(m_swapChainImageSize.width()), uint32_t(m_swapChainImageSize.height()) };
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAtt;

    m_devFuncs->vkCmdBeginRendering(cb, &renderingInfo);
    // this is where the drawing would happen
    m_devFuncs->vkCmdEndRendering(cb);

    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = 0;
    m_devFuncs->vkCmdPipelineBarrier(cb,
                                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                     VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                     0, 0, nullptr, 0, nullptr, 1, &barrier);

    err = m_devFuncs->vkEndCommandBuffer(cb);
    if (err != VK_SUCCESS)
        qFatal("Failed to end command buffer: %d", err);
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));

    QVulkanInstance inst;

    if (inst.supportedApiVersion() < QVersionNumber(1, 3))
        qFatal("This example requires Vulkan 1.3");
    inst.setApiVersion(QVersionNumber(1, 3));

    // Enable validation layer, if supported.
    inst.setLayers({ "VK_LAYER_KHRONOS_validation" });

    if (!inst.create())
        qFatal("Failed to create Vulkan instance: %d", inst.errorCode());

    VulkanWindow w;
    w.setVulkanInstance(&inst);
    w.resize(1024, 768);
    w.show();

    return app.exec();
}
