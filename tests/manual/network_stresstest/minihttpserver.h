/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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
