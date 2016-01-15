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

#ifndef DATATRANSFERER_H
#define DATATRANSFERER_H

#include <QObject>
#include <QString>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QTcpSocket>
#include <QDebug>

// Interface-class for data transferring object

class DataTransferer : public QObject
{
    Q_OBJECT
public:
    explicit DataTransferer(QObject *parent = 0);
    virtual ~DataTransferer() {
        if (m_dataTransferOngoing) {
            qDebug("BearerEx Warning: dataobjects transfer was ongoing when destroyed.");
        }
    }
    virtual bool transferData() = 0;
    bool dataTransferOngoing();

signals:
    void finished(quint32 errorCode, qint64 dataReceived, QString errorType);

public slots:

protected:
    bool m_dataTransferOngoing;
};


// Specializations/concrete classes

class DataTransfererQTcp : public DataTransferer
{
    Q_OBJECT
public:
    DataTransfererQTcp(QObject* parent = 0);
    ~DataTransfererQTcp();

    virtual bool transferData();

public slots:
    void readyRead();
    void error(QAbstractSocket::SocketError socketError);
    void connected();

private:
    QTcpSocket m_qsocket;
};

class DataTransfererQNam : public DataTransferer
{
    Q_OBJECT
public:
    DataTransfererQNam(QObject* parent = 0);
    ~DataTransfererQNam();

    virtual bool transferData();

public slots:
    void replyFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager m_qnam;
};

#endif // DATATRANSFERER_H
