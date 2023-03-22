// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QVulkanInstance>
#include <QLoggingCategory>
#include "../shared/trianglerenderer.h"

Q_LOGGING_CATEGORY(lcVk, "qt.vulkan")

//! [2]
class VulkanWindow : public QVulkanWindow
{
public:
    QVulkanWindowRenderer *createRenderer() override;
};
//! [2]

QVulkanWindowRenderer *VulkanWindow::createRenderer()
{
    return new TriangleRenderer(this, true); // try MSAA, when available
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));

//! [0]
    QVulkanInstance inst;
    inst.setLayers({ "VK_LAYER_KHRONOS_validation" });
    if (!inst.create())
        qFatal("Failed to create Vulkan instance: %d", inst.errorCode());
//! [0]

//! [1]
    VulkanWindow w;
    w.setVulkanInstance(&inst);

    w.resize(1024, 768);
    w.show();
//! [1]

    return app.exec();
}
