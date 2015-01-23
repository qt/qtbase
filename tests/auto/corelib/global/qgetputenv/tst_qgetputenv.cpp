/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qdebug.h>
#include <QtTest/QtTest>

#include <qglobal.h>

class tst_QGetPutEnv : public QObject
{
Q_OBJECT
private slots:
    void getSetCheck();
    void intValue_data();
    void intValue();
};

void tst_QGetPutEnv::getSetCheck()
{
    const char varName[] = "should_not_exist";

    bool ok;

    QVERIFY(!qEnvironmentVariableIsSet(varName));
    QVERIFY(qEnvironmentVariableIsEmpty(varName));
    ok = true;
    QCOMPARE(qEnvironmentVariableIntValue(varName), 0);
    QCOMPARE(qEnvironmentVariableIntValue(varName, &ok), 0);
    QVERIFY(!ok);
    QByteArray result = qgetenv(varName);
    QCOMPARE(result, QByteArray());

#ifndef Q_OS_WIN
    QVERIFY(qputenv(varName, "")); // deletes varName instead of making it empty, on Windows

    QVERIFY(qEnvironmentVariableIsSet(varName));
    QVERIFY(qEnvironmentVariableIsEmpty(varName));
    ok = true;
    QCOMPARE(qEnvironmentVariableIntValue(varName), 0);
    QCOMPARE(qEnvironmentVariableIntValue(varName, &ok), 0);
    QVERIFY(!ok);
#endif

    QVERIFY(qputenv(varName, QByteArray("supervalue")));

    QVERIFY(qEnvironmentVariableIsSet(varName));
    QVERIFY(!qEnvironmentVariableIsEmpty(varName));
    ok = true;
    QCOMPARE(qEnvironmentVariableIntValue(varName), 0);
    QCOMPARE(qEnvironmentVariableIntValue(varName, &ok), 0);
    QVERIFY(!ok);
    result = qgetenv(varName);
    QVERIFY(result == "supervalue");

    qputenv(varName,QByteArray());

    // Now test qunsetenv
    QVERIFY(qunsetenv(varName));
    QVERIFY(!qEnvironmentVariableIsSet(varName));
    QVERIFY(qEnvironmentVariableIsEmpty(varName));
    ok = true;
    QCOMPARE(qEnvironmentVariableIntValue(varName), 0);
    QCOMPARE(qEnvironmentVariableIntValue(varName, &ok), 0);
    QVERIFY(!ok);
    result = qgetenv(varName);
    QCOMPARE(result, QByteArray());
}

void tst_QGetPutEnv::intValue_data()
{
    QTest::addColumn<QByteArray>("value");
    QTest::addColumn<int>("expected");
    QTest::addColumn<bool>("ok");

    // most non-success cases already tested in getSetCheck()

#define ROW(x, i, b) \
    QTest::newRow(#x) << QByteArray(#x) << (i) << (b)
    ROW(auto, 0, false);
    ROW(0, 0, true);
    ROW(1, 1, true);
    ROW(010, 8, true);
    ROW(0x10, 16, true);
    ROW(-1, -1, true);
    ROW(-010, -8, true);
    // ROW(0xffffffff, -1, true); // could be expected, but not how QByteArray::toInt() works
    ROW(0xffffffff, 0, false);
    const int bases[] = {10, 8, 16};
    for (size_t i = 0; i < sizeof bases / sizeof *bases; ++i) {
        QTest::newRow(qPrintable(QString::asprintf("INT_MAX, base %d", bases[i])))
                << QByteArray::number(INT_MAX) << INT_MAX << true;
        QTest::newRow(qPrintable(QString::asprintf("INT_MAX+1, base %d", bases[i])))
                << QByteArray::number(qlonglong(INT_MAX) + 1) << 0 << false;
        QTest::newRow(qPrintable(QString::asprintf("INT_MIN, base %d", bases[i])))
                << QByteArray::number(INT_MIN) << INT_MIN << true;
        QTest::newRow(qPrintable(QString::asprintf("INT_MIN-1, base %d", bases[i])))
                << QByteArray::number(qlonglong(INT_MIN) - 1) << 0 << false;
    };
}

void tst_QGetPutEnv::intValue()
{
    const char varName[] = "should_not_exist";

    QFETCH(QByteArray, value);
    QFETCH(int, expected);
    QFETCH(bool, ok);

    bool actualOk = !ok;

    QVERIFY(qputenv(varName, value));
    QCOMPARE(qEnvironmentVariableIntValue(varName), expected);
    QCOMPARE(qEnvironmentVariableIntValue(varName, &actualOk), expected);
    QCOMPARE(actualOk, ok);
}

QTEST_MAIN(tst_QGetPutEnv)
#include "tst_qgetputenv.moc"
