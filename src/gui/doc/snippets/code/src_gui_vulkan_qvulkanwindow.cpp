/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
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

//! [0]
  class VulkanRenderer : public QVulkanWindowRenderer
  {
  public:
      VulkanRenderer(QVulkanWindow *w) : m_window(w) { }

      void initResources() override
      {
          m_devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
          ...
      }
      void initSwapChainResources() override { ... }
      void releaseSwapChainResources() override { ... }
      void releaseResources() override { ... }

      void startNextFrame() override
      {
          VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();
          ...
          m_devFuncs->vkCmdBeginRenderPass(...);
          ...
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
      inst.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");
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
        ...
        VkDescriptorBufferInfo m_uniformBufInfo[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];
    };

    void Renderer::startNextFrame()
    {
        VkDescriptorBufferInfo &uniformBufInfo(m_uniformBufInfo[m_window->currentFrame()]);
        ...
    }
//! [1]

//! [2]
    class Renderer {
        ...
        VkDescriptorBufferInfo m_uniformBufInfo[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];
    };

    void Renderer::startNextFrame()
    {
        const int count = m_window->concurrentFrameCount();
        for (int i = 0; i < count; ++i)
            m_uniformBufInfo[i] = ...
        ...
    }
//! [2]
