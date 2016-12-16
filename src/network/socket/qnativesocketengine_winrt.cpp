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
    }

    ~SocketEngineWorker()
    {
        if (Q_UNLIKELY(initialReadOp)) {
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
    }

signals:
    void newDatagramsReceived(const QList<WinRtDatagram> &datagram);
    void newDataReceived(const QVector<QByteArray> &data);
    void socketErrorOccured(QAbstractSocket::SocketError error);

public slots:
    Q_INVOKABLE void notifyAboutNewDatagrams()
    {
        QMutexLocker locker(&mutex);
        QList<WinRtDatagram> datagrams = pendingDatagrams;
        pendingDatagrams.clear();
        emit newDatagramsReceived(datagrams);
    }

    Q_INVOKABLE void notifyAboutNewData()
    {
        QMutexLocker locker(&mutex);
        const QVector<QByteArray> newData = std::move(pendingData);
        pendingData.clear();
        emit newDataReceived(newData);
    }

public:
    void startReading()
    {
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

    HRESULT OnNewDatagramReceived(IDatagramSocket *, IDatagramSocketMessageReceivedEventArgs *args)
    {
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
        if (pendingDatagrams.isEmpty())
            QMetaObject::invokeMethod(this, "notifyAboutNewDatagrams", Qt::QueuedConnection);
        pendingDatagrams << datagram;

        return S_OK;
    }

    HRESULT onReadyRead(IAsyncBufferOperation *asyncInfo, AsyncStatus status)
    {
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
        if (pendingData.isEmpty())
            QMetaObject::invokeMethod(this, "notifyAboutNewData", Qt::QueuedConnection);
        pendingData << newData;
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
    ComPtr<IStreamSocket> tcpSocket;

    QList<WinRtDatagram> pendingDatagrams;
    QVector<QByteArray> pendingData;

    // Protects pendingData/pendingDatagrams which are accessed from native callbacks
    QMutex mutex;

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
    qRegisterMetaType<WinRtDatagram>();
#ifndef QT_NO_SSL
    Q_D(QNativeSocketEngine);
    if (parent)
        d->sslSocket = qobject_cast<QSslSocket *>(parent->parent());
#endif

    connect(this, SIGNAL(connectionReady()), SLOT(connectionNotification()), Qt::QueuedConnection);
    connect(this, SIGNAL(readReady()), SLOT(readNotification()), Qt::QueuedConnection);
    connect(this, SIGNAL(writeReady()), SLOT(writeNotification()), Qt::QueuedConnection);
    connect(d->worker, &SocketEngineWorker::newDatagramsReceived, this, &QNativeSocketEngine::handleNewDatagrams, Qt::QueuedConnection);
    connect(d->worker, &SocketEngineWorker::newDataReceived,
            this, &QNativeSocketEngine::handleNewData, Qt::QueuedConnection);
    connect(d->worker, &SocketEngineWorker::socketErrorOccured,
            this, &QNativeSocketEngine::handleTcpError, Qt::QueuedConnection);
}

QNativeSocketEngine::~QNativeSocketEngine()
{
    close();
}

bool QNativeSocketEngine::initialize(QAbstractSocket::SocketType type, QAbstractSocket::NetworkLayerProtocol protocol)
{
    Q_D(QNativeSocketEngine);
    if (isValid())
        close();

    // Create the socket
    if (!d->createNewSocket(type, protocol))
        return false;

    d->socketType = type;
    d->socketProtocol = protocol;
    return true;
}

bool QNativeSocketEngine::initialize(qintptr socketDescriptor, QAbstractSocket::SocketState socketState)
{
    Q_D(QNativeSocketEngine);

    if (isValid())
        close();

    // Currently, only TCP sockets are initialized this way.
    IStreamSocket *socket = gSocketHandler->pendingTcpSockets.take(socketDescriptor);
    d->socketDescriptor = qintptr(socket);
    d->socketType = QAbstractSocket::TcpSocket;

    if (!d->socketDescriptor || !d->fetchConnectionParameters()) {
        d->setError(QAbstractSocket::UnsupportedSocketOperationError,
            d->InvalidSocketErrorString);
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
    const QString addressString = address.toString();
    return connectToHostByName(addressString, port);
}

bool QNativeSocketEngine::connectToHostByName(const QString &name, quint16 port)
{
    Q_D(QNativeSocketEngine);
    HRESULT hr;

#if _MSC_VER >= 1900
    ComPtr<IThreadNetworkContext> networkContext;
    if (!qEnvironmentVariableIsEmpty("QT_WINRT_USE_THREAD_NETWORK_CONTEXT")) {
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
        hr = d->tcpSocket()->ConnectAsync(remoteHost.Get(), portReference.Get(), &d->connectOp);
    else if (d->socketType == QAbstractSocket::UdpSocket)
        hr = d->udpSocket()->ConnectAsync(remoteHost.Get(), portReference.Get(), &d->connectOp);
    if (hr == E_ACCESSDENIED) {
        qErrnoWarning(hr, "QNativeSocketEngine::connectToHostByName: Unable to connect to host (%s:%hu/%s). "
                          "Please check your manifest capabilities.",
                      qPrintable(name), port, socketDescription(this).constData());
        return false;
    }
    Q_ASSERT_SUCCEEDED(hr);

#if _MSC_VER >= 1900
    if (networkContext != nullptr) {
        ComPtr<IClosable> networkContextCloser;
        hr = networkContext.As(&networkContextCloser);
        Q_ASSERT_SUCCEEDED(hr);
        hr = networkContextCloser->Close();
        Q_ASSERT_SUCCEEDED(hr);
    }
#endif // _MSC_VER >= 1900

    d->socketState = QAbstractSocket::ConnectingState;
    QEventDispatcherWinRT::runOnXamlThread([d, &hr]() {
        hr = d->connectOp->put_Completed(Callback<IAsyncActionCompletedHandler>(
                                         d, &QNativeSocketEnginePrivate::handleConnectOpFinished).Get());
        RETURN_OK_IF_FAILED("connectToHostByName: Could not register \"connectOp\" callback");
        return S_OK;
    });
    if (FAILED(hr))
        return false;

    return d->socketState == QAbstractSocket::ConnectedState;
}

bool QNativeSocketEngine::bind(const QHostAddress &address, quint16 port)
{
    Q_D(QNativeSocketEngine);
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
                     QNativeSocketEnginePrivate::AccessErrorString);
            d->socketState = QAbstractSocket::UnconnectedState;
            specificErrorSet = true;
            return S_OK;
        }
        RETURN_OK_IF_FAILED("QNativeSocketEngine::bind: Unable to bind socket");

        hr = QWinRTFunctions::await(op);
        if (hr == 0x80072741) { // The requested address is not valid in its context
            d->setError(QAbstractSocket::SocketAddressNotAvailableError,
                     QNativeSocketEnginePrivate::AddressNotAvailableErrorString);
            d->socketState = QAbstractSocket::UnconnectedState;
            specificErrorSet = true;
            return S_OK;
        // Only one usage of each socket address (protocol/network address/port) is normally permitted
        } else if (hr == 0x80072740) {
            d->setError(QAbstractSocket::AddressInUseError,
                QNativeSocketEnginePrivate::AddressInuseErrorString);
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
                     QNativeSocketEnginePrivate::UnknownSocketErrorString);
            d->socketState = QAbstractSocket::UnconnectedState;
        }
        return false;
    }

    d->socketState = QAbstractSocket::BoundState;
    return d->fetchConnectionParameters();
}

bool QNativeSocketEngine::listen()
{
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::listen(), false);
    Q_CHECK_STATE(QNativeSocketEngine::listen(), QAbstractSocket::BoundState, false);
    Q_CHECK_TYPE(QNativeSocketEngine::listen(), QAbstractSocket::TcpSocket, false);

    if (d->tcpListener && d->socketDescriptor != -1) {
        d->socketState = QAbstractSocket::ListeningState;
        return true;
    }
    return false;
}

int QNativeSocketEngine::accept()
{
    Q_D(QNativeSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QNativeSocketEngine::accept(), -1);
    Q_CHECK_STATE(QNativeSocketEngine::accept(), QAbstractSocket::ListeningState, -1);
    Q_CHECK_TYPE(QNativeSocketEngine::accept(), QAbstractSocket::TcpSocket, -1);

    if (d->socketDescriptor == -1 || d->pendingConnections.isEmpty()) {
        d->setError(QAbstractSocket::TemporaryError, QNativeSocketEnginePrivate::TemporaryErrorString);
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
    Q_D(QNativeSocketEngine);

    if (d->closingDown)
        return;

    d->closingDown = true;


    d->notifyOnRead = false;
    d->notifyOnWrite = false;
    d->notifyOnException = false;

    HRESULT hr;
    if (d->connectOp) {
        ComPtr<IAsyncInfo> info;
        hr = d->connectOp.As(&info);
        Q_ASSERT_SUCCEEDED(hr);
        if (info) {
            hr = info->Cancel();
            Q_ASSERT_SUCCEEDED(hr);
            hr = info->Close();
            Q_ASSERT_SUCCEEDED(hr);
        }
    }

#if _MSC_VER >= 1900
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
#endif // _MSC_VER >= 1900

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
    Q_UNUSED(groupAddress);
    Q_UNUSED(iface);
    Q_UNIMPLEMENTED();
    return false;
}

bool QNativeSocketEngine::leaveMulticastGroup(const QHostAddress &groupAddress, const QNetworkInterface &iface)
{
    Q_UNUSED(groupAddress);
    Q_UNUSED(iface);
    Q_UNIMPLEMENTED();
    return false;
}

QNetworkInterface QNativeSocketEngine::multicastInterface() const
{
    Q_UNIMPLEMENTED();
    return QNetworkInterface();
}

bool QNativeSocketEngine::setMulticastInterface(const QNetworkInterface &iface)
{
    Q_UNUSED(iface);
    Q_UNIMPLEMENTED();
    return false;
}

qint64 QNativeSocketEngine::bytesAvailable() const
{
    Q_D(const QNativeSocketEngine);
    if (d->socketType != QAbstractSocket::TcpSocket)
        return -1;

    return d->bytesAvailable;
}

qint64 QNativeSocketEngine::read(char *data, qint64 maxlen)
{
    Q_D(QNativeSocketEngine);
    if (d->socketType != QAbstractSocket::TcpSocket)
        return -1;

    // There will be a read notification when the socket was closed by the remote host. If that
    // happens and there isn't anything left in the buffer, we have to return -1 in order to signal
    // the closing of the socket.
    QMutexLocker mutexLocker(&d->readMutex);
    if (d->pendingData.isEmpty() && d->socketState != QAbstractSocket::ConnectedState) {
        close();
        return -1;
    }

    QByteArray readData;
    qint64 leftToMaxLen = maxlen;
    while (leftToMaxLen > 0 && !d->pendingData.isEmpty()) {
        QByteArray pendingData = d->pendingData.takeFirst();
        // Do not read the whole data. Put the rest of it back into the "queue"
        if (leftToMaxLen < pendingData.length()) {
            readData += pendingData.left(leftToMaxLen);
            pendingData = pendingData.remove(0, maxlen);
            d->pendingData.prepend(pendingData);
            break;
        } else {
            readData += pendingData;
            leftToMaxLen -= pendingData.length();
        }
    }
    const int copyLength = qMin(maxlen, qint64(readData.length()));
    d->bytesAvailable -= copyLength;
    mutexLocker.unlock();

    memcpy(data, readData, copyLength);
    return copyLength;
}

qint64 QNativeSocketEngine::write(const char *data, qint64 len)
{
    Q_D(QNativeSocketEngine);
    if (!isValid())
        return -1;

    HRESULT hr = E_FAIL;
    ComPtr<IOutputStream> stream;
    if (d->socketType == QAbstractSocket::TcpSocket)
        hr = d->tcpSocket()->get_OutputStream(&stream);
    else if (d->socketType == QAbstractSocket::UdpSocket)
        hr = d->udpSocket()->get_OutputStream(&stream);
    Q_ASSERT_SUCCEEDED(hr);

    qint64 bytesWritten = writeIOStream(stream, data, len);
    if (bytesWritten < 0)
        d->setError(QAbstractSocket::SocketAccessError, QNativeSocketEnginePrivate::AccessErrorString);
    else if (bytesWritten > 0 && d->notifyOnWrite)
        emit writeReady();

    return bytesWritten;
}

qint64 QNativeSocketEngine::readDatagram(char *data, qint64 maxlen, QIpPacketHeader *header,
                                         PacketHeaderOptions)
{
#ifndef QT_NO_UDPSOCKET
    Q_D(QNativeSocketEngine);
    QMutexLocker locker(&d->readMutex);
    if (d->socketType != QAbstractSocket::UdpSocket || d->pendingDatagrams.isEmpty()) {
        if (header)
            header->clear();
        return -1;
    }

    WinRtDatagram datagram = d->pendingDatagrams.takeFirst();
    if (header)
        *header = datagram.header;

    QByteArray readOrigin;
    // Do not read the whole datagram. Put the rest of it back into the "queue"
    if (maxlen < datagram.data.length()) {
        readOrigin = datagram.data.left(maxlen);
        datagram.data = datagram.data.remove(0, maxlen);
        d->pendingDatagrams.prepend(datagram);
    } else {
        readOrigin = datagram.data;
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
#ifndef QT_NO_UDPSOCKET
    Q_D(QNativeSocketEngine);
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
    QMutexLocker locker(&d->readMutex);
    return d->pendingDatagrams.length() > 0;
}

qint64 QNativeSocketEngine::pendingDatagramSize() const
{
    Q_D(const QNativeSocketEngine);
    QMutexLocker locker(&d->readMutex);
    if (d->pendingDatagrams.isEmpty())
        return -1;

    return d->pendingDatagrams.at(0).data.length();
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
    Q_D(QNativeSocketEngine);
    return d->setOption(option, value);
}

bool QNativeSocketEngine::waitForRead(int msecs, bool *timedOut)
{
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
        QMutexLocker locker(&d->readMutex);
        if (!d->pendingData.isEmpty())
            return true;

        // Nothing to do, wait for more events
        d->eventLoop.processEvents();
    }

    d->setError(QAbstractSocket::SocketTimeoutError,
                QNativeSocketEnginePrivate::TimeOutErrorString);

    if (timedOut)
        *timedOut = true;
    return false;
}

bool QNativeSocketEngine::waitForWrite(int msecs, bool *timedOut)
{
    Q_UNUSED(msecs);
    Q_UNUSED(timedOut);
    Q_D(QNativeSocketEngine);
    if (d->socketState == QAbstractSocket::ConnectingState) {
        HRESULT hr = QWinRTFunctions::await(d->connectOp, QWinRTFunctions::ProcessMainThreadEvents);
        if (SUCCEEDED(hr)) {
            d->handleConnectOpFinished(d->connectOp.Get(), Completed);
            return true;
        }
    }
    return false;
}

bool QNativeSocketEngine::waitForReadOrWrite(bool *readyToRead, bool *readyToWrite, bool checkRead, bool checkWrite, int msecs, bool *timedOut)
{
    Q_UNUSED(readyToRead);
    Q_UNUSED(readyToWrite);
    Q_UNUSED(checkRead);
    Q_UNUSED(checkWrite);
    Q_UNUSED(msecs);
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
    Q_D(QNativeSocketEngine);
    d->notifyOnException = enable;
}

void QNativeSocketEngine::establishRead()
{
    Q_D(QNativeSocketEngine);

    HRESULT hr;
    hr = QEventDispatcherWinRT::runOnXamlThread([d]() {
        d->worker->setTcpSocket(d->tcpSocket());
        d->worker->startReading();
        return S_OK;
    });
    Q_ASSERT_SUCCEEDED(hr);
}

void QNativeSocketEngine::handleNewDatagrams(const QList<WinRtDatagram> &datagrams)
{
    Q_D(QNativeSocketEngine);
    // Defer putting the datagrams into the list until the next event loop iteration
    // (where the readyRead signal is emitted as well)
    QMetaObject::invokeMethod(this, "putIntoPendingDatagramsList", Qt::QueuedConnection,
                              Q_ARG(QList<WinRtDatagram>, datagrams));
    if (d->notifyOnRead)
        emit readReady();
}

void QNativeSocketEngine::handleNewData(const QVector<QByteArray> &data)
{
    // Defer putting the data into the list until the next event loop iteration
    // (where the readyRead signal is emitted as well)
    QMetaObject::invokeMethod(this, "putIntoPendingData", Qt::QueuedConnection,
                              Q_ARG(QVector<QByteArray>, data));
}

void QNativeSocketEngine::handleTcpError(QAbstractSocket::SocketError error)
{
    Q_D(QNativeSocketEngine);
    QNativeSocketEnginePrivate::ErrorString errorString;
    switch (error) {
    case QAbstractSocket::RemoteHostClosedError:
        errorString = QNativeSocketEnginePrivate::RemoteHostClosedErrorString;
        break;
    default:
        errorString = QNativeSocketEnginePrivate::UnknownSocketErrorString;
    }

    d->setError(error, errorString);
    d->socketState = QAbstractSocket::UnconnectedState;
    if (d->notifyOnRead)
        emit readReady();
}

void QNativeSocketEngine::putIntoPendingDatagramsList(const QList<WinRtDatagram> &datagrams)
{
    Q_D(QNativeSocketEngine);
    QMutexLocker locker(&d->readMutex);
    d->pendingDatagrams.append(datagrams);
}

void QNativeSocketEngine::putIntoPendingData(const QVector<QByteArray> &data)
{
    Q_D(QNativeSocketEngine);
    QMutexLocker locker(&d->readMutex);
    d->pendingData.append(data);
    for (const QByteArray &newData : data)
        d->bytesAvailable += newData.length();
    locker.unlock();
    if (d->notifyOnRead)
        readNotification();
}

bool QNativeSocketEnginePrivate::createNewSocket(QAbstractSocket::SocketType socketType, QAbstractSocket::NetworkLayerProtocol &socketProtocol)
{
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
        setError(QAbstractSocket::UnsupportedSocketOperationError, NonBlockingInitFailedErrorString);
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
    , sslSocket(Q_NULLPTR)
    , connectionToken( { -1 } )
{
}

QNativeSocketEnginePrivate::~QNativeSocketEnginePrivate()
{
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

void QNativeSocketEnginePrivate::setError(QAbstractSocket::SocketError error, ErrorString errorString) const
{
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
    case NonBlockingInitFailedErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unable to initialize non-blocking socket");
        break;
    case BroadcastingInitFailedErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unable to initialize broadcast socket");
        break;
    // should not happen anymore
    case NoIpV6ErrorString:
        socketErrorString = QNativeSocketEngine::tr("Attempt to use IPv6 socket on a platform with no IPv6 support");
        break;
    case RemoteHostClosedErrorString:
        socketErrorString = QNativeSocketEngine::tr("The remote host closed the connection");
        break;
    case TimeOutErrorString:
        socketErrorString = QNativeSocketEngine::tr("Network operation timed out");
        break;
    case ResourceErrorString:
        socketErrorString = QNativeSocketEngine::tr("Out of resources");
        break;
    case OperationUnsupportedErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unsupported socket operation");
        break;
    case ProtocolUnsupportedErrorString:
        socketErrorString = QNativeSocketEngine::tr("Protocol type not supported");
        break;
    case InvalidSocketErrorString:
        socketErrorString = QNativeSocketEngine::tr("Invalid socket descriptor");
        break;
    case HostUnreachableErrorString:
        socketErrorString = QNativeSocketEngine::tr("Host unreachable");
        break;
    case NetworkUnreachableErrorString:
        socketErrorString = QNativeSocketEngine::tr("Network unreachable");
        break;
    case AccessErrorString:
        socketErrorString = QNativeSocketEngine::tr("Permission denied");
        break;
    case ConnectionTimeOutErrorString:
        socketErrorString = QNativeSocketEngine::tr("Connection timed out");
        break;
    case ConnectionRefusedErrorString:
        socketErrorString = QNativeSocketEngine::tr("Connection refused");
        break;
    case AddressInuseErrorString:
        socketErrorString = QNativeSocketEngine::tr("The bound address is already in use");
        break;
    case AddressNotAvailableErrorString:
        socketErrorString = QNativeSocketEngine::tr("The address is not available");
        break;
    case AddressProtectedErrorString:
        socketErrorString = QNativeSocketEngine::tr("The address is protected");
        break;
    case DatagramTooLargeErrorString:
        socketErrorString = QNativeSocketEngine::tr("Datagram was too large to send");
        break;
    case SendDatagramErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unable to send a message");
        break;
    case ReceiveDatagramErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unable to receive a message");
        break;
    case WriteErrorString:
        socketErrorString = QNativeSocketEngine::tr("Unable to write");
        break;
    case ReadErrorString:
        socketErrorString = QNativeSocketEngine::tr("Network error");
        break;
    case PortInuseErrorString:
        socketErrorString = QNativeSocketEngine::tr("Another socket is already listening on the same port");
        break;
    case NotSocketErrorString:
        socketErrorString = QNativeSocketEngine::tr("Operation on non-socket");
        break;
    case InvalidProxyTypeString:
        socketErrorString = QNativeSocketEngine::tr("The proxy type is invalid for this operation");
        break;
    case TemporaryErrorString:
        socketErrorString = QNativeSocketEngine::tr("Temporary error");
        break;
    case UnknownSocketErrorString:
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
    default:
        return -1;
    }
    return -1;
}

bool QNativeSocketEnginePrivate::setOption(QAbstractSocketEngine::SocketOption opt, int v)
{
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
    default:
        return false;
    }
    return false;
}

bool QNativeSocketEnginePrivate::fetchConnectionParameters()
{
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

HRESULT QNativeSocketEnginePrivate::handleConnectOpFinished(IAsyncAction *action, AsyncStatus)
{
    Q_Q(QNativeSocketEngine);
    if (wasDeleted || !connectOp) // Protect against a late callback
        return S_OK;

    HRESULT hr = action->GetResults();
    switch (hr) {
    case 0x8007274c: // A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond.
        setError(QAbstractSocket::NetworkError, ConnectionTimeOutErrorString);
        socketState = QAbstractSocket::UnconnectedState;
        return S_OK;
    case 0x80072751: // A socket operation was attempted to an unreachable host.
        setError(QAbstractSocket::HostNotFoundError, HostUnreachableErrorString);
        socketState = QAbstractSocket::UnconnectedState;
        return S_OK;
    case 0x8007274d: // No connection could be made because the target machine actively refused it.
        setError(QAbstractSocket::ConnectionRefusedError, ConnectionRefusedErrorString);
        socketState = QAbstractSocket::UnconnectedState;
        return S_OK;
    default:
        if (FAILED(hr)) {
            setError(QAbstractSocket::UnknownSocketError, UnknownSocketErrorString);
            socketState = QAbstractSocket::UnconnectedState;
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

    socketState = QAbstractSocket::ConnectedState;
    emit q->connectionReady();

    if (socketType != QAbstractSocket::TcpSocket)
        return S_OK;

#ifndef QT_NO_SSL
    // Delay the reader so that the SSL socket can upgrade
    if (sslSocket)
        QObject::connect(qobject_cast<QSslSocket *>(sslSocket), &QSslSocket::encrypted, q, &QNativeSocketEngine::establishRead);
    else
#endif
        q->establishRead();
    return S_OK;
}

HRESULT QNativeSocketEnginePrivate::handleNewDatagram(IDatagramSocket *socket, IDatagramSocketMessageReceivedEventArgs *args)
{
    Q_Q(QNativeSocketEngine);
    Q_UNUSED(socket);

    WinRtDatagram datagram;
    QHostAddress returnAddress;
    ComPtr<IHostName> remoteHost;
    HRESULT hr = args->get_RemoteAddress(&remoteHost);
    RETURN_OK_IF_FAILED("Could not obtain remote host");
    HString remoteHostString;
    remoteHost->get_CanonicalName(remoteHostString.GetAddressOf());
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
    emit q->newDatagramReceived(datagram);

    return S_OK;
}

QT_END_NAMESPACE

#include "qnativesocketengine_winrt.moc"
