/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QGuiApplication>
#include <QVulkanInstance>
#include <QVulkanFunctions>
#include <QWindow>
#include <QLoggingCategory>
#include <qevent.h>

static const int SWAPCHAIN_BUFFER_COUNT = 2;
static const int FRAME_LAG = 2;

class VWindow : public QWindow
{
public:
    VWindow() { setSurfaceType(VulkanSurface); }
    ~VWindow() { releaseResources(); }

private:
    void exposeEvent(QExposeEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    bool event(QEvent *) override;

    void init();
    void releaseResources();
    void recreateSwapChain();
    void createDefaultRenderPass();
    void releaseSwapChain();
    void render();
    void buildDrawCalls();

    bool m_inited = false;
    VkSurfaceKHR m_vkSurface;
    VkPhysicalDevice m_vkPhysDev;
    VkPhysicalDeviceProperties m_physDevProps;
    VkDevice m_vkDev = 0;
    QVulkanDeviceFunctions *m_devFuncs;
    VkQueue m_vkGfxQueue;
    VkQueue m_vkPresQueue;
    VkCommandPool m_vkCmdPool = 0;

    PFN_vkCreateSwapchainKHR m_vkCreateSwapchainKHR = nullptr;
    PFN_vkDestroySwapchainKHR m_vkDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR m_vkGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR m_vkAcquireNextImageKHR;
    PFN_vkQueuePresentKHR m_vkQueuePresentKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR m_vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR m_vkGetPhysicalDeviceSurfaceFormatsKHR;

    QSize m_swapChainImageSize;
    VkFormat m_colorFormat;
    VkSwapchainKHR m_swapChain = 0;
    uint32_t m_swapChainBufferCount = 0;

    struct ImageResources {
        VkImage image = 0;
        VkImageView imageView = 0;
        VkCommandBuffer cmdBuf = 0;
        VkFence cmdFence = 0;
        bool cmdFenceWaitable = false;
        VkFramebuffer fb = 0;
    } m_imageRes[SWAPCHAIN_BUFFER_COUNT];

    uint32_t m_currentImage;

    struct FrameResources {
        VkFence fence = 0;
        bool fenceWaitable = false;
        VkSemaphore imageSem = 0;
        VkSemaphore drawSem = 0;
    } m_frameRes[FRAME_LAG];

    uint32_t m_currentFrame;

    VkRenderPass m_defaultRenderPass = 0;
};

void VWindow::exposeEvent(QExposeEvent *)
{
    if (isExposed() && !m_inited) {
        qDebug("initializing");
        m_inited = true;
        init();
        recreateSwapChain();
        render();
    }

    // Release everything when unexposed - the meaning of which is platform specific.
    // Can be essential on mobile, to release resources while in background.
#if 1
    if (!isExposed() && m_inited) {
        m_inited = false;
        releaseSwapChain();
        releaseResources();
    }
#endif
}

void VWindow::resizeEvent(QResizeEvent *)
{
    // Nothing to do here - recreating the swapchain is handled in render(),
    // in fact calling recreateSwapChain() from here leads to problems.
}

bool VWindow::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::UpdateRequest:
        render();
        break;

    // Now the fun part: the swapchain must be destroyed before the surface as per
    // spec. This is not ideal for us because the surface is managed by the
    // QPlatformWindow which may be gone already when the unexpose comes, making the
    // validation layer scream. The solution is to listen to the PlatformSurface events.
    case QEvent::PlatformSurface:
        if (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
            releaseSwapChain();
        break;

    default:
        break;
    }

    return QWindow::event(e);
}

void VWindow::init()
{
    m_vkSurface = QVulkanInstance::surfaceForWindow(this);
    if (!m_vkSurface)
        qFatal("Failed to get surface for window");

    QVulkanInstance *inst = vulkanInstance();
    QVulkanFunctions *f = inst->functions();
    uint32_t devCount = 0;
    f->vkEnumeratePhysicalDevices(inst->vkInstance(), &devCount, nullptr);
    qDebug("%d physical devices", devCount);
    if (!devCount)
        qFatal("No physical devices");

    // Just pick the first physical device for now.
    devCount = 1;
    VkResult err = f->vkEnumeratePhysicalDevices(inst->vkInstance(), &devCount, &m_vkPhysDev);
    if (err != VK_SUCCESS)
        qFatal("Failed to enumerate physical devices: %d", err);

    f->vkGetPhysicalDeviceProperties(m_vkPhysDev, &m_physDevProps);
    qDebug("Device name: %s Driver version: %d.%d.%d", m_physDevProps.deviceName,
           VK_VERSION_MAJOR(m_physDevProps.driverVersion), VK_VERSION_MINOR(m_physDevProps.driverVersion),
           VK_VERSION_PATCH(m_physDevProps.driverVersion));

    uint32_t queueCount = 0;
    f->vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysDev, &queueCount, nullptr);
    QVector<VkQueueFamilyProperties> queueFamilyProps(queueCount);
    f->vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysDev, &queueCount, queueFamilyProps.data());
    int gfxQueueFamilyIdx = -1;
    int presQueueFamilyIdx = -1;
    // First look for a queue that supports both.
    for (int i = 0; i < queueFamilyProps.count(); ++i) {
        qDebug("queue family %d: flags=0x%x count=%d", i, queueFamilyProps[i].queueFlags, queueFamilyProps[i].queueCount);
        if (gfxQueueFamilyIdx == -1
                && (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                && inst->supportsPresent(m_vkPhysDev, i, this))
            gfxQueueFamilyIdx = i;
    }
    if (gfxQueueFamilyIdx != -1) {
        presQueueFamilyIdx = gfxQueueFamilyIdx;
    } else {
        // Separate queues then.
        qDebug("No queue with graphics+present; trying separate queues");
        for (int i = 0; i < queueFamilyProps.count(); ++i) {
            if (gfxQueueFamilyIdx == -1 && (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
                gfxQueueFamilyIdx = i;
            if (presQueueFamilyIdx == -1 && inst->supportsPresent(m_vkPhysDev, i, this))
                presQueueFamilyIdx = i;
        }
    }
    if (gfxQueueFamilyIdx == -1)
        qFatal("No graphics queue family found");
    if (presQueueFamilyIdx == -1)
        qFatal("No present queue family found");

    VkDeviceQueueCreateInfo queueInfo[2];
    const float prio[] = { 0 };
    memset(queueInfo, 0, sizeof(queueInfo));
    queueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo[0].queueFamilyIndex = gfxQueueFamilyIdx;
    queueInfo[0].queueCount = 1;
    queueInfo[0].pQueuePriorities = prio;
    if (gfxQueueFamilyIdx != presQueueFamilyIdx) {
        queueInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo[1].queueFamilyIndex = presQueueFamilyIdx;
        queueInfo[1].queueCount = 1;
        queueInfo[1].pQueuePriorities = prio;
    }

    QVector<const char *> devLayers;
    if (inst->layers().contains("VK_LAYER_LUNARG_standard_validation"))
        devLayers.append("VK_LAYER_LUNARG_standard_validation");

    QVector<const char *> devExts;
    devExts.append("VK_KHR_swapchain");

    VkDeviceCreateInfo devInfo;
    memset(&devInfo, 0, sizeof(devInfo));
    devInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    devInfo.queueCreateInfoCount = gfxQueueFamilyIdx == presQueueFamilyIdx ? 1 : 2;
    devInfo.pQueueCreateInfos = queueInfo;
    devInfo.enabledLayerCount = devLayers.count();
    devInfo.ppEnabledLayerNames = devLayers.constData();
    devInfo.enabledExtensionCount = devExts.count();
    devInfo.ppEnabledExtensionNames = devExts.constData();

    err = f->vkCreateDevice(m_vkPhysDev, &devInfo, nullptr, &m_vkDev);
    if (err != VK_SUCCESS)
        qFatal("Failed to create device: %d", err);

    m_devFuncs = inst->deviceFunctions(m_vkDev);

    m_devFuncs->vkGetDeviceQueue(m_vkDev, gfxQueueFamilyIdx, 0, &m_vkGfxQueue);
    if (gfxQueueFamilyIdx == presQueueFamilyIdx)
        m_vkPresQueue = m_vkGfxQueue;
    else
        m_devFuncs->vkGetDeviceQueue(m_vkDev, presQueueFamilyIdx, 0, &m_vkPresQueue);

    VkCommandPoolCreateInfo poolInfo;
    memset(&poolInfo, 0, sizeof(poolInfo));
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = gfxQueueFamilyIdx;
    err = m_devFuncs->vkCreateCommandPool(m_vkDev, &poolInfo, nullptr, &m_vkCmdPool);
    if (err != VK_SUCCESS)
        qFatal("Failed to create command pool: %d", err);

    m_colorFormat = VK_FORMAT_B8G8R8A8_UNORM; // may get changed later when setting up the swapchain
}

void VWindow::releaseResources()
{
    if (!m_vkDev)
        return;

    m_devFuncs->vkDeviceWaitIdle(m_vkDev);

    if (m_vkCmdPool) {
        m_devFuncs->vkDestroyCommandPool(m_vkDev, m_vkCmdPool, nullptr);
        m_vkCmdPool = 0;
    }

    if (m_vkDev) {
        m_devFuncs->vkDestroyDevice(m_vkDev, nullptr);

        // Play nice and notify QVulkanInstance that the QVulkanDeviceFunctions
        // for m_vkDev needs to be invalidated.
        vulkanInstance()->resetDeviceFunctions(m_vkDev);

        m_vkDev = 0;
    }

    m_vkSurface = 0;
}

void VWindow::recreateSwapChain()
{
    m_swapChainImageSize = size();

    if (m_swapChainImageSize.isEmpty())
        return;

    QVulkanInstance *inst = vulkanInstance();
    QVulkanFunctions *f = inst->functions();
    m_devFuncs->vkDeviceWaitIdle(m_vkDev);

    if (!m_vkCreateSwapchainKHR) {
        m_vkGetPhysicalDeviceSurfaceCapabilitiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
                    inst->getInstanceProcAddr("vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
        m_vkGetPhysicalDeviceSurfaceFormatsKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
                    inst->getInstanceProcAddr("vkGetPhysicalDeviceSurfaceFormatsKHR"));
        // note: device-specific functions
        m_vkCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(f->vkGetDeviceProcAddr(m_vkDev, "vkCreateSwapchainKHR"));
        m_vkDestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(f->vkGetDeviceProcAddr(m_vkDev, "vkDestroySwapchainKHR"));
        m_vkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(f->vkGetDeviceProcAddr(m_vkDev, "vkGetSwapchainImagesKHR"));
        m_vkAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(f->vkGetDeviceProcAddr(m_vkDev, "vkAcquireNextImageKHR"));
        m_vkQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(f->vkGetDeviceProcAddr(m_vkDev, "vkQueuePresentKHR"));
    }

    VkColorSpaceKHR colorSpace = VkColorSpaceKHR(0);
    uint32_t formatCount = 0;
    m_vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysDev, m_vkSurface, &formatCount, nullptr);
    if (formatCount) {
        QVector<VkSurfaceFormatKHR> formats(formatCount);
        m_vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysDev, m_vkSurface, &formatCount, formats.data());
        if (formats[0].format != VK_FORMAT_UNDEFINED) {
            m_colorFormat = formats[0].format;
            colorSpace = formats[0].colorSpace;
        }
    }

    VkSurfaceCapabilitiesKHR surfaceCaps;
    m_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vkPhysDev, m_vkSurface, &surfaceCaps);
    uint32_t reqBufferCount = SWAPCHAIN_BUFFER_COUNT;
    if (surfaceCaps.maxImageCount)
        reqBufferCount = qBound(surfaceCaps.minImageCount, reqBufferCount, surfaceCaps.maxImageCount);

    VkExtent2D bufferSize = surfaceCaps.currentExtent;
    if (bufferSize.width == uint32_t(-1))
        bufferSize.width = m_swapChainImageSize.width();
    if (bufferSize.height == uint32_t(-1))
        bufferSize.height = m_swapChainImageSize.height();

    VkSurfaceTransformFlagBitsKHR preTransform =
            (surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
            ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
            : surfaceCaps.currentTransform;

    VkCompositeAlphaFlagBitsKHR compositeAlpha =
            (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
            ? VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
            : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

    VkSwapchainKHR oldSwapChain = m_swapChain;
    VkSwapchainCreateInfoKHR swapChainInfo;
    memset(&swapChainInfo, 0, sizeof(swapChainInfo));
    swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainInfo.surface = m_vkSurface;
    swapChainInfo.minImageCount = reqBufferCount;
    swapChainInfo.imageFormat = m_colorFormat;
    swapChainInfo.imageColorSpace = colorSpace;
    swapChainInfo.imageExtent = bufferSize;
    swapChainInfo.imageArrayLayers = 1;
    swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainInfo.preTransform = preTransform;
    swapChainInfo.compositeAlpha = compositeAlpha;
    swapChainInfo.presentMode = presentMode;
    swapChainInfo.clipped = true;
    swapChainInfo.oldSwapchain = oldSwapChain;

    qDebug("creating new swap chain of %d buffers, size %dx%d", reqBufferCount, bufferSize.width, bufferSize.height);

    VkSwapchainKHR newSwapChain;
    VkResult err = m_vkCreateSwapchainKHR(m_vkDev, &swapChainInfo, nullptr, &newSwapChain);
    if (err != VK_SUCCESS)
        qFatal("Failed to create swap chain: %d", err);

    if (oldSwapChain)
        releaseSwapChain();

    m_swapChain = newSwapChain;

    m_swapChainBufferCount = 0;
    err = m_vkGetSwapchainImagesKHR(m_vkDev, m_swapChain, &m_swapChainBufferCount, nullptr);
    if (err != VK_SUCCESS || m_swapChainBufferCount < 2)
        qFatal("Failed to get swapchain images: %d (count=%d)", err, m_swapChainBufferCount);

    qDebug("actual swap chain buffer count: %d", m_swapChainBufferCount);
    Q_ASSERT(m_swapChainBufferCount <= SWAPCHAIN_BUFFER_COUNT);

    VkImage swapChainImages[SWAPCHAIN_BUFFER_COUNT];
    err = m_vkGetSwapchainImagesKHR(m_vkDev, m_swapChain, &m_swapChainBufferCount, swapChainImages);
    if (err != VK_SUCCESS)
        qFatal("Failed to get swapchain images: %d", err);

    // Now that we know m_colorFormat, create the default renderpass, the framebuffers will need it.
    createDefaultRenderPass();

    VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };

    for (uint32_t i = 0; i < m_swapChainBufferCount; ++i) {
        ImageResources &image(m_imageRes[i]);
        image.image = swapChainImages[i];

        VkImageViewCreateInfo imgViewInfo;
        memset(&imgViewInfo, 0, sizeof(imgViewInfo));
        imgViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imgViewInfo.image = swapChainImages[i];
        imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imgViewInfo.format = m_colorFormat;
        imgViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        imgViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        imgViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        imgViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        imgViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imgViewInfo.subresourceRange.levelCount = imgViewInfo.subresourceRange.layerCount = 1;
        err = m_devFuncs->vkCreateImageView(m_vkDev, &imgViewInfo, nullptr, &image.imageView);
        if (err != VK_SUCCESS)
            qFatal("Failed to create swapchain image view %d: %d", i, err);

        err = m_devFuncs->vkCreateFence(m_vkDev, &fenceInfo, nullptr, &image.cmdFence);
        if (err != VK_SUCCESS)
            qFatal("Failed to create command buffer fence: %d", err);
        image.cmdFenceWaitable = true;

        VkImageView views[1] = { image.imageView };
        VkFramebufferCreateInfo fbInfo;
        memset(&fbInfo, 0, sizeof(fbInfo));
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = m_defaultRenderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = views;
        fbInfo.width = m_swapChainImageSize.width();
        fbInfo.height = m_swapChainImageSize.height();
        fbInfo.layers = 1;
        VkResult err = m_devFuncs->vkCreateFramebuffer(m_vkDev, &fbInfo, nullptr, &image.fb);
        if (err != VK_SUCCESS)
            qFatal("Failed to create framebuffer: %d", err);
    }

    m_currentImage = 0;

    VkSemaphoreCreateInfo semInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
    for (uint32_t i = 0; i < FRAME_LAG; ++i) {
        FrameResources &frame(m_frameRes[i]);
        m_devFuncs->vkCreateFence(m_vkDev, &fenceInfo, nullptr, &frame.fence);
        frame.fenceWaitable = true;
        m_devFuncs->vkCreateSemaphore(m_vkDev, &semInfo, nullptr, &frame.imageSem);
        m_devFuncs->vkCreateSemaphore(m_vkDev, &semInfo, nullptr, &frame.drawSem);
    }

    m_currentFrame = 0;
}

void VWindow::createDefaultRenderPass()
{
    VkAttachmentDescription attDesc[1];
    memset(attDesc, 0, sizeof(attDesc));
    attDesc[0].format = m_colorFormat;
    attDesc[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attDesc[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attDesc[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attDesc[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    attDesc[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    attDesc[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attDesc[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subPassDesc;
    memset(&subPassDesc, 0, sizeof(subPassDesc));
    subPassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subPassDesc.colorAttachmentCount = 1;
    subPassDesc.pColorAttachments = &colorRef;

    VkRenderPassCreateInfo rpInfo;
    memset(&rpInfo, 0, sizeof(rpInfo));
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments = attDesc;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subPassDesc;
    VkResult err = m_devFuncs->vkCreateRenderPass(m_vkDev, &rpInfo, nullptr, &m_defaultRenderPass);
    if (err != VK_SUCCESS)
        qFatal("Failed to create renderpass: %d", err);
}

void VWindow::releaseSwapChain()
{
    if (!m_vkDev)
        return;

    m_devFuncs->vkDeviceWaitIdle(m_vkDev);

    if (m_defaultRenderPass) {
        m_devFuncs->vkDestroyRenderPass(m_vkDev, m_defaultRenderPass, nullptr);
        m_defaultRenderPass = 0;
    }

    for (uint32_t i = 0; i < FRAME_LAG; ++i) {
        FrameResources &frame(m_frameRes[i]);
        if (frame.fence) {
            if (frame.fenceWaitable)
                m_devFuncs->vkWaitForFences(m_vkDev, 1, &frame.fence, VK_TRUE, UINT64_MAX);
            m_devFuncs->vkDestroyFence(m_vkDev, frame.fence, nullptr);
            frame.fence = 0;
            frame.fenceWaitable = false;
        }
        if (frame.imageSem) {
            m_devFuncs->vkDestroySemaphore(m_vkDev, frame.imageSem, nullptr);
            frame.imageSem = 0;
        }
        if (frame.drawSem) {
            m_devFuncs->vkDestroySemaphore(m_vkDev, frame.drawSem, nullptr);
            frame.drawSem = 0;
        }
    }

    for (uint32_t i = 0; i < m_swapChainBufferCount; ++i) {
        ImageResources &image(m_imageRes[i]);
        if (image.cmdFence) {
            if (image.cmdFenceWaitable)
                m_devFuncs->vkWaitForFences(m_vkDev, 1, &image.cmdFence, VK_TRUE, UINT64_MAX);
            m_devFuncs->vkDestroyFence(m_vkDev, image.cmdFence, nullptr);
            image.cmdFence = 0;
            image.cmdFenceWaitable = false;
        }
        if (image.fb) {
            m_devFuncs->vkDestroyFramebuffer(m_vkDev, image.fb, nullptr);
            image.fb = 0;
        }
        if (image.imageView) {
            m_devFuncs->vkDestroyImageView(m_vkDev, image.imageView, nullptr);
            image.imageView = 0;
        }
        if (image.cmdBuf) {
            m_devFuncs->vkFreeCommandBuffers(m_vkDev, m_vkCmdPool, 1, &image.cmdBuf);
            image.cmdBuf = 0;
        }
    }

    if (m_swapChain) {
        m_vkDestroySwapchainKHR(m_vkDev, m_swapChain, nullptr);
        m_swapChain = 0;
    }
}

void VWindow::render()
{
    if (!m_swapChain)
        return;

    if (size() != m_swapChainImageSize) {
        recreateSwapChain();
        if (!m_swapChain)
            return;
    }

    FrameResources &frame(m_frameRes[m_currentFrame]);

    // Wait if we are too far ahead, i.e. the thread gets throttled based on the presentation rate
    // (note that we are using FIFO mode -> vsync)
    if (frame.fenceWaitable) {
        m_devFuncs->vkWaitForFences(m_vkDev, 1, &frame.fence, VK_TRUE, UINT64_MAX);
        m_devFuncs->vkResetFences(m_vkDev, 1, &frame.fence);
    }

    // move on to next swapchain image
    VkResult err = m_vkAcquireNextImageKHR(m_vkDev, m_swapChain, UINT64_MAX,
                                           frame.imageSem, frame.fence, &m_currentImage);
    if (err == VK_SUCCESS || err == VK_SUBOPTIMAL_KHR) {
        frame.fenceWaitable = true;
    } else if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        frame.fenceWaitable = false;
        recreateSwapChain();
        requestUpdate();
        return;
    } else {
        qWarning("Failed to acquire next swapchain image: %d", err);
        frame.fenceWaitable = false;
        requestUpdate();
        return;
    }

    // make sure the previous draw for the same image has finished
    ImageResources &image(m_imageRes[m_currentImage]);
    if (image.cmdFenceWaitable) {
        m_devFuncs->vkWaitForFences(m_vkDev, 1, &image.cmdFence, VK_TRUE, UINT64_MAX);
        m_devFuncs->vkResetFences(m_vkDev, 1, &image.cmdFence);
    }

    // build new draw command buffer
    buildDrawCalls();

    // submit draw calls
    VkSubmitInfo submitInfo;
    memset(&submitInfo, 0, sizeof(submitInfo));
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &image.cmdBuf;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &frame.imageSem;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &frame.drawSem;
    VkPipelineStageFlags psf = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &psf;

    err = m_devFuncs->vkQueueSubmit(m_vkGfxQueue, 1, &submitInfo, image.cmdFence);
    if (err == VK_SUCCESS) {
        image.cmdFenceWaitable = true;
    } else {
        qWarning("Failed to submit to command queue: %d", err);
        image.cmdFenceWaitable = false;
    }

    // queue present
    VkPresentInfoKHR presInfo;
    memset(&presInfo, 0, sizeof(presInfo));
    presInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presInfo.swapchainCount = 1;
    presInfo.pSwapchains = &m_swapChain;
    presInfo.pImageIndices = &m_currentImage;
    presInfo.waitSemaphoreCount = 1;
    presInfo.pWaitSemaphores = &frame.drawSem;

    // we do not currently handle the case when the present queue is separate
    Q_ASSERT(m_vkGfxQueue == m_vkPresQueue);

    err = m_vkQueuePresentKHR(m_vkGfxQueue, &presInfo);
    if (err != VK_SUCCESS) {
        if (err == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            requestUpdate();
            return;
        } else if (err != VK_SUBOPTIMAL_KHR) {
            qWarning("Failed to present: %d", err);
        }
    }

    vulkanInstance()->presentQueued(this);

    m_currentFrame = (m_currentFrame + 1) % FRAME_LAG;
    requestUpdate();
}

void VWindow::buildDrawCalls()
{
    ImageResources &image(m_imageRes[m_currentImage]);

    if (image.cmdBuf) {
        m_devFuncs->vkFreeCommandBuffers(m_vkDev, m_vkCmdPool, 1, &image.cmdBuf);
        image.cmdBuf = 0;
    }

    VkCommandBufferAllocateInfo cmdBufInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, m_vkCmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
    VkResult err = m_devFuncs->vkAllocateCommandBuffers(m_vkDev, &cmdBufInfo, &image.cmdBuf);
    if (err != VK_SUCCESS)
        qFatal("Failed to allocate frame command buffer: %d", err);

    VkCommandBufferBeginInfo cmdBufBeginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr };
    err = m_devFuncs->vkBeginCommandBuffer(image.cmdBuf, &cmdBufBeginInfo);
    if (err != VK_SUCCESS)
        qFatal("Failed to begin frame command buffer: %d", err);

    static float g = 0;
    g += 0.005f;
    if (g > 1.0f)
        g = 0.0f;
    VkClearColorValue clearColor = { 0.0f, g, 0.0f, 1.0f };
    VkClearValue clearValues[1];
    clearValues[0].color = clearColor;

    VkRenderPassBeginInfo rpBeginInfo;
    memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = m_defaultRenderPass;
    rpBeginInfo.framebuffer = image.fb;
    rpBeginInfo.renderArea.extent.width = m_swapChainImageSize.width();
    rpBeginInfo.renderArea.extent.height = m_swapChainImageSize.height();
    rpBeginInfo.clearValueCount = 1;
    rpBeginInfo.pClearValues = clearValues;
    m_devFuncs->vkCmdBeginRenderPass(image.cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    m_devFuncs->vkCmdEndRenderPass(image.cmdBuf);

    err = m_devFuncs->vkEndCommandBuffer(image.cmdBuf);
    if (err != VK_SUCCESS)
        qFatal("Failed to end frame command buffer: %d", err);
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));

    QVulkanInstance inst;
    // Test the early queries for supported layers/exts.
    qDebug() << inst.supportedLayers() << inst.supportedExtensions();

    // Enable validation layer, if supported.
    inst.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");

    bool ok = inst.create();
    qDebug("QVulkanInstance::create() returned %d", ok);
    if (!ok)
        return 1;

    VWindow w;
    w.setVulkanInstance(&inst);
    w.resize(1024, 768);
    w.show();

    return app.exec();
}
