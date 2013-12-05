/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qnativesocketengine_winrt_p.h"

#include <qnetworkinterface.h>

QT_BEGIN_NAMESPACE

QNativeSocketEngine::QNativeSocketEngine(QObject *parent)
    : QAbstractSocketEngine(*new QNativeSocketEnginePrivate(), parent)
{
}

QNativeSocketEngine::~QNativeSocketEngine()
{
    close();
}

bool QNativeSocketEngine::initialize(QAbstractSocket::SocketType type, QAbstractSocket::NetworkLayerProtocol protocol)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(type);
    Q_UNUSED(protocol);
    return false;
}

bool QNativeSocketEngine::initialize(qintptr socketDescriptor, QAbstractSocket::SocketState socketState)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(socketDescriptor);
    Q_UNUSED(socketState);
    return false;
}

qintptr QNativeSocketEngine::socketDescriptor() const
{
    Q_UNIMPLEMENTED();
    return -1;
}

bool QNativeSocketEngine::isValid() const
{
    Q_UNIMPLEMENTED();
    return false;
}

bool QNativeSocketEngine::connectToHost(const QHostAddress &address, quint16 port)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(address);
    Q_UNUSED(port);
    return false;
}

bool QNativeSocketEngine::connectToHostByName(const QString &name, quint16 port)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(name);
    Q_UNUSED(port);
    return false;
}

bool QNativeSocketEngine::bind(const QHostAddress &address, quint16 port)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(address);
    Q_UNUSED(port);
    return false;
}

bool QNativeSocketEngine::listen()
{
    Q_UNIMPLEMENTED();
    return false;
}

int QNativeSocketEngine::accept()
{
    Q_UNIMPLEMENTED();
    return -1;
}

void QNativeSocketEngine::close()
{
}

bool QNativeSocketEngine::joinMulticastGroup(const QHostAddress &groupAddress, const QNetworkInterface &iface)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(groupAddress);
    Q_UNUSED(iface);
    return false;
}

bool QNativeSocketEngine::leaveMulticastGroup(const QHostAddress &groupAddress, const QNetworkInterface &iface)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(groupAddress);
    Q_UNUSED(iface);
    return false;
}

QNetworkInterface QNativeSocketEngine::multicastInterface() const
{
    Q_UNIMPLEMENTED();
    return QNetworkInterface();
}

bool QNativeSocketEngine::setMulticastInterface(const QNetworkInterface &iface)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(iface);
    return false;
}

qint64 QNativeSocketEngine::bytesAvailable() const
{
    Q_UNIMPLEMENTED();
    return -1;
}

qint64 QNativeSocketEngine::read(char *data, qint64 maxlen)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(data);
    Q_UNUSED(maxlen);
    return -1;
}

qint64 QNativeSocketEngine::write(const char *data, qint64 len)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(data);
    Q_UNUSED(len);
    return -1;
}

qint64 QNativeSocketEngine::readDatagram(char *data, qint64 maxlen, QHostAddress *addr, quint16 *port)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(data);
    Q_UNUSED(maxlen);
    Q_UNUSED(addr);
    Q_UNUSED(port);
    return -1;
}

qint64 QNativeSocketEngine::writeDatagram(const char *data, qint64 len, const QHostAddress &addr, quint16 port)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(data);
    Q_UNUSED(len);
    Q_UNUSED(addr);
    Q_UNUSED(port);
    return -1;
}

bool QNativeSocketEngine::hasPendingDatagrams() const
{
    Q_UNIMPLEMENTED();
    return false;
}

qint64 QNativeSocketEngine::pendingDatagramSize() const
{
    Q_UNIMPLEMENTED();
    return 0;
}

qint64 QNativeSocketEngine::bytesToWrite() const
{
    Q_UNIMPLEMENTED();
    return 0;
}

qint64 QNativeSocketEngine::receiveBufferSize() const
{
    Q_UNIMPLEMENTED();
    return 0;
}

void QNativeSocketEngine::setReceiveBufferSize(qint64 bufferSize)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(bufferSize);
}

qint64 QNativeSocketEngine::sendBufferSize() const
{
    Q_UNIMPLEMENTED();
    return 0;
}

void QNativeSocketEngine::setSendBufferSize(qint64 bufferSize)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(bufferSize);
}

int QNativeSocketEngine::option(QAbstractSocketEngine::SocketOption option) const
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(option);
    return -1;
}

bool QNativeSocketEngine::setOption(QAbstractSocketEngine::SocketOption option, int value)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(option);
    Q_UNUSED(value);
    return false;
}

bool QNativeSocketEngine::waitForRead(int msecs, bool *timedOut)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(msecs);
    Q_UNUSED(timedOut);
    return false;
}

bool QNativeSocketEngine::waitForWrite(int msecs, bool *timedOut)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(msecs);
    Q_UNUSED(timedOut);
    return false;
}

bool QNativeSocketEngine::waitForReadOrWrite(bool *readyToRead, bool *readyToWrite, bool checkRead, bool checkWrite, int msecs, bool *timedOut)
{
    Q_UNIMPLEMENTED();
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
    Q_UNIMPLEMENTED();
    return false;
}

void QNativeSocketEngine::setReadNotificationEnabled(bool enable)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(enable);
}

bool QNativeSocketEngine::isWriteNotificationEnabled() const
{
    Q_UNIMPLEMENTED();
    return false;
}

void QNativeSocketEngine::setWriteNotificationEnabled(bool enable)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(enable);
}

bool QNativeSocketEngine::isExceptionNotificationEnabled() const
{
    Q_UNIMPLEMENTED();
    return false;
}

void QNativeSocketEngine::setExceptionNotificationEnabled(bool enable)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(enable);
}

QNativeSocketEnginePrivate::QNativeSocketEnginePrivate()
    : QAbstractSocketEnginePrivate()
{
}

QNativeSocketEnginePrivate::~QNativeSocketEnginePrivate()
{
}

QT_END_NAMESPACE
