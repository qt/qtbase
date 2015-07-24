/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QDomDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QtDebug>
#include <QtTest/QtTest>
#include <QXmlDefaultHandler>
#include <QXmlInputSource>
#include <QXmlSimpleReader>

class tst_QXmlInputSource : public QObject
{
    Q_OBJECT

private slots:
    void reset() const;
    void resetSimplified() const;
    void waitForReadyIODevice() const;
};

/*!
  \internal
  \since 4.4

  See task 166278.
 */
void tst_QXmlInputSource::reset() const
{
    const QString input(QString::fromLatin1("<element attribute1='value1' attribute2='value2'/>"));

    QXmlSimpleReader reader;
    QXmlDefaultHandler handler;
    reader.setContentHandler(&handler);

    QXmlInputSource source;
    source.setData(input);

    QCOMPARE(source.data(), input);

    source.reset();
    QCOMPARE(source.data(), input);

    source.reset();
    QVERIFY(reader.parse(source));
    source.reset();
    QCOMPARE(source.data(), input);
}

/*!
  \internal
  \since 4.4

  See task 166278.
 */
void tst_QXmlInputSource::resetSimplified() const
{
    const QString input(QString::fromLatin1("<element/>"));

    QXmlSimpleReader reader;

    QXmlInputSource source;
    source.setData(input);

    QVERIFY(reader.parse(source));
    source.reset();
    QCOMPARE(source.data(), input);
}

class ServerAndClient : public QObject
{
    Q_OBJECT

public:
    ServerAndClient(QEventLoop &ev) : success(false)
                                    , eventLoop(ev)
                                    , bodyBytesRead(0)
                                    , bodyLength(-1)
                                    , isBody(false)
    {
        setObjectName("serverAndClient");
        tcpServer = new QTcpServer(this);
        connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
        tcpServer->listen(QHostAddress::LocalHost, 1088);
        httpClient = new QNetworkAccessManager(this);
        connect(httpClient, SIGNAL(finished(QNetworkReply*)), SLOT(requestFinished(QNetworkReply*)));
    }

    bool success;
    QEventLoop &eventLoop;

public slots:
    void doIt()
    {
        QUrl url("http://127.0.0.1:1088");
        QNetworkRequest req(url);
        req.setRawHeader("POST", url.path().toLatin1());
        req.setRawHeader("user-agent", "xml-test");
        req.setRawHeader("keep-alive", "false");
        req.setRawHeader("host", url.host().toLatin1());

        QByteArray xmlrpc("<methodCall>\r\n\
                <methodName>SFD.GetVersion</methodName>\r\n\
                <params/>\r\n\
                </methodCall>");
        req.setHeader(QNetworkRequest::ContentLengthHeader, xmlrpc.size());
        req.setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");

        httpClient->post(req, xmlrpc);
    }

    void requestFinished(QNetworkReply *reply)
    {
        QCOMPARE(reply->error(), QNetworkReply::NoError);
        reply->deleteLater();
    }

private slots:
    void newConnection()
    {
        QTcpSocket *const s = tcpServer->nextPendingConnection();

        if(s)
            connect(s, SIGNAL(readyRead()), this, SLOT(readyRead()));
    }

    void readyRead()
    {
        QTcpSocket *const s = static_cast<QTcpSocket *>(sender());

        while (s->bytesAvailable())
        {
            const QString line(s->readLine());

            if (line.startsWith("Content-Length:"))
                bodyLength = line.mid(15).toInt();

            if (isBody)
            {
                body.append(line);
                bodyBytesRead += line.length();
            }
            else if (line == "\r\n")
            {
                isBody = true;
                if (bodyLength == -1)
                {
                    qFatal("No length was specified in the header.");
                }
            }
        }

        if (bodyBytesRead == bodyLength)
        {
            QDomDocument domDoc;
            success = domDoc.setContent(body);
            eventLoop.exit();
        }
    }

private:
    QByteArray body;
    int bodyBytesRead, bodyLength;
    bool isBody;
    QTcpServer *tcpServer;
    QNetworkAccessManager* httpClient;
};

void tst_QXmlInputSource::waitForReadyIODevice() const
{
    QEventLoop el;
    ServerAndClient sv(el);
    QTimer::singleShot(1, &sv, SLOT(doIt()));

    el.exec();
    QVERIFY(sv.success);
}

QTEST_MAIN(tst_QXmlInputSource)
#include "tst_qxmlinputsource.moc"
