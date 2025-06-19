// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qlocalserver.h"
#include "qlocalserver_p.h"
#include "qlocalsocket.h"
#include <QtCore/private/qsystemerror_p.h>

#include <qdebug.h>

#include <aclapi.h>
#include <accctrl.h>
#include <sddl.h>

#include <memory>

// The buffer size need to be 0 otherwise data could be
// lost if the socket that has written data closes the connection
// before it is read.  Pipewriter is used for write buffering.
#define BUFSIZE 0

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

bool QLocalServerPrivate::addListener()
{
    // The object must not change its address once the
    // contained OVERLAPPED struct is passed to Windows.
    listeners.push_back(std::make_unique<Listener>());
    auto &listener = listeners.back();

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;      //non inheritable handle, same as default
    sa.lpSecurityDescriptor = 0;    //default security descriptor

    std::unique_ptr<SECURITY_DESCRIPTOR> pSD;
    PSID worldSID = 0;
    QByteArray aclBuffer;
    QByteArray tokenUserBuffer;
    QByteArray tokenGroupBuffer;

    // create security descriptor if access options were specified
    if ((socketOptions.value() & QLocalServer::WorldAccessOption)) {
        pSD.reset(new SECURITY_DESCRIPTOR);
        if (!InitializeSecurityDescriptor(pSD.get(), SECURITY_DESCRIPTOR_REVISION)) {
            setError("QLocalServerPrivate::addListener"_L1);
            return false;
        }
        HANDLE hToken = NULL;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
            return false;
        DWORD dwBufferSize = 0;
        GetTokenInformation(hToken, TokenUser, 0, 0, &dwBufferSize);
        tokenUserBuffer.fill(0, dwBufferSize);
        auto pTokenUser = reinterpret_cast<PTOKEN_USER>(tokenUserBuffer.data());
        if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwBufferSize, &dwBufferSize)) {
            setError("QLocalServerPrivate::addListener"_L1);
            CloseHandle(hToken);
            return false;
        }

        dwBufferSize = 0;
        GetTokenInformation(hToken, TokenPrimaryGroup, 0, 0, &dwBufferSize);
        tokenGroupBuffer.fill(0, dwBufferSize);
        auto pTokenGroup = reinterpret_cast<PTOKEN_PRIMARY_GROUP>(tokenGroupBuffer.data());
        if (!GetTokenInformation(hToken, TokenPrimaryGroup, pTokenGroup, dwBufferSize, &dwBufferSize)) {
            setError("QLocalServerPrivate::addListener"_L1);
            CloseHandle(hToken);
            return false;
        }
        CloseHandle(hToken);

#ifdef QLOCALSERVER_DEBUG
        DWORD groupNameSize;
        DWORD domainNameSize;
        SID_NAME_USE groupNameUse;
        LPWSTR groupNameSid;
        LookupAccountSid(0, pTokenGroup->PrimaryGroup, 0, &groupNameSize, 0, &domainNameSize, &groupNameUse);
        auto groupName = std::unique_ptr<wchar_t[]>(new wchar_t[groupNameSize]);
        auto domainName = std::unique_ptr<wchar_t[]>(new wchar_t[domainNameSize]);
        const bool lookup = LookupAccountSid(0, pTokenGroup->PrimaryGroup, groupName.get(),
                                             &groupNameSize, domainName.get(), &domainNameSize,
                                             &groupNameUse);
        if (lookup) {
            qDebug() << "primary group" << QString::fromWCharArray(domainName.get()) << "\\"
                     << QString::fromWCharArray(groupName.get()) << "type=" << groupNameUse;
        }
        if (ConvertSidToStringSid(pTokenGroup->PrimaryGroup, &groupNameSid)) {
            qDebug() << "primary group SID" << QString::fromWCharArray(groupNameSid) << "valid" << IsValidSid(pTokenGroup->PrimaryGroup);
            LocalFree(groupNameSid);
        }
#endif

        SID_IDENTIFIER_AUTHORITY WorldAuth = { SECURITY_WORLD_SID_AUTHORITY };
        if (!AllocateAndInitializeSid(&WorldAuth, 1, SECURITY_WORLD_RID,
            0, 0, 0, 0, 0, 0, 0,
            &worldSID)) {
            setError("QLocalServerPrivate::addListener"_L1);
            return false;
        }

        //calculate size of ACL buffer
        DWORD aclSize = sizeof(ACL) + ((sizeof(ACCESS_ALLOWED_ACE)) * 3);
        aclSize += GetLengthSid(pTokenUser->User.Sid) - sizeof(DWORD);
        aclSize += GetLengthSid(pTokenGroup->PrimaryGroup) - sizeof(DWORD);
        aclSize += GetLengthSid(worldSID) - sizeof(DWORD);
        aclSize = (aclSize + (sizeof(DWORD) - 1)) & 0xfffffffc;

        aclBuffer.fill(0, aclSize);
        auto acl = reinterpret_cast<PACL>(aclBuffer.data());
        InitializeAcl(acl, aclSize, ACL_REVISION_DS);

        if (socketOptions.value() & QLocalServer::UserAccessOption) {
            if (!AddAccessAllowedAce(acl, ACL_REVISION, FILE_ALL_ACCESS, pTokenUser->User.Sid)) {
                setError("QLocalServerPrivate::addListener"_L1);
                FreeSid(worldSID);
                return false;
            }
        }
        if (socketOptions.value() & QLocalServer::GroupAccessOption) {
            if (!AddAccessAllowedAce(acl, ACL_REVISION, FILE_ALL_ACCESS, pTokenGroup->PrimaryGroup)) {
                setError("QLocalServerPrivate::addListener"_L1);
                FreeSid(worldSID);
                return false;
            }
        }
        if (socketOptions.value() & QLocalServer::OtherAccessOption) {
            if (!AddAccessAllowedAce(acl, ACL_REVISION, FILE_ALL_ACCESS, worldSID)) {
                setError("QLocalServerPrivate::addListener"_L1);
                FreeSid(worldSID);
                return false;
            }
        }
        SetSecurityDescriptorOwner(pSD.get(), pTokenUser->User.Sid, FALSE);
        SetSecurityDescriptorGroup(pSD.get(), pTokenGroup->PrimaryGroup, FALSE);
        if (!SetSecurityDescriptorDacl(pSD.get(), TRUE, acl, FALSE)) {
            setError("QLocalServerPrivate::addListener"_L1);
            FreeSid(worldSID);
            return false;
        }

        sa.lpSecurityDescriptor = pSD.get();
    }

    listener->handle = CreateNamedPipe(
                 reinterpret_cast<const wchar_t *>(fullServerName.utf16()), // pipe name
                 PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,       // read/write access
                 PIPE_TYPE_BYTE |          // byte type pipe
                 PIPE_READMODE_BYTE |      // byte-read mode
                 PIPE_WAIT,                // blocking mode
                 PIPE_UNLIMITED_INSTANCES, // max. instances
                 BUFSIZE,                  // output buffer size
                 BUFSIZE,                  // input buffer size
                 3000,                     // client time-out
                 &sa);

    if (listener->handle == INVALID_HANDLE_VALUE) {
        setError("QLocalServerPrivate::addListener"_L1);
        listeners.pop_back();
        return false;
    }

    if (worldSID)
        FreeSid(worldSID);

    memset(&listener->overlapped, 0, sizeof(OVERLAPPED));
    listener->overlapped.hEvent = eventHandle;

    // Beware! ConnectNamedPipe will reset the eventHandle to non-signaled.
    // Callers of addListener must check all listeners for connections.
    if (!ConnectNamedPipe(listener->handle, &listener->overlapped)) {
        switch (GetLastError()) {
        case ERROR_IO_PENDING:
            listener->connected = false;
            break;
        case ERROR_PIPE_CONNECTED:
            listener->connected = true;
            break;
        default:
            CloseHandle(listener->handle);
            setError("QLocalServerPrivate::addListener"_L1);
            listeners.pop_back();
            return false;
        }
    } else {
        Q_ASSERT_X(false, "QLocalServerPrivate::addListener", "The impossible happened");
        SetEvent(eventHandle);
    }
    return true;
}

void QLocalServerPrivate::setError(const QString &function)
{
    int windowsError = GetLastError();
    errorString = QString::fromLatin1("%1: %2").arg(function, qt_error_string(windowsError));
    error = QAbstractSocket::UnknownSocketError;
}

void QLocalServerPrivate::init()
{
}

bool QLocalServerPrivate::removeServer(const QString &name)
{
    Q_UNUSED(name);
    return true;
}

bool QLocalServerPrivate::listen(const QString &name)
{
    Q_Q(QLocalServer);

    const auto pipePath = "\\\\.\\pipe\\"_L1;
    if (name.startsWith(pipePath))
        fullServerName = name;
    else
        fullServerName = pipePath + name;

    // Use only one event for all listeners of one socket.
    // The idea is that listener events are rare, so polling all listeners once in a while is
    // cheap compared to waiting for N additional events in each iteration of the main loop.
    eventHandle = CreateEvent(NULL, TRUE, FALSE, NULL); // If the function fails, the return value is NULL
    connectionEventNotifier = new QWinEventNotifier(eventHandle , q);
    q->connect(connectionEventNotifier, SIGNAL(activated(HANDLE)), q, SLOT(_q_onNewConnection()));

    for (int i = 0; i < listenBacklog; ++i)
        if (!addListener())
            return false;

    _q_onNewConnection();
    return true;
}

bool QLocalServerPrivate::listen(qintptr)
{
    qWarning("QLocalServer::listen(qintptr) is not supported on Windows QTBUG-24230");
    return false;
}

void QLocalServerPrivate::_q_onNewConnection()
{
    Q_Q(QLocalServer);
    DWORD dummy;
    bool tryAgain;
    do {
        tryAgain = false;

        // Reset first, otherwise we could reset an event which was asserted
        // immediately after we checked the conn status.
        ResetEvent(eventHandle);

        // Testing shows that there is indeed absolutely no guarantee which listener gets
        // a client connection first, so there is no way around polling all of them.
        for (size_t i = 0; i < listeners.size(); ) {
            HANDLE handle = listeners[i]->handle;
            if (listeners[i]->connected
                || GetOverlappedResult(handle, &listeners[i]->overlapped, &dummy, FALSE))
            {
                listeners.erase(listeners.begin() + i);

                addListener();

                if (pendingConnections.size() > maxPendingConnections)
                    connectionEventNotifier->setEnabled(false);
                else
                    tryAgain = true;

                // Make this the last thing so connected slots can wreak the least havoc
                q->incomingConnection(reinterpret_cast<quintptr>(handle));
            } else {
                if (GetLastError() != ERROR_IO_INCOMPLETE) {
                    q->close();
                    setError("QLocalServerPrivate::_q_onNewConnection"_L1);
                    return;
                }

                ++i;
            }
        }
    } while (tryAgain);
}

void QLocalServerPrivate::closeServer()
{
    connectionEventNotifier->setEnabled(false); // Otherwise, closed handle is checked before deleter runs
    connectionEventNotifier->deleteLater();
    connectionEventNotifier = 0;
    CloseHandle(eventHandle);
    eventHandle = nullptr;
    for (size_t i = 0; i < listeners.size(); ++i)
        CloseHandle(listeners[i]->handle);
    listeners.clear();
}

void QLocalServerPrivate::waitForNewConnection(int msecs, bool *timedOut)
{
    Q_Q(QLocalServer);
    if (!pendingConnections.isEmpty() || !q->isListening())
        return;

    DWORD result = WaitForSingleObject(eventHandle, (msecs == -1) ? INFINITE : msecs);
    if (result == WAIT_TIMEOUT) {
        if (timedOut)
            *timedOut = true;
    } else {
        _q_onNewConnection();
    }
}

QT_END_NAMESPACE
