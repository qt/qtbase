// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>


#include <qpixmapcache.h>
#include "private/qpixmapcache_p.h"

#include <functional>

QT_BEGIN_NAMESPACE // The test requires QT_BUILD_INTERNAL
Q_AUTOTEST_EXPORT void qt_qpixmapcache_flush_detached_pixmaps();
Q_AUTOTEST_EXPORT int qt_qpixmapcache_qpixmapcache_total_used();
Q_AUTOTEST_EXPORT int q_QPixmapCache_keyHashSize();
QT_END_NAMESPACE

using namespace Qt::StringLiterals;

class tst_QPixmapCache : public QObject
{
    Q_OBJECT

public:
    tst_QPixmapCache();
    virtual ~tst_QPixmapCache();


public slots:
    void init();
private slots:
    void cacheLimit();
    void setCacheLimit();
    void find();
    void insert();
    void failedInsertReturnsInvalidKey();
#if QT_DEPRECATED_SINCE(6, 6)
    void replace();
#endif
    void remove();
    void clear();
    void pixmapKey();
    void noLeak();
    void clearDoesNotLeakStringKeys();
    void evictionDoesNotLeakStringKeys();
    void reducingCacheLimitDoesNotLeakStringKeys();
    void strictCacheLimit();
    void noCrashOnLargeInsert();

private:
    void stringLeak_impl(std::function<void()> whenOp);
};

static QPixmapCache::KeyData* getPrivate(QPixmapCache::Key &key)
{
    return (*reinterpret_cast<QPixmapCache::KeyData**>(&key));
}

static QPixmapCache::KeyData** getPrivateRef(QPixmapCache::Key &key)
{
    return (reinterpret_cast<QPixmapCache::KeyData**>(&key));
}

static int originalCacheLimit;

tst_QPixmapCache::tst_QPixmapCache()
{
    originalCacheLimit = QPixmapCache::cacheLimit();
}

tst_QPixmapCache::~tst_QPixmapCache()
{
}

void tst_QPixmapCache::init()
{
    QPixmapCache::setCacheLimit(originalCacheLimit);
    QPixmapCache::clear();
}

void tst_QPixmapCache::cacheLimit()
{
    // make sure the default is reasonable;
    // it was between 2048 and 10240 last time I looked at it
    QVERIFY(originalCacheLimit >= 1024 && originalCacheLimit <= 20480);

    QPixmapCache::setCacheLimit(std::numeric_limits<int>::max());
    QCOMPARE(QPixmapCache::cacheLimit(), std::numeric_limits<int>::max());

    QPixmapCache::setCacheLimit(100);
    QCOMPARE(QPixmapCache::cacheLimit(), 100);

    QPixmapCache::setCacheLimit(-50);
    QCOMPARE(QPixmapCache::cacheLimit(), -50);
}

void tst_QPixmapCache::setCacheLimit()
{
    QPixmap res;
    QPixmap *p1 = new QPixmap(2, 3);
    QPixmapCache::insert("P1", *p1);
    QVERIFY(QPixmapCache::find("P1", &res));
    delete p1;

    QPixmapCache::setCacheLimit(0);
    QVERIFY(!QPixmapCache::find("P1", &res));

    p1 = new QPixmap(2, 3);
    QPixmapCache::setCacheLimit(1000);
    QPixmapCache::insert("P1", *p1);
    QVERIFY(QPixmapCache::find("P1", &res));

    delete p1;

    //The int part of the API
    p1 = new QPixmap(2, 3);
    QPixmapCache::Key key = QPixmapCache::insert(*p1);
    QVERIFY(QPixmapCache::find(key, p1));
    delete p1;

    QPixmapCache::setCacheLimit(0);
    QVERIFY(!QPixmapCache::find(key, p1));

    QPixmapCache::setCacheLimit(1000);
#if QT_DEPRECATED_SINCE(6, 6)
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    p1 = new QPixmap(2, 3);
    QVERIFY(!QPixmapCache::replace(key, *p1));
    QVERIFY(!QPixmapCache::find(key, p1));

    delete p1;
#endif // QT_DEPRECATED_SINCE(6, 6)

    //Let check if keys are released when the pixmap cache is
    //full or has been flushed.
    QPixmapCache::clear();
    p1 = new QPixmap(2, 3);
    key = QPixmapCache::insert(*p1);
    QVERIFY(QPixmapCache::find(key, p1));
    p1->detach(); // dectach so that the cache thinks no-one is using it.
    QPixmapCache::setCacheLimit(0);
    QVERIFY(!QPixmapCache::find(key, p1));
    QPixmapCache::setCacheLimit(1000);
    key = QPixmapCache::insert(*p1);
    QVERIFY(key.isValid());
    QCOMPARE(getPrivate(key)->key, 1);

    delete p1;

    //Let check if removing old entries doesn't let you get
    // wrong pixmaps
    QPixmapCache::clear();
    QPixmap p2;
    p1 = new QPixmap(2, 3);
    key = QPixmapCache::insert(*p1);
    QVERIFY(QPixmapCache::find(key, &p2));
    //we flush the cache
    p1->detach();
    p2.detach();
    QPixmapCache::setCacheLimit(0);
    QPixmapCache::setCacheLimit(1000);
    QPixmapCache::Key key2 = QPixmapCache::insert(*p1);
    QCOMPARE(getPrivate(key2)->key, 1);
    QVERIFY(!QPixmapCache::find(key, &p2));
    QVERIFY(QPixmapCache::find(key2, &p2));
    QCOMPARE(p2, *p1);

    delete p1;

    //Here we simulate the flushing when the app is idle
    QPixmapCache::clear();
    QPixmapCache::setCacheLimit(originalCacheLimit);
    p1 = new QPixmap(300, 300);
    key = QPixmapCache::insert(*p1);
    p1->detach();
    QCOMPARE(getPrivate(key)->key, 1);
    key2 = QPixmapCache::insert(*p1);
    p1->detach();
    key2 = QPixmapCache::insert(*p1);
    p1->detach();
    QPixmapCache::Key key3 = QPixmapCache::insert(*p1);
    p1->detach();
    qt_qpixmapcache_flush_detached_pixmaps();
    key2 = QPixmapCache::insert(*p1);
    QCOMPARE(getPrivate(key2)->key, 1);
    //This old key is not valid anymore after the flush
    QVERIFY(!key.isValid());
    QVERIFY(!QPixmapCache::find(key, &p2));
    delete p1;
}

void tst_QPixmapCache::find()
{
    QPixmap p1(10, 10);
    p1.fill(Qt::red);
    QVERIFY(QPixmapCache::insert("P1", p1));

    QPixmap p2;

    QVERIFY(QPixmapCache::find("P1", &p2));
    QCOMPARE(p2.width(), 10);
    QCOMPARE(p2.height(), 10);
    QCOMPARE(p1, p2);

    //The int part of the API
    QPixmapCache::Key key = QPixmapCache::insert(p1);

    QVERIFY(QPixmapCache::find(key, &p2));
    QCOMPARE(p2.width(), 10);
    QCOMPARE(p2.height(), 10);
    QCOMPARE(p1, p2);

    QPixmapCache::clear();
    QPixmapCache::setCacheLimit(128);

    QPixmap p4(10,10);
    key = QPixmapCache::insert(p4);
    p4.detach();

    QPixmap p5(10,10);
    QList<QPixmapCache::Key> keys;
    for (int i = 0; i < 4000; ++i)
        QPixmapCache::insert(p5);

    //at that time the first key has been erase because no more place in the cache
    QVERIFY(!QPixmapCache::find(key, &p1));
    QVERIFY(!key.isValid());
}

void tst_QPixmapCache::insert()
{
    QPixmap p1(10, 10);
    p1.fill(Qt::red);

    QPixmap p2(10, 10);
    p2.fill(Qt::yellow);

    // Calcuate estimated num of items what fits to cache
    int estimatedNum = (1024 * QPixmapCache::cacheLimit())
                       / ((p1.width() * p1.height() * p1.depth()) / 8);

    // Mare sure we will put enough items to reach the cache limit
    const int numberOfKeys = estimatedNum + 1000;

    // make sure it doesn't explode
    for (int i = 0; i < numberOfKeys; ++i)
        QPixmapCache::insert("0", p1);

    // ditto
    for (int j = 0; j < numberOfKeys; ++j) {
        QPixmap p3(10, 10);
        QPixmapCache::insert(QString::number(j), p3);
    }

    int num = 0;
    QPixmap res;
    for (int k = 0; k < numberOfKeys; ++k) {
        if (QPixmapCache::find(QString::number(k), &res))
            ++num;
    }

    if (QPixmapCache::find("0", &res))
        ++num;

    QVERIFY(num <= estimatedNum);
    QPixmap p3;
    QPixmapCache::insert("null", p3);

    QPixmap c1(10, 10);
    c1.fill(Qt::yellow);
    QPixmapCache::insert("custom", c1);
    QVERIFY(!c1.isDetached());
    QPixmap c2(10, 10);
    c2.fill(Qt::red);
    QPixmapCache::insert("custom", c2);
    //We have deleted the old pixmap in the cache for the same key
    QVERIFY(c1.isDetached());

    //The int part of the API
    // make sure it doesn't explode
    QList<QPixmapCache::Key> keys;
    for (int i = 0; i < numberOfKeys; ++i) {
        QPixmap p3(10,10);
        keys.append(QPixmapCache::insert(p3));
        QVERIFY(keys.back().isValid());
    }

    num = 0;
    for (int k = 0; k < numberOfKeys; ++k) {
        if (QPixmapCache::find(keys.at(k), &p2))
            ++num;
    }

    estimatedNum = (1024 * QPixmapCache::cacheLimit())
                       / ((p1.width() * p1.height() * p1.depth()) / 8);
    QVERIFY(num <= estimatedNum);
}

void tst_QPixmapCache::failedInsertReturnsInvalidKey()
{
    //
    // GIVEN: a pixmap whose memory footprint exceeds the cache's limit:
    //
    QPixmapCache::setCacheLimit(20);

    QPixmap pm(256, 256);
    pm.fill(Qt::transparent);
    QCOMPARE_GT(pm.width() * pm.height() * pm.depth() / 8,
                QPixmapCache::cacheLimit() * 1024);

    //
    // WHEN: trying to add this pixmap to the cache
    //
    const auto success = QPixmapCache::insert(u"foo"_s, pm); // QString API
    { QPixmap r; QVERIFY(!QPixmapCache::find(u"foo"_s, &r)); }
    const auto key = QPixmapCache::insert(pm);               // "int" API

    //
    // THEN: failure is reported to the user
    //
    QVERIFY(!key.isValid()); // "int" API
    QVERIFY(!success);       // QString API
}

#if QT_DEPRECATED_SINCE(6, 6)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
void tst_QPixmapCache::replace()
{
    //The int part of the API
    QPixmap p1(10, 10);
    p1.fill(Qt::red);

    QPixmap p2(10, 10);
    p2.fill(Qt::yellow);

    QPixmapCache::Key key = QPixmapCache::insert(p1);
    QVERIFY(key.isValid());

    QPixmap p3;
    QVERIFY(QPixmapCache::find(key, &p3) == 1);

    QPixmapCache::replace(key, p2);

    QVERIFY(QPixmapCache::find(key, &p3) == 1);
    QVERIFY(key.isValid());
    QCOMPARE(getPrivate(key)->key, 1);

    QCOMPARE(p3.width(), 10);
    QCOMPARE(p3.height(), 10);
    QCOMPARE(p3, p2);

    //Broken keys
    QCOMPARE(QPixmapCache::replace(QPixmapCache::Key(), p2), false);
}
QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 6)

void tst_QPixmapCache::remove()
{
    QPixmap p1(10, 10);
    p1.fill(Qt::red);

    QPixmapCache::insert("red", p1);
    p1.fill(Qt::yellow);

    QPixmap p2;
    QVERIFY(QPixmapCache::find("red", &p2));
    QVERIFY(p1.toImage() != p2.toImage());
    QVERIFY(p1.toImage() == p1.toImage()); // sanity check

    QPixmapCache::remove("red");
    QVERIFY(!QPixmapCache::find("red", &p2));
    QPixmapCache::remove("red");
    QVERIFY(!QPixmapCache::find("red", &p2));

    QPixmapCache::remove("green");
    QVERIFY(!QPixmapCache::find("green", &p2));

    //The int part of the API
    QPixmapCache::clear();
    p1.fill(Qt::red);
    QPixmapCache::Key key = QPixmapCache::insert(p1);
    p1.fill(Qt::yellow);

    QVERIFY(QPixmapCache::find(key, &p2));
    QVERIFY(p1.toImage() != p2.toImage());
    QVERIFY(p1.toImage() == p1.toImage()); // sanity check

    QPixmapCache::remove(key);
    QVERIFY(!QPixmapCache::find(key, &p1));

    //Broken key
    QPixmapCache::remove(QPixmapCache::Key());
    QVERIFY(!QPixmapCache::find(QPixmapCache::Key(), &p1));

    //Test if keys are release
    QPixmapCache::clear();
    key = QPixmapCache::insert(p1);
    QCOMPARE(getPrivate(key)->key, 1);
    QPixmapCache::remove(key);
    key = QPixmapCache::insert(p1);
    QCOMPARE(getPrivate(key)->key, 1);

    //Test if pixmaps are correctly deleted
    QPixmapCache::clear();
    key = QPixmapCache::insert(p1);
    QCOMPARE(getPrivate(key)->key, 1);
    QVERIFY(QPixmapCache::find(key, &p1));
    QPixmapCache::remove(key);
    QCOMPARE(p1.isDetached(), true);

    //We mix both part of the API
    QPixmapCache::clear();
    p1.fill(Qt::red);
    QPixmapCache::insert("red", p1);
    key = QPixmapCache::insert(p1);
    QPixmapCache::remove(key);
    QVERIFY(!QPixmapCache::find(key, &p1));
    QVERIFY(QPixmapCache::find("red", &p1));
}

void tst_QPixmapCache::clear()
{
    QPixmap p1(10, 10);
    p1.fill(Qt::red);

    // Calcuate estimated num of items what fits to cache
    int estimatedNum = (1024 * QPixmapCache::cacheLimit())
                       / ((p1.width() * p1.height() * p1.depth()) / 8);

    // Mare sure we will put enough items to reach the cache limit
    const int numberOfKeys = estimatedNum + 1000;

    for (int i = 0; i < numberOfKeys; ++i)
        QVERIFY(!QPixmapCache::find("x" + QString::number(i), &p1));

    for (int j = 0; j < numberOfKeys; ++j)
        QPixmapCache::insert(QString::number(j), p1);

    int num = 0;
    for (int k = 0; k < numberOfKeys; ++k) {
        if (QPixmapCache::find(QString::number(k), &p1))
            ++num;
    }
    QVERIFY(num > 0);

    QPixmapCache::clear();

    for (int k = 0; k < numberOfKeys; ++k)
        QVERIFY(!QPixmapCache::find(QString::number(k), &p1));

    //The int part of the API
    QPixmap p2(10, 10);
    p2.fill(Qt::red);

    QList<QPixmapCache::Key> keys;
    for (int k = 0; k < numberOfKeys; ++k)
        keys.append(QPixmapCache::insert(p2));

    QPixmapCache::clear();

    for (int k = 0; k < numberOfKeys; ++k) {
        QVERIFY(!QPixmapCache::find(keys.at(k), &p1));
        QVERIFY(!keys[k].isValid());
    }
}

void tst_QPixmapCache::pixmapKey()
{
    QPixmapCache::Key key;
    //Default constructed keys have no d pointer unless
    //we use them
    QVERIFY(!getPrivate(key));
    //Let's put a d pointer
    QPixmapCache::KeyData** keyd = getPrivateRef(key);
    *keyd = new QPixmapCache::KeyData;
    QCOMPARE(getPrivate(key)->ref, 1);
    QPixmapCache::Key key2;
    //Let's put a d pointer
    QPixmapCache::KeyData** key2d = getPrivateRef(key2);
    *key2d = new QPixmapCache::KeyData;
    QCOMPARE(getPrivate(key2)->ref, 1);
    key = key2;
    QCOMPARE(getPrivate(key2)->ref, 2);
    QCOMPARE(getPrivate(key)->ref, 2);
    QPixmapCache::Key key3;
    //Let's put a d pointer
    QPixmapCache::KeyData** key3d = getPrivateRef(key3);
    *key3d = new QPixmapCache::KeyData;
    QPixmapCache::Key key4 = key3;
    QCOMPARE(getPrivate(key3)->ref, 2);
    QCOMPARE(getPrivate(key4)->ref, 2);
    key4 = key;
    QCOMPARE(getPrivate(key4)->ref, 3);
    QCOMPARE(getPrivate(key3)->ref, 1);
    QPixmapCache::Key key5(key3);
    QCOMPARE(getPrivate(key3)->ref, 2);
    QCOMPARE(getPrivate(key5)->ref, 2);

    //let test default constructed keys
    QPixmapCache::Key key6;
    QVERIFY(!getPrivate(key6));
    QPixmapCache::Key key7;
    QVERIFY(!getPrivate(key7));
    key6 = key7;
    QVERIFY(!getPrivate(key6));
    QVERIFY(!getPrivate(key7));
    QPixmapCache::Key key8(key7);
    QVERIFY(!getPrivate(key8));
}

void tst_QPixmapCache::noLeak()
{
    QPixmapCache::Key key;

    int oldSize = q_QPixmapCache_keyHashSize();
    for (int i = 0; i < 100; ++i) {
        QPixmap pm(128, 128);
        pm.fill(Qt::transparent);
        key = QPixmapCache::insert(pm);
        QPixmapCache::remove(key);
    }
    int newSize = q_QPixmapCache_keyHashSize();

    QCOMPARE(oldSize, newSize);
}

void tst_QPixmapCache::clearDoesNotLeakStringKeys()
{
    stringLeak_impl([] { QPixmapCache::clear(); });
}

void tst_QPixmapCache::evictionDoesNotLeakStringKeys()
{
    stringLeak_impl([] {
        // fill the cache with other pixmaps to force eviction of "our" pixmap:
        constexpr int Iterations = 10;
        for (int i = 0; i < Iterations; ++i) {
            QPixmap pm(64, 64);
            pm.fill(Qt::transparent);
            [[maybe_unused]] auto r = QPixmapCache::insert(pm);
        }
    });
}

void tst_QPixmapCache::reducingCacheLimitDoesNotLeakStringKeys()
{
    stringLeak_impl([] {
        QPixmapCache::setCacheLimit(0);
    });
}

void tst_QPixmapCache::stringLeak_impl(std::function<void()> whenOp)
{
    QVERIFY(whenOp);

    QPixmapCache::setCacheLimit(20); // 20KiB
    //
    // GIVEN: a QPixmap with QString key `key` in QPixmapCache
    //
    QString key;
    {
        QPixmap pm(64, 64);
        QCOMPARE_LT(pm.width() * pm.height() * std::ceil(pm.depth() / 8.0),
                    QPixmapCache::cacheLimit() * 1024);
        pm.fill(Qt::transparent);
        key = u"theKey"_s.repeated(20); // avoid eventual QString SSO
        QVERIFY(key.isDetached());
        QPixmapCache::insert(key, pm);
    }
    QVERIFY(!key.isDetached()); // was saved inside QPixmapCache

    //
    // WHEN: performing the given operation
    //
    whenOp();
    if (QTest::currentTestFailed())
        return;

    //
    // THEN: `key` is no longer referenced by QPixmapCache:
    //
    QVERIFY(key.isDetached());
    // verify that the pixmap is really gone from the cache
    // (do it after the key check, because QPixmapCache cleans up `key` on a failed lookup)
    QPixmap r;
    QVERIFY(!QPixmapCache::find(key, &r));
}


void tst_QPixmapCache::strictCacheLimit()
{
    const int limit = 1024; // 1024 KB

    QPixmapCache::clear();
    QPixmapCache::setCacheLimit(limit);

    // insert 200 64x64 pixmaps
    // 3200 KB for 32-bit depths
    // 1600 KB for 16-bit depths
    // not counting the duplicate entries
    for (int i = 0; i < 200; ++i) {
        QPixmap pixmap(64, 64);
        pixmap.fill(Qt::transparent);

        QString id = QString::number(i);
        QPixmapCache::insert(id + "-a", pixmap);
        QPixmapCache::insert(id + "-b", pixmap);
    }

    QVERIFY(qt_qpixmapcache_qpixmapcache_total_used() <= limit);
}

void tst_QPixmapCache::noCrashOnLargeInsert()
{
    QPixmapCache::clear();
    QPixmapCache::setCacheLimit(100);
    QPixmap pixmap(500, 500);
    pixmap.fill(Qt::transparent);
    QPixmapCache::insert("test", pixmap);
    QVERIFY(true); // no crash
}

QTEST_MAIN(tst_QPixmapCache)
#include "tst_qpixmapcache.moc"
