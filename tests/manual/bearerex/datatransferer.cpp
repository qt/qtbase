/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QDebug>
#include <QUrl>
#include <QByteArray>
#include <QDataStream>
#include "datatransferer.h"

DataTransferer::DataTransferer(QObject *parent) :
    QObject(parent), m_dataTransferOngoing(false)
{
}

bool DataTransferer::dataTransferOngoing()
{
    return m_dataTransferOngoing;
}



// -------- Based on QTcp

DataTransfererQTcp::DataTransfererQTcp(QObject* parent)
:   DataTransferer(parent)
{
    qDebug("BearerEx DataTransferer QTcp created.");

    connect(&m_qsocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(&m_qsocket, SIGNAL(connected()), this, SLOT(connected()));
    connect(&m_qsocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(error(QAbstractSocket::SocketError)));
}

DataTransfererQTcp::~DataTransfererQTcp()
{
    qDebug("BearerEx DataTransferer QTcp destroyed.");
    m_qsocket.abort();
}

bool DataTransfererQTcp::transferData()
{
    if (m_dataTransferOngoing) {
        return false;
    }
    qDebug("BearerEx datatransfer for QTcp requested.");
    // Connect to host
    QUrl url("http://www.google.com.au");
    m_qsocket.connectToHost(url.host(), url.port(80));

    // m_qsocket.connectToHost("http://www.google.com", 80);
    // Wait for connected() signal.
    m_dataTransferOngoing = true;
    return true;
}

void DataTransfererQTcp::connected()
{
    qDebug("BearerEx DataTransfererQtcp connected, requesting data.");
    // Establish HTTP request
    //QByteArray request("GET / HTTP/1.1 \nHost: www.google.com\n\n");
    QByteArray request("GET / HTTP/1.1\n\n");

    // QByteArray request("GET /index.html HTTP/1.1 \n Host: www.google.com \n\n");
    qint64 dataWritten = m_qsocket.write(request);
    m_qsocket.flush();

    qDebug() << "BearerEx DataTransferQTcp wrote " << dataWritten << " bytes";
    // Start waiting for readyRead() of error()
}

void DataTransfererQTcp::readyRead()
{
    qDebug() << "BearerEx DataTransfererQTcp readyRead() with ";
    qint64 bytesAvailable = m_qsocket.bytesAvailable();
    qDebug() << bytesAvailable << " bytes available.";

    // QDataStream in(&m_qsocket);
    QByteArray array = m_qsocket.readAll();
    QString data = QString::fromAscii(array);

    // in >> data;

    qDebug() << "BearerEx DataTransferQTcp data received: " << data;
    m_dataTransferOngoing = false;
    // m_qsocket.error() returns uninitialized value in case no error has occurred,
    // so emit '0'
    emit finished(0, bytesAvailable, "QAbstractSocket::SocketError");
}

void DataTransfererQTcp::error(QAbstractSocket::SocketError socketError)
{
    qDebug("BearerEx DataTransfererQTcp error(), aborting socket.");
    m_qsocket.abort();
    m_dataTransferOngoing = false;
    emit finished(socketError, 0, "QAbstractSocket::SocketError");
}

// -------- Based on QHttp

DataTransfererQHttp::DataTransfererQHttp(QObject* parent)
:   DataTransferer(parent)
{
    connect(&m_qhttp, SIGNAL(done(bool)), this, SLOT(done(bool)));
    qDebug("BearerEx DataTransferer QHttp created.");
}

DataTransfererQHttp::~DataTransfererQHttp()
{
    qDebug("BearerEx DataTransferer QHttp destroyed.");
}

bool DataTransfererQHttp::transferData()
{
    qDebug("BearerEx datatransfer for QHttp requested.");
    if (m_dataTransferOngoing) {
        return false;
    }
    QString urlstring("http://www.google.com");
    QUrl url(urlstring);
    m_qhttp.setHost(url.host(), QHttp::ConnectionModeHttp, url.port() == -1 ? 0 : url.port());
    m_qhttp.get(urlstring);
    m_dataTransferOngoing = true;
    return true;
}

void DataTransfererQHttp::done(bool /*error*/ )
{
    qDebug("BearerEx DatatransfererQHttp reply was finished (error code is type QHttp::Error).");
    qint64 dataReceived = 0;
    quint32 errorCode = m_qhttp.error();
    if (m_qhttp.error() == QHttp::NoError) {
        QString result(m_qhttp.readAll());
        dataReceived = result.length();
    }
    m_dataTransferOngoing = false;
    emit finished(errorCode, dataReceived, "QHttp::Error");
}

// -------- Based on QNetworkAccessManager

DataTransfererQNam::DataTransfererQNam(QObject* parent)
:   DataTransferer(parent)
{
    connect(&m_qnam, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));
    qDebug("BearerEx DataTransferer QNam created.");
}

DataTransfererQNam::~DataTransfererQNam()
{
    qDebug("BearerEx DataTransferer QNam destroyed.");
}

bool DataTransfererQNam::transferData()
{
    qDebug("BearerEx datatransfer for QNam requested.");
    if (m_dataTransferOngoing) {
        return false;
    }
    m_qnam.get(QNetworkRequest(QUrl("http://www.google.com")));
    m_dataTransferOngoing = true;
    return true;
}

void DataTransfererQNam::replyFinished(QNetworkReply *reply)
{
    qDebug("BearerEx DatatransfererQNam reply was finished (error code is type QNetworkReply::NetworkError).");
    qint64 dataReceived = 0;
    quint32 errorCode = (quint32)reply->error();

    if (reply->error() == QNetworkReply::NoError) {
        QString result(reply->readAll());
        dataReceived = result.length();
    }
    m_dataTransferOngoing = false;
    emit finished(errorCode, dataReceived, "QNetworkReply::NetworkError");
    reply->deleteLater();
}



