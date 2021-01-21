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

#ifndef QSCTPSERVER_H
#define QSCTPSERVER_H

#include <QtNetwork/qtcpserver.h>

QT_BEGIN_NAMESPACE


#if !defined(QT_NO_SCTP) || defined(Q_CLANG_QDOC)

class QSctpServerPrivate;
class QSctpSocket;

class Q_NETWORK_EXPORT QSctpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit QSctpServer(QObject *parent = nullptr);
    virtual ~QSctpServer();

    void setMaximumChannelCount(int count);
    int maximumChannelCount() const;

    QSctpSocket *nextPendingDatagramConnection();

protected:
    void incomingConnection(qintptr handle) override;

private:
    Q_DISABLE_COPY(QSctpServer)
    Q_DECLARE_PRIVATE(QSctpServer)
};

#endif // QT_NO_SCTP

QT_END_NAMESPACE

#endif // QSCTPSERVER_H
