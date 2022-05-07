/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QTCPSOCKET_H
#define QTCPSOCKET_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtNetwork/qabstractsocket.h>
#include <QtNetwork/qhostaddress.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE


class QTcpSocketPrivate;

class Q_NETWORK_EXPORT QTcpSocket : public QAbstractSocket
{
    Q_OBJECT
public:
    explicit QTcpSocket(QObject *parent = nullptr);
    virtual ~QTcpSocket();

#if QT_VERSION < QT_VERSION_CHECK(7,0,0) && !defined(Q_CLANG_QDOC)
    // ### Qt7: move into QAbstractSocket
    using QAbstractSocket::bind;
    bool bind(QHostAddress::SpecialAddress addr, quint16 port = 0, BindMode mode = DefaultForPlatform)
    { return bind(QHostAddress(addr), port, mode); }
#endif

protected:
    QTcpSocket(QTcpSocketPrivate &dd, QObject *parent = nullptr);
    QTcpSocket(QAbstractSocket::SocketType socketType, QTcpSocketPrivate &dd,
               QObject *parent = nullptr);

private:
    Q_DISABLE_COPY_MOVE(QTcpSocket)
    Q_DECLARE_PRIVATE(QTcpSocket)
};

QT_END_NAMESPACE

#endif // QTCPSOCKET_H
