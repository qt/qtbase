/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#define Q_TEST_QPIXMAPCACHE

#include <QtTest/QtTest>


#include <qpixmapcache.h>
#include "private/qpixmapcache_p.h"

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
    void replace();
    void remove();
    void clear();
    void pixmapKey();
    void noLeak();
    void strictCacheLimit();
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

    QPixmapCache::setCacheLimit(100);
    QCOMPARE(QPixmapCache::cacheLimit(), 100);

    QPixmapCache::setCacheLimit(-50);
    QCOMPARE(QPixmapCache::cacheLimit(), -50);
}

void tst_QPixmapCache::setCacheLimit()
{
    QPixmap *p1 = new QPixmap(2, 3);
    QPixmapCache::insert("P1", *p1);
    QVERIFY(QPixmapCache::find("P1") != 0);
    delete p1;

    QPixmapCache::setCacheLimit(0);
    QVERIFY(QPixmapCache::find("P1") == 0);

    p1 = new QPixmap(2, 3);
    QPixmapCache::setCacheLimit(1000);
    QPixmapCache::insert("P1", *p1);
    QVERIFY(QPixmapCache::find("P1") != 0);

    delete p1;

    //The int part of the API
    p1 = new QPixmap(2, 3);
    QPixmapCache::Key key = QPixmapCache::insert(*p1);
    QVERIFY(QPixmapCache::find(key, p1) != 0);
    delete p1;

    QPixmapCache::setCacheLimit(0);
    QVERIFY(QPixmapCache::find(key, p1) == 0);

    p1 = new QPixmap(2, 3);
    QPixmapCache::setCacheLimit(1000);
    QPixmapCache::replace(key, *p1);
    QVERIFY(QPixmapCache::find(key, p1) == 0);

    delete p1;

    //Let check if keys are released when the pixmap cache is
    //full or has been flushed.
    QPixmapCache::clear();
    p1 = new QPixmap(2, 3);
    key = QPixmapCache::insert(*p1);
    QVERIFY(QPixmapCache::find(key, p1) != 0);
    p1->detach(); // dectach so that the cache thinks no-one is using it.
    QPixmapCache::setCacheLimit(0);
    QVERIFY(QPixmapCache::find(key, p1) == 0);
    QPixmapCache::setCacheLimit(1000);
    key = QPixmapCache::insert(*p1);
    QCOMPARE(getPrivate(key)->isValid, true);
    QCOMPARE(getPrivate(key)->key, 1);

    delete p1;

    //Let check if removing old entries doesn't let you get
    // wrong pixmaps
    QPixmapCache::clear();
    QPixmap p2;
    p1 = new QPixmap(2, 3);
    key = QPixmapCache::insert(*p1);
    QVERIFY(QPixmapCache::find(key, &p2) != 0);
    //we flush the cache
    p1->detach();
    p2.detach();
    QPixmapCache::setCacheLimit(0);
    QPixmapCache::setCacheLimit(1000);
    QPixmapCache::Key key2 = QPixmapCache::insert(*p1);
    QCOMPARE(getPrivate(key2)->key, 1);
    QVERIFY(QPixmapCache::find(key, &p2) == 0);
    QVERIFY(QPixmapCache::find(key2, &p2) != 0);
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
    QPixmapCache::flushDetachedPixmaps();
    key2 = QPixmapCache::insert(*p1);
    QCOMPARE(getPrivate(key2)->key, 1);
    //This old key is not valid anymore after the flush
    QCOMPARE(getPrivate(key)->isValid, false);
    QVERIFY(QPixmapCache::find(key, &p2) == 0);
    delete p1;
}

void tst_QPixmapCache::find()
{
    QPixmap p1(10, 10);
    p1.fill(Qt::red);
    QVERIFY(QPixmapCache::insert("P1", p1));

    QPixmap p2;
    QVERIFY(QPixmapCache::find("P1", p2));
    QCOMPARE(p2.width(), 10);
    QCOMPARE(p2.height(), 10);
    QCOMPARE(p1, p2);

    // obsolete
    QPixmap *p3 = QPixmapCache::find("P1");
    QVERIFY(p3);
    QCOMPARE(p1, *p3);

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
    QVERIFY(QPixmapCache::find(key, &p1) == 0);
    QCOMPARE(getPrivate(key)->isValid, false);
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
    for (int k = 0; k < numberOfKeys; ++k) {
        if (QPixmapCache::find(QString::number(k)))
            ++num;
    }

    if (QPixmapCache::find("0"))
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

void tst_QPixmapCache::replace()
{
    //The int part of the API
    QPixmap p1(10, 10);
    p1.fill(Qt::red);

    QPixmap p2(10, 10);
    p2.fill(Qt::yellow);

    QPixmapCache::Key key = QPixmapCache::insert(p1);
    QCOMPARE(getPrivate(key)->isValid, true);

    QPixmap p3;
    QVERIFY(QPixmapCache::find(key, &p3) == 1);

    QPixmapCache::replace(key, p2);

    QVERIFY(QPixmapCache::find(key, &p3) == 1);
    QCOMPARE(getPrivate(key)->isValid, true);
    QCOMPARE(getPrivate(key)->key, 1);

    QCOMPARE(p3.width(), 10);
    QCOMPARE(p3.height(), 10);
    QCOMPARE(p3, p2);

    //Broken keys
    QCOMPARE(QPixmapCache::replace(QPixmapCache::Key(), p2), false);
}

void tst_QPixmapCache::remove()
{
    QPixmap p1(10, 10);
    p1.fill(Qt::red);

    QPixmapCache::insert("red", p1);
    p1.fill(Qt::yellow);

    QPixmap p2;
    QVERIFY(QPixmapCache::find("red", p2));
    QVERIFY(p1.toImage() != p2.toImage());
    QVERIFY(p1.toImage() == p1.toImage()); // sanity check

    QPixmapCache::remove("red");
    QVERIFY(QPixmapCache::find("red") == 0);
    QPixmapCache::remove("red");
    QVERIFY(QPixmapCache::find("red") == 0);

    QPixmapCache::remove("green");
    QVERIFY(QPixmapCache::find("green") == 0);

    //The int part of the API
    QPixmapCache::clear();
    p1.fill(Qt::red);
    QPixmapCache::Key key = QPixmapCache::insert(p1);
    p1.fill(Qt::yellow);

    QVERIFY(QPixmapCache::find(key, &p2));
    QVERIFY(p1.toImage() != p2.toImage());
    QVERIFY(p1.toImage() == p1.toImage()); // sanity check

    QPixmapCache::remove(key);
    QVERIFY(QPixmapCache::find(key, &p1) == 0);

    //Broken key
    QPixmapCache::remove(QPixmapCache::Key());
    QVERIFY(QPixmapCache::find(QPixmapCache::Key(), &p1) == 0);

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
    QVERIFY(QPixmapCache::find(key, &p1) != 0);
    QPixmapCache::remove(key);
    QCOMPARE(p1.isDetached(), true);

    //We mix both part of the API
    QPixmapCache::clear();
    p1.fill(Qt::red);
    QPixmapCache::insert("red", p1);
    key = QPixmapCache::insert(p1);
    QPixmapCache::remove(key);
    QVERIFY(QPixmapCache::find(key, &p1) == 0);
    QVERIFY(QPixmapCache::find("red") != 0);
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
        QVERIFY(QPixmapCache::find("x" + QString::number(i)) == 0);

    for (int j = 0; j < numberOfKeys; ++j)
        QPixmapCache::insert(QString::number(j), p1);

    int num = 0;
    for (int k = 0; k < numberOfKeys; ++k) {
        if (QPixmapCache::find(QString::number(k), p1))
            ++num;
    }
    QVERIFY(num > 0);

    QPixmapCache::clear();

    for (int k = 0; k < numberOfKeys; ++k)
        QVERIFY(QPixmapCache::find(QString::number(k)) == 0);

    //The int part of the API
    QPixmap p2(10, 10);
    p2.fill(Qt::red);

    QList<QPixmapCache::Key> keys;
    for (int k = 0; k < numberOfKeys; ++k)
        keys.append(QPixmapCache::insert(p2));

    QPixmapCache::clear();

    for (int k = 0; k < numberOfKeys; ++k) {
        QVERIFY(QPixmapCache::find(keys.at(k), &p1) == 0);
        QCOMPARE(getPrivate(keys[k])->isValid, false);
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

QT_BEGIN_NAMESPACE
extern int q_QPixmapCache_keyHashSize();
QT_END_NAMESPACE

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

    QVERIFY(QPixmapCache::totalUsed() <= limit);
}

QTEST_MAIN(tst_QPixmapCache)
#include "tst_qpixmapcache.moc"
