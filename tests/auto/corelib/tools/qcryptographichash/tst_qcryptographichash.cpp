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


#include <QtCore/QCoreApplication>
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
    void sha3();
    void files_data();
    void files();
};

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
    QTest::newRow("sha224") << int(QCryptographicHash::Sha224)
                            << QByteArray("abc") << QByteArray("abc")
                            << QByteArray::fromHex("23097D223405D8228642A477BDA255B32AADBCE4BDA0B3F7E36C9DA7")
                            << QByteArray::fromHex("7C9C91FC479626AA1A525301084DEB96716131D146A2DB61B533F4C9");
    QTest::newRow("sha256") << int(QCryptographicHash::Sha256)
                            << QByteArray("abc") << QByteArray("abc")
                            << QByteArray::fromHex("BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD")
                            << QByteArray::fromHex("BBB59DA3AF939F7AF5F360F2CEB80A496E3BAE1CD87DDE426DB0AE40677E1C2C");
    QTest::newRow("sha384") << int(QCryptographicHash::Sha384)
                            << QByteArray("abc") << QByteArray("abc")
                            << QByteArray::fromHex("CB00753F45A35E8BB5A03D699AC65007272C32AB0EDED1631A8B605A43FF5BED8086072BA1E7CC2358BAECA134C825A7")
                            << QByteArray::fromHex("CAF33A735C9535CE7F5D24FB5B3A4834F0E9316664AD15A9E8221679D4A3B4FB7E962404BA0C10C1D43AB49D03A08B8D");
    QTest::newRow("sha512") << int(QCryptographicHash::Sha512)
                            << QByteArray("abc") << QByteArray("abc")
                            << QByteArray::fromHex("DDAF35A193617ABACC417349AE20413112E6FA4E89A97EA20A9EEEE64B55D39A2192992A274FC1A836BA3C23A3FEEBBD454D4423643CE80E2A9AC94FA54CA49F")
                            << QByteArray::fromHex("F3C41E7B63EE869596FC28BAD64120612C520F65928AB4D126C72C6998B551B8FF1CEDDFED4373E6717554DC89D1EEE6F0AB22FD3675E561ABA9AE26A3EEC53B");

    QTest::newRow("sha3_224") << int(QCryptographicHash::Sha3_224)
                         << QByteArray("abc") << QByteArray("abc")
                         << QByteArray::fromHex("C30411768506EBE1C2871B1EE2E87D38DF342317300A9B97A95EC6A8")
                         << QByteArray::fromHex("048330E7C7C8B4A41AB713B3A6F958D77B8CF3EE969930F1584DD550");
    QTest::newRow("sha3_256") << int(QCryptographicHash::Sha3_256)
                         << QByteArray("abc") << QByteArray("abc")
                         << QByteArray::fromHex("4E03657AEA45A94FC7D47BA826C8D667C0D1E6E33A64A036EC44F58FA12D6C45")
                         << QByteArray::fromHex("9F0ADAD0A59B05D2E04A1373342B10B9EB16C57C164C8A3BFCBF46DCCEE39A21");
    QTest::newRow("sha3_384") << int(QCryptographicHash::Sha3_384)
                         << QByteArray("abc") << QByteArray("abc")
                         << QByteArray::fromHex("F7DF1165F033337BE098E7D288AD6A2F74409D7A60B49C36642218DE161B1F99F8C681E4AFAF31A34DB29FB763E3C28E")
                         << QByteArray::fromHex("D733B87D392D270889D3DA23AE113F349E25574B445F319CDE4CD3F877C753E9E3C65980421339B3A131457FF393939F");
    QTest::newRow("sha3_512") << int(QCryptographicHash::Sha3_512)
                         << QByteArray("abc") << QByteArray("abc")
                         << QByteArray::fromHex("18587DC2EA106B9A1563E32B3312421CA164C7F1F07BC922A9C83D77CEA3A1E5D0C69910739025372DC14AC9642629379540C17E2A65B19D77AA511A9D00BB96")
                         << QByteArray::fromHex("A7C392D2A42155761CA76BDDDE1C47D55486B007EDF465397BFB9DFA74D11C8F0D7C86CD29415283F1B5E7F655CEC25B869C9E9C33A8986F0B38542FB12BFB93");
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

void tst_QCryptographicHash::sha3()
{
    // SHA3-224("The quick brown fox jumps over the lazy dog")
    // 10aee6b30c47350576ac2873fa89fd190cdc488442f3ef654cf23fe
    QCOMPARE(QCryptographicHash::hash("The quick brown fox jumps over the lazy dog",
                                      QCryptographicHash::Sha3_224).toHex(),
             QByteArray("310aee6b30c47350576ac2873fa89fd190cdc488442f3ef654cf23fe"));
    // SHA3-224("The quick brown fox jumps over the lazy dog.")
    // c59d4eaeac728671c635ff645014e2afa935bebffdb5fbd207ffdeab
    QCOMPARE(QCryptographicHash::hash("The quick brown fox jumps over the lazy dog.",
                                      QCryptographicHash::Sha3_224).toHex(),
             QByteArray("c59d4eaeac728671c635ff645014e2afa935bebffdb5fbd207ffdeab"));

    // SHA3-256("The quick brown fox jumps over the lazy dog")
    // 4d741b6f1eb29cb2a9b9911c82f56fa8d73b04959d3d9d222895df6c0b28aa15
    QCOMPARE(QCryptographicHash::hash("The quick brown fox jumps over the lazy dog",
                                      QCryptographicHash::Sha3_256).toHex(),
             QByteArray("4d741b6f1eb29cb2a9b9911c82f56fa8d73b04959d3d9d222895df6c0b28aa15"));

    // SHA3-256("The quick brown fox jumps over the lazy dog.")
    // 578951e24efd62a3d63a86f7cd19aaa53c898fe287d2552133220370240b572d
    QCOMPARE(QCryptographicHash::hash("The quick brown fox jumps over the lazy dog.",
                                      QCryptographicHash::Sha3_256).toHex(),
             QByteArray("578951e24efd62a3d63a86f7cd19aaa53c898fe287d2552133220370240b572d"));

    // SHA3-384("The quick brown fox jumps over the lazy dog")
    // 283990fa9d5fb731d786c5bbee94ea4db4910f18c62c03d173fc0a5e494422e8a0b3da7574dae7fa0baf005e504063b3
    QCOMPARE(QCryptographicHash::hash("The quick brown fox jumps over the lazy dog",
                                      QCryptographicHash::Sha3_384).toHex(),
             QByteArray("283990fa9d5fb731d786c5bbee94ea4db4910f18c62c03d173fc0a5e494422e8a0b3da7574dae7fa0baf005e504063b3"));

    // SHA3-384("The quick brown fox jumps over the lazy dog.")
    // 9ad8e17325408eddb6edee6147f13856ad819bb7532668b605a24a2d958f88bd5c169e56dc4b2f89ffd325f6006d820b
    QCOMPARE(QCryptographicHash::hash("The quick brown fox jumps over the lazy dog.",
                                      QCryptographicHash::Sha3_384).toHex(),
             QByteArray("9ad8e17325408eddb6edee6147f13856ad819bb7532668b605a24a2d958f88bd5c169e56dc4b2f89ffd325f6006d820b"));

    // SHA3-512("The quick brown fox jumps over the lazy dog")
    // d135bb84d0439dbac432247ee573a23ea7d3c9deb2a968eb31d47c4fb45f1ef4422d6c531b5b9bd6f449ebcc449ea94d0a8f05f62130fda612da53c79659f609
    QCOMPARE(QCryptographicHash::hash("The quick brown fox jumps over the lazy dog",
                                      QCryptographicHash::Sha3_512).toHex(),
             QByteArray("d135bb84d0439dbac432247ee573a23ea7d3c9deb2a968eb31d47c4fb45f1ef4422d6c531b5b9bd6f449ebcc449ea94d0a8f05f62130fda612da53c79659f609"));

    // SHA3-512("The quick brown fox jumps over the lazy dog.")
    // ab7192d2b11f51c7dd744e7b3441febf397ca07bf812cceae122ca4ded6387889064f8db9230f173f6d1ab6e24b6e50f065b039f799f5592360a6558eb52d760
    QCOMPARE(QCryptographicHash::hash("The quick brown fox jumps over the lazy dog.",
                                      QCryptographicHash::Sha3_512).toHex(),
             QByteArray("ab7192d2b11f51c7dd744e7b3441febf397ca07bf812cceae122ca4ded6387889064f8db9230f173f6d1ab6e24b6e50f065b039f799f5592360a6558eb52d760"));
}

Q_DECLARE_METATYPE(QCryptographicHash::Algorithm);

void tst_QCryptographicHash::files_data() {
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QCryptographicHash::Algorithm>("algorithm");
    QTest::addColumn<QByteArray>("md5sum");
    QTest::newRow("data1") << QString::fromLatin1("data/2c1517dad3678f03917f15849b052fd5.md5") << QCryptographicHash::Md5 << QByteArray("2c1517dad3678f03917f15849b052fd5");
    QTest::newRow("data2") << QString::fromLatin1("data/d41d8cd98f00b204e9800998ecf8427e.md5") << QCryptographicHash::Md5 << QByteArray("d41d8cd98f00b204e9800998ecf8427e");
}


void tst_QCryptographicHash::files()
{
    QFETCH(QString, filename);
    QFETCH(QCryptographicHash::Algorithm, algorithm);
    QFETCH(QByteArray, md5sum);
    {
        QString testData = QFINDTESTDATA(filename);
        QVERIFY2(!testData.isEmpty(), qPrintable(QString("Cannot find test data: %1").arg(filename)));
        QFile f(testData);
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
