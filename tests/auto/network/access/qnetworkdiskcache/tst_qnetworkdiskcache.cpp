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


#include <QtTest/QtTest>
#include <QtNetwork/QtNetwork>
#include <qnetworkdiskcache.h>

#include <algorithm>

#define EXAMPLE_URL "http://user:pass@localhost:4/#foo"
//cached objects are organized into these many subdirs
#define NUM_SUBDIRECTORIES 16

class tst_QNetworkDiskCache : public QObject
{
    Q_OBJECT

public:
    tst_QNetworkDiskCache();

public slots:
    void accessAfterRemoveReadyReadSlot();
    void setCookieHeaderMetaDataChangedSlot();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void qnetworkdiskcache_data();
    void qnetworkdiskcache();

    void prepare();
    void cacheSize();
    void clear();
    void data_data();
    void data();
    void metaData();
    void remove();
    void accessAfterRemove(); // QTBUG-17400
    void setCookieHeader(); // QTBUG-41514
    void setCacheDirectory_data();
    void setCacheDirectory();
    void updateMetaData();
    void fileMetaData();
    void expire();

    void oldCacheVersionFile_data();
    void oldCacheVersionFile();

    void streamVersion_data();
    void streamVersion();

    void sync();

    void crashWhenParentingCache();

private:
    QTemporaryDir tempDir;
    QUrl url; // used by accessAfterRemove(), setCookieHeader()
    QNetworkDiskCache *diskCache; // used by accessAfterRemove()
    QNetworkAccessManager *manager; // used by setCookieHeader()
};

// FIXME same as in tst_qnetworkreply.cpp .. could be unified
// Does not work for POST/PUT!
class MiniHttpServer: public QTcpServer
{
    Q_OBJECT
public:
    QTcpSocket *client; // always the last one that was received
    QByteArray dataToTransmit;
    QByteArray receivedData;
    bool doClose;
    bool multiple;
    int totalConnections;

    MiniHttpServer(const QByteArray &data) : client(0), dataToTransmit(data), doClose(true), multiple(false), totalConnections(0)
    {
        listen();
        connect(this, SIGNAL(newConnection()), this, SLOT(doAccept()));
    }

public slots:
    void doAccept()
    {
        client = nextPendingConnection();
        client->setParent(this);
        ++totalConnections;
        connect(client, SIGNAL(readyRead()), this, SLOT(readyReadSlot()));
    }

    void readyReadSlot()
    {
        receivedData += client->readAll();
        int doubleEndlPos = receivedData.indexOf("\r\n\r\n");

        if (doubleEndlPos != -1) {
            // multiple requests incoming. remove the bytes of the current one
            if (multiple)
                receivedData.remove(0, doubleEndlPos+4);

            client->write(dataToTransmit);
            if (doClose) {
                client->disconnectFromHost();
                disconnect(client, 0, this, 0);
                client = 0;
            }
        }
    }
};

// Subclass that exposes the protected functions.
class SubQNetworkDiskCache : public QNetworkDiskCache
{
public:
    ~SubQNetworkDiskCache()
    {
        if (!cacheDirectory().isEmpty())
            clear();
    }

    QNetworkCacheMetaData call_fileMetaData(QString const &fileName)
        { return SubQNetworkDiskCache::fileMetaData(fileName); }

    qint64 call_expire()
        { return SubQNetworkDiskCache::expire(); }

    void setupWithOne(const QString &path, const QUrl &url, const QNetworkCacheMetaData &metaData = QNetworkCacheMetaData())
    {
        setCacheDirectory(path);

        QIODevice *d = 0;
        if (metaData.isValid()) {
            d = prepare(metaData);
        } else {
            QNetworkCacheMetaData m;
            m.setUrl(url);
            QNetworkCacheMetaData::RawHeader header("content-type", "text/html");
            QNetworkCacheMetaData::RawHeaderList list;
            list.append(header);
            m.setRawHeaders(list);
            d = prepare(m);
        }
        d->write("Hello World!");
        insert(d);
    }
};

tst_QNetworkDiskCache::tst_QNetworkDiskCache()
    : tempDir(QDir::tempPath() + "/tst_qnetworkdiskcache.XXXXXX")
{
}

// This will be called before the first test function is executed.
// It is only called once.
void tst_QNetworkDiskCache::initTestCase()
{
    QVERIFY(tempDir.isValid());

    SubQNetworkDiskCache cache;
    cache.setCacheDirectory(tempDir.path());
}

// This will be called after the last test function is executed.
// It is only called once.
void tst_QNetworkDiskCache::cleanupTestCase()
{
    QDir workingDir("foo");
    if (workingDir.exists())
        workingDir.removeRecursively();
}

void tst_QNetworkDiskCache::qnetworkdiskcache_data()
{
}

void tst_QNetworkDiskCache::qnetworkdiskcache()
{
    QUrl url(EXAMPLE_URL);
    SubQNetworkDiskCache cache;
    QCOMPARE(cache.cacheDirectory(), QString());
    QCOMPARE(cache.cacheSize(), qint64(0));
    cache.clear();
    QCOMPARE(cache.metaData(QUrl()), QNetworkCacheMetaData());
    QCOMPARE(cache.remove(QUrl()), false);
    QCOMPARE(cache.remove(url), false);
    cache.insert((QIODevice*)0);
    cache.setCacheDirectory(QString());
    cache.updateMetaData(QNetworkCacheMetaData());
    cache.prepare(QNetworkCacheMetaData());
    QCOMPARE(cache.call_fileMetaData(QString()), QNetworkCacheMetaData());

    // leave one hanging around...
    QNetworkDiskCache badCache;
    QNetworkCacheMetaData metaData;
    metaData.setUrl(url);
    badCache.prepare(metaData);
    badCache.setCacheDirectory(tempDir.path());
    badCache.prepare(metaData);
}

void tst_QNetworkDiskCache::prepare()
{
    SubQNetworkDiskCache cache;
    cache.setCacheDirectory(tempDir.path());

    QUrl url(EXAMPLE_URL);
    QNetworkCacheMetaData metaData;
    metaData.setUrl(url);

    cache.prepare(metaData);
    cache.remove(url);
}

// public qint64 cacheSize() const
void tst_QNetworkDiskCache::cacheSize()
{
    SubQNetworkDiskCache cache;
    cache.setCacheDirectory(tempDir.path());
    QCOMPARE(cache.cacheSize(), qint64(0));

    QUrl url(EXAMPLE_URL);
    QNetworkCacheMetaData metaData;
    metaData.setUrl(url);
    QIODevice *d = cache.prepare(metaData);
    cache.insert(d);
    QVERIFY(cache.cacheSize() > qint64(0));

    cache.clear();
    QCOMPARE(cache.cacheSize(), qint64(0));
}

static QStringList countFiles(const QString dir)
{
    QStringList list;
    QDir::Filters filter(QDir::AllEntries | QDir::NoDotAndDotDot);
    QDirIterator it(dir, filter, QDirIterator::Subdirectories);
    while (it.hasNext())
        list.append(it.next());
    return list;
}

// public void clear()
void tst_QNetworkDiskCache::clear()
{
    SubQNetworkDiskCache cache;
    QUrl url(EXAMPLE_URL);
    cache.setupWithOne(tempDir.path(), url);
    QVERIFY(cache.cacheSize() > qint64(0));

    QString cacheDirectory = cache.cacheDirectory();
    QCOMPARE(countFiles(cacheDirectory).count(), NUM_SUBDIRECTORIES + 3);
    cache.clear();
    QCOMPARE(countFiles(cacheDirectory).count(), NUM_SUBDIRECTORIES + 2);

    // don't delete files that it didn't create
    QTemporaryFile file(cacheDirectory + "/XXXXXX");
    if (file.open()) {
        QCOMPARE(countFiles(cacheDirectory).count(), NUM_SUBDIRECTORIES + 3);
        cache.clear();
        QCOMPARE(countFiles(cacheDirectory).count(), NUM_SUBDIRECTORIES + 3);
    }
}

Q_DECLARE_METATYPE(QNetworkCacheMetaData)
void tst_QNetworkDiskCache::data_data()
{
    QTest::addColumn<QNetworkCacheMetaData>("data");

    QTest::newRow("null") << QNetworkCacheMetaData();

    QUrl url(EXAMPLE_URL);
    QNetworkCacheMetaData metaData;
    metaData.setUrl(url);
    QNetworkCacheMetaData::RawHeaderList headers;
    headers.append(QNetworkCacheMetaData::RawHeader("type", "bin"));
    metaData.setRawHeaders(headers);
    QTest::newRow("non-null") << metaData;
}

// public QIODevice* data(QUrl const& url)
void tst_QNetworkDiskCache::data()
{
    QFETCH(QNetworkCacheMetaData, data);
    SubQNetworkDiskCache cache;
    QUrl url(EXAMPLE_URL);
    cache.setupWithOne(tempDir.path(), url, data);

    for (int i = 0; i < 3; ++i) {
        QIODevice *d = cache.data(url);
        QVERIFY(d);
        QCOMPARE(d->readAll(), QByteArray("Hello World!"));
        delete d;
    }
}

// public QNetworkCacheMetaData metaData(QUrl const& url)
void tst_QNetworkDiskCache::metaData()
{
    SubQNetworkDiskCache cache;

    QUrl url(EXAMPLE_URL);
    QNetworkCacheMetaData metaData;
    metaData.setUrl(url);
    QNetworkCacheMetaData::RawHeaderList headers;
    headers.append(QNetworkCacheMetaData::RawHeader("type", "bin"));
    metaData.setRawHeaders(headers);
    metaData.setLastModified(QDateTime::currentDateTime());
    metaData.setExpirationDate(QDateTime::currentDateTime());
    metaData.setSaveToDisk(true);

    cache.setupWithOne(tempDir.path(), url, metaData);

    for (int i = 0; i < 3; ++i) {
        QNetworkCacheMetaData cacheMetaData = cache.metaData(url);
        QVERIFY(cacheMetaData.isValid());
        QCOMPARE(metaData, cacheMetaData);
    }
}

// public bool remove(QUrl const& url)
void tst_QNetworkDiskCache::remove()
{
    SubQNetworkDiskCache cache;
    QUrl url(EXAMPLE_URL);
    cache.setupWithOne(tempDir.path(), url);
    QString cacheDirectory = cache.cacheDirectory();
    QCOMPARE(countFiles(cacheDirectory).count(), NUM_SUBDIRECTORIES + 3);
    cache.remove(url);
    QCOMPARE(countFiles(cacheDirectory).count(), NUM_SUBDIRECTORIES + 2);
}

void tst_QNetworkDiskCache::accessAfterRemove() // QTBUG-17400
{
    QByteArray data("HTTP/1.1 200 OK\r\n"
                    "Content-Length: 1\r\n"
                    "\r\n"
                    "a");

    MiniHttpServer server(data);

    QNetworkAccessManager *manager = new QNetworkAccessManager();
    SubQNetworkDiskCache subCache;
    subCache.setCacheDirectory(QLatin1String("cacheDir"));
    diskCache = &subCache;
    manager->setCache(&subCache);

    url = QUrl("http://127.0.0.1:" + QString::number(server.serverPort()));
    QNetworkRequest request(url);

    QNetworkReply *reply = manager->get(request);
    connect(reply, SIGNAL(readyRead()), this, SLOT(accessAfterRemoveReadyReadSlot()));
    connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));

    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());

    reply->deleteLater();
    manager->deleteLater();
}

void tst_QNetworkDiskCache::accessAfterRemoveReadyReadSlot()
{
    diskCache->remove(url); // this used to cause a crash later on
}

void tst_QNetworkDiskCache::setCookieHeader() // QTBUG-41514
{
    SubQNetworkDiskCache *cache = new SubQNetworkDiskCache();
    url = QUrl("http://localhost:4/cookieTest.html");   // hopefully no one is running an HTTP server on port 4
    QNetworkCacheMetaData metaData;
    metaData.setUrl(url);

    QNetworkCacheMetaData::RawHeaderList headers;
    headers.append(QNetworkCacheMetaData::RawHeader("Set-Cookie", "aaa=bbb"));
    metaData.setRawHeaders(headers);
    metaData.setSaveToDisk(true);
    cache->setupWithOne(tempDir.path(), url, metaData);

    manager = new QNetworkAccessManager();
    manager->setCache(cache);

    QNetworkRequest request(url);
    QNetworkReply  *reply = manager->get(request);
    connect(reply, SIGNAL(metaDataChanged()), this, SLOT(setCookieHeaderMetaDataChangedSlot()));
    connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));

    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());

    reply->deleteLater();
    manager->deleteLater();
}

void tst_QNetworkDiskCache::setCookieHeaderMetaDataChangedSlot()
{
    QList<QNetworkCookie> actualCookieJar = manager->cookieJar()->cookiesForUrl(url);
    QVERIFY(!actualCookieJar.empty());
}

void tst_QNetworkDiskCache::setCacheDirectory_data()
{
    QTest::addColumn<QString>("cacheDir");
    QTest::newRow("null") << QString();
    QDir dir("foo");
    QTest::newRow("foo") << dir.absolutePath() + QString("/");
}

// public void setCacheDirectory(QString const& cacheDir)
void tst_QNetworkDiskCache::setCacheDirectory()
{
    QFETCH(QString, cacheDir);

    SubQNetworkDiskCache cache;
    cache.setCacheDirectory(cacheDir);
    QCOMPARE(cache.cacheDirectory(), cacheDir);
}

// public void updateMetaData(QNetworkCacheMetaData const& metaData)
void tst_QNetworkDiskCache::updateMetaData()
{
    QUrl url(EXAMPLE_URL);
    SubQNetworkDiskCache cache;
    cache.setupWithOne(tempDir.path(), url);

    QNetworkCacheMetaData metaData = cache.metaData(url);
    metaData.setLastModified(QDateTime::currentDateTime());
    cache.updateMetaData(metaData);
    QNetworkCacheMetaData newMetaData = cache.metaData(url);
    QCOMPARE(newMetaData, metaData);
}

// protected QNetworkCacheMetaData fileMetaData(QString const& fileName)
void tst_QNetworkDiskCache::fileMetaData()
{
    SubQNetworkDiskCache cache;
    QUrl url(EXAMPLE_URL);
    cache.setupWithOne(tempDir.path(), url);

    url.setPassword(QString());
    url.setFragment(QString());

    QString cacheDirectory = cache.cacheDirectory();
    QStringList list = countFiles(cacheDirectory);
    QCOMPARE(list.count(), NUM_SUBDIRECTORIES + 3);
    foreach(QString fileName, list) {
        QFileInfo info(fileName);
        if (info.isFile()) {
            QNetworkCacheMetaData metaData = cache.call_fileMetaData(fileName);
            QCOMPARE(metaData.url(), url);
        }
    }

    QTemporaryFile file(cacheDirectory + "/qt_temp.XXXXXX");
    if (file.open()) {
        QNetworkCacheMetaData metaData = cache.call_fileMetaData(file.fileName());
        QVERIFY(!metaData.isValid());
    }
}

// protected qint64 expire()
void tst_QNetworkDiskCache::expire()
{
    SubQNetworkDiskCache cache;
    cache.setCacheDirectory(tempDir.path());
    QCOMPARE(cache.call_expire(), (qint64)0);
    QUrl url(EXAMPLE_URL);
    cache.setupWithOne(tempDir.path(), url);
    QVERIFY(cache.call_expire() > (qint64)0);
    qint64 limit = (1024 * 1024 / 4) * 5;
    cache.setMaximumCacheSize(limit);

    qint64 max = cache.maximumCacheSize();
    QCOMPARE(max, limit);
    for (int i = 0; i < 10; ++i) {
        if (i % 3 == 0)
            QTest::qWait(2000);
        QNetworkCacheMetaData m;
        m.setUrl(QUrl("http://localhost:4/" + QString::number(i)));
        QIODevice *d = cache.prepare(m);
        QString bigString;
        bigString.fill(QLatin1Char('Z'), (1024 * 1024 / 4));
        d->write(bigString.toLatin1().data());
        cache.insert(d);
        QVERIFY(cache.call_expire() < max);
    }

    QString cacheDirectory = cache.cacheDirectory();
    QStringList list = countFiles(cacheDirectory);
    QStringList cacheList;
    foreach(QString fileName, list) {
        QFileInfo info(fileName);
        if (info.isFile()) {
            QNetworkCacheMetaData metaData = cache.call_fileMetaData(fileName);
            cacheList.append(metaData.url().toString());
        }
    }
    std::sort(cacheList.begin(), cacheList.end());
    for (int i = 0; i < cacheList.count(); ++i) {
        QString fileName = cacheList[i];
        QCOMPARE(fileName, QLatin1String("http://localhost:4/") + QString::number(i + 6));
    }
}

void tst_QNetworkDiskCache::oldCacheVersionFile_data()
{
    QTest::addColumn<int>("pass");
    QTest::newRow("0") << 0;
    QTest::newRow("1") << 1;
}

void tst_QNetworkDiskCache::oldCacheVersionFile()
{
    QFETCH(int, pass);
    SubQNetworkDiskCache cache;
    QUrl url(EXAMPLE_URL);
    cache.setupWithOne(tempDir.path(), url);

    if (pass == 0) {
        QString name;
        {
        QTemporaryFile file(cache.cacheDirectory() + "/XXXXXX.d");
        file.setAutoRemove(false);
        QVERIFY2(file.open(), qPrintable(file.errorString()));
        QDataStream out(&file);
        out << qint32(0xe8);
        out << qint32(2);
        name = file.fileName();
        file.close();
        }

        QVERIFY(QFile::exists(name));
        QNetworkCacheMetaData metaData = cache.call_fileMetaData(name);
        QVERIFY(!metaData.isValid());
        QVERIFY(!QFile::exists(name));
    } else {
        QStringList files = countFiles(cache.cacheDirectory());
        QCOMPARE(files.count(), NUM_SUBDIRECTORIES + 3);
        // find the file
        QString cacheFile;
        foreach (QString file, files) {
            QFileInfo info(file);
            if (info.isFile())
                cacheFile = file;
        }
        QVERIFY(QFile::exists(cacheFile));

        QFile file(cacheFile);
        QVERIFY(file.open(QFile::ReadWrite));
        QDataStream out(&file);
        out << qint32(0xe8);
        out << qint32(2);
        file.close();

        QIODevice *device = cache.data(url);
        QVERIFY(!device);
        QVERIFY(!QFile::exists(cacheFile));
    }
}

void tst_QNetworkDiskCache::streamVersion_data()
{
    QTest::addColumn<int>("version");
    QTest::newRow("Qt 5.1") << int(QDataStream::Qt_5_1);
    QDataStream ds;
    QTest::newRow("current") << ds.version();
    QTest::newRow("higher than current") << ds.version() + 1;
}

void tst_QNetworkDiskCache::streamVersion()
{
    SubQNetworkDiskCache cache;
    QUrl url(EXAMPLE_URL);
    cache.setupWithOne(tempDir.path(), url);

    QString cacheFile;
    // find the file
    QStringList files = countFiles(cache.cacheDirectory());
    foreach (const QString &file, files) {
        QFileInfo info(file);
        if (info.isFile()) {
            cacheFile = file;
            break;
        }
    }

    QFile file(cacheFile);
    QVERIFY(file.open(QFile::ReadWrite|QIODevice::Truncate));
    QDataStream out(&file);
    QFETCH(int, version);
    if (version < out.version())
        out.setVersion(version);
    out << qint32(0xe8);    // cache magic
    // Following code works only for cache file version 8 and should be updated on version change
    out << qint32(8);
    out << qint32(version);

    QNetworkCacheMetaData md;
    md.setUrl(url);
    QNetworkCacheMetaData::RawHeader header("content-type", "text/html");
    QNetworkCacheMetaData::RawHeaderList list;
    list.append(header);
    md.setRawHeaders(list);
    md.setLastModified(QDateTime::currentDateTimeUtc().toOffsetFromUtc(3600));
    out << md;

    bool compressed = true;
    out << compressed;

    QByteArray data("Hello World!");
    out << qCompress(data);

    file.close();

    QNetworkCacheMetaData cachedMetaData = cache.call_fileMetaData(cacheFile);
    if (version > out.version()) {
        QVERIFY(!cachedMetaData.isValid());
        QVERIFY(!QFile::exists(cacheFile));
    } else {
        QVERIFY(cachedMetaData.isValid());
        QVERIFY(QFile::exists(cacheFile));
        QIODevice *dataDevice = cache.data(url);
        QVERIFY(dataDevice != 0);
        QByteArray cachedData = dataDevice->readAll();
        QCOMPARE(cachedData, data);
    }
}

class Runner : public QThread
{

public:
    Runner(const QString& cachePath)
        : QThread()
        , other(0)
        , cachePath(cachePath)
    {}

    void run()
    {
        QByteArray longString = "Hello World, this is some long string, well not really that long";
        for (int j = 0; j < 10; ++j)
            longString += longString;
        QByteArray longString2 = "Help, I am stuck in an autotest!";
        QUrl url(EXAMPLE_URL);

        QNetworkCacheMetaData metaData;
        metaData.setUrl(url);
        QNetworkCacheMetaData::RawHeaderList headers;
        headers.append(QNetworkCacheMetaData::RawHeader("type", "bin"));
        metaData.setRawHeaders(headers);
        metaData.setLastModified(dt);
        metaData.setSaveToDisk(true);

        QNetworkCacheMetaData metaData2 = metaData;
        metaData2.setExpirationDate(dt);

        QNetworkDiskCache cache;
        cache.setCacheDirectory(cachePath);

        int read = 0;

        int i = 0;
        for (; i < 5000; ++i) {
            if (other && other->isFinished())
                break;

            if (write) {
                QNetworkCacheMetaData m;
                if (qrand() % 2 == 0)
                    m = metaData;
                else
                    m = metaData2;

                if (qrand() % 20 == 1) {
                    //qDebug() << "write update";
                    cache.updateMetaData(m);
                    continue;
                }

                QIODevice *device = cache.prepare(m);
                if (qrand() % 20 == 1) {
                    //qDebug() << "write remove";
                    cache.remove(url);
                    continue;
                }
                QVERIFY(device);
                if (qrand() % 2 == 0)
                    device->write(longString);
                else
                    device->write(longString2);
                //qDebug() << "write write" << device->size();
                cache.insert(device);
                continue;
            }

            QNetworkCacheMetaData gotMetaData = cache.metaData(url);
            if (gotMetaData.isValid()) {
                QVERIFY(gotMetaData == metaData || gotMetaData == metaData2);
                QIODevice *d = cache.data(url);
                if (d) {
                    QByteArray x = d->readAll();
                    if (x != longString && x != longString2) {
                        qDebug() << x.length() << QString(x);
                        gotMetaData = cache.metaData(url);
                        qDebug() << (gotMetaData.url().toString())
                         << gotMetaData.lastModified()
                         << gotMetaData.expirationDate()
                         << gotMetaData.saveToDisk();
                    }
                    if (gotMetaData.isValid())
                        QVERIFY(x == longString || x == longString2);
                    read++;
                    delete d;
                }
            }
            if (qrand() % 5 == 1)
                cache.remove(url);
            if (qrand() % 5 == 1)
                cache.clear();
            sleep(0);
        }
        //qDebug() << "read!" << read << i;
    }

    QDateTime dt;
    bool write;
    Runner *other;
    QString cachePath;
};

void tst_QNetworkDiskCache::crashWhenParentingCache()
{
    // the trick here is to not send the complete response
    // but some data. So we get a readyRead() and it gets tried
    // to be saved to the cache
    QByteArray data("HTTP/1.0 200 OK\r\nCache-Control: max-age=300\r\nAge: 1\r\nContent-Length: 5\r\n\r\n123");
    MiniHttpServer server(data);

    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkDiskCache *diskCache = new QNetworkDiskCache(manager); // parent to qnam!
    // we expect the temp dir to be cleaned at some point anyway

    const QString diskCachePath = QDir::tempPath() + QLatin1String("/cacheDir_")
        + QString::number(QCoreApplication::applicationPid());
    diskCache->setCacheDirectory(diskCachePath);
    manager->setCache(diskCache);

    QUrl url("http://127.0.0.1:" + QString::number(server.serverPort()));
    QNetworkRequest request(url);
   // request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    QNetworkReply *reply = manager->get(request); // new reply is parented to qnam

    // wait for readyRead of reply!
    connect(reply, SIGNAL(readyRead()), &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());

    delete manager; // crashed before..
}

void tst_QNetworkDiskCache::sync()
{
    // This tests would be a nice to have, but is currently not supported.
    return;

    QTime midnight(0, 0, 0);
    qsrand(midnight.secsTo(QTime::currentTime()));
    Runner reader(tempDir.path());
    reader.dt = QDateTime::currentDateTime();
    reader.write = false;

    Runner writer(tempDir.path());
    writer.dt = reader.dt;
    writer.write = true;

    writer.other = &reader;
    reader.other = &writer;

    writer.start();
    reader.start();
    writer.wait();
    reader.wait();
}

QTEST_MAIN(tst_QNetworkDiskCache)
#include "tst_qnetworkdiskcache.moc"

