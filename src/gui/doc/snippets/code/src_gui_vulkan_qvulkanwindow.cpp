// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QGuiApplication>
#include <QVulkanDeviceFunctions>
#include <QVulkanWindow>

namespace src_gui_vulkan_qvulkanwindow {
VkCommandBuffer commandBuffer;
const VkRenderPassBeginInfo *renderPassBegin = nullptr;
VkSubpassContents contents;
//! [0]
class VulkanRenderer : public QVulkanWindowRenderer
{
public:
    VulkanRenderer(QVulkanWindow *w) : m_window(w), m_devFuncs(nullptr) { }

    void initResources() override
    {
        m_devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
        // ..
    }
    void initSwapChainResources() override { /* ... */ }
    void releaseSwapChainResources() override { /* ... */ }
    void releaseResources() override { /* ... */ }

    void startNextFrame() override
    {
        VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();
        // ...
        m_devFuncs->vkCmdBeginRenderPass(commandBuffer, renderPassBegin, contents);
        // ...
        m_window->frameReady();
    }

private:
    QVulkanWindow *m_window;
    QVulkanDeviceFunctions *m_devFuncs;
};

class VulkanWindow : public QVulkanWindow
{
public:
    QVulkanWindowRenderer *createRenderer() override {
        return new VulkanRenderer(this);
    }
};

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QVulkanInstance inst;
    // enable the standard validation layers, when available
    inst.setLayers({ "VK_LAYER_KHRONOS_validation" });
    if (!inst.create())
        qFatal("Failed to create Vulkan instance: %d", inst.errorCode());

    VulkanWindow w;
    w.setVulkanInstance(&inst);
    w.showMaximized();

    return app.exec();
}
//! [0]

//! [1]
    class Renderer {
        void startNextFrame();
        // ...

        VkDescriptorBufferInfo m_uniformBufInfo[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];
        QVulkanWindow *m_window = nullptr;
    };

    void Renderer::startNextFrame()
    {
        VkDescriptorBufferInfo &uniformBufInfo(m_uniformBufInfo[m_window->currentFrame()]);
        // ...
    }
//! [1]

} // src_gui_vulkan_qvulkanwindow


namespace src_gui_vulkan_qvulkanwindow2 {

//! [2]
    class Renderer {
        void startNextFrame();
        // ...

        VkDescriptorBufferInfo m_uniformBufInfo[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];
        QVulkanWindow *m_window = nullptr;
    };

    void Renderer::startNextFrame()
    {
        const int count = m_window->concurrentFrameCount();
        // for (int i = 0; i < count; ++i)
            // m_uniformBufInfo[i] = ...
        // ...
    }
//! [2]

} // src_gui_vulkan_qvulkanwindow2
