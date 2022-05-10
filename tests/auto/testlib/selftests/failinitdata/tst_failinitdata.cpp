// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>

class tst_FailInitData: public QObject
{
Q_OBJECT
private slots:
    void initTestCase() const;
    void initTestCase_data() const;
    void aTestFunction() const;
};

void tst_FailInitData::initTestCase_data() const
{
    QVERIFY(false);
}

/*! \internal
  This function should never be run because initTestCase_data fails.
 */
void tst_FailInitData::initTestCase() const
{
    qDebug() << "This function is NOT supposed to be called.";
}

/*! \internal
  This function should never be run because initTestCase_data fails.
 */
void tst_FailInitData::aTestFunction() const
{
    qDebug() << "This function is NOT supposed to be called.";
}

QTEST_APPLESS_MAIN(tst_FailInitData)

#include "tst_failinitdata.moc"
