// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
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
