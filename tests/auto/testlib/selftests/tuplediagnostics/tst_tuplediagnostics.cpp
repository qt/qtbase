// Copyright (C) 2017 Samuel Gaist <samuel.gaist@edeltech.ch>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Make sure we get a real Q_ASSERT even in release builds
#ifdef QT_NO_DEBUG
# undef QT_NO_DEBUG
#endif

#include <QTest>

class tst_TupleDiagnostics: public QObject
{
    Q_OBJECT

private slots:
    void testEmptyTuple() const;
    void testSimpleTuple() const;
    void testTuple() const;
};

void tst_TupleDiagnostics::testEmptyTuple() const
{
    QCOMPARE(std::tuple<>{}, std::tuple<>{});
}

void tst_TupleDiagnostics::testSimpleTuple() const
{
    QCOMPARE(std::tuple<int>{1}, std::tuple<int>{2});
}

void tst_TupleDiagnostics::testTuple() const
{
    std::tuple<int, char, QString> tuple1{42, 'Y', QStringLiteral("tuple1")};
    std::tuple<int, char, QString> tuple2{42, 'Y', QStringLiteral("tuple2")};
    QCOMPARE(tuple1, tuple2);
}

QTEST_MAIN(tst_TupleDiagnostics)

#include "tst_tuplediagnostics.moc"
