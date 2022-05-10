// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "hellovulkanwindow.h"
#include <QVulkanFunctions>

//! [0]
QVulkanWindowRenderer *VulkanWindow::createRenderer()
{
    return new VulkanRenderer(this);
}

VulkanRenderer::VulkanRenderer(QVulkanWindow *w)
    : m_window(w)
{
}
//! [0]

//! [1]
void VulkanRenderer::initResources()
{
    qDebug("initResources");

    m_devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
}
//! [1]

void VulkanRenderer::initSwapChainResources()
{
    qDebug("initSwapChainResources");
}

void VulkanRenderer::releaseSwapChainResources()
{
    qDebug("releaseSwapChainResources");
}

void VulkanRenderer::releaseResources()
{
    qDebug("releaseResources");
}

//! [2]
void VulkanRenderer::startNextFrame()
{
    m_green += 0.005f;
    if (m_green > 1.0f)
        m_green = 0.0f;

    VkClearColorValue clearColor = {{ 0.0f, m_green, 0.0f, 1.0f }};
    VkClearDepthStencilValue clearDS = { 1.0f, 0 };
    VkClearValue clearValues[2];
    memset(clearValues, 0, sizeof(clearValues));
    clearValues[0].color = clearColor;
    clearValues[1].depthStencil = clearDS;

    VkRenderPassBeginInfo rpBeginInfo;
    memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = m_window->defaultRenderPass();
    rpBeginInfo.framebuffer = m_window->currentFramebuffer();
    const QSize sz = m_window->swapChainImageSize();
    rpBeginInfo.renderArea.extent.width = sz.width();
    rpBeginInfo.renderArea.extent.height = sz.height();
    rpBeginInfo.clearValueCount = 2;
    rpBeginInfo.pClearValues = clearValues;
    VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();
    m_devFuncs->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Do nothing else. We will just clear to green, changing the component on
    // every invocation. This also helps verifying the rate to which the thread
    // is throttled to. (The elapsed time between startNextFrame calls should
    // typically be around 16 ms. Note that rendering is 2 frames ahead of what
    // is displayed.)

    m_devFuncs->vkCmdEndRenderPass(cmdBuf);

    m_window->frameReady();
    m_window->requestUpdate(); // render continuously, throttled by the presentation rate
}
//! [2]
