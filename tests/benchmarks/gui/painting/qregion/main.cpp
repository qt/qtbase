/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
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
