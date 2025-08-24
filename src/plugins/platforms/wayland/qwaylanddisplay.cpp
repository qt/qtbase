// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2024 Jie Liu <liujie01@kylinos.cn>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylanddisplay_p.h"

#include "qwaylandappmenu_p.h"
#include "qwaylandintegration_p.h"
#include "qwaylandwindow_p.h"
#include "qwaylandsurface_p.h"
#include "qwaylandabstractdecoration_p.h"
#include "qwaylandscreen_p.h"
#include "qwaylandcursor_p.h"
#include "qwaylandinputdevice_p.h"
#if QT_CONFIG(clipboard)
#include "qwaylandclipboard_p.h"
#include "qwaylanddatacontrolv1_p.h"
#endif
#if QT_CONFIG(wayland_datadevice)
#include "qwaylanddatadevicemanager_p.h"
#include "qwaylanddatadevice_p.h"
#endif // QT_CONFIG(wayland_datadevice)
#if QT_CONFIG(wayland_client_primary_selection)
#include "qwaylandprimaryselectionv1_p.h"
#endif // QT_CONFIG(wayland_client_primary_selection)
#if QT_CONFIG(cursor)
#include <wayland-cursor.h>
#endif
#include "qwaylandhardwareintegration_p.h"
#include "qwaylandtextinputv1_p.h"
#include "qwaylandtextinputv2_p.h"
#include "qwaylandtextinputv3_p.h"
#include "qwaylandinputcontext_p.h"
#include "qwaylandinputmethodcontext_p.h"
#include "qwaylandeventdispatcher_p.h"

#include "qwaylandwindowmanagerintegration_p.h"
#include "qwaylandshellintegration_p.h"
#include "qwaylandclientbufferintegration_p.h"

#include "qwaylandpointergestures_p.h"
#include "qwaylandsubsurface_p.h"
#if QT_CONFIG(tabletevent)
#include "qwaylandtabletv2_p.h"
#endif

#include "qwaylandcolormanagement_p.h"

#include <QtWaylandClient/private/qwayland-text-input-unstable-v1.h>
#include <QtWaylandClient/private/qwayland-text-input-unstable-v2.h>
#include <QtWaylandClient/private/qwayland-text-input-unstable-v3.h>
#include <QtWaylandClient/private/qwayland-wp-primary-selection-unstable-v1.h>
#include <QtWaylandClient/private/qwayland-qt-text-input-method-unstable-v1.h>
#include <QtWaylandClient/private/qwayland-fractional-scale-v1.h>
#include <QtWaylandClient/private/qwayland-viewporter.h>
#include <QtWaylandClient/private/qwayland-cursor-shape-v1.h>
#include <QtWaylandClient/private/qwayland-xdg-system-bell-v1.h>
#include <QtWaylandClient/private/qwayland-xdg-toplevel-drag-v1.h>
#include <QtWaylandClient/private/qwayland-wlr-data-control-unstable-v1.h>
#include <QtWaylandClient/private/qwayland-pointer-warp-v1.h>

#include <QtCore/private/qcore_unix_p.h>

#include <QtCore/QAbstractEventDispatcher>
#include <QtGui/qpa/qwindowsysteminterface.h>
#include <QtGui/private/qguiapplication_p.h>

#include <QtCore/QDebug>

#include <errno.h>

#include <tuple> // for std::tie

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class EventThread : public QThread
{
    Q_OBJECT
public:
    enum OperatingMode {
        EmitToDispatch, // Emit the signal, allow dispatching in a differnt thread.
        SelfDispatch, // Dispatch the events inside this thread.
    };

    EventThread(struct wl_display * wl, struct wl_event_queue * ev_queue,
                OperatingMode mode)
        : m_fd(wl_display_get_fd(wl))
        , m_pipefd{ -1, -1 }
        , m_wldisplay(wl)
        , m_wlevqueue(ev_queue)
        , m_mode(mode)
        , m_reading(true)
        , m_quitting(false)
    {
        setObjectName(QStringLiteral("WaylandEventThread"));
    }

    void readAndDispatchEvents()
    {
        /*
         * Dispatch pending events and flush the requests at least once. If the event thread
         * is not reading, try to call _prepare_read() to allow the event thread to poll().
         * If that fails, re-try dispatch & flush again until _prepare_read() is successful.
         *
         * This allow any call to readAndDispatchEvents() to start event thread's polling,
         * not only the one issued from event thread's waitForReading(), which means functions
         * called from dispatch_pending() can safely spin an event loop.
         */
        if (m_quitting.loadRelaxed())
            return;

        for (;;) {
            if (dispatchQueuePending() < 0) {
                Q_EMIT waylandError();
                m_quitting.storeRelaxed(true);
                return;
            }

            wl_display_flush(m_wldisplay);

            // We have to check if event thread is reading every time we dispatch
            // something, as that may recursively call this function.
            if (m_reading.loadAcquire())
                break;

            if (prepareReadQueue() == 0) {
                QMutexLocker l(&m_mutex);
                m_reading.storeRelease(true);
                m_cond.wakeOne();
                break;
            }
        }
    }

    void stop()
    {
        // We have to both write to the pipe and set the flag, as the thread may be
        // either in the poll() or waiting for _prepare_read().
        if (m_pipefd[1] != -1 && write(m_pipefd[1], "\0", 1) == -1)
            qWarning("Failed to write to the pipe: %s.", strerror(errno));

        m_quitting.storeRelaxed(true);
        m_cond.wakeOne();

        wait();
    }

Q_SIGNALS:
    void needReadAndDispatch();
    void waylandError();

protected:
    void run() override
    {
        // we use this pipe to make the loop exit otherwise if we simply used a flag on the loop condition, if stop() gets
        // called while poll() is blocking the thread will never quit since there are no wayland messages coming anymore.
        struct Pipe
        {
            Pipe(int *fds)
                : fds(fds)
            {
                if (qt_safe_pipe(fds) != 0)
                    qWarning("Pipe creation failed. Quitting may hang.");
            }
            ~Pipe()
            {
                if (fds[0] != -1) {
                    close(fds[0]);
                    close(fds[1]);
                }
            }

            int *fds;
        } pipe(m_pipefd);

        // Make the main thread call wl_prepare_read(), dispatch the pending messages and flush the
        // outbound ones. Wait until it's done before proceeding, unless we're told to quit.
        while (waitForReading()) {
            if (!m_reading.loadRelaxed())
                break;

            pollfd fds[2] = { { m_fd, POLLIN, 0 }, { m_pipefd[0], POLLIN, 0 } };
            poll(fds, 2, -1);

            if (fds[1].revents & POLLIN) {
                // we don't really care to read the byte that was written here since we're closing down
                wl_display_cancel_read(m_wldisplay);
                break;
            }

            if (fds[0].revents & POLLIN)
                wl_display_read_events(m_wldisplay);
                // The poll was succesfull and the event thread did the wl_display_read_events(). On the next iteration of the loop
                // the event sent to the main thread will cause it to dispatch the messages just read, unless the loop exits in which
                // case we don't care anymore about them.
            else
                wl_display_cancel_read(m_wldisplay);
        }
    }

private:
    bool waitForReading()
    {
        Q_ASSERT(QThread::currentThread() == this);

        m_reading.storeRelease(false);

        if (m_mode == SelfDispatch) {
            readAndDispatchEvents();
        } else {
            Q_EMIT needReadAndDispatch();

            QMutexLocker lock(&m_mutex);
            // m_reading might be set from our emit or some other invocation of
            // readAndDispatchEvents().
            while (!m_reading.loadRelaxed() && !m_quitting.loadRelaxed())
                m_cond.wait(&m_mutex);
        }

        return !m_quitting.loadRelaxed();
    }

    int dispatchQueuePending()
    {
        if (m_wlevqueue)
            return wl_display_dispatch_queue_pending(m_wldisplay, m_wlevqueue);
        else
            return wl_display_dispatch_pending(m_wldisplay);
    }

    int prepareReadQueue()
    {
        if (m_wlevqueue)
            return wl_display_prepare_read_queue(m_wldisplay, m_wlevqueue);
        else
            return wl_display_prepare_read(m_wldisplay);
    }

    int m_fd;
    int m_pipefd[2];
    wl_display *m_wldisplay;
    wl_event_queue *m_wlevqueue;
    OperatingMode m_mode;

    /* Concurrency note when operating in EmitToDispatch mode:
     * m_reading is set to false inside event thread's waitForReading(), and is
     * set to true inside main thread's readAndDispatchEvents().
     * The lock is not taken when setting m_reading to false, as the main thread
     * is not actively waiting for it to turn false. However, the lock is taken
     * inside readAndDispatchEvents() before setting m_reading to true,
     * as the event thread is actively waiting for it under the wait condition.
     */

    QAtomicInteger<bool> m_reading;
    QAtomicInteger<bool> m_quitting;
    QMutex m_mutex;
    QWaitCondition m_cond;
};

Q_LOGGING_CATEGORY(lcQpaWayland, "qt.qpa.wayland"); // for general (uncategorized) Wayland platform logging

struct wl_surface *QWaylandDisplay::createSurface(void *handle)
{
    struct wl_surface *surface = mGlobals.compositor->create_surface();
    wl_surface_set_user_data(surface, handle);
    return surface;
}

struct ::wl_region *QWaylandDisplay::createRegion(const QRegion &qregion)
{
    struct ::wl_region *region = mGlobals.compositor->create_region();

    for (const QRect &rect : qregion)
        wl_region_add(region, rect.x(), rect.y(), rect.width(), rect.height());

    return region;
}

::wl_subsurface *QWaylandDisplay::createSubSurface(QWaylandWindow *window, QWaylandWindow *parent)
{
    if (!mGlobals.subCompositor) {
        qCWarning(lcQpaWayland) << "Can't create subsurface, not supported by the compositor.";
        return nullptr;
    }

    // Make sure we don't pass NULL surfaces to libwayland (crashes)
    Q_ASSERT(parent->wlSurface());
    Q_ASSERT(window->wlSurface());

    return mGlobals.subCompositor->get_subsurface(window->wlSurface(), parent->wlSurface());
}

::wp_viewport *QWaylandDisplay::createViewport(QWaylandWindow *window)
{
    if (!mGlobals.viewporter) {
        qCWarning(lcQpaWayland) << "Can't create wp_viewport, not supported by the compositor.";
        return nullptr;
    }

    Q_ASSERT(window->wlSurface());
    return mGlobals.viewporter->get_viewport(window->wlSurface());
}

QWaylandShellIntegration *QWaylandDisplay::shellIntegration() const
{
    return mWaylandIntegration->shellIntegration();
}

QWaylandClientBufferIntegration * QWaylandDisplay::clientBufferIntegration() const
{
    return mWaylandIntegration->clientBufferIntegration();
}

QWaylandWindowManagerIntegration *QWaylandDisplay::windowManagerIntegration() const
{
    return mGlobals.windowManagerIntegration.get();
}

QWaylandDisplay::QWaylandDisplay(QWaylandIntegration *waylandIntegration)
    : mWaylandIntegration(waylandIntegration)
{
    qRegisterMetaType<uint32_t>("uint32_t");

    mDisplay = wl_display_connect(nullptr);
    if (mDisplay) {
        setupConnection();
    } else {
        qErrnoWarning(errno, "Failed to create wl_display");
    }

    mWaylandTryReconnect = qEnvironmentVariableIsSet("QT_WAYLAND_RECONNECT");
    mPreferWlrDataControl = qEnvironmentVariableIntValue("QT_WAYLAND_USE_DATA_CONTROL") > 0;
}

void QWaylandDisplay::setupConnection()
{
    struct ::wl_registry *registry = wl_display_get_registry(mDisplay);
    init(registry);

#if QT_CONFIG(xkbcommon)
    mXkbContext.reset(xkb_context_new(XKB_CONTEXT_NO_FLAGS));
    if (!mXkbContext)
        qCWarning(lcQpaWayland, "failed to create xkb context");
#endif
    if (mWaylandInputContextRequested)
        checkTextInputProtocol();
}

QWaylandDisplay::~QWaylandDisplay(void)
{
    if (m_eventThread)
        m_eventThread->stop();

    if (m_frameEventQueueThread)
        m_frameEventQueueThread->stop();

    if (mSyncCallback)
        wl_callback_destroy(mSyncCallback);

    qDeleteAll(std::exchange(mInputDevices, {}));

    for (QWaylandScreen *screen : std::exchange(mScreens, {})) {
        QWindowSystemInterface::handleScreenRemoved(screen);
    }
    qDeleteAll(mWaitingScreens);

#if QT_CONFIG(cursor)
    mCursorThemes.clear();
#endif

    if (m_frameEventQueue)
        wl_event_queue_destroy(m_frameEventQueue);

    // Reset the globals manually since they need to be destroyed before the wl_display
    mGlobals = {};

    if (object())
        wl_registry_destroy(object());

    if (mDisplay)
        wl_display_disconnect(mDisplay);
}

// Steps which is called just after constructor. This separates registry_global() out of the constructor
// so that factory functions in integration can be overridden.
bool QWaylandDisplay::initialize()
{
    if (!isInitialized())
        return false;

    forceRoundTrip();

    emit connected();

    if (!mWaitingScreens.isEmpty()) {
        // Give wl_output.done and zxdg_output_v1.done events a chance to arrive
        forceRoundTrip();
    }
    if (mWaylandInputContextRequested)
        mTextInputManagerIndex = INT_MAX;

    return qEnvironmentVariableIntValue("QT_WAYLAND_DONT_CHECK_SHELL_INTEGRATION") || shellIntegration();
}

void QWaylandDisplay::ensureScreen()
{
    if (!mScreens.empty() || mPlaceholderScreen)
        return; // There are real screens or we already have a fake one

    qCInfo(lcQpaWayland) << "There are no outputs - creating placeholder screen";

    mPlaceholderScreen = new QPlatformPlaceholderScreen();
    QWindowSystemInterface::handleScreenAdded(mPlaceholderScreen);
    Q_ASSERT(!QGuiApplication::screens().empty());
}

void QWaylandDisplay::reconnect()
{
    qCWarning(lcQpaWayland) << "Attempting wayland reconnect";
    m_eventThread->stop();
    m_frameEventQueueThread->stop();
    m_eventThread->wait();
    m_frameEventQueueThread->wait();

    qDeleteAll(mWaitingScreens);
    mWaitingScreens.clear();

    while (!mScreens.isEmpty()) {
        auto screen = mScreens.takeLast();
        ensureScreen();
        QWindowSystemInterface::handleScreenRemoved(screen);
    }

    mCursorThemes.clear();
    mCursor.reset();

    mGlobals = GlobalHolder();

    mWaylandIntegration->reset();

    qDeleteAll(std::exchange(mInputDevices, {}));
    mLastInputDevice = nullptr;

    for (const RegistryGlobal &global : mRegistryGlobals) {
        emit globalRemoved(global);
    }
    mRegistryGlobals.clear();

    mLastInputSerial = 0;
    mLastInputWindow.clear();
    mLastKeyboardFocus.clear();
    mActiveWindows.clear();

    const auto windows = QGuiApplication::allWindows();
    QList<QWaylandWindow *> allPlatformWindows;
    for (auto window : windows) {
        if (auto waylandWindow = static_cast<QWaylandWindow *>(window->handle())) {
            waylandWindow->closeChildPopups();
            allPlatformWindows.push_back(waylandWindow);
        }
    }

    // Remove windows that do not need to be recreated and now closed popups
    QList<QWaylandWindow *> recreateWindows;
    for (auto window : std::as_const(allPlatformWindows)) {
        if (window->subSurfaceWindow() || window->shellSurface()) {
            recreateWindows.push_back(window);
        }
        window->reset();
    }

    if (mSyncCallback) {
        wl_callback_destroy(mSyncCallback);
        mSyncCallback = nullptr;
    }

    if (object())
        wl_registry_destroy(object());
    mDisplay = wl_display_connect(nullptr);
    if (!mDisplay)
        _exit(1);

    connect(
            this, &QWaylandDisplay::connected, this,
            [&allPlatformWindows] {
                for (auto &window : std::as_const(allPlatformWindows)) {
                    window->initializeWlSurface();
                }
            },
            Qt::SingleShotConnection);

    setupConnection();
    initialize();

    if (m_frameEventQueue)
        wl_event_queue_destroy(m_frameEventQueue);
    initEventThread();

    auto needsRecreate = [](QPlatformWindow *window) {
        auto waylandWindow = static_cast<QWaylandWindow *>(window);
        return waylandWindow && !waylandWindow->subSurfaceWindow() && !waylandWindow->shellSurface();
    };
    auto window = recreateWindows.begin();
    while (!recreateWindows.isEmpty()) {
        if (!needsRecreate((*window)->QPlatformWindow::parent()) && !needsRecreate((*window)->transientParent())) {
            (*window)->reinit();
            window = recreateWindows.erase(window);
        } else {
            ++window;
        }
        if (window == recreateWindows.end())
            window = recreateWindows.begin();
    }

    mWaylandIntegration->reconfigureInputContext();
}

void QWaylandDisplay::flushRequests()
{
    m_eventThread->readAndDispatchEvents();
    QWindowSystemInterface::flushWindowSystemEvents(QWaylandEventDispatcher::eventDispatcher->flags());
}

// We have to wait until we have an eventDispatcher before creating the eventThread,
// otherwise forceRoundTrip() may block inside _events_read() because eventThread is
// polling.
void QWaylandDisplay::initEventThread()
{
    m_eventThread.reset(
            new EventThread(mDisplay, /* default queue */ nullptr, EventThread::EmitToDispatch));
    connect(m_eventThread.get(), &EventThread::needReadAndDispatch, this,
            &QWaylandDisplay::flushRequests, Qt::QueuedConnection);
    connect(m_eventThread.get(), &EventThread::waylandError, this,
            &QWaylandDisplay::checkWaylandError, Qt::QueuedConnection);
    m_eventThread->start();

    // wl_display_disconnect() free this.
    m_frameEventQueue = wl_display_create_queue(mDisplay);
    m_frameEventQueueThread.reset(
            new EventThread(mDisplay, m_frameEventQueue, EventThread::SelfDispatch));
    m_frameEventQueueThread->start();
}

void QWaylandDisplay::checkWaylandError()
{
    int ecode = wl_display_get_error(mDisplay);
    if ((ecode == EPIPE || ecode == ECONNRESET)) {
        qWarning("The Wayland connection broke. Did the Wayland compositor die?");
        if (mWaylandTryReconnect) {
            reconnect();
            return;
        }
    } else {
        qWarning("The Wayland connection experienced a fatal error: %s", strerror(ecode));
    }
    _exit(-1);
}

void QWaylandDisplay::checkTextInputProtocol()
{
    QStringList tips, timps; // for text input protocols and text input manager protocols
    // zwp_text_input_v2 is preferred over zwp_text_input_v3 because:
    // - Currently, v3 is not as feature rich as v2.
    // - While v2 is not upstreamed, it is well supported by KWin since Plasma 5 and Plasma
    //   Mobile uses some v2 only.
    tips << QLatin1String(QtWayland::qt_text_input_method_v1::interface()->name)
         << QLatin1String(QtWayland::zwp_text_input_v2::interface()->name)
         << QLatin1String(QtWayland::zwp_text_input_v3::interface()->name)
         << QLatin1String(QtWayland::zwp_text_input_v1::interface()->name);
    timps << QLatin1String(QtWayland::qt_text_input_method_manager_v1::interface()->name)
          << QLatin1String(QtWayland::zwp_text_input_manager_v2::interface()->name)
          << QLatin1String(QtWayland::zwp_text_input_manager_v3::interface()->name)
          << QLatin1String(QtWayland::zwp_text_input_manager_v1::interface()->name);

    QString tiProtocols = QString::fromLocal8Bit(qgetenv("QT_WAYLAND_TEXT_INPUT_PROTOCOL"));
    qCDebug(lcQpaWayland) << "QT_WAYLAND_TEXT_INPUT_PROTOCOL=" << tiProtocols;
    QStringList keys;
    if (!tiProtocols.isEmpty()) {
        keys = tiProtocols.split(QLatin1Char(';'));
        QList<QString>::iterator it = keys.begin();
        while (it != keys.end()) {
            if (tips.contains(*it))
                mTextInputManagerList.append(timps.at(tips.indexOf(*it)));
            else
                qCDebug(lcQpaWayland) << "text input: unknown protocol - " << *it;
            ++it;
        }
    }
    if (mTextInputManagerList.isEmpty()) // fallback
        mTextInputManagerList = timps;
}

QWaylandScreen *QWaylandDisplay::screenForOutput(struct wl_output *output) const
{
    for (auto screen : std::as_const(mScreens)) {
        if (screen->output() == output)
            return screen;
    }
    return nullptr;
}

void QWaylandDisplay::handleScreenInitialized(QWaylandScreen *screen)
{
    if (!mWaitingScreens.removeOne(screen))
        return;
    mScreens.append(screen);
    QWindowSystemInterface::handleScreenAdded(screen);
    if (mPlaceholderScreen) {
        // handleScreenRemoved deletes the platform screen
        QPlatformScreen *s = mPlaceholderScreen;
        mPlaceholderScreen = nullptr;
        QWindowSystemInterface::handleScreenRemoved(s);

    }
}

template <typename T, auto f>
struct WithDestructor : public T
{
    using T::T;
    ~WithDestructor()
    {
        f(this->object());
    }
};

void QWaylandDisplay::registry_global(uint32_t id, const QString &interface, uint32_t version)
{
    struct ::wl_registry *registry = object();

    static QStringList interfaceBlacklist = qEnvironmentVariable("QT_WAYLAND_DISABLED_INTERFACES").split(u',');
    if (interfaceBlacklist.contains(interface)) {
        return;
    }

    if (interface == QLatin1String(QtWayland::wl_output::interface()->name)) {
        mWaitingScreens << mWaylandIntegration->createPlatformScreen(this, version, id);
    } else if (interface == QLatin1String(QtWayland::wl_compositor::interface()->name)) {
        mGlobals.compositor.reset(
                new WithDestructor<QtWayland::wl_compositor, wl_compositor_destroy>(
                        registry, id, qMin((int)version, 6)));
    } else if (interface == QLatin1String(QWaylandShm::interface()->name)) {
        mGlobals.shm.reset(new QWaylandShm(this, version, id));
    } else if (interface == QLatin1String(QWaylandInputDevice::interface()->name)) {
        QWaylandInputDevice *inputDevice = mWaylandIntegration->createInputDevice(this, version, id);
        mInputDevices.append(inputDevice);
#if QT_CONFIG(wayland_datadevice)
    } else if (interface == QLatin1String(QWaylandDataDeviceManager::interface()->name)) {
        mGlobals.dndSelectionHandler.reset(new QWaylandDataDeviceManager(this, version, id));
#endif
    } else if (interface == QLatin1String(QtWayland::wl_subcompositor::interface()->name)) {
        mGlobals.subCompositor.reset(
                new WithDestructor<QtWayland::wl_subcompositor, wl_subcompositor_destroy>(registry,
                                                                                          id, 1));
#if QT_CONFIG(tabletevent)
    } else if (interface == QLatin1String(QWaylandTabletManagerV2::interface()->name)) {
        mGlobals.tabletManager.reset(new QWaylandTabletManagerV2(this, id, qMin(1, int(version))));
        for (QWaylandInputDevice *inputDevice : std::as_const(mInputDevices))
            inputDevice->setTabletSeat(
                    new QWaylandTabletSeatV2(mGlobals.tabletManager.get(), inputDevice));
#endif
    } else if (interface == QLatin1String(QWaylandPointerGestures::interface()->name)) {
        mGlobals.pointerGestures.reset(new QWaylandPointerGestures(this, id, 1));
#if QT_CONFIG(wayland_client_primary_selection)
    } else if (interface == QLatin1String(QWaylandPrimarySelectionDeviceManagerV1::interface()->name)) {
        mGlobals.primarySelectionManager.reset(
                new QWaylandPrimarySelectionDeviceManagerV1(this, id, 1));
        for (QWaylandInputDevice *inputDevice : std::as_const(mInputDevices))
            inputDevice->setPrimarySelectionDevice(
                    mGlobals.primarySelectionManager->createDevice(inputDevice));
#endif
    } else if (interface == QLatin1String(QtWayland::qt_text_input_method_manager_v1::interface()->name)
            && (mTextInputManagerList.contains(interface) && mTextInputManagerList.indexOf(interface) < mTextInputManagerIndex)) {
        qCDebug(lcQpaWayland) << "text input: register qt_text_input_method_manager_v1";
        if (mTextInputManagerIndex < INT_MAX) {
            mGlobals.textInputManagerv1.reset();
            mGlobals.textInputManagerv2.reset();
            mGlobals.textInputManagerv3.reset();
            for (QWaylandInputDevice *inputDevice : std::as_const(mInputDevices))
                inputDevice->setTextInput(nullptr);
        }

        mGlobals.textInputMethodManager.reset(
                new WithDestructor<QtWayland::qt_text_input_method_manager_v1,
                                   qt_text_input_method_manager_v1_destroy>(registry, id, 1));
        for (QWaylandInputDevice *inputDevice : std::as_const(mInputDevices))
            inputDevice->setTextInputMethod(new QWaylandTextInputMethod(
                    this,
                    mGlobals.textInputMethodManager->get_text_input_method(
                            inputDevice->wl_seat())));
        mWaylandIntegration->reconfigureInputContext();
        mTextInputManagerIndex = mTextInputManagerList.indexOf(interface);
    } else if (interface == QLatin1String(QtWayland::zwp_text_input_manager_v1::interface()->name)
               && (mTextInputManagerList.contains(interface) && mTextInputManagerList.indexOf(interface) < mTextInputManagerIndex)) {
        qCDebug(lcQpaWayland) << "text input: register zwp_text_input_v1";
        if (mTextInputManagerIndex < INT_MAX) {
            mGlobals.textInputMethodManager.reset();
            mGlobals.textInputManagerv2.reset();
            mGlobals.textInputManagerv3.reset();
            for (QWaylandInputDevice *inputDevice : std::as_const(mInputDevices))
                inputDevice->setTextInputMethod(nullptr);
        }

        mGlobals.textInputManagerv1.reset(
                new WithDestructor<QtWayland::zwp_text_input_manager_v1,
                                   zwp_text_input_manager_v1_destroy>(registry, id, 1));
        for (QWaylandInputDevice *inputDevice : std::as_const(mInputDevices)) {
            auto textInput =
                    new QWaylandTextInputv1(this, mGlobals.textInputManagerv1->create_text_input());
            textInput->setSeat(inputDevice->wl_seat());
            inputDevice->setTextInput(textInput);
        }

        mWaylandIntegration->reconfigureInputContext();
        mTextInputManagerIndex = mTextInputManagerList.indexOf(interface);
    } else if (interface == QLatin1String(QtWayland::zwp_text_input_manager_v2::interface()->name)
            && (mTextInputManagerList.contains(interface) && mTextInputManagerList.indexOf(interface) < mTextInputManagerIndex)) {
        qCDebug(lcQpaWayland) << "text input: register zwp_text_input_v2";
        if (mTextInputManagerIndex < INT_MAX) {
            mGlobals.textInputMethodManager.reset();
            mGlobals.textInputManagerv1.reset();
            mGlobals.textInputManagerv3.reset();
            for (QWaylandInputDevice *inputDevice : std::as_const(mInputDevices))
                inputDevice->setTextInputMethod(nullptr);
        }

        mGlobals.textInputManagerv2.reset(
                new WithDestructor<QtWayland::zwp_text_input_manager_v2,
                                   zwp_text_input_manager_v2_destroy>(registry, id, 1));
        for (QWaylandInputDevice *inputDevice : std::as_const(mInputDevices))
            inputDevice->setTextInput(new QWaylandTextInputv2(
                    this, mGlobals.textInputManagerv2->get_text_input(inputDevice->wl_seat())));
        mWaylandIntegration->reconfigureInputContext();
        mTextInputManagerIndex = mTextInputManagerList.indexOf(interface);
    } else if (interface == QLatin1String(QtWayland::zwp_text_input_manager_v3::interface()->name)
            && (mTextInputManagerList.contains(interface) && mTextInputManagerList.indexOf(interface) < mTextInputManagerIndex)) {
        qCDebug(lcQpaWayland) << "text input: register zwp_text_input_v3";
        if (mTextInputManagerIndex < INT_MAX) {
            mGlobals.textInputMethodManager.reset();
            mGlobals.textInputManagerv2.reset();
            for (QWaylandInputDevice *inputDevice : std::as_const(mInputDevices))
                inputDevice->setTextInputMethod(nullptr);
        }
        mGlobals.textInputManagerv3.reset(
                new WithDestructor<QtWayland::zwp_text_input_manager_v3,
                                   zwp_text_input_manager_v3_destroy>(registry, id, 1));
        for (QWaylandInputDevice *inputDevice : std::as_const(mInputDevices))
            inputDevice->setTextInput(new QWaylandTextInputv3(
                    this, mGlobals.textInputManagerv3->get_text_input(inputDevice->wl_seat())));

        mWaylandIntegration->reconfigureInputContext();
        mTextInputManagerIndex = mTextInputManagerList.indexOf(interface);
    }else if (interface == QLatin1String(QWaylandHardwareIntegration::interface()->name)) {
        bool disableHardwareIntegration = qEnvironmentVariableIntValue("QT_WAYLAND_DISABLE_HW_INTEGRATION");
        if (!disableHardwareIntegration) {
            mGlobals.hardwareIntegration.reset(new QWaylandHardwareIntegration(registry, id));
            // make a roundtrip here since we need to receive the events sent by
            // qt_hardware_integration before creating windows
            forceRoundTrip();
        }
    } else if (interface == QLatin1String(QWaylandXdgOutputManagerV1::interface()->name)) {
        mGlobals.xdgOutputManager.reset(new QWaylandXdgOutputManagerV1(this, id, version));
        for (auto *screen : std::as_const(mWaitingScreens))
            screen->initXdgOutput(xdgOutputManager());
    } else if (interface == QLatin1String(QtWayland::wp_fractional_scale_manager_v1::interface()->name)) {
        mGlobals.fractionalScaleManager.reset(
                new WithDestructor<QtWayland::wp_fractional_scale_manager_v1,
                                   wp_fractional_scale_manager_v1_destroy>(registry, id, 1));
    } else if (interface == QLatin1String("wp_viewporter")) {
        mGlobals.viewporter.reset(
                new WithDestructor<QtWayland::wp_viewporter, wp_viewporter_destroy>(
                        registry, id, qMin(1u, version)));
    } else if (interface == QLatin1String(QtWayland::wp_cursor_shape_manager_v1::interface()->name)) {
        mGlobals.cursorShapeManager.reset(new WithDestructor<QtWayland::wp_cursor_shape_manager_v1,
                                                             wp_cursor_shape_manager_v1_destroy>(
                registry, id, std::min(1u, version)));
    } else if (
            interface == QLatin1String(QtWayland::xdg_toplevel_drag_manager_v1::interface()->name)) {
        mGlobals.xdgToplevelDragManager.reset(
                new WithDestructor<QtWayland::xdg_toplevel_drag_manager_v1,
                                   xdg_toplevel_drag_manager_v1_destroy>(registry, id, 1));
    } else if (interface == QLatin1String(QtWayland::qt_windowmanager::interface()->name)) {
        mGlobals.windowManagerIntegration.reset(
                new QWaylandWindowManagerIntegration(this, id, version));
    } else if (interface == QLatin1String(QtWayland::xdg_system_bell_v1::interface()->name)) {
        mGlobals.systemBell.reset(new WithDestructor<QtWayland::xdg_system_bell_v1, xdg_system_bell_v1_destroy>(registry, id, 1));
    } else if (
            interface == QLatin1String(QtWayland::org_kde_kwin_appmenu_manager::interface()->name)) {
        mGlobals.appMenuManager.reset(new QWaylandAppMenuManager(registry, id, 1));
#if QT_CONFIG(clipboard)
    } else if (mPreferWlrDataControl && interface == QLatin1String(QWaylandDataControlManagerV1::interface()->name)) {
        mGlobals.dataControlManager.reset(new QWaylandDataControlManagerV1(this, id, 2));
        for (QWaylandInputDevice *inputDevice : std::as_const(mInputDevices)) {
            inputDevice->setDataControlDevice(mGlobals.dataControlManager->createDevice(inputDevice));
        }
#endif
    } else if (interface == QLatin1String(QtWayland::xx_color_manager_v4::interface()->name)) {
        mGlobals.colorManager = std::make_unique<ColorManager>(registry, id, 1);
        // we need a roundtrip to receive the features the compositor supports
        forceRoundTrip();
    } else if (interface == QLatin1String(QtWayland::wp_pointer_warp_v1::interface()->name)) {
        mGlobals.pointerWarp.reset(new WithDestructor<QtWayland::wp_pointer_warp_v1, wp_pointer_warp_v1_destroy>(
                registry, id, 1));
    }


    mRegistryGlobals.append(RegistryGlobal(id, interface, version, registry));
    emit globalAdded(mRegistryGlobals.back());

    const auto copy = mRegistryListeners; // be prepared for listeners unregistering on notification
    for (Listener l : copy)
        (*l.listener)(l.data, registry, id, interface, version);
}

void QWaylandDisplay::registry_global_remove(uint32_t id)
{
    for (int i = 0, ie = mRegistryGlobals.size(); i != ie; ++i) {
        RegistryGlobal &global = mRegistryGlobals[i];
        if (global.id == id) {
            if (global.interface == QLatin1String(QtWayland::wl_output::interface()->name)) {
                for (auto *screen : mWaitingScreens) {
                    if (screen->outputId() == id) {
                        mWaitingScreens.removeOne(screen);
                        delete screen;
                        break;
                    }
                }

                for (QWaylandScreen *screen : std::as_const(mScreens)) {
                    if (screen->outputId() == id) {
                        mScreens.removeOne(screen);
                        // If this is the last screen, we have to add a fake screen, or Qt will break.
                        ensureScreen();
                        QWindowSystemInterface::handleScreenRemoved(screen);
                        break;
                    }
                }
            }
            if (global.interface == QLatin1String(QtWayland::zwp_text_input_manager_v1::interface()->name)) {
                mGlobals.textInputManagerv1.reset();
                for (QWaylandInputDevice *inputDevice : std::as_const(mInputDevices))
                    inputDevice->setTextInput(nullptr);
                mWaylandIntegration->reconfigureInputContext();
            }
            if (global.interface == QLatin1String(QtWayland::zwp_text_input_manager_v2::interface()->name)) {
                mGlobals.textInputManagerv2.reset();
                for (QWaylandInputDevice *inputDevice : std::as_const(mInputDevices))
                    inputDevice->setTextInput(nullptr);
                mWaylandIntegration->reconfigureInputContext();
            }
            if (global.interface == QLatin1String(QtWayland::zwp_text_input_manager_v3::interface()->name)) {
                mGlobals.textInputManagerv3.reset();
                for (QWaylandInputDevice *inputDevice : std::as_const(mInputDevices))
                    inputDevice->setTextInput(nullptr);
                mWaylandIntegration->reconfigureInputContext();
            }
            if (global.interface == QLatin1String(QtWayland::qt_text_input_method_manager_v1::interface()->name)) {
                mGlobals.textInputMethodManager.reset();
                for (QWaylandInputDevice *inputDevice : std::as_const(mInputDevices))
                    inputDevice->setTextInputMethod(nullptr);
                mWaylandIntegration->reconfigureInputContext();
            }
#if QT_CONFIG(wayland_client_primary_selection)
            if (global.interface == QLatin1String(QtWayland::zwp_primary_selection_device_manager_v1::interface()->name)) {
                mGlobals.primarySelectionManager.reset();
                for (QWaylandInputDevice *inputDevice : std::as_const(mInputDevices))
                    inputDevice->setPrimarySelectionDevice(nullptr);
            }
#endif
#if QT_CONFIG(clipboard)
            if (global.interface == QLatin1String(QtWayland::zwlr_data_control_manager_v1::interface()->name)) {
                mGlobals.dataControlManager.reset();
                for (QWaylandInputDevice *inputDevice : std::as_const(mInputDevices))
                    inputDevice->setDataControlDevice(nullptr);
            }
#endif
            emit globalRemoved(mRegistryGlobals.takeAt(i));
            break;
        }
    }
}

bool QWaylandDisplay::hasRegistryGlobal(QStringView interfaceName) const
{
    for (const RegistryGlobal &global : mRegistryGlobals)
        if (global.interface == interfaceName)
            return true;

    return false;
}

void QWaylandDisplay::addRegistryListener(RegistryListener listener, void *data)
{
    Listener l = { listener, data };
    mRegistryListeners.append(l);
    for (int i = 0, ie = mRegistryGlobals.size(); i != ie; ++i)
        (*l.listener)(l.data, mRegistryGlobals[i].registry, mRegistryGlobals[i].id,
                      mRegistryGlobals[i].interface, mRegistryGlobals[i].version);
}

void QWaylandDisplay::removeListener(RegistryListener listener, void *data)
{
    auto iter = std::remove_if(mRegistryListeners.begin(), mRegistryListeners.end(), [=](Listener l){
        return (l.listener == listener && l.data == data);
    });
    mRegistryListeners.erase(iter, mRegistryListeners.end());
}

void QWaylandDisplay::forceRoundTrip()
{
     wl_display_roundtrip(mDisplay);
}

bool QWaylandDisplay::supportsWindowDecoration() const
{
    static bool disabled = qgetenv("QT_WAYLAND_DISABLE_WINDOWDECORATION").toInt();
    // Stop early when disabled via the environment. Do not try to load the integration in
    // order to play nice with SHM-only, buffer integration-less systems.
    if (disabled)
        return false;

    // Don't initialize client buffer integration just to check whether it can have a decoration.
    if (!mWaylandIntegration->mClientBufferIntegrationInitialized)
        return true;

    // We can do software-rendered decorations, only disable them if the integration explicitly says it can't.
    static bool integrationSupport = !clientBufferIntegration() || clientBufferIntegration()->supportsWindowDecoration();
    return integrationSupport;
}

QWaylandWindow *QWaylandDisplay::lastInputWindow() const
{
    return mLastInputWindow.data();
}

void QWaylandDisplay::setLastInputDevice(QWaylandInputDevice *device, uint32_t serial, QWaylandWindow *win)
{
    mLastInputDevice = device;
    mLastInputSerial = serial;
    mLastInputWindow = win;
}

bool QWaylandDisplay::isWindowActivated(const QWaylandWindow *window)
{
    return mActiveWindows.contains(const_cast<QWaylandWindow *>(window));
}

void QWaylandDisplay::handleWindowActivated(QWaylandWindow *window)
{
    if (mActiveWindows.contains(window))
        return;

    mActiveWindows.append(window);
    requestWaylandSync();

    if (auto *decoration = window->decoration())
        decoration->update();
}

void QWaylandDisplay::handleWindowDeactivated(QWaylandWindow *window)
{
    Q_ASSERT(!mActiveWindows.empty());

    if (mActiveWindows.last() == window)
        requestWaylandSync();

    mActiveWindows.removeOne(window);

    if (QCoreApplication::closingDown())
        return;

    if (auto *decoration = window->decoration())
        decoration->update();
}

void QWaylandDisplay::handleKeyboardFocusChanged(QWaylandInputDevice *inputDevice)
{
    QWaylandWindow *keyboardFocus = inputDevice->keyboardFocus();

    if (mLastKeyboardFocus == keyboardFocus)
        return;

    if (keyboardFocus)
        handleWindowActivated(keyboardFocus);
    if (mLastKeyboardFocus)
        handleWindowDeactivated(mLastKeyboardFocus);

    mLastKeyboardFocus = keyboardFocus;
}

void QWaylandDisplay::handleWindowDestroyed(QWaylandWindow *window)
{
    if (mActiveWindows.contains(window))
        handleWindowDeactivated(window);
}

void QWaylandDisplay::handleWaylandSync()
{
    // This callback is used to set the window activation because we may get an activate/deactivate
    // pair, and the latter one would be lost in the QWindowSystemInterface queue, if we issue the
    // handleWindowActivated() calls immediately.
    QWindow *activeWindow = mActiveWindows.empty() ? nullptr : mActiveWindows.last()->window();
    if (activeWindow != QGuiApplication::focusWindow())
        QWindowSystemInterface::handleFocusWindowChanged(activeWindow);

    if (!activeWindow) {
        if (lastInputDevice()) {
#if QT_CONFIG(clipboard)
            if (auto *dataDevice = lastInputDevice()->dataDevice())
                dataDevice->invalidateSelectionOffer();
#endif
#if QT_CONFIG(wayland_client_primary_selection)
            if (auto *device = lastInputDevice()->primarySelectionDevice())
                device->invalidateSelectionOffer();
#endif
        }
    }
}

const wl_callback_listener QWaylandDisplay::syncCallbackListener = {
    [](void *data, struct wl_callback *callback, uint32_t time){
        Q_UNUSED(time);
        wl_callback_destroy(callback);
        QWaylandDisplay *display = static_cast<QWaylandDisplay *>(data);
        display->mSyncCallback = nullptr;
        display->handleWaylandSync();
    }
};

void QWaylandDisplay::requestWaylandSync()
{
    if (mSyncCallback)
        return;

    mSyncCallback = wl_display_sync(mDisplay);
    wl_callback_add_listener(mSyncCallback, &syncCallbackListener, this);
}

QWaylandInputDevice *QWaylandDisplay::defaultInputDevice() const
{
    return mInputDevices.isEmpty() ? nullptr : mInputDevices.first();
}

bool QWaylandDisplay::isKeyboardAvailable() const
{
    return std::any_of(
            mInputDevices.constBegin(), mInputDevices.constEnd(),
            [](const QWaylandInputDevice *device) { return device->keyboard() != nullptr; });
}

bool QWaylandDisplay::isWaylandInputContextRequested() const {
    return mWaylandInputContextRequested;
}

#if QT_CONFIG(cursor)

QWaylandCursor *QWaylandDisplay::waylandCursor()
{
    if (!mCursor)
        mCursor.reset(mWaylandIntegration->createPlatformCursor(this));
    return mCursor.data();
}

auto QWaylandDisplay::findExistingCursorTheme(const QString &name, int pixelSize) const noexcept
    -> FindExistingCursorThemeResult
{
    const auto byNameAndSize = [](const WaylandCursorTheme &lhs, const WaylandCursorTheme &rhs) {
        return std::tie(lhs.pixelSize, lhs.name) < std::tie(rhs.pixelSize, rhs.name);
    };

    const WaylandCursorTheme prototype = {name, pixelSize, nullptr};

    const auto it = std::lower_bound(mCursorThemes.cbegin(), mCursorThemes.cend(), prototype, byNameAndSize);
    if (it != mCursorThemes.cend() && it->name == name && it->pixelSize == pixelSize)
        return {it, true};
    else
        return {it, false};
}

QWaylandCursorTheme *QWaylandDisplay::loadCursorTheme(const QString &name, int pixelSize)
{
    const auto result = findExistingCursorTheme(name, pixelSize);
    if (result.found)
        return result.theme();

    if (auto theme = QWaylandCursorTheme::create(shm(), pixelSize, name))
        return mCursorThemes.insert(result.position, {name, pixelSize, std::move(theme)})->theme.get();

    return nullptr;
}

#endif // QT_CONFIG(cursor)

} // namespace QtWaylandClient

QT_END_NAMESPACE

#include "qwaylanddisplay.moc"
#include "moc_qwaylanddisplay_p.cpp"
