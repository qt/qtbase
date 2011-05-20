/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
// Qt
#include <QByteArray>
#include <QCoreApplication>
#include <QDataStream>
#include <QTimer>

// Test
#include "Test.h"

//------------------------------------------------------------------------------
My4Socket::My4Socket(QObject *parent) 
    : QTcpSocket(parent), safeShutDown(false)
{
    connect(this, SIGNAL(readyRead()), this, SLOT(read()));
    connect(this, SIGNAL(disconnected()), this, SLOT(closed()));
}

//------------------------------------------------------------------------------
void My4Socket::read(void)
{
    QDataStream in(this);

    quint32 num, reply;

    while (bytesAvailable()) {
        in >> num;
        if (num == 42) {
            safeShutDown = true;
            qDebug("SUCCESS");
            QCoreApplication::instance()->quit();
            return;
        }
        reply = num + 1;
        if (reply == 42)
            ++reply;
    }
    
    // Reply with a bigger number
    sendTest(reply);
}

//------------------------------------------------------------------------------
void My4Socket::closed(void)
{
    if (!safeShutDown)
        qDebug("FAILED");
    QCoreApplication::instance()->quit();
}

//------------------------------------------------------------------------------
void My4Socket::sendTest(quint32 num)
{
    QByteArray  block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << num;

    write(block, block.size());
}

//------------------------------------------------------------------------------
My4Server::My4Server(QObject *parent)
    : QTcpServer(parent)
{
    if (listen(QHostAddress::Any, 7700))
        qDebug("qt4server");
    QTimer::singleShot(5000, this, SLOT(stopServer()));
}

//------------------------------------------------------------------------------
void My4Server::incomingConnection(int socketId)
{
    m_socket = new My4Socket(this);
    m_socket->setSocketDescriptor(socketId);
}

//------------------------------------------------------------------------------
void My4Server::stopServer()
{
    if (m_socket) {
        qDebug("SUCCESS");
        m_socket->safeShutDown = true;
        m_socket->sendTest(42);
    } else {
        QCoreApplication::instance()->quit();
    }
}

//------------------------------------------------------------------------------
Test::Test(Type type)
{
    switch (type) {
    case Qt4Client: {
        qDebug("qt4client");
        My4Socket *s = new My4Socket(this);
        s->connectToHost("localhost", 7700);
        s->sendTest(1);
        break;
    }
    case Qt4Server: {
        new My4Server(this);
        break;
    }
    default:
        break;
    }
}
