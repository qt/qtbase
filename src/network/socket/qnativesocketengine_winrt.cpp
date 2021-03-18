/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qt_windows.h>

#include "qnativesocketengine_winrt_p.h"

#include <qcoreapplication.h>
#include <qabstracteventdispatcher.h>
#include <qsocketnotifier.h>
#include <qdatetime.h>
#include <qnetworkinterface.h>
#include <qelapsedtimer.h>
#include <qthread.h>
#include <qabstracteventdispatcher.h>
#include <qfunctions_winrt.h>

#include <private/qthread_p.h>
#include <private/qabstractsocket_p.h>
#include <private/qeventdispatcher_winrt_p.h>

#ifndef QT_NO_SSL
#include <QSslSocket>
#endif

#include <functional>
#include <wrl.h>
#include <windows.foundation.collections.h>
#include <windows.storage.streams.h>
#include <windows.networking.h>
#include <windows.networking.sockets.h>
#include <robuffer.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Storage::Streams;
using namespace ABI::Windows::Networking;
using namespace ABI::Windows::Networking::Connectivity;
using namespace ABI::Windows::Networking::Sockets;
#if _MSC_VER >= 1900
using namespace ABI::Windows::Security::EnterpriseData;
#endif

typedef ITypedEventHandler<StreamSocketListener *, StreamSocketListenerConnectionReceivedEventArgs *> ClientConnectedHandler;
typedef ITypedEventHandler<DatagramSocket *, DatagramSocketMessageReceivedEventArgs *> DatagramReceivedHandler;
typedef IAsyncOperationWithProgressCompletedHandler<IBuffer *, UINT32> SocketReadCompletedHandler;
typedef IAsyncOperationWithProgressCompletedHandler<UINT32, UINT32> SocketWriteCompletedHandler;
typedef IAsyncOperationWithProgress<IBuffer *, UINT32> IAsyncBufferOperation;

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcNetworkSocket, "qt.network.socket");
Q_LOGGING_CATEGORY(lcNetworkSocketVerbose, "qt.network.socket.verbose");

#if _MSC_VER >= 1900
static HRESULT qt_winrt_try_create_thread_network_context(QString host, ComPtr<IThreadNetworkContext> &context)
{
    HRESULT hr;
    ComPtr<IProtectionPolicyManagerStatics> protectionPolicyManager;

    hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_EnterpriseData_ProtectionPolicyManager).Get(),
                              &protectionPolicyManager);
    RETURN_HR_IF_FAILED("Could not access ProtectionPolicyManager statics.");

    ComPtr<IHostNameFactory> hostNameFactory;
    hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_HostName).Get(),
                              &hostNameFactory);
    RETURN_HR_IF_FAILED("Could not access HostName factory.");

    ComPtr<IHostName> hostName;
    HStringReference hostRef(reinterpret_cast<LPCWSTR>(host.utf16()), host.length());
    hr = hostNameFactory->CreateHostName(hostRef.Get(), &hostName);
    RETURN_HR_IF_FAILED("Could not create hostname.");

    ComPtr<IAsyncOperation<HSTRING>> op;
    hr = protectionPolicyManager->GetPrimaryManagedIdentityForNetworkEndpointAsync(hostName.Get(), &op);
    RETURN_HR_IF_FAILED("Could not get identity operation.");

    HSTRING hIdentity;
    hr = QWinRTFunctions::await(op, &hIdentity);
    RETURN_HR_IF_FAILED("Could not wait for identity operation.");

    // Implies there is no need for a network context for this address
    if (hIdentity == nullptr)
        return S_OK;

    hr = protectionPolicyManager->CreateCurrentThreadNetworkContext(hIdentity, &context);
    RETURN_HR_IF_FAILED("Could not create thread network context");

    return S_OK;
}
#endif // _MSC_VER >= 1900

typedef QHash<qintptr, IStreamSocket *> TcpSocketHash;

struct SocketHandler
{
    SocketHandler() : socketCount(0) {}
    qintptr socketCount;
    TcpSocketHash pendingTcpSockets;
};

Q_GLOBAL_STATIC(SocketHandler, gSocketHandler)

struct SocketGlobal
{
    SocketGlobal()
    {
        HRESULT hr;
        hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_Streams_Buffer).Get(),
                                  &bufferFactory);
        Q_ASSERT_SUCCEEDED(hr);
    }

    ComPtr<IBufferFactory> bufferFactory;
};
Q_GLOBAL_STATIC(SocketGlobal, g)

#define READ_BUFFER_SIZE 65536

static inline QString qt_QStringFromHString(const HString &string)
{
    UINT32 length;
    PCWSTR rawString = string.GetRawBuffer(&length);
    return QString::fromWCharArray(rawString, length);
}

class SocketEngineWorker : public QObject
{
    Q_OBJECT
public:
    SocketEngineWorker(QNativeSocketEnginePrivate *engine)
            : enginePrivate(engine)
    {
        qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << engine;
    }

    ~SocketEngineWorker()
    {
        qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO;
        if (Q_UNLIKELY(initialReadOp)) {
            qCDebug(lcNetworkSocket) << Q_FUNC_INFO << "Closing initial read operation";
            ComPtr<IAsyncInfo> info;
            HRESULT hr = initialReadOp.As(&info);
            Q_ASSERT_SUCCEEDED(hr);
            if (info) {
                hr = info->Cancel();
                Q_ASSERT_SUCCEEDED(hr);
                hr = info->Close();
                Q_ASSERT_SUCCEEDED(hr);
            }
        }

        if (readOp) {
            qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << "Closing read operation";
            ComPtr<IAsyncInfo> info;
            HRESULT hr = readOp.As(&info);
            Q_ASSERT_SUCCEEDED(hr);
            if (info) {
                hr = info->Cancel();
                Q_ASSERT_SUCCEEDED(hr);
                hr = info->Close();
                Q_ASSERT_SUCCEEDED(hr);
            }
        }

        if (connectOp) {
            qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << "Closing connect operation";
            ComPtr<IAsyncInfo> info;
            HRESULT hr = connectOp.As(&info);
            Q_ASSERT_SUCCEEDED(hr);
            if (info) {
                hr = info->Cancel();
                Q_ASSERT_SUCCEEDED(hr);
                hr = info->Close();
                Q_ASSERT_SUCCEEDED(hr);
            }
        }
    }

signals:
    void connectOpFinished(bool success, QAbstractSocket::SocketError error, WinRTSocketEngine::ErrorString errorString);
    void newDataReceived();
    void socketErrorOccured(QAbstractSocket::SocketError error);

public:
    void startReading()
    {
        qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO;
        ComPtr<IBuffer> buffer;
        HRESULT hr = g->bufferFactory->Create(READ_BUFFER_SIZE, &buffer);
        Q_ASSERT_SUCCEEDED(hr);
        ComPtr<IInputStream> stream;
        hr = tcpSocket->get_InputStream(&stream);
        Q_ASSERT_SUCCEEDED(hr);
        hr = stream->ReadAsync(buffer.Get(), READ_BUFFER_SIZE, InputStreamOptions_Partial, initialReadOp.GetAddressOf());
        Q_ASSERT_SUCCEEDED(hr);
        enginePrivate->socketState = QAbstractSocket::ConnectedState;
        hr = initialReadOp->put_Completed(Callback<SocketReadCompletedHandler>(this, &SocketEngineWorker::onReadyRead).Get());
        Q_ASSERT_SUCCEEDED(hr);
    }

    HRESULT onConnectOpFinished(IAsyncAction *action, AsyncStatus)
    {
        qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO;
        HRESULT hr = action->GetResults();
        if (FAILED(hr)) {
            if (hr == HRESULT_FROM_WIN32(WSAETIMEDOUT)) {
                emit connectOpFinished(false, QAbstractSocket::NetworkError, WinRTSocketEngine::ConnectionTimeOutErrorString);
                return S_OK;
            } else if (hr == HRESULT_FROM_WIN32(WSAEHOSTUNREACH)) {
                emit connectOpFinished(false, QAbstractSocket::HostNotFoundError, WinRTSocketEngine::HostUnreachableErrorString);
                return S_OK;
            } else if (hr == HRESULT_FROM_WIN32(WSAECONNREFUSED)) {
                emit connectOpFinished(false, QAbstractSocket::ConnectionRefusedError, WinRTSocketEngine::ConnectionRefusedErrorString);
                return S_OK;
            } else {
                emit connectOpFinished(false, QAbstractSocket::UnknownSocketError, WinRTSocketEngine::UnknownSocketErrorString);
                return S_OK;
            }
        }

        // The callback might be triggered several times if we do not cancel/reset it here
        if (connectOp) {
            ComPtr<IAsyncInfo> info;
            hr = connectOp.As(&info);
            Q_ASSERT_SUCCEEDED(hr);
            if (info) {
                hr = info->Cancel();
                Q_ASSERT_SUCCEEDED(hr);
                hr = info->Close();
                Q_ASSERT_SUCCEEDED(hr);
            }
            hr = connectOp.Reset();
            Q_ASSERT_SUCCEEDED(hr);
        }

        emit connectOpFinished(true, QAbstractSocket::UnknownSocketError, WinRTSocketEngine::UnknownSocketErrorString);
        return S_OK;
    }

    HRESULT OnNewDatagramReceived(IDatagramSocket *, IDatagramSocketMessageReceivedEventArgs *args)
    {
        qCDebug(lcNetworkSocketVerbose) << this << Q_FUNC_INFO;
        WinRtDatagram datagram;
        QHostAddress returnAddress;
        ComPtr<IHostName> remoteHost;
        HRESULT hr = args->get_RemoteAddress(&remoteHost);
        RETURN_OK_IF_FAILED("Could not obtain remote host");
        HString remoteHostString;
        hr = remoteHost->get_CanonicalName(remoteHostString.GetAddressOf());
        RETURN_OK_IF_FAILED("Could not obtain remote host's canonical name");
        returnAddress.setAddress(qt_QStringFromHString(remoteHostString));
        datagram.header.senderAddress = returnAddress;
        HString remotePort;
        hr = args->get_RemotePort(remotePort.GetAddressOf());
        RETURN_OK_IF_FAILED("Could not obtain remote port");
        datagram.header.senderPort = qt_QStringFromHString(remotePort).toInt();

        ComPtr<IDataReader> reader;
        hr = args->GetDataReader(&reader);
        RETURN_OK_IF_FAILED("Could not obtain data reader");
        quint32 length;
        hr = reader->get_UnconsumedBufferLength(&length);
        RETURN_OK_IF_FAILED("Could not obtain unconsumed buffer length");
        datagram.data.resize(length);
        hr = reader->ReadBytes(length, reinterpret_cast<BYTE *>(datagram.data.data()));
        RETURN_OK_IF_FAILED("Could not read datagram");

        QMutexLocker locker(&mutex);
        // Notify the engine about new datagrams being present at the next event loop iteration
        if (emitDataReceived)
            emit newDataReceived();
        pendingDatagrams << datagram;

        return S_OK;
    }

    HRESULT onReadyRead(IAsyncBufferOperation *asyncInfo, AsyncStatus status)
    {
        qCDebug(lcNetworkSocketVerbose) << this << Q_FUNC_INFO;
        if (asyncInfo == initialReadOp.Get()) {
            initialReadOp.Reset();
        } else if (asyncInfo == readOp.Get()) {
            readOp.Reset();
        } else {
            Q_ASSERT(false);
        }

        // A read in UnconnectedState will close the socket and return -1 and thus tell the caller,
        // that the connection was closed. The socket cannot be closed here, as the subsequent read
        // might fail then.
        if (status == Error || status == Canceled) {
            qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << "Remote host closed";
            emit socketErrorOccured(QAbstractSocket::RemoteHostClosedError);
            return S_OK;
        }

        ComPtr<IBuffer> buffer;
        HRESULT hr = asyncInfo->GetResults(&buffer);
        if (FAILED(hr)) {
            qErrnoWarning(hr, "Failed to get read results buffer");
            emit socketErrorOccured(QAbstractSocket::UnknownSocketError);
            return S_OK;
        }

        UINT32 bufferLength;
        hr = buffer->get_Length(&bufferLength);
        if (FAILED(hr)) {
            qErrnoWarning(hr, "Failed to get buffer length");
            emit socketErrorOccured(QAbstractSocket::UnknownSocketError);
            return S_OK;
        }
        // A zero sized buffer length signals, that the remote host closed the connection. The socket
        // cannot be closed though, as the following read might have socket descriptor -1 and thus and
        // the closing of the socket won't be communicated to the caller. So only the error is set. The
        // actual socket close happens inside of read.
        if (!bufferLength) {
            qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << "Remote host closed";
            emit socketErrorOccured(QAbstractSocket::RemoteHostClosedError);
            return S_OK;
        }

        ComPtr<Windows::Storage::Streams::IBufferByteAccess> byteArrayAccess;
        hr = buffer.As(&byteArrayAccess);
        if (FAILED(hr)) {
            qErrnoWarning(hr, "Failed to get cast buffer");
            emit socketErrorOccured(QAbstractSocket::UnknownSocketError);
            return S_OK;
        }
        byte *data;
        hr = byteArrayAccess->Buffer(&data);
        if (FAILED(hr)) {
            qErrnoWarning(hr, "Failed to access buffer data");
            emit socketErrorOccured(QAbstractSocket::UnknownSocketError);
            return S_OK;
        }

        QByteArray newData(reinterpret_cast<const char*>(data), qint64(bufferLength));

        QMutexLocker readLocker(&mutex);
        emit newDataReceived();
        pendingData.append(newData);
        readLocker.unlock();

        hr = QEventDispatcherWinRT::runOnXamlThread([buffer, this]() {
            UINT32 readBufferLength;
            ComPtr<IInputStream> stream;
            HRESULT hr = tcpSocket->get_InputStream(&stream);
            if (FAILED(hr)) {
                qErrnoWarning(hr, "Failed to obtain input stream");
                emit socketErrorOccured(QAbstractSocket::UnknownSocketError);
                return S_OK;
            }

            // Reuse the stream buffer
            hr = buffer->get_Capacity(&readBufferLength);
            if (FAILED(hr)) {
                qErrnoWarning(hr, "Failed to get buffer capacity");
                emit socketErrorOccured(QAbstractSocket::UnknownSocketError);
                return S_OK;
            }
            hr = buffer->put_Length(0);
            if (FAILED(hr)) {
                qErrnoWarning(hr, "Failed to set buffer length");
                emit socketErrorOccured(QAbstractSocket::UnknownSocketError);
                return S_OK;
            }

            hr = stream->ReadAsync(buffer.Get(), readBufferLength, InputStreamOptions_Partial, &readOp);
            if (FAILED(hr)) {
                qErrnoWarning(hr, "onReadyRead(): Could not read into socket stream buffer.");
                emit socketErrorOccured(QAbstractSocket::UnknownSocketError);
                return S_OK;
            }
            hr = readOp->put_Completed(Callback<SocketReadCompletedHandler>(this, &SocketEngineWorker::onReadyRead).Get());
            if (FAILED(hr)) {
                qErrnoWarning(hr, "onReadyRead(): Failed to set socket read callback.");
                emit socketErrorOccured(QAbstractSocket::UnknownSocketError);
                return S_OK;
            }
            return S_OK;
        });
        Q_ASSERT_SUCCEEDED(hr);
        return S_OK;
    }

    void setTcpSocket(ComPtr<IStreamSocket> socket) { tcpSocket = socket; }

private:
    friend class QNativeSocketEngine;
    ComPtr<IStreamSocket> tcpSocket;

    QList<WinRtDatagram> pendingDatagrams;
    bool emitDataReceived = true;
    QByteArray pendingData;

    // Protects pendingData/pendingDatagrams which are accessed from native callbacks
    QMutex mutex;

    ComPtr<IAsyncAction> connectOp;
    ComPtr<IAsyncOperationWithProgress<IBuffer *, UINT32>> initialReadOp;
    ComPtr<IAsyncOperationWithProgress<IBuffer *, UINT32>> readOp;

    QNativeSocketEnginePrivate *enginePrivate;
};

static QByteArray socketDescription(const QAbstractSocketEngine *s)
{
    QByteArray result;
    if (const QObject *o = s->parent()) {
        const QString name = o->objectName();
        if (!name.isEmpty()) {
            result += '"';
            result += name.toLocal8Bit();
            result += "\"/";
        }
        result += o->metaObject()->className();
    }
    return result;
}

// Common constructs
#define Q_CHECK_VALID_SOCKETLAYER(function, returnValue) do { \
    if (!isValid()) { \
        qWarning(""#function" was called on an uninitialized socket device"); \
        return returnValue; \
    } } while (0)
#define Q_CHECK_INVALID_SOCKETLAYER(function, returnValue) do { \
    if (isValid()) { \
        qWarning(""#function" was called on an already initialized socket device"); \
        return returnValue; \
    } } while (0)
#define Q_CHECK_STATE(function, checkState, returnValue) do { \
    if (d->socketState != (checkState)) { \
        qWarning(""#function" was not called in "#checkState); \
        return (returnValue); \
    } } while (0)
#define Q_CHECK_NOT_STATE(function, checkState, returnValue) do { \
    if (d->socketState == (checkState)) { \
        qWarning(""#function" was called in "#checkState); \
        return (returnValue); \
    } } while (0)
#define Q_CHECK_STATES(function, state1, state2, returnValue) do { \
    if (d->socketState != (state1) && d->socketState != (state2)) { \
        qWarning(""#function" was called" \
                 " not in "#state1" or "#state2); \
        return (returnValue); \
    } } while (0)
#define Q_CHECK_STATES3(function, state1, state2, state3, returnValue) do { \
    if (d->socketState != (state1) && d->socketState != (state2) && d->socketState != (state3)) { \
        qWarning(""#function" was called" \
                 " not in "#state1", "#state2" or "#state3); \
        return (returnValue); \
    } } while (0)
#define Q_CHECK_TYPE(function, type, returnValue) do { \
    if (d->socketType != (type)) { \
        qWarning(#function" was called by a" \
                 " socket other than "#type""); \
        return (returnValue); \
    } } while (0)
#define Q_TR(a) QT_TRANSLATE_NOOP(QNativeSocketEngine, a)

template <typename T>
static AsyncStatus opStatus(const ComPtr<T> &op)
{
    ComPtr<IAsyncInfo> info;
    HRESULT hr = op.As(&info);
    Q_ASSERT_SUCCEEDED(hr);
    AsyncStatus status;
    hr = info->get_Status(&status);
    Q_ASSERT_SUCCEEDED(hr);
    return status;
}

static qint64 writeIOStream(ComPtr<IOutputStream> stream, const char *data, qint64 len)
{
    qCDebug(lcNetworkSocket) << Q_FUNC_INFO << data << len;
    ComPtr<IBuffer> buffer;
    HRESULT hr = g->bufferFactory->Create(len, &buffer);
    Q_ASSERT_SUCCEEDED(hr);
    hr = buffer->put_Length(len);
    Q_ASSERT_SUCCEEDED(hr);
    ComPtr<Windows::Storage::Streams::IBufferByteAccess> byteArrayAccess;
    hr = buffer.As(&byteArrayAccess);
    Q_ASSERT_SUCCEEDED(hr);
    byte *bytes;
    hr = byteArrayAccess->Buffer(&bytes);
    Q_ASSERT_SUCCEEDED(hr);
    memcpy(bytes, data, len);
    ComPtr<IAsyncOperationWithProgress<UINT32, UINT32>> op;
    hr = stream->WriteAsync(buffer.Get(), &op);
    RETURN_IF_FAILED("Failed to write to stream", return -1);
    UINT32 bytesWritten;
    hr = QWinRTFunctions::await(op, &bytesWritten);
    RETURN_IF_FAILED("Failed to write to stream", return -1);
    return bytesWritten;
}

QNativeSocketEngine::QNativeSocketEngine(QObject *parent)
    : QAbstractSocketEngine(*new QNativeSocketEnginePrivate(), parent)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << parent;
    qRegisterMetaType<WinRtDatagram>();
    qRegisterMetaType<WinRTSocketEngine::ErrorString>();
    Q_D(QNativeSocketEngine);
#ifndef QT_NO_SSL
    if (parent)
        d->sslSocket = qobject_cast<QSslSocket *>(parent->parent());
#endif

    connect(this, &QNativeSocketEngine::connectionReady,
            this, &QNativeSocketEngine::connectionNotification, Qt::QueuedConnection);
    connect(this, &QNativeSocketEngine::readReady,
            this, &QNativeSocketEngine::processReadReady, Qt::QueuedConnection);
    connect(this, &QNativeSocketEngine::writeReady,
            this, &QNativeSocketEngine::writeNotification, Qt::QueuedConnection);
    connect(d->worker, &SocketEngineWorker::connectOpFinished,
            this, &QNativeSocketEngine::handleConnectOpFinished, Qt::QueuedConnection);
    connect(d->worker, &SocketEngineWorker::newDataReceived, this, &QNativeSocketEngine::handleNewData, Qt::QueuedConnection);
    connect(d->worker, &SocketEngineWorker::socketErrorOccured,
            this, &QNativeSocketEngine::handleTcpError, Qt::QueuedConnection);
}

QNativeSocketEngine::~QNativeSocketEngine()
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO;
    close();
}

bool QNativeSocketEngine::initialize(QAbstractSocket::SocketType type, QAbstractSocket::NetworkLayerProtocol protocol)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << type << protocol;
    Q_D(QNativeSocketEngine);
    if (isValid())
        close();

    // Create the socket
    if (!d->createNewSocket(type, protocol))
        return false;

    if (type == QAbstractSocket::UdpSocket) {
        // Set the broadcasting flag if it's a UDP socket.
        if (!setOption(BroadcastSocketOption, 1)) {
            d->setError(QAbstractSocket::UnsupportedSocketOperationError,
                WinRTSocketEngine::BroadcastingInitFailedErrorString);
            close();
            return false;
        }

        // Set some extra flags that are interesting to us, but accept failure
        setOption(ReceivePacketInformation, 1);
        setOption(ReceiveHopLimit, 1);
    }


    // Make sure we receive out-of-band data
    if (type == QAbstractSocket::TcpSocket
        && !setOption(ReceiveOutOfBandData, 1)) {
        qWarning("QNativeSocketEngine::initialize unable to inline out-of-band data");
    }


    d->socketType = type;
    d->socketProtocol = protocol;
    return true;
}

bool QNativeSocketEngine::initialize(qintptr socketDescriptor, QAbstractSocket::SocketState socketState)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << socketDescriptor << socketState;
    Q_D(QNativeSocketEngine);

    if (isValid())
        close();

    // Currently, only TCP sockets are initialized this way.
    IStreamSocket *socket = gSocketHandler->pendingTcpSockets.take(socketDescriptor);
    d->socketDescriptor = qintptr(socket);
    d->socketType = QAbstractSocket::TcpSocket;

    if (!d->socketDescriptor || !d->fetchConnectionParameters()) {
        d->setError(QAbstractSocket::UnsupportedSocketOperationError,
            WinRTSocketEngine::InvalidSocketErrorString);
        d->socketDescriptor = -1;
        return false;
    }

    // Start processing incoming data
    if (d->socketType == QAbstractSocket::TcpSocket) {
        HRESULT hr = QEventDispatcherWinRT::runOnXamlThread([d, socket, this]() {
            d->worker->setTcpSocket(socket);
            d->worker->startReading();
            return S_OK;
        });
        if (FAILED(hr))
            return false;
    } else {
        d->socketState = socketState;
    }

    return true;
}

qintptr QNativeSocketEngine::socketDescriptor() const
{
    Q_D(const QNativeSocketEngine);
    return d->socketDescriptor;
}

bool QNativeSocketEngine::isValid() const
{
    Q_D(const QNativeSocketEngine);
    return d->socketDescriptor != -1;
}

bool QNativeSocketEngine::connectToHost(const QHostAddress &address, quint16 port)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << address << port;
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::connectToHost(), false);
    Q_CHECK_STATES3(QNativeSocketEngine::connectToHost(), QAbstractSocket::BoundState,
        QAbstractSocket::UnconnectedState, QAbstractSocket::ConnectingState, false);
    const QString addressString = address.toString();
    return connectToHostByName(addressString, port);
}

bool QNativeSocketEngine::connectToHostByName(const QString &name, quint16 port)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << name << port;
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::connectToHostByName(), false);
    Q_CHECK_STATES3(QNativeSocketEngine::connectToHostByName(), QAbstractSocket::BoundState,
        QAbstractSocket::UnconnectedState, QAbstractSocket::ConnectingState, false);
    HRESULT hr;

#if _MSC_VER >= 1900
    ComPtr<IThreadNetworkContext> networkContext;
    if (!qEnvironmentVariableIsEmpty("QT_WINRT_USE_THREAD_NETWORK_CONTEXT")) {
        qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << "Creating network context";
        hr = qt_winrt_try_create_thread_network_context(name, networkContext);
        if (FAILED(hr)) {
            setError(QAbstractSocket::ConnectionRefusedError, QLatin1String("Could not create thread network context."));
            d->socketState = QAbstractSocket::ConnectedState;
            return true;
        }
    }
#endif // _MSC_VER >= 1900

    HStringReference hostNameRef(reinterpret_cast<LPCWSTR>(name.utf16()));
    ComPtr<IHostNameFactory> hostNameFactory;
    hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_HostName).Get(),
                                      &hostNameFactory);
    Q_ASSERT_SUCCEEDED(hr);
    ComPtr<IHostName> remoteHost;
    hr = hostNameFactory->CreateHostName(hostNameRef.Get(), &remoteHost);
    RETURN_FALSE_IF_FAILED("QNativeSocketEngine::connectToHostByName: Could not create hostname.");

    const QString portString = QString::number(port);
    HStringReference portReference(reinterpret_cast<LPCWSTR>(portString.utf16()));
    if (d->socketType == QAbstractSocket::TcpSocket)
        hr = d->tcpSocket()->ConnectAsync(remoteHost.Get(), portReference.Get(), &d->worker->connectOp);
    else if (d->socketType == QAbstractSocket::UdpSocket)
        hr = d->udpSocket()->ConnectAsync(remoteHost.Get(), portReference.Get(), &d->worker->connectOp);
    if (hr == E_ACCESSDENIED) {
        qErrnoWarning(hr, "QNativeSocketEngine::connectToHostByName: Unable to connect to host (%s:%hu/%s). "
                          "Please check your manifest capabilities.",
                      qPrintable(name), port, socketDescription(this).constData());
        return false;
    }
    Q_ASSERT_SUCCEEDED(hr);

#if _MSC_VER >= 1900
    if (networkContext != nullptr) {
        qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << "Closing network context";
        ComPtr<IClosable> networkContextCloser;
        hr = networkContext.As(&networkContextCloser);
        Q_ASSERT_SUCCEEDED(hr);
        hr = networkContextCloser->Close();
        Q_ASSERT_SUCCEEDED(hr);
    }
#endif // _MSC_VER >= 1900

    d->socketState = QAbstractSocket::ConnectingState;
    QEventDispatcherWinRT::runOnXamlThread([d, &hr]() {
        hr = d->worker->connectOp->put_Completed(Callback<IAsyncActionCompletedHandler>(
                                         d->worker, &SocketEngineWorker::onConnectOpFinished).Get());
        RETURN_OK_IF_FAILED("connectToHostByName: Could not register \"connectOp\" callback");
        return S_OK;
    });
    if (FAILED(hr))
        return false;

    return d->socketState == QAbstractSocket::ConnectedState;
}

bool QNativeSocketEngine::bind(const QHostAddress &address, quint16 port)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << address << port;
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::bind(), false);
    Q_CHECK_STATE(QNativeSocketEngine::bind(), QAbstractSocket::UnconnectedState, false);

    HRESULT hr;
    // runOnXamlThread may only return S_OK (will assert otherwise) so no need to check its result.
    // hr is set inside the lambda though. If an error occurred hr will point that out.
    bool specificErrorSet = false;
    QEventDispatcherWinRT::runOnXamlThread([address, d, &hr, port, &specificErrorSet, this]() {
        ComPtr<IHostName> hostAddress;

        if (address != QHostAddress::Any && address != QHostAddress::AnyIPv4 && address != QHostAddress::AnyIPv6) {
            ComPtr<IHostNameFactory> hostNameFactory;
            hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_HostName).Get(),
                                      &hostNameFactory);
            RETURN_OK_IF_FAILED("QNativeSocketEngine::bind: Could not obtain hostname factory");
            const QString addressString = address.toString();
            HStringReference addressRef(reinterpret_cast<LPCWSTR>(addressString.utf16()));
            hr = hostNameFactory->CreateHostName(addressRef.Get(), &hostAddress);
            RETURN_OK_IF_FAILED("QNativeSocketEngine::bind: Could not create hostname.");
        }

        QString portQString = port ? QString::number(port) : QString();
        HStringReference portString(reinterpret_cast<LPCWSTR>(portQString.utf16()));

        ComPtr<IAsyncAction> op;
        if (d->socketType == QAbstractSocket::TcpSocket) {
            if (!d->tcpListener) {
                hr = RoActivateInstance(HString::MakeReference(RuntimeClass_Windows_Networking_Sockets_StreamSocketListener).Get(),
                                        &d->tcpListener);
                RETURN_OK_IF_FAILED("QNativeSocketEngine::bind: Could not create tcp listener");
            }

            hr = d->tcpListener->add_ConnectionReceived(
                        Callback<ClientConnectedHandler>(d, &QNativeSocketEnginePrivate::handleClientConnection).Get(),
                        &d->connectionToken);
            RETURN_OK_IF_FAILED("QNativeSocketEngine::bind: Could not register client connection callback");
            hr = d->tcpListener->BindEndpointAsync(hostAddress.Get(), portString.Get(), &op);
        } else if (d->socketType == QAbstractSocket::UdpSocket) {
            hr = d->udpSocket()->BindEndpointAsync(hostAddress.Get(), portString.Get(), &op);
        }
        if (hr == E_ACCESSDENIED) {
            qErrnoWarning(hr, "Unable to bind socket (%s:%hu/%s). Please check your manifest capabilities.",
                          qPrintable(address.toString()), port, socketDescription(this).constData());
            d->setError(QAbstractSocket::SocketAccessError,
                     WinRTSocketEngine::AccessErrorString);
            d->socketState = QAbstractSocket::UnconnectedState;
            specificErrorSet = true;
            return S_OK;
        }
        RETURN_OK_IF_FAILED("QNativeSocketEngine::bind: Unable to bind socket");

        hr = QWinRTFunctions::await(op);
        if (hr == 0x80072741) { // The requested address is not valid in its context
            d->setError(QAbstractSocket::SocketAddressNotAvailableError,
                     WinRTSocketEngine::AddressNotAvailableErrorString);
            d->socketState = QAbstractSocket::UnconnectedState;
            specificErrorSet = true;
            return S_OK;
        // Only one usage of each socket address (protocol/network address/port) is normally permitted
        } else if (hr == 0x80072740) {
            d->setError(QAbstractSocket::AddressInUseError,
                WinRTSocketEngine::AddressInuseErrorString);
            d->socketState = QAbstractSocket::UnconnectedState;
            specificErrorSet = true;
            return S_OK;
        }
        RETURN_OK_IF_FAILED("QNativeSocketEngine::bind: Could not wait for bind to finish");
        return S_OK;
    });
    if (FAILED(hr)) {
        if (!specificErrorSet) {
            d->setError(QAbstractSocket::UnknownSocketError,
                     WinRTSocketEngine::UnknownSocketErrorString);
            d->socketState = QAbstractSocket::UnconnectedState;
        }
        return false;
    }

    d->socketState = QAbstractSocket::BoundState;
    return d->fetchConnectionParameters();
}

bool QNativeSocketEngine::listen()
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO;
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::listen(), false);
    Q_CHECK_STATE(QNativeSocketEngine::listen(), QAbstractSocket::BoundState, false);
#if QT_CONFIG(sctp)
    Q_CHECK_TYPES(QNativeSocketEngine::listen(), QAbstractSocket::TcpSocket,
        QAbstractSocket::SctpSocket, false);
#else
    Q_CHECK_TYPE(QNativeSocketEngine::listen(), QAbstractSocket::TcpSocket, false);
#endif

    if (d->tcpListener && d->socketDescriptor != -1) {
        d->socketState = QAbstractSocket::ListeningState;
        return true;
    }
    return false;
}

int QNativeSocketEngine::accept()
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO;
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::accept(), -1);
    Q_CHECK_STATE(QNativeSocketEngine::accept(), QAbstractSocket::ListeningState, -1);
#if QT_CONFIG(sctp)
    Q_CHECK_TYPES(QNativeSocketEngine::accept(), QAbstractSocket::TcpSocket,
        QAbstractSocket::SctpSocket, -1);
#else
    Q_CHECK_TYPE(QNativeSocketEngine::accept(), QAbstractSocket::TcpSocket, -1);
#endif

    if (d->socketDescriptor == -1 || d->pendingConnections.isEmpty()) {
        d->setError(QAbstractSocket::TemporaryError, WinRTSocketEngine::TemporaryErrorString);
        return -1;
    }

    if (d->socketType == QAbstractSocket::TcpSocket) {
        IStreamSocket *socket = d->pendingConnections.takeFirst();

        SocketHandler *handler = gSocketHandler();
        handler->pendingTcpSockets.insert(++handler->socketCount, socket);
        return handler->socketCount;
    }

    return -1;
}

void QNativeSocketEngine::close()
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO;
    Q_D(QNativeSocketEngine);

    if (d->closingDown)
        return;

    if (d->pendingReadNotification) {
        // We use QPointer here to see if this QNativeSocketEngine was deleted as a result of
        // finishing and cleaning up a network request when calling "processReadReady".
        QPointer<QNativeSocketEngine> alive(this);
        processReadReady();
        if (alive.isNull())
            return;
    }

    d->closingDown = true;

    d->notifyOnRead = false;
    d->notifyOnWrite = false;
    d->notifyOnException = false;
    d->emitReadReady = false;

    HRESULT hr;
    if (d->socketType == QAbstractSocket::TcpSocket) {
        hr = QEventDispatcherWinRT::runOnXamlThread([d]() {
            HRESULT hr;
            // To close the connection properly (not with a hard reset) all pending read operation have to
            // be finished or cancelled. The API isn't available on Windows 8.1 though.
            ComPtr<IStreamSocket3> socket3;
            hr = d->tcpSocket()->QueryInterface(IID_PPV_ARGS(&socket3));
            Q_ASSERT_SUCCEEDED(hr);

            ComPtr<IAsyncAction> action;
            hr = socket3->CancelIOAsync(&action);
            Q_ASSERT_SUCCEEDED(hr);
            hr = QWinRTFunctions::await(action, QWinRTFunctions::YieldThread, 5000);
            // If there is no pending IO (no read established before) the function will fail with
            // "function was called at an unexpected time" which is fine.
            // Timeout is fine as well. The result will be the socket being hard reset instead of
            // being closed gracefully
            if (hr != E_ILLEGAL_METHOD_CALL && hr != ERROR_TIMEOUT)
                Q_ASSERT_SUCCEEDED(hr);
            return S_OK;
        });
        Q_ASSERT_SUCCEEDED(hr);
    }

    if (d->socketDescriptor != -1) {
        ComPtr<IClosable> socket;
        if (d->socketType == QAbstractSocket::TcpSocket) {
            hr = d->tcpSocket()->QueryInterface(IID_PPV_ARGS(&socket));
            Q_ASSERT_SUCCEEDED(hr);
            hr = d->tcpSocket()->Release();
            Q_ASSERT_SUCCEEDED(hr);
        } else if (d->socketType == QAbstractSocket::UdpSocket) {
            hr = d->udpSocket()->QueryInterface(IID_PPV_ARGS(&socket));
            Q_ASSERT_SUCCEEDED(hr);
            hr = d->udpSocket()->Release();
            Q_ASSERT_SUCCEEDED(hr);
        }

        if (socket) {
            hr = socket->Close();
            Q_ASSERT_SUCCEEDED(hr);
        }
        d->socketDescriptor = -1;
    }
    d->socketState = QAbstractSocket::UnconnectedState;
    d->hasSetSocketError = false;
    d->localPort = 0;
    d->localAddress.clear();
    d->peerPort = 0;
    d->peerAddress.clear();
    d->inboundStreamCount = d->outboundStreamCount = 0;
}

bool QNativeSocketEngine::joinMulticastGroup(const QHostAddress &groupAddress, const QNetworkInterface &iface)
{
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::joinMulticastGroup(), false);
    Q_CHECK_STATE(QNativeSocketEngine::joinMulticastGroup(), QAbstractSocket::BoundState, false);
    Q_CHECK_TYPE(QNativeSocketEngine::joinMulticastGroup(), QAbstractSocket::UdpSocket, false);
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << groupAddress << iface;
    Q_UNIMPLEMENTED();
    return false;
}

bool QNativeSocketEngine::leaveMulticastGroup(const QHostAddress &groupAddress, const QNetworkInterface &iface)
{
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::leaveMulticastGroup(), false);
    Q_CHECK_STATE(QNativeSocketEngine::leaveMulticastGroup(), QAbstractSocket::BoundState, false);
    Q_CHECK_TYPE(QNativeSocketEngine::leaveMulticastGroup(), QAbstractSocket::UdpSocket, false);
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << groupAddress << iface;
    Q_UNIMPLEMENTED();
    return false;
}

QNetworkInterface QNativeSocketEngine::multicastInterface() const
{
    Q_D(const QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::multicastInterface(), QNetworkInterface());
    Q_CHECK_TYPE(QNativeSocketEngine::multicastInterface(), QAbstractSocket::UdpSocket, QNetworkInterface());
    Q_UNIMPLEMENTED();
    return QNetworkInterface();
}

bool QNativeSocketEngine::setMulticastInterface(const QNetworkInterface &iface)
{
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::setMulticastInterface(), false);
    Q_CHECK_TYPE(QNativeSocketEngine::setMulticastInterface(), QAbstractSocket::UdpSocket, false);
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << iface;
    Q_UNIMPLEMENTED();
    return false;
}

qint64 QNativeSocketEngine::bytesAvailable() const
{
    Q_D(const QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::bytesAvailable(), -1);
    Q_CHECK_NOT_STATE(QNativeSocketEngine::bytesAvailable(), QAbstractSocket::UnconnectedState, -1);
    if (d->socketType != QAbstractSocket::TcpSocket)
        return -1;

    QMutexLocker locker(&d->worker->mutex);
    const qint64 bytesAvailable = d->worker->pendingData.length();

    qCDebug(lcNetworkSocketVerbose) << this << Q_FUNC_INFO << bytesAvailable;
    return bytesAvailable;
}

qint64 QNativeSocketEngine::read(char *data, qint64 maxlen)
{
    qCDebug(lcNetworkSocketVerbose) << this << Q_FUNC_INFO << maxlen;
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::read(), -1);
    Q_CHECK_STATES(QNativeSocketEngine::read(), QAbstractSocket::ConnectedState, QAbstractSocket::BoundState, -1);
    if (d->socketType != QAbstractSocket::TcpSocket)
        return -1;

    // There will be a read notification when the socket was closed by the remote host. If that
    // happens and there isn't anything left in the buffer, we have to return -1 in order to signal
    // the closing of the socket.
    QMutexLocker mutexLocker(&d->worker->mutex);
    if (d->worker->pendingData.isEmpty() && d->socketState != QAbstractSocket::ConnectedState) {
        close();
        return -1;
    }

    QByteArray readData;
    const int copyLength = qMin(maxlen, qint64(d->worker->pendingData.length()));
    if (maxlen >= d->worker->pendingData.length()) {
        qCDebug(lcNetworkSocketVerbose) << this << Q_FUNC_INFO << "Reading full buffer";
        readData = d->worker->pendingData;
        d->worker->pendingData.clear();
        d->emitReadReady = true;
    } else {
        qCDebug(lcNetworkSocketVerbose) << this << Q_FUNC_INFO << "Reading part of the buffer ("
            << copyLength << "of" << d->worker->pendingData.length() << "bytes";
        readData = d->worker->pendingData.left(maxlen);
        d->worker->pendingData.remove(0, maxlen);
        if (d->notifyOnRead) {
            d->pendingReadNotification = true;
            emit readReady();
        }
    }
    mutexLocker.unlock();

    memcpy(data, readData, copyLength);
    qCDebug(lcNetworkSocketVerbose) << this << Q_FUNC_INFO << "Read" << copyLength << "bytes";
    return copyLength;
}

qint64 QNativeSocketEngine::write(const char *data, qint64 len)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << data << len;
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::write(), -1);
    Q_CHECK_STATE(QNativeSocketEngine::write(), QAbstractSocket::ConnectedState, -1);

    HRESULT hr = E_FAIL;
    ComPtr<IOutputStream> stream;
    if (d->socketType == QAbstractSocket::TcpSocket)
        hr = d->tcpSocket()->get_OutputStream(&stream);
    else if (d->socketType == QAbstractSocket::UdpSocket)
        hr = d->udpSocket()->get_OutputStream(&stream);
    Q_ASSERT_SUCCEEDED(hr);

    qint64 bytesWritten = writeIOStream(stream, data, len);
    if (bytesWritten < 0)
        d->setError(QAbstractSocket::SocketAccessError, WinRTSocketEngine::AccessErrorString);
    else if (bytesWritten > 0 && d->notifyOnWrite)
        emit writeReady();

    return bytesWritten;
}

qint64 QNativeSocketEngine::readDatagram(char *data, qint64 maxlen, QIpPacketHeader *header,
                                         PacketHeaderOptions)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << maxlen;
#ifndef QT_NO_UDPSOCKET
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::readDatagram(), -1);
    Q_CHECK_STATES(QNativeSocketEngine::readDatagram(), QAbstractSocket::BoundState,
        QAbstractSocket::ConnectedState, -1);

    QMutexLocker locker(&d->worker->mutex);
    if (d->socketType != QAbstractSocket::UdpSocket || d->worker->pendingDatagrams.isEmpty()) {
        if (header)
            header->clear();
        return -1;
    }

    WinRtDatagram datagram = d->worker->pendingDatagrams.takeFirst();
    if (header)
        *header = datagram.header;

    QByteArray readOrigin;
    if (maxlen < datagram.data.length())
        readOrigin = datagram.data.left(maxlen);
    else
        readOrigin = datagram.data;
    if (d->worker->pendingDatagrams.isEmpty()) {
        qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << "That's all folks";
        d->worker->emitDataReceived = true;
        d->emitReadReady = true;
    }

    locker.unlock();
    memcpy(data, readOrigin, qMin(maxlen, qint64(datagram.data.length())));
    return readOrigin.length();
#else
    Q_UNUSED(data)
    Q_UNUSED(maxlen)
    Q_UNUSED(header)
    return -1;
#endif // QT_NO_UDPSOCKET
}

qint64 QNativeSocketEngine::writeDatagram(const char *data, qint64 len, const QIpPacketHeader &header)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << data << len;
#ifndef QT_NO_UDPSOCKET
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::writeDatagram(), -1);
    Q_CHECK_STATES(QNativeSocketEngine::writeDatagram(), QAbstractSocket::BoundState,
        QAbstractSocket::ConnectedState, -1);

    if (d->socketType != QAbstractSocket::UdpSocket)
        return -1;

    ComPtr<IHostName> remoteHost;
    ComPtr<IHostNameFactory> hostNameFactory;

    HRESULT hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Networking_HostName).Get(),
                                    &hostNameFactory);
    Q_ASSERT_SUCCEEDED(hr);
    const QString addressString = header.destinationAddress.toString();
    HStringReference hostNameRef(reinterpret_cast<LPCWSTR>(addressString.utf16()));
    hr = hostNameFactory->CreateHostName(hostNameRef.Get(), &remoteHost);
    RETURN_IF_FAILED("QNativeSocketEngine::writeDatagram: Could not create hostname.", return -1);

    ComPtr<IAsyncOperation<IOutputStream *>> streamOperation;
    ComPtr<IOutputStream> stream;
    const QString portString = QString::number(header.destinationPort);
    HStringReference portRef(reinterpret_cast<LPCWSTR>(portString.utf16()));
    hr = d->udpSocket()->GetOutputStreamAsync(remoteHost.Get(), portRef.Get(), &streamOperation);
    Q_ASSERT_SUCCEEDED(hr);

    hr = QWinRTFunctions::await(streamOperation, stream.GetAddressOf());
    Q_ASSERT_SUCCEEDED(hr);

    return writeIOStream(stream, data, len);
#else
    Q_UNUSED(data)
    Q_UNUSED(len)
    Q_UNUSED(header)
    return -1;
#endif // QT_NO_UDPSOCKET
}

bool QNativeSocketEngine::hasPendingDatagrams() const
{
    Q_D(const QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::hasPendingDatagrams(), false);
    Q_CHECK_NOT_STATE(QNativeSocketEngine::hasPendingDatagrams(), QAbstractSocket::UnconnectedState, false);
    Q_CHECK_TYPE(QNativeSocketEngine::hasPendingDatagrams(), QAbstractSocket::UdpSocket, false);

    QMutexLocker locker(&d->worker->mutex);
    return d->worker->pendingDatagrams.length() > 0;
}

qint64 QNativeSocketEngine::pendingDatagramSize() const
{
    Q_D(const QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::pendingDatagramSize(), -1);
    Q_CHECK_TYPE(QNativeSocketEngine::pendingDatagramSize(), QAbstractSocket::UdpSocket, -1);

    QMutexLocker locker(&d->worker->mutex);
    if (d->worker->pendingDatagrams.isEmpty())
        return -1;

    return d->worker->pendingDatagrams.at(0).data.length();
}

qint64 QNativeSocketEngine::bytesToWrite() const
{
    return 0;
}

qint64 QNativeSocketEngine::receiveBufferSize() const
{
    Q_D(const QNativeSocketEngine);
    return d->option(QAbstractSocketEngine::ReceiveBufferSocketOption);
}

void QNativeSocketEngine::setReceiveBufferSize(qint64 bufferSize)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << bufferSize;
    Q_D(QNativeSocketEngine);
    d->setOption(QAbstractSocketEngine::ReceiveBufferSocketOption, bufferSize);
}

qint64 QNativeSocketEngine::sendBufferSize() const
{
    Q_D(const QNativeSocketEngine);
    return d->option(QAbstractSocketEngine::SendBufferSocketOption);
}

void QNativeSocketEngine::setSendBufferSize(qint64 bufferSize)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << bufferSize;
    Q_D(QNativeSocketEngine);
    d->setOption(QAbstractSocketEngine::SendBufferSocketOption, bufferSize);
}

int QNativeSocketEngine::option(QAbstractSocketEngine::SocketOption option) const
{
    Q_D(const QNativeSocketEngine);
    return d->option(option);
}

bool QNativeSocketEngine::setOption(QAbstractSocketEngine::SocketOption option, int value)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << option << value;
    Q_D(QNativeSocketEngine);
    return d->setOption(option, value);
}

bool QNativeSocketEngine::waitForRead(int msecs, bool *timedOut)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << msecs;
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::waitForRead(), false);
    Q_CHECK_NOT_STATE(QNativeSocketEngine::waitForRead(),
                      QAbstractSocket::UnconnectedState, false);

    if (timedOut)
        *timedOut = false;

    QElapsedTimer timer;
    timer.start();
    while (msecs > timer.elapsed()) {
        // Servers with active connections are ready for reading
        if (!d->currentConnections.isEmpty())
            return true;

        // If we are a client, we are ready to read if our buffer has data
        QMutexLocker locker(&d->worker->mutex);
        if (!d->worker->pendingData.isEmpty())
            return true;

        // Nothing to do, wait for more events
        d->eventLoop.processEvents();
    }

    d->setError(QAbstractSocket::SocketTimeoutError,
                WinRTSocketEngine::TimeOutErrorString);

    if (timedOut)
        *timedOut = true;
    return false;
}

bool QNativeSocketEngine::waitForWrite(int msecs, bool *timedOut)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << msecs;
    Q_UNUSED(timedOut);
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::waitForWrite(), false);
    Q_CHECK_NOT_STATE(QNativeSocketEngine::waitForWrite(),
        QAbstractSocket::UnconnectedState, false);

    if (d->socketState == QAbstractSocket::ConnectingState) {
        HRESULT hr = QWinRTFunctions::await(d->worker->connectOp, QWinRTFunctions::ProcessMainThreadEvents);
        if (SUCCEEDED(hr)) {
            handleConnectOpFinished(true, QAbstractSocket::UnknownSocketError, WinRTSocketEngine::UnknownSocketErrorString);
            return true;
        }
    }
    return false;
}

bool QNativeSocketEngine::waitForReadOrWrite(bool *readyToRead, bool *readyToWrite, bool checkRead, bool checkWrite, int msecs, bool *timedOut)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << checkRead << checkWrite << msecs;
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::waitForReadOrWrite(), false);
    Q_CHECK_NOT_STATE(QNativeSocketEngine::waitForReadOrWrite(),
        QAbstractSocket::UnconnectedState, false);

    Q_UNUSED(readyToRead);
    Q_UNUSED(readyToWrite);
    Q_UNUSED(timedOut);
    return false;
}

bool QNativeSocketEngine::isReadNotificationEnabled() const
{
    Q_D(const QNativeSocketEngine);
    return d->notifyOnRead;
}

void QNativeSocketEngine::setReadNotificationEnabled(bool enable)
{
    qCDebug(lcNetworkSocketVerbose) << this << Q_FUNC_INFO << enable;
    Q_D(QNativeSocketEngine);
    d->notifyOnRead = enable;
}

bool QNativeSocketEngine::isWriteNotificationEnabled() const
{
    Q_D(const QNativeSocketEngine);
    return d->notifyOnWrite;
}

void QNativeSocketEngine::setWriteNotificationEnabled(bool enable)
{
    qCDebug(lcNetworkSocketVerbose) << this << Q_FUNC_INFO << enable;
    Q_D(QNativeSocketEngine);
    d->notifyOnWrite = enable;
    if (enable && d->socketState == QAbstractSocket::ConnectedState) {
        if (bytesToWrite())
            return; // will be emitted as a result of bytes written
        writeNotification();
    }
}

bool QNativeSocketEngine::isExceptionNotificationEnabled() const
{
    Q_D(const QNativeSocketEngine);
    return d->notifyOnException;
}

void QNativeSocketEngine::setExceptionNotificationEnabled(bool enable)
{
    qCDebug(lcNetworkSocketVerbose) << this << Q_FUNC_INFO << enable;
    Q_D(QNativeSocketEngine);
    d->notifyOnException = enable;
}

void QNativeSocketEngine::establishRead()
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO;
    Q_D(QNativeSocketEngine);

    HRESULT hr;
    hr = QEventDispatcherWinRT::runOnXamlThread([d]() {
        d->worker->setTcpSocket(d->tcpSocket());
        d->worker->startReading();
        return S_OK;
    });
    Q_ASSERT_SUCCEEDED(hr);
}

void QNativeSocketEngine::handleConnectOpFinished(bool success, QAbstractSocket::SocketError error, WinRTSocketEngine::ErrorString errorString)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << success << error << errorString;
    Q_D(QNativeSocketEngine);
    disconnect(d->worker, &SocketEngineWorker::connectOpFinished,
            this, &QNativeSocketEngine::handleConnectOpFinished);
    if (!success) {
        d->setError(error, errorString);
        d->socketState = QAbstractSocket::UnconnectedState;
        close();
        return;
    }

    d->socketState = QAbstractSocket::ConnectedState;
    d->fetchConnectionParameters();
    emit connectionReady();

    if (d->socketType != QAbstractSocket::TcpSocket)
        return;

#ifndef QT_NO_SSL
    // Delay the reader so that the SSL socket can upgrade
    if (d->sslSocket)
        QObject::connect(qobject_cast<QSslSocket *>(d->sslSocket), &QSslSocket::encrypted, this, &QNativeSocketEngine::establishRead);
    else
#endif
        establishRead();
}

void QNativeSocketEngine::handleNewData()
{
    qCDebug(lcNetworkSocketVerbose) << this << Q_FUNC_INFO;
    Q_D(QNativeSocketEngine);

    if (d->notifyOnRead && d->emitReadReady) {
        if (d->socketType == QAbstractSocket::UdpSocket && !d->worker->emitDataReceived)
            return;
        qCDebug(lcNetworkSocketVerbose) << this << Q_FUNC_INFO << "Emitting readReady";
        d->pendingReadNotification = true;
        emit readReady();
        d->worker->emitDataReceived = false;
        d->emitReadReady = false;
    }
}

void QNativeSocketEngine::handleTcpError(QAbstractSocket::SocketError error)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << error;
    Q_D(QNativeSocketEngine);
    WinRTSocketEngine::ErrorString errorString;
    switch (error) {
    case QAbstractSocket::RemoteHostClosedError:
        errorString = WinRTSocketEngine::RemoteHostClosedErrorString;
        break;
    default:
        errorString = WinRTSocketEngine::UnknownSocketErrorString;
    }

    d->setError(error, errorString);
    close();
}

void QNativeSocketEngine::processReadReady()
{
    Q_D(QNativeSocketEngine);
    if (d->closingDown)
        return;

    d->pendingReadNotification = false;
    readNotification();
}

bool QNativeSocketEnginePrivate::createNewSocket(QAbstractSocket::SocketType socketType, QAbstractSocket::NetworkLayerProtocol &socketProtocol)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << socketType << socketProtocol;
    Q_UNUSED(socketProtocol);
    HRESULT hr;

    switch (socketType) {
    case QAbstractSocket::TcpSocket: {
        ComPtr<IStreamSocket> socket;
        hr = RoActivateInstance(HString::MakeReference(RuntimeClass_Windows_Networking_Sockets_StreamSocket).Get(), &socket);
        RETURN_FALSE_IF_FAILED("createNewSocket: Could not create socket instance");
        socketDescriptor = qintptr(socket.Detach());
        break;
    }
    case QAbstractSocket::UdpSocket: {
        ComPtr<IDatagramSocket> socket;
        hr = RoActivateInstance(HString::MakeReference(RuntimeClass_Windows_Networking_Sockets_DatagramSocket).Get(), &socket);
        RETURN_FALSE_IF_FAILED("createNewSocket: Could not create socket instance");
        socketDescriptor = qintptr(socket.Detach());
        QEventDispatcherWinRT::runOnXamlThread([&hr, this]() {
            hr = udpSocket()->add_MessageReceived(Callback<DatagramReceivedHandler>(worker, &SocketEngineWorker::OnNewDatagramReceived).Get(), &connectionToken);
            if (FAILED(hr)) {
                qErrnoWarning(hr, "createNewSocket: Could not add \"message received\" callback");
                return hr;
            }
            return S_OK;
        });
        if (FAILED(hr))
            return false;
        break;
    }
    default:
        qWarning("Invalid socket type");
        return false;
    }

    this->socketType = socketType;

    // Make the socket nonblocking.
    if (!setOption(QAbstractSocketEngine::NonBlockingSocketOption, 1)) {
        setError(QAbstractSocket::UnsupportedSocketOperationError, WinRTSocketEngine::NonBlockingInitFailedErrorString);
        q_func()->close();
        return false;
    }

    return true;
}

QNativeSocketEnginePrivate::QNativeSocketEnginePrivate()
    : QAbstractSocketEnginePrivate()
    , notifyOnRead(true)
    , notifyOnWrite(true)
    , notifyOnException(false)
    , closingDown(false)
    , socketDescriptor(-1)
    , worker(new SocketEngineWorker(this))
    , sslSocket(nullptr)
    , connectionToken( { -1 } )
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO;
}

QNativeSocketEnginePrivate::~QNativeSocketEnginePrivate()
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO;
    if (socketDescriptor == -1 || connectionToken.value == -1)
        return;

    HRESULT hr;
    if (socketType == QAbstractSocket::UdpSocket)
        hr = udpSocket()->remove_MessageReceived(connectionToken);
    else if (socketType == QAbstractSocket::TcpSocket)
        hr = tcpListener->remove_ConnectionReceived(connectionToken);
    Q_ASSERT_SUCCEEDED(hr);

    worker->deleteLater();
}

void QNativeSocketEnginePrivate::setError(QAbstractSocket::SocketError error, WinRTSocketEngine::ErrorString errorString) const
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << error << errorString;
    if (hasSetSocketError) {
        // Only set socket errors once for one engine; expect the
        // socket to recreate its engine after an error. Note: There's
        // one exception: SocketError(11) bypasses this as it's purely
        // a temporary internal error condition.
        // Another exception is the way the waitFor*() functions set
        // an error when a timeout occurs. After the call to setError()
        // they reset the hasSetSocketError to false
        return;
    }
    if (error != QAbstractSocket::SocketError(11))
        hasSetSocketError = true;

    socketError = error;

    switch (errorString) {
    case WinRTSocketEngine::NonBlockingInitFailedErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unable to initialize non-blocking socket");
        break;
    case WinRTSocketEngine::BroadcastingInitFailedErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unable to initialize broadcast socket");
        break;
    // should not happen anymore
    case WinRTSocketEngine::NoIpV6ErrorString:
        socketErrorString = QNativeSocketEngine::tr("Attempt to use IPv6 socket on a platform with no IPv6 support");
        break;
    case WinRTSocketEngine::RemoteHostClosedErrorString:
        socketErrorString = QNativeSocketEngine::tr("The remote host closed the connection");
        break;
    case WinRTSocketEngine::TimeOutErrorString:
        socketErrorString = QNativeSocketEngine::tr("Network operation timed out");
        break;
    case WinRTSocketEngine::ResourceErrorString:
        socketErrorString = QNativeSocketEngine::tr("Out of resources");
        break;
    case WinRTSocketEngine::OperationUnsupportedErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unsupported socket operation");
        break;
    case WinRTSocketEngine::ProtocolUnsupportedErrorString:
        socketErrorString = QNativeSocketEngine::tr("Protocol type not supported");
        break;
    case WinRTSocketEngine::InvalidSocketErrorString:
        socketErrorString = QNativeSocketEngine::tr("Invalid socket descriptor");
        break;
    case WinRTSocketEngine::HostUnreachableErrorString:
        socketErrorString = QNativeSocketEngine::tr("Host unreachable");
        break;
    case WinRTSocketEngine::NetworkUnreachableErrorString:
        socketErrorString = QNativeSocketEngine::tr("Network unreachable");
        break;
    case WinRTSocketEngine::AccessErrorString:
        socketErrorString = QNativeSocketEngine::tr("Permission denied");
        break;
    case WinRTSocketEngine::ConnectionTimeOutErrorString:
        socketErrorString = QNativeSocketEngine::tr("Connection timed out");
        break;
    case WinRTSocketEngine::ConnectionRefusedErrorString:
        socketErrorString = QNativeSocketEngine::tr("Connection refused");
        break;
    case WinRTSocketEngine::AddressInuseErrorString:
        socketErrorString = QNativeSocketEngine::tr("The bound address is already in use");
        break;
    case WinRTSocketEngine::AddressNotAvailableErrorString:
        socketErrorString = QNativeSocketEngine::tr("The address is not available");
        break;
    case WinRTSocketEngine::AddressProtectedErrorString:
        socketErrorString = QNativeSocketEngine::tr("The address is protected");
        break;
    case WinRTSocketEngine::DatagramTooLargeErrorString:
        socketErrorString = QNativeSocketEngine::tr("Datagram was too large to send");
        break;
    case WinRTSocketEngine::SendDatagramErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unable to send a message");
        break;
    case WinRTSocketEngine::ReceiveDatagramErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unable to receive a message");
        break;
    case WinRTSocketEngine::WriteErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unable to write");
        break;
    case WinRTSocketEngine::ReadErrorString:
        socketErrorString = QNativeSocketEngine::tr("Network error");
        break;
    case WinRTSocketEngine::PortInuseErrorString:
        socketErrorString = QNativeSocketEngine::tr("Another socket is already listening on the same port");
        break;
    case WinRTSocketEngine::NotSocketErrorString:
        socketErrorString = QNativeSocketEngine::tr("Operation on non-socket");
        break;
    case WinRTSocketEngine::InvalidProxyTypeString:
        socketErrorString = QNativeSocketEngine::tr("The proxy type is invalid for this operation");
        break;
    case WinRTSocketEngine::TemporaryErrorString:
        socketErrorString = QNativeSocketEngine::tr("Temporary error");
        break;
    case WinRTSocketEngine::UnknownSocketErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unknown error");
        break;
    }
}

int QNativeSocketEnginePrivate::option(QAbstractSocketEngine::SocketOption opt) const
{
    ComPtr<IStreamSocketControl> control;
    if (socketType == QAbstractSocket::TcpSocket) {
        if (FAILED(tcpSocket()->get_Control(&control))) {
            qWarning("QNativeSocketEnginePrivate::option: Could not obtain socket control");
            return -1;
        }
    }
    switch (opt) {
    case QAbstractSocketEngine::NonBlockingSocketOption:
    case QAbstractSocketEngine::BroadcastSocketOption:
    case QAbstractSocketEngine::ReceiveOutOfBandData:
        return 1;
    case QAbstractSocketEngine::SendBufferSocketOption:
        if (socketType == QAbstractSocket::UdpSocket)
            return -1;

        UINT32 bufferSize;
        if (FAILED(control->get_OutboundBufferSizeInBytes(&bufferSize))) {
            qWarning("Could not obtain OutboundBufferSizeInBytes information vom socket control");
            return -1;
        }
        return bufferSize;
    case QAbstractSocketEngine::LowDelayOption:
        if (socketType == QAbstractSocket::UdpSocket)
            return -1;

        boolean noDelay;
        if (FAILED(control->get_NoDelay(&noDelay))) {
            qWarning("Could not obtain NoDelay information from socket control");
            return -1;
        }
        return noDelay;
    case QAbstractSocketEngine::KeepAliveOption:
        if (socketType == QAbstractSocket::UdpSocket)
            return -1;

        boolean keepAlive;
        if (FAILED(control->get_KeepAlive(&keepAlive))) {
            qWarning("Could not obtain KeepAlive information from socket control");
            return -1;
        }
        return keepAlive;
    case QAbstractSocketEngine::ReceiveBufferSocketOption:
    case QAbstractSocketEngine::AddressReusable:
    case QAbstractSocketEngine::BindExclusively:
    case QAbstractSocketEngine::MulticastTtlOption:
    case QAbstractSocketEngine::MulticastLoopbackOption:
    case QAbstractSocketEngine::TypeOfServiceOption:
    case QAbstractSocketEngine::MaxStreamsSocketOption:
    case QAbstractSocketEngine::PathMtuInformation:
    default:
        return -1;
    }
    return -1;
}

bool QNativeSocketEnginePrivate::setOption(QAbstractSocketEngine::SocketOption opt, int v)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO << opt << v;
    ComPtr<IStreamSocketControl> control;
    if (socketType == QAbstractSocket::TcpSocket) {
        if (FAILED(tcpSocket()->get_Control(&control))) {
            qWarning("QNativeSocketEnginePrivate::setOption: Could not obtain socket control");
            return false;
        }
    }
    switch (opt) {
    case QAbstractSocketEngine::NonBlockingSocketOption:
    case QAbstractSocketEngine::BroadcastSocketOption:
    case QAbstractSocketEngine::ReceiveOutOfBandData:
        return v != 0;
    case QAbstractSocketEngine::SendBufferSocketOption:
        if (socketType == QAbstractSocket::UdpSocket)
            return false;

        if (FAILED(control->put_OutboundBufferSizeInBytes(v))) {
            qWarning("Could not set OutboundBufferSizeInBytes");
            return false;
        }
        return true;
    case QAbstractSocketEngine::LowDelayOption: {
        if (socketType == QAbstractSocket::UdpSocket)
            return false;

        boolean noDelay = v;
        if (FAILED(control->put_NoDelay(noDelay))) {
            qWarning("Could not obtain NoDelay information from socket control");
            return false;
        }
        return true;
    }
    case QAbstractSocketEngine::KeepAliveOption: {
        if (socketType == QAbstractSocket::UdpSocket
                || socketState != QAbstractSocket::UnconnectedState)
            return false;

        boolean keepAlive = v;
        if (FAILED(control->put_KeepAlive(keepAlive))) {
            qWarning("Could not set KeepAlive value");
            return false;
        }
        return true;
    }
    case QAbstractSocketEngine::ReceiveBufferSocketOption:
    case QAbstractSocketEngine::AddressReusable:
    case QAbstractSocketEngine::BindExclusively:
    case QAbstractSocketEngine::MulticastTtlOption:
    case QAbstractSocketEngine::MulticastLoopbackOption:
    case QAbstractSocketEngine::TypeOfServiceOption:
    case QAbstractSocketEngine::MaxStreamsSocketOption:
    case QAbstractSocketEngine::PathMtuInformation:
    default:
        return false;
    }
    return false;
}

bool QNativeSocketEnginePrivate::fetchConnectionParameters()
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO;
    localPort = 0;
    localAddress.clear();
    peerPort = 0;
    peerAddress.clear();
    inboundStreamCount = outboundStreamCount = 0;

    HRESULT hr;
    if (socketType == QAbstractSocket::TcpSocket) {
        ComPtr<IHostName> hostName;
        HString tmpHString;
        ComPtr<IStreamSocketInformation> info;
        hr = tcpSocket()->get_Information(&info);
        Q_ASSERT_SUCCEEDED(hr);
        hr = info->get_LocalAddress(&hostName);
        Q_ASSERT_SUCCEEDED(hr);
        if (hostName) {
            hr = hostName->get_CanonicalName(tmpHString.GetAddressOf());
            Q_ASSERT_SUCCEEDED(hr);
            localAddress.setAddress(qt_QStringFromHString(tmpHString));
            hr = info->get_LocalPort(tmpHString.GetAddressOf());
            Q_ASSERT_SUCCEEDED(hr);
            localPort = qt_QStringFromHString(tmpHString).toInt();
        }
        if (!localPort && tcpListener) {
            ComPtr<IStreamSocketListenerInformation> listenerInfo = 0;
            hr = tcpListener->get_Information(&listenerInfo);
            Q_ASSERT_SUCCEEDED(hr);
            hr = listenerInfo->get_LocalPort(tmpHString.GetAddressOf());
            Q_ASSERT_SUCCEEDED(hr);
            localPort = qt_QStringFromHString(tmpHString).toInt();
            localAddress = QHostAddress::Any;
        }
        info->get_RemoteAddress(&hostName);
        if (hostName) {
            hr = hostName->get_CanonicalName(tmpHString.GetAddressOf());
            Q_ASSERT_SUCCEEDED(hr);
            peerAddress.setAddress(qt_QStringFromHString(tmpHString));
            hr = info->get_RemotePort(tmpHString.GetAddressOf());
            Q_ASSERT_SUCCEEDED(hr);
            peerPort = qt_QStringFromHString(tmpHString).toInt();
            inboundStreamCount = outboundStreamCount = 1;
        }
    } else if (socketType == QAbstractSocket::UdpSocket) {
        ComPtr<IHostName> hostName;
        HString tmpHString;
        ComPtr<IDatagramSocketInformation> info;
        hr = udpSocket()->get_Information(&info);
        Q_ASSERT_SUCCEEDED(hr);
        hr = info->get_LocalAddress(&hostName);
        Q_ASSERT_SUCCEEDED(hr);
        if (hostName) {
            hr = hostName->get_CanonicalName(tmpHString.GetAddressOf());
            Q_ASSERT_SUCCEEDED(hr);
            localAddress.setAddress(qt_QStringFromHString(tmpHString));
            hr = info->get_LocalPort(tmpHString.GetAddressOf());
            Q_ASSERT_SUCCEEDED(hr);
            localPort = qt_QStringFromHString(tmpHString).toInt();
        }

        hr = info->get_RemoteAddress(&hostName);
        Q_ASSERT_SUCCEEDED(hr);
        if (hostName) {
            hr = hostName->get_CanonicalName(tmpHString.GetAddressOf());
            Q_ASSERT_SUCCEEDED(hr);
            peerAddress.setAddress(qt_QStringFromHString(tmpHString));
            hr = info->get_RemotePort(tmpHString.GetAddressOf());
            Q_ASSERT_SUCCEEDED(hr);
            peerPort = qt_QStringFromHString(tmpHString).toInt();
            inboundStreamCount = outboundStreamCount = 1;
        }
    }
    return true;
}

HRESULT QNativeSocketEnginePrivate::handleClientConnection(IStreamSocketListener *listener, IStreamSocketListenerConnectionReceivedEventArgs *args)
{
    qCDebug(lcNetworkSocket) << this << Q_FUNC_INFO;
    Q_Q(QNativeSocketEngine);
    Q_UNUSED(listener)
    IStreamSocket *socket;
    args->get_Socket(&socket);
    pendingConnections.append(socket);
    emit q->connectionReady();
    if (notifyOnRead)
        emit q->readReady();
    return S_OK;
}

QT_END_NAMESPACE

#include "qnativesocketengine_winrt.moc"
