/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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


#include <QtCore/QCoreApplication>
#include <QTest>
#include <QScopeGuard>
#include <QCryptographicHash>
#include <QtCore/QMetaEnum>

#if QT_CONFIG(cxx11_future)
#  include <thread>
#endif

Q_DECLARE_METATYPE(QCryptographicHash::Algorithm)

class tst_QCryptographicHash : public QObject
{
    Q_OBJECT
private slots:
    void repeated_result_data();
    void repeated_result();
    void intermediary_result_data();
    void intermediary_result();
    void sha1();
    void sha3_data();
    void sha3();
    void blake2_data();
    void blake2();
    void files_data();
    void files();
    void hashLength_data();
    void hashLength();
    // keep last
    void moreThan4GiBOfData_data();
    void moreThan4GiBOfData();
private:
    std::vector<char> large;
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

    QTest::newRow("sha3_224_empty_abc")
            << int(QCryptographicHash::Sha3_224)
            << QByteArray("") << QByteArray("abc")
            << QByteArray::fromHex("6B4E03423667DBB73B6E15454F0EB1ABD4597F9A1B078E3F5B5A6BC7")
            << QByteArray::fromHex("E642824C3F8CF24AD09234EE7D3C766FC9A3A5168D0C94AD73B46FDF");
    QTest::newRow("sha3_256_empty_abc")
            << int(QCryptographicHash::Sha3_256)
            << QByteArray("") << QByteArray("abc")
            << QByteArray::fromHex("A7FFC6F8BF1ED76651C14756A061D662F580FF4DE43B49FA82D80A4B80F8434A")
            << QByteArray::fromHex("3A985DA74FE225B2045C172D6BD390BD855F086E3E9D525B46BFE24511431532");
    QTest::newRow("sha3_384_empty_abc")
            << int(QCryptographicHash::Sha3_384)
            << QByteArray("") << QByteArray("abc")
            << QByteArray::fromHex("0C63A75B845E4F7D01107D852E4C2485C51A50AAAA94FC61995E71BBEE983A2AC3713831264ADB47FB6BD1E058D5F004")
            << QByteArray::fromHex("EC01498288516FC926459F58E2C6AD8DF9B473CB0FC08C2596DA7CF0E49BE4B298D88CEA927AC7F539F1EDF228376D25");
    QTest::newRow("sha3_512_empty_abc")
            << int(QCryptographicHash::Sha3_512)
            << QByteArray("") << QByteArray("abc")
            << QByteArray::fromHex("A69F73CCA23A9AC5C8B567DC185A756E97C982164FE25859E0D1DCC1475C80A615B2123AF1F5F94C11E3E9402C3AC558F500199D95B6D3E301758586281DCD26")
            << QByteArray::fromHex("B751850B1A57168A5693CD924B6B096E08F621827444F70D884F5D0240D2712E10E116E9192AF3C91A7EC57647E3934057340B4CF408D5A56592F8274EEC53F0");

    QTest::newRow("sha3_224_abc_abc")
            << int(QCryptographicHash::Sha3_224)
            << QByteArray("abc") << QByteArray("abc")
            << QByteArray::fromHex("E642824C3F8CF24AD09234EE7D3C766FC9A3A5168D0C94AD73B46FDF")
            << QByteArray::fromHex("58F426458091E16FBC61DDCB8F2D2A6F30F729CAFA3C289A4EB2BCF8");
    QTest::newRow("sha3_256_abc_abc")
            << int(QCryptographicHash::Sha3_256)
            << QByteArray("abc") << QByteArray("abc")
            << QByteArray::fromHex("3A985DA74FE225B2045C172D6BD390BD855F086E3E9D525B46BFE24511431532")
            << QByteArray::fromHex("6C0872716337DE1EE664C1E37F64ADE109448F02681C63A912BC230FDEFC0058");
    QTest::newRow("sha3_384_abc_abc")
            << int(QCryptographicHash::Sha3_384)
            << QByteArray("abc") << QByteArray("abc")
            << QByteArray::fromHex("EC01498288516FC926459F58E2C6AD8DF9B473CB0FC08C2596DA7CF0E49BE4B298D88CEA927AC7F539F1EDF228376D25")
            << QByteArray::fromHex("34FA93E11E467D610524EC91CEDC848EE1395BCF8F4F987455478E63DB0BCE47194D33D1251A3CC32BBB18D8726040D0");
    QTest::newRow("sha3_512_abc_abc")
            << int(QCryptographicHash::Sha3_512)
            << QByteArray("abc") << QByteArray("abc")
            << QByteArray::fromHex("B751850B1A57168A5693CD924B6B096E08F621827444F70D884F5D0240D2712E10E116E9192AF3C91A7EC57647E3934057340B4CF408D5A56592F8274EEC53F0")
            << QByteArray::fromHex("BB582DA40D15399ACF62AFCBBD6CFC9EE1DD5129B1EF9935DD3B21668F1A73D7841018BE3B13F281C3A8E9DA7EDB60F57B9F9F1C04033DF4CE3654B7B2ADB310");
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

void tst_QCryptographicHash::sha3_data()
{
    QTest::addColumn<QCryptographicHash::Algorithm>("algorithm");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QByteArray>("expectedResult");

#define ROW(Tag, Algorithm, Input, Result) \
    QTest::newRow(Tag) << Algorithm << QByteArrayLiteral(Input) << QByteArray::fromHex(Result)

    ROW("sha3_224_pangram",
        QCryptographicHash::Sha3_224,
        "The quick brown fox jumps over the lazy dog",
        "d15dadceaa4d5d7bb3b48f446421d542e08ad8887305e28d58335795");

    ROW("sha3_224_pangram_dot",
        QCryptographicHash::Sha3_224,
        "The quick brown fox jumps over the lazy dog.",
        "2d0708903833afabdd232a20201176e8b58c5be8a6fe74265ac54db0");

    ROW("sha3_256_pangram",
        QCryptographicHash::Sha3_256,
        "The quick brown fox jumps over the lazy dog",
        "69070dda01975c8c120c3aada1b282394e7f032fa9cf32f4cb2259a0897dfc04");

    ROW("sha3_256_pangram_dot",
        QCryptographicHash::Sha3_256,
        "The quick brown fox jumps over the lazy dog.",
        "a80f839cd4f83f6c3dafc87feae470045e4eb0d366397d5c6ce34ba1739f734d");

    ROW("sha3_384_pangram",
        QCryptographicHash::Sha3_384,
        "The quick brown fox jumps over the lazy dog",
        "7063465e08a93bce31cd89d2e3ca8f602498696e253592ed26f07bf7e703cf328581e1471a7ba7ab119b1a9ebdf8be41");

    ROW("sha3_384_pangram_dot",
        QCryptographicHash::Sha3_384,
        "The quick brown fox jumps over the lazy dog.",
        "1a34d81695b622df178bc74df7124fe12fac0f64ba5250b78b99c1273d4b080168e10652894ecad5f1f4d5b965437fb9");

    ROW("sha3_512_pangram",
        QCryptographicHash::Sha3_512,
        "The quick brown fox jumps over the lazy dog",
        "01dedd5de4ef14642445ba5f5b97c15e47b9ad931326e4b0727cd94cefc44fff23f07bf543139939b49128caf436dc1bdee54fcb24023a08d9403f9b4bf0d450");

    ROW("sha3_512_pangram_dot",
        QCryptographicHash::Sha3_512,
        "The quick brown fox jumps over the lazy dog.",
        "18f4f4bd419603f95538837003d9d254c26c23765565162247483f65c50303597bc9ce4d289f21d1c2f1f458828e33dc442100331b35e7eb031b5d38ba6460f8");

#undef ROW
}

void tst_QCryptographicHash::sha3()
{
    QFETCH(QCryptographicHash::Algorithm, algorithm);
    QFETCH(QByteArray, data);
    QFETCH(QByteArray, expectedResult);

    const auto result = QCryptographicHash::hash(data, algorithm);
    QCOMPARE(result, expectedResult);
}

void tst_QCryptographicHash::blake2_data()
{
    QTest::addColumn<QCryptographicHash::Algorithm>("algorithm");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QByteArray>("expectedResult");

#define ROW(Tag, Algorithm, Input, Result) \
    QTest::newRow(Tag) << Algorithm << QByteArrayLiteral(Input) << QByteArray::fromHex(Result)

    // BLAKE2b
    ROW("blake2b_160_pangram",
        QCryptographicHash::Blake2b_160,
        "The quick brown fox jumps over the lazy dog",
        "3c523ed102ab45a37d54f5610d5a983162fde84f");

    ROW("blake2b_160_pangram_dot",
        QCryptographicHash::Blake2b_160,
        "The quick brown fox jumps over the lazy dog.",
        "d0c8bb0bdd830296d1d4f4348176699ccccc16bb");

    ROW("blake2b_256_pangram",
        QCryptographicHash::Blake2b_256,
        "The quick brown fox jumps over the lazy dog",
        "01718cec35cd3d796dd00020e0bfecb473ad23457d063b75eff29c0ffa2e58a9");

    ROW("blake2b_256_pangram_dot",
        QCryptographicHash::Blake2b_256,
        "The quick brown fox jumps over the lazy dog.",
        "69d7d3b0afba81826d27024c17f7f183659ed0812cf27b382eaef9fdc29b5712");

    ROW("blake2b_384_pangram",
        QCryptographicHash::Blake2b_384,
        "The quick brown fox jumps over the lazy dog",
        "b7c81b228b6bd912930e8f0b5387989691c1cee1e65aade4da3b86a3c9f678fc8018f6ed9e2906720c8d2a3aeda9c03d");

    ROW("blake2b_384_pangram_dot",
        QCryptographicHash::Blake2b_384,
        "The quick brown fox jumps over the lazy dog.",
        "16d65de1a3caf1c26247234c39af636284c7e19ca448c0de788272081410778852c94d9cef6b939968d4f872c7f78337");

    ROW("blake2b_512_pangram",
        QCryptographicHash::Blake2b_512,
        "The quick brown fox jumps over the lazy dog",
        "a8add4bdddfd93e4877d2746e62817b116364a1fa7bc148d95090bc7333b3673f82401cf7aa2e4cb1ecd90296e3f14cb5413f8ed77be73045b13914cdcd6a918");

    ROW("blake2b_512_pangram_dot",
        QCryptographicHash::Blake2b_512,
        "The quick brown fox jumps over the lazy dog.",
        "87af9dc4afe5651b7aa89124b905fd214bf17c79af58610db86a0fb1e0194622a4e9d8e395b352223a8183b0d421c0994b98286cbf8c68a495902e0fe6e2bda2");

    // BLAKE2s
    ROW("blake2s_128_pangram",
        QCryptographicHash::Blake2s_128,
        "The quick brown fox jumps over the lazy dog",
        "96fd07258925748a0d2fb1c8a1167a73");

    ROW("blake2s_128_pangram_dot",
        QCryptographicHash::Blake2s_128,
        "The quick brown fox jumps over the lazy dog.",
        "1f298f2e1f9c2490e506c2308f64e7c0");

    ROW("blake2s_160_pangram",
        QCryptographicHash::Blake2s_160,
        "The quick brown fox jumps over the lazy dog",
        "5a604fec9713c369e84b0ed68daed7d7504ef240");

    ROW("blake2s_160_pangram_dot",
        QCryptographicHash::Blake2s_160,
        "The quick brown fox jumps over the lazy dog.",
        "cd4a863226463aac852662d16275d399966e3ffe");

    ROW("blake2s_224_pangram",
        QCryptographicHash::Blake2s_224,
        "The quick brown fox jumps over the lazy dog",
        "e4e5cb6c7cae41982b397bf7b7d2d9d1949823ae78435326e8db4912");

    ROW("blake2s_224_pangram_dot",
        QCryptographicHash::Blake2s_224,
        "The quick brown fox jumps over the lazy dog.",
        "fd1557500ef49f308882969507acd18a13e155c26f8fcd82f9bf2ff7");

    ROW("blake2s_256_pangram",
        QCryptographicHash::Blake2s_256,
        "The quick brown fox jumps over the lazy dog",
        "606beeec743ccbeff6cbcdf5d5302aa855c256c29b88c8ed331ea1a6bf3c8812");

    ROW("blake2s_256_pangram_dot",
        QCryptographicHash::Blake2s_256,
        "The quick brown fox jumps over the lazy dog.",
        "95bca6e1b761dca1323505cc629949a0e03edf11633cc7935bd8b56f393afcf2");

#undef ROW
}

void tst_QCryptographicHash::blake2()
{
    QFETCH(QCryptographicHash::Algorithm, algorithm);
    QFETCH(QByteArray, data);
    QFETCH(QByteArray, expectedResult);

    const auto result = QCryptographicHash::hash(data, algorithm);
    QCOMPARE(result, expectedResult);
}

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

void tst_QCryptographicHash::hashLength_data()
{
    QTest::addColumn<QCryptographicHash::Algorithm>("algorithm");
    auto metaEnum = QMetaEnum::fromType<QCryptographicHash::Algorithm>();
    for (int i = 0, value = metaEnum.value(i); value != -1; value = metaEnum.value(++i)) {
        auto algorithm = QCryptographicHash::Algorithm(value);
        QTest::addRow("%s", metaEnum.valueToKey(value)) << algorithm;
    }
}

void tst_QCryptographicHash::hashLength()
{
    QFETCH(const QCryptographicHash::Algorithm, algorithm);

    QByteArray output = QCryptographicHash::hash(QByteArrayLiteral("test"), algorithm);
    QCOMPARE(QCryptographicHash::hashLength(algorithm), output.length());
}

void tst_QCryptographicHash::moreThan4GiBOfData_data()
{
#if QT_POINTER_SIZE > 4
    QElapsedTimer timer;
    timer.start();
    const size_t GiB = 1024 * 1024 * 1024;
    try {
        large.resize(4 * GiB + 1, '\0');
    } catch (const std::bad_alloc &) {
        QSKIP("Could not allocate 4GiB plus one byte of RAM.");
    }
    QCOMPARE(large.size(), 4 * GiB + 1);
    large.back() = '\1';
    qDebug("created dataset in %lld ms", timer.elapsed());

    QTest::addColumn<QCryptographicHash::Algorithm>("algorithm");
    auto me = QMetaEnum::fromType<QCryptographicHash::Algorithm>();
    auto row = [me] (QCryptographicHash::Algorithm algo) {
        QTest::addRow("%s", me.valueToKey(int(algo))) << algo;
    };
    // these are reasonably fast (O(secs))
    row(QCryptographicHash::Md4);
    row(QCryptographicHash::Md5);
    row(QCryptographicHash::Sha1);
    if (!qgetenv("QTEST_ENVIRONMENT").split(' ').contains("ci")) {
        // This is important but so slow (O(minute)) that, on CI, it tends to time out.
        // Retain it for manual runs, all the same, as most dev machines will be fast enough.
        row(QCryptographicHash::Sha512);
    }
    // the rest is just too slow
#else
    QSKIP("This test is 64-bit only.");
#endif
}

void tst_QCryptographicHash::moreThan4GiBOfData()
{
    QFETCH(const QCryptographicHash::Algorithm, algorithm);

# if QT_CONFIG(cxx11_future)
    using MaybeThread = std::thread;
# else
    struct MaybeThread {
        std::function<void()> func;
        void join() { func(); }
    };
# endif

    QElapsedTimer timer;
    timer.start();
    const auto sg = qScopeGuard([&] {
        qDebug() << algorithm << "test finished in" << timer.restart() << "ms";
    });

    const auto begin = large.data();
    const auto mid = begin + large.size() / 2;
    const auto end = begin + large.size();

    QByteArray single;
    QByteArray chunked;

    auto t = MaybeThread{[&] {
        QCryptographicHash h(algorithm);
        h.addData(begin, end - begin);
        single = h.result();
    }};
    {
        QCryptographicHash h(algorithm);
        h.addData(begin, mid - begin);
        h.addData(mid, end - mid);
        chunked = h.result();
    }
    t.join();

    QCOMPARE(single, chunked);
}

QTEST_MAIN(tst_QCryptographicHash)
#include "tst_qcryptographichash.moc"
