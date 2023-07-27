// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QVulkanDeviceFunctions>
#include <QVulkanFunctions>
#include <QVulkanInstance>

namespace src_gui_vulkan_qvulkanfunctions {

struct Window {
    void init();
    QVulkanInstance *vulkanInstance() { return nullptr; }
};

//! [0]
void Window::init()
{
    QVulkanInstance *inst = vulkanInstance();
    QVulkanFunctions *f = inst->functions();
    // ...
    uint32_t count = 0;
    VkResult err = f->vkEnumeratePhysicalDevices(inst->vkInstance(), &count, nullptr);
    // ...
}
//! [0]

} // namespace src_gui_vulkan_qvulkanfunctions {


namespace src_gui_vulkan_qvulkanfunctions2 {
struct Window {
    void render();
    QVulkanInstance *vulkanInstance() { return nullptr; }
};
VkDevice_T *device = nullptr;
VkCommandBufferAllocateInfo cmdBufInfo;
VkCommandBuffer cmdBuf;

//! [1]
void Window::render()
{
    QVulkanInstance *inst = vulkanInstance();
    QVulkanDeviceFunctions *df = inst->deviceFunctions(device);
    VkResult err = df->vkAllocateCommandBuffers(device, &cmdBufInfo, &cmdBuf);
    // ...
}
//! [1]

} // src_gui_vulkan_qvulkanfunctions2
