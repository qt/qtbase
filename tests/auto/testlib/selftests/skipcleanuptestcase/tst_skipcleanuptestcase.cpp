// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

class tst_SkipCleanupTestCase: public QObject
{
Q_OBJECT
private slots:
    void aTestFunction() const;
    void cleanupTestCase() const;
};

void tst_SkipCleanupTestCase::aTestFunction() const
{
    QVERIFY(true);
}

void tst_SkipCleanupTestCase::cleanupTestCase() const
{
    QSKIP("Skip inside cleanupTestCase.");
}

QTEST_APPLESS_MAIN(tst_SkipCleanupTestCase)
#include "tst_skipcleanuptestcase.moc"
