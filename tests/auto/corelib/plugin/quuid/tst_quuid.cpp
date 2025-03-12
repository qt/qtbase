// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
#include <QtTest/private/qcomparisontesthelper_p.h>

#if QT_CONFIG(process)
#include <QProcess>
#endif

#include <qcoreapplication.h>
#include <quuid.h>
#include <QtCore/private/quuid_p.h>

#ifdef Q_OS_ANDROID
#include <QStandardPaths>
#endif

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

class tst_QUuid : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void compareCompiles();
    void fromChar();
    void toString();
    void fromString_data();
    void fromString();
    void toByteArray();
    void fromByteArray();
    void toRfc4122();
    void fromRfc4122();
    void id128();
    void uint128();
    void createUuidV3OrV5();
    void createUuidV7_unique();
    void createUuidV7_data();
    void createUuidV7();
    void check_QDataStream();
    void isNull();
    void equal();
    void notEqual();
    void cpp11();
    void ordering_data();
    void ordering();

    // Only in Qt > 3.2.x
    void generate();
    void less();
    void more();
    void variants_data();
    void variants();
    void versions_data();
    void versions();

    void threadUniqueness();
    void processUniqueness();

    void hash();

    void qvariant();
    void qvariant_conversion();

    void darwinTypes();

public:
    // Variables
    QUuid uuidNS;
    QUuid uuidA;
    QUuid uuidB;
    QUuid uuidC;
    QUuid uuidD;
};

void tst_QUuid::initTestCase()
{
    //It's NameSpace_DNS in RFC4122
    //"{6ba7b810-9dad-11d1-80b4-00c04fd430c8}";
    uuidNS = QUuid(0x6ba7b810, 0x9dad, 0x11d1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8);

    //"{fc69b59e-cc34-4436-a43c-ee95d128b8c5}";
    uuidA = QUuid(0xfc69b59e, 0xcc34, 0x4436, 0xa4, 0x3c, 0xee, 0x95, 0xd1, 0x28, 0xb8, 0xc5);

    //"{1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b}";
    uuidB = QUuid(0x1ab6e93a, 0xb1cb, 0x4a87, 0xba, 0x47, 0xec, 0x7e, 0x99, 0x03, 0x9a, 0x7b);

#if QT_CONFIG(process)
    // chdir to the directory containing our testdata, then refer to it with relative paths
#ifdef Q_OS_ANDROID
    QString testdata_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else // !Q_OS_ANDROID
    QString testdata_dir = QFileInfo(QFINDTESTDATA("testProcessUniqueness")).absolutePath();
#endif
    QVERIFY2(QDir::setCurrent(testdata_dir), qPrintable("Could not chdir to " + testdata_dir));
#endif

    //"{3d813cbb-47fb-32ba-91df-831e1593ac29}"; http://www.rfc-editor.org/errata_search.php?rfc=4122&eid=1352
    uuidC = QUuid(0x3d813cbb, 0x47fb, 0x32ba, 0x91, 0xdf, 0x83, 0x1e, 0x15, 0x93, 0xac, 0x29);

    //"{21f7f8de-8051-5b89-8680-0195ef798b6a}";
    uuidD = QUuid(0x21f7f8de, 0x8051, 0x5b89, 0x86, 0x80, 0x01, 0x95, 0xef, 0x79, 0x8b, 0x6a);
}

void tst_QUuid::compareCompiles()
{
    QTestPrivate::testAllComparisonOperatorsCompile<QUuid>();
#if defined(Q_OS_WIN)
    QTestPrivate::testEqualityOperatorsCompile<QUuid, GUID>();
#endif
}

void tst_QUuid::fromChar()
{
    QT_TEST_EQUALITY_OPS(uuidA, QUuid("{fc69b59e-cc34-4436-a43c-ee95d128b8c5}"), true);
    QT_TEST_EQUALITY_OPS(uuidA, QUuid("fc69b59e-cc34-4436-a43c-ee95d128b8c5}"), true);
    QT_TEST_EQUALITY_OPS(uuidA, QUuid("{fc69b59e-cc34-4436-a43c-ee95d128b8c5"), true);
    QT_TEST_EQUALITY_OPS(uuidA, QUuid("fc69b59e-cc34-4436-a43c-ee95d128b8c5"), true);
    QT_TEST_EQUALITY_OPS(QUuid(), QUuid("{fc69b59e-cc34-4436-a43c-ee95d128b8c"), true);
    QT_TEST_EQUALITY_OPS(QUuid(), QUuid("{fc69b59e-cc34"), true);
    QT_TEST_EQUALITY_OPS(QUuid(), QUuid("fc69b59e-cc34-"), true);
    QT_TEST_EQUALITY_OPS(QUuid(), QUuid("fc69b59e-cc34"), true);
    QT_TEST_EQUALITY_OPS(QUuid(), QUuid("cc34"), true);
    QT_TEST_EQUALITY_OPS(QUuid(), QUuid(nullptr), true);

    QT_TEST_EQUALITY_OPS(uuidB, QUuid(QString("{1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b}")), true);
}

void tst_QUuid::toString()
{
    QCOMPARE(uuidA.toString(), QString("{fc69b59e-cc34-4436-a43c-ee95d128b8c5}"));
    QCOMPARE(uuidA.toString(QUuid::WithoutBraces),
             QString("fc69b59e-cc34-4436-a43c-ee95d128b8c5"));
    QCOMPARE(uuidA.toString(QUuid::Id128),
             QString("fc69b59ecc344436a43cee95d128b8c5"));

    QCOMPARE(uuidB.toString(), QString("{1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b}"));
    QCOMPARE(uuidB.toString(QUuid::WithoutBraces),
             QString("1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b"));
    QCOMPARE(uuidB.toString(QUuid::Id128),
             QString("1ab6e93ab1cb4a87ba47ec7e99039a7b"));
}

void tst_QUuid::fromString_data()
{
    QTest::addColumn<QUuid>("expected");
    QTest::addColumn<QString>("input");

    QUuid invalid = {};

#define ROW(which, string) \
    QTest::addRow("%-38s -> %s", string, #which) << which << string
    ROW(uuidA,   "{fc69b59e-cc34-4436-a43c-ee95d128b8c5}");
    ROW(uuidA,    "fc69b59e-cc34-4436-a43c-ee95d128b8c5}");
    ROW(uuidA,   "{fc69b59e-cc34-4436-a43c-ee95d128b8c5" );
    ROW(uuidA,    "fc69b59e-cc34-4436-a43c-ee95d128b8c5" );

    ROW(uuidA,   "{fc69b59e-cc34-4436-a43c-ee95d128b8c56"); // too long (not an error!)
    ROW(invalid, "{fc69b59e-cc34-4436-a43c-ee95d128b8c"  ); // premature end (within length limits)
    ROW(invalid, " fc69b59e-cc34-4436-a43c-ee95d128b8c5}"); // leading space
    ROW(uuidB,   "{1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b "); // trailing space (not an error!)
    ROW(invalid, "{gc69b59e-cc34-4436-a43c-ee95d128b8c5}"); // non-hex digit in 1st group
    ROW(invalid, "{fc69b59e-cp34-4436-a43c-ee95d128b8c5}"); // non-hex digit in 2nd group
    ROW(invalid, "{fc69b59e-cc34-44r6-a43c-ee95d128b8c5}"); // non-hex digit in 3rd group
    ROW(invalid, "{fc69b59e-cc34-4436-a4yc-ee95d128b8c5}"); // non-hex digit in 4th group
    ROW(invalid, "{fc69b59e-cc34-4436-a43c-ee95d128j8c5}"); // non-hex digit in last group
    ROW(invalid, "(fc69b59e-cc34-4436-a43c-ee95d128b8c5}"); // wrong initial character
    ROW(invalid, "{fc69b59e+cc34-4436-a43c-ee95d128b8c5}"); // wrong 1st separator
    ROW(invalid, "{fc69b59e-cc34*4436-a43c-ee95d128b8c5}"); // wrong 2nd separator
    ROW(invalid, "{fc69b59e-cc34-44366a43c-ee95d128b8c5}"); // wrong 3rd separator
    ROW(invalid, "{fc69b59e-cc34-4436-a43c\303\244ee95d128b8c5}"); // wrong 4th separator (&auml;)
    ROW(uuidA,   "{fc69b59e-cc34-4436-a43c-ee95d128b8c5)"); // wrong final character (not an error!)

    ROW(uuidB,   "{1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b}");
#undef ROW
}

void tst_QUuid::fromString()
{
    QFETCH(const QUuid, expected);
    QFETCH(const QString, input);

    const auto inputL1 = input.toLatin1();
    const auto inputU8 = input.toUtf8();

    QT_TEST_EQUALITY_OPS(expected, QUuid(input), true);
    QT_TEST_EQUALITY_OPS(expected, QUuid(inputU8), true);
    QT_TEST_EQUALITY_OPS(expected, QUuid(inputL1), true);

    QT_TEST_EQUALITY_OPS(expected, QUuid::fromString(input), true);

    // for QLatin1String, construct one whose data() is not NUL-terminated:
    const auto longerInputL1 = inputL1 + '5'; // the '5' makes the premature end check incorrectly succeed
    const auto inputL1S = QLatin1String(longerInputL1.data(), inputL1.size());
    QT_TEST_EQUALITY_OPS(expected, QUuid::fromString(inputL1S), true);

    // for QUtf8StringView, too:
    const auto longerInputU8 = inputU8 + '5'; // the '5' makes the premature end check incorrectly succeed
    const auto inputU8S = QUtf8StringView(longerInputU8.data(), inputU8.size());
    QT_TEST_EQUALITY_OPS(expected, QUuid::fromString(inputU8S), true);
}

void tst_QUuid::toByteArray()
{
    QCOMPARE(uuidA.toByteArray(), QByteArray("{fc69b59e-cc34-4436-a43c-ee95d128b8c5}"));
    QCOMPARE(uuidA.toByteArray(QUuid::WithoutBraces),
             QByteArray("fc69b59e-cc34-4436-a43c-ee95d128b8c5"));
    QCOMPARE(uuidA.toByteArray(QUuid::Id128),
             QByteArray("fc69b59ecc344436a43cee95d128b8c5"));

    QCOMPARE(uuidB.toByteArray(), QByteArray("{1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b}"));
    QCOMPARE(uuidB.toByteArray(QUuid::WithoutBraces),
             QByteArray("1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b"));
    QCOMPARE(uuidB.toByteArray(QUuid::Id128),
             QByteArray("1ab6e93ab1cb4a87ba47ec7e99039a7b"));
}

void tst_QUuid::fromByteArray()
{
    QT_TEST_EQUALITY_OPS(uuidA, QUuid(QByteArray("{fc69b59e-cc34-4436-a43c-ee95d128b8c5}")), true);
    QT_TEST_EQUALITY_OPS(uuidA, QUuid(QByteArray("fc69b59e-cc34-4436-a43c-ee95d128b8c5}")), true);
    QT_TEST_EQUALITY_OPS(uuidA, QUuid(QByteArray("{fc69b59e-cc34-4436-a43c-ee95d128b8c5")), true);
    QT_TEST_EQUALITY_OPS(uuidA, QUuid(QByteArray("fc69b59e-cc34-4436-a43c-ee95d128b8c5")), true);
    QT_TEST_EQUALITY_OPS(QUuid(), QUuid(QByteArray("{fc69b59e-cc34-4436-a43c-ee95d128b8c")), true);

    QT_TEST_EQUALITY_OPS(uuidB, QUuid(QByteArray("{1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b}")), true);
}

void tst_QUuid::toRfc4122()
{
    QCOMPARE(uuidA.toRfc4122(), QByteArray::fromHex("fc69b59ecc344436a43cee95d128b8c5"));
    QCOMPARE(uuidB.toRfc4122(), QByteArray::fromHex("1ab6e93ab1cb4a87ba47ec7e99039a7b"));
}

void tst_QUuid::fromRfc4122()
{
    QT_TEST_EQUALITY_OPS(
            uuidA,
            QUuid::fromRfc4122(QByteArray::fromHex("fc69b59ecc344436a43cee95d128b8c5")), true);

    QT_TEST_EQUALITY_OPS(
            uuidB, QUuid::fromRfc4122(QByteArray::fromHex("1ab6e93ab1cb4a87ba47ec7e99039a7b")),
            true);
}

void tst_QUuid::id128()
{
    constexpr QUuid::Id128Bytes bytesA = { {
        0xfc, 0x69, 0xb5, 0x9e,
        0xcc, 0x34,
        0x44, 0x36,
        0xa4, 0x3c, 0xee, 0x95, 0xd1, 0x28, 0xb8, 0xc5,
    } };
    constexpr QUuid::Id128Bytes bytesB = { {
        0x1a, 0xb6, 0xe9, 0x3a,
        0xb1, 0xcb,
        0x4a, 0x87,
        0xba, 0x47, 0xec, 0x7e, 0x99, 0x03, 0x9a, 0x7b,
    } };

    QT_TEST_EQUALITY_OPS(QUuid(bytesA), uuidA, true);
    QT_TEST_EQUALITY_OPS(QUuid(bytesB), uuidB, true);
    QVERIFY(memcmp(uuidA.toBytes().data, bytesA.data, sizeof(QUuid::Id128Bytes)) == 0);
    QVERIFY(memcmp(uuidB.toBytes().data, bytesB.data, sizeof(QUuid::Id128Bytes)) == 0);

    QUuid::Id128Bytes leBytesA = {};
    for (int i = 0; i < 16; i++)
        leBytesA.data[15 - i] = bytesA.data[i];
    QT_TEST_EQUALITY_OPS(QUuid(leBytesA, QSysInfo::LittleEndian), uuidA, true);
    QVERIFY(memcmp(uuidA.toBytes(QSysInfo::LittleEndian).data, leBytesA.data, sizeof(leBytesA)) == 0);

    // check the new q{To,From}{Big,Little}Endian() overloads
    QUuid::Id128Bytes roundtrip = qFromLittleEndian(qToLittleEndian(bytesA));
    QVERIFY(memcmp(roundtrip.data, bytesA.data, sizeof(bytesA)) == 0);
    roundtrip = qFromBigEndian(qToBigEndian(bytesA));
    QVERIFY(memcmp(roundtrip.data, bytesA.data, sizeof(bytesA)) == 0);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    const QUuid::Id128Bytes beBytesA = qToBigEndian(leBytesA);
    QVERIFY(memcmp(beBytesA.data, bytesA.data, sizeof(beBytesA)) == 0);
    const QUuid::Id128Bytes otherLeBytesA = qFromBigEndian(bytesA);
    QVERIFY(memcmp(otherLeBytesA.data, leBytesA.data, sizeof(leBytesA)) == 0);
#else // Q_BIG_ENDIAN
    const QUuid::Id128Bytes otherLeBytesA = qToLittleEndian(bytesA);
    QVERIFY(memcmp(otherLeBytesA.data, leBytesA.data, sizeof(leBytesA)) == 0);
    const QUuid::Id128Bytes beBytesA = qFromLittleEndian(leBytesA);
    QVERIFY(memcmp(beBytesA.data, bytesA.data, sizeof(beBytesA)) == 0);
#endif // Q_BYTE_ORDER == Q_LITTLE_ENDIAN
}

void tst_QUuid::uint128()
{
#ifdef QT_SUPPORTS_INT128
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
        QSKIP("Not implemented for big endian. Feel free to submit fixes.");

    // {fc69b59e-cc34-4436-a43c-ee95d128b8c5}
    constexpr quint128 u = Q_UINT128_C(0xfc69b59e'cc344436'a43cee95'd128b8c5); // This is LE
    constexpr quint128 be = qToBigEndian(u);
    constexpr QUuid uuid = QUuid::fromUInt128(be);
    static_assert(uuid.toUInt128() == be, "Round-trip through QUuid failed");

    QT_TEST_EQUALITY_OPS(uuid, uuidA, true);
    QCOMPARE(uuid.toUInt128(), be);

    quint128 le = qFromBigEndian(be);
    QCOMPARE(uuid.toUInt128(QSysInfo::LittleEndian), le);
    QT_TEST_EQUALITY_OPS(QUuid::fromUInt128(le, QSysInfo::LittleEndian), uuidA, true);

    QUuid::Id128Bytes bytes = { .data128 = { qToBigEndian(u) } };
    QUuid uuid2(bytes);
    QT_TEST_EQUALITY_OPS(uuid2, uuid, true);

    // verify that toBytes() and toUInt128() provide bytewise similar result
    constexpr quint128 val = uuid.toUInt128();
    bytes = uuid.toBytes();
    QVERIFY(memcmp(&val, bytes.data, sizeof(val)) == 0);
#else
    QSKIP("This platform has no support for 128-bit integer");
#endif
}

void tst_QUuid::createUuidV3OrV5()
{
    //"www.widgets.com" is also from RFC4122
    QT_TEST_EQUALITY_OPS(uuidC, QUuid::createUuidV3(uuidNS, QByteArray("www.widgets.com")), true);
    QT_TEST_EQUALITY_OPS(uuidC, QUuid::createUuidV3(uuidNS, QString("www.widgets.com")), true);

    QT_TEST_EQUALITY_OPS(uuidD, QUuid::createUuidV5(uuidNS, QByteArray("www.widgets.com")), true);
    QT_TEST_EQUALITY_OPS(uuidD, QUuid::createUuidV5(uuidNS, QString("www.widgets.com")), true);
}

void tst_QUuid::createUuidV7_unique()
{
    const int count = 1000;
    std::vector<QUuid> vec;
    vec.reserve(count);
    for (int i = 0; i < count; ++i) {
        auto id = QUuid::createUuidV7();
        QCOMPARE(id.version(), QUuid::UnixEpoch);
        QCOMPARE(id.variant(), QUuid::DCE);
        vec.push_back(id);
    }

    QVERIFY(std::unique(vec.begin(), vec.end()) == vec.end());
}

void tst_QUuid::createUuidV7_data()
{
    QTest::addColumn<QDateTime>("dt");
    QTest::addColumn<QUuid>("expected");

    // February 22, 2022 2:22:22.00 PM GMT-05:00, example from:
    // https://datatracker.ietf.org/doc/html/rfc9562#name-example-of-a-uuidv7-value
    QTest::newRow("feb2022")
        << QDateTime::fromString("2022-02-22T14:22:22.00-05:00"_L1, Qt::ISODateWithMs)
        << QUuid::fromString("017F22E2-79B0-7CC3-98C4-DC0C0C07398F"_L1);

    QTest::newRow("jan2000")
        << QDateTime::fromString("2000-01-02T14:22:22.00-05:00"_L1, Qt::ISODateWithMs)
        << QUuid("00dc741e-35b0-7643-947d-0380e108ce80"_L1);
}

void tst_QUuid::createUuidV7()
{
    QFETCH(QDateTime, dt);
    QFETCH(QUuid, expected);

    QVERIFY(dt.isValid());

    using namespace std::chrono;
    auto extractTimestamp = [](const QUuid &id) { return (quint64(id.data1) << 16) | id.data2; };
    const auto result =
        createUuidV7_internal(time_point<system_clock, milliseconds>(dt.toMSecsSinceEpoch() * 1ms));
    QCOMPARE_EQ(extractTimestamp(result), extractTimestamp(expected));
}

void tst_QUuid::check_QDataStream()
{
    QUuid tmp;
    QByteArray ar;
    {
        QDataStream out(&ar,QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::BigEndian);
        out << uuidA;
    }
    {
        QDataStream in(&ar,QIODevice::ReadOnly);
        in.setByteOrder(QDataStream::BigEndian);
        in >> tmp;
        QT_TEST_EQUALITY_OPS(uuidA, tmp, true);
    }
    {
        QDataStream out(&ar,QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::LittleEndian);
        out << uuidA;
    }
    {
        QDataStream in(&ar,QIODevice::ReadOnly);
        in.setByteOrder(QDataStream::LittleEndian);
        in >> tmp;
        QT_TEST_EQUALITY_OPS(uuidA, tmp, true);
    }
}

void tst_QUuid::isNull()
{
    constexpr QUuid null;
    static_assert(null.isNull());

    constexpr QUuid nonNull{1, 2, 3, 4, 0, 1, 2, 3, 4, 5, 6};
    static_assert(!nonNull.isNull());

    QVERIFY( !uuidA.isNull() );

    QUuid should_be_null_uuid;
    QVERIFY( should_be_null_uuid.isNull() );
}


void tst_QUuid::equal()
{
    QT_TEST_EQUALITY_OPS(uuidA, uuidB, false);

    QUuid copy(uuidA);
    QT_TEST_EQUALITY_OPS(uuidA, copy, true);

    QUuid assigned;
    assigned = uuidA;
    QT_TEST_EQUALITY_OPS(uuidA, assigned, true);
}


void tst_QUuid::notEqual()
{
    QVERIFY( uuidA != uuidB );
}

void tst_QUuid::cpp11() {
#ifdef Q_COMPILER_UNIFORM_INIT
    // "{fc69b59e-cc34-4436-a43c-ee95d128b8c5}" cf, initTestCase
    constexpr QUuid u1{0xfc69b59e, 0xcc34, 0x4436, 0xa4, 0x3c, 0xee, 0x95, 0xd1, 0x28, 0xb8, 0xc5};
    constexpr QUuid u2 = {0xfc69b59e, 0xcc34, 0x4436, 0xa4, 0x3c, 0xee, 0x95, 0xd1, 0x28, 0xb8, 0xc5};
    Q_UNUSED(u1);
    Q_UNUSED(u2);
#else
    QSKIP("This compiler is not in C++11 mode or it doesn't support uniform initialization");
#endif
}

constexpr QUuid make_minimal(QUuid::Variant variant)
{
    using V = QUuid::Variant;
    switch (variant) {
    case V::VarUnknown: // special case
        return {};
    case V::NCS:        // special case: null would be NCS, but is treated as Unknown
        return {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    case V::DCE:        // special case: DCE should be 0b100, but is 0b10
        return {0, 0, 0, 0b1000'0000, 0, 0, 0, 0, 0, 0, 0};
    case V::Microsoft:
    case V::Reserved:
        return {0, 0, 0, uchar(variant << 5), 0, 0, 0, 0, 0, 0, 0};
    }
    // GCC 8.x does not treat __builtin_unreachable() as constexpr
#if !defined(Q_CC_GNU_ONLY) || (Q_CC_GNU >= 900)
    // NOLINTNEXTLINE(qt-use-unreachable-return): Triggers on Clang, breaking GCC 8
    Q_UNREACHABLE();
#endif
    return {};
}

void tst_QUuid::ordering_data()
{
    QTest::addColumn<QUuid>("lhs");
    QTest::addColumn<QUuid>("rhs");
    QTest::addColumn<Qt::strong_ordering>("expected");

    // QUuid is sorted by variant() first, then the dataN fields, in order
    // Exhaustive testing is pointless, so pick some strategic values

    constexpr QUuid null = make_minimal(QUuid::Variant::VarUnknown);
    QCOMPARE(null.variant(), QUuid::Variant::VarUnknown);

    constexpr QUuid minNCS = make_minimal(QUuid::Variant::NCS);
    QCOMPARE(minNCS.variant(), QUuid::Variant::NCS);

    constexpr QUuid ncs000_0000_0001 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    QCOMPARE(ncs000_0000_0001, minNCS);
    constexpr QUuid ncs000_0000_0010 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0};
    constexpr QUuid ncs000_0000_0100 = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0};
    constexpr QUuid ncs000_0000_1000 = {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0};

    constexpr QUuid ncs000_0001_0000 = {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};
    constexpr QUuid ncs000_0010_0000 = {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0};
    constexpr QUuid ncs000_0100_0000 = {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
    constexpr QUuid ncs000_1000_0000 = {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0};

    constexpr QUuid ncs001_0000_0000 = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
    constexpr QUuid ncs010_0000_0000 = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    constexpr QUuid ncs100_0000_0000 = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    constexpr QUuid minDCE = make_minimal(QUuid::Variant::DCE);
    QCOMPARE(minDCE.variant(), QUuid::Variant::DCE);

    constexpr QUuid minMS = make_minimal(QUuid::Variant::Microsoft);
    QCOMPARE(minMS.variant(), QUuid::Variant::Microsoft);

    constexpr QUuid minR = make_minimal(QUuid::Variant::Reserved);
    QCOMPARE(minR.variant(), QUuid::Variant::Reserved);

    constexpr QUuid ones = {0xFFFF'FFFFU, 0xFFFFu, 0xFFFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu};
    QCOMPARE(ones.variant(), QUuid::Variant::Reserved);

#define ROW(l, r, c) \
    QTest::addRow("%s<>%s", #l, #r) << l << r << Qt::strong_ordering:: c \
    /* end */
#define EQUAL(x) ROW(x, x, equal)
    EQUAL(null);
    EQUAL(minNCS);
    EQUAL(minDCE);
    EQUAL(minMS);
    EQUAL(minR);
    EQUAL(ones);
#undef EQUAL
#define AFTER_NULL(x) ROW(null, x, less)
    AFTER_NULL(minNCS);
    AFTER_NULL(minDCE);
    AFTER_NULL(minMS);
    AFTER_NULL(minR);
    AFTER_NULL(ones);
#undef AFTER_NULL
#define AFTER_NCS(x) ROW(minNCS, x, less)
    AFTER_NCS(ncs000_0000_0010);
    AFTER_NCS(ncs000_0000_0100);
    ROW(ncs000_0000_0010, ncs000_0000_0100, less);
    AFTER_NCS(ncs000_0000_1000);
    AFTER_NCS(ncs000_0001_0000);
    AFTER_NCS(ncs000_0010_0000);
    AFTER_NCS(ncs000_0100_0000);
    AFTER_NCS(ncs000_1000_0000);
    AFTER_NCS(ncs001_0000_0000);
    AFTER_NCS(ncs010_0000_0000);
    AFTER_NCS(ncs100_0000_0000);
    ROW(ncs100_0000_0000, minDCE, less);
    AFTER_NCS(minDCE);
    AFTER_NCS(minMS);
    AFTER_NCS(minR);
    AFTER_NCS(ones);
#undef AFTER_NCS
#define AFTER_DCE(x) ROW(minDCE, x, less)
    AFTER_DCE(minMS);
    AFTER_DCE(minR);
    AFTER_DCE(ones);
#undef AFTER_DCE
#define AFTER_MS(x) ROW(minMS, x, less)
    AFTER_MS(minR);
    AFTER_MS(ones);
#undef AFTER_MS
#define AFTER_R(x) ROW(minR, x, less)
    AFTER_R(ones);
#undef AFTER_R
#undef ROW

    // due to the way we store data1,2,3 in memory, the ordering will flip
    QTest::newRow("qt7-integer-portions")
            << QUuid{0x01000002, 0x0000, 0x0000, 0, 0, 0, 0, 0, 0, 0, 0}
            << QUuid{0x02000001, 0x0000, 0x0000, 0, 0, 0, 0, 0, 0, 0, 0}
            << (QSysInfo::ByteOrder == QSysInfo::BigEndian || QT_VERSION_MAJOR < 7 ?
                    Qt::strong_ordering::less : Qt::strong_ordering::greater);
}

void tst_QUuid::ordering()
{
    QFETCH(const QUuid, lhs);
    QFETCH(const QUuid, rhs);
    QFETCH(const Qt::strong_ordering, expected);

    QCOMPARE(qCompareThreeWay(lhs, rhs), expected);
    QT_TEST_ALL_COMPARISON_OPS(lhs, rhs, expected);
}

void tst_QUuid::generate()
{
    QUuid shouldnt_be_null_uuidA = QUuid::createUuid();
    QUuid shouldnt_be_null_uuidB = QUuid::createUuid();
    QVERIFY( !shouldnt_be_null_uuidA.isNull() );
    QVERIFY( !shouldnt_be_null_uuidB.isNull() );
    QVERIFY( shouldnt_be_null_uuidA != shouldnt_be_null_uuidB );
}


void tst_QUuid::less()
{
    QVERIFY(  uuidB <  uuidA);
    QVERIFY(  uuidB <= uuidA);
    QVERIFY(!(uuidA <  uuidB) );
    QVERIFY(!(uuidA <= uuidB));
    QT_TEST_ALL_COMPARISON_OPS(uuidB, uuidA, Qt::strong_ordering::less);

    QUuid null_uuid;
    QVERIFY(null_uuid < uuidA); // Null uuid is always less than a valid one
    QVERIFY(null_uuid <= uuidA);
    QT_TEST_ALL_COMPARISON_OPS(null_uuid, uuidA, Qt::strong_ordering::less);

    QVERIFY(null_uuid <= null_uuid);
    QVERIFY(uuidA <= uuidA);
}


void tst_QUuid::more()
{
    QVERIFY(  uuidA >  uuidB);
    QVERIFY(  uuidA >= uuidB);
    QVERIFY(!(uuidB >  uuidA));
    QVERIFY(!(uuidB >= uuidA));
    QT_TEST_ALL_COMPARISON_OPS(uuidA, uuidB, Qt::strong_ordering::greater);

    QUuid null_uuid;
    QVERIFY(!(null_uuid >  uuidA)); // Null uuid is always less than a valid one
    QVERIFY(!(null_uuid >= uuidA));

    QVERIFY(null_uuid >= null_uuid);
    QVERIFY(uuidA >= uuidA);
    QT_TEST_ALL_COMPARISON_OPS(uuidA, uuidA, Qt::strong_ordering::equal);
}

void tst_QUuid::variants_data()
{
    QTest::addColumn<QUuid>("uuid");
    QTest::addColumn<QUuid::Variant>("variant");

    QTest::newRow("default-constructed") << QUuid() << QUuid::VarUnknown;
    QTest::newRow("minimal-NCS") << make_minimal(QUuid::NCS) << QUuid::NCS;
    QTest::newRow("minimal-DCE") << make_minimal(QUuid::DCE) << QUuid::DCE;
    QTest::newRow("minimal-Microsoft") << make_minimal(QUuid::Microsoft) << QUuid::Microsoft;
    QTest::newRow("minimal-Reserved") << make_minimal(QUuid::Reserved) << QUuid::Reserved;
    QTest::newRow("uuidA") << uuidA << QUuid::DCE;
    QTest::newRow("uuidB") << uuidB << QUuid::DCE;
    QTest::newRow("NCS") << QUuid("{3a2f883c-4000-000d-0000-00fb40000000}") << QUuid::NCS;

    // compile-time checks
    constexpr QUuid defaultConstructed;
    static_assert(defaultConstructed.variant() == QUuid::Variant::VarUnknown);
    constexpr QUuid minDCE = make_minimal(QUuid::Variant::DCE);
    static_assert(minDCE.variant() == QUuid::Variant::DCE);
}

void tst_QUuid::variants()
{
    QFETCH(const QUuid, uuid);
    QFETCH(const QUuid::Variant, variant);

    QCOMPARE_EQ(uuid.variant(), variant);
}

void tst_QUuid::versions_data()
{
    QTest::addColumn<QUuid>("uuid");
    QTest::addColumn<QUuid::Version>("version");

    QTest::newRow("default-constructed") << QUuid() << QUuid::VerUnknown;
    QTest::newRow("DCE-time") << QUuid("{406c45a0-3b7e-11d0-80a3-0000c08810a7}") << QUuid::Time;
    QTest::newRow("DCE-EmbPosix")
            << QUuid(0, 0, 0b0010'0010'1000'0010, 0b1010'0000, 0, 0, 0, 0, 0, 0, 0)
            << QUuid::EmbeddedPOSIX;
    QTest::newRow("DCE-Md5")
            << QUuid(0, 0, 0b0011'0001'0100'1001, 0b1011'0000, 0, 0, 0, 0, 0, 0, 0)
            << QUuid::Md5;
    QTest::newRow("DCE-Random")
            << QUuid(0, 0, 0b0100'0101'0001'1101, 0b1000'0000, 0, 0, 0, 0, 0, 0, 0)
            << QUuid::Random;
    QTest::newRow("DCE-Sha1")
            << QUuid(0, 0, 0b0101'1101'0101'1011, 0b1001'0000, 0, 0, 0, 0, 0, 0, 0)
            << QUuid::Sha1;
    QTest::newRow("DCE-inv-less-than-Time->unknown")
            << QUuid(0, 0, 0b0000'1101'0101'1011, 0b1000'0000, 0, 0, 0, 0, 0, 0, 0)
            << QUuid::VerUnknown;
    QTest::newRow("DCE-inv-greater-than-UnixEpoch->unknown")
            << QUuid(0, 0, 0b1000'1101'0101'1011, 0b1000'0000, 0, 0, 0, 0, 0, 0, 0)
            << QUuid::VerUnknown;
    QTest::newRow("NCS-Time->unknown")
            << QUuid(0, 0, 0b0001'0000'0000'0000, 0b0100'0000, 0, 0, 0, 0, 0, 0, 0)
            << QUuid::VerUnknown;
    QTest::newRow("MS-Sha1->unknown")
            << QUuid(0, 0, 0b0101'0000'0000'0000, 0b1100'0000, 0, 0, 0, 0, 0, 0, 0)
            << QUuid::VerUnknown;
    QTest::newRow("Reserved-Random->unknown")
            << QUuid(0, 0, 0b0100'0000'0000'0000, 0b1110'0000, 0, 0, 0, 0, 0, 0, 0)
            << QUuid::VerUnknown;
    QTest::newRow("uuidA") << uuidA << QUuid::Random;
    QTest::newRow("uuidB") << uuidB << QUuid::Random;

    // compile-time checks
    constexpr QUuid defaultConstructed;
    static_assert(defaultConstructed.version() == QUuid::Version::VerUnknown);
    constexpr QUuid timeVer = {0, 0, 0x1000, 0b1000'0000, 0, 0, 0, 0, 0, 0, 0};
    static_assert(timeVer.version() == QUuid::Version::Time);
}

void tst_QUuid::versions()
{
    QFETCH(const QUuid, uuid);
    QFETCH(const QUuid::Version, version);

    QCOMPARE_EQ(uuid.version(), version);
}

class UuidThread : public QThread
{
public:
    QUuid uuid;

    void run() override
    {
        uuid = QUuid::createUuid();
    }
};

void tst_QUuid::threadUniqueness()
{
    QList<UuidThread *> threads(qMax(2, QThread::idealThreadCount()));
    for (int i = 0; i < threads.size(); ++i)
        threads[i] = new UuidThread;
    for (int i = 0; i < threads.size(); ++i)
        threads[i]->start();
    for (int i = 0; i < threads.size(); ++i)
        QVERIFY(threads[i]->wait(1000));
    for (int i = 1; i < threads.size(); ++i)
        QVERIFY(threads[0]->uuid != threads[i]->uuid);
    qDeleteAll(threads);
}

void tst_QUuid::processUniqueness()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support");
#else
#ifdef Q_OS_ANDROID
    QSKIP("This test crashes on Android");
#endif
    QProcess process;
    QString processOneOutput;
    QString processTwoOutput;

    // Start it once
#ifdef Q_OS_DARWIN
    process.start("testProcessUniqueness/testProcessUniqueness.app");
#elif defined(Q_OS_ANDROID)
    process.start("libtestProcessUniqueness.so");
#else
    process.start("testProcessUniqueness/testProcessUniqueness");
#endif
    QVERIFY(process.waitForFinished());
    processOneOutput = process.readAllStandardOutput();

    // Start it twice
#ifdef Q_OS_DARWIN
    process.start("testProcessUniqueness/testProcessUniqueness.app");
#elif defined(Q_OS_ANDROID)
    process.start("libtestProcessUniqueness.so");
#else
    process.start("testProcessUniqueness/testProcessUniqueness");
#endif
    QVERIFY(process.waitForFinished());
    processTwoOutput = process.readAllStandardOutput();

    // They should be *different*!
    QVERIFY(processOneOutput != processTwoOutput);
#endif
}

void tst_QUuid::hash()
{
    size_t h = qHash(uuidA);
    QCOMPARE(qHash(uuidA), h);
    QCOMPARE(qHash(QUuid(uuidA.toString())), h);
}

void tst_QUuid::qvariant()
{
    QUuid uuid = QUuid::createUuid();
    QVariant v = QVariant::fromValue(uuid);
    QVERIFY(!v.isNull());
    QCOMPARE(v.metaType(), QMetaType(QMetaType::QUuid));

    QUuid uuid2 = v.value<QUuid>();
    QVERIFY(!uuid2.isNull());
    QT_TEST_EQUALITY_OPS(uuid, uuid2, true);
}

void tst_QUuid::qvariant_conversion()
{
    QUuid uuid = QUuid::createUuid();
    QVariant v = QVariant::fromValue(uuid);

    // QUuid -> QString
    QVERIFY(v.canConvert<QString>());
    QCOMPARE(v.toString(), uuid.toString());
    QCOMPARE(v.value<QString>(), uuid.toString());

    // QUuid -> QByteArray
    QVERIFY(v.canConvert<QByteArray>());
    QCOMPARE(v.toByteArray(), uuid.toByteArray());
    QCOMPARE(v.value<QByteArray>(), uuid.toByteArray());

    QVERIFY(!v.canConvert<int>());
    QVERIFY(!v.canConvert<QStringList>());

    // try reverse conversion QString -> QUuid
    QVariant sv = QVariant::fromValue(uuid.toString());
    QCOMPARE(sv.metaType(), QMetaType(QMetaType::QString));
    QVERIFY(sv.canConvert<QUuid>());
    QCOMPARE(sv.value<QUuid>(), uuid);

    // QString -> QUuid
    {
        QVariant sv = QVariant::fromValue(uuid.toByteArray());
        QCOMPARE(sv.metaType(), QMetaType(QMetaType::QByteArray));
        QVERIFY(sv.canConvert<QUuid>());
        QT_TEST_EQUALITY_OPS(sv.value<QUuid>(), uuid, true);
    }
}

void tst_QUuid::darwinTypes()
{
#ifndef Q_OS_DARWIN
    QSKIP("This is a Darwin-only test");
#else
    extern void tst_QUuid_darwinTypes(); // in tst_quuid_darwin.mm
    tst_QUuid_darwinTypes();
#endif
}

QTEST_MAIN(tst_QUuid)
#include "tst_quuid.moc"
