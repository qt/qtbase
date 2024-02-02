// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

class tst_SkipBlacklisted : public QObject
{
    Q_OBJECT

private slots:
    void pass();
    void blacklisted();
    void blacklistedData();
    void blacklistedData_data();
};

void tst_SkipBlacklisted::pass()
{
    QVERIFY(true);
}

// This test have been blacklisted in skipblacklisted/BLACKLIST
void tst_SkipBlacklisted::blacklisted()
{
    QFAIL("this line should never be reached, since we skip all blacklisted test functions");
}

// blacklisted 1 and blacklisted 2 have been blacklisted in skipblacklisted/BLACKLIST
void tst_SkipBlacklisted::blacklistedData()
{
    QFETCH(int, testdata);
    QCOMPARE(testdata, 2);
}

void tst_SkipBlacklisted::blacklistedData_data()
{
    QTest::addColumn<int>("testdata");

    QTest::newRow("blacklisted 1") << 1;
    QTest::newRow("should pass") << 2;
    QTest::newRow("blacklisted 2") << 3;
}

QTEST_MAIN_WRAPPER(tst_SkipBlacklisted,
    std::vector<const char*> args(argv, argv + argc);
    args.push_back("-skipblacklisted");
    argc = int(args.size());
    argv = const_cast<char**>(&args[0]);
    QTEST_MAIN_SETUP())

#include "tst_skipblacklisted.moc"
