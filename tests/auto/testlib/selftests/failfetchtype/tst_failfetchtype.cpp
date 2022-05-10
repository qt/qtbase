// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>

class tst_FailFetchType: public QObject
{
Q_OBJECT
private slots:
    void fetch_data() const;
    void fetch() const;
};

void tst_FailFetchType::fetch_data() const
{
    QTest::addColumn<bool>("value");

    QTest::newRow("bool") << true;
}

void tst_FailFetchType::fetch() const
{
    QFETCH(QString, value); // assertion should fail here
    QFAIL("ERROR: this function is NOT supposed to be run.");
}

QTEST_APPLESS_MAIN(tst_FailFetchType)

#include "tst_failfetchtype.moc"
