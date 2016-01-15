/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef MINIHTTPSERVER_H
#define MINIHTTPSERVER_H

#include <QtCore/QThread>
#include <QtCore/QFile>
#include <QtCore/QTimer>

class QFile;
class QSemaphore;
class QTcpServer;
class QTcpSocket;

class MiniHttpServer : public QThread
{
    Q_OBJECT
public:
    explicit MiniHttpServer(QObject *parent = 0);
    ~MiniHttpServer();

    int port() { return portnum; }

protected:
    void run();

private slots:
    void handleConnection();

private:
    QTcpServer *server;
    QObject *quitObject;
    QSemaphore *readyToGo;
    int portnum;
};

class MiniHttpServerConnection: public QObject
{
    Q_OBJECT
    QTcpSocket * const socket;
    QFile source;
    QTimer timeout;
    QByteArray buffer;
    bool connectionClose;
public:
    explicit MiniHttpServerConnection(QTcpSocket *socket);

    void sendError500();
    void sendError404();
    void handlePendingRequest();

public slots:
    void handleReadyRead();
    void handleBytesWritten();
    void handleDisconnected();
    void handleTimeout();
};

#endif // MINIHTTPSERVER_H
