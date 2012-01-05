/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

class tst_QCryptographicHash : public QObject
{
    Q_OBJECT
private slots:
    void repeated_result_data();
    void repeated_result();
    void intermediary_result_data();
    void intermediary_result();
    void sha1();
    void files_data();
    void files();
};
#include <QtCore>

void tst_QCryptographicHash::repeated_result_data()
{
    intermediary_result_data();
}

void tst_QCryptographicHash::repeated_result()
{
    QFETCH(int, algo);
    QCryptographicHash::Algorithm _algo = QCryptographicHash::Algorithm(algo);
    QCryptographicHash hash(_algo);

    QFETCH(QByteArray, first);
    hash.addData(first);

    QFETCH(QByteArray, hash_first);
    QByteArray result = hash.result();
    QCOMPARE(result, hash_first);
    QCOMPARE(result, hash.result());

    hash.reset();
    hash.addData(first);
    result = hash.result();
    QCOMPARE(result, hash_first);
    QCOMPARE(result, hash.result());
}

void tst_QCryptographicHash::intermediary_result_data()
{
    QTest::addColumn<int>("algo");
    QTest::addColumn<QByteArray>("first");
    QTest::addColumn<QByteArray>("second");
    QTest::addColumn<QByteArray>("hash_first");
    QTest::addColumn<QByteArray>("hash_firstsecond");

    QTest::newRow("md4") << int(QCryptographicHash::Md4)
                         << QByteArray("abc") << QByteArray("abc")
                         << QByteArray::fromHex("A448017AAF21D8525FC10AE87AA6729D")
                         << QByteArray::fromHex("03E5E436DAFAF3B9B3589DB83C417C6B");
    QTest::newRow("md5") << int(QCryptographicHash::Md5)
                         << QByteArray("abc") << QByteArray("abc")
                         << QByteArray::fromHex("900150983CD24FB0D6963F7D28E17F72")
                         << QByteArray::fromHex("440AC85892CA43AD26D44C7AD9D47D3E");
    QTest::newRow("sha1") << int(QCryptographicHash::Sha1)
                          << QByteArray("abc") << QByteArray("abc")
                          << QByteArray::fromHex("A9993E364706816ABA3E25717850C26C9CD0D89D")
                          << QByteArray::fromHex("F8C1D87006FBF7E5CC4B026C3138BC046883DC71");
}

void tst_QCryptographicHash::intermediary_result()
{
    QFETCH(int, algo);
    QCryptographicHash::Algorithm _algo = QCryptographicHash::Algorithm(algo);
    QCryptographicHash hash(_algo);

    QFETCH(QByteArray, first);
    hash.addData(first);

    QFETCH(QByteArray, hash_first);
    QByteArray result = hash.result();
    QCOMPARE(result, hash_first);

    // don't reset
    QFETCH(QByteArray, second);
    QFETCH(QByteArray, hash_firstsecond);
    hash.addData(second);

    result = hash.result();
    QCOMPARE(result, hash_firstsecond);

    hash.reset();
}


void tst_QCryptographicHash::sha1()
{
//  SHA1("abc") =
//      A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D
    QCOMPARE(QCryptographicHash::hash("abc", QCryptographicHash::Sha1).toHex().toUpper(),
             QByteArray("A9993E364706816ABA3E25717850C26C9CD0D89D"));
             
//  SHA1("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") =
//      84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1
    QCOMPARE(QCryptographicHash::hash("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
                                      QCryptographicHash::Sha1).toHex().toUpper(),
             QByteArray("84983E441C3BD26EBAAE4AA1F95129E5E54670F1"));
             
//  SHA1(A million repetitions of "a") =
//      34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
    QByteArray as;
    for (int i = 0; i < 1000000; ++i)
        as += 'a';
    QCOMPARE(QCryptographicHash::hash(as, QCryptographicHash::Sha1).toHex().toUpper(), 
             QByteArray("34AA973CD4C4DAA4F61EEB2BDBAD27316534016F"));
}


Q_DECLARE_METATYPE(QCryptographicHash::Algorithm);

void tst_QCryptographicHash::files_data() {
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QCryptographicHash::Algorithm>("algorithm");
    QTest::addColumn<QByteArray>("md5sum");
    QTest::newRow("Line") << QString::fromAscii("data/2c1517dad3678f03917f15849b052fd5.md5") << QCryptographicHash::Md5 << QByteArray("2c1517dad3678f03917f15849b052fd5");
    QTest::newRow("Line") << QString::fromAscii("data/d41d8cd98f00b204e9800998ecf8427e.md5") << QCryptographicHash::Md5 << QByteArray("d41d8cd98f00b204e9800998ecf8427e");
}


void tst_QCryptographicHash::files()
{
    QFETCH(QString, filename);
    QFETCH(QCryptographicHash::Algorithm, algorithm);
    QFETCH(QByteArray, md5sum);
    {
        QFile f(QString::fromLocal8Bit(SRCDIR) + filename);
        QCryptographicHash hash(algorithm);
        QVERIFY(! hash.addData(&f)); // file is not open for reading;
        if (f.open(QIODevice::ReadOnly)) {
            QVERIFY(hash.addData(&f));
            QCOMPARE(hash.result().toHex(),md5sum);
        } else {
            QFAIL("Failed to open file for testing. should not happen");
        }
    }
}


QTEST_MAIN(tst_QCryptographicHash)
#include "tst_qcryptographichash.moc"
