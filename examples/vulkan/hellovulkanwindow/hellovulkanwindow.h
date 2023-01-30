// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef HELLOVULKANWINDOW_H
#define HELLOVULKANWINDOW_H

#include <QVulkanWindow>

//! [0]
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
    QVulkanWindow *m_window;
    QVulkanDeviceFunctions *m_devFuncs;
    float m_green = 0;
};

class VulkanWindow : public QVulkanWindow
{
public:
    QVulkanWindowRenderer *createRenderer() override;
};
//! [0]

#endif // HELLOVULKANWINDOW_H
