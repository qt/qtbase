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

#include <qtest.h>
#include <QPixmapCache>

class tst_QPixmapCache : public QObject
{
    Q_OBJECT

public:
    tst_QPixmapCache();
    virtual ~tst_QPixmapCache();

public slots:
    void init();
    void cleanup();

private slots:
    void insert_data();
    void insert();
    void find_data();
    void find();
    void styleUseCaseComplexKey();
    void styleUseCaseComplexKey_data();
};

tst_QPixmapCache::tst_QPixmapCache()
{
}

tst_QPixmapCache::~tst_QPixmapCache()
{
}

void tst_QPixmapCache::init()
{
}

void tst_QPixmapCache::cleanup()
{
}

void tst_QPixmapCache::insert_data()
{
    QTest::addColumn<bool>("cacheType");
    QTest::newRow("QPixmapCache") << true;
    QTest::newRow("QPixmapCache (int API)") << false;
}

QList<QPixmapCache::Key> keys;

void tst_QPixmapCache::insert()
{
    QFETCH(bool, cacheType);
    QPixmap p;
    if (cacheType) {
        QBENCHMARK {
            for (int i = 0 ; i <= 10000 ; i++)
                QPixmapCache::insert(QString::asprintf("my-key-%d", i), p);
        }
    } else {
        QBENCHMARK {
            for (int i = 0 ; i <= 10000 ; i++)
                keys.append(QPixmapCache::insert(p));
        }
    }
}

void tst_QPixmapCache::find_data()
{
    QTest::addColumn<bool>("cacheType");
    QTest::newRow("QPixmapCache") << true;
    QTest::newRow("QPixmapCache (int API)") << false;
}

void tst_QPixmapCache::find()
{
    QFETCH(bool, cacheType);
    QPixmap p;
    if (cacheType) {
        QBENCHMARK {
            for (int i = 0 ; i <= 10000 ; i++)
                QPixmapCache::find(QString::asprintf("my-key-%d", i), p);
        }
    } else {
        QBENCHMARK {
            for (int i = 0 ; i <= 10000 ; i++)
                QPixmapCache::find(keys.at(i), &p);
        }
    }

}

void tst_QPixmapCache::styleUseCaseComplexKey_data()
{
    QTest::addColumn<bool>("cacheType");
    QTest::newRow("QPixmapCache") << true;
    QTest::newRow("QPixmapCache (int API)") << false;
}

struct styleStruct {
    QString key;
    uint state;
    uint direction;
    uint complex;
    uint palette;
    int width;
    int height;
    bool operator==(const styleStruct &str) const
    {
        return  str.state == state && str.direction == direction
                && str.complex == complex && str.palette == palette && str.width == width
                && str.height == height && str.key == key;
    }
};

uint qHash(const styleStruct &myStruct)
{
    return qHash(myStruct.state);
}

void tst_QPixmapCache::styleUseCaseComplexKey()
{
    QFETCH(bool, cacheType);
    QPixmap p;
    if (cacheType) {
        QBENCHMARK {
            for (int i = 0 ; i <= 10000 ; i++)
                QPixmapCache::insert(QString::asprintf("%s-%d-%d-%d-%d-%d-%d", QString("my-progressbar-%1").arg(i).toLatin1().constData(), 5, 3, 0, 358, 100, 200), p);

            for (int i = 0 ; i <= 10000 ; i++)
                QPixmapCache::find(QString::asprintf("%s-%d-%d-%d-%d-%d-%d", QString("my-progressbar-%1").arg(i).toLatin1().constData(), 5, 3, 0, 358, 100, 200), p);
        }
    } else {
        QHash<styleStruct, QPixmapCache::Key> hash;
        QBENCHMARK {
            for (int i = 0 ; i <= 10000 ; i++)
            {
                styleStruct myStruct;
                myStruct.key = QString("my-progressbar-%1").arg(i);
                myStruct.key = 5;
                myStruct.key = 4;
                myStruct.key = 3;
                myStruct.palette = 358;
                myStruct.width = 100;
                myStruct.key = 200;
                QPixmapCache::Key key = QPixmapCache::insert(p);
                hash.insert(myStruct, key);
            }
            for (int i = 0 ; i <= 10000 ; i++)
            {
                styleStruct myStruct;
                myStruct.key = QString("my-progressbar-%1").arg(i);
                myStruct.key = 5;
                myStruct.key = 4;
                myStruct.key = 3;
                myStruct.palette = 358;
                myStruct.width = 100;
                myStruct.key = 200;
                QPixmapCache::Key key = hash.value(myStruct);
                QPixmapCache::find(key, &p);
            }
        }
    }

}


QTEST_MAIN(tst_QPixmapCache)
#include "tst_qpixmapcache.moc"
