// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qabstractsocketengine_p.h"

#include "qnativesocketengine_p.h"

#include "qmutex.h"
#include "qnetworkproxy.h"

QT_BEGIN_NAMESPACE

class QSocketEngineHandlerList : public QList<QSocketEngineHandler*>
{
public:
    QMutex mutex;
};

Q_GLOBAL_STATIC(QSocketEngineHandlerList, socketHandlers)

QSocketEngineHandler::QSocketEngineHandler()
{
    if (!socketHandlers())
        return;
    QMutexLocker locker(&socketHandlers()->mutex);
    socketHandlers()->prepend(this);
}

QSocketEngineHandler::~QSocketEngineHandler()
{
    if (!socketHandlers())
        return;
    QMutexLocker locker(&socketHandlers()->mutex);
    socketHandlers()->removeAll(this);
}

QAbstractSocketEnginePrivate::QAbstractSocketEnginePrivate()
    : socketError(QAbstractSocket::UnknownSocketError)
    , hasSetSocketError(false)
    , socketErrorString(QLatin1StringView(QT_TRANSLATE_NOOP(QSocketLayer, "Unknown error")))
    , socketState(QAbstractSocket::UnconnectedState)
    , socketType(QAbstractSocket::UnknownSocketType)
    , socketProtocol(QAbstractSocket::UnknownNetworkLayerProtocol)
    , localPort(0)
    , peerPort(0)
    , inboundStreamCount(0)
    , outboundStreamCount(0)
    , receiver(nullptr)
{
}

QAbstractSocketEngine::QAbstractSocketEngine(QObject *parent)
    : QObject(*new QAbstractSocketEnginePrivate(), parent)
{
}

QAbstractSocketEngine::QAbstractSocketEngine(QAbstractSocketEnginePrivate &dd, QObject* parent)
    : QObject(dd, parent)
{
}

QAbstractSocketEngine *QAbstractSocketEngine::createSocketEngine(QAbstractSocket::SocketType socketType, const QNetworkProxy &proxy, QObject *parent)
{
#ifndef QT_NO_NETWORKPROXY
    // proxy type must have been resolved by now
    if (proxy.type() == QNetworkProxy::DefaultProxy)
        return nullptr;
#endif

    QMutexLocker locker(&socketHandlers()->mutex);
    for (int i = 0; i < socketHandlers()->size(); i++) {
        if (QAbstractSocketEngine *ret = socketHandlers()->at(i)->createSocketEngine(socketType, proxy, parent))
            return ret;
    }

#ifndef QT_NO_NETWORKPROXY
    // only NoProxy can have reached here
    if (proxy.type() != QNetworkProxy::NoProxy)
        return nullptr;
#endif

    return new QNativeSocketEngine(parent);
}

QAbstractSocketEngine *QAbstractSocketEngine::createSocketEngine(qintptr socketDescripter, QObject *parent)
{
    QMutexLocker locker(&socketHandlers()->mutex);
    for (int i = 0; i < socketHandlers()->size(); i++) {
        if (QAbstractSocketEngine *ret = socketHandlers()->at(i)->createSocketEngine(socketDescripter, parent))
            return ret;
    }
    return new QNativeSocketEngine(parent);
}

QAbstractSocket::SocketError QAbstractSocketEngine::error() const
{
    return d_func()->socketError;
}

QString QAbstractSocketEngine::errorString() const
{
    return d_func()->socketErrorString;
}

void QAbstractSocketEngine::setError(QAbstractSocket::SocketError error, const QString &errorString) const
{
    Q_D(const QAbstractSocketEngine);
    d->socketError = error;
    d->socketErrorString = errorString;
}

void QAbstractSocketEngine::setReceiver(QAbstractSocketEngineReceiver *receiver)
{
    d_func()->receiver = receiver;
}

void QAbstractSocketEngine::readNotification()
{
    if (QAbstractSocketEngineReceiver *receiver = d_func()->receiver)
        receiver->readNotification();
}

void QAbstractSocketEngine::writeNotification()
{
    if (QAbstractSocketEngineReceiver *receiver = d_func()->receiver)
        receiver->writeNotification();
}

void QAbstractSocketEngine::exceptionNotification()
{
    if (QAbstractSocketEngineReceiver *receiver = d_func()->receiver)
        receiver->exceptionNotification();
}

void QAbstractSocketEngine::closeNotification()
{
    if (QAbstractSocketEngineReceiver *receiver = d_func()->receiver)
        receiver->closeNotification();
}

void QAbstractSocketEngine::connectionNotification()
{
    if (QAbstractSocketEngineReceiver *receiver = d_func()->receiver)
        receiver->connectionNotification();
}

#ifndef QT_NO_NETWORKPROXY
void QAbstractSocketEngine::proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)
{
    if (QAbstractSocketEngineReceiver *receiver = d_func()->receiver)
        receiver->proxyAuthenticationRequired(proxy, authenticator);
}
#endif


QAbstractSocket::SocketState QAbstractSocketEngine::state() const
{
    return d_func()->socketState;
}

void QAbstractSocketEngine::setState(QAbstractSocket::SocketState state)
{
    d_func()->socketState = state;
}

QAbstractSocket::SocketType QAbstractSocketEngine::socketType() const
{
    return d_func()->socketType;
}

void QAbstractSocketEngine::setSocketType(QAbstractSocket::SocketType socketType)
{
    d_func()->socketType = socketType;
}

QAbstractSocket::NetworkLayerProtocol QAbstractSocketEngine::protocol() const
{
    return d_func()->socketProtocol;
}

void QAbstractSocketEngine::setProtocol(QAbstractSocket::NetworkLayerProtocol protocol)
{
    d_func()->socketProtocol = protocol;
}

QHostAddress QAbstractSocketEngine::localAddress() const
{
    return d_func()->localAddress;
}

void QAbstractSocketEngine::setLocalAddress(const QHostAddress &address)
{
    d_func()->localAddress = address;
}

quint16 QAbstractSocketEngine::localPort() const
{
    return d_func()->localPort;
}

void QAbstractSocketEngine::setLocalPort(quint16 port)
{
    d_func()->localPort = port;
}

QHostAddress QAbstractSocketEngine::peerAddress() const
{
    return d_func()->peerAddress;
}

void QAbstractSocketEngine::setPeerAddress(const QHostAddress &address)
{
   d_func()->peerAddress = address;
}

quint16 QAbstractSocketEngine::peerPort() const
{
    return d_func()->peerPort;
}

void QAbstractSocketEngine::setPeerPort(quint16 port)
{
    d_func()->peerPort = port;
}

int QAbstractSocketEngine::inboundStreamCount() const
{
    return d_func()->inboundStreamCount;
}

int QAbstractSocketEngine::outboundStreamCount() const
{
    return d_func()->outboundStreamCount;
}

QT_END_NAMESPACE

#include "moc_qabstractsocketengine_p.cpp"
