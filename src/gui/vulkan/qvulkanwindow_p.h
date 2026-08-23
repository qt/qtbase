// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QVULKANWINDOW_P_H
#define QVULKANWINDOW_P_H

#include <QtGui/private/qtguiglobal_p.h>

#if (QT_CONFIG(vulkan) && __has_include(<vulkan/vulkan.h>)) || defined(Q_QDOC)

#include "qvulkanwindow.h"
#include <QtCore/QHash>
#include <private/qwindow_p.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QVulkanWindowPrivate : public QWindowPrivate
{
    Q_DECLARE_PUBLIC(QVulkanWindow)

public:
    ~QVulkanWindowPrivate();

    void ensureStarted();
    void init();
    void reset();
    bool createDefaultRenderPass();
    QSize surfacePixelSize() const;
    void recreateSwapChain();
    uint32_t chooseTransientImageMemType(VkImage img, uint32_t startIndex);
    bool createTransientImage(VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspectMask,
                              VkImage *images, VkDeviceMemory *mem, VkImageView *views, int count);
    bool createSwapChainResources();
    void destroySwapChainResources();
    void releaseSwapChain();
    void beginFrame();
    void endFrame();
    bool checkDeviceLost(VkResult err);
    bool addReadback();
    void releaseReadbackResources();
    void finishBlockingReadback();

    enum Status {
        StatusUninitialized,
        StatusFail,
        StatusFailRetry,
        StatusDeviceReady,
        StatusReady
    };
    Status status = StatusUninitialized;
    QVulkanWindowRenderer *renderer = nullptr;
    QVulkanInstance *inst = nullptr;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    int physDevIndex = 0;
    QList<VkPhysicalDevice> physDevs;
    QList<VkPhysicalDeviceProperties> physDevProps;
    QVulkanWindow::Flags flags;
    QByteArrayList requestedDevExtensions;
    QHash<VkPhysicalDevice, QVulkanInfoVector<QVulkanExtension> > supportedDevExtensions;
    QList<VkFormat> requestedColorFormats;
    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
    QVulkanWindow::QueueCreateInfoModifier queueCreateInfoModifier;
    QVulkanWindow::EnabledFeaturesModifier enabledFeaturesModifier;
    QVulkanWindow::EnabledFeatures2Modifier enabledFeatures2Modifier;

    VkDevice dev = VK_NULL_HANDLE;
    QVulkanDeviceFunctions *devFuncs = nullptr;
    uint32_t gfxQueueFamilyIdx = uint32_t(-1);
    uint32_t presQueueFamilyIdx = uint32_t(-1);
    VkQueue gfxQueue = VK_NULL_HANDLE;
    VkQueue presQueue = VK_NULL_HANDLE;
    VkCommandPool cmdPool = VK_NULL_HANDLE;
    VkCommandPool presCmdPool = VK_NULL_HANDLE;
    uint32_t hostVisibleMemIndex = 0;
    uint32_t deviceLocalMemIndex = 0;
    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkFormat dsFormat = VK_FORMAT_D24_UNORM_S8_UINT;

    PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = nullptr;
    PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR = nullptr;
    PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = nullptr;
    PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR = nullptr;
    PFN_vkQueuePresentKHR vkQueuePresentKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR = nullptr;

    static const int MAX_SWAPCHAIN_BUFFER_COUNT = 4;
    static const int MAX_FRAME_LAG = QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT;
    // QVulkanWindow only supports the always available FIFO mode. The
    // rendering thread will get throttled to the presentation rate (vsync).
    // This is in effect Example 5 from the VK_KHR_swapchain spec.
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    int swapChainBufferCount = 0;
    int frameLag = 2;

    QSize swapChainImageSize;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    bool swapChainSupportsReadBack = false;

    struct ImageResources {
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkFramebuffer fb = VK_NULL_HANDLE;
        VkCommandBuffer presTransCmdBuf = VK_NULL_HANDLE;
        VkImage msaaImage = VK_NULL_HANDLE;
        VkImageView msaaImageView = VK_NULL_HANDLE;
    } imageRes[MAX_SWAPCHAIN_BUFFER_COUNT];

    VkDeviceMemory msaaImageMem = VK_NULL_HANDLE;

    uint32_t currentImage = 0;

    struct FrameResources {
        VkSemaphore imageSem = VK_NULL_HANDLE;
        VkSemaphore drawSem = VK_NULL_HANDLE;
        VkSemaphore presTransSem = VK_NULL_HANDLE;
        VkFence cmdFence = VK_NULL_HANDLE;
        VkCommandBuffer cmdBuf = VK_NULL_HANDLE;
        bool imageAcquired = false;
        bool imageSemWaitable = false;
        bool cmdFenceWaitable = false;
    } frameRes[MAX_FRAME_LAG];

    uint32_t currentFrame = 0;

    VkRenderPass defaultRenderPass = VK_NULL_HANDLE;

    VkDeviceMemory dsMem = VK_NULL_HANDLE;
    VkImage dsImage = VK_NULL_HANDLE;
    VkImageView dsView = VK_NULL_HANDLE;

    bool framePending = false;
    bool frameGrabbing = false;
    QImage frameGrabTargetImage;
    VkImage frameGrabImage = VK_NULL_HANDLE;
    VkDeviceMemory frameGrabImageMem = VK_NULL_HANDLE;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(vulkan) && __has_include(<vulkan/vulkan.h>)

#endif
