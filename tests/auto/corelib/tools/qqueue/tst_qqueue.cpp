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
