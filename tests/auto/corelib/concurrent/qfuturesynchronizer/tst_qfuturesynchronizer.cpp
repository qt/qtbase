/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QtTest>

#include <QtCore/qfuturesynchronizer.h>
#include <QtCore/qfuture.h>

class tst_QFutureSynchronizer : public QObject
{
    Q_OBJECT


private Q_SLOTS:
    void construction();
    void addFuture();
    void cancelOnWait();
    void clearFutures();
    void futures();
    void setFuture();
    void waitForFinished();
};


void tst_QFutureSynchronizer::construction()
{

    QFuture<void> future;
    QFutureSynchronizer<void> synchronizer;
    QFutureSynchronizer<void> synchronizerWithFuture(future);

    QCOMPARE(synchronizer.futures().size(), 0);
    QCOMPARE(synchronizerWithFuture.futures().size(), 1);
}

void tst_QFutureSynchronizer::addFuture()
{
    QFutureSynchronizer<void> synchronizer;

    synchronizer.addFuture(QFuture<void>());
    QFuture<void> future;
    synchronizer.addFuture(future);
    synchronizer.addFuture(future);

    QCOMPARE(synchronizer.futures().size(), 3);
}

void tst_QFutureSynchronizer::cancelOnWait()
{
    QFutureSynchronizer<void> synchronizer;
    QVERIFY(!synchronizer.cancelOnWait());
    synchronizer.setCancelOnWait(true);
    QVERIFY(synchronizer.cancelOnWait());
    synchronizer.setCancelOnWait(false);
    QVERIFY(!synchronizer.cancelOnWait());
    synchronizer.setCancelOnWait(true);
    QVERIFY(synchronizer.cancelOnWait());
}

void tst_QFutureSynchronizer::clearFutures()
{
    QFutureSynchronizer<void> synchronizer;
    synchronizer.clearFutures();
    QVERIFY(synchronizer.futures().isEmpty());

    synchronizer.addFuture(QFuture<void>());
    QFuture<void> future;
    synchronizer.addFuture(future);
    synchronizer.addFuture(future);
    synchronizer.clearFutures();
    QVERIFY(synchronizer.futures().isEmpty());
}

void tst_QFutureSynchronizer::futures()
{
    QFutureSynchronizer<void> synchronizer;

    QList<QFuture<void> > futures;
    for (int i=0; i<100; i++) {
        QFuture<void> future;
        futures.append(future);
        synchronizer.addFuture(future);
    }

    QCOMPARE(futures, synchronizer.futures());
}

void tst_QFutureSynchronizer::setFuture()
{
    QFutureSynchronizer<void> synchronizer;

    for (int i=0; i<100; i++) {
        synchronizer.addFuture(QFuture<void>());
    }
    QCOMPARE(synchronizer.futures().size(), 100);

    QFuture<void> future;
    synchronizer.setFuture(future);
    QCOMPARE(synchronizer.futures().size(), 1);
    QCOMPARE(synchronizer.futures().first(), future);
}

void tst_QFutureSynchronizer::waitForFinished()
{
    QFutureSynchronizer<void> synchronizer;

    for (int i=0; i<100; i++) {
        synchronizer.addFuture(QFuture<void>());
    }
    synchronizer.waitForFinished();
    const QList<QFuture<void> > futures = synchronizer.futures();

    for (int i=0; i<100; i++) {
        QVERIFY(futures.at(i).isFinished());
    }
}

QTEST_MAIN(tst_QFutureSynchronizer)

#include "tst_qfuturesynchronizer.moc"
