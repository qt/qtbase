// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QVulkanFunctions>
#include <QVulkanInstance>
#include <QWindow>

namespace src_gui_vulkan_qvulkaninstance {

struct Window {
    void setVulkanInstance(QVulkanInstance *instance) { Q_UNUSED(instance); }
    void show();
};
Window *window = nullptr;


//! [0]
    int main(int argc, char **argv)
    {
        QGuiApplication app(argc, argv);

        QVulkanInstance inst;
        if (!inst.create())
            return 1;

        // ...
        window->setVulkanInstance(&inst);
        window->show();

        return app.exec();
    }
//! [0]


void wrapper0() {
//! [1]
    QVulkanInstance inst;

    // Enable validation layer, if supported. Messages go to qDebug by default.
    inst.setLayers({ "VK_LAYER_KHRONOS_validation" });

    bool ok = inst.create();
    if (!ok) {
        // ... Vulkan not available
    }

    if (!inst.layers().contains("VK_LAYER_KHRONOS_validation")) {
        // ... validation layer not available
    }
//! [1]
}


void wrapper1() {
//! [2]
    QVulkanInstance inst;

    if (inst.supportedLayers().contains("VK_LAYER_KHRONOS_validation")) {
        // ...
    }
    bool ok = inst.create();
    // ...
//! [2]

Q_UNUSED(ok);
} // wrapper1
} // src_gui_vulkan_qvulkaninstance


namespace src_gui_vulkan_qvulkaninstance2 {

//! [3]
class VulkanWindow : public QWindow
{
public:
    VulkanWindow() {
        setSurfaceType(VulkanSurface);
    }

    void exposeEvent(QExposeEvent *) {
        if (isExposed()) {
            if (!m_initialized) {
                m_initialized = true;
                // initialize device, swapchain, etc.
                QVulkanInstance *inst = vulkanInstance();
                QVulkanFunctions *f = inst->functions();
                uint32_t devCount = 0;
                f->vkEnumeratePhysicalDevices(inst->vkInstance(), &devCount, nullptr);
                // ...
                // build the first frame
                render();
            }
        }
    }

    bool event(QEvent *e) {
        if (e->type() == QEvent::UpdateRequest)
            render();
        return QWindow::event(e);
    }

    void render() {
       // ...
       requestUpdate(); // render continuously
    }

private:
    bool m_initialized = false;
};

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QVulkanInstance inst;
    if (!inst.create()) {
        qWarning("Vulkan not available");
        return 1;
    }

    VulkanWindow window;
    window.showMaximized();

    return app.exec();

}
//! [3]


} // src_gui_vulkan_qvulkaninstance2
