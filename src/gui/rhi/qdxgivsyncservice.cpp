// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdxgivsyncservice_p.h"
#include <QThread>
#include <QWaitCondition>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QScreen>
#include <QVarLengthArray>
#include <QtCore/private/qsystemerror_p.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcQpaScreenUpdates, "qt.qpa.screen.updates", QtCriticalMsg);

class QDxgiVSyncThread : public QThread
{
public:
    // the HMONITOR is unique (i.e. identifies the output), the IDXGIOutput (the pointer/object itself) is not
    using Callback = std::function<void(IDXGIOutput *, HMONITOR, qint64)>;
    QDxgiVSyncThread(IDXGIOutput *output, float vsyncIntervalMsReportedForScreen, Callback callback);
    void stop(); // to be called from a thread that's not this thread
    void run() override;

private:
    IDXGIOutput *output;
    float vsyncIntervalMsReportedForScreen;
    Callback callback;
    HMONITOR monitor;
    QAtomicInt quit;
    QMutex mutex;
    QWaitCondition cond;
};

QDxgiVSyncThread::QDxgiVSyncThread(IDXGIOutput *output, float vsyncIntervalMsReportedForScreen, Callback callback)
    : output(output),
      vsyncIntervalMsReportedForScreen(vsyncIntervalMsReportedForScreen),
      callback(callback)
{
    DXGI_OUTPUT_DESC desc;
    output->GetDesc(&desc);
    monitor = desc.Monitor;
}

void QDxgiVSyncThread::run()
{
    qCDebug(lcQpaScreenUpdates) << "QDxgiVSyncThread" << this << "for output" << output << "monitor" << monitor << "entered run()";
    QElapsedTimer timestamp;
    QElapsedTimer elapsed;
    timestamp.start();
    while (!quit.loadAcquire()) {
        elapsed.start();
        HRESULT hr = output->WaitForVBlank();
        if (FAILED(hr) || elapsed.nsecsElapsed() <= 1000000) {
            // 1 ms minimum; if less than that was spent in WaitForVBlank
            // (reportedly can happen e.g. when a screen gets powered on/off?),
            // or it reported an error, do a sleep; spinning unthrottled is
            // never acceptable
            QThread::msleep((unsigned long) vsyncIntervalMsReportedForScreen);
        } else {
            callback(output, monitor, timestamp.nsecsElapsed());
        }
    }
    qCDebug(lcQpaScreenUpdates) << "QDxgiVSyncThread" << this << "is stopping";
    mutex.lock();
    cond.wakeOne();
    mutex.unlock();
    qCDebug(lcQpaScreenUpdates) << "QDxgiVSyncThread" << this << "run() out";
}

void QDxgiVSyncThread::stop()
{
    mutex.lock();
    qCDebug(lcQpaScreenUpdates) << "Requesting QDxgiVSyncThread stop from thread" << QThread::currentThread() << "on" << this;
    if (isRunning() && !quit.loadAcquire()) {
        quit.storeRelease(1);
        cond.wait(&mutex);
    }
    wait();
    mutex.unlock();
}

QDxgiVSyncService *QDxgiVSyncService::instance()
{
    static QDxgiVSyncService service;
    return &service;
}

QDxgiVSyncService::QDxgiVSyncService()
{
    qCDebug(lcQpaScreenUpdates) << "New QDxgiVSyncService" << this;

    disableService = qEnvironmentVariableIntValue("QT_D3D_NO_VBLANK_THREAD");
    if (disableService) {
        qCDebug(lcQpaScreenUpdates) << "QDxgiVSyncService disabled by environment";
        return;
    }
}

QDxgiVSyncService::~QDxgiVSyncService()
{
    qCDebug(lcQpaScreenUpdates) << "~QDxgiVSyncService" << this;

    // Deadlock is almost guaranteed if we try to clean up here, when the global static is being destructed.
    // Must have been done earlier.
    if (dxgiFactory)
        qWarning("QDxgiVSyncService not destroyed in time");
}

void QDxgiVSyncService::global_destroy()
{
    QDxgiVSyncService *inst = QDxgiVSyncService::instance();
    inst->cleanupRegistered = false;
    inst->destroy();
}

void QDxgiVSyncService::destroy()
{
    qCDebug(lcQpaScreenUpdates) << "QDxgiVSyncService::destroy()";

    if (disableService)
        return;

    for (auto it = windows.begin(), end = windows.end(); it != end; ++it)
        cleanupWindowData(&*it);
    windows.clear();

    teardownDxgi();
}

void QDxgiVSyncService::teardownDxgi()
{
    for (auto it = adapters.begin(), end = adapters.end(); it != end; ++it)
        cleanupAdapterData(&*it);
    adapters.clear();

    if (dxgiFactory) {
        dxgiFactory->Release();
        dxgiFactory = nullptr;
    }

    qCDebug(lcQpaScreenUpdates) << "QDxgiVSyncService DXGI teardown complete";
}

void QDxgiVSyncService::beginFrame(LUID)
{
    QMutexLocker lock(&mutex);
    if (disableService)
        return;

    // Handle "the possible need to re-create the factory and re-enumerate
    // adapters". At the time of writing the QRhi D3D11 and D3D12 backends do
    // not handle this at all (and rendering does not actually break or stop
    // just because the factory says !IsCurrent), whereas here it makes more
    // sense to act since we may want to get rid of threads that are no longer
    // needed. Keep the adapter IDs and the registered windows, drop everything
    // else, then start from scratch.

    if (dxgiFactory && !dxgiFactory->IsCurrent()) {
        qCDebug(lcQpaScreenUpdates, "QDxgiVSyncService: DXGI Factory is no longer Current");
        QVarLengthArray<LUID, 8> luids;
        for (auto it = adapters.begin(), end = adapters.end(); it != end; ++it)
            luids.append(it->luid);
        for (auto it = windows.begin(), end = windows.end(); it != end; ++it)
            cleanupWindowData(&*it);
        lock.unlock();
        teardownDxgi();
        for (LUID luid : luids)
            refAdapter(luid);
        lock.relock();
        for (auto it = windows.begin(), end = windows.end(); it != end; ++it)
            updateWindowData(it.key(), &*it);
    }
}

void QDxgiVSyncService::refAdapter(LUID luid)
{
    QMutexLocker lock(&mutex);
    if (disableService)
        return;

    if (!dxgiFactory) {
        HRESULT hr = CreateDXGIFactory2(0, __uuidof(IDXGIFactory2), reinterpret_cast<void **>(&dxgiFactory));
        if (FAILED(hr)) {
            disableService = true;
            qWarning("QDxgiVSyncService: CreateDXGIFactory2 failed: %s", qPrintable(QSystemError::windowsComString(hr)));
            return;
        }
        if (!cleanupRegistered) {
            qAddPostRoutine(QDxgiVSyncService::global_destroy);
            cleanupRegistered = true;
        }
    }

    for (AdapterData &a : adapters) {
        if (a.luid.LowPart == luid.LowPart && a.luid.HighPart == luid.HighPart) {
            a.ref += 1;
            return;
        }
    }

    AdapterData a;
    a.ref = 1;
    a.luid = luid;
    a.adapter = nullptr;

    IDXGIAdapter1 *ad;
    for (int adapterIndex = 0; dxgiFactory->EnumAdapters1(UINT(adapterIndex), &ad) != DXGI_ERROR_NOT_FOUND; ++adapterIndex) {
        DXGI_ADAPTER_DESC1 desc;
        ad->GetDesc1(&desc);
        if (desc.AdapterLuid.LowPart == luid.LowPart && desc.AdapterLuid.HighPart == luid.HighPart) {
            a.adapter = ad;
            break;
        }
        ad->Release();
    }

    if (!a.adapter) {
        qWarning("VSyncService: Failed to find adapter (via EnumAdapters1), skipping");
        return;
    }

    adapters.append(a);

    qCDebug(lcQpaScreenUpdates) << "QDxgiVSyncService refAdapter for not yet seen adapter" << luid.LowPart << luid.HighPart;

    // windows may have been registered before any adapters
    for (auto it = windows.begin(), end = windows.end(); it != end; ++it)
        updateWindowData(it.key(), &*it);
}

void QDxgiVSyncService::derefAdapter(LUID luid)
{
    QVarLengthArray<AdapterData, 4> cleanupList;

    {
        QMutexLocker lock(&mutex);
        if (disableService)
            return;

        for (qsizetype i = 0; i < adapters.count(); ++i) {
            AdapterData &a(adapters[i]);
            if (a.luid.LowPart == luid.LowPart && a.luid.HighPart == luid.HighPart) {
                if (!--a.ref) {
                    cleanupList.append(a);
                    adapters.removeAt(i);
                }
                break;
            }
        }
    }

    // the lock must *not* be held when triggering cleanup
    for (AdapterData &a : cleanupList)
        cleanupAdapterData(&a);
}

void QDxgiVSyncService::cleanupAdapterData(AdapterData *a)
{
    for (auto it = a->notifiers.begin(), end = a->notifiers.end(); it != end; ++it) {
        qCDebug(lcQpaScreenUpdates) << "QDxgiVSyncService::cleanupAdapterData(): about to call stop()";
        it->thread->stop();
        qCDebug(lcQpaScreenUpdates) << "QDxgiVSyncService::cleanupAdapterData(): stop() called";
        delete it->thread;
        it->output->Release();
    }
    a->notifiers.clear();

    a->adapter->Release();
    a->adapter = nullptr;
}

void QDxgiVSyncService::cleanupWindowData(WindowData *w)
{
    if (w->output) {
        w->output->Release();
        w->output = nullptr;
    }
}

static IDXGIOutput *outputForWindow(QWindow *w, IDXGIAdapter *adapter)
{
    // Generic canonical solution as per
    // https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-getcontainingoutput
    // and
    // https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range

    QRect wr = w->geometry();
    wr = QRect(wr.topLeft() * w->devicePixelRatio(), wr.size() * w->devicePixelRatio());
    const QPoint center = wr.center();
    IDXGIOutput *currentOutput = nullptr;
    IDXGIOutput *output = nullptr;
    for (UINT i = 0; adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);
        const RECT r = desc.DesktopCoordinates;
        const QRect dr(QPoint(r.left, r.top), QPoint(r.right - 1, r.bottom - 1));
        if (dr.contains(center)) {
            currentOutput = output;
            break;
        } else {
            output->Release();
        }
    }
    return currentOutput; // has a ref on it, will need Release by caller
}

void QDxgiVSyncService::updateWindowData(QWindow *window, WindowData *wd)
{
    for (auto it = adapters.begin(), end = adapters.end(); it != end; ++it) {
        IDXGIOutput *output = outputForWindow(window, it->adapter);
        if (!output)
            continue;

        // Two windows on the same screen may well return two different
        // IDXGIOutput pointers due to enumerating outputs every time; always
        // compare the HMONITOR, not the pointer itself.

        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);

        if (wd->output && wd->output != output) {
            if (desc.Monitor == wd->monitor) {
                output->Release();
                return;
            }
            wd->output->Release();
        }

        wd->output = output;
        wd->monitor = desc.Monitor;

        QScreen *screen = window->screen();
        const qreal refresh = screen ? screen->refreshRate() : 60;
        wd->reportedRefreshIntervalMs = refresh > 0 ? 1000.0f / float(refresh) : 1000.f / 60.0f;

        qCDebug(lcQpaScreenUpdates) << "QDxgiVSyncService: Output for window" << window
                                    << "on the actively used adapters is now" << output
                                    << "HMONITOR" << wd->monitor
                                    << "refresh" << wd->reportedRefreshIntervalMs;

        if (!it->notifiers.contains(wd->monitor)) {
            output->AddRef();
            QDxgiVSyncThread *t = new QDxgiVSyncThread(output, wd->reportedRefreshIntervalMs,
            [this](IDXGIOutput *, HMONITOR monitor, qint64 timestampNs) {
                CallbackWindowList w;
                QMutexLocker lock(&mutex);
                for (auto it = windows.cbegin(), end = windows.cend(); it != end; ++it) {
                    if (it->output && it->monitor == monitor)
                        w.append(it.key());
                }
                if (!w.isEmpty()) {
#if 0
                    qDebug() << "vsync thread" << QThread::currentThread() << monitor << "window list" << w << timestampNs;
#endif
                    for (const Callback &cb : std::as_const(callbacks)) {
                        if (cb)
                            cb(w, timestampNs);
                    }
                }
            });
            t->start(QThread::TimeCriticalPriority);
            it->notifiers.insert(wd->monitor, { wd->output, t });
        }
        return;
    }

    // If we get here, there is no IDXGIOutput and supportsWindow() will return false for this window.
    // This is perfectly normal when using an adapter such as WARP.
}

void QDxgiVSyncService::registerWindow(QWindow *window)
{
    QMutexLocker lock(&mutex);
    if (disableService || windows.contains(window))
        return;

    qCDebug(lcQpaScreenUpdates) << "QDxgiVSyncService: adding window" << window;

    WindowData wd;
    wd.output = nullptr;
    updateWindowData(window, &wd);
    windows.insert(window, wd);

    QObject::connect(window, &QWindow::screenChanged, window, [this, window](QScreen *screen) {
        qCDebug(lcQpaScreenUpdates) << "QDxgiVSyncService: screen changed for window:" << window << screen;
        QMutexLocker lock(&mutex);
        auto it = windows.find(window);
        if (it != windows.end())
            updateWindowData(window, &*it);
    }, Qt::QueuedConnection); // intentionally using Queued
    // It has been observed that with DirectConnection _sometimes_ we do not
    // find any IDXGIOutput for the window when moving it to a different screen.
    // Add a delay by going through the event loop.
}

void QDxgiVSyncService::unregisterWindow(QWindow *window)
{
    QMutexLocker lock(&mutex);
    auto it = windows.find(window);
    if (it == windows.end())
        return;

    qCDebug(lcQpaScreenUpdates) << "QDxgiVSyncService: removing window" << window;

    cleanupWindowData(&*it);

    windows.remove(window);
}

bool QDxgiVSyncService::supportsWindow(QWindow *window)
{
    QMutexLocker lock(&mutex);
    auto it = windows.constFind(window);
    return it != windows.cend() ? (it->output != nullptr) : false;
}

qsizetype QDxgiVSyncService::registerCallback(Callback cb)
{
    QMutexLocker lock(&mutex);
    for (qsizetype i = 0; i < callbacks.count(); ++i) {
        if (!callbacks[i]) {
            callbacks[i] = cb;
            return i + 1;
        }
    }
    callbacks.append(cb);
    return callbacks.count();
}

void QDxgiVSyncService::unregisterCallback(qsizetype id)
{
    QMutexLocker lock(&mutex);
    const qsizetype index = id - 1;
    if (index >= 0 && index < callbacks.count())
        callbacks[index] = nullptr;
}

QT_END_NAMESPACE
