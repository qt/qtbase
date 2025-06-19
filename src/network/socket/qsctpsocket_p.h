// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSCTPSOCKET_P_H
#define QSCTPSOCKET_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/qsctpsocket.h>
#include <private/qtcpsocket_p.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>
#include <private/qnetworkdatagram_p.h>

#include <deque>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SCTP

class QSctpSocketPrivate : public QTcpSocketPrivate
{
    Q_DECLARE_PUBLIC(QSctpSocket)
public:
    QSctpSocketPrivate();
    virtual ~QSctpSocketPrivate();

    bool canReadNotification() override;
    bool writeToSocket() override;

    QByteArray incomingDatagram;
    int maximumChannelCount;

    typedef std::deque<QIpPacketHeader> IpHeaderList;
    QList<IpHeaderList> readHeaders;
    QList<IpHeaderList> writeHeaders;

    void configureCreatedSocket() override;
};

#endif // QT_NO_SCTP

QT_END_NAMESPACE

#endif // QSCTPSOCKET_P_H
