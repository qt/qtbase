// Copyright (C) 2015 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

#include <QtCore/qobject.h>
#include <QtCore/qmetaobject.h>

class tst_QMetaEnum : public QObject
{
    Q_OBJECT
public:
    // these untyped enums are signed
    enum SuperEnum { SuperValue1 = 1, SuperValue2 = INT_MIN };
    enum Flag { Flag1 = 1, Flag2 = INT_MIN };

    // we must force to ": unsigned" to get cross-platform behavior because
    // MSVC always chooses int to back un-fixed enums
    enum UnsignedEnum : unsigned { UnsignedValue1 = 1, UnsignedValue2 = 0x8000'0000 };
    enum Flag32 : unsigned { Flag32_1 = 1, Flag32_2 = 0x8000'0000 };

    enum SignedEnum64 : qint64 {
        SignedValue64_0,
        SignedValue64_1 = 1,
        SignedValue64_Large = Q_INT64_C(1) << 32,
        SignedValue64_M1 = -1,
    };
    enum UnsignedEnum64 : quint64 {
        UnsignedValue64_0 = 0,
        UnsignedValue64_1 = 1,
        UnsignedValue64_Large = Q_UINT64_C(1) << 32,
        UnsignedValue64_Max = ~Q_UINT64_C(0),
    };

    // flags should always be unsigned
    enum Flag64 : quint64 {
        Flag64_1 = 1,
        Flag64_2 = Q_UINT64_C(1) << 31,
        Flag64_3 = Q_UINT64_C(1) << 32,
    };
    Q_DECLARE_FLAGS(Flags, Flag)
    Q_DECLARE_FLAGS(Flags32, Flag32)
    Q_DECLARE_FLAGS(Flags64, Flag64)
    Q_ENUM(SuperEnum)
    Q_ENUM(UnsignedEnum)
    Q_ENUM(SignedEnum64)
    Q_ENUM(UnsignedEnum64)
    Q_FLAG(Flags)
    Q_FLAG(Flags32)
    Q_FLAG(Flags64)

private slots:
    void fromType();
    void keyToValue_data();
    void keyToValue();
    void valuesToKeys_data();
    void valuesToKeys();
    void defaultConstructed();
};

void tst_QMetaEnum::fromType()
{
    QMetaEnum meta = QMetaEnum::fromType<SuperEnum>();
    QVERIFY(meta.isValid());
    QVERIFY(!meta.is64Bit());
    QVERIFY(!meta.isFlag());
    QCOMPARE(meta.name(), "SuperEnum");
    QCOMPARE(meta.enumName(), "SuperEnum");
    QCOMPARE(meta.enclosingMetaObject(), &staticMetaObject);
    QCOMPARE(meta.keyCount(), 2);
    QCOMPARE(meta.metaType(), QMetaType::fromType<SuperEnum>());

    meta = QMetaEnum::fromType<UnsignedEnum>();
    QVERIFY(meta.isValid());
    QVERIFY(!meta.is64Bit());
    QVERIFY(!meta.isFlag());
    QCOMPARE(meta.name(), "UnsignedEnum");
    QCOMPARE(meta.enumName(), "UnsignedEnum");
    QCOMPARE(meta.enclosingMetaObject(), &staticMetaObject);
    QCOMPARE(meta.keyCount(), 2);
    QCOMPARE(meta.metaType(), QMetaType::fromType<UnsignedEnum>());

    meta = QMetaEnum::fromType<SignedEnum64>();
    QVERIFY(meta.isValid());
    QVERIFY(meta.is64Bit());
    QVERIFY(!meta.isFlag());
    QCOMPARE(meta.name(), "SignedEnum64");
    QCOMPARE(meta.enumName(), "SignedEnum64");
    QCOMPARE(meta.enclosingMetaObject(), &staticMetaObject);
    QCOMPARE(meta.keyCount(), 4);
    QCOMPARE(meta.metaType(), QMetaType::fromType<SignedEnum64>());

    meta = QMetaEnum::fromType<UnsignedEnum64>();
    QVERIFY(meta.isValid());
    QVERIFY(meta.is64Bit());
    QVERIFY(!meta.isFlag());
    QCOMPARE(meta.name(), "UnsignedEnum64");
    QCOMPARE(meta.enumName(), "UnsignedEnum64");
    QCOMPARE(meta.enclosingMetaObject(), &staticMetaObject);
    QCOMPARE(meta.keyCount(), 4);
    QCOMPARE(meta.metaType(), QMetaType::fromType<UnsignedEnum64>());

    meta = QMetaEnum::fromType<Flags>();
    QVERIFY(meta.isValid());
    QVERIFY(!meta.is64Bit());
    QVERIFY(meta.isFlag());
    QCOMPARE(meta.name(), "Flags");
    QCOMPARE(meta.enumName(), "Flag");
    QCOMPARE(meta.enclosingMetaObject(), &staticMetaObject);
    QCOMPARE(meta.keyCount(), 2);
    QCOMPARE(meta.metaType(), QMetaType::fromType<Flags>());

    meta = QMetaEnum::fromType<Flags32>();
    QVERIFY(meta.isValid());
    QVERIFY(!meta.is64Bit());
    QVERIFY(meta.isFlag());
    QCOMPARE(meta.name(), "Flags32");
    QCOMPARE(meta.enumName(), "Flag32");
    QCOMPARE(meta.enclosingMetaObject(), &staticMetaObject);
    QCOMPARE(meta.keyCount(), 2);
    QCOMPARE(meta.metaType(), QMetaType::fromType<Flags32>());

    meta = QMetaEnum::fromType<Flags64>();
    QVERIFY(meta.isValid());
    QVERIFY(meta.is64Bit());
    QVERIFY(meta.isFlag());
    QCOMPARE(meta.name(), "Flags64");
    QCOMPARE(meta.enumName(), "Flag64");
    QCOMPARE(meta.enclosingMetaObject(), &staticMetaObject);
    QCOMPARE(meta.keyCount(), 3);
    QCOMPARE(meta.metaType(), QMetaType::fromType<Flags64>());
}

Q_DECLARE_METATYPE(Qt::WindowFlags)

template <typename E> quint64 toUnderlying(E e, std::enable_if_t<std::is_enum_v<E>, bool> = true)
{
    // keep signedness if it's just an enum
    return qToUnderlying(e);
}

template <typename E> quint64 toUnderlying(QFlags<E> f)
{
    // force to unsigned so we zero-extend if it's QFlags
    return typename QIntegerForSizeof<E>::Unsigned(f);
}

void tst_QMetaEnum::keyToValue_data()
{
    QTest::addColumn<QMetaEnum>("me");
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<quint64>("value");
    QTest::addColumn<bool>("success");

    QByteArray notfoundkey = QByteArray::fromRawData("Foobar", 6);
    auto addNotFoundRow = [&](auto value) {
        QMetaEnum me = QMetaEnum::fromType<decltype(value)>();

        QTest::addRow("notfound-%s", me.name())
                << me << notfoundkey << quint64(value) << false;
    };
    auto addRow = [&](const char *name, auto value) {
        using T = decltype(value);
        QMetaEnum me = QMetaEnum::fromType<T>();
        QTest::addRow("%s", name) << me << QByteArray(name) << toUnderlying(value) << true;

        if constexpr (sizeof(value) == sizeof(int)) {
            // repeat with the upper half negated
            quint64 v = toUnderlying(value);
            v ^= Q_UINT64_C(0xffff'ffff'0000'0000);
            QTest::addRow("mangled-%s", name) << me << notfoundkey << v << false;
        }
    };
    addRow("Window", Qt::Window);
    addRow("Dialog", Qt::Dialog);
    addRow("WindowFullscreenButtonHint", Qt::WindowFullscreenButtonHint);

    addNotFoundRow(SuperEnum(2));
    addRow("SuperValue1", SuperValue1);
    addRow("SuperValue2", SuperValue2);

    addNotFoundRow(Flags(2));
    addRow("Flag1", Flags(Flag1));
    addRow("Flag2", Flags(Flag2));

    addNotFoundRow(UnsignedEnum(2));
    addRow("UnsignedValue1", UnsignedValue1);
    addRow("UnsignedValue2", UnsignedValue2);

    addNotFoundRow(Flags32(2));
    addRow("Flag32_1", Flags32(Flag32_1));
    addRow("Flag32_2", Flags32(Flag32_2));

    addNotFoundRow(SignedEnum64(2));
    addRow("SignedValue64_0", SignedValue64_0);
    addRow("SignedValue64_1", SignedValue64_1);
    addRow("SignedValue64_Large", SignedValue64_Large);
    addRow("SignedValue64_M1", SignedValue64_M1);

    addNotFoundRow(UnsignedEnum64(2));
    addRow("UnsignedValue64_0", UnsignedValue64_0);
    addRow("UnsignedValue64_1", UnsignedValue64_1);
    addRow("UnsignedValue64_Large", UnsignedValue64_Large);
    addRow("UnsignedValue64_Max", UnsignedValue64_Max);

    addNotFoundRow(Flags64(std::in_place, 2));
    addRow("Flag64_1", Flags64(Flag64_1));
    addRow("Flag64_2", Flags64(Flag64_2));
    addRow("Flag64_3", Flags64(Flag64_3));
}

void tst_QMetaEnum::keyToValue()
{
    QFETCH(QMetaEnum, me);
    QFETCH(quint64, value);
    QFETCH(QByteArray, key);
    QFETCH(bool, success);

    if (!key.isEmpty()) {
        // look up value of key
        if (value == uint(value)) {
            bool ok;
            int value32 = success ? value : -1;
            int result32 = me.keyToValue(key, &ok);
            QCOMPARE(ok, success);
            QCOMPARE(result32, value32);

            result32 = me.keysToValue(key, &ok);
            QCOMPARE(ok, success);
            QCOMPARE(result32, value32);
        }

        std::optional value64 = me.keyToValue64(key);
        QCOMPARE(value64.has_value(), success);
        if (success)
            QCOMPARE(*value64, value);

        value64 = me.keysToValue64(key);
        QCOMPARE(value64.has_value(), success);
        if (success)
            QCOMPARE(*value64, value);
    }

    // look up value
    QByteArray expected = success ? key : QByteArray();
    QByteArray result = me.valueToKey(value);
    QCOMPARE(result, expected);

    result = me.valueToKeys(value);
    QCOMPARE(result, expected);
}

void tst_QMetaEnum::valuesToKeys_data()
{
   QTest::addColumn<QMetaEnum>("me");
   QTest::addColumn<quint64>("flags");
   QTest::addColumn<QByteArray>("expected");

   QTest::newRow("Window")
       << QMetaEnum::fromType<Qt::WindowFlags>()
       << quint64(Qt::Window)
       << QByteArrayLiteral("Window");

   // Verify that Qt::Dialog does not cause 'Window' to appear in the output.
   QTest::newRow("Frameless_Dialog")
       << QMetaEnum::fromType<Qt::WindowFlags>()
       << quint64(Qt::Dialog | Qt::FramelessWindowHint)
       << QByteArrayLiteral("Dialog|FramelessWindowHint");

   // Similarly, Qt::WindowMinMaxButtonsHint should not show up as
   // WindowMinimizeButtonHint|WindowMaximizeButtonHint
   QTest::newRow("Tool_MinMax_StaysOnTop")
       << QMetaEnum::fromType<Qt::WindowFlags>()
       << quint64(Qt::Tool | Qt::WindowMinMaxButtonsHint | Qt::WindowStaysOnTopHint)
       << QByteArrayLiteral("Tool|WindowMinMaxButtonsHint|WindowStaysOnTopHint");

   // Verify that upper bits set don't cause a mistaken detection
   QTest::newRow("upperbits-Window")
           << QMetaEnum::fromType<Qt::WindowFlags>()
           << (quint64(Qt::Window) | Q_UINT64_C(0x1'0000'0000))
           << QByteArray();

   QTest::newRow("INT_MIN-as-enum")
           << QMetaEnum::fromType<SuperEnum>()
           << quint64(SuperValue2)
           << QByteArrayLiteral("SuperValue2");
   QTest::newRow("mangled-INT_MIN-as-enum")
           << QMetaEnum::fromType<SuperEnum>()
           << quint64(uint(SuperValue2))
           << QByteArray();

   QTest::newRow("INT_MIN-as-flags")
           << QMetaEnum::fromType<Flags>()
           << quint64(uint(Flag2))
           << QByteArrayLiteral("Flag2");
   QTest::newRow("mangled-INT_MIN-as-flags")
           << QMetaEnum::fromType<Flags>()
           << quint64(Flag2)
           << QByteArray();

   QTest::newRow("Flag32_2")
           << QMetaEnum::fromType<Flags32>()
           << quint64(uint(Flag32_2))
           << QByteArrayLiteral("Flag32_2");
   QTest::newRow("mangled-Flag32_2")
           << QMetaEnum::fromType<Flags32>()
           << quint64(int(Flag32_2))
           << QByteArray();

   QTest::newRow("Flag64_all")
           << QMetaEnum::fromType<Flags64>()
           << quint64(Flag64_1 | Flag64_2 | Flag64_3)
           << QByteArrayLiteral("Flag64_1|Flag64_2|Flag64_3");
}

void tst_QMetaEnum::valuesToKeys()
{
    QFETCH(QMetaEnum, me);
    QFETCH(quint64, flags);
    QFETCH(QByteArray, expected);

    QCOMPARE(me.valueToKeys(flags), expected);
    if (!expected.isEmpty()) {
        bool ok = false;
        QCOMPARE(uint(me.keysToValue(expected, &ok)), uint(flags));
        QVERIFY(ok);
        QCOMPARE(me.keysToValue64(expected), flags);
    }
}

void tst_QMetaEnum::defaultConstructed()
{
    QMetaEnum e;
    QVERIFY(!e.isValid());
    QVERIFY(!e.isScoped());
    QVERIFY(!e.isFlag());
    QVERIFY(!e.is64Bit());
    QCOMPARE(e.name(), QByteArray());
    QCOMPARE(e.scope(), QByteArray());
    QCOMPARE(e.enclosingMetaObject(), nullptr);
    QCOMPARE(e.keyCount(), 0);
    QCOMPARE(e.metaType(), QMetaType());
}

static_assert(QtPrivate::IsQEnumHelper<tst_QMetaEnum::SuperEnum>::Value);
static_assert(QtPrivate::IsQEnumHelper<Qt::WindowFlags>::Value);
static_assert(QtPrivate::IsQEnumHelper<Qt::Orientation>::Value);
static_assert(!QtPrivate::IsQEnumHelper<int>::Value);
static_assert(!QtPrivate::IsQEnumHelper<QObject>::Value);
static_assert(!QtPrivate::IsQEnumHelper<QObject*>::Value);
static_assert(!QtPrivate::IsQEnumHelper<void>::Value);

QTEST_MAIN(tst_QMetaEnum)
#include "tst_qmetaenum.moc"
