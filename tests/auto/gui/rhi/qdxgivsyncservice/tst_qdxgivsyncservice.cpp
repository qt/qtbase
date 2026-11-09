// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QWindow>
#include <rhi/qrhi.h>
#include <QtGui/private/qdxgivsyncservice_p.h>

#include <atomic>
#include <memory>

namespace {

// A window with its own QRhi, like a QQuickWindow.
struct RhiWindow
{
    bool create(const QPoint &pos)
    {
        window.setSurfaceType(QSurface::Direct3DSurface);
        window.setGeometry(QRect(pos, QSize(320, 240)));
        window.show();
        if (!QTest::qWaitForWindowExposed(&window))
            return false;
        QRhiD3D11InitParams params;
        rhi.reset(QRhi::create(QRhi::D3D11, &params));
        if (!rhi)
            return false;
        swapChain.reset(rhi->newSwapChain());
        swapChain->setWindow(&window);
        renderPass.reset(swapChain->newCompatibleRenderPassDescriptor());
        swapChain->setRenderPassDescriptor(renderPass.get());
        return swapChain->createOrResize();
    }

    void destroy()
    {
        renderPass.reset();
        swapChain.reset();
        rhi.reset();
        window.destroy();
    }

    LUID adapterLuid() const
    {
        const auto *h = static_cast<const QRhiD3D11NativeHandles *>(rhi->nativeHandles());
        return { h->adapterLuidLow, h->adapterLuidHigh };
    }

    QWindow window;
    std::unique_ptr<QRhi> rhi;
    std::unique_ptr<QRhiSwapChain> swapChain;
    std::unique_ptr<QRhiRenderPassDescriptor> renderPass;
};

class VBlankCounter
{
public:
    explicit VBlankCounter(QWindow *window)
        : m_window(window)
    {
        m_id = QDxgiVSyncService::instance()->registerCallback(
                [this](const QDxgiVSyncService::CallbackWindowList &windows, qint64) {
                    if (windows.contains(m_window))
                        m_count.fetch_add(1);
                });
    }
    ~VBlankCounter() { QDxgiVSyncService::instance()->unregisterCallback(m_id); }

    void reset() { m_count.store(0); }
    int count() const { return m_count.load(); }

private:
    QWindow *m_window;
    qsizetype m_id = 0;
    std::atomic<int> m_count = 0;
};

} // namespace

class tst_QDxgiVSyncService : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void closeOneOfTwoWindows_data();
    void closeOneOfTwoWindows();
};

void tst_QDxgiVSyncService::initTestCase()
{
    if (qEnvironmentVariableIntValue("QT_D3D_NO_VBLANK_THREAD"))
        QSKIP("The vblank service is disabled by QT_D3D_NO_VBLANK_THREAD");
    QRhiD3D11InitParams params;
    if (!QRhi::probe(QRhi::D3D11, &params))
        QSKIP("D3D11 is not available");
}

void tst_QDxgiVSyncService::closeOneOfTwoWindows_data()
{
    QTest::addColumn<bool>("recreateFactory");

    QTest::newRow("factory unchanged") << false;
    QTest::newRow("factory recreated") << true; // QTBUG-152049
}

void tst_QDxgiVSyncService::closeOneOfTwoWindows()
{
    QFETCH(const bool, recreateFactory);
#ifndef QT_BUILD_INTERNAL
    if (recreateFactory)
        QSKIP("Simulating a stale DXGI factory requires a developer build");
#endif

    RhiWindow a, b;
    QVERIFY(a.create(QPoint(100, 100)));
    QVERIFY(b.create(QPoint(200, 200)));

    const LUID luidA = a.adapterLuid();
    const LUID luidB = b.adapterLuid();
    if (luidA.LowPart != luidB.LowPart || luidA.HighPart != luidB.HighPart)
        QSKIP("The two QRhi instances use different adapters");

    QDxgiVSyncService *service = QDxgiVSyncService::instance();
    if (!service->supportsWindow(&a.window))
        QSKIP("No DXGI output for the window (e.g. WARP)");

    VBlankCounter counter(&a.window);
    QTRY_VERIFY(counter.count() > 0);

    if (recreateFactory) {
#ifdef QT_BUILD_INTERNAL
        service->simulateStaleFactory();
#endif
        counter.reset();
        QTRY_VERIFY(counter.count() > 0);
    }

    // Each QRhi holds its own reference to the adapter, so destroying one of
    // them must not stop the vblank notifications for the other window.
    b.destroy();
    counter.reset();
    QTRY_VERIFY(counter.count() > 0);
    QVERIFY(service->supportsWindow(&a.window));
}

QTEST_MAIN(tst_QDxgiVSyncService)

#include "tst_qdxgivsyncservice.moc"
