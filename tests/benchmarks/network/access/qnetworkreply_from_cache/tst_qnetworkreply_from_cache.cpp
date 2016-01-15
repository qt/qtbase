/****************************************************************************
**
** Copyright (C) 2012 Hewlett-Packard Development Company, L.P.
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

#include <QtTest/QtTest>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkDiskCache>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#define TEST_CASE_TIMEOUT 30

class NetworkDiskCache : public QNetworkDiskCache
{
public:
    NetworkDiskCache(QObject *parent = 0)
        : QNetworkDiskCache(parent)
    {
    }

    QByteArray cachedData;

    virtual QNetworkCacheMetaData metaData(const QUrl &url)
    {
        QNetworkCacheMetaData metaData;
        if (!cachedData.isEmpty()) {
            metaData.setUrl(url);
            QDateTime now = QDateTime::currentDateTime();
            metaData.setLastModified(now.addDays(-1));
            metaData.setExpirationDate(now.addDays(1));
            metaData.setSaveToDisk(true);
        }
        return metaData;
    }

    virtual QIODevice *data(const QUrl &/*url*/)
    {
        if (cachedData.isEmpty())
            return 0;

        QBuffer *buffer = new QBuffer;
        buffer->setData(cachedData);
        buffer->open(QIODevice::ReadOnly);
        return buffer;
    }
};

class HttpServer : public QTcpServer
{
    Q_OBJECT
public:
    HttpServer(const QByteArray &reply)
        : m_reply(reply), m_writePos(), m_client()
    {
        listen(QHostAddress::AnyIPv4);
        connect(this, SIGNAL(newConnection()), this, SLOT(accept()));
    }

private Q_SLOTS:
    void accept()
    {
        m_client = nextPendingConnection();
        m_client->setParent(this);
        connect(m_client, SIGNAL(readyRead()), this, SLOT(reply()));
    }

    void reply()
    {
        disconnect(m_client, SIGNAL(readyRead()));
        m_client->readAll();
        connect(m_client, SIGNAL(bytesWritten(qint64)), this, SLOT(write()));
        write();
    }

    void write()
    {
        qint64 pos = m_client->write(m_reply.mid(m_writePos));
        if (pos > 0)
            m_writePos += pos;
        if (m_writePos >= m_reply.size())
            m_client->disconnect();
    }

private:
    QByteArray m_reply;
    qint64 m_writePos;
    QTcpSocket *m_client;
};

class tst_qnetworkreply_from_cache : public QObject
{
    Q_OBJECT
public:
    tst_qnetworkreply_from_cache();

    void timeReadAll(const QString &headers, const QByteArray &data = QByteArray());

private Q_SLOTS:
    void initTestCase();
    void cleanup();

    void readAll_data();
    void readAll();
    void readAllFromCache_data();
    void readAllFromCache();

protected Q_SLOTS:
    void replyReadAll() { m_replyData += m_reply->readAll(); }

private:
    QTemporaryDir m_tempDir;
    QNetworkAccessManager *m_networkAccessManager;
    NetworkDiskCache *m_networkDiskCache;
    QNetworkReply *m_reply;
    QByteArray m_replyData;
};

tst_qnetworkreply_from_cache::tst_qnetworkreply_from_cache()
    : m_tempDir(QDir::tempPath() + "/tst_qnetworkreply_from_cache.XXXXXX")
{
}

void tst_qnetworkreply_from_cache::timeReadAll(const QString &headers, const QByteArray &data)
{
    QByteArray reply;
    reply.append(headers);
    reply.append(data);

    m_replyData.reserve(data.size());

    HttpServer server(reply);

    QBENCHMARK_ONCE {
        QNetworkRequest request(QUrl(QString("http://127.0.0.1:%1").arg(server.serverPort())));
        m_reply = m_networkAccessManager->get(request);
        connect(m_reply, SIGNAL(readyRead()), this, SLOT(replyReadAll()), Qt::QueuedConnection);
        connect(m_reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()), Qt::QueuedConnection);
        QTestEventLoop::instance().enterLoop(TEST_CASE_TIMEOUT);
        QVERIFY(!QTestEventLoop::instance().timeout());
        delete m_reply;
    }

    QCOMPARE(data.size(), m_replyData.size());
    QCOMPARE(data, m_replyData);
}

void tst_qnetworkreply_from_cache::initTestCase()
{
    m_networkAccessManager = new QNetworkAccessManager(this);
    m_networkDiskCache = new NetworkDiskCache(m_networkAccessManager);
    m_networkDiskCache->setCacheDirectory(m_tempDir.path());
    m_networkAccessManager->setCache(m_networkDiskCache);
}

void tst_qnetworkreply_from_cache::cleanup()
{
    m_replyData.clear();
}

void tst_qnetworkreply_from_cache::readAll_data()
{
    QTest::addColumn<int>("dataSize");
    QTest::newRow("1MB") << (int)1e6;
    QTest::newRow("5MB") << (int)5e6;
    QTest::newRow("10MB") << (int)10e6;
}

void tst_qnetworkreply_from_cache::readAll()
{
    QFETCH(int, dataSize);
    QString headers = QString("HTTP/1.0 200 OK\r\nContent-Length: %1\r\n\r\n").arg(dataSize);
    QByteArray data(QByteArray(dataSize, (char)42));
    m_networkDiskCache->cachedData.clear();
    timeReadAll(headers, data);
}

void tst_qnetworkreply_from_cache::readAllFromCache_data()
{
    readAll_data();
}

void tst_qnetworkreply_from_cache::readAllFromCache()
{
    QFETCH(int, dataSize);
    QByteArray headers("HTTP/1.0 304 Use Cache\r\n\r\n");
    QByteArray data(QByteArray(dataSize, (char)42));
    m_networkDiskCache->cachedData = data;
    timeReadAll(headers, data);
}

QTEST_MAIN(tst_qnetworkreply_from_cache)
#include "tst_qnetworkreply_from_cache.moc"
