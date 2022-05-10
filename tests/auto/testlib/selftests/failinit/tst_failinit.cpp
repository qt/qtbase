// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>

class tst_FailInit: public QObject
{
Q_OBJECT
private slots:
    void initTestCase() const;
    void aTestFunction() const;
};

void tst_FailInit::initTestCase() const
{
    QVERIFY(false);
}

/*! \internal
  This function should never be run because initTestCase fails.
 */
void tst_FailInit::aTestFunction() const
{
    qDebug() << "ERROR: this function is NOT supposed to be run.";
}

QTEST_APPLESS_MAIN(tst_FailInit)

#include "tst_failinit.moc"
