// Copyright (C) 2013 Ruslan Nigmatullin <euroelessar@yandex.ru>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtCore/QCoreApplication>
#include <QTest>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QBuffer>

class tst_QMessageAuthenticationCode : public QObject
{
    Q_OBJECT
private slots:
    void repeated_setKey_data();
    void repeated_setKey();
    void result_data();
    void result();
    void result_incremental_data();
    void result_incremental();
    void addData_overloads_data();
    void addData_overloads();
    void move();
    void swap();
};

Q_DECLARE_METATYPE(QCryptographicHash::Algorithm)

void tst_QMessageAuthenticationCode::repeated_setKey_data()
{
    using A = QCryptographicHash::Algorithm;
    QTest::addColumn<A>("algo");

    const auto me = QMetaEnum::fromType<A>();
    for (int i = 0, value; (value = me.value(i)) != -1; ++i)
        QTest::addRow("%s", me.key(i)) << A(value);
}

void tst_QMessageAuthenticationCode::repeated_setKey()
{
    QFETCH(const QCryptographicHash::Algorithm, algo);

    if (!QCryptographicHash::supportsAlgorithm(algo))
        QSKIP("QCryptographicHash doesn't support this algorithm");

    // GIVEN: two long keys, so we're sure the key needs to be hashed in order
    //        to fit into the hash algorithm's block

    static const QByteArray key1(1024, 'a');
    static const QByteArray key2(2048, 'b');

    // WHEN: processing the same message

    QMessageAuthenticationCode macX(algo);
    QMessageAuthenticationCode mac1(algo, key1);
    QMessageAuthenticationCode mac2(algo, key2);

    const auto check = [](QMessageAuthenticationCode &mac) {
        mac.addData("This is nonsense, ignore it, please.");
        return mac.result();
    };

    macX.setKey(key1);
    QCOMPARE(check(macX), check(mac1));

    // THEN: the result does not depend on whether a new QMAC instance was used
    //       or an old one re-used (iow: setKey() reset()s)

    macX.setKey(key2);
    QCOMPARE(check(macX), check(mac2));
}

void tst_QMessageAuthenticationCode::result_data()
{
    QTest::addColumn<QCryptographicHash::Algorithm>("algo");
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("message");
    QTest::addColumn<QByteArray>("code");

    // Empty values
    QTest::newRow("md5-empty") << QCryptographicHash::Md5
                               << QByteArray()
                               << QByteArray()
                               << QByteArray::fromHex("74e6f7298a9c2d168935f58c001bad88");
    QTest::newRow("sha1-empty") << QCryptographicHash::Sha1
                                << QByteArray()
                                << QByteArray()
                                << QByteArray::fromHex("fbdb1d1b18aa6c08324b7d64b71fb76370690e1d");
    QTest::newRow("sha256-empty") << QCryptographicHash::Sha256
                                  << QByteArray()
                                  << QByteArray()
                                  << QByteArray::fromHex("b613679a0814d9ec772f95d778c35fc5ff1697c493715653c6c712144292c5ad");
    QTest::newRow("sha384-empty") << QCryptographicHash::Sha384 << QByteArray() << QByteArray()
                                  << QByteArray::fromHex(
                                             "6c1f2ee938fad2e24bd91298474382ca218c75db3d83e114b3d43"
                                             "67776d14d3551289e75e8209cd4b792302840234adc");
    QTest::newRow("sha512-empty")
            << QCryptographicHash::Sha512 << QByteArray() << QByteArray()
            << QByteArray::fromHex(
                       "b936cee86c9f87aa5d3c6f2e84cb5a4239a5fe50480a6ec66b70ab5b1f4ac6730c6c515421b"
                       "327ec1d69402e53dfb49ad7381eb067b338fd7b0cb22247225d47");

    // Some not-empty
    QTest::newRow("md5") << QCryptographicHash::Md5
                         << QByteArray("key")
                         << QByteArray("The quick brown fox jumps over the lazy dog")
                         << QByteArray::fromHex("80070713463e7749b90c2dc24911e275");
    QTest::newRow("sha1") << QCryptographicHash::Sha1
                          << QByteArray("key")
                          << QByteArray("The quick brown fox jumps over the lazy dog")
                          << QByteArray::fromHex("de7c9b85b8b78aa6bc8a7a36f70a90701c9db4d9");
    QTest::newRow("sha256") << QCryptographicHash::Sha256
                            << QByteArray("key")
                            << QByteArray("The quick brown fox jumps over the lazy dog")
                            << QByteArray::fromHex("f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8");
    QTest::newRow("sha384") << QCryptographicHash::Sha384 << QByteArray("key")
                            << QByteArray("The quick brown fox jumps over the lazy dog")
                            << QByteArray::fromHex(
                                       "d7f4727e2c0b39ae0f1e40cc96f60242d5b7801841cea6fc592c5d3e1ae"
                                       "50700582a96cf35e1e554995fe4e03381c237");
    QTest::newRow("sha512")
            << QCryptographicHash::Sha512 << QByteArray("key")
            << QByteArray("The quick brown fox jumps over the lazy dog")
            << QByteArray::fromHex(
                       "b42af09057bac1e2d41708e48a902e09b5ff7f12ab428a4fe86653c73dd248fb82f948a549f"
                       "7b791a5b41915ee4d1ec3935357e4e2317250d0372afa2ebeeb3a");

    // Some from rfc-2104
    QTest::newRow("rfc-md5-1") << QCryptographicHash::Md5
                               << QByteArray::fromHex("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b")
                               << QByteArray("Hi There")
                               << QByteArray::fromHex("9294727a3638bb1c13f48ef8158bfc9d");
    QTest::newRow("rfc-md5-2") << QCryptographicHash::Md5
                               << QByteArray("Jefe")
                               << QByteArray("what do ya want for nothing?")
                               << QByteArray::fromHex("750c783e6ab0b503eaa86e310a5db738");
    QTest::newRow("rfc-md5-3") << QCryptographicHash::Md5
                               << QByteArray::fromHex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA")
                               << QByteArray(50, char(0xdd))
                               << QByteArray::fromHex("56be34521d144c88dbb8c733f0e8b3f6");
}

void tst_QMessageAuthenticationCode::result()
{
    QFETCH(QCryptographicHash::Algorithm, algo);
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, message);
    QFETCH(QByteArray, code);

    QMessageAuthenticationCode mac(algo, key);
    mac.addData(message);
    QByteArrayView resultView = mac.resultView();

    QCOMPARE(resultView, code);

    const auto result = QMessageAuthenticationCode::hash(message, key, algo);
    QCOMPARE(result, code);
}

void tst_QMessageAuthenticationCode::result_incremental_data()
{
    result_data();
}

void tst_QMessageAuthenticationCode::result_incremental()
{
    QFETCH(QCryptographicHash::Algorithm, algo);
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, message);
    QFETCH(QByteArray, code);

    int index = message.size() / 2;
    QByteArray leftPart(message.mid(0, index));
    QByteArray rightPart(message.mid(index));

    QCOMPARE(leftPart + rightPart, message);

    QMessageAuthenticationCode mac(algo, key);
    mac.addData(leftPart);
    mac.addData(rightPart);
    QByteArrayView result = mac.resultView();

    QCOMPARE(result, code);
}

void tst_QMessageAuthenticationCode::addData_overloads_data()
{
    result_data();
}

void tst_QMessageAuthenticationCode::addData_overloads()
{
    QFETCH(QCryptographicHash::Algorithm, algo);
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, message);
    QFETCH(QByteArray, code);

    // overload using const char* and length
    {
        QMessageAuthenticationCode mac(algo);
        mac.setKey(key);
        mac.addData(message.constData(), message.size());
        QByteArrayView result = mac.resultView();

        QCOMPARE(result, code);
    }

    // overload using QIODevice
    {
        QBuffer buffer(&message);
        buffer.open(QIODevice::ReadOnly);
        QMessageAuthenticationCode mac(algo);
        mac.setKey(key);
        QVERIFY(mac.addData(&buffer));
        QByteArrayView result = mac.resultView();
        buffer.close();

        QCOMPARE(result, code);
    }
}

void tst_QMessageAuthenticationCode::move()
{
    const QByteArray key = "123";

    QMessageAuthenticationCode src(QCryptographicHash::Sha1, key);
    src.addData("a");

    // move constructor
    auto intermediary = std::move(src);
    intermediary.addData("b");

    // move assign operator
    QMessageAuthenticationCode dst(QCryptographicHash::Sha256, key);
    dst.addData("no effect on the end result");
    dst = std::move(intermediary);
    dst.addData("c");

    QCOMPARE(dst.resultView(),
             QMessageAuthenticationCode::hash("abc", key, QCryptographicHash::Sha1));
}

void tst_QMessageAuthenticationCode::swap()
{
    const QByteArray key1 = "123";
    const QByteArray key2 = "abcdefg";

    QMessageAuthenticationCode mac1(QCryptographicHash::Sha1,   key1);
    QMessageAuthenticationCode mac2(QCryptographicHash::Sha256, key2);

    mac1.addData("da");
    mac2.addData("te");

    mac1.swap(mac2);

    mac2.addData("ta");
    mac1.addData("st");

    QCOMPARE(mac2.resultView(),
             QMessageAuthenticationCode::hash("data", key1, QCryptographicHash::Sha1));
    QCOMPARE(mac1.resultView(),
             QMessageAuthenticationCode::hash("test", key2, QCryptographicHash::Sha256));
}

QTEST_MAIN(tst_QMessageAuthenticationCode)
#include "tst_qmessageauthenticationcode.moc"
