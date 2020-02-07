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
    connect(&m_qsocket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)),
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
    QString data = QString::fromLatin1(array);

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



