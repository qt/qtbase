// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtCore/QtCore>

class tst_QString_NoCastFromByteArray: public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
};

void tst_QString_NoCastFromByteArray::initTestCase()
{
    qWarning("This is a compile test only");
}

QTEST_APPLESS_MAIN(tst_QString_NoCastFromByteArray)

#include "tst_qstring_no_cast_from_bytearray.moc"
