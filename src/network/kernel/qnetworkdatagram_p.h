// Copyright (C) 2015 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNETWORKDATAGRAM_P_H
#define QNETWORKDATAGRAM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include <QtNetwork/qhostaddress.h>

QT_BEGIN_NAMESPACE

class QIpPacketHeader
{
public:
    QIpPacketHeader(const QHostAddress &dstAddr = QHostAddress(), quint16 port = 0)
        : destinationAddress(dstAddr), ifindex(0), hopLimit(-1), streamNumber(-1),
          destinationPort(port), endOfRecord(false)
    {}

    void clear()
    {
        senderAddress.clear();
        destinationAddress.clear();
        ifindex = 0;
        hopLimit = -1;
        streamNumber = -1;
        endOfRecord = false;
    }

    QHostAddress senderAddress;
    QHostAddress destinationAddress;

    uint ifindex;
    int hopLimit;
    int streamNumber;
    quint16 senderPort;
    quint16 destinationPort;
    bool endOfRecord;
};

class QNetworkDatagramPrivate
{
public:
    QNetworkDatagramPrivate(const QByteArray &data = QByteArray(),
                            const QHostAddress &dstAddr = QHostAddress(), quint16 port = 0)
        : data(data), header(dstAddr, port)
    {}
    QNetworkDatagramPrivate(const QByteArray &data, const QIpPacketHeader &header)
        : data(data), header(header)
    {}

    QByteArray data;
    QIpPacketHeader header;
};

QT_END_NAMESPACE

#endif // QNETWORKDATAGRAM_P_H
