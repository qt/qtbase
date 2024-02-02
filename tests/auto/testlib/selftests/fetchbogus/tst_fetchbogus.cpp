// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtCore/QCoreApplication>
#include <QTest>

class tst_FetchBogus: public QObject
{
    Q_OBJECT

private slots:
    void fetchBogus_data();
    void fetchBogus();
};

void tst_FetchBogus::fetchBogus_data()
{
    QTest::addColumn<QString>("string");
    QTest::newRow("foo") << QString("blah");
}

void tst_FetchBogus::fetchBogus()
{
    QFETCH(QString, bubu);
}

QTEST_MAIN(tst_FetchBogus)

#include "tst_fetchbogus.moc"
