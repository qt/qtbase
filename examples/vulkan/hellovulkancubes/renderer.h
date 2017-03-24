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

#ifndef RENDERER_H
#define RENDERER_H

#include "vulkanwindow.h"
#include "mesh.h"
#include "shader.h"
#include "camera.h"
#include <QFutureWatcher>
#include <QMutex>

class Renderer : public QVulkanWindowRenderer
{
public:
    Renderer(VulkanWindow *w, int initialCount);

    void preInitResources() override;
    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;

    void startNextFrame() override;

    bool animating() const { return m_animating; }
    void setAnimating(bool a) { m_animating = a; }

    int instanceCount() const { return m_instCount; }
    void addNew();

    void yaw(float degrees);
    void pitch(float degrees);
    void walk(float amount);
    void strafe(float amount);

    void setUseLogo(bool b);

private:
    void createPipelines();
    void createItemPipeline();
    void createFloorPipeline();
    void ensureBuffers();
    void ensureInstanceBuffer();
    void getMatrices(QMatrix4x4 *mvp, QMatrix4x4 *model, QMatrix3x3 *modelNormal, QVector3D *eyePos);
    void writeFragUni(quint8 *p, const QVector3D &eyePos);
    void buildFrame();
    void buildDrawCallsForItems();
    void buildDrawCallsForFloor();

    void markViewProjDirty() { m_vpDirty = m_window->concurrentFrameCount(); }

    VulkanWindow *m_window;
    QVulkanDeviceFunctions *m_devFuncs;

    bool m_useLogo = false;
    Mesh m_blockMesh;
    Mesh m_logoMesh;
    VkBuffer m_blockVertexBuf = VK_NULL_HANDLE;
    VkBuffer m_logoVertexBuf = VK_NULL_HANDLE;
    struct {
        VkDeviceSize vertUniSize;
        VkDeviceSize fragUniSize;
        VkDeviceSize uniMemStartOffset;
        Shader vs;
        Shader fs;
        VkDescriptorPool descPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout descSetLayout = VK_NULL_HANDLE;
        VkDescriptorSet descSet;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
    } m_itemMaterial;

    VkBuffer m_floorVertexBuf = VK_NULL_HANDLE;
    struct {
        Shader vs;
        Shader fs;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
    } m_floorMaterial;

    VkDeviceMemory m_bufMem = VK_NULL_HANDLE;
    VkBuffer m_uniBuf = VK_NULL_HANDLE;

    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    QFuture<void> m_pipelinesFuture;

    QVector3D m_lightPos;
    Camera m_cam;

    QMatrix4x4 m_proj;
    int m_vpDirty = 0;
    QMatrix4x4 m_floorModel;

    bool m_animating;
    float m_rotation = 0.0f;

    int m_instCount;
    int m_preparedInstCount = 0;
    QByteArray m_instData;
    VkBuffer m_instBuf = VK_NULL_HANDLE;
    VkDeviceMemory m_instBufMem = VK_NULL_HANDLE;

    QFutureWatcher<void> m_frameWatcher;
    bool m_framePending;

    QMutex m_guiMutex;
};

#endif
