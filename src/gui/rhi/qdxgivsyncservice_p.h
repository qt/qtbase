// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDXGIVSYNCSERVICE_P_H
#define QDXGIVSYNCSERVICE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QWindow>
#include <QMutex>
#include <QHash>

#include <dxgi1_6.h>

QT_BEGIN_NAMESPACE

class QDxgiVSyncThread;

class Q_GUI_EXPORT QDxgiVSyncService
{
public:
    // The public functions can be called on any thread. (but not at all from inside a callback)

    static QDxgiVSyncService *instance();

    // We want to know what adapters are in use by all the active QRhi instances
    // at any time. This is to avoid blind EnumAdapters calls to enumerate the
    // world every time a window is created. To be called by create()/destroy()
    // of the QRhi D3D11/12 backends.
    void refAdapter(LUID luid);
    void derefAdapter(LUID luid);

    // To be called from QRhi's beginFrame. Used to check if the factory is stale.
    void beginFrame(LUID luid);

    // Registering the windows can be done either by the QRhi swapchain in the
    // backends, or e.g. in the platformwindow implementation based on the
    // surfaceType.
    void registerWindow(QWindow *window);
    void unregisterWindow(QWindow *window);

    // The catch is that in some cases (using an adapter such as WARP) there are
    // no IDXGIOutputs at all, by design (even though there is very definitely a
    // QScreen etc. from our perspective), so none of this VSyncService is
    // functional. Hence this can never be the only way to drive updates for
    // D3D-based windows.
    //
    // A requestUpdate implementation can call this function on every request in
    // order to decide if it can rely on the callback from this service, or it
    // should use the traditional timer-based approach for a particular window.
    // For windows not registered the result is always false.
    bool supportsWindow(QWindow *window);

    // Callbacks are invoked on a vsync watcher thread. Whatever is done in
    // there, it must be fast and cheap. Ideally fire a notification for the
    // main thread somehow, and nothing else.
    using CallbackWindowList = QVarLengthArray<QWindow *, 16>;
    using Callback = std::function<void(const CallbackWindowList &windowList, qint64 timestampNs)>;
    qsizetype registerCallback(Callback cb); // result is an id, always >= 1
    void unregisterCallback(qsizetype id);

private:
    QDxgiVSyncService();
    ~QDxgiVSyncService();
    void destroy();
    void teardownDxgi();
    static void global_destroy();

    struct AdapterData {
        int ref;
        LUID luid;
        IDXGIAdapter *adapter;
        struct NotifierData {
            IDXGIOutput *output;
            QDxgiVSyncThread *thread;
        };
        QHash<HMONITOR, NotifierData> notifiers;
    };

    struct WindowData {
        IDXGIOutput *output;
        HMONITOR monitor;
        float reportedRefreshIntervalMs;
    };

    void cleanupAdapterData(AdapterData *a);
    void cleanupWindowData(WindowData *w);
    void updateWindowData(QWindow *window, WindowData *w);

    bool disableService = false;
    bool cleanupRegistered = false;
    IDXGIFactory2 *dxgiFactory = nullptr;
    QMutex mutex;
    QVector<AdapterData> adapters;
    QHash<QWindow *, WindowData> windows;
    QVector<Callback> callbacks;
};

QT_END_NAMESPACE

#endif
