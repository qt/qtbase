/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWSSOCKET_QWS_H
#define QWSSOCKET_QWS_H

#include <QtCore/qconfig.h>
#include <QtGui/qwsutils_qws.h>

#ifndef QT_NO_QWS_MULTIPROCESS

#ifndef QT_NO_SXE
#include <QtCore/qmutex.h>
#include <QtGui/private/qunixsocketserver_p.h>
#include <QtGui/private/qunixsocket_p.h>
#else
#include <QtNetwork/qtcpsocket.h>
#include <QtNetwork/qtcpserver.h>
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)


class QWSSocket : public QWS_SOCK_BASE
{
    Q_OBJECT
public:
    explicit QWSSocket(QObject *parent=0);
    ~QWSSocket();

    bool connectToLocalFile(const QString &file);

#ifndef QT_NO_SXE
    QString errorString();
Q_SIGNALS:
    void connected();
    void disconnected();
    void error(QAbstractSocket::SocketError);
private Q_SLOTS:
    void forwardStateChange(SocketState);
#endif

private:
    Q_DISABLE_COPY(QWSSocket)
};


class QWSServerSocket : public QWS_SOCK_SERVER_BASE
{
    Q_OBJECT
public:
    QWSServerSocket(const QString& file, QObject *parent=0);
    ~QWSServerSocket();

#ifndef QT_NO_SXE
    QWSSocket *nextPendingConnection();
Q_SIGNALS:
    void newConnection();
protected:
    void incomingConnection(int socketDescriptor);
private:
    QMutex ssmx;
    QList<int> inboundConnections;
#endif

private:
    Q_DISABLE_COPY(QWSServerSocket)

    void init(const QString &file);
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_QWS_MULTIPROCESS

#endif // QWSSOCKET_QWS_H
