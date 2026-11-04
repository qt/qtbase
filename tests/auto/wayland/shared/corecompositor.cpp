// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "corecompositor.h"
#include <thread>

namespace MockCompositor {

CoreCompositor::CoreCompositor(CompositorType t, int socketFd)
    : m_type(t)
    , m_display(wl_display_create())
    , m_eventLoop(wl_display_get_event_loop(m_display))

    // Start dispatching
    , m_dispatchThread([this](){
        while (m_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            dispatch();
        }
    })
{
    if (socketFd == -1) {
        QByteArray socketName = wl_display_add_socket_auto(m_display);
        qputenv("WAYLAND_DISPLAY", socketName);
    } else {
        wl_display_add_socket_fd(m_display, socketFd);
    }
    m_timer.start();
    Q_ASSERT(isClean());
}

CoreCompositor::~CoreCompositor()
{
    m_running = false;
    m_dispatchThread.join();
    wl_display_destroy_clients(m_display);
    qDeleteAll(m_globals);
    m_globals.clear();
    wl_display_destroy(m_display);
    qDebug() << "cleanup";
}

bool CoreCompositor::isClean()
{
    Lock lock(this);
    for (auto *global : std::as_const(m_globals)) {
        if (!global->isClean())
            return false;
    }
    return true;
}

QString CoreCompositor::dirtyMessage()
{
    Lock lock(this);
    QStringList messages;
    for (auto *global : std::as_const(m_globals)) {
        if (!global->isClean())
            messages << (global->metaObject()->className() % QLatin1String(": ") % global->dirtyMessage());
    }
    return messages.join(", ");
}

void CoreCompositor::dispatch(int timeout)
{
    Lock lock(this);
    wl_display_flush_clients(m_display);
    wl_event_loop_dispatch(m_eventLoop, timeout);
}

/*!
 * \brief Adds a new global interface for the compositor
 *
 * Takes ownership of \a global
 */
void CoreCompositor::add(Global *global)
{
    warnIfNotLockedByThread(Q_FUNC_INFO);
    m_globals.append(global);
}

void CoreCompositor::remove(Global *global)
{
    warnIfNotLockedByThread(Q_FUNC_INFO);
    m_globals.removeAll(global);
    delete global;
}

uint CoreCompositor::nextSerial()
{
    warnIfNotLockedByThread(Q_FUNC_INFO);
    return wl_display_next_serial(m_display);
}

uint CoreCompositor::currentTimeMilliseconds()
{
    warnIfNotLockedByThread(Q_FUNC_INFO);
    return uint(m_timer.elapsed());
}

wl_client *CoreCompositor::client(int index)
{
    warnIfNotLockedByThread(Q_FUNC_INFO);
    wl_list *clients = wl_display_get_client_list(m_display);
    wl_client *client = nullptr;
    int i = 0;
    wl_client_for_each(client, clients) {
        if (i++ == index)
            return client;
    }
    return nullptr;
}

void CoreCompositor::warnIfNotLockedByThread(const char *caller)
{
    if (!m_lock || !m_lock->isOwnedByCurrentThread()) {
        qWarning() << caller << "called without locking the compositor to the current thread."
                   << "This means the compositor can start dispatching at any moment,"
                   << "potentially leading to threading issues."
                   << "Unless you know what you are doing you should probably fix the test"
                   << "by locking the compositor before accessing it (see mutex()).";
    }
}

} // namespace MockCompositor
