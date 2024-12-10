// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMSOCKET_P_H
#define QWASMSOCKET_P_H

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

#include "QtCore/private/qeventdispatcher_wasm_p.h"

class QWasmSocket
{
public:
    static void registerSocketNotifier(QSocketNotifier *notifier);
    static void unregisterSocketNotifier(QSocketNotifier *notifier);
    static void clearSocketNotifiers();

    static void setEmscriptenSocketCallbacks();
    static void clearEmscriptenSocketCallbacks();
    static void socketError(int fd, int err, const char* msg, void *context);
    static void socketOpen(int fd, void *context);
    static void socketListen(int fd, void *context);
    static void socketConnection(int fd, void *context);
    static void socketMessage(int fd, void *context);
    static void socketClose(int fd, void *context);

    static void setSocketState(int socket, bool setReadyRead, bool setReadyWrite);
    static void clearSocketState(int socket);
    static void waitForSocketState(QEventDispatcherWasm *eventDispatcher, int timeout, int socket,
        bool checkRead, bool checkWrite, bool *selectForRead, bool *selectForWrite, bool *socketDisconnect);
private:

#if QT_CONFIG(thread)
    Q_CONSTINIT static std::mutex g_socketDataMutex;
#endif
    static std::multimap<int, QSocketNotifier *> g_socketNotifiers;
    struct SocketReadyState {
        QEventDispatcherWasm *waiter = nullptr;
        bool waitForReadyRead = false;
        bool waitForReadyWrite = false;
        bool readyRead = false;
        bool readyWrite = false;
    };
    static std::map<int, SocketReadyState> g_socketState;
};

#endif // QWASMSOCKET_P_H
