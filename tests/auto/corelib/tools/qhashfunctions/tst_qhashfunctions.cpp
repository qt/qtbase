/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <qhash.h>

class tst_QHashFunctions : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void qhash();
    void fp_qhash_of_zero_is_zero();
    void qthash_data();
    void qthash();
};

void tst_QHashFunctions::qhash()
{
    {
        QBitArray a1;
        QBitArray a2;
        QVERIFY(qHash(a1) == 0);

        a1.resize(1);
        a1.setBit(0, true);

        a2.resize(1);
        a2.setBit(0, false);

        uint h1 = qHash(a1);
        uint h2 = qHash(a2);

        QVERIFY(h1 != h2);

        a2.setBit(0, true);
        QVERIFY(h1 == qHash(a2));

        a1.fill(true, 8);
        a1.resize(7);

        h1 = qHash(a1);

        a2.fill(true, 7);
        h2 = qHash(a2);

        QVERIFY(h1 == h2);

        a2.setBit(0, false);
        uint h3 = qHash(a2);
        QVERIFY(h2 != h3);

        a2.setBit(0, true);
        QVERIFY(h2 == qHash(a2));

        a2.setBit(6, false);
        uint h4 = qHash(a2);
        QVERIFY(h2 != h4);

        a2.setBit(6, true);
        QVERIFY(h2 == qHash(a2));

        QVERIFY(h3 != h4);
    }

    {
        QPair<int, int> p12(1, 2);
        QPair<int, int> p21(2, 1);

        QVERIFY(qHash(p12) == qHash(p12));
        QVERIFY(qHash(p21) == qHash(p21));
        QVERIFY(qHash(p12) != qHash(p21));

        QPair<int, int> pA(0x12345678, 0x12345678);
        QPair<int, int> pB(0x12345675, 0x12345675);

        QVERIFY(qHash(pA) != qHash(pB));
    }
}

void tst_QHashFunctions::fp_qhash_of_zero_is_zero()
{
    QCOMPARE(qHash(-0.0f), 0U);
    QCOMPARE(qHash( 0.0f), 0U);

    QCOMPARE(qHash(-0.0 ), 0U);
    QCOMPARE(qHash( 0.0 ), 0U);

#ifndef Q_OS_DARWIN
    QCOMPARE(qHash(-0.0L), 0U);
    QCOMPARE(qHash( 0.0L), 0U);
#endif
}

void tst_QHashFunctions::qthash_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<uint>("hash");

    QTest::newRow("null") << QString() << 0u;
    QTest::newRow("empty") << QStringLiteral("") << 0u;
    QTest::newRow("abcdef") << QStringLiteral("abcdef") << 108567222u;
    QTest::newRow("tqbfjotld") << QStringLiteral("The quick brown fox jumps over the lazy dog") << 140865879u;
    QTest::newRow("42") << QStringLiteral("42") << 882u;
}

void tst_QHashFunctions::qthash()
{
    QFETCH(QString, key);
    const uint result = qt_hash(key);
    QTEST(result, "hash");
}

QTEST_APPLESS_MAIN(tst_QHashFunctions)
#include "tst_qhashfunctions.moc"
