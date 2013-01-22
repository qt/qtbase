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
            {
                QString tmp;
                tmp.sprintf("my-key-%d", i);
                QPixmapCache::insert(tmp, p);
            }
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
            QString tmp;
            for (int i = 0 ; i <= 10000 ; i++)
            {
                tmp.sprintf("my-key-%d", i);
                QPixmapCache::find(tmp, p);
            }
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
            {
                QString tmp;
                tmp.sprintf("%s-%d-%d-%d-%d-%d-%d", QString("my-progressbar-%1").arg(i).toLatin1().constData(), 5, 3, 0, 358, 100, 200);
                QPixmapCache::insert(tmp, p);
            }

            for (int i = 0 ; i <= 10000 ; i++)
            {
                QString tmp;
                tmp.sprintf("%s-%d-%d-%d-%d-%d-%d", QString("my-progressbar-%1").arg(i).toLatin1().constData(), 5, 3, 0, 358, 100, 200);
                QPixmapCache::find(tmp, p);
            }
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
