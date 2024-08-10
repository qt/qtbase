// Copyright (C) 2024 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Testing qtmochelpers.h is probably pointless... if there's a problem with it
// then you most likely can't compile this test in the first place.
#include <QtCore/qtmochelpers.h>
#include "qtmocconstants.h"

#include <QTest>

#include <QtCore/qobject.h>

#include <initializer_list>
#include <q20algorithm.h>

QT_BEGIN_NAMESPACE
namespace QtMocHelpers {
} QT_END_NAMESPACE

class tst_MocHelpers : public QObject
{
    Q_OBJECT
private slots:
    void stringData();

    // parts of the uint array
    void classinfoData();
    void classinfoDataGroup();
    void enumUintData();
    void enumUintGroup();
    void propertyUintData();
    void propertyUintGroup();
    void methodUintData();
    void methodUintGroup();
    void constructorUintData();
    void constructorUintGroup();

    void emptyUintArray();
    void uintArrayNoMethods();
    void uintArray();
};

template <int Count, size_t StringSize>
void verifyStringData(const QtMocHelpers::StringData<Count, StringSize> &data,
                      std::initializer_list<const char *> strings)
{
    QCOMPARE(std::size(strings), size_t(Count) / 2);
    ptrdiff_t i = 0;
    for (const char *str : strings) {
        uint offset = data.offsetsAndSizes[i++] - sizeof(data.offsetsAndSizes);
        uint len = data.offsetsAndSizes[i++];
        QByteArrayView result(data.stringdata0 + offset, len);

        QCOMPARE(len, strlen(str));
        QCOMPARE(result, str);
    }
}

void tst_MocHelpers::stringData()
{
#define CHECK(...)  \
    verifyStringData(QtMocHelpers::stringData(__VA_ARGS__), { __VA_ARGS__ })

    QTest::setThrowOnFail(true);
    CHECK("Hello");
    CHECK("Hello", "World");
    CHECK("Hello", "", "World");
#undef CHECK
}

void tst_MocHelpers::classinfoData()
{
    {
        auto result = QtMocHelpers::ClassInfos({{1, 2}});
        QCOMPARE(result.headerSize(), 2U);
        QCOMPARE(result.header[0], 1U);
        QCOMPARE(result.header[1], 2U);
    }
    {
        auto result = QtMocHelpers::ClassInfos({{1, 2}, {3, 4}});
        QCOMPARE(result.headerSize(), 4U);
        QCOMPARE(result.header[0], 1U);
        QCOMPARE(result.header[1], 2U);
        QCOMPARE(result.header[2], 3U);
        QCOMPARE(result.header[3], 4U);
    }
}

template <size_t N> static void checkClassInfos(const std::array<uint, N> &data)
{
    QCOMPARE(data[2], 2U);
    QCOMPARE_GE(data[3], 14U);

    const uint *classinfos = data.data() + data[3];
    QCOMPARE(classinfos[0], 1U);
    QCOMPARE(classinfos[1], 2U);
    QCOMPARE(classinfos[2], 3U);
    QCOMPARE(classinfos[3], 4U);
}

void tst_MocHelpers::classinfoDataGroup()
{
    constexpr auto data = QtMocHelpers::metaObjectData(0,
            QtMocHelpers::UintData{}, QtMocHelpers::UintData{},
            QtMocHelpers::UintData{}, QtMocHelpers::UintData{},
            QtMocHelpers::ClassInfos({{1, 2}, {3, 4}}));
    checkClassInfos(data.data);
}

template <typename E, int N> void enumUintData_check(const E (&values)[N])
{
    using namespace QtMocHelpers;

    // make an array of dummy offsets
    typename EnumData<E>::EnumEntry namesAndOffsets[N];
    for (int i = 0; i < N; ++i) {
        namesAndOffsets[i].nameIndex = 2 * (i + 7);
        namesAndOffsets[i].value = values[i];
    }

    auto result = EnumData<E>(0, 0, 0).add(namesAndOffsets);
    for (uint i = 0; i < N; ++i) {
        QCOMPARE(result.payload[2 * i + 0], uint(namesAndOffsets[i].nameIndex));
        QCOMPARE(result.payload[2 * i + 1], uint(values[i]));
    }
}

enum E1 { AnEnumValue };
enum class E2 { V0 = INT_MAX, V1 = INT_MIN };
enum class E3 : int { V = 0x1111'2222, V2 = -V };
void tst_MocHelpers::enumUintData()
{
    using namespace QtMocHelpers;
    using namespace QtMocConstants;
    {
        auto result = QtMocHelpers::EnumData<E1>(1, 1, EnumFlags{});
        QCOMPARE(result.header[0], 1U);
        QCOMPARE(result.header[1], 1U);
        QCOMPARE(result.header[2], EnumFlags{});
        QCOMPARE(result.header[3], 0U);
        QCOMPARE(result.payloadSize(), 0U);
    }
    {
        auto result = QtMocHelpers::EnumData<QFlags<E1>>(1, 2, EnumIsFlag);
        QCOMPARE(result.header[0], 1U);
        QCOMPARE(result.header[1], 2U);
        QCOMPARE(result.header[2], EnumIsFlag);
        QCOMPARE(result.header[3], 0U);
        QCOMPARE(result.payloadSize(), 0U);
    }
    {
        auto result = QtMocHelpers::EnumData<E1>(1, 1, EnumFlags{}).add({ { 1, E1::AnEnumValue } });
        QCOMPARE(result.header[0], 1U);
        QCOMPARE(result.header[1], 1U);
        QCOMPARE(result.header[2], EnumFlags{});
        QCOMPARE(result.header[3], 1U);
        QCOMPARE(result.payload[0], 1U);
        QCOMPARE(result.payload[1], uint(E1::AnEnumValue));
    }
    {
        auto result = QtMocHelpers::EnumData<QFlags<E1>>(1, 2, EnumFlags{}).add({ { 1, E1::AnEnumValue } });
        QCOMPARE(result.header[0], 1U);
        QCOMPARE(result.header[1], 2U);
        QCOMPARE(result.header[2], uint(EnumIsFlag));
        QCOMPARE(result.header[3], 1U);
        QCOMPARE(result.payload[0], 1U);
        QCOMPARE(result.payload[1], uint(E1::AnEnumValue));
    }
    {
        constexpr auto result = QtMocHelpers::EnumData<E3>(1, 1, EnumIsScoped)
                .add({ { 2, E3::V }, {3, E3::V2 }, });
        QCOMPARE(result.header[0], 1U);
        QCOMPARE(result.header[1], 1U);
        QCOMPARE(result.header[2], EnumIsScoped);
        QCOMPARE(result.header[3], 2U);
        QCOMPARE(result.payload[0], 2U);
        QCOMPARE(result.payload[1], uint(E3::V));
        QCOMPARE(result.payload[2], 3U);
        QCOMPARE(result.payload[3], uint(E3::V2));
    }

    QTest::setThrowOnFail(true);
    {
        enum E { E0, E1 = -1, E2 = 123, E3 = INT_MIN };
        enumUintData_check({E0, E1, E2, E3});
    }
}

template <typename Data> void testUintData(const Data &data)
{
    uint count = 0;
    size_t headerSize = 0;
    size_t payloadSize = 0;
    data.forEach([&](const auto &block) {
        ++count;
        headerSize += block.headerSize();
        payloadSize += block.payloadSize();
    });

    QCOMPARE(data.count(), count);
    QCOMPARE(data.headerSize(), headerSize);
    QCOMPARE(data.payloadSize(), payloadSize);
}

template <size_t N> static void checkEnums(const std::array<uint, N> &data)
{
    using namespace QtMocConstants;
    QCOMPARE(data[8], 4U);
    QCOMPARE_NE(data[9], 0U);

    const uint *header = data.data() + data[9];

    // E1:
    QCOMPARE(header[0 + 0], 1U);
    QCOMPARE(header[0 + 1], 1U);
    QCOMPARE(header[0 + 2], 0U);
    QCOMPARE(header[0 + 3], 1U);
    QCOMPARE_GE(header[0 + 4], 14U);
    const uint *payload = data.data() + header[0 + 4];
    QCOMPARE(payload[0], 3U);
    QCOMPARE(payload[1], uint(E1::AnEnumValue));

    // E3:
    QCOMPARE(header[5 + 0], 4U);
    QCOMPARE(header[5 + 1], 5U);
    QCOMPARE(header[5 + 2], EnumIsFlag | EnumIsScoped);
    QCOMPARE(header[5 + 3], 2U);
    QCOMPARE_GE(header[5 + 4], 14U);
    payload = data.data() + header[5 + 4];
    QCOMPARE(payload[0], 6U);
    QCOMPARE(payload[1], uint(E3::V));
    QCOMPARE(payload[2], 8U);
    QCOMPARE(payload[3], uint(E3::V2));

    // E2:
    QCOMPARE(header[10 + 0], 7U);
    QCOMPARE(header[10 + 1], 6U);
    QCOMPARE(header[10 + 2], EnumIsFlag | EnumIsScoped);
    QCOMPARE(header[10 + 3], 2U);
    QCOMPARE_GE(header[10 + 4], 14U);
    payload = data.data() + header[10 + 4];
    QCOMPARE(payload[0], 7U);
    QCOMPARE(payload[1], uint(E2::V0));
    QCOMPARE(payload[2], 10U);
    QCOMPARE(payload[3], uint(E2::V1));

    // QFlags<E1>
    QCOMPARE(header[15 + 0], 11U);
    QCOMPARE(header[15 + 1], 1U);
    QCOMPARE(header[15 + 2], EnumIsFlag);
    QCOMPARE(header[15 + 3], 1U);
    QCOMPARE_GE(header[15 + 4], 14U);
    payload = data.data() + header[15 + 4];
    QCOMPARE(payload[0], 3U);
    QCOMPARE(payload[1], uint(E1::AnEnumValue));
}

void tst_MocHelpers::enumUintGroup()
{
    using namespace QtMocConstants;
    QTest::setThrowOnFail(true);
    constexpr QtMocHelpers::UintData enums = {
        QtMocHelpers::EnumData<E1>(1, 1, 0x00).add({ { 3, E1::AnEnumValue } }),
        QtMocHelpers::EnumData<E3>(4, 5, EnumIsFlag | EnumIsScoped)
            .add({ { 6, E3::V }, { 8, E3::V2 }, }),
        QtMocHelpers::EnumData<E2>(7, 6, EnumIsFlag | EnumIsScoped)
            .add({ { 7, E2::V0 }, { 10, E2::V1 }, }),
        QtMocHelpers::EnumData<QFlags<E1>>(11, 1, EnumIsFlag).add({ { 3, E1::AnEnumValue } }),
    };
    testUintData(enums);

    constexpr auto data = QtMocHelpers::metaObjectData(0,
            QtMocHelpers::UintData{}, QtMocHelpers::UintData{}, enums);
    checkEnums(data.data);
}

void tst_MocHelpers::propertyUintData()
{
    using namespace QtMocHelpers;
    {
        auto result = PropertyData(3, QMetaType::Int, 0x3, 13, 0x101);
        QCOMPARE(result.payloadSize(), 0U);
        QCOMPARE(result.header[0], 3U);
        QCOMPARE(result.header[1], uint(QMetaType::Int));
        QCOMPARE(result.header[2], 0x3U);
        QCOMPARE(result.header[3], 13U);
        QCOMPARE(result.header[4], 0x0101U);
    }
    {
        // check that QMetaType doesn't override if it's an alias
        auto result = PropertyData(3, 0x80000000 | 4, 0x03);
        QCOMPARE(result.header[1], 0x80000000U | 4);
    }
    {
        // Or derived from
        struct Dummy : QString {};
        auto result = PropertyData(3, 0x80000000 | 4, 0x03);
        QCOMPARE(result.header[1], 0x80000000U | 4);
    }
}

template <size_t N> static void checkProperties(const std::array<uint, N> &data)
{
    QCOMPARE(data[6], 3U);
    QCOMPARE_NE(data[7], 0U);

    const uint *header = data.data() + data[7];

    QCOMPARE(header[0 + 0], 3U);
    QCOMPARE(header[0 + 1], uint(QMetaType::Int));
    QCOMPARE(header[0 + 2], 0x3U);
    QCOMPARE(header[0 + 3], 13U);
    QCOMPARE(header[0 + 4], 0x0101U);

    QCOMPARE(header[5 + 0], 4U);
    QCOMPARE(header[5 + 1], 0x80000000U | 5);
    QCOMPARE(header[5 + 2], 0x3U);
    QCOMPARE(header[5 + 3], uint(-1));
    QCOMPARE(header[5 + 4], 0U);

    QCOMPARE(header[10 + 0], 6U);
    QCOMPARE(header[10 + 1], 0x80000000U | 7);
    QCOMPARE(header[10 + 2], 0x3U);
    QCOMPARE(header[10 + 3], uint(-1));
    QCOMPARE(header[10 + 4], 0U);
}

void tst_MocHelpers::propertyUintGroup()
{
    QTest::setThrowOnFail(true);
    constexpr QtMocHelpers::UintData properties = {
        QtMocHelpers::PropertyData(3, QMetaType::Int, 0x3, 13, 0x101),
        QtMocHelpers::PropertyData(4, 0x80000000 | 5, 0x03),
        QtMocHelpers::PropertyData(6, 0x80000000 | 7, 0x03)
    };
    testUintData(properties);

    constexpr auto data = QtMocHelpers::metaObjectData(0, QtMocHelpers::UintData{}, properties, QtMocHelpers::UintData{});
    checkProperties(data.data);
}

void tst_MocHelpers::methodUintData()
{
    using namespace QtMocHelpers;
    using namespace QtMocConstants;
    {
        auto result = SignalData<void()>(1, 2, 0, AccessPublic, QMetaType::Void, {});
        QCOMPARE(result.header[0], 1U);
        QCOMPARE(result.header[1], 0U);
        QCOMPARE(result.header[3], 2U);
        QCOMPARE(result.header[4], AccessPublic | MethodSignal);
        QCOMPARE(result.header[5], 0U);
        QCOMPARE(result.payload[0], uint(QMetaType::Void));
    }
    {
        auto result = SlotData<void (const QString &) const>(1, 2, 0, AccessPublic,
                    QMetaType::Void, { { { QMetaType::QString, 1000 } } });
        QCOMPARE(result.header[0], 1U);
        QCOMPARE(result.header[1], 1U);
        QCOMPARE(result.header[3], 2U);
        QCOMPARE(result.header[4], AccessPublic | MethodSlot | MethodIsConst);
        QCOMPARE(result.header[5], 0U);
        QCOMPARE(result.payload[0], uint(QMetaType::Void));
        QCOMPARE(result.payload[1], uint(QMetaType::QString));
        QCOMPARE(result.payload[2], 1000U);
    }
    {
        auto result = RevisionedSlotData<void (const QString &)>(1, 2, 0, AccessPublic, 0xff01,
                    QMetaType::Void, { { { QMetaType::QString, 1000 } } });
        QCOMPARE(result.header[0], 1U);
        QCOMPARE(result.header[1], 1U);
        QCOMPARE(result.header[3], 2U);
        QCOMPARE(result.header[4], AccessPublic | MethodSlot | MethodRevisioned);
        QCOMPARE(result.header[5], 0U);
        QCOMPARE(result.payload[0], 0xff01U);
        QCOMPARE(result.payload[1], uint(QMetaType::Void));
        QCOMPARE(result.payload[2], uint(QMetaType::QString));
        QCOMPARE(result.payload[3], 1000U);
    }
}

template <size_t N> static void checkMethods(const std::array<uint, N> &data)
{
    using namespace QtMocConstants;
    QCOMPARE(data[4], 3U);
    QCOMPARE_NE(data[5], 0U);

    const uint *header = data.data() + data[5];

    // signals: void signal()
    QCOMPARE(header[0], 1U);
    QCOMPARE(header[1], 0U);
    QCOMPARE_NE(header[2], 0U);
    QCOMPARE(header[3], 2U);
    QCOMPARE(header[4], AccessPublic | MethodSignal | MethodRevisioned);
    QCOMPARE(header[5], 6U);    // ###
    const uint *payload = data.data() + header[2];
    QCOMPARE(payload[-1], 0x0509U);
    QCOMPARE(payload[0], uint(QMetaType::Void));

    // signals: void signal(E1, Dummy) [Dummy = QString]
    header += 6;
    QCOMPARE(header[0], 3U);
    QCOMPARE(header[1], 2U);
    QCOMPARE_NE(header[2], 0U);
    QCOMPARE(header[3], 2U);
    QCOMPARE(header[4], AccessPublic | MethodSignal);
    QCOMPARE(header[5], 7U);    // ###
    payload = data.data() + header[2];
    QCOMPARE(payload[0], uint(QMetaType::Void));
    QCOMPARE(payload[1], 0x80000000U | 4);  // not a builtin type
    QCOMPARE(payload[2], 0x80000000U | 5);
    QCOMPARE(payload[3], 6U);
    QCOMPARE(payload[4], 7U);

    // public slots: bool slot(QString &) const
    header += 6;
    QCOMPARE(header[0], 8U);
    QCOMPARE(header[1], 1U);
    QCOMPARE_NE(header[2], 0U);
    QCOMPARE(header[3], 2U);
    QCOMPARE(header[4], AccessPublic | MethodSlot | MethodIsConst);
    QCOMPARE(header[5], 9U);    // ###
    payload = data.data() + header[2];
    QCOMPARE(payload[0], uint(QMetaType::Bool));
    QCOMPARE(payload[1], 0x80000000U | 10); // not a builtin type
    QCOMPARE(payload[2], 11U);
}

void tst_MocHelpers::methodUintGroup()
{
    QTest::setThrowOnFail(true);
    using Dummy = QString;
    constexpr QtMocHelpers::UintData methods = {
        QtMocHelpers::RevisionedSignalData<void()>(1, 2, 6, QtMocConstants::AccessPublic, 0x509,
            QMetaType::Void, {{ }}
        ),
        QtMocHelpers::SignalData<void (E1, Dummy)>(3, 2, 7, QtMocConstants::AccessPublic,
            QMetaType::Void, {{ { 0x80000000 | 4, 6 }, { 0x80000000 | 5, 7 }} }
        ),
        QtMocHelpers::SlotData<bool (QString &) const>(8, 2, 9, QtMocConstants::AccessPublic,
            QMetaType::Bool, {{ { 0x80000000 | 10,  11 } }}
        )
    };
    testUintData(methods);

    constexpr auto data = QtMocHelpers::metaObjectData(0, methods, QtMocHelpers::UintData{},
                                                       QtMocHelpers::UintData{});
    checkMethods(data.data);
}

void tst_MocHelpers::constructorUintData()
{
    constexpr uint NoType = 0x80000000 | 1;
    using namespace QtMocHelpers;
    using namespace QtMocConstants;
    {
        auto result = ConstructorData<QtMocHelpers::NoType()>(1, 2, 0, AccessPublic, NoType, {});
        QCOMPARE(result.header[0], 1U);
        QCOMPARE(result.header[1], 0U);
        QCOMPARE(result.header[3], 2U);
        QCOMPARE(result.header[4], AccessPublic | MethodConstructor);
        QCOMPARE(result.header[5], 0U);
        QCOMPARE(result.payload[0], NoType);
    }
    {
        auto result = ConstructorData<QtMocHelpers::NoType(QObject *)>(0, 1, 0, AccessPublic, NoType,
                                                                       {{ { QMetaType::QObjectStar, 2 } }});
        QCOMPARE(result.header[0], 0U);
        QCOMPARE(result.header[1], 1U);
        QCOMPARE(result.header[3], 1U);
        QCOMPARE(result.header[4], AccessPublic | MethodConstructor);
        QCOMPARE(result.header[5], 1U);
        QCOMPARE(result.payload[0], NoType);
        QCOMPARE(result.payload[1], uint(QMetaType::QObjectStar));
        QCOMPARE(result.payload[2], 2U);
    }
}

template <size_t N> static void checkConstructors(const std::array<uint, N> &data)
{
    using namespace QtMocConstants;
    QCOMPARE(data[10], 3U);
    QCOMPARE_NE(data[11], 0U);

    const uint *header = data.data() + data[11];

    // Constructor(QObject *)
    QCOMPARE(header[0], 0U);
    QCOMPARE(header[1], 1U);
    QCOMPARE_NE(header[2], 0U);
    QCOMPARE(header[3], 1U);
    QCOMPARE(header[4], AccessPublic | MethodConstructor);
    QCOMPARE_GT(header[5], 0U);
    const uint *payload = data.data() + header[2];
    QCOMPARE(payload[0], 0x80000000U | 1);
    QCOMPARE(payload[1], uint(QMetaType::QObjectStar));

    // Constructor() [cloned from the previous with a default argument]
    header += 6;
    QCOMPARE(header[0], 0U);
    QCOMPARE(header[1], 0U);
    QCOMPARE_NE(header[2], 0U);
    QCOMPARE(header[3], 1U);
    QCOMPARE(header[4], AccessPublic | MethodConstructor | MethodCloned);
    QCOMPARE_GT(header[5], 0U);
    payload = data.data() + header[2];
    QCOMPARE(payload[0], 0x80000000U | 1);

    // Constructor(const QString &)
    header += 6;
    QCOMPARE(header[0], 0U);
    QCOMPARE(header[1], 1U);
    QCOMPARE_NE(header[2], 0U);
    QCOMPARE(header[3], 1U);
    QCOMPARE(header[4], AccessPublic | MethodConstructor);
    QCOMPARE_GT(header[5], 0U);
    payload = data.data() + header[2];
    QCOMPARE(payload[0], 0x80000000U | 1);
    QCOMPARE(payload[1], uint(QMetaType::QString));
}

void tst_MocHelpers::constructorUintGroup()
{
    using QtMocHelpers::NoType;
    QTest::setThrowOnFail(true);
    constexpr QtMocHelpers::UintData constructors = {
        QtMocHelpers::ConstructorData<NoType(QObject *)>(0, 1, 1, QtMocConstants::AccessPublic,
            0x80000000 | 1, {{ { QMetaType::QObjectStar, 2 } }}
        ),
        QtMocHelpers::ConstructorData<NoType()>(0, 1, 2, QtMocConstants::AccessPublic | QtMocConstants::MethodCloned,
            0x80000000 | 1, {{ }}
        ),
        QtMocHelpers::ConstructorData<NoType(const QString &)>(0, 1, 2, QtMocConstants::AccessPublic,
            0x80000000 | 1, {{ { QMetaType::QString,  3 }, }}
        )
    };
    testUintData(constructors);

    constexpr auto data = QtMocHelpers::metaObjectData(0,
            QtMocHelpers::UintData{}, QtMocHelpers::UintData{},
            QtMocHelpers::UintData{}, constructors);
    checkConstructors(data.data);
}

template <size_t N> static void checkUintArrayGeneric(const std::array<uint, N> &data, uint flags = 0)
{
    using namespace QtMocConstants;
    QCOMPARE(data[0], uint(OutputRevision));
    QCOMPARE(data[1], 0U);
    QCOMPARE(data[12], flags);
    QCOMPARE(data[N-1], 0U);

    // check the offsets are valid
    QCOMPARE_LT(data[2], N);     // classinfos
    QCOMPARE_LT(data[4], N);     // methods
    QCOMPARE_LT(data[6], N);     // properties
    QCOMPARE_LT(data[8], N);     // enums
    QCOMPARE_LT(data[10], N);    // constructors
}

void tst_MocHelpers::emptyUintArray()
{
    using namespace QtMocConstants;
    constexpr auto data = QtMocHelpers::metaObjectData(MetaObjectFlag{},
            QtMocHelpers::UintData{}, QtMocHelpers::UintData{}, QtMocHelpers::UintData{});
    QTest::setThrowOnFail(true);
    checkUintArrayGeneric(data.data, MetaObjectFlag{});
    QTest::setThrowOnFail(false);

    // check it says nothing was added
    QCOMPARE(data.data[2], 0U);     // classinfos
    QCOMPARE(data.data[4], 0U);     // methods
    QCOMPARE(data.data[6], 0U);     // properties
    QCOMPARE(data.data[8], 0U);     // enums
    QCOMPARE(data.data[10], 0U);    // constructors
    QCOMPARE(data.data[13], 0U);    // signals
}

void tst_MocHelpers::uintArrayNoMethods()
{
    using namespace QtMocConstants;
    constexpr auto data = QtMocHelpers::metaObjectData(PropertyAccessInStaticMetaCall,
            QtMocHelpers::UintData{},
            QtMocHelpers::UintData{
                QtMocHelpers::PropertyData(3, QMetaType::Int, 0x3, 13, 0x101),
                QtMocHelpers::PropertyData(4, 0x80000000 | 5, 0x03),
                QtMocHelpers::PropertyData(6, 0x80000000 | 7, 0x03),
            }, QtMocHelpers::UintData{
                QtMocHelpers::EnumData<E1>(1, 1, 0x00).add({ { 3, E1::AnEnumValue } }),
                QtMocHelpers::EnumData<E3>(4, 5, EnumIsFlag | EnumIsScoped)
                    .add({ { 6, E3::V }, { 8, E3::V2 }, }),
                QtMocHelpers::EnumData<E2>(7, 6, EnumIsFlag | EnumIsScoped)
                    .add({ { 7, E2::V0 }, { 10, E2::V1 }, }),
                QtMocHelpers::EnumData<QFlags<E1>>(11, 1, EnumIsFlag).add({ { 3, E1::AnEnumValue } }),
            }, QtMocHelpers::UintData{}, QtMocHelpers::ClassInfos({{1, 2}, {3, 4}}));

    QTest::setThrowOnFail(true);
    checkUintArrayGeneric(data.data, PropertyAccessInStaticMetaCall);
    checkClassInfos(data.data);
    checkProperties(data.data);
    checkEnums(data.data);
    QTest::setThrowOnFail(false);
    QCOMPARE(data.data[4], 0U);     // methods
    QCOMPARE(data.data[10], 0U);    // constructors
    QCOMPARE(data.data[13], 0U);    // signals
}

void tst_MocHelpers::uintArray()
{
    using Dummy = QString;
    using QtMocHelpers::NoType;
    using namespace QtMocConstants;
    constexpr auto data = QtMocHelpers::metaObjectData(PropertyAccessInStaticMetaCall,
            QtMocHelpers::UintData{
                QtMocHelpers::RevisionedSignalData<void()>(1, 2, 6, QtMocConstants::AccessPublic, 0x509,
                    QMetaType::Void, {{ }}
                ),
                QtMocHelpers::SignalData<void (E1, Dummy)>(3, 2, 7, QtMocConstants::AccessPublic,
                    QMetaType::Void, {{ { 0x80000000 | 4, 6 }, { 0x80000000 | 5, 7 }} }
                ),
                QtMocHelpers::SlotData<bool (QString &) const>(8, 2, 9, QtMocConstants::AccessPublic,
                    QMetaType::Bool, {{ { 0x80000000 | 10,  11 } }}
                )
            },
            QtMocHelpers::UintData{
                QtMocHelpers::PropertyData(3, QMetaType::Int, 0x3, 13, 0x101),
                QtMocHelpers::PropertyData(4, 0x80000000 | 5, 0x03),
                QtMocHelpers::PropertyData(6, 0x80000000 | 7, 0x03),
            }, QtMocHelpers::UintData{
                QtMocHelpers::EnumData<E1>(1, 1, 0x00).add({ { 3, E1::AnEnumValue } }),
                QtMocHelpers::EnumData<E3>(4, 5, EnumIsFlag | EnumIsScoped)
                    .add({ { 6, E3::V }, { 8, E3::V2 }, }),
                QtMocHelpers::EnumData<E2>(7, 6, EnumIsFlag | EnumIsScoped)
                    .add({ { 7, E2::V0 }, { 10, E2::V1 }, }),
                QtMocHelpers::EnumData<QFlags<E1>>(11, 1, EnumIsFlag).add({ { 3, E1::AnEnumValue } }),
            },
            QtMocHelpers::UintData{
                QtMocHelpers::ConstructorData<NoType(QObject *)>(0, 1, 1, QtMocConstants::AccessPublic,
                    0x80000000 | 1, {{ { QMetaType::QObjectStar, 2 } }}
                ),
                QtMocHelpers::ConstructorData<NoType()>(0, 1, 2, QtMocConstants::AccessPublic | QtMocConstants::MethodCloned,
                    0x80000000 | 1, {{ }}
                ),
                QtMocHelpers::ConstructorData<NoType(const QString &)>(0, 1, 3, QtMocConstants::AccessPublic,
                    0x80000000 | 1, {{ { QMetaType::QString,  3 }, }}
                )
            }, QtMocHelpers::ClassInfos({{1, 2}, {3, 4}}));

    QTest::setThrowOnFail(true);
    checkUintArrayGeneric(data.data, PropertyAccessInStaticMetaCall);
    checkClassInfos(data.data);
    checkProperties(data.data);
    checkEnums(data.data);
    checkMethods(data.data);
    checkConstructors(data.data);
}

QTEST_MAIN(tst_MocHelpers)
#include "tst_mochelpers.moc"
