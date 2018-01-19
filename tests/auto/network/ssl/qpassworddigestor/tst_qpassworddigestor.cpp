/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtNetwork/qpassworddigestor.h>
#include <QtCore/QByteArray>

class tst_QPasswordDigestor : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void pbkdf1Vectors_data();
    void pbkdf1Vectors();
    void pbkdf2Vectors_data();
    void pbkdf2Vectors();
};

void tst_QPasswordDigestor::pbkdf1Vectors_data()
{
    QTest::addColumn<QCryptographicHash::Algorithm>("algorithm");
    QTest::addColumn<QByteArray>("password");
    QTest::addColumn<QByteArray>("salt");
    QTest::addColumn<int>("iterations");
    QTest::addColumn<int>("dkLen");
    QTest::addColumn<QByteArray>("result");

    // data from
    // https://web.archive.org/web/20160912052752/https://www.di-mgt.com.au/cryptoKDFs.html#examplespbkdf
    // (Note: this is not official, but at least it's something to compare with.)
    QTest::newRow("di-mgt") << QCryptographicHash::Sha1 << QByteArray::fromHex("70617373776F7264")
                            << QByteArray::fromHex("78578E5A5D63CB06") << 1000 << 16
                            << QByteArray::fromHex("DC19847E05C64D2FAF10EBFB4A3D2A20");
}

void tst_QPasswordDigestor::pbkdf1Vectors()
{
    QFETCH(QCryptographicHash::Algorithm, algorithm);
    QFETCH(QByteArray, password);
    QFETCH(QByteArray, salt);
    QFETCH(int, iterations);
    QFETCH(int, dkLen);
    QFETCH(QByteArray, result);

    QCOMPARE(QPasswordDigestor::deriveKeyPbkdf1(algorithm, password, salt, iterations, dkLen), result);
}

void tst_QPasswordDigestor::pbkdf2Vectors_data()
{
    QTest::addColumn<QCryptographicHash::Algorithm>("algorithm");
    QTest::addColumn<QByteArray>("password");
    QTest::addColumn<QByteArray>("salt");
    QTest::addColumn<int>("iterations");
    QTest::addColumn<int>("dkLen");
    QTest::addColumn<QByteArray>("result");

    // data from https://tools.ietf.org/html/rfc6070
    auto hash = QCryptographicHash::Sha1;
    QTest::newRow("rfc6070-1") << hash << QByteArrayLiteral("password") << QByteArrayLiteral("salt")
                               << 1 << 20
                               << QByteArray::fromHex("0c60c80f961f0e71f3a9b524af6012062fe037a6");
    QTest::newRow("rfc6070-2") << hash << QByteArrayLiteral("password") << QByteArrayLiteral("salt")
                               << 2 << 20
                               << QByteArray::fromHex("ea6c014dc72d6f8ccd1ed92ace1d41f0d8de8957");
    QTest::newRow("rfc6070-3") << hash << QByteArrayLiteral("password") << QByteArrayLiteral("salt")
                               << 4096 << 20
                               << QByteArray::fromHex("4b007901b765489abead49d926f721d065a429c1");
#if 0
    // Excluding: takes about 3 minutes to run
    QTest::newRow("rfc6070-4") << hash << QByteArrayLiteral("password") << QByteArrayLiteral("salt")
                               << 16777216 << 20
                               << QByteArray::fromHex("eefe3d61cd4da4e4e9945b3d6ba2158c2634e984");
#endif
    QTest::newRow("rfc6070-5") << hash << QByteArrayLiteral("passwordPASSWORDpassword")
                               << QByteArrayLiteral("saltSALTsaltSALTsaltSALTsaltSALTsalt") << 4096
                               << 25
                               << QByteArray::fromHex(
                                      "3d2eec4fe41c849b80c8d83662c0e44a8b291a964cf2f07038");
    QTest::newRow("rfc6070-6") << hash << QByteArrayLiteral("pass\0word")
                               << QByteArrayLiteral("sa\0lt") << 4096 << 16
                               << QByteArray::fromHex("56fa6aa75548099dcc37d7f03425e0c3");

    // the next few bits of data are from https://tools.ietf.org/html/rfc3962#appendix-B
    QByteArray password = QByteArrayLiteral("password");
    QByteArray salt = QByteArrayLiteral("ATHENA.MIT.EDUraeburn");
    QTest::newRow("rfc3962-1") << hash << password << salt << 1 << 16
                               << QByteArray::fromHex("cdedb5281bb2f801565a1122b2563515");
    QTest::newRow("rfc3962-2")
        << hash << password << salt << 1 << 32
        << QByteArray::fromHex("cdedb5281bb2f801565a1122b25635150ad1f7a04bb9f3a333ecc0e2e1f70837");
    QTest::newRow("rfc3962-3") << hash << password << salt << 2 << 16
                               << QByteArray::fromHex("01dbee7f4a9e243e988b62c73cda935d");
    QTest::newRow("rfc3962-4")
        << hash << QByteArrayLiteral("password") << salt << 2 << 32
        << QByteArray::fromHex("01dbee7f4a9e243e988b62c73cda935da05378b93244ec8f48a99e61ad799d86");
    QTest::newRow("rfc3962-5") << hash << password << salt << 1200 << 16
                               << QByteArray::fromHex("5c08eb61fdf71e4e4ec3cf6ba1f5512b");
    QTest::newRow("rfc3962-6")
        << hash << password << salt << 1200 << 32
        << QByteArray::fromHex("5c08eb61fdf71e4e4ec3cf6ba1f5512ba7e52ddbc5e5142f708a31e2e62b1e13");

    salt = QByteArray::fromHex("1234567878563412"); // 0x1234567878563412
    QTest::newRow("rfc3962-7") << hash << password << salt << 5 << 16
                               << QByteArray::fromHex("d1daa78615f287e6a1c8b120d7062a49");
    QTest::newRow("rfc3962-8")
        << hash << password << salt << 5 << 32
        << QByteArray::fromHex("d1daa78615f287e6a1c8b120d7062a493f98d203e6be49a6adf4fa574b6e64ee");

    password = QByteArray(64, 'X');
    salt = "pass phrase equals block size";
    QTest::newRow("rfc3962-9") << hash << password << salt << 1200 << 16
                               << QByteArray::fromHex("139c30c0966bc32ba55fdbf212530ac9");
    QTest::newRow("rfc3962-10")
        << hash << password << salt << 1200 << 32
        << QByteArray::fromHex("139c30c0966bc32ba55fdbf212530ac9c5ec59f1a452f5cc9ad940fea0598ed1");

    password.append('X');
    salt = "pass phrase exceeds block size";
    QTest::newRow("rfc3962-11") << hash << password << salt << 1200 << 16
                                << QByteArray::fromHex("9ccad6d468770cd51b10e6a68721be61");
    QTest::newRow("rfc3962-12")
        << hash << password << salt << 1200 << 32
        << QByteArray::fromHex("9ccad6d468770cd51b10e6a68721be611a8b4d282601db3b36be9246915ec82a");

    password = QByteArray::fromHex("f09d849e"); // 0xf09d849e
    salt = "EXAMPLE.COMpianist";
    QTest::newRow("rfc3962-13") << hash << password << salt << 50 << 16
                                << QByteArray::fromHex("6b9cf26d45455a43a5b8bb276a403b39");
    QTest::newRow("rfc3962-14")
        << hash << password << salt << 50 << 32
        << QByteArray::fromHex("6b9cf26d45455a43a5b8bb276a403b39e7fe37a0c41e02c281ff3069e1e94f52");
}

void tst_QPasswordDigestor::pbkdf2Vectors()
{
    QFETCH(QCryptographicHash::Algorithm, algorithm);
    QFETCH(QByteArray, password);
    QFETCH(QByteArray, salt);
    QFETCH(int, iterations);
    QFETCH(int, dkLen);
    QFETCH(QByteArray, result);

    QCOMPARE(QPasswordDigestor::deriveKeyPbkdf2(algorithm, password, salt, iterations, dkLen), result);
}

QTEST_MAIN(tst_QPasswordDigestor)
#include "tst_qpassworddigestor.moc"
