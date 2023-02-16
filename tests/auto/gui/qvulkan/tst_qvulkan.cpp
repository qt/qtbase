// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtGui/QVulkanInstance>
#include <QtGui/QVulkanFunctions>
#include <QtGui/QVulkanWindow>
#include <QtCore/qvarlengtharray.h>

#include <QTest>

#include <QSignalSpy>

class tst_QVulkan : public QObject
{
    Q_OBJECT

private slots:
    void vulkanInstance();
    void vulkanCheckSupported();
    void vulkanPlainWindow();
    void vulkanVersionRequest();
    void vulkan11();
    void vulkanWindow();
    void vulkanWindowRenderer();
    void vulkanWindowGrab();
};

void tst_QVulkan::vulkanInstance()
{
#ifdef Q_OS_ANDROID
    if (QNativeInterface::QAndroidApplication::sdkVersion() >= 31)
        QSKIP("Fails on Android 12 (QTBUG-111236)");
#endif
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
#ifdef Q_OS_ANDROID
    if (QNativeInterface::QAndroidApplication::sdkVersion() >= 31)
        QSKIP("Fails on Android 12 (QTBUG-111236)");
#endif
    // Test the early calls to supportedLayers/extensions/apiVersion that need
    // the library and some basics, but do not initialize the instance.
    QVulkanInstance inst;
    QVERIFY(!inst.isValid());

    QVulkanInfoVector<QVulkanLayer> vl = inst.supportedLayers();
    qDebug() << vl;
    QVERIFY(!inst.isValid());

    QVulkanInfoVector<QVulkanExtension> ve = inst.supportedExtensions();
    qDebug() << ve;
    QVERIFY(!inst.isValid());

    const QVersionNumber supportedApiVersion = inst.supportedApiVersion();
    qDebug() << supportedApiVersion.majorVersion() << supportedApiVersion.minorVersion();

    if (inst.create()) { // skip the rest when Vulkan is not supported at all
        QVERIFY(!ve.isEmpty());
        QVERIFY(ve == inst.supportedExtensions());
        QVERIFY(supportedApiVersion.majorVersion() >= 1);
    }
}

void tst_QVulkan::vulkan11()
{
#ifdef Q_OS_ANDROID
    if (QNativeInterface::QAndroidApplication::sdkVersion() >= 31)
        QSKIP("Fails on Android 12 (QTBUG-105739)");
#endif
#if VK_VERSION_1_1
    QVulkanInstance inst;
    if (inst.supportedApiVersion() < QVersionNumber(1, 1))
        QSKIP("Vulkan 1.1 is not supported by the VkInstance; skip");

    inst.setApiVersion(QVersionNumber(1, 1));
    if (!inst.create())
        QSKIP("Vulkan 1.1 instance creation failed; skip");

    QCOMPARE(inst.errorCode(), VK_SUCCESS);

    // exercise some 1.1 commands
    QVulkanFunctions *f = inst.functions();
    QVERIFY(f);
    uint32_t count = 0;
    VkResult err = f->vkEnumeratePhysicalDeviceGroups(inst.vkInstance(), &count, nullptr);
    if (err != VK_SUCCESS)
        QSKIP("No physical devices; skip");

    if (count) {
        QVarLengthArray<VkPhysicalDeviceGroupProperties, 4> groupProperties;
        groupProperties.resize(count);
        err = f->vkEnumeratePhysicalDeviceGroups(inst.vkInstance(), &count, groupProperties.data()); // 1.1 API
        QCOMPARE(err, VK_SUCCESS);
        for (const VkPhysicalDeviceGroupProperties &gp : groupProperties) {
            QCOMPARE(gp.sType, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES);
            for (uint32_t i = 0; i != gp.physicalDeviceCount; ++i) {
                VkPhysicalDevice physDev = gp.physicalDevices[i];

                // Instance and physical device apiVersion are two different things.
                VkPhysicalDeviceProperties props;
                f->vkGetPhysicalDeviceProperties(physDev, &props);
                QVersionNumber physDevVer(VK_VERSION_MAJOR(props.apiVersion),
                                          VK_VERSION_MINOR(props.apiVersion),
                                          VK_VERSION_PATCH(props.apiVersion));
                qDebug() << "Physical device" << physDev << "apiVersion" << physDevVer;

                if (physDevVer >= QVersionNumber(1, 1)) {
                    // Now that we ensured that we have an 1.1 capable instance and physical device,
                    // query something that was not in 1.0.
                    VkPhysicalDeviceIDProperties deviceIdProps = {}; // new in 1.1
                    deviceIdProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
                    VkPhysicalDeviceProperties2 props2 = {};
                    props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
                    props2.pNext = &deviceIdProps;
                    f->vkGetPhysicalDeviceProperties2(physDev, &props2); // 1.1 API
                    QByteArray deviceUuid = QByteArray::fromRawData((const char *) deviceIdProps.deviceUUID, VK_UUID_SIZE).toHex();
                    QByteArray driverUuid = QByteArray::fromRawData((const char *) deviceIdProps.driverUUID, VK_UUID_SIZE).toHex();
                    qDebug() << "deviceUUID" << deviceUuid << "driverUUID" << driverUuid;
                    const bool deviceUuidZero = std::find_if(deviceUuid.cbegin(), deviceUuid.cend(), [](char c) -> bool { return c; }) == deviceUuid.cend();
                    const bool driverUuidZero = std::find_if(driverUuid.cbegin(), driverUuid.cend(), [](char c) -> bool { return c; }) == driverUuid.cend();
                    // deviceUUID cannot be all zero as per spec
                    if (!driverUuidZero) {
                        // ...but then there are implementations such as some
                        // versions of Mesa lavapipe, that returns all zeroes
                        // for both uuids. skip the check if the driver uuid
                        // was zero too.
                        // https://gitlab.freedesktop.org/mesa/mesa/-/issues/5875
                        QVERIFY(!deviceUuidZero);
                    }
                } else {
                    qDebug("Physical device is not Vulkan 1.1 capable");
                }
            }
        }
    }

#else
    QSKIP("Vulkan header is not 1.1 capable; skip");
#endif
}

void tst_QVulkan::vulkanPlainWindow()
{
#ifdef Q_OS_ANDROID
    QSKIP("Fails on Android 7 emulator (QTBUG-108328)");
#endif

    QVulkanInstance inst;
    if (!inst.create())
        QSKIP("Vulkan init failed; skip");

    QWindow w;
    w.setSurfaceType(QSurface::VulkanSurface);
    w.setVulkanInstance(&inst);
    w.resize(1024, 768);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

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
#ifdef Q_OS_ANDROID
    if (QNativeInterface::QAndroidApplication::sdkVersion() >= 31)
        QSKIP("Fails on Android 12 (QTBUG-111236)");
#endif
    QVulkanInstance inst;
    if (!inst.create())
        QSKIP("Vulkan init failed; skip");

    // Now that we know Vulkan is functional, check the requested apiVersion is
    // passed to vkCreateInstance as expected.

    inst.destroy();

    inst.setApiVersion(QVersionNumber(10, 0, 0));

    bool result = inst.create();

    // Starting with Vulkan 1.1 the spec does not allow the implementation to
    // fail the instance creation. So check for the 1.0 behavior only when
    // create() failed, skip this verification with 1.1+ (where create() will
    // succeed for any bogus api version).
    if (!result)
        QCOMPARE(inst.errorCode(), VK_ERROR_INCOMPATIBLE_DRIVER);

    inst.destroy();

    // Verify that specifying the version returned from supportedApiVersion
    // (either 1.0.0 or what vkEnumerateInstanceVersion returns in Vulkan 1.1+)
    // leads to successful instance creation.
    inst.setApiVersion(inst.supportedApiVersion());
    result = inst.create();
    QVERIFY(result);
}

static void waitForUnexposed(QWindow *w)
{
    QElapsedTimer timer;
    timer.start();
    while (w->isExposed()) {
        int remaining = 5000 - int(timer.elapsed());
        if (remaining <= 0)
            break;
        QCoreApplication::processEvents(QEventLoop::AllEvents, remaining);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QTest::qSleep(10);
    }
}

void tst_QVulkan::vulkanWindow()
{
#ifdef Q_OS_ANDROID
    if (QNativeInterface::QAndroidApplication::sdkVersion() >= 31)
        QSKIP("Fails on Android 12 (QTBUG-111236)");
#endif
    QVulkanInstance inst;
    if (!inst.create())
        QSKIP("Vulkan init failed; skip");

    // First let's forget to set the instance.
    QVulkanWindow w;
    QVERIFY(!w.isValid());
    w.resize(1024, 768);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QVERIFY(!w.isValid());

    // Now set it. A simple hide - show should be enough to correct, this, no
    // need for a full destroy - create.
    w.hide();
    waitForUnexposed(&w);
    w.setVulkanInstance(&inst);
    QList<VkPhysicalDeviceProperties> pdevs = w.availablePhysicalDevices();
    if (pdevs.isEmpty())
        QSKIP("No Vulkan physical devices; skip");
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QVERIFY(w.isValid());
    QCOMPARE(w.vulkanInstance(), &inst);
    QVulkanInfoVector<QVulkanExtension> exts = w.supportedDeviceExtensions();

    // Now destroy and recreate.
    w.destroy();
    waitForUnexposed(&w);
    QVERIFY(!w.isValid());
    // check that flags can be set between a destroy() - show()
    w.setFlags(QVulkanWindow::PersistentResources);
    // supported lists can be queried before expose too
    QVERIFY(w.supportedDeviceExtensions() == exts);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QVERIFY(w.isValid());
    QVERIFY(w.flags().testFlag(QVulkanWindow::PersistentResources));

    QVERIFY(w.physicalDevice() != VK_NULL_HANDLE);
    QVERIFY(w.physicalDeviceProperties() != nullptr);
    QVERIFY(w.device() != VK_NULL_HANDLE);
    QVERIFY(w.graphicsQueue() != VK_NULL_HANDLE);
    QVERIFY(w.graphicsCommandPool() != VK_NULL_HANDLE);
    QVERIFY(w.defaultRenderPass() != VK_NULL_HANDLE);

    QVERIFY(w.concurrentFrameCount() > 0);
    QVERIFY(w.concurrentFrameCount() <= QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT);
}

class TestVulkanRenderer;

class TestVulkanWindow : public QVulkanWindow
{
public:
    QVulkanWindowRenderer *createRenderer() override;

private:
    TestVulkanRenderer *m_renderer = nullptr;
};

struct TestVulkan {
    int preInitResCount = 0;
    int initResCount = 0;
    int initSwcResCount = 0;
    int releaseResCount = 0;
    int releaseSwcResCount = 0;
    int startNextFrameCount = 0;
} testVulkan;

class TestVulkanRenderer : public QVulkanWindowRenderer
{
public:
    TestVulkanRenderer(QVulkanWindow *w) : m_window(w) { }

    void preInitResources() override;
    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;

    void startNextFrame() override;

private:
    QVulkanWindow *m_window;
    QVulkanDeviceFunctions *m_devFuncs;
};

void TestVulkanRenderer::preInitResources()
{
    if (testVulkan.initResCount) {
        qWarning("initResources called before preInitResources?!");
        testVulkan.preInitResCount = -1;
        return;
    }

    // Ensure the physical device and the surface are available at this stage.
    VkPhysicalDevice physDev = m_window->physicalDevice();
    if (physDev == VK_NULL_HANDLE) {
        qWarning("No physical device in preInitResources");
        testVulkan.preInitResCount = -1;
        return;
    }
    VkSurfaceKHR surface = m_window->vulkanInstance()->surfaceForWindow(m_window);
    if (surface == VK_NULL_HANDLE) {
        qWarning("No surface in preInitResources");
        testVulkan.preInitResCount = -1;
        return;
    }

    ++testVulkan.preInitResCount;
}

void TestVulkanRenderer::initResources()
{
    m_devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    ++testVulkan.initResCount;
}

void TestVulkanRenderer::initSwapChainResources()
{
    ++testVulkan.initSwcResCount;
}

void TestVulkanRenderer::releaseSwapChainResources()
{
    ++testVulkan.releaseSwcResCount;
}

void TestVulkanRenderer::releaseResources()
{
    ++testVulkan.releaseResCount;
}

void TestVulkanRenderer::startNextFrame()
{
    ++testVulkan.startNextFrameCount;

    VkClearColorValue clearColor = { 0, 1, 0, 1 };
    VkClearDepthStencilValue clearDS = { 1, 0 };
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

    m_devFuncs->vkCmdEndRenderPass(cmdBuf);

    m_window->frameReady();
}

QVulkanWindowRenderer *TestVulkanWindow::createRenderer()
{
    Q_ASSERT(!m_renderer);
    m_renderer = new TestVulkanRenderer(this);
    return m_renderer;
}

void tst_QVulkan::vulkanWindowRenderer()
{
    QVulkanInstance inst;
    if (!inst.create())
        QSKIP("Vulkan init failed; skip");

    testVulkan = TestVulkan();

    TestVulkanWindow w;
    w.setVulkanInstance(&inst);
    w.resize(1024, 768);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    if (w.availablePhysicalDevices().isEmpty())
        QSKIP("No Vulkan physical devices; skip");

    QVERIFY(testVulkan.preInitResCount == 1);
    QVERIFY(testVulkan.initResCount == 1);
    QVERIFY(testVulkan.initSwcResCount == 1);
    // this has to be QTRY due to the async update in QVulkanWindowPrivate::ensureStarted()
    QTRY_VERIFY(testVulkan.startNextFrameCount >= 1);

    QVERIFY(!w.swapChainImageSize().isEmpty());
    QVERIFY(w.colorFormat() != VK_FORMAT_UNDEFINED);
    QVERIFY(w.depthStencilFormat() != VK_FORMAT_UNDEFINED);

    w.destroy();
    waitForUnexposed(&w);
    QVERIFY(testVulkan.releaseSwcResCount == 1);
    QVERIFY(testVulkan.releaseResCount == 1);
}

void tst_QVulkan::vulkanWindowGrab()
{
#ifdef Q_OS_ANDROID
    if (QNativeInterface::QAndroidApplication::sdkVersion() >= 31)
        QSKIP("Fails on Android 12 (QTBUG-105739)");
#endif
    QVulkanInstance inst;
    inst.setLayers(QByteArrayList() << "VK_LAYER_KHRONOS_validation");
    if (!inst.create())
        QSKIP("Vulkan init failed; skip");

    testVulkan = TestVulkan();

    TestVulkanWindow w;
    w.setVulkanInstance(&inst);
    w.resize(1024, 768);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    if (w.availablePhysicalDevices().isEmpty())
        QSKIP("No Vulkan physical devices; skip");

    if (!w.supportsGrab())
        QSKIP("No grab support; skip");

    QVERIFY(!w.swapChainImageSize().isEmpty());

    QImage img1 = w.grab();
    QImage img2 = w.grab();
    QImage img3 = w.grab();

    QVERIFY(!img1.isNull());
    QVERIFY(!img2.isNull());
    QVERIFY(!img3.isNull());

    QCOMPARE(img1.size(), w.swapChainImageSize());
    QCOMPARE(img2.size(), w.swapChainImageSize());
    QCOMPARE(img3.size(), w.swapChainImageSize());

    QRgb a = img1.pixel(10, 20);
    QRgb b = img2.pixel(5, 5);
    QRgb c = img3.pixel(50, 30);

    QCOMPARE(a, b);
    QCOMPARE(b, c);
    QRgb refPixel = qRgb(0, 255, 0);

    int redFuzz = qAbs(qRed(a) - qRed(refPixel));
    int greenFuzz = qAbs(qGreen(a) - qGreen(refPixel));
    int blueFuzz = qAbs(qBlue(a) - qBlue(refPixel));

    QVERIFY(redFuzz <= 1);
    QVERIFY(blueFuzz <= 1);
    QVERIFY(greenFuzz <= 1);

    w.destroy();
}

QTEST_MAIN(tst_QVulkan)

#include "tst_qvulkan.moc"
