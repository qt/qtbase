/****************************************************************************
**
** Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
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
****************************************************************************/

#ifndef QSCTPSOCKET_H
#define QSCTPSOCKET_H

#include <QtNetwork/qtcpsocket.h>
#include <QtNetwork/qnetworkdatagram.h>

QT_BEGIN_NAMESPACE

#if !defined(QT_NO_SCTP) || defined(Q_CLANG_QDOC)

class QSctpSocketPrivate;

class Q_NETWORK_EXPORT QSctpSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit QSctpSocket(QObject *parent = nullptr);
    virtual ~QSctpSocket();

    void close() override;
    void disconnectFromHost() override;

    void setMaximumChannelCount(int count);
    int maximumChannelCount() const;
    bool isInDatagramMode() const;

    QNetworkDatagram readDatagram();
    bool writeDatagram(const QNetworkDatagram &datagram);

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 readLineData(char *data, qint64 maxlen) override;

private:
    Q_DISABLE_COPY(QSctpSocket)
    Q_DECLARE_PRIVATE(QSctpSocket)
};

#endif // QT_NO_SCTP

QT_END_NAMESPACE

#endif // QSCTPSOCKET_H
