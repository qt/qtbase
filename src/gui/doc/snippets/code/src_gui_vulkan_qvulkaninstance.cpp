/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//! [0]
    int main(int argc, char **argv)
    {
        QGuiApplication app(argc, argv);

        QVulkanInstance inst;
        if (!inst.create())
            return 1;

        ...
        window->setVulkanInstance(&inst);
        window->show();

        return app.exec();
    }
//! [0]

//! [1]
    QVulkanInstance inst;

    // Enable validation layer, if supported. Messages go to qDebug by default.
    inst.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");

    bool ok = inst.create();
    if (!ok)
        ... // Vulkan not available
    if (!inst.layers().contains("VK_LAYER_LUNARG_standard_validation"))
        ... // validation layer not available
//! [1]

//! [2]
    QVulkanInstance inst;

    if (inst.supportedLayers().contains("VK_LAYER_LUNARG_standard_validation"))
        ...

    bool ok = inst.create();
    ...
//! [2]

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
                    ...
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
           ...
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
