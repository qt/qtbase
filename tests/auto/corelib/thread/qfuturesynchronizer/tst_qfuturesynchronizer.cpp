// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

#include <QtCore/qfuturesynchronizer.h>
#include <QtCore/qfuture.h>

class tst_QFutureSynchronizer : public QObject
{
    Q_OBJECT


private Q_SLOTS:
    void construction();
    void setFutureAliasingExistingMember();
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

void tst_QFutureSynchronizer::setFutureAliasingExistingMember()
{
    //
    // GIVEN: a QFutureSynchronizer with one QFuture:
    //
    QFutureSynchronizer synchronizer(QtFuture::makeReadyValueFuture(42));

    //
    // WHEN: calling setFuture() with an alias of the QFuture already in `synchronizer`:
    //
    for (int i = 0; i < 2; ++i) {
        // The next line triggers -Wdangling-reference, but it's a FP because
        // of implicit sharing. We cannot keep a copy of synchronizer.futures()
        // around to avoid the warning, as the extra copy would cause a detach()
        // of m_futures inside setFuture() with the consequence that `f` no longer
        // aliases an element in m_futures, which is the goal of this test.
QT_WARNING_PUSH
#if defined(Q_CC_GNU_ONLY) && Q_CC_GNU >= 1301
QT_WARNING_DISABLE_GCC("-Wdangling-reference")
#endif
        const auto &f = synchronizer.futures().constFirst();
QT_WARNING_POP
        synchronizer.setFuture(f);
    }

    //
    // THEN: it didn't crash
    //
    QCOMPARE(synchronizer.futures().size(), 1);
    QCOMPARE(synchronizer.futures().constFirst().result(), 42);
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

    QCOMPARE(futures.size(), synchronizer.futures().size());
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
