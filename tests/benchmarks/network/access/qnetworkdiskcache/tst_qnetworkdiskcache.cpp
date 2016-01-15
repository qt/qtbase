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

#include <QNetworkDiskCache>
#include <QNetworkCacheMetaData>
#include <QDir>
#include <QBuffer>
#include <QTextStream>
#include <QDebug>
#include <QtTest/QtTest>
#include <QIODevice>
#include <QStandardPaths>



enum Numbers { NumFakeCacheObjects   = 200,    //entries in pre-populated cache
               NumInsertions  = 100,           //insertions to be timed
               NumRemovals    = 100,           //removals to be timed
               NumReadContent = 100,           //meta requests to be timed
               HugeCacheLimit = 50*1024*1024,  // max size for a big cache
               TinyCacheLimit = 1*512*1024}; //  max size for a tiny cache

const QString fakeURLbase = "http://127.0.0.1/fake/";
//fake HTTP body aka payload
const QByteArray payload("Qt rocks!");

class tst_qnetworkdiskcache : public QObject
{
    Q_OBJECT
private:
    void injectFakeData();
    void insertOneItem();
    bool isUrlCached(quint32 id);
    void cleanRecursive(QString &path);
    void cleanupCacheObject();
    void initCacheObject();
    QString cacheDir;
    QNetworkDiskCache *cache;

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:

    void timeInsertion_data();
    void timeInsertion();
    void timeRead_data();
    void timeRead();
    void timeRemoval_data();
    void timeRemoval();

    void timeExpiration_data();
    void timeExpiration();
};


void tst_qnetworkdiskcache::initTestCase()
{
    cache = 0;
}


void tst_qnetworkdiskcache::cleanupTestCase()
{
    cleanupCacheObject();
    cleanRecursive(cacheDir);
}

void tst_qnetworkdiskcache::timeInsertion_data()
{
    QTest::addColumn<QString>("cacheRootDirectory");

    QString cacheLoc = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QTest::newRow("QStandardPaths Cache Location") << cacheLoc;
}

//This functions times an insert() operation.
//You can run it after populating the cache with
//fake data so that more realistic performance
//estimates are obtained.
void tst_qnetworkdiskcache::timeInsertion()
{

    QFETCH(QString, cacheRootDirectory);

    cacheDir = QString( cacheRootDirectory + QDir::separator() + "man_qndc");
    QDir d;
    qDebug() << "Setting cache directory to = " << d.absoluteFilePath(cacheDir);

    //Housekeeping
    cleanRecursive(cacheDir); // slow op.
    initCacheObject();

    cache->setCacheDirectory(cacheDir);
    cache->setMaximumCacheSize(qint64(HugeCacheLimit));
    cache->clear();

    //populate some fake data to simulate partially full cache
    injectFakeData(); // SLOW

    //Sanity-check that the first URL that we insert below isn't already in there.
    QVERIFY(isUrlCached(NumFakeCacheObjects) == false);

    // IMPORTANT: max cache size should be HugeCacheLimit, to avoid evictions below
    //time insertion of previously-uncached URLs.
    QBENCHMARK_ONCE {
        for (quint32 i = NumFakeCacheObjects; i < (NumFakeCacheObjects + NumInsertions); i++) {
            //prepare metata for url
            QNetworkCacheMetaData meta;
            QString fakeURL;
            QTextStream stream(&fakeURL);
            stream << fakeURLbase << i;
            QUrl url(fakeURL);
            meta.setUrl(url);
            meta.setSaveToDisk(true);

            //commit payload and metadata to disk
            QIODevice *device = cache->prepare(meta);
            device->write(payload);
            cache->insert(device);
        }
    }

    //SLOW cleanup
    cleanupCacheObject();
    cleanRecursive(cacheDir);

}

void tst_qnetworkdiskcache::timeRead_data()
{
    QTest::addColumn<QString>("cacheRootDirectory");

    QString cacheLoc = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QTest::newRow("QStandardPaths Cache Location") << cacheLoc;
}

//Times metadata as well payload lookup
// i.e metaData(), rawHeaders() and data()
void tst_qnetworkdiskcache::timeRead()
{

    QFETCH(QString, cacheRootDirectory);

    cacheDir = QString( cacheRootDirectory + QDir::separator() + "man_qndc");
    QDir d;
    qDebug() << "Setting cache directory to = " << d.absoluteFilePath(cacheDir);

    //Housekeeping
    cleanRecursive(cacheDir); // slow op.
    initCacheObject();
    cache->setCacheDirectory(cacheDir);
    cache->setMaximumCacheSize(qint64(HugeCacheLimit));
    cache->clear();

    //populate some fake data to simulate partially full cache
    injectFakeData();

    //Entries in the cache should be > what we try to remove
    QVERIFY(NumFakeCacheObjects > NumReadContent);

    //time metadata lookup of previously inserted URL.
    QBENCHMARK_ONCE {
        for (quint32 i = 0; i < NumReadContent; i++) {
            QString fakeURL;
            QTextStream stream(&fakeURL);
            stream << fakeURLbase << i;
            QUrl url(fakeURL);

            QNetworkCacheMetaData qndc = cache->metaData(url);
            QVERIFY(qndc.isValid()); // we must have read the metadata

            QNetworkCacheMetaData::RawHeaderList raw(qndc.rawHeaders());
            QVERIFY(raw.size()); // we must have parsed the headers from the meta

            QIODevice *iodevice(cache->data(url));
            QVERIFY(iodevice);    //must not be NULL
            iodevice->close();
            delete iodevice;
        }
    }

    //Cleanup (slow)
    cleanupCacheObject();
    cleanRecursive(cacheDir);

}

void tst_qnetworkdiskcache::timeRemoval_data()
{
    QTest::addColumn<QString>("cacheRootDirectory");

    QString cacheLoc = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QTest::newRow("QStandardPaths Cache Location") << cacheLoc;
}

void tst_qnetworkdiskcache::timeRemoval()
{

    QFETCH(QString, cacheRootDirectory);

    cacheDir = QString( cacheRootDirectory + QDir::separator() + "man_qndc");
    QDir d;
    qDebug() << "Setting cache directory to = " << d.absoluteFilePath(cacheDir);

    //Housekeeping
    initCacheObject();
    cleanRecursive(cacheDir); // slow op.
    cache->setCacheDirectory(cacheDir);
    // Make max cache size HUGE, so that evictions don't happen below
    cache->setMaximumCacheSize(qint64(HugeCacheLimit));
    cache->clear();

    //populate some fake data to simulate partially full cache
    injectFakeData();

    //Sanity-check that the URL is already in there somewhere
    QVERIFY(isUrlCached(NumRemovals-1) == true);
    //Entries in the cache should be > what we try to remove
    QVERIFY(NumFakeCacheObjects > NumRemovals);

    //time removal of previously-inserted URL.
    QBENCHMARK_ONCE {
        for (quint32 i = 0; i < NumRemovals; i++) {
            QString fakeURL;
            QTextStream stream(&fakeURL);
            stream << fakeURLbase << i;
            QUrl url(fakeURL);
            cache->remove(url);
        }
    }

    //Cleanup (slow)
    cleanupCacheObject();
    cleanRecursive(cacheDir);

}

void tst_qnetworkdiskcache::timeExpiration_data()
{
    QTest::addColumn<QString>("cacheRootDirectory");

    QString cacheLoc = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QTest::newRow("QStandardPaths Cache Location") << cacheLoc;
}

void tst_qnetworkdiskcache::timeExpiration()
{

    QFETCH(QString, cacheRootDirectory);

    cacheDir = QString( cacheRootDirectory + QDir::separator() + "man_qndc");
    QDir d;
    qDebug() << "Setting cache directory to = " << d.absoluteFilePath(cacheDir);

    //Housekeeping
    initCacheObject();
    cleanRecursive(cacheDir); // slow op.
    cache->setCacheDirectory(cacheDir);
    // Make max cache size HUGE, so that evictions don't happen below
    cache->setMaximumCacheSize(qint64(HugeCacheLimit));
    cache->clear();

    //populate some fake data to simulate partially full cache
    injectFakeData();

    //Sanity-check that the URL is already in there somewhere
    QVERIFY(isUrlCached(NumRemovals-1) == true);
    //Entries in the cache should be > what we try to remove
    QVERIFY(NumFakeCacheObjects > NumRemovals);


    //Set cache limit lower, so this force 1 round of eviction
    cache->setMaximumCacheSize(qint64(TinyCacheLimit));

    //time insertions of additional content, which is likely to internally cause evictions
    QBENCHMARK_ONCE {
        for (quint32 i = NumFakeCacheObjects; i < (NumFakeCacheObjects + NumInsertions); i++) {
            //prepare metata for url
            QNetworkCacheMetaData meta;
            QString fakeURL;
            QTextStream stream(&fakeURL);
            stream << fakeURLbase << i;//codescanner::leave
            QUrl url(fakeURL);
            meta.setUrl(url);
            meta.setSaveToDisk(true);

            //commit payload and metadata to disk
            QIODevice *device = cache->prepare(meta);
            device->write(payload);
            cache->insert(device); // this should trigger evictions, if TinyCacheLimit is small enough
        }
    }

    //Cleanup (slow)
    cleanupCacheObject();
    cleanRecursive(cacheDir);

}
// This function simulates a partially or fully occupied disk cache
// like a normal user of a cache might encounter is real-life browsing.
// The point of this is to trigger degradation in file-system and media performance
// that occur due to the quantity and layout of data.
void tst_qnetworkdiskcache::injectFakeData()
{

    QNetworkCacheMetaData::RawHeaderList headers;
    headers.append(qMakePair(QByteArray("X-TestHeader"),QByteArray("HeaderValue")));


    //Prep cache dir with fake data using QNetworkDiskCache APIs
    for (quint32 i = 0; i < NumFakeCacheObjects; i++) {

        //prepare metata for url
        QNetworkCacheMetaData meta;
        QString fakeURL;
        QTextStream stream(&fakeURL);
        stream << fakeURLbase << i;
        QUrl url(fakeURL);
        meta.setUrl(url);
        meta.setRawHeaders(headers);
        meta.setSaveToDisk(true);

        //commit payload and metadata to disk
        QIODevice *device = cache->prepare(meta);
        device->write(payload);
        cache->insert(device);
    }

}


// Checks if the fake URL #id is already cached or not.
bool tst_qnetworkdiskcache::isUrlCached(quint32 id)
{
    QString str;
    QTextStream stream(&str);
    stream << fakeURLbase << id;
    QUrl url(str);
    QIODevice *iod = cache->data(url);
    return ((iod == 0) ? false : true) ;

}


// Utility function for recursive directory cleanup.
void tst_qnetworkdiskcache::cleanRecursive(QString &path)
{
    QDirIterator it(path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFile f(it.next());
        bool err = f.remove();
        Q_UNUSED(err);
    }

    QDirIterator it2(path, QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it2.hasNext()) {
        QString s(it2.next());
        QDir dir(s);
        dir.rmdir(s);
    }
}

void tst_qnetworkdiskcache::cleanupCacheObject()
{
    delete cache;
    cache = 0;
}

void tst_qnetworkdiskcache::initCacheObject()
{

   cache = new QNetworkDiskCache();

}
QTEST_MAIN(tst_qnetworkdiskcache)
#include "tst_qnetworkdiskcache.moc"
