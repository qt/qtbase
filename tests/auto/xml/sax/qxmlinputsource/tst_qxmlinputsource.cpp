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


#include <QDomDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QElapsedTimer>
#include <QtDebug>
#include <QtTest/QtTest>
#include <QXmlDefaultHandler>
#include <QXmlInputSource>
#include <QXmlSimpleReader>

class tst_QXmlInputSource : public QObject
{
    Q_OBJECT

#if QT_DEPRECATED_SINCE(5, 15)
private slots:
    void reset() const;
    void resetSimplified() const;
    void waitForReadyIODevice() const;
    void inputFromSlowDevice() const;
#endif // QT_DEPRECATED_SINCE(5, 15)
};

#if QT_DEPRECATED_SINCE(5, 15)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
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
                body.append(line.toUtf8());
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

// This class is used to emulate a case where less than 4 bytes are sent in
// a single packet to ensure it is still parsed correctly
class SlowIODevice : public QIODevice
{
public:
    SlowIODevice(const QString &expectedData, QObject *parent = 0)
        : QIODevice(parent), currentPos(0), readyToSend(true)
    {
        stringData = expectedData.toUtf8();
        dataTimer = new QTimer(this);
        connect(dataTimer, &QTimer::timeout, [=]() {
            readyToSend = true;
            emit readyRead();
            dataTimer->stop();
        });
        dataTimer->start(1000);
    }
    bool open(SlowIODevice::OpenMode) override
    {
        setOpenMode(ReadOnly);
        return true;
    }
    bool isSequential() const override
    {
        return true;
    }
    qint64 bytesAvailable() const override
    {
        if (readyToSend && stringData.size() != currentPos)
            return qMax(3, stringData.size() - currentPos);
        return 0;
    }
    qint64 readData(char *data, qint64 maxSize) override
    {
        if (!readyToSend)
            return 0;
        const qint64 readSize = qMin(qMin((qint64)3, maxSize), (qint64)(stringData.size() - currentPos));
        if (readSize > 0)
            memcpy(data, &stringData.constData()[currentPos], readSize);
        currentPos += readSize;
        readyToSend = false;
        if (currentPos != stringData.size())
            dataTimer->start(1000);
        return readSize;
    }
    qint64 writeData(const char *, qint64) override { return 0; }
    bool waitForReadyRead(int msecs) override
    {
        // Delibrately wait a maximum of 10 seconds for the sake
        // of the test, so it doesn't unduly hang
        const int waitTime = qMax(10000, msecs);
        QElapsedTimer t;
        t.start();
        while (t.elapsed() < waitTime) {
            QCoreApplication::processEvents();
            if (readyToSend)
                return true;
        }
        return false;
    }
private:
    QByteArray stringData;
    int currentPos;
    bool readyToSend;
    QTimer *dataTimer;
};

void tst_QXmlInputSource::inputFromSlowDevice() const
{
    QString expectedData = QStringLiteral("<foo><bar>kake</bar><bar>ja</bar></foo>");
    SlowIODevice slowDevice(expectedData);
    QXmlInputSource source(&slowDevice);
    QString data;
    while (true) {
        const QChar nextChar = source.next();
        if (nextChar == QXmlInputSource::EndOfDocument)
            break;
        else if (nextChar != QXmlInputSource::EndOfData)
            data += nextChar;
    }
    QCOMPARE(data, expectedData);
}

QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(5, 15)

QTEST_MAIN(tst_QXmlInputSource)
#include "tst_qxmlinputsource.moc"
