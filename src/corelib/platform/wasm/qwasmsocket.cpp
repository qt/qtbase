// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmsocket_p.h"
#include "qwasmglobal_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qsocketnotifier.h>

#include "emscripten.h"
#include <sys/ioctl.h>

#if QT_CONFIG(thread)
#define LOCK_GUARD(M) std::lock_guard<std::mutex> lock(M)
#else
#define LOCK_GUARD(M)
#endif

#if QT_CONFIG(thread)
Q_CONSTINIT std::mutex QWasmSocket::g_socketDataMutex;
#endif

// ### dynamic initialization:
std::multimap<int, QSocketNotifier *> QWasmSocket::g_socketNotifiers;
std::map<int, QWasmSocket::SocketReadyState> QWasmSocket::g_socketState;

void QWasmSocket::registerSocketNotifier(QSocketNotifier *notifier)
{
    LOCK_GUARD(g_socketDataMutex);

    bool wasEmpty = g_socketNotifiers.empty();
    g_socketNotifiers.insert({notifier->socket(), notifier});
    if (wasEmpty)
        qwasmglobal::runOnMainThread([] { setEmscriptenSocketCallbacks(); });

    int count;
    ioctl(notifier->socket(), FIONREAD, &count);

    // message may have arrived already
    if (count > 0 && notifier->type() == QSocketNotifier::Read) {
        QCoreApplication::postEvent(notifier, new QEvent(QEvent::SockAct));
    }
}

void QWasmSocket::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    LOCK_GUARD(g_socketDataMutex);

    auto notifiers = g_socketNotifiers.equal_range(notifier->socket());
    for (auto it = notifiers.first; it != notifiers.second; ++it) {
        if (it->second == notifier) {
            g_socketNotifiers.erase(it);
            break;
        }
    }

    if (g_socketNotifiers.empty())
        qwasmglobal::runOnMainThread([] { clearEmscriptenSocketCallbacks(); });
}

void QWasmSocket::clearSocketNotifiers()
{
    LOCK_GUARD(g_socketDataMutex);
    if (!g_socketNotifiers.empty()) {
        qWarning("QWasmSocket: Sockets cleared with active socket notifiers");
        clearEmscriptenSocketCallbacks();
        g_socketNotifiers.clear();
    }
    g_socketState.clear();
}

void QWasmSocket::setEmscriptenSocketCallbacks()
{
    qCDebug(lcEventDispatcher) << "setEmscriptenSocketCallbacks";

    emscripten_set_socket_error_callback(nullptr, QWasmSocket::socketError);
    emscripten_set_socket_open_callback(nullptr, QWasmSocket::socketOpen);
    emscripten_set_socket_listen_callback(nullptr, QWasmSocket::socketListen);
    emscripten_set_socket_connection_callback(nullptr, QWasmSocket::socketConnection);
    emscripten_set_socket_message_callback(nullptr, QWasmSocket::socketMessage);
    emscripten_set_socket_close_callback(nullptr, QWasmSocket::socketClose);
}

void QWasmSocket::clearEmscriptenSocketCallbacks()
{
    qCDebug(lcEventDispatcher) << "clearEmscriptenSocketCallbacks";

    emscripten_set_socket_error_callback(nullptr, nullptr);
    emscripten_set_socket_open_callback(nullptr, nullptr);
    emscripten_set_socket_listen_callback(nullptr, nullptr);
    emscripten_set_socket_connection_callback(nullptr, nullptr);
    emscripten_set_socket_message_callback(nullptr, nullptr);
    emscripten_set_socket_close_callback(nullptr, nullptr);
}

void QWasmSocket::socketError(int socket, int err, const char* msg, void *context)
{
    Q_UNUSED(err);
    Q_UNUSED(msg);
    Q_UNUSED(context);

    // Emscripten makes socket callbacks while the main thread is busy-waiting for a mutex,
    // which can cause deadlocks if the callback code also tries to lock the same mutex.
    // This is most easily reproducible by adding print statements, where each print requires
    // taking a mutex lock. Work around this by running the callback asynchronously, i.e. by using
    // a native zero-timer, to make sure the main thread stack is completely unwond before calling
    // the Qt handler.
    // It is currently unclear if this problem is caused by code in Qt or in Emscripten, or
    // if this completely fixes the problem.
    qwasmglobal::runAsync([socket](){
        auto notifiersRange = g_socketNotifiers.equal_range(socket);
        std::vector<std::pair<int, QSocketNotifier *>> notifiers(notifiersRange.first, notifiersRange.second);
        for (auto [_, notifier]: notifiers) {
            QCoreApplication::postEvent(notifier, new QEvent(QEvent::SockAct));
        }
        setSocketState(socket, true, true);
    });
}

void QWasmSocket::socketOpen(int socket, void *context)
{
    Q_UNUSED(context);

    qwasmglobal::runAsync([socket](){
        auto notifiersRange = g_socketNotifiers.equal_range(socket);
        std::vector<std::pair<int, QSocketNotifier *>> notifiers(notifiersRange.first, notifiersRange.second);
        for (auto [_, notifier]: notifiers) {
            if (notifier->type() == QSocketNotifier::Write) {
                QCoreApplication::postEvent(notifier, new QEvent(QEvent::SockAct));
            }
        }
        setSocketState(socket, false, true);
    });
}

void QWasmSocket::socketListen(int socket, void *context)
{
    Q_UNUSED(socket);
    Q_UNUSED(context);
}

void QWasmSocket::socketConnection(int socket, void *context)
{
    Q_UNUSED(socket);
    Q_UNUSED(context);
}

void QWasmSocket::socketMessage(int socket, void *context)
{
    Q_UNUSED(context);

    qwasmglobal::runAsync([socket](){
        auto notifiersRange = g_socketNotifiers.equal_range(socket);
        std::vector<std::pair<int, QSocketNotifier *>> notifiers(notifiersRange.first, notifiersRange.second);
        for (auto [_, notifier]: notifiers) {
            if (notifier->type() == QSocketNotifier::Read) {
                QCoreApplication::postEvent(notifier, new QEvent(QEvent::SockAct));
            }
        }
        setSocketState(socket, true, false);
    });
}

void QWasmSocket::socketClose(int socket, void *context)
{
    Q_UNUSED(context);

    // Emscripten makes emscripten_set_socket_close_callback() calls to socket 0,
    // which is not a valid socket. see https://github.com/emscripten-core/emscripten/issues/6596
    if (socket == 0)
        return;

    qwasmglobal::runAsync([socket](){
        auto notifiersRange = g_socketNotifiers.equal_range(socket);
        std::vector<std::pair<int, QSocketNotifier *>> notifiers(notifiersRange.first, notifiersRange.second);
        for (auto [_, notifier]: notifiers)
            QCoreApplication::postEvent(notifier, new QEvent(QEvent::SockClose));

        setSocketState(socket, true, true);
        clearSocketState(socket);
    });
}

void QWasmSocket::setSocketState(int socket, bool setReadyRead, bool setReadyWrite)
{
    LOCK_GUARD(g_socketDataMutex);
    SocketReadyState &state = g_socketState[socket];

    // Additively update socket ready state, e.g. if it
    // was already ready read then it stays ready read.
    state.readyRead |= setReadyRead;
    state.readyWrite |= setReadyWrite;

    // Wake any waiters for the given readiness. The waiter consumes
    // the ready state, returning the socket to not-ready.
    if (QEventDispatcherWasm *waiter = state.waiter)
        if ((state.readyRead && state.waitForReadyRead) || (state.readyWrite && state.waitForReadyWrite))
            waiter->wakeUp();
}

void QWasmSocket::clearSocketState(int socket)
{
    LOCK_GUARD(g_socketDataMutex);
    g_socketState.erase(socket);
}

void QWasmSocket::waitForSocketState(QEventDispatcherWasm *eventDispatcher, int timeout, int socket, bool checkRead,
    bool checkWrite, bool *selectForRead, bool *selectForWrite, bool *socketDisconnect)
{
    // Loop until the socket becomes readyRead or readyWrite. Wait for
    // socket activity if it currently is neither.
    while (true) {
        *selectForRead = false;
        *selectForWrite = false;

        {
            LOCK_GUARD(g_socketDataMutex);

            // Access or create socket state: we want to register that a thread is waitng
            // even if we have not received any socket callbacks yet.
            SocketReadyState &state = g_socketState[socket];
            if (state.waiter) {
                qWarning() << "QEventDispatcherWasm::waitForSocketState: a thread is already waiting";
                break;
            }

            bool shouldWait = true;
            if (checkRead && state.readyRead) {
                shouldWait = false;
                state.readyRead = false;
                *selectForRead = true;
            }
            if (checkWrite && state.readyWrite) {
                shouldWait = false;
                state.readyWrite = false;
                *selectForRead = true;
            }
            if (!shouldWait)
                break;

            state.waiter = eventDispatcher;
            state.waitForReadyRead = checkRead;
            state.waitForReadyWrite = checkWrite;
        }

        bool didTimeOut = !eventDispatcher->wait(timeout);
        {
            LOCK_GUARD(g_socketDataMutex);

            // Missing socket state after a wakeup means that the socket has been closed.
            auto it = g_socketState.find(socket);
            if (it == g_socketState.end()) {
                *socketDisconnect = true;
                break;
            }
            it->second.waiter = nullptr;
            it->second.waitForReadyRead = false;
            it->second.waitForReadyWrite = false;
        }

        if (didTimeOut)
            break;
    }
}
