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
// This file contains benchmarks for QRegion functions.

#include <QDebug>
#include <qtest.h>

class tst_qregion : public QObject
{
    Q_OBJECT
private slots:
    void map_data();
    void map();

    void intersects_data();
    void intersects();
};


void tst_qregion::map_data()
{
    QTest::addColumn<QRegion>("region");

    {
        QRegion region(0, 0, 100, 100);
        QTest::newRow("single rect") << region;
    }
    {
        QRegion region;
        region = region.united(QRect(0, 0, 100, 100));
        region = region.united(QRect(120, 20, 100, 100));

        QTest::newRow("two rects") << region;
    }
    {
        QRegion region(0, 0, 100, 100, QRegion::Ellipse);
        QTest::newRow("ellipse") << region;
    }
}

void tst_qregion::map()
{
    QFETCH(QRegion, region);

    QTransform transform;
    transform.rotate(30);
    QBENCHMARK {
        transform.map(region);
    }
}

void tst_qregion::intersects_data()
{
    QTest::addColumn<QRegion>("region");
    QTest::addColumn<QRect>("rect");

    QRegion region(0, 0, 100, 100);
    QRegion complexRegion;
    complexRegion = complexRegion.united(QRect(0, 0, 100, 100));
    complexRegion = complexRegion.united(QRect(120, 20, 100, 100));

    {
        QRect rect(0, 0, 100, 100);
        QTest::newRow("same -- simple") << region << rect;
    }
    {
        QRect rect(10, 10, 10, 10);
        QTest::newRow("inside -- simple") << region << rect;
    }
    {
        QRect rect(110, 110, 10, 10);
        QTest::newRow("outside -- simple") << region << rect;
    }

    {
        QRect rect(0, 0, 100, 100);
        QTest::newRow("same -- complex") << complexRegion << rect;
    }
    {
        QRect rect(10, 10, 10, 10);
        QTest::newRow("inside -- complex") << complexRegion << rect;
    }
    {
        QRect rect(110, 110, 10, 10);
        QTest::newRow("outside -- complex") << complexRegion << rect;
    }
}

void tst_qregion::intersects()
{
    QFETCH(QRegion, region);
    QFETCH(QRect, rect);

    QBENCHMARK {
        region.intersects(rect);
    }
}

QTEST_MAIN(tst_qregion)

#include "main.moc"
