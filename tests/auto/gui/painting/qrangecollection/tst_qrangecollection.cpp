/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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
#include <qrangecollection.h>

typedef QList<QPair<int, int>> PageRangeList;

class tst_QRangeCollection : public QObject
{
    Q_OBJECT

private slots:
    void parseFromString_data();
    void parseFromString();
};

void tst_QRangeCollection::parseFromString_data()
{
    QTest::addColumn<QString>("rangeString");
    QTest::addColumn<QList<QPair<int, int>>>("rangeList");

    QList<QPair<int, int>> invalid;
    QTest::newRow("invalid") << QString(",-8")
                             << invalid;

    QList<QPair<int, int>> overlapping;
    overlapping << qMakePair(1, 3)
                << qMakePair(5, 11);
    QTest::newRow("overlapping") << QString("1-3,5-9,6-7,8-11")
                                 << overlapping;
}

void tst_QRangeCollection::parseFromString()
{
    QFETCH(QString, rangeString);
    QFETCH(PageRangeList, rangeList);

    QRangeCollection rangeCollection;
    rangeCollection.parse(rangeString);
    QList<QPair<int, int>> result = rangeCollection.toList();
    QCOMPARE(result.length(), rangeList.length());
    for (const QPair<int, int> &pair : result) {
        QVERIFY(rangeList.contains(pair));
    }
}

QTEST_APPLESS_MAIN(tst_QRangeCollection)

#include "tst_qrangecollection.moc"
