/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "hellovulkantexture.h"
#include <QVulkanFunctions>
#include <QCoreApplication>
#include <QFile>

// Use a triangle strip to get a quad.
//
// Note that the vertex data and the projection matrix assume OpenGL. With
// Vulkan Y is negated in clip space and the near/far plane is at 0/1 instead
// of -1/1. These will be corrected for by an extra transformation when
// calculating the modelview-projection matrix.
static float vertexData[] = { // Y up, front = CW
    // x, y, z, u, v
    -1, -1, 0, 0, 1,
    -1,  1, 0, 0, 0,
     1, -1, 0, 1, 1,
     1,  1, 0, 1, 0
};

static const int UNIFORM_DATA_SIZE = 16 * sizeof(float);

static inline VkDeviceSize aligned(VkDeviceSize v, VkDeviceSize byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

QVulkanWindowRenderer *VulkanWindow::createRenderer()
{
    return new VulkanRenderer(this);
}

VulkanRenderer::VulkanRenderer(QVulkanWindow *w)
    : m_window(w)
{
}

VkShaderModule VulkanRenderer::createShader(const QString &name)
{
    QFile file(name);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Failed to read shader %s", qPrintable(name));
        return VK_NULL_HANDLE;
    }
    QByteArray blob = file.readAll();
    file.close();

    VkShaderModuleCreateInfo shaderInfo;
    memset(&shaderInfo, 0, sizeof(shaderInfo));
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = blob.size();
    shaderInfo.pCode = reinterpret_cast<const uint32_t *>(blob.constData());
    VkShaderModule shaderModule;
    VkResult err = m_devFuncs->vkCreateShaderModule(m_window->device(), &shaderInfo, nullptr, &shaderModule);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create shader module: %d", err);
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

bool VulkanRenderer::createTexture(const QString &name)
{
    QImage img(name);
    if (img.isNull()) {
        qWarning("Failed to load image %s", qPrintable(name));
        return false;
    }

    // Convert to byte ordered RGBA8. Use premultiplied alpha, see pColorBlendState in the pipeline.
    img = img.convertToFormat(QImage::Format_RGBA8888_Premultiplied);

    QVulkanFunctions *f = m_window->vulkanInstance()->functions();
    VkDevice dev = m_window->device();

    const bool srgb = QCoreApplication::arguments().contains(QStringLiteral("--srgb"));
    if (srgb)
        qDebug("sRGB swapchain was requested, making texture sRGB too");

    m_texFormat = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

    // Now we can either map and copy the image data directly, or have to go
    // through a staging buffer to copy and convert into the internal optimal
    // tiling format.
    VkFormatProperties props;
    f->vkGetPhysicalDeviceFormatProperties(m_window->physicalDevice(), m_texFormat, &props);
    const bool canSampleLinear = (props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
    const bool canSampleOptimal = (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
    if (!canSampleLinear && !canSampleOptimal) {
        qWarning("Neither linear nor optimal image sampling is supported for RGBA8");
        return false;
    }

    static bool alwaysStage = qEnvironmentVariableIntValue("QT_VK_FORCE_STAGE_TEX");

    if (canSampleLinear && !alwaysStage) {
        if (!createTextureImage(img.size(), &m_texImage, &m_texMem,
                                VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT,
                                m_window->hostVisibleMemoryIndex()))
            return false;

        if (!writeLinearImage(img, m_texImage, m_texMem))
            return false;

        m_texLayoutPending = true;
    } else {
        if (!createTextureImage(img.size(), &m_texStaging, &m_texStagingMem,
                                VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                m_window->hostVisibleMemoryIndex()))
            return false;

        if (!createTextureImage(img.size(), &m_texImage, &m_texMem,
                                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                m_window->deviceLocalMemoryIndex()))
            return false;

        if (!writeLinearImage(img, m_texStaging, m_texStagingMem))
            return false;

        m_texStagingPending = true;
    }

    VkImageViewCreateInfo viewInfo;
    memset(&viewInfo, 0, sizeof(viewInfo));
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_texImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_texFormat;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = viewInfo.subresourceRange.layerCount = 1;

    VkResult err = m_devFuncs->vkCreateImageView(dev, &viewInfo, nullptr, &m_texView);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create image view for texture: %d", err);
        return false;
    }

    m_texSize = img.size();

    return true;
}

bool VulkanRenderer::createTextureImage(const QSize &size, VkImage *image, VkDeviceMemory *mem,
                                        VkImageTiling tiling, VkImageUsageFlags usage, uint32_t memIndex)
{
    VkDevice dev = m_window->device();

    VkImageCreateInfo imageInfo;
    memset(&imageInfo, 0, sizeof(imageInfo));
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = m_texFormat;
    imageInfo.extent.width = size.width();
    imageInfo.extent.height = size.height();
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = tiling;
    imageInfo.usage = usage;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

    VkResult err = m_devFuncs->vkCreateImage(dev, &imageInfo, nullptr, image);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create linear image for texture: %d", err);
        return false;
    }

    VkMemoryRequirements memReq;
    m_devFuncs->vkGetImageMemoryRequirements(dev, *image, &memReq);

    VkMemoryAllocateInfo allocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memReq.size,
        memIndex
    };
    qDebug("allocating %u bytes for texture image", uint32_t(memReq.size));

    err = m_devFuncs->vkAllocateMemory(dev, &allocInfo, nullptr, mem);
    if (err != VK_SUCCESS) {
        qWarning("Failed to allocate memory for linear image: %d", err);
        return false;
    }

    err = m_devFuncs->vkBindImageMemory(dev, *image, *mem, 0);
    if (err != VK_SUCCESS) {
        qWarning("Failed to bind linear image memory: %d", err);
        return false;
    }

    return true;
}

bool VulkanRenderer::writeLinearImage(const QImage &img, VkImage image, VkDeviceMemory memory)
{
    VkDevice dev = m_window->device();

    VkImageSubresource subres = {
        VK_IMAGE_ASPECT_COLOR_BIT,
        0, // mip level
        0
    };
    VkSubresourceLayout layout;
    m_devFuncs->vkGetImageSubresourceLayout(dev, image, &subres, &layout);

    uchar *p;
    VkResult err = m_devFuncs->vkMapMemory(dev, memory, layout.offset, layout.size, 0, reinterpret_cast<void **>(&p));
    if (err != VK_SUCCESS) {
        qWarning("Failed to map memory for linear image: %d", err);
        return false;
    }

    for (int y = 0; y < img.height(); ++y) {
        const uchar *line = img.constScanLine(y);
        memcpy(p, line, img.width() * 4);
        p += layout.rowPitch;
    }

    m_devFuncs->vkUnmapMemory(dev, memory);
    return true;
}

void VulkanRenderer::ensureTexture()
{
    if (!m_texLayoutPending && !m_texStagingPending)
        return;

    Q_ASSERT(m_texLayoutPending != m_texStagingPending);
    VkCommandBuffer cb = m_window->currentCommandBuffer();

    VkImageMemoryBarrier barrier;
    memset(&barrier, 0, sizeof(barrier));
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = barrier.subresourceRange.layerCount = 1;

    if (m_texLayoutPending) {
        m_texLayoutPending = false;

        barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = 0; // VK_ACCESS_HOST_WRITE_BIT ### no, keep validation layer happy (??)
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.image = m_texImage;

        m_devFuncs->vkCmdPipelineBarrier(cb,
                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                0, 0, nullptr, 0, nullptr,
                                1, &barrier);
    } else {
        m_texStagingPending = false;

        barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.image = m_texStaging;
        m_devFuncs->vkCmdPipelineBarrier(cb,
                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                0, 0, nullptr, 0, nullptr,
                                1, &barrier);

        barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.image = m_texImage;
        m_devFuncs->vkCmdPipelineBarrier(cb,
                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                0, 0, nullptr, 0, nullptr,
                                1, &barrier);

        VkImageCopy copyInfo;
        memset(&copyInfo, 0, sizeof(copyInfo));
        copyInfo.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyInfo.srcSubresource.layerCount = 1;
        copyInfo.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyInfo.dstSubresource.layerCount = 1;
        copyInfo.extent.width = m_texSize.width();
        copyInfo.extent.height = m_texSize.height();
        copyInfo.extent.depth = 1;
        m_devFuncs->vkCmdCopyImage(cb, m_texStaging, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          m_texImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyInfo);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.image = m_texImage;
        m_devFuncs->vkCmdPipelineBarrier(cb,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                0, 0, nullptr, 0, nullptr,
                                1, &barrier);
    }
}

void VulkanRenderer::initResources()
{
    qDebug("initResources");

    VkDevice dev = m_window->device();
    m_devFuncs = m_window->vulkanInstance()->deviceFunctions(dev);

    // The setup is similar to hellovulkantriangle. The difference is the
    // presence of a second vertex attribute (texcoord), a sampler, and that we
    // need blending.

    const int concurrentFrameCount = m_window->concurrentFrameCount();
    const VkPhysicalDeviceLimits *pdevLimits = &m_window->physicalDeviceProperties()->limits;
    const VkDeviceSize uniAlign = pdevLimits->minUniformBufferOffsetAlignment;
    qDebug("uniform buffer offset alignment is %u", (uint) uniAlign);
    VkBufferCreateInfo bufInfo;
    memset(&bufInfo, 0, sizeof(bufInfo));
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    // Our internal layout is vertex, uniform, uniform, ... with each uniform buffer start offset aligned to uniAlign.
    const VkDeviceSize vertexAllocSize = aligned(sizeof(vertexData), uniAlign);
    const VkDeviceSize uniformAllocSize = aligned(UNIFORM_DATA_SIZE, uniAlign);
    bufInfo.size = vertexAllocSize + concurrentFrameCount * uniformAllocSize;
    bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VkResult err = m_devFuncs->vkCreateBuffer(dev, &bufInfo, nullptr, &m_buf);
    if (err != VK_SUCCESS)
        qFatal("Failed to create buffer: %d", err);

    VkMemoryRequirements memReq;
    m_devFuncs->vkGetBufferMemoryRequirements(dev, m_buf, &memReq);

    VkMemoryAllocateInfo memAllocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memReq.size,
        m_window->hostVisibleMemoryIndex()
    };

    err = m_devFuncs->vkAllocateMemory(dev, &memAllocInfo, nullptr, &m_bufMem);
    if (err != VK_SUCCESS)
        qFatal("Failed to allocate memory: %d", err);

    err = m_devFuncs->vkBindBufferMemory(dev, m_buf, m_bufMem, 0);
    if (err != VK_SUCCESS)
        qFatal("Failed to bind buffer memory: %d", err);

    quint8 *p;
    err = m_devFuncs->vkMapMemory(dev, m_bufMem, 0, memReq.size, 0, reinterpret_cast<void **>(&p));
    if (err != VK_SUCCESS)
        qFatal("Failed to map memory: %d", err);
    memcpy(p, vertexData, sizeof(vertexData));
    QMatrix4x4 ident;
    memset(m_uniformBufInfo, 0, sizeof(m_uniformBufInfo));
    for (int i = 0; i < concurrentFrameCount; ++i) {
        const VkDeviceSize offset = vertexAllocSize + i * uniformAllocSize;
        memcpy(p + offset, ident.constData(), 16 * sizeof(float));
        m_uniformBufInfo[i].buffer = m_buf;
        m_uniformBufInfo[i].offset = offset;
        m_uniformBufInfo[i].range = uniformAllocSize;
    }
    m_devFuncs->vkUnmapMemory(dev, m_bufMem);

    VkVertexInputBindingDescription vertexBindingDesc = {
        0, // binding
        5 * sizeof(float),
        VK_VERTEX_INPUT_RATE_VERTEX
    };
    VkVertexInputAttributeDescription vertexAttrDesc[] = {
        { // position
            0, // location
            0, // binding
            VK_FORMAT_R32G32B32_SFLOAT,
            0
        },
        { // texcoord
            1,
            0,
            VK_FORMAT_R32G32_SFLOAT,
            3 * sizeof(float)
        }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pNext = nullptr;
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttrDesc;

    // Sampler.
    VkSamplerCreateInfo samplerInfo;
    memset(&samplerInfo, 0, sizeof(samplerInfo));
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxAnisotropy = 1.0f;
    err = m_devFuncs->vkCreateSampler(dev, &samplerInfo, nullptr, &m_sampler);
    if (err != VK_SUCCESS)
        qFatal("Failed to create sampler: %d", err);

    // Texture.
    if (!createTexture(QStringLiteral(":/qt256.png")))
        qFatal("Failed to create texture");

    // Set up descriptor set and its layout.
    VkDescriptorPoolSize descPoolSizes[2] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uint32_t(concurrentFrameCount) },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, uint32_t(concurrentFrameCount) }
    };
    VkDescriptorPoolCreateInfo descPoolInfo;
    memset(&descPoolInfo, 0, sizeof(descPoolInfo));
    descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descPoolInfo.maxSets = concurrentFrameCount;
    descPoolInfo.poolSizeCount = 2;
    descPoolInfo.pPoolSizes = descPoolSizes;
    err = m_devFuncs->vkCreateDescriptorPool(dev, &descPoolInfo, nullptr, &m_descPool);
    if (err != VK_SUCCESS)
        qFatal("Failed to create descriptor pool: %d", err);

    VkDescriptorSetLayoutBinding layoutBinding[2] =
    {
        {
            0, // binding
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1, // descriptorCount
            VK_SHADER_STAGE_VERTEX_BIT,
            nullptr
        },
        {
            1, // binding
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1, // descriptorCount
            VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        }
    };
    VkDescriptorSetLayoutCreateInfo descLayoutInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        2, // bindingCount
        layoutBinding
    };
    err = m_devFuncs->vkCreateDescriptorSetLayout(dev, &descLayoutInfo, nullptr, &m_descSetLayout);
    if (err != VK_SUCCESS)
        qFatal("Failed to create descriptor set layout: %d", err);

    for (int i = 0; i < concurrentFrameCount; ++i) {
        VkDescriptorSetAllocateInfo descSetAllocInfo = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            nullptr,
            m_descPool,
            1,
            &m_descSetLayout
        };
        err = m_devFuncs->vkAllocateDescriptorSets(dev, &descSetAllocInfo, &m_descSet[i]);
        if (err != VK_SUCCESS)
            qFatal("Failed to allocate descriptor set: %d", err);

        VkWriteDescriptorSet descWrite[2];
        memset(descWrite, 0, sizeof(descWrite));
        descWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descWrite[0].dstSet = m_descSet[i];
        descWrite[0].dstBinding = 0;
        descWrite[0].descriptorCount = 1;
        descWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descWrite[0].pBufferInfo = &m_uniformBufInfo[i];

        VkDescriptorImageInfo descImageInfo = {
            m_sampler,
            m_texView,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        descWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descWrite[1].dstSet = m_descSet[i];
        descWrite[1].dstBinding = 1;
        descWrite[1].descriptorCount = 1;
        descWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descWrite[1].pImageInfo = &descImageInfo;

        m_devFuncs->vkUpdateDescriptorSets(dev, 2, descWrite, 0, nullptr);
    }

    // Pipeline cache
    VkPipelineCacheCreateInfo pipelineCacheInfo;
    memset(&pipelineCacheInfo, 0, sizeof(pipelineCacheInfo));
    pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    err = m_devFuncs->vkCreatePipelineCache(dev, &pipelineCacheInfo, nullptr, &m_pipelineCache);
    if (err != VK_SUCCESS)
        qFatal("Failed to create pipeline cache: %d", err);

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descSetLayout;
    err = m_devFuncs->vkCreatePipelineLayout(dev, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if (err != VK_SUCCESS)
        qFatal("Failed to create pipeline layout: %d", err);

    // Shaders
    VkShaderModule vertShaderModule = createShader(QStringLiteral(":/texture_vert.spv"));
    VkShaderModule fragShaderModule = createShader(QStringLiteral(":/texture_frag.spv"));

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo;
    memset(&pipelineInfo, 0, sizeof(pipelineInfo));
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineShaderStageCreateInfo shaderStages[2] = {
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_VERTEX_BIT,
            vertShaderModule,
            "main",
            nullptr
        },
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            fragShaderModule,
            "main",
            nullptr
        }
    };
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    pipelineInfo.pVertexInputState = &vertexInputInfo;

    VkPipelineInputAssemblyStateCreateInfo ia;
    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    pipelineInfo.pInputAssemblyState = &ia;

    // The viewport and scissor will be set dynamically via vkCmdSetViewport/Scissor.
    // This way the pipeline does not need to be touched when resizing the window.
    VkPipelineViewportStateCreateInfo vp;
    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount = 1;
    pipelineInfo.pViewportState = &vp;

    VkPipelineRasterizationStateCreateInfo rs;
    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_BACK_BIT;
    rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rs.lineWidth = 1.0f;
    pipelineInfo.pRasterizationState = &rs;

    VkPipelineMultisampleStateCreateInfo ms;
    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipelineInfo.pMultisampleState = &ms;

    VkPipelineDepthStencilStateCreateInfo ds;
    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    pipelineInfo.pDepthStencilState = &ds;

    VkPipelineColorBlendStateCreateInfo cb;
    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // assume pre-multiplied alpha, blend, write out all of rgba
    VkPipelineColorBlendAttachmentState att;
    memset(&att, 0, sizeof(att));
    att.colorWriteMask = 0xF;
    att.blendEnable = VK_TRUE;
    att.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    att.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    att.colorBlendOp = VK_BLEND_OP_ADD;
    att.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    att.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    att.alphaBlendOp = VK_BLEND_OP_ADD;
    cb.attachmentCount = 1;
    cb.pAttachments = &att;
    pipelineInfo.pColorBlendState = &cb;

    VkDynamicState dynEnable[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dyn;
    memset(&dyn, 0, sizeof(dyn));
    dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = sizeof(dynEnable) / sizeof(VkDynamicState);
    dyn.pDynamicStates = dynEnable;
    pipelineInfo.pDynamicState = &dyn;

    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_window->defaultRenderPass();

    err = m_devFuncs->vkCreateGraphicsPipelines(dev, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_pipeline);
    if (err != VK_SUCCESS)
        qFatal("Failed to create graphics pipeline: %d", err);

    if (vertShaderModule)
        m_devFuncs->vkDestroyShaderModule(dev, vertShaderModule, nullptr);
    if (fragShaderModule)
        m_devFuncs->vkDestroyShaderModule(dev, fragShaderModule, nullptr);
}

void VulkanRenderer::initSwapChainResources()
{
    qDebug("initSwapChainResources");

    // Projection matrix
    m_proj = m_window->clipCorrectionMatrix(); // adjust for Vulkan-OpenGL clip space differences
    const QSize sz = m_window->swapChainImageSize();
    m_proj.perspective(45.0f, sz.width() / (float) sz.height(), 0.01f, 100.0f);
    m_proj.translate(0, 0, -4);
}

void VulkanRenderer::releaseSwapChainResources()
{
    qDebug("releaseSwapChainResources");
}

void VulkanRenderer::releaseResources()
{
    qDebug("releaseResources");

    VkDevice dev = m_window->device();

    if (m_sampler) {
        m_devFuncs->vkDestroySampler(dev, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }

    if (m_texStaging) {
        m_devFuncs->vkDestroyImage(dev, m_texStaging, nullptr);
        m_texStaging = VK_NULL_HANDLE;
    }

    if (m_texStagingMem) {
        m_devFuncs->vkFreeMemory(dev, m_texStagingMem, nullptr);
        m_texStagingMem = VK_NULL_HANDLE;
    }

    if (m_texView) {
        m_devFuncs->vkDestroyImageView(dev, m_texView, nullptr);
        m_texView = VK_NULL_HANDLE;
    }

    if (m_texImage) {
        m_devFuncs->vkDestroyImage(dev, m_texImage, nullptr);
        m_texImage = VK_NULL_HANDLE;
    }

    if (m_texMem) {
        m_devFuncs->vkFreeMemory(dev, m_texMem, nullptr);
        m_texMem = VK_NULL_HANDLE;
    }

    if (m_pipeline) {
        m_devFuncs->vkDestroyPipeline(dev, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLayout) {
        m_devFuncs->vkDestroyPipelineLayout(dev, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if (m_pipelineCache) {
        m_devFuncs->vkDestroyPipelineCache(dev, m_pipelineCache, nullptr);
        m_pipelineCache = VK_NULL_HANDLE;
    }

    if (m_descSetLayout) {
        m_devFuncs->vkDestroyDescriptorSetLayout(dev, m_descSetLayout, nullptr);
        m_descSetLayout = VK_NULL_HANDLE;
    }

    if (m_descPool) {
        m_devFuncs->vkDestroyDescriptorPool(dev, m_descPool, nullptr);
        m_descPool = VK_NULL_HANDLE;
    }

    if (m_buf) {
        m_devFuncs->vkDestroyBuffer(dev, m_buf, nullptr);
        m_buf = VK_NULL_HANDLE;
    }

    if (m_bufMem) {
        m_devFuncs->vkFreeMemory(dev, m_bufMem, nullptr);
        m_bufMem = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::startNextFrame()
{
    VkDevice dev = m_window->device();
    VkCommandBuffer cb = m_window->currentCommandBuffer();
    const QSize sz = m_window->swapChainImageSize();

    // Add the necessary barriers and do the host-linear -> device-optimal copy, if not yet done.
    ensureTexture();

    VkClearColorValue clearColor = {{ 0, 0, 0, 1 }};
    VkClearDepthStencilValue clearDS = { 1, 0 };
    VkClearValue clearValues[2];
    memset(clearValues, 0, sizeof(clearValues));
    clearValues[0].color = clearColor;
    clearValues[1].depthStencil = clearDS;

    VkRenderPassBeginInfo rpBeginInfo;
    memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = m_window->defaultRenderPass();
    rpBeginInfo.framebuffer = m_window->currentFramebuffer();
    rpBeginInfo.renderArea.extent.width = sz.width();
    rpBeginInfo.renderArea.extent.height = sz.height();
    rpBeginInfo.clearValueCount = 2;
    rpBeginInfo.pClearValues = clearValues;
    VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();
    m_devFuncs->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    quint8 *p;
    VkResult err = m_devFuncs->vkMapMemory(dev, m_bufMem, m_uniformBufInfo[m_window->currentFrame()].offset,
            UNIFORM_DATA_SIZE, 0, reinterpret_cast<void **>(&p));
    if (err != VK_SUCCESS)
        qFatal("Failed to map memory: %d", err);
    QMatrix4x4 m = m_proj;
    m.rotate(m_rotation, 0, 0, 1);
    memcpy(p, m.constData(), 16 * sizeof(float));
    m_devFuncs->vkUnmapMemory(dev, m_bufMem);

    // Not exactly a real animation system, just advance on every frame for now.
    m_rotation += 1.0f;

    m_devFuncs->vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    m_devFuncs->vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                               &m_descSet[m_window->currentFrame()], 0, nullptr);
    VkDeviceSize vbOffset = 0;
    m_devFuncs->vkCmdBindVertexBuffers(cb, 0, 1, &m_buf, &vbOffset);

    VkViewport viewport;
    viewport.x = viewport.y = 0;
    viewport.width = sz.width();
    viewport.height = sz.height();
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    m_devFuncs->vkCmdSetViewport(cb, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = viewport.width;
    scissor.extent.height = viewport.height;
    m_devFuncs->vkCmdSetScissor(cb, 0, 1, &scissor);

    m_devFuncs->vkCmdDraw(cb, 4, 1, 0, 0);

    m_devFuncs->vkCmdEndRenderPass(cmdBuf);

    m_window->frameReady();
    m_window->requestUpdate(); // render continuously, throttled by the presentation rate
}
