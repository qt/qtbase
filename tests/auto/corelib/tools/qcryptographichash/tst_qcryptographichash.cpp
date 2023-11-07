// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


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
    void keccak();
    void keccak_data();
    void blake2_data();
    void blake2();
    void files_data();
    void files();
    void hashLength_data();
    void hashLength();
    void addDataAcceptsNullByteArrayView_data() { hashLength_data(); }
    void addDataAcceptsNullByteArrayView();
    void move();
    void swap();
    // keep last
    void moreThan4GiBOfData_data();
    void moreThan4GiBOfData();
    void keccakBufferOverflow();
private:
    void ensureLargeData();
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

    QCOMPARE_EQ(hash.algorithm(), _algo);

    QFETCH(QByteArray, first);
    hash.addData(first);

    QFETCH(QByteArray, hash_first);
    QByteArrayView result = hash.resultView();
    QCOMPARE(result, hash_first);
    QCOMPARE(result, hash.resultView());
    QCOMPARE(result, hash.result());

    hash.reset();
    hash.addData(first);
    result = hash.resultView();
    QCOMPARE(result, hash_first);
    QCOMPARE(result, hash.result());
    QCOMPARE(result, hash.resultView());
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

    QTest::newRow("keccak_224_abc_abc")
            << int(QCryptographicHash::Keccak_224)
            << QByteArray("abc") << QByteArray("abc")
            << QByteArray::fromHex("c30411768506ebe1c2871b1ee2e87d38df342317300a9b97a95ec6a8")
            << QByteArray::fromHex("048330e7c7c8b4a41ab713b3a6f958d77b8cf3ee969930f1584dd550");
    QTest::newRow("keccak_256_abc_abc")
            << int(QCryptographicHash::Keccak_256)
            << QByteArray("abc") << QByteArray("abc")
            << QByteArray::fromHex("4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45")
            << QByteArray::fromHex("9f0adad0a59b05d2e04a1373342b10b9eb16c57c164c8a3bfcbf46dccee39a21");
    QTest::newRow("keccak_384_abc_abc")
            << int(QCryptographicHash::Keccak_384)
            << QByteArray("abc") << QByteArray("abc")
            << QByteArray::fromHex("f7df1165f033337be098e7d288ad6a2f74409d7a60b49c36642218de161b1f99f8c681e4afaf31a34db29fb763e3c28e")
            << QByteArray::fromHex("d733b87d392d270889d3da23ae113f349e25574b445f319cde4cd3f877c753e9e3c65980421339b3a131457ff393939f");
    QTest::newRow("keccak_512_abc_abc")
            << int(QCryptographicHash::Keccak_512)
            << QByteArray("abc") << QByteArray("abc")
            << QByteArray::fromHex("18587dc2ea106b9a1563e32b3312421ca164c7f1f07bc922a9c83d77cea3a1e5d0c69910739025372dc14ac9642629379540c17e2a65b19d77aa511a9d00bb96")
            << QByteArray::fromHex("a7c392d2a42155761ca76bddde1c47d55486b007edf465397bfb9dfa74d11c8f0d7c86cd29415283f1b5e7f655cec25b869c9e9c33a8986f0b38542fb12bfb93");
}

void tst_QCryptographicHash::intermediary_result()
{
    QFETCH(int, algo);
    QCryptographicHash::Algorithm _algo = QCryptographicHash::Algorithm(algo);
    QCryptographicHash hash(_algo);

    QFETCH(QByteArray, first);
    hash.addData(first);

    QFETCH(QByteArray, hash_first);
    QCOMPARE(hash.resultView(), hash_first);

    // don't reset
    QFETCH(QByteArray, second);
    QFETCH(QByteArray, hash_firstsecond);
    hash.addData(second);

    QCOMPARE(hash.resultView(), hash_firstsecond);

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
    QCOMPARE(QCryptographicHash::hash(QByteArray(1'000'000, 'a'), QCryptographicHash::Sha1).toHex().toUpper(),
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

void tst_QCryptographicHash::keccak_data()
{
    QTest::addColumn<QCryptographicHash::Algorithm>("algorithm");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QByteArray>("expectedResult");

#define ROW(Tag, Algorithm, Input, Result) \
    QTest::newRow(Tag) << Algorithm << QByteArrayLiteral(Input) << QByteArray::fromHex(Result)

    ROW("keccak_224_pangram",
        QCryptographicHash::Keccak_224,
        "The quick brown fox jumps over the lazy dog",
        "310aee6b30c47350576ac2873fa89fd190cdc488442f3ef654cf23fe");

    ROW("keccak_224_pangram_dot",
        QCryptographicHash::Keccak_224,
        "The quick brown fox jumps over the lazy dog.",
        "c59d4eaeac728671c635ff645014e2afa935bebffdb5fbd207ffdeab");

    ROW("keccak_256_pangram",
        QCryptographicHash::Keccak_256,
        "The quick brown fox jumps over the lazy dog",
        "4d741b6f1eb29cb2a9b9911c82f56fa8d73b04959d3d9d222895df6c0b28aa15");

    ROW("keccak_256_pangram_dot",
        QCryptographicHash::Keccak_256,
        "The quick brown fox jumps over the lazy dog.",
        "578951e24efd62a3d63a86f7cd19aaa53c898fe287d2552133220370240b572d");

    ROW("keccak_384_pangram",
        QCryptographicHash::Keccak_384,
        "The quick brown fox jumps over the lazy dog",
        "283990fa9d5fb731d786c5bbee94ea4db4910f18c62c03d173fc0a5e494422e8a0b3da7574dae7fa0baf005e504063b3");

    ROW("keccak_384_pangram_dot",
        QCryptographicHash::Keccak_384,
        "The quick brown fox jumps over the lazy dog.",
        "9ad8e17325408eddb6edee6147f13856ad819bb7532668b605a24a2d958f88bd5c169e56dc4b2f89ffd325f6006d820b");

    ROW("skeccak_512_pangram",
        QCryptographicHash::Keccak_512,
        "The quick brown fox jumps over the lazy dog",
        "d135bb84d0439dbac432247ee573a23ea7d3c9deb2a968eb31d47c4fb45f1ef4422d6c531b5b9bd6f449ebcc449ea94d0a8f05f62130fda612da53c79659f609");

    ROW("keccak_512_pangram_dot",
        QCryptographicHash::Keccak_512,
        "The quick brown fox jumps over the lazy dog.",
        "ab7192d2b11f51c7dd744e7b3441febf397ca07bf812cceae122ca4ded6387889064f8db9230f173f6d1ab6e24b6e50f065b039f799f5592360a6558eb52d760");

#undef ROW
}

void tst_QCryptographicHash::keccak()
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
        QTest::addRow("%s", metaEnum.key(i)) << algorithm;
    }
}

void tst_QCryptographicHash::hashLength()
{
    QFETCH(const QCryptographicHash::Algorithm, algorithm);

    qsizetype expectedSize;
    if (algorithm == QCryptographicHash::NumAlgorithms) {
        // It's UB to call ::hash() with NumAlgorithms, but hashLength() is
        // fine and returns 0 for invalid values:
        expectedSize = 0;
    } else {
        expectedSize = QCryptographicHash::hash("test", algorithm).size();
    }
    QCOMPARE(QCryptographicHash::hashLength(algorithm), expectedSize);
}

void tst_QCryptographicHash::addDataAcceptsNullByteArrayView()
{
    QFETCH(const QCryptographicHash::Algorithm, algorithm);

    if (!QCryptographicHash::supportsAlgorithm(algorithm))
        QSKIP("QCryptographicHash doesn't support this algorithm");

    QCryptographicHash hash1(algorithm);
    hash1.addData("meep");
    hash1.addData(QByteArrayView{}); // after other data

    QCryptographicHash hash2(algorithm);
    hash2.addData(QByteArrayView{}); // before any other data
    hash2.addData("meep");

    const auto expected = QCryptographicHash::hash("meep", algorithm);

    QCOMPARE(hash1.resultView(), expected);
    QCOMPARE(hash2.resultView(), expected);
}

void tst_QCryptographicHash::move()
{
    QCryptographicHash hash1(QCryptographicHash::Sha1);
    hash1.addData("a");

    // move constructor
    auto hash2(std::move(hash1));
    hash2.addData("b");

    // move assign operator
    QCryptographicHash hash3(QCryptographicHash::Sha256);
    hash3.addData("no effect on the end result");
    hash3 = std::move(hash2);
    hash3.addData("c");

    QCOMPARE(hash3.resultView(), QByteArray::fromHex("A9993E364706816ABA3E25717850C26C9CD0D89D"));
}

void tst_QCryptographicHash::swap()
{
    QCryptographicHash hash1(QCryptographicHash::Sha1);
    QCryptographicHash hash2(QCryptographicHash::Sha256);

    hash1.addData("da");
    hash2.addData("te");

    hash1.swap(hash2);

    hash2.addData("ta");
    hash1.addData("st");

    QCOMPARE(hash2.result(), QCryptographicHash::hash("data", QCryptographicHash::Sha1));
    QCOMPARE(hash1.result(), QCryptographicHash::hash("test", QCryptographicHash::Sha256));
}

void tst_QCryptographicHash::ensureLargeData()
{
#if QT_POINTER_SIZE > 4
    QElapsedTimer timer;
    timer.start();
    const size_t GiB = 1024 * 1024 * 1024;
    if (large.size() == 4 * GiB + 1)
        return;
    try {
        large.resize(4 * GiB + 1, '\0');
    } catch (const std::bad_alloc &) {
        QSKIP("Could not allocate 4GiB plus one byte of RAM.");
    }
    QCOMPARE(large.size(), 4 * GiB + 1);
    large.back() = '\1';
    qDebug("created dataset in %lld ms", timer.elapsed());
#endif
}

void tst_QCryptographicHash::moreThan4GiBOfData_data()
{
#if QT_POINTER_SIZE > 4
    if (ensureLargeData(); large.empty())
        return;
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

    const auto view = QByteArrayView{large};
    const auto first = view.first(view.size() / 2);
    const auto last = view.sliced(view.size() / 2);

    QByteArray single;
    QByteArray chunked;

    auto t = MaybeThread{[&] {
        QCryptographicHash h(algorithm);
        h.addData(view);
        single = h.result();
    }};
    {
        QCryptographicHash h(algorithm);
        h.addData(first);
        h.addData(last);
        chunked = h.result();
    }
    t.join();

    QCOMPARE(single, chunked);
}

void tst_QCryptographicHash::keccakBufferOverflow()
{
#if QT_POINTER_SIZE == 4
    QSKIP("This is a 64-bit-only test");
#else

    if (ensureLargeData(); large.empty())
        return;

    QElapsedTimer timer;
    timer.start();
    const auto sg = qScopeGuard([&] {
        qDebug() << "test finished in" << timer.restart() << "ms";
    });

    constexpr qsizetype magic = INT_MAX/4;
    QCOMPARE_GE(large.size(), size_t(magic + 1));

    QCryptographicHash hash(QCryptographicHash::Algorithm::Keccak_224);
    const auto first = QByteArrayView{large}.first(1);
    const auto second = QByteArrayView{large}.sliced(1, magic);
    hash.addData(first);
    hash.addData(second);
    (void)hash.resultView();
    QVERIFY(true); // didn't crash
#endif
}

QTEST_MAIN(tst_QCryptographicHash)
#include "tst_qcryptographichash.moc"
