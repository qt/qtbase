// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtCore/QCoreApplication>
#include <QTest>

class tst_SingleSkip: public QObject
{
    Q_OBJECT

private slots:
    void myTest() const;
};

void tst_SingleSkip::myTest() const
{
    QSKIP("skipping test");
}

QTEST_MAIN(tst_SingleSkip)

#include "tst_singleskip.moc"
