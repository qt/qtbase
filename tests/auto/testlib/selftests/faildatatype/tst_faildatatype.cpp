// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>

class tst_FailDataType: public QObject
{
Q_OBJECT
private slots:
    void value_data() const;
    void value() const;
};

void tst_FailDataType::value_data() const
{
    QTest::addColumn<QString>("value");

    QTest::newRow("bool-as-string") << true; // assertion should fail here
}

/*! \internal
  This function should never be run because its _data() fails.
 */
void tst_FailDataType::value() const
{
    QFAIL("ERROR: this function is NOT supposed to be run.");
}

QTEST_APPLESS_MAIN(tst_FailDataType)

#include "tst_faildatatype.moc"
