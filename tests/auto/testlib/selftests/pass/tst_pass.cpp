// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QCoreApplication>
#include <QTest>

class tst_Pass: public QObject
{
    Q_OBJECT

private slots:
    void testNumber1() const;
    void testNumber2() const;
    void testNumber3() const;
};

void tst_Pass::testNumber1() const
{
}

void tst_Pass::testNumber2() const
{
}

void tst_Pass::testNumber3() const
{
}

QTEST_MAIN(tst_Pass)

#include "tst_pass.moc"
