// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef HELLOVULKANTEXTURE_H
#define HELLOVULKANTEXTURE_H

#include <QVulkanWindow>
#include <QImage>

class VulkanRenderer : public QVulkanWindowRenderer
{
public:
    VulkanRenderer(QVulkanWindow *w);

    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;

    void startNextFrame() override;

private:
    VkShaderModule createShader(const QString &name);
    bool createTexture(const QString &name);
    bool createTextureImage(const QSize &size, VkImage *image, VkDeviceMemory *mem,
                            VkImageTiling tiling, VkImageUsageFlags usage, uint32_t memIndex);
    bool writeLinearImage(const QImage &img, VkImage image, VkDeviceMemory memory);
    void ensureTexture();

    QVulkanWindow *m_window;
    QVulkanDeviceFunctions *m_devFuncs;

    VkDeviceMemory m_bufMem = VK_NULL_HANDLE;
    VkBuffer m_buf = VK_NULL_HANDLE;
    VkDescriptorBufferInfo m_uniformBufInfo[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkDescriptorPool m_descPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descSet[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    VkSampler m_sampler = VK_NULL_HANDLE;
    VkImage m_texImage = VK_NULL_HANDLE;
    VkDeviceMemory m_texMem = VK_NULL_HANDLE;
    bool m_texLayoutPending = false;
    VkImageView m_texView = VK_NULL_HANDLE;
    VkImage m_texStaging = VK_NULL_HANDLE;
    VkDeviceMemory m_texStagingMem = VK_NULL_HANDLE;
    bool m_texStagingPending = false;
    QSize m_texSize;
    VkFormat m_texFormat;

    QMatrix4x4 m_proj;
    float m_rotation = 0.0f;
};

class VulkanWindow : public QVulkanWindow
{
public:
    QVulkanWindowRenderer *createRenderer() override;
};

#endif // HELLOVULKANTEXTURE_H
