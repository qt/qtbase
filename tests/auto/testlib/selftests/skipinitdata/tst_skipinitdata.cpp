// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

class tst_SkipInitData: public QObject
{
Q_OBJECT
private slots:
    void initTestCase_data() const;
    void initTestCase() const;
    void aTestFunction() const;
};

void tst_SkipInitData::initTestCase_data() const
{
    QSKIP("Skip inside initTestCase_data. This should skip all tests in the class.");
}

void tst_SkipInitData::initTestCase() const
{
}

/*! \internal
  This function should never be run because initTestCase fails.
 */
void tst_SkipInitData::aTestFunction() const
{
    qDebug() << "ERROR: this function is NOT supposed to be run.";
}

QTEST_APPLESS_MAIN(tst_SkipInitData)

#include "tst_skipinitdata.moc"
