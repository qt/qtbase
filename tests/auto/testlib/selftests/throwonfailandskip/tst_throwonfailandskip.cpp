// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#ifndef QTEST_THROW_ON_FAIL
# error This test requires throw-on-fail mode.
#endif

#ifndef QTEST_THROW_ON_SKIP
# error This test requires throw-on-skip mode.
#endif

QT_REQUIRE_CONFIG(future); // otherwise QException doesn't exist

#ifdef QT_NO_CONCURRENT
# error This test requires QtConcurrent::run().
#endif

#include <QtConcurrent/qtconcurrentrun.h>

class tst_ThrowOnFailAndSkip: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void throwOnFail() const;
    void throwOnSkip() const;
    void throwOnFailWorksFromConcurrent() const;
    void throwOnSkipWorksFromConcurrent() const;
};


void tst_ThrowOnFailAndSkip::throwOnFail() const
{
    int i = 17;
    // This would not compile if QTEST_FAIL_ACTION was just a `return;`
    i = [&] { QCOMPARE(i, 42); return 42; }();
    // When throw-on-fail works, the following line
    // won't be executed anymore:
    QCOMPARE(i, 67);
}

void tst_ThrowOnFailAndSkip::throwOnSkip() const
{
    int i = 17;
    // This would not compile if QTEST_SKIP_ACTION was just a `return;`
    i = [&] { QSKIP("skipped"); return 42; }();
    // When throw-on-skip works, the following line
    // won't be executed anymore:
    QCOMPARE(i, 67);
}

static int function(int i)
{
    // This would not compile if QTEST_FAIL_ACTION was just a `return;`
    QCOMPARE_NE(i, 42);
    return 17;
}

void tst_ThrowOnFailAndSkip::throwOnFailWorksFromConcurrent() const
{
    QCOMPARE(QtConcurrent::run(&function, 42).result(), 17);
    // When throw-on-fail works, the following line (and the outer QCOMPARE above)
    // won't be executed anymore:
    QVERIFY(false);
}

void tst_ThrowOnFailAndSkip::throwOnSkipWorksFromConcurrent() const
{
    const auto lambda = [] {
        QSKIP("skipped from QtConcurrent::run()");
        return 42;
    };
    QCOMPARE(QtConcurrent::run(lambda).result(), 42);
    // When throw-on-skip works, the following line (and the QCOMPARE above)
    // won't be executed anymore:
    QVERIFY(false);
}

QTEST_MAIN(tst_ThrowOnFailAndSkip)

#include "tst_throwonfailandskip.moc"
