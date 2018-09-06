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

#include "renderer.h"
#include <QVulkanFunctions>
#include <QtConcurrentRun>
#include <QTime>

static float quadVert[] = { // Y up, front = CW
    -1, -1, 0,
    -1,  1, 0,
     1, -1, 0,
     1,  1, 0
};

#define DBG Q_UNLIKELY(m_window->isDebugEnabled())

const int MAX_INSTANCES = 16384;
const VkDeviceSize PER_INSTANCE_DATA_SIZE = 6 * sizeof(float); // instTranslate, instDiffuseAdjust

static inline VkDeviceSize aligned(VkDeviceSize v, VkDeviceSize byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

Renderer::Renderer(VulkanWindow *w, int initialCount)
    : m_window(w),
      // Have the light positioned just behind the default camera position, looking forward.
      m_lightPos(0.0f, 0.0f, 25.0f),
      m_cam(QVector3D(0.0f, 0.0f, 20.0f)), // starting camera position
      m_instCount(initialCount)
{
    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));

    m_floorModel.translate(0, -5, 0);
    m_floorModel.rotate(-90, 1, 0, 0);
    m_floorModel.scale(20, 100, 1);

    m_blockMesh.load(QStringLiteral(":/block.buf"));
    m_logoMesh.load(QStringLiteral(":/qt_logo.buf"));

    QObject::connect(&m_frameWatcher, &QFutureWatcherBase::finished, [this] {
        if (m_framePending) {
            m_framePending = false;
            m_window->frameReady();
            m_window->requestUpdate();
        }
    });
}

void Renderer::preInitResources()
{
    const QVector<int> sampleCounts = m_window->supportedSampleCounts();
    if (DBG)
        qDebug() << "Supported sample counts:" << sampleCounts;
    if (sampleCounts.contains(4)) {
        if (DBG)
            qDebug("Requesting 4x MSAA");
        m_window->setSampleCount(4);
    }
}

void Renderer::initResources()
{
    if (DBG)
        qDebug("Renderer init");

    m_animating = true;
    m_framePending = false;

    QVulkanInstance *inst = m_window->vulkanInstance();
    VkDevice dev = m_window->device();
    const VkPhysicalDeviceLimits *pdevLimits = &m_window->physicalDeviceProperties()->limits;
    const VkDeviceSize uniAlign = pdevLimits->minUniformBufferOffsetAlignment;

    m_devFuncs = inst->deviceFunctions(dev);

    // Note the std140 packing rules. A vec3 still has an alignment of 16,
    // while a mat3 is like 3 * vec3.
    m_itemMaterial.vertUniSize = aligned(2 * 64 + 48, uniAlign); // see color_phong.vert
    m_itemMaterial.fragUniSize = aligned(6 * 16 + 12 + 2 * 4, uniAlign); // see color_phong.frag

    if (!m_itemMaterial.vs.isValid())
        m_itemMaterial.vs.load(inst, dev, QStringLiteral(":/color_phong_vert.spv"));
    if (!m_itemMaterial.fs.isValid())
        m_itemMaterial.fs.load(inst, dev, QStringLiteral(":/color_phong_frag.spv"));

    if (!m_floorMaterial.vs.isValid())
        m_floorMaterial.vs.load(inst, dev, QStringLiteral(":/color_vert.spv"));
    if (!m_floorMaterial.fs.isValid())
        m_floorMaterial.fs.load(inst, dev, QStringLiteral(":/color_frag.spv"));

    m_pipelinesFuture = QtConcurrent::run(this, &Renderer::createPipelines);
}

void Renderer::createPipelines()
{
    VkDevice dev = m_window->device();

    VkPipelineCacheCreateInfo pipelineCacheInfo;
    memset(&pipelineCacheInfo, 0, sizeof(pipelineCacheInfo));
    pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VkResult err = m_devFuncs->vkCreatePipelineCache(dev, &pipelineCacheInfo, nullptr, &m_pipelineCache);
    if (err != VK_SUCCESS)
        qFatal("Failed to create pipeline cache: %d", err);

    createItemPipeline();
    createFloorPipeline();
}

void Renderer::createItemPipeline()
{
    VkDevice dev = m_window->device();

    // Vertex layout.
    VkVertexInputBindingDescription vertexBindingDesc[] = {
        {
            0, // binding
            8 * sizeof(float),
            VK_VERTEX_INPUT_RATE_VERTEX
        },
        {
            1,
            6 * sizeof(float),
            VK_VERTEX_INPUT_RATE_INSTANCE
        }
    };
    VkVertexInputAttributeDescription vertexAttrDesc[] = {
        { // position
            0, // location
            0, // binding
            VK_FORMAT_R32G32B32_SFLOAT,
            0 // offset
        },
        { // normal
            1,
            0,
            VK_FORMAT_R32G32B32_SFLOAT,
            5 * sizeof(float)
        },
        { // instTranslate
            2,
            1,
            VK_FORMAT_R32G32B32_SFLOAT,
            0
        },
        { // instDiffuseAdjust
            3,
            1,
            VK_FORMAT_R32G32B32_SFLOAT,
            3 * sizeof(float)
        }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pNext = nullptr;
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = sizeof(vertexBindingDesc) / sizeof(vertexBindingDesc[0]);
    vertexInputInfo.pVertexBindingDescriptions = vertexBindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = sizeof(vertexAttrDesc) / sizeof(vertexAttrDesc[0]);
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttrDesc;

    // Descriptor set layout.
    VkDescriptorPoolSize descPoolSizes[] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2 }
    };
    VkDescriptorPoolCreateInfo descPoolInfo;
    memset(&descPoolInfo, 0, sizeof(descPoolInfo));
    descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descPoolInfo.maxSets = 1; // a single set is enough due to the dynamic uniform buffer
    descPoolInfo.poolSizeCount = sizeof(descPoolSizes) / sizeof(descPoolSizes[0]);
    descPoolInfo.pPoolSizes = descPoolSizes;
    VkResult err = m_devFuncs->vkCreateDescriptorPool(dev, &descPoolInfo, nullptr, &m_itemMaterial.descPool);
    if (err != VK_SUCCESS)
        qFatal("Failed to create descriptor pool: %d", err);

    VkDescriptorSetLayoutBinding layoutBindings[] =
    {
        {
            0, // binding
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            1, // descriptorCount
            VK_SHADER_STAGE_VERTEX_BIT,
            nullptr
        },
        {
            1,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        }
    };
    VkDescriptorSetLayoutCreateInfo descLayoutInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        sizeof(layoutBindings) / sizeof(layoutBindings[0]),
        layoutBindings
    };
    err = m_devFuncs->vkCreateDescriptorSetLayout(dev, &descLayoutInfo, nullptr, &m_itemMaterial.descSetLayout);
    if (err != VK_SUCCESS)
        qFatal("Failed to create descriptor set layout: %d", err);

    VkDescriptorSetAllocateInfo descSetAllocInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        m_itemMaterial.descPool,
        1,
        &m_itemMaterial.descSetLayout
    };
    err = m_devFuncs->vkAllocateDescriptorSets(dev, &descSetAllocInfo, &m_itemMaterial.descSet);
    if (err != VK_SUCCESS)
        qFatal("Failed to allocate descriptor set: %d", err);

    // Graphics pipeline.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_itemMaterial.descSetLayout;

    err = m_devFuncs->vkCreatePipelineLayout(dev, &pipelineLayoutInfo, nullptr, &m_itemMaterial.pipelineLayout);
    if (err != VK_SUCCESS)
        qFatal("Failed to create pipeline layout: %d", err);

    VkGraphicsPipelineCreateInfo pipelineInfo;
    memset(&pipelineInfo, 0, sizeof(pipelineInfo));
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineShaderStageCreateInfo shaderStages[2] = {
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_VERTEX_BIT,
            m_itemMaterial.vs.data()->shaderModule,
            "main",
            nullptr
        },
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            m_itemMaterial.fs.data()->shaderModule,
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
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInfo.pInputAssemblyState = &ia;

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
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;
    pipelineInfo.pRasterizationState = &rs;

    VkPipelineMultisampleStateCreateInfo ms;
    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = m_window->sampleCountFlagBits();
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
    VkPipelineColorBlendAttachmentState att;
    memset(&att, 0, sizeof(att));
    att.colorWriteMask = 0xF;
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

    pipelineInfo.layout = m_itemMaterial.pipelineLayout;
    pipelineInfo.renderPass = m_window->defaultRenderPass();

    err = m_devFuncs->vkCreateGraphicsPipelines(dev, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_itemMaterial.pipeline);
    if (err != VK_SUCCESS)
        qFatal("Failed to create graphics pipeline: %d", err);
}

void Renderer::createFloorPipeline()
{
    VkDevice dev = m_window->device();

    // Vertex layout.
    VkVertexInputBindingDescription vertexBindingDesc = {
        0, // binding
        3 * sizeof(float),
        VK_VERTEX_INPUT_RATE_VERTEX
    };
    VkVertexInputAttributeDescription vertexAttrDesc[] = {
        { // position
            0, // location
            0, // binding
            VK_FORMAT_R32G32B32_SFLOAT,
            0 // offset
        },
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pNext = nullptr;
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = sizeof(vertexAttrDesc) / sizeof(vertexAttrDesc[0]);
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttrDesc;

    // Do not bother with uniform buffers and descriptors, all the data fits
    // into the spec mandated minimum of 128 bytes for push constants.
    VkPushConstantRange pcr[] = {
        // mvp
        {
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            64
        },
        // color
        {
            VK_SHADER_STAGE_FRAGMENT_BIT,
            64,
            12
        }
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = sizeof(pcr) / sizeof(pcr[0]);
    pipelineLayoutInfo.pPushConstantRanges = pcr;

    VkResult err = m_devFuncs->vkCreatePipelineLayout(dev, &pipelineLayoutInfo, nullptr, &m_floorMaterial.pipelineLayout);
    if (err != VK_SUCCESS)
        qFatal("Failed to create pipeline layout: %d", err);

    VkGraphicsPipelineCreateInfo pipelineInfo;
    memset(&pipelineInfo, 0, sizeof(pipelineInfo));
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineShaderStageCreateInfo shaderStages[2] = {
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_VERTEX_BIT,
            m_floorMaterial.vs.data()->shaderModule,
            "main",
            nullptr
        },
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            m_floorMaterial.fs.data()->shaderModule,
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
    ms.rasterizationSamples = m_window->sampleCountFlagBits();
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
    VkPipelineColorBlendAttachmentState att;
    memset(&att, 0, sizeof(att));
    att.colorWriteMask = 0xF;
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

    pipelineInfo.layout = m_floorMaterial.pipelineLayout;
    pipelineInfo.renderPass = m_window->defaultRenderPass();

    err = m_devFuncs->vkCreateGraphicsPipelines(dev, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_floorMaterial.pipeline);
    if (err != VK_SUCCESS)
        qFatal("Failed to create graphics pipeline: %d", err);
}

void Renderer::initSwapChainResources()
{
    m_proj = m_window->clipCorrectionMatrix();
    const QSize sz = m_window->swapChainImageSize();
    m_proj.perspective(45.0f, sz.width() / (float) sz.height(), 0.01f, 1000.0f);
    markViewProjDirty();
}

void Renderer::releaseSwapChainResources()
{
    // It is important to finish the pending frame right here since this is the
    // last opportunity to act with all resources intact.
    m_frameWatcher.waitForFinished();
    // Cannot count on the finished() signal being emitted before returning
    // from here.
    if (m_framePending) {
        m_framePending = false;
        m_window->frameReady();
    }
}

void Renderer::releaseResources()
{
    if (DBG)
        qDebug("Renderer release");

    m_pipelinesFuture.waitForFinished();

    VkDevice dev = m_window->device();

    if (m_itemMaterial.descSetLayout) {
        m_devFuncs->vkDestroyDescriptorSetLayout(dev, m_itemMaterial.descSetLayout, nullptr);
        m_itemMaterial.descSetLayout = VK_NULL_HANDLE;
    }

    if (m_itemMaterial.descPool) {
        m_devFuncs->vkDestroyDescriptorPool(dev, m_itemMaterial.descPool, nullptr);
        m_itemMaterial.descPool = VK_NULL_HANDLE;
    }

    if (m_itemMaterial.pipeline) {
        m_devFuncs->vkDestroyPipeline(dev, m_itemMaterial.pipeline, nullptr);
        m_itemMaterial.pipeline = VK_NULL_HANDLE;
    }

    if (m_itemMaterial.pipelineLayout) {
        m_devFuncs->vkDestroyPipelineLayout(dev, m_itemMaterial.pipelineLayout, nullptr);
        m_itemMaterial.pipelineLayout = VK_NULL_HANDLE;
    }

    if (m_floorMaterial.pipeline) {
        m_devFuncs->vkDestroyPipeline(dev, m_floorMaterial.pipeline, nullptr);
        m_floorMaterial.pipeline = VK_NULL_HANDLE;
    }

    if (m_floorMaterial.pipelineLayout) {
        m_devFuncs->vkDestroyPipelineLayout(dev, m_floorMaterial.pipelineLayout, nullptr);
        m_floorMaterial.pipelineLayout = VK_NULL_HANDLE;
    }

    if (m_pipelineCache) {
        m_devFuncs->vkDestroyPipelineCache(dev, m_pipelineCache, nullptr);
        m_pipelineCache = VK_NULL_HANDLE;
    }

    if (m_blockVertexBuf) {
        m_devFuncs->vkDestroyBuffer(dev, m_blockVertexBuf, nullptr);
        m_blockVertexBuf = VK_NULL_HANDLE;
    }

    if (m_logoVertexBuf) {
        m_devFuncs->vkDestroyBuffer(dev, m_logoVertexBuf, nullptr);
        m_logoVertexBuf = VK_NULL_HANDLE;
    }

    if (m_floorVertexBuf) {
        m_devFuncs->vkDestroyBuffer(dev, m_floorVertexBuf, nullptr);
        m_floorVertexBuf = VK_NULL_HANDLE;
    }

    if (m_uniBuf) {
        m_devFuncs->vkDestroyBuffer(dev, m_uniBuf, nullptr);
        m_uniBuf = VK_NULL_HANDLE;
    }

    if (m_bufMem) {
        m_devFuncs->vkFreeMemory(dev, m_bufMem, nullptr);
        m_bufMem = VK_NULL_HANDLE;
    }

    if (m_instBuf) {
        m_devFuncs->vkDestroyBuffer(dev, m_instBuf, nullptr);
        m_instBuf = VK_NULL_HANDLE;
    }

    if (m_instBufMem) {
        m_devFuncs->vkFreeMemory(dev, m_instBufMem, nullptr);
        m_instBufMem = VK_NULL_HANDLE;
    }

    if (m_itemMaterial.vs.isValid()) {
        m_devFuncs->vkDestroyShaderModule(dev, m_itemMaterial.vs.data()->shaderModule, nullptr);
        m_itemMaterial.vs.reset();
    }
    if (m_itemMaterial.fs.isValid()) {
        m_devFuncs->vkDestroyShaderModule(dev, m_itemMaterial.fs.data()->shaderModule, nullptr);
        m_itemMaterial.fs.reset();
    }

    if (m_floorMaterial.vs.isValid()) {
        m_devFuncs->vkDestroyShaderModule(dev, m_floorMaterial.vs.data()->shaderModule, nullptr);
        m_floorMaterial.vs.reset();
    }
    if (m_floorMaterial.fs.isValid()) {
        m_devFuncs->vkDestroyShaderModule(dev, m_floorMaterial.fs.data()->shaderModule, nullptr);
        m_floorMaterial.fs.reset();
    }
}

void Renderer::ensureBuffers()
{
    if (m_blockVertexBuf)
        return;

    VkDevice dev = m_window->device();
    const int concurrentFrameCount = m_window->concurrentFrameCount();

    // Vertex buffer for the block.
    VkBufferCreateInfo bufInfo;
    memset(&bufInfo, 0, sizeof(bufInfo));
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    const int blockMeshByteCount = m_blockMesh.data()->vertexCount * 8 * sizeof(float);
    bufInfo.size = blockMeshByteCount;
    bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VkResult err = m_devFuncs->vkCreateBuffer(dev, &bufInfo, nullptr, &m_blockVertexBuf);
    if (err != VK_SUCCESS)
        qFatal("Failed to create vertex buffer: %d", err);

    VkMemoryRequirements blockVertMemReq;
    m_devFuncs->vkGetBufferMemoryRequirements(dev, m_blockVertexBuf, &blockVertMemReq);

    // Vertex buffer for the logo.
    const int logoMeshByteCount = m_logoMesh.data()->vertexCount * 8 * sizeof(float);
    bufInfo.size = logoMeshByteCount;
    bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    err = m_devFuncs->vkCreateBuffer(dev, &bufInfo, nullptr, &m_logoVertexBuf);
    if (err != VK_SUCCESS)
        qFatal("Failed to create vertex buffer: %d", err);

    VkMemoryRequirements logoVertMemReq;
    m_devFuncs->vkGetBufferMemoryRequirements(dev, m_logoVertexBuf, &logoVertMemReq);

    // Vertex buffer for the floor.
    bufInfo.size = sizeof(quadVert);
    err = m_devFuncs->vkCreateBuffer(dev, &bufInfo, nullptr, &m_floorVertexBuf);
    if (err != VK_SUCCESS)
        qFatal("Failed to create vertex buffer: %d", err);

    VkMemoryRequirements floorVertMemReq;
    m_devFuncs->vkGetBufferMemoryRequirements(dev, m_floorVertexBuf, &floorVertMemReq);

    // Uniform buffer. Instead of using multiple descriptor sets, we take a
    // different approach: have a single dynamic uniform buffer and specify the
    // active-frame-specific offset at the time of binding the descriptor set.
    bufInfo.size = (m_itemMaterial.vertUniSize + m_itemMaterial.fragUniSize) * concurrentFrameCount;
    bufInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    err = m_devFuncs->vkCreateBuffer(dev, &bufInfo, nullptr, &m_uniBuf);
    if (err != VK_SUCCESS)
        qFatal("Failed to create uniform buffer: %d", err);

    VkMemoryRequirements uniMemReq;
    m_devFuncs->vkGetBufferMemoryRequirements(dev, m_uniBuf, &uniMemReq);

    // Allocate memory for everything at once.
    VkDeviceSize logoVertStartOffset = aligned(0 + blockVertMemReq.size, logoVertMemReq.alignment);
    VkDeviceSize floorVertStartOffset = aligned(logoVertStartOffset + logoVertMemReq.size, floorVertMemReq.alignment);
    m_itemMaterial.uniMemStartOffset = aligned(floorVertStartOffset + floorVertMemReq.size, uniMemReq.alignment);
    VkMemoryAllocateInfo memAllocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        m_itemMaterial.uniMemStartOffset + uniMemReq.size,
        m_window->hostVisibleMemoryIndex()
    };
    err = m_devFuncs->vkAllocateMemory(dev, &memAllocInfo, nullptr, &m_bufMem);
    if (err != VK_SUCCESS)
        qFatal("Failed to allocate memory: %d", err);

    err = m_devFuncs->vkBindBufferMemory(dev, m_blockVertexBuf, m_bufMem, 0);
    if (err != VK_SUCCESS)
        qFatal("Failed to bind vertex buffer memory: %d", err);
    err = m_devFuncs->vkBindBufferMemory(dev, m_logoVertexBuf, m_bufMem, logoVertStartOffset);
    if (err != VK_SUCCESS)
        qFatal("Failed to bind vertex buffer memory: %d", err);
    err = m_devFuncs->vkBindBufferMemory(dev, m_floorVertexBuf, m_bufMem, floorVertStartOffset);
    if (err != VK_SUCCESS)
        qFatal("Failed to bind vertex buffer memory: %d", err);
    err = m_devFuncs->vkBindBufferMemory(dev, m_uniBuf, m_bufMem, m_itemMaterial.uniMemStartOffset);
    if (err != VK_SUCCESS)
        qFatal("Failed to bind uniform buffer memory: %d", err);

    // Copy vertex data.
    quint8 *p;
    err = m_devFuncs->vkMapMemory(dev, m_bufMem, 0, m_itemMaterial.uniMemStartOffset, 0, reinterpret_cast<void **>(&p));
    if (err != VK_SUCCESS)
        qFatal("Failed to map memory: %d", err);
    memcpy(p, m_blockMesh.data()->geom.constData(), blockMeshByteCount);
    memcpy(p + logoVertStartOffset, m_logoMesh.data()->geom.constData(), logoMeshByteCount);
    memcpy(p + floorVertStartOffset, quadVert, sizeof(quadVert));
    m_devFuncs->vkUnmapMemory(dev, m_bufMem);

    // Write descriptors for the uniform buffers in the vertex and fragment shaders.
    VkDescriptorBufferInfo vertUni = { m_uniBuf, 0, m_itemMaterial.vertUniSize };
    VkDescriptorBufferInfo fragUni = { m_uniBuf, m_itemMaterial.vertUniSize, m_itemMaterial.fragUniSize };

    VkWriteDescriptorSet descWrite[2];
    memset(descWrite, 0, sizeof(descWrite));
    descWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descWrite[0].dstSet = m_itemMaterial.descSet;
    descWrite[0].dstBinding = 0;
    descWrite[0].descriptorCount = 1;
    descWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descWrite[0].pBufferInfo = &vertUni;

    descWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descWrite[1].dstSet = m_itemMaterial.descSet;
    descWrite[1].dstBinding = 1;
    descWrite[1].descriptorCount = 1;
    descWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descWrite[1].pBufferInfo = &fragUni;

    m_devFuncs->vkUpdateDescriptorSets(dev, 2, descWrite, 0, nullptr);
}

void Renderer::ensureInstanceBuffer()
{
    if (m_instCount == m_preparedInstCount && m_instBuf)
        return;

    Q_ASSERT(m_instCount <= MAX_INSTANCES);

    VkDevice dev = m_window->device();

    // allocate only once, for the maximum instance count
    if (!m_instBuf) {
        VkBufferCreateInfo bufInfo;
        memset(&bufInfo, 0, sizeof(bufInfo));
        bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size = MAX_INSTANCES * PER_INSTANCE_DATA_SIZE;
        bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        // Keep a copy of the data since we may lose all graphics resources on
        // unexpose, and reinitializing to new random positions afterwards
        // would not be nice.
        m_instData.resize(bufInfo.size);

        VkResult err = m_devFuncs->vkCreateBuffer(dev, &bufInfo, nullptr, &m_instBuf);
        if (err != VK_SUCCESS)
            qFatal("Failed to create instance buffer: %d", err);

        VkMemoryRequirements memReq;
        m_devFuncs->vkGetBufferMemoryRequirements(dev, m_instBuf, &memReq);
        if (DBG)
            qDebug("Allocating %u bytes for instance data", uint32_t(memReq.size));

        VkMemoryAllocateInfo memAllocInfo = {
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            nullptr,
            memReq.size,
            m_window->hostVisibleMemoryIndex()
        };
        err = m_devFuncs->vkAllocateMemory(dev, &memAllocInfo, nullptr, &m_instBufMem);
        if (err != VK_SUCCESS)
            qFatal("Failed to allocate memory: %d", err);

        err = m_devFuncs->vkBindBufferMemory(dev, m_instBuf, m_instBufMem, 0);
        if (err != VK_SUCCESS)
            qFatal("Failed to bind instance buffer memory: %d", err);
    }

    if (m_instCount != m_preparedInstCount) {
        if (DBG)
            qDebug("Preparing instances %d..%d", m_preparedInstCount, m_instCount - 1);
        char *p = m_instData.data();
        p += m_preparedInstCount * PER_INSTANCE_DATA_SIZE;
        auto gen = [](float a, float b) { return float((qrand() % int(b - a + 1)) + a); };
        for (int i = m_preparedInstCount; i < m_instCount; ++i) {
            // Apply a random translation to each instance of the mesh.
            float t[] = { gen(-5, 5), gen(-4, 6), gen(-30, 5) };
            memcpy(p, t, 12);
            // Apply a random adjustment to the diffuse color for each instance. (default is 0.7)
            float d[] = { gen(-6, 3) / 10.0f, gen(-6, 3) / 10.0f, gen(-6, 3) / 10.0f };
            memcpy(p + 12, d, 12);
            p += PER_INSTANCE_DATA_SIZE;
        }
        m_preparedInstCount = m_instCount;
    }

    quint8 *p;
    VkResult err = m_devFuncs->vkMapMemory(dev, m_instBufMem, 0, m_instCount * PER_INSTANCE_DATA_SIZE, 0,
                                           reinterpret_cast<void **>(&p));
    if (err != VK_SUCCESS)
        qFatal("Failed to map memory: %d", err);
    memcpy(p, m_instData.constData(), m_instData.size());
    m_devFuncs->vkUnmapMemory(dev, m_instBufMem);
}

void Renderer::getMatrices(QMatrix4x4 *vp, QMatrix4x4 *model, QMatrix3x3 *modelNormal, QVector3D *eyePos)
{
    model->setToIdentity();
    if (m_useLogo)
        model->rotate(90, 1, 0, 0);
    model->rotate(m_rotation, 1, 1, 0);

    *modelNormal = model->normalMatrix();

    QMatrix4x4 view = m_cam.viewMatrix();
    *vp = m_proj * view;

    *eyePos = view.inverted().column(3).toVector3D();
}

void Renderer::writeFragUni(quint8 *p, const QVector3D &eyePos)
{
    float ECCameraPosition[] = { eyePos.x(), eyePos.y(), eyePos.z() };
    memcpy(p, ECCameraPosition, 12);
    p += 16;

    // Material
    float ka[] = { 0.05f, 0.05f, 0.05f };
    memcpy(p, ka, 12);
    p += 16;

    float kd[] = { 0.7f, 0.7f, 0.7f };
    memcpy(p, kd, 12);
    p += 16;

    float ks[] = { 0.66f, 0.66f, 0.66f };
    memcpy(p, ks, 12);
    p += 16;

    // Light parameters
    float ECLightPosition[] = { m_lightPos.x(), m_lightPos.y(), m_lightPos.z() };
    memcpy(p, ECLightPosition, 12);
    p += 16;

    float att[] = { 1, 0, 0 };
    memcpy(p, att, 12);
    p += 16;

    float color[] = { 1.0f, 1.0f, 1.0f };
    memcpy(p, color, 12);
    p += 12; // next we have two floats which have an alignment of 4, hence 12 only

    float intensity = 0.8f;
    memcpy(p, &intensity, 4);
    p += 4;

    float specularExp = 150.0f;
    memcpy(p, &specularExp, 4);
    p += 4;
}

void Renderer::startNextFrame()
{
    // For demonstration purposes offload the command buffer generation onto a
    // worker thread and continue with the frame submission only when it has
    // finished.
    Q_ASSERT(!m_framePending);
    m_framePending = true;
    QFuture<void> future = QtConcurrent::run(this, &Renderer::buildFrame);
    m_frameWatcher.setFuture(future);
}

void Renderer::buildFrame()
{
    QMutexLocker locker(&m_guiMutex);

    ensureBuffers();
    ensureInstanceBuffer();
    m_pipelinesFuture.waitForFinished();

    VkCommandBuffer cb = m_window->currentCommandBuffer();
    const QSize sz = m_window->swapChainImageSize();

    VkClearColorValue clearColor = {{ 0.67f, 0.84f, 0.9f, 1.0f }};
    VkClearDepthStencilValue clearDS = { 1, 0 };
    VkClearValue clearValues[3];
    memset(clearValues, 0, sizeof(clearValues));
    clearValues[0].color = clearValues[2].color = clearColor;
    clearValues[1].depthStencil = clearDS;

    VkRenderPassBeginInfo rpBeginInfo;
    memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = m_window->defaultRenderPass();
    rpBeginInfo.framebuffer = m_window->currentFramebuffer();
    rpBeginInfo.renderArea.extent.width = sz.width();
    rpBeginInfo.renderArea.extent.height = sz.height();
    rpBeginInfo.clearValueCount = m_window->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
    rpBeginInfo.pClearValues = clearValues;
    VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();
    m_devFuncs->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {
        0, 0,
        float(sz.width()), float(sz.height()),
        0, 1
    };
    m_devFuncs->vkCmdSetViewport(cb, 0, 1, &viewport);

    VkRect2D scissor = {
        { 0, 0 },
        { uint32_t(sz.width()), uint32_t(sz.height()) }
    };
    m_devFuncs->vkCmdSetScissor(cb, 0, 1, &scissor);

    buildDrawCallsForFloor();
    buildDrawCallsForItems();

    m_devFuncs->vkCmdEndRenderPass(cmdBuf);
}

void Renderer::buildDrawCallsForItems()
{
    VkDevice dev = m_window->device();
    VkCommandBuffer cb = m_window->currentCommandBuffer();

    m_devFuncs->vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_itemMaterial.pipeline);

    VkDeviceSize vbOffset = 0;
    m_devFuncs->vkCmdBindVertexBuffers(cb, 0, 1, m_useLogo ? &m_logoVertexBuf : &m_blockVertexBuf, &vbOffset);
    m_devFuncs->vkCmdBindVertexBuffers(cb, 1, 1, &m_instBuf, &vbOffset);

    // Now provide offsets so that the two dynamic buffers point to the
    // beginning of the vertex and fragment uniform data for the current frame.
    uint32_t frameUniOffset = m_window->currentFrame() * (m_itemMaterial.vertUniSize + m_itemMaterial.fragUniSize);
    uint32_t frameUniOffsets[] = { frameUniOffset, frameUniOffset };
    m_devFuncs->vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_itemMaterial.pipelineLayout, 0, 1,
                                        &m_itemMaterial.descSet, 2, frameUniOffsets);

    if (m_animating)
        m_rotation += 0.5;

    if (m_animating || m_vpDirty) {
        if (m_vpDirty)
            --m_vpDirty;
        QMatrix4x4 vp, model;
        QMatrix3x3 modelNormal;
        QVector3D eyePos;
        getMatrices(&vp, &model, &modelNormal, &eyePos);

        // Map the uniform data for the current frame, ignore the geometry data at
        // the beginning and the uniforms for other frames.
        quint8 *p;
        VkResult err = m_devFuncs->vkMapMemory(dev, m_bufMem,
                                               m_itemMaterial.uniMemStartOffset + frameUniOffset,
                                               m_itemMaterial.vertUniSize + m_itemMaterial.fragUniSize,
                                               0, reinterpret_cast<void **>(&p));
        if (err != VK_SUCCESS)
            qFatal("Failed to map memory: %d", err);

        // Vertex shader uniforms
        memcpy(p, vp.constData(), 64);
        memcpy(p + 64, model.constData(), 64);
        const float *mnp = modelNormal.constData();
        memcpy(p + 128, mnp, 12);
        memcpy(p + 128 + 16, mnp + 3, 12);
        memcpy(p + 128 + 32, mnp + 6, 12);

        // Fragment shader uniforms
        p += m_itemMaterial.vertUniSize;
        writeFragUni(p, eyePos);

        m_devFuncs->vkUnmapMemory(dev, m_bufMem);
    }

    m_devFuncs->vkCmdDraw(cb, (m_useLogo ? m_logoMesh.data() : m_blockMesh.data())->vertexCount, m_instCount, 0, 0);
}

void Renderer::buildDrawCallsForFloor()
{
    VkCommandBuffer cb = m_window->currentCommandBuffer();

    m_devFuncs->vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_floorMaterial.pipeline);

    VkDeviceSize vbOffset = 0;
    m_devFuncs->vkCmdBindVertexBuffers(cb, 0, 1, &m_floorVertexBuf, &vbOffset);

    QMatrix4x4 mvp = m_proj * m_cam.viewMatrix() * m_floorModel;
    m_devFuncs->vkCmdPushConstants(cb, m_floorMaterial.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, mvp.constData());
    float color[] = { 0.67f, 1.0f, 0.2f };
    m_devFuncs->vkCmdPushConstants(cb, m_floorMaterial.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 64, 12, color);

    m_devFuncs->vkCmdDraw(cb, 4, 1, 0, 0);
}

void Renderer::addNew()
{
    QMutexLocker locker(&m_guiMutex);
    m_instCount = qMin(m_instCount + 16, MAX_INSTANCES);
}

void Renderer::yaw(float degrees)
{
    QMutexLocker locker(&m_guiMutex);
    m_cam.yaw(degrees);
    markViewProjDirty();
}

void Renderer::pitch(float degrees)
{
    QMutexLocker locker(&m_guiMutex);
    m_cam.pitch(degrees);
    markViewProjDirty();
}

void Renderer::walk(float amount)
{
    QMutexLocker locker(&m_guiMutex);
    m_cam.walk(amount);
    markViewProjDirty();
}

void Renderer::strafe(float amount)
{
    QMutexLocker locker(&m_guiMutex);
    m_cam.strafe(amount);
    markViewProjDirty();
}

void Renderer::setUseLogo(bool b)
{
    QMutexLocker locker(&m_guiMutex);
    m_useLogo = b;
    if (!m_animating)
        m_window->requestUpdate();
}
