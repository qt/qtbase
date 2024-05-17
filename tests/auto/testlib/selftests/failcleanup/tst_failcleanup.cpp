// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

class tst_FailCleanup: public QObject
{
Q_OBJECT
private slots:
    void aTestFunction() const;
    void cleanup() const;
};

void tst_FailCleanup::aTestFunction() const
{
    QVERIFY(true);
}

void tst_FailCleanup::cleanup() const
{
    QVERIFY2(false, "Fail inside cleanup");
}

QTEST_APPLESS_MAIN(tst_FailCleanup)
#include "tst_failcleanup.moc"
