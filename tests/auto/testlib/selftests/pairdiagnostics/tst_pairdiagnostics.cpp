// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Make sure we get a real Q_ASSERT even in release builds
#ifdef QT_NO_DEBUG
# undef QT_NO_DEBUG
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QPair>
#include <QTest>

class tst_PairDiagnostics: public QObject
{
    Q_OBJECT

private slots:
    void testQPair() const;
    void testStdPair() const;
};

void tst_PairDiagnostics::testQPair() const
{
    QPair<int, int> pair1 = qMakePair(1, 1);
    QPair<int, int> pair2 = qMakePair(1, 2);
    QCOMPARE(pair1, pair2);
}

void tst_PairDiagnostics::testStdPair() const
{
    std::pair<int, int> pair1 = std::make_pair(1, 1);
    std::pair<int, int> pair2 = std::make_pair(1, 2);
    QCOMPARE(pair1, pair2);
}

QTEST_MAIN(tst_PairDiagnostics)

#include "tst_pairdiagnostics.moc"
