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
#include <qqueue.h>

class tst_QQueue : public QObject
{
    Q_OBJECT

private slots:
    void enqueue_dequeue_data();
    void enqueue_dequeue();
};

void tst_QQueue::enqueue_dequeue_data()
{
    QTest::addColumn<int>("num_items");

    QTest::newRow("1") << 11;
    QTest::newRow("2") << 211;
    QTest::newRow("3") << 1024 + 211;
}

void tst_QQueue::enqueue_dequeue()
{
    QFETCH(int, num_items);

    int *values = new int[num_items];
    QQueue<int>  queue_v;
    QQueue<int*> queue_p;

    QVERIFY(queue_v.empty());
    QVERIFY(queue_p.empty());

    for (int i = 0; i < num_items; i++ ) {
        values[i] = i;
        queue_p.enqueue(values + i);
        queue_v.enqueue(values[i]);
    }
    QVERIFY(!queue_p.empty());
    QVERIFY(!queue_v.empty());
    for (int i = 0; i < num_items; i++ ) {
        int v, *p;
        v = queue_v.head();
        p = queue_p.head();
        QCOMPARE(*p, v);
        QCOMPARE(v, i);
        v = queue_v.dequeue();
        p = queue_p.dequeue();
        QCOMPARE(*p, v);
        QCOMPARE(v, values[i]);
    }
    QVERIFY(queue_v.empty());
    QVERIFY(queue_p.empty());

    delete[] values;
}

QTEST_APPLESS_MAIN(tst_QQueue)
#include "tst_qqueue.moc"
