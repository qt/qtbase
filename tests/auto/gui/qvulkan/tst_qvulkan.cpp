/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/QVulkanInstance>
#include <QtGui/QVulkanFunctions>
#include <QtGui/QWindow>

#include <QtTest/QtTest>

#include <QSignalSpy>

class tst_QVulkan : public QObject
{
    Q_OBJECT

private slots:
    void vulkanInstance();
    void vulkanCheckSupported();
    void vulkanWindow();
    void vulkanVersionRequest();
};

void tst_QVulkan::vulkanInstance()
{
    QVulkanInstance inst;
    if (!inst.create())
        QSKIP("Vulkan init failed; skip");

    QVERIFY(inst.isValid());
    QVERIFY(inst.vkInstance() != VK_NULL_HANDLE);
    QVERIFY(inst.functions());
    QVERIFY(!inst.flags().testFlag(QVulkanInstance::NoDebugOutputRedirect));

    inst.destroy();

    QVERIFY(!inst.isValid());
    QVERIFY(inst.handle() == nullptr);

    inst.setFlags(QVulkanInstance::NoDebugOutputRedirect);
    // pass a bogus layer and extension
    inst.setExtensions(QByteArrayList() << "abcdefg" << "notanextension");
    inst.setLayers(QByteArrayList() << "notalayer");
    QVERIFY(inst.create());

    QVERIFY(inst.isValid());
    QVERIFY(inst.vkInstance() != VK_NULL_HANDLE);
    QVERIFY(inst.handle() != nullptr);
    QVERIFY(inst.functions());
    QVERIFY(inst.flags().testFlag(QVulkanInstance::NoDebugOutputRedirect));
    QVERIFY(!inst.extensions().contains("abcdefg"));
    QVERIFY(!inst.extensions().contains("notanextension"));
    QVERIFY(!inst.extensions().contains("notalayer"));
    // at least the surface extensions should be there however
    QVERIFY(inst.extensions().contains("VK_KHR_surface"));

    QVERIFY(inst.getInstanceProcAddr("vkGetDeviceQueue"));
}

void tst_QVulkan::vulkanCheckSupported()
{
    // Test the early calls to supportedLayers/extensions that need the library
    // and some basics, but do not initialize the instance.
    QVulkanInstance inst;
    QVERIFY(!inst.isValid());

    QVulkanInfoVector<QVulkanLayer> vl = inst.supportedLayers();
    qDebug() << vl;
    QVERIFY(!inst.isValid());

    QVulkanInfoVector<QVulkanExtension> ve = inst.supportedExtensions();
    qDebug() << ve;
    QVERIFY(!inst.isValid());

    if (inst.create()) { // skip the rest when Vulkan is not supported at all
        QVERIFY(!ve.isEmpty());
        QVERIFY(ve == inst.supportedExtensions());
    }
}

void tst_QVulkan::vulkanWindow()
{
    QVulkanInstance inst;
    if (!inst.create())
        QSKIP("Vulkan init failed; skip");

    QWindow w;
    w.setSurfaceType(QSurface::VulkanSurface);
    w.setVulkanInstance(&inst);
    w.resize(1024, 768);
    w.show();
    QTest::qWaitForWindowExposed(&w);

    QCOMPARE(w.vulkanInstance(), &inst);

    VkSurfaceKHR surface = QVulkanInstance::surfaceForWindow(&w);
    QVERIFY(surface != VK_NULL_HANDLE);

    // exercise supportsPresent (and QVulkanFunctions) a bit
    QVulkanFunctions *f = inst.functions();
    VkPhysicalDevice physDev;
    uint32_t count = 1;
    VkResult err = f->vkEnumeratePhysicalDevices(inst.vkInstance(), &count, &physDev);
    if (err != VK_SUCCESS)
        QSKIP("No physical devices; skip");

    VkPhysicalDeviceProperties physDevProps;
    f->vkGetPhysicalDeviceProperties(physDev, &physDevProps);
    qDebug("Device name: %s Driver version: %d.%d.%d", physDevProps.deviceName,
           VK_VERSION_MAJOR(physDevProps.driverVersion), VK_VERSION_MINOR(physDevProps.driverVersion),
           VK_VERSION_PATCH(physDevProps.driverVersion));

    bool supports = inst.supportsPresent(physDev, 0, &w);
    qDebug("queue family 0 supports presenting to window = %d", supports);
}

void tst_QVulkan::vulkanVersionRequest()
{
    QVulkanInstance inst;
    if (!inst.create())
        QSKIP("Vulkan init failed; skip");

    // Now that we know Vulkan is functional, check the requested apiVersion is
    // passed to vkCreateInstance as expected.

    inst.destroy();

    inst.setApiVersion(QVersionNumber(10, 0, 0));
    QVERIFY(!inst.create());
    QCOMPARE(inst.errorCode(), VK_ERROR_INCOMPATIBLE_DRIVER);
}

QTEST_MAIN(tst_QVulkan)

#include "tst_qvulkan.moc"
