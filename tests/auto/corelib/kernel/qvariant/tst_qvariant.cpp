// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qvariant.h>

#include <QtCore/qttypetraits.h>

// don't assume <type_traits>
template <typename T, typename U>
constexpr inline bool my_is_same_v = false;
template <typename T>
constexpr inline bool my_is_same_v<T, T> = true;

#define CHECK_IMPL(func, arg, Variant, cvref, R) \
    static_assert(my_is_same_v<decltype( func < arg >(std::declval< Variant cvref >())), R cvref >)

#define CHECK_GET_IF(Variant, cvref) \
    CHECK_IMPL(get_if, int, Variant, cvref *, int)

#define CHECK_GET(Variant, cvref) \
    CHECK_IMPL(get, int, Variant, cvref, int)

CHECK_GET_IF(QVariant, /* unadorned */);
CHECK_GET_IF(QVariant, const);

CHECK_GET(QVariant, &);
CHECK_GET(QVariant, const &);
CHECK_GET(QVariant, &&);
CHECK_GET(QVariant, const &&);

// check for a type derived from QVariant:

struct MyVariant : QVariant
{
    using QVariant::QVariant;
};

CHECK_GET_IF(MyVariant, /* unadorned */);
CHECK_GET_IF(MyVariant, const);

CHECK_GET(MyVariant, &);
CHECK_GET(MyVariant, const &);
CHECK_GET(MyVariant, &&);
CHECK_GET(MyVariant, const &&);

#undef CHECK_GET_IF
#undef CHECK_GET
#undef CHECK_IMPL

#include <QTest>

// In each group:
// Please stick to alphabetic order.
#include <QtGui/qtransform.h>

// QtCore:
#include <QAssociativeIterable>
#include <QBitArray>
#include <QBuffer>
#include <QByteArrayList>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QEasingCurve>
#include <QMap>
#include <QIODevice>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QQueue>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSequentialIterable>
#include <QSet>
#include <QStack>
#include <QTimeZone>
#include <QtNumeric>
#include <QUrl>
#include <QUuid>

#include <private/qcomparisontesthelper_p.h>
#include <private/qlocale_p.h>
#include <private/qmetatype_p.h>
#include "tst_qvariant_common.h"

#include <limits>
#include <float.h>
#include <cmath>
#include <variant>
#include <unordered_map>

using namespace Qt::StringLiterals;

class CustomNonQObject;
struct NonDefaultConstructible;

template<typename T, typename  = void>
struct QVariantFromValueCompiles
{
    static inline constexpr bool value = false;
};

template<typename T>
struct QVariantFromValueCompiles<T, std::void_t<decltype (QVariant::fromValue(std::declval<T>()))>>
{
    static inline constexpr bool value = true;
};

static_assert(QVariantFromValueCompiles<int>::value);
static_assert(!QVariantFromValueCompiles<QObject>::value);

enum EnumTest_Enum0 { EnumTest_Enum0_value = 42, EnumTest_Enum0_negValue = -8 };
Q_DECLARE_METATYPE(EnumTest_Enum0)
enum EnumTest_Enum1 : qint64 { EnumTest_Enum1_value = 42, EnumTest_Enum1_bigValue = (Q_INT64_C(1) << 33) + 50 };
Q_DECLARE_METATYPE(EnumTest_Enum1)

enum EnumTest_Enum3 : qint64 { EnumTest_Enum3_value = -47, EnumTest_Enum3_bigValue = (Q_INT64_C(1) << 56) + 5  };
Q_DECLARE_METATYPE(EnumTest_Enum3)
enum EnumTest_Enum4 : quint64 { EnumTest_Enum4_value = 47, EnumTest_Enum4_bigValue = (Q_INT64_C(1) << 52) + 45 };
Q_DECLARE_METATYPE(EnumTest_Enum4)
enum EnumTest_Enum5 : uint { EnumTest_Enum5_value = 47 };
Q_DECLARE_METATYPE(EnumTest_Enum5)
enum EnumTest_Enum6 : uchar { EnumTest_Enum6_value = 47 };
Q_DECLARE_METATYPE(EnumTest_Enum6)
enum class EnumTest_Enum7 { EnumTest_Enum7_value = 47, ensureSignedEnum7 = -1 };
Q_DECLARE_METATYPE(EnumTest_Enum7)
enum EnumTest_Enum8 : short { EnumTest_Enum8_value = 47 };
Q_DECLARE_METATYPE(EnumTest_Enum8)

template <typename T> int qToUnderlying(QFlags<T> f)
{
    return f.toInt();
}

class tst_QVariant : public QObject
{
    Q_OBJECT

    static void runTestFunction()
    {
        QFETCH(QFunctionPointer, testFunction);
        testFunction();
    }

public:
    tst_QVariant(QObject *parent = nullptr)
      : QObject(parent), customNonQObjectPointer(0)
    {
    }


    enum MetaEnumTest_Enum0 { MetaEnumTest_Enum0_dummy = 2, MetaEnumTest_Enum0_value = 42, MetaEnsureSignedEnum0 = -1 };
    Q_ENUM(MetaEnumTest_Enum0)
    enum MetaEnumTest_Enum1 : qint64 { MetaEnumTest_Enum1_value = 42, MetaEnumTest_Enum1_bigValue = (Q_INT64_C(1) << 33) + 50 };
    Q_ENUM(MetaEnumTest_Enum1)

    enum MetaEnumTest_Enum3 : qint64 { MetaEnumTest_Enum3_value = -47, MetaEnumTest_Enum3_bigValue = (Q_INT64_C(1) << 56) + 5, MetaEnumTest_Enum3_bigNegValue = -(Q_INT64_C(1) << 56) - 3 };
    Q_ENUM(MetaEnumTest_Enum3)
    enum MetaEnumTest_Enum4 : quint64 { MetaEnumTest_Enum4_value = 47, MetaEnumTest_Enum4_bigValue = (Q_INT64_C(1) << 52) + 45 };
    Q_ENUM(MetaEnumTest_Enum4)
    enum MetaEnumTest_Enum5 : uint { MetaEnumTest_Enum5_value = 47 };
    Q_ENUM(MetaEnumTest_Enum5)
    enum MetaEnumTest_Enum6 : uchar { MetaEnumTest_Enum6_value = 47 };
    Q_ENUM(MetaEnumTest_Enum6)
    enum MetaEnumTest_Enum8 : short { MetaEnumTest_Enum8_value = 47 };
    Q_ENUM(MetaEnumTest_Enum8)

private slots:
    void cleanupTestCase();

    void constructor();
    void copy_constructor();
    void constructor_invalid_data();
    void constructor_invalid();
    void isNull();
    void swap();

    void canConvert_data();
    void canConvert();

    void canConvertAndConvert_ReturnFalse_WhenConvertingBetweenPointerAndValue_data();
    void canConvertAndConvert_ReturnFalse_WhenConvertingBetweenPointerAndValue();

    void canConvertAndConvert_ReturnFalse_WhenConvertingQObjectBetweenPointerAndValue();

    void convert();

    void toSize_data();
    void toSize();

    void toSizeF_data();
    void toSizeF();

    void toPoint_data();
    void toPoint();

    void toRect_data();
    void toRect();

    void toChar_data();
    void toChar();

    void toLine_data();
    void toLine();

    void toLineF_data();
    void toLineF();

    void toInt_data();
    void toInt();

    void toUInt_data();
    void toUInt();

    void toBool_data();
    void toBool();

    void toSChar_data();
    void toSChar();

    void toUChar_data();
    void toUChar();

    void toShort_data();
    void toShort();

    void toUShort_data();
    void toUShort();

    void toLong_data();
    void toLong();

    void toULong_data();
    void toULong();

    void toLongLong_data();
    void toLongLong();

    void toULongLong_data();
    void toULongLong();

    void toByteArray_data();
    void toByteArray();

    void toString_data();
    void toString();

    void toDate_data();
    void toDate();

    void toTime_data();
    void toTime();

    void toDateTime_data();
    void toDateTime();

    void toDouble_data();
    void toDouble();

    void toFloat_data();
    void toFloat();

    void toFloat16_data();
    void toFloat16();

    void toPointF_data();
    void toPointF();

    void toRectF_data();
    void toRectF();

    void qvariant_cast_QObject_data();
    void qvariant_cast_QObject();
    void qvariant_cast_QObject_derived();
    void qvariant_cast_QObject_wrapper();
    void qvariant_cast_QSharedPointerQObject();
    void qvariant_cast_const();
    void qvariant_cast_QTransform();

    void toLocale();

    void toRegularExpression();

    void url();

    void userType();
    void basicUserType();

    void variant_to();

    void writeToReadFromDataStream_data();
    void writeToReadFromDataStream();
    void writeToReadFromOldDataStream();
    void checkDataStream();

    void operator_eq_eq_data();
    void operator_eq_eq();

#if QT_DEPRECATED_SINCE(6, 0)
    void typeName_data();
    void typeName();
    void typeToName();
#endif

    void streamInvalidVariant();

    void podUserType();

    void data();
    void constData();

    void saveLoadCustomTypes();

    void variantMap();
    void variantHash();

    void convertToQUint8() const;
    void compareCompiles() const;
    void compareNumerics_data() const;
    void compareNumerics() const;
    void comparePointers() const;
    void voidStar() const;
    void dataStar() const;
    void canConvertQStringList() const;
    void canConvertQStringList_data() const;
    void canConvertMetaTypeToInt() const;
    void variantToDateTimeWithoutWarnings() const;
    void invalidDateTime() const;

    void loadUnknownUserType();
    void loadBrokenUserType();

    void invalidDate() const;
    void compareCustomTypes_data() const;
    void compareCustomTypes() const;
    void timeToDateTime() const;
    void copyingUserTypes() const;
    void valueClassHierarchyConversion() const;
    void convertBoolToByteArray() const;
    void convertBoolToByteArray_data() const;
    void convertByteArrayToBool() const;
    void convertByteArrayToBool_data() const;
    void convertIterables() const;
    void convertConstNonConst() const;
    void toIntFromQString() const;
    void toIntFromDouble() const;
    void setValue();
    void fpStringRoundtrip_data() const;
    void fpStringRoundtrip() const;

    void numericalConvert_data();
    void numericalConvert();
    void moreCustomTypes();
    void movabilityTest();
    void variantInVariant();
    void userConversion();
    void modelIndexConversion();

    void forwardDeclare();
    void debugStream_data();
    void debugStream();
#if QT_DEPRECATED_SINCE(6, 0)
    void debugStreamType_data();
    void debugStreamType();
#endif

    void loadQt4Stream_data();
    void loadQt4Stream();
    void saveQt4Stream_data();
    void saveQt4Stream();
    void loadQt5Stream_data();
    void loadQt5Stream();
    void saveQt5Stream_data();
    void saveQt5Stream();
    void saveInvalid_data();
    void saveInvalid();
    void saveNewBuiltinWithOldStream();

    void implicitConstruction();

    void iterateSequentialContainerElements_data();
    void iterateSequentialContainerElements() { runTestFunction(); }
    void iterateAssociativeContainerElements_data();
    void iterateAssociativeContainerElements() { runTestFunction(); }
    void iterateContainerElements();
    void pairElements_data();
    void pairElements() { runTestFunction(); }

    void enums_data();
    void enums() { runTestFunction(); }
    void metaEnums_data();
    void metaEnums() { runTestFunction(); }

    void nullConvert();

    void accessSequentialContainerKey();
    void shouldDeleteVariantDataWorksForSequential();
    void shouldDeleteVariantDataWorksForAssociative();
    void fromStdVariant();
    void qt4UuidDataStream();
    void sequentialIterableEndianessSanityCheck();
    void sequentialIterableAppend();

    void preferDirectConversionOverInterfaces();
    void mutableView();

    void canViewAndView_ReturnFalseAndDefault_WhenConvertingBetweenPointerAndValue();

    void moveOperations();
    void equalsWithoutMetaObject();

    void constructFromIncompatibleMetaType_data();
    void constructFromIncompatibleMetaType();
    void constructFromQtLT65MetaType();
    void copyNonDefaultConstructible();

    void inplaceConstruct();
    void emplace();

    void getIf_int() { getIf_impl(42); }
    void getIf_QString() { getIf_impl(u"string"_s); };
    void getIf_QTransform() { getIf_impl(QTransform{1, 2, 3, 4, 5, 6, 7, 8, 9}); } // too large
    void getIf_NonDefaultConstructible();
    void getIfSpecial();

    void get_int() { get_impl(42); }
    void get_QString() { get_impl(u"string"_s); }
    void get_QTransform() { get_impl(QTransform{1, 2, 3, 4, 5, 6, 7, 8, 9}); } // too large
    void get_NonDefaultConstructible();

private:
    using StdVariant = std::variant<std::monostate,
            // list here all the types with which we instantiate getIf_impl:
            int,
            QString,
            QTransform,
            NonDefaultConstructible
        >;
    template <typename T>
    void getIf_impl(T t) const;
    template <typename T>
    void get_impl(T t) const;
    template<typename T>
    void canViewAndView_ReturnFalseAndDefault_WhenConvertingBetweenPointerAndValue_impl(const QByteArray &typeName);
    void dataStream_data(QDataStream::Version version);
    void loadQVariantFromDataStream(QDataStream::Version version);
    void saveQVariantFromDataStream(QDataStream::Version version);

    CustomNonQObject *customNonQObjectPointer;
    QList<QObject*> objectPointerTestData;
};

const qlonglong intMax1 = (qlonglong)INT_MAX + 1;
const qulonglong uintMax1 = (qulonglong)UINT_MAX + 1;

void tst_QVariant::constructor()
{
    QVariant variant;
    QVERIFY( !variant.isValid() );
    QVERIFY( variant.isNull() );

    QVariant var2(variant);
    QVERIFY( !var2.isValid() );
    QVERIFY( variant.isNull() );

    QVariant varll(intMax1);
    QVariant varll2(varll);
    QCOMPARE(varll2, varll);

    QVariant var3 {QMetaType::fromType<QString>()};
    QCOMPARE(var3.typeName(), "QString");
    QVERIFY(var3.isNull());
    QVERIFY(var3.isValid());

    QVariant var3a = QVariant::fromMetaType(QMetaType::fromType<QString>());
    QCOMPARE(var3a.typeName(), "QString");
    QVERIFY(var3a.isNull());
    QVERIFY(var3a.isValid());

    QVariant var4 {QMetaType()};
    QCOMPARE(var4.typeId(), QMetaType::UnknownType);
    QVERIFY(var4.isNull());
    QVERIFY(!var4.isValid());

    QVariant var4a = QVariant::fromMetaType(QMetaType());
    QCOMPARE(var4a.typeId(), QMetaType::UnknownType);
    QVERIFY(var4a.isNull());
    QVERIFY(!var4a.isValid());

    QVariant var5(QLatin1String("hallo"));
    QCOMPARE(var5.typeId(), QMetaType::QString);
    QCOMPARE(var5.typeName(), "QString");

    QVariant var6(qlonglong(0));
    QCOMPARE(var6.typeId(), QMetaType::LongLong);
    QCOMPARE(var6.typeName(), "qlonglong");

    QVariant var7 = 5;
    QVERIFY(var7.isValid());
    QVERIFY(!var7.isNull());
    QVariant var8;
    var8.setValue(5);
    QVERIFY(var8.isValid());
    QVERIFY(!var8.isNull());
}

void tst_QVariant::constructor_invalid_data()
{
    QTest::addColumn<uint>("typeId");

    QTest::newRow("-1") << uint(-1);
    QTest::newRow("-122234567") << uint(-122234567);
    QTest::newRow("0xfffffffff") << uint(0xfffffffff);
    QTest::newRow("LastCoreType + 1") << uint(QMetaType::LastCoreType + 1);
    QVERIFY(!QMetaType::isRegistered(QMetaType::LastCoreType + 1));
}

void tst_QVariant::constructor_invalid()
{

    QFETCH(uint, typeId);
    {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Trying to construct an instance of an invalid type"));
        QVariant variant {QMetaType(typeId)};
        QVERIFY(!variant.isValid());
        QVERIFY(variant.isNull());
        QCOMPARE(variant.typeId(), int(QMetaType::UnknownType));
        QCOMPARE(variant.userType(), int(QMetaType::UnknownType));
    }
    {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Trying to construct an instance of an invalid type"));
        QVariant variant = QVariant::fromMetaType(QMetaType(typeId));
        QVERIFY(!variant.isValid());
        QVERIFY(variant.isNull());
        QCOMPARE(variant.typeId(), int(QMetaType::UnknownType));
        QCOMPARE(variant.userType(), int(QMetaType::UnknownType));
    }
    {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Trying to construct an instance of an invalid type"));
        QVariant variant(QMetaType(typeId), /* copy */ nullptr);
        QVERIFY(!variant.isValid());
        QVERIFY(variant.isNull());
        QCOMPARE(variant.userType(), int(QMetaType::UnknownType));
    }
}

void tst_QVariant::copy_constructor()
{
    QVariant var7 {QMetaType::fromType<int>()};
    QVariant var8(var7);
    QCOMPARE(var8.typeId(), QMetaType::Int);
    QVERIFY(var8.isNull());
}

Q_DECLARE_METATYPE(int*)

void tst_QVariant::isNull()
{
    QVariant var;
    QVERIFY( var.isNull() );

    QVariant empty = QString();
    QVERIFY(empty.toString().isNull());
    QVERIFY(!empty.isNull());
    QVERIFY(empty.isValid());
    QCOMPARE(empty.typeName(), "QString");

    QVariant var3( QString( "blah" ) );
    QVERIFY( !var3.isNull() );

    var3 = QVariant(QMetaType::fromType<QString>());
    QVERIFY( var3.isNull() );

    var3.setValue(QString());
    QVERIFY( !var3.isNull() );

    QVariant var4( 0 );
    QVERIFY( !var4.isNull() );

    QVariant var5 = QString();
    QVERIFY( !var5.isNull() );

    QVariant var6( QString( "blah" ) );
    QVERIFY( !var6.isNull() );
    var6 = QVariant();
    QVERIFY( var6.isNull() );
    var6.convert(QMetaType::fromType<QString>());
    QVERIFY( var6.isNull() );
    QVariant varLL( (qlonglong)0 );
    QVERIFY( !varLL.isNull() );

    QVariant var8(QMetaType::fromType<std::nullptr_t>(), nullptr);
    QVERIFY(var8.isNull());
    var8 = QVariant::fromValue<std::nullptr_t>(nullptr);
    QVERIFY(var8.isNull());
    QVariant var9 = QVariant(QJsonValue(QJsonValue::Null));
    QVERIFY(!var9.isNull());
    var9 = QVariant::fromValue<QJsonValue>(QJsonValue(QJsonValue::Null));
    QVERIFY(!var9.isNull());

    QVariant var10(QMetaType::fromType<void*>(), nullptr);
    QVERIFY(var10.isNull());
    var10 = QVariant::fromValue<void*>(nullptr);
    QVERIFY(var10.isNull());

    QVariant var11(QMetaType::fromType<QObject*>(), nullptr);
    QVERIFY(var11.isNull());
    var11 = QVariant::fromValue<QObject*>(nullptr);
    QVERIFY(var11.isNull());

    QVERIFY(QVariant::fromValue<int*>(nullptr).isNull());

    QVariant var12(QVariant::fromValue<QString>(QString()));
    QVERIFY(!var12.isNull());
}

void tst_QVariant::swap()
{
    QVariant v1 = 1, v2 = 2.0;
    v1.swap(v2);
    QCOMPARE(v1.typeId(), QMetaType::Double);
    QCOMPARE(v1.toDouble(),2.0);
    QCOMPARE(v2.typeId(), QMetaType::Int);
    QCOMPARE(v2.toInt(),1);
}

void tst_QVariant::canConvert_data()
{
    TST_QVARIANT_CANCONVERT_DATATABLE_HEADERS


#ifdef Y
#undef Y
#endif
#ifdef N
#undef N
#endif
#define Y true
#define N false
    //            bita bitm bool brsh byta col  curs date dt   dbl  font img  int  inv  kseq list ll   map  pal  pen  pix  pnt  rect reg  size sp   str  strl time uint ull


    QVariant var(QBitArray(0));
    QTest::newRow("BitArray")
        << var << Y << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N;
    var = QVariant(QByteArray());
    QTest::newRow("ByteArray")
        << var << N << N << Y << N << Y << Y << N << N << N << Y << N << N << Y << N << N << Y << Y << N << N << N << N << N << N << N << N << N << Y << N << N << Y << Y;
    var = QVariant(QDate());
    QTest::newRow("Date")
        << var << N << N << N << N << N << N << N << Y << Y << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N;
    var = QVariant(QDateTime());
    QTest::newRow("DateTime")
        << var << N << N << N << N << N << N << N << Y << Y << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << Y << N << N;
    var = QVariant((double)0.1);
    QTest::newRow("Double")
        << var << N << N << Y << N << Y << N << N << N << N << Y << N << N << Y << N << N << N << Y << N << N << N << N << N << N << N << N << N << Y << N << N << Y << Y;
    var = QVariant(0.1f);
    QTest::newRow("Float")
        << var << N << N << Y << N << Y << N << N << N << N << Y << N << N << Y << N << N << N << Y << N << N << N << N << N << N << N << N << N << Y << N << N << Y << Y;
    var = QVariant((int)1);
    QTest::newRow("Int")
        << var << N << N << Y << N << Y << N << N << N << N << Y << N << N << Y << N << Y << N << Y << N << N << N << N << N << N << N << N << N << Y << N << N << Y << Y;
    var = QVariant();
    QTest::newRow("Invalid")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N;
    var = QVariant(QList<QVariant>());
    QTest::newRow("List")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N;
    var = QVariant((qlonglong)1);
    QTest::newRow("LongLong")
        << var << N << N << Y << N << Y << N << N << N << N << Y << N << N << Y << N << N << N << Y << N << N << N << N << N << N << N << N << N << Y << N << N << Y << Y;
    var = QVariant(QMap<QString,QVariant>());
    QTest::newRow("Map")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << N << N << N << N << N;
    var = QVariant(QPoint());
    QTest::newRow("Point")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << N;
    var = QVariant(QRect());
    QTest::newRow("Rect")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N;
    var = QVariant(QSize());
    QTest::newRow("Size")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N;
    var = QVariant(QString());
    QTest::newRow("String")
        << var << N << N << Y << N << Y << Y << N << Y << Y << Y << Y << N << Y << N << Y << Y << Y << N << N << N << N << N << N << N << N << N << Y << Y << Y << Y << Y;
   var = QVariant(QStringList("entry"));
    QTest::newRow("StringList")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << N << N << Y << Y << N << N << N;
    var = QVariant(QTime());
    QTest::newRow("Time")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << Y << N << N;
    var = QVariant((uint)1);
    QTest::newRow("UInt")
        << var << N << N << Y << N << Y << N << N << N << N << Y << N << N << Y << N << N << N << Y << N << N << N << N << N << N << N << N << N << Y << N << N << Y << Y;
    var = QVariant((qulonglong)1);
    QTest::newRow("ULongLong")
        << var << N << N << Y << N << Y << N << N << N << N << Y << N << N << Y << N << N << N << Y << N << N << N << N << N << N << N << N << N << Y << N << N << Y << Y;
    var = QVariant::fromValue('a');
    QTest::newRow("Char")
        << var << N << N << Y << N << Y << N << N << N << N << Y << N << N << Y << N << N << N << Y << N << N << N << N << N << N << N << N << N << Y << N << N << Y << Y;
    var = QVariant::fromValue<signed char>(-1);
    QTest::newRow("SChar")
        << var << N << N << Y << N << Y << N << N << N << N << Y << N << N << Y << N << N << N << Y << N << N << N << N << N << N << N << N << N << Y << N << N << Y << Y;
    var = QVariant::fromValue((short)-3);
    QTest::newRow("Short")
        << var << N << N << Y << N << Y << N << N << N << N << Y << N << N << Y << N << N << N << Y << N << N << N << N << N << N << N << N << N << Y << N << N << Y << Y;
    var = QVariant::fromValue((ushort)7);
    QTest::newRow("UShort")
        << var << N << N << Y << N << Y << N << N << N << N << Y << N << N << Y << N << N << N << Y << N << N << N << N << N << N << N << N << N << Y << N << N << Y << Y;
    var = QVariant::fromValue<QJsonValue>(QJsonValue(QStringLiteral("hello")));
    QTest::newRow("JsonValue")
        << var << N << N << Y << N << N << N << N << N << N << Y << N << N << Y << N << N << Y << Y << Y << N << N << N << N << N << N << N << N << Y << N << N << Y << Y;
    var = QVariant::fromValue<QJsonArray>(QJsonArray());
    QTest::newRow("JsonArray")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N;
    var = QVariant::fromValue<QJsonObject>(QJsonObject());
    QTest::newRow("JsonObject")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << N << N << N << N << N;

#undef N
#undef Y
}

void tst_QVariant::canConvert()
{
    TST_QVARIANT_CANCONVERT_FETCH_DATA

    // This test links against QtGui but not QtWidgets, so QSizePolicy isn't real for it.
    QTest::ignoreMessage(QtWarningMsg, // QSizePolicy's id is 0x2000, a.k.a. 8192
                         "Trying to construct an instance of an invalid type, type id: 8192");
    TST_QVARIANT_CANCONVERT_COMPARE_DATA

#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    // Invalid type ids
    QTest::ignoreMessage(QtWarningMsg,
                         "Trying to construct an instance of an invalid type, type id: -1");
    QCOMPARE(val.canConvert(-1), false);
    QTest::ignoreMessage(QtWarningMsg,
                         "Trying to construct an instance of an invalid type, type id: -23");
    QCOMPARE(val.canConvert(-23), false);
    QTest::ignoreMessage(QtWarningMsg,
                         "Trying to construct an instance of an invalid type, type id: -23876");
    QCOMPARE(val.canConvert(-23876), false);
    QTest::ignoreMessage(QtWarningMsg,
                         "Trying to construct an instance of an invalid type, type id: 23876");
    QCOMPARE(val.canConvert(23876), false);
QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 0)
}

namespace {

// Used for testing canConvert/convert of QObject derived types
struct QObjectDerived : QObject
{
    Q_OBJECT
};

// Adds a test table row for checking value <-> pointer conversion
// If type is a pointer, the target type is value type and vice versa.
template<typename T>
void addRowForPointerValueConversion()
{
    using ValueType = std::remove_pointer_t<T>;
    if constexpr (!std::is_same_v<ValueType, std::nullptr_t>) {

        static ValueType instance{}; // static since we may need a pointer to a valid object

        QVariant variant;
        if constexpr (std::is_pointer_v<T>)
            variant = QVariant::fromValue(&instance);
        else
            variant = QVariant::fromValue(instance);

        // Toggle pointer/value type
        using TargetType = std::conditional_t<std::is_pointer_v<T>, ValueType, T *>;

        const QMetaType fromType = QMetaType::fromType<T>();
        const QMetaType toType = QMetaType::fromType<TargetType>();

        QTest::addRow("%s->%s", fromType.name(), toType.name())
                << variant << QMetaType::fromType<TargetType>();
    }
}

} // namespace

void tst_QVariant::canConvertAndConvert_ReturnFalse_WhenConvertingBetweenPointerAndValue_data()
{
    QTest::addColumn<QVariant>("variant");
    QTest::addColumn<QMetaType>("targetType");

#define ADD_ROW(typeName, typeNameId, realType)  \
    addRowForPointerValueConversion<realType>(); \
    addRowForPointerValueConversion<realType *>();

    // Add rows for static primitive types
    QT_FOR_EACH_STATIC_PRIMITIVE_NON_VOID_TYPE(ADD_ROW)

    // Add rows for static core types
    QT_FOR_EACH_STATIC_CORE_CLASS(ADD_ROW)
#undef ADD_ROW

}

void tst_QVariant::canConvertAndConvert_ReturnFalse_WhenConvertingBetweenPointerAndValue()
{
    QFETCH(QVariant, variant);
    QFETCH(QMetaType, targetType);

    QVERIFY(!variant.canConvert(targetType));

    QVERIFY(!variant.convert(targetType));

    // As per the documentation, when QVariant::convert fails, the
    // QVariant is cleared and changed to the requested type.
    QVERIFY(variant.isNull());
    QCOMPARE(variant.metaType(), targetType);
}

void tst_QVariant::canConvertAndConvert_ReturnFalse_WhenConvertingQObjectBetweenPointerAndValue()
{
    // Types derived from QObject are non-copyable and require their own test.
    // We only test pointer -> value conversion, because constructing a QVariant
    // from a non-copyable object will just set the QVariant to null.

    QObjectDerived object;
    QVariant variant = QVariant::fromValue(&object);

    constexpr QMetaType targetType = QMetaType::fromType<QObjectDerived>();
    QVERIFY(!variant.canConvert(targetType));

    QTest::ignoreMessage(
            QtWarningMsg,
            QRegularExpression(".*does not support destruction and copy construction"));

    QVERIFY(!variant.convert(targetType));

    // When the QVariant::convert fails, the QVariant is cleared, and since the target type is
    // invalid for QVariant, the QVariant's type is also cleared to an unknown type.
    QVERIFY(variant.isNull());
    QCOMPARE(variant.metaType(), QMetaType());
}

void tst_QVariant::convert()
{
   // verify that after convert(), the variant's type has been changed
   QVariant var = QVariant::fromValue(QString("A string"));
   var.convert(QMetaType::fromType<int>());
   QCOMPARE(var.metaType(), QMetaType::fromType<int>());
   QCOMPARE(var.toInt(), 0);
}

template <typename To, typename From> static void addNumberConversionHelper(From v)
{
    if constexpr (std::is_same_v<From, To>)
        return;

    To to = To(v);
    QVariant var = QVariant::fromValue(v);

    // std::numeric_limits is specialized for qfloat16, std::is_floating_point isn't.
    if constexpr (std::numeric_limits<From>::is_iec559 && std::is_unsigned_v<To>) {
        using STo = std::make_signed_t<To>;
        STo s = STo(v);
        Q_ASSERT(v < 0);
        Q_ASSERT(s < 0);
        QTest::addRow("negative-%s", var.typeName()) << var << To(s) << true;

        v = -v;
        to = To(v);
        var = QVariant::fromValue(v);
    }
    QTest::addRow("%s", var.typeName()) << var << to << true;
}

template <typename To> static void addNumberConversions()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<To>("result");
    QTest::addColumn<bool>("valueOK");

    QTest::newRow("invalid") << QVariant() << To{} << false;

    auto addNumber = [](auto v) { addNumberConversionHelper<To>(v); };
    addNumber(true);
    addNumber(char(1));
    addNumber(qint8(-1));
    addNumber(quint8(2));
    // addNumber(u'A');
    addNumber(short(-256));
    addNumber(ushort(256));
    // addNumber(U'\U00010000');
    addNumber(-65536);
    addNumber(65536U);
    addNumber(-0x8000'0000L);
    addNumber(0x8000'0000UL);
    addNumber(-0x1'0000'0000LL);
    addNumber(0x1'0000'000'0000ULL);
    addNumber(qfloat16(-3.1415927f));
    addNumber(-3.1415927f);
    addNumber(-3.1415927);

    if constexpr (sizeof(To) > sizeof(int)) {
        // note: this includes double
        qint64 value64 = (Q_INT64_C(12) << 35) + 8;
        QTest::newRow("qint64") << QVariant::fromValue(value64) << To(value64) << true;
        QTest::newRow("-qint64") << QVariant::fromValue(-value64) << To(-value64) << true;
        QTest::newRow("LONG_MIN") << QVariant::fromValue(LONG_MIN) << To(LONG_MIN)  << true;
        QTest::newRow("-LONG_MIN/2") << QVariant::fromValue(-(LONG_MIN / 2)) << -To(LONG_MIN/2)  << true;

        if constexpr (std::is_integral_v<To>) {
            QTest::newRow("LONG_MAX") << QVariant::fromValue(LONG_MAX) << To(LONG_MAX)  << true;
            QTest::newRow("ULONG_MAX") << QVariant::fromValue(ULONG_MAX) << To(ULONG_MAX)  << true;
        }
    }

    if constexpr (std::is_integral_v<To>) {
        QTest::newRow("QChar") << QVariant(QChar('a')) << To('a') << true;

        QTest::newRow("NaN") << QVariant::fromValue(qQNaN()) << To(0) << false;

        constexpr qint64 maximal = std::numeric_limits<qint64>::max();
        constexpr qint64 minimal = std::numeric_limits<qint64>::min();

        QTest::newRow("positive overflow") << QVariant(1.0e200) << To(maximal) << true;
        QTest::newRow("positive inf") << QVariant(qInf()) << To(maximal) << true;
        QTest::newRow("negative overflow") << QVariant(-1.0e200) << To(minimal) << true;
        QTest::newRow("negative inf") << QVariant(-qInf()) << To(minimal) << true;
    }

    QTest::newRow("nonint-QByteArray") << QVariant(QByteArray("zzzz")) << To{} << false;
    QTest::newRow("nonint-QString") << QVariant(QString("zzzz")) << To{} << false;
    QTest::newRow("undefined-QCborValue") << QVariant::fromValue<QCborValue>({}) << To{} << false;
    QTest::newRow("undefined-QJsonValue") << QVariant(QJsonValue()) << To{} << false;

    QTest::newRow("int-QByteArray") << QVariant(QByteArray("123")) << To(123) << true;
    QTest::newRow("int-QCborValue") << QVariant::fromValue(QCborValue(321)) << To(321) << true;
    QTest::newRow("int-QJsonValue") << QVariant(QJsonValue(321)) << To(321) << true;
    QTest::newRow("int-QString") << QVariant(QString("123")) << To(123) << true;

    if constexpr (std::numeric_limits<To>::is_iec559) {
        QTest::newRow("fp-QByteArray") << QVariant(QByteArray("32.1")) << To(32.1) << true;
        QTest::newRow("fp-QCborValue") << QVariant::fromValue(QCborValue(32.1)) << To(32.1) << true;
        QTest::newRow("fp-QJsonValue") << QVariant(QJsonValue(32.1)) << To(32.1) << true;
        QTest::newRow("fp-QString") << QVariant(QString("32.1")) << To(32.1) << true;
    }
}

void tst_QVariant::toInt_data()
{
    addNumberConversions<int>();
}

#if QT_DEPRECATED_SINCE(6, 0)
# define EXEC_DEPRECATED_CALL(x) QT_IGNORE_DEPRECATIONS(x)
#else
# define EXEC_DEPRECATED_CALL(x)
#endif

template <typename T> static void checkNumberConversions(T (QVariant:: *toType)(bool *) const = nullptr)
{
    QFETCH( QVariant, value );
    QFETCH( T, result );
    QFETCH( bool, valueOK );

    QMetaType mt = QMetaType::fromType<T>();
    int typeId = mt.id();
    EXEC_DEPRECATED_CALL(QVERIFY(value.isValid() == value.canConvert(typeId));)
    QCOMPARE(value.isValid(), value.canConvert(mt));

    if (toType) {
        // QVariant conversion API
        bool ok;
        T i = (value.*toType)(&ok);
        QCOMPARE(i, result);
        QVERIFY(ok == valueOK);
    }

    if (value.isValid()) {
        // test the conversion API (through QMetaType)
        QCOMPARE(value.convert(mt), valueOK);
        QCOMPARE(qvariant_cast<T>(value), result);
    }
}

void tst_QVariant::toInt()
{
    checkNumberConversions(&QVariant::toInt);
}

void tst_QVariant::toUInt_data()
{
    addNumberConversions<uint>();
}

void tst_QVariant::toUInt()
{
    checkNumberConversions(&QVariant::toUInt);
}

void tst_QVariant::toSize_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QSize>("result");
    QTest::newRow( "qsizef4" ) << QVariant( QSizeF(4, 2) ) << QSize(4, 2);
    QTest::newRow( "qsizef1" ) << QVariant( QSizeF(0, 0) ) << QSize(0, 0);
    QTest::newRow( "qsizef2" ) << QVariant( QSizeF(-5, -1) ) << QSize(-5, -1);
    QTest::newRow( "qsizef3" ) << QVariant( QSizeF() ) << QSize();
}

void tst_QVariant::toSize()
{
    QFETCH( QVariant, value );
    QFETCH( QSize, result );
    QVERIFY( value.isValid() );
    EXEC_DEPRECATED_CALL(QVERIFY( value.canConvert( QVariant::Size ) );)
    QVERIFY( value.canConvert(QMetaType::fromType<QSize>()) );

    QSize i = value.toSize();
    QCOMPARE( i, result );
}

void tst_QVariant::toSizeF_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QSizeF>("result");
    QTest::newRow( "qsize1" ) << QVariant( QSize(0, 0) ) << QSizeF(0, 0);
    QTest::newRow( "qsize2" ) << QVariant( QSize(-5, -1) ) << QSizeF(-5, -1);
     QTest::newRow( "qsize3" ) << QVariant( QSize() ) << QSizeF();
    QTest::newRow( "qsize4" ) << QVariant(QSize(4,2)) << QSizeF(4,2);
}

void tst_QVariant::toSizeF()
{
    QFETCH( QVariant, value );
    QFETCH( QSizeF, result );
    QVERIFY( value.isValid() );
    EXEC_DEPRECATED_CALL(QVERIFY( value.canConvert( QVariant::SizeF ) );)
    QVERIFY( value.canConvert(QMetaType::fromType<QSizeF>()) );

    QSizeF i = value.toSizeF();
    QCOMPARE( i, result );
}

void tst_QVariant::toLine_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QLine>("result");
    QTest::newRow( "linef1" ) << QVariant( QLineF(1, 2, 3, 4) ) << QLine(1, 2, 3, 4);
    QTest::newRow( "linef2" ) << QVariant( QLineF(-1, -2, -3, -4) ) << QLine(-1, -2, -3, -4);
    QTest::newRow( "linef3" ) << QVariant( QLineF(0, 0, 0, 0) ) << QLine(0, 0, 0, 0);
    QTest::newRow( "linef4" ) << QVariant( QLineF() ) << QLine();
}

void tst_QVariant::toLine()
{
    QFETCH( QVariant, value );
    QFETCH( QLine, result );
    QVERIFY( value.isValid() );
    EXEC_DEPRECATED_CALL(QVERIFY( value.canConvert( QVariant::Line ) );)
    QVERIFY( value.canConvert(QMetaType::fromType<QLine>()) );

    QLine i = value.toLine();
    QCOMPARE( i, result );
}

void tst_QVariant::toLineF_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QLineF>("result");
    QTest::newRow( "line1" ) << QVariant( QLine(-1, -2, -3, -4) ) << QLineF(-1, -2, -3, -4);
    QTest::newRow( "line2" ) << QVariant( QLine(0, 0, 0, 0) ) << QLineF(0, 0, 0, 0);
    QTest::newRow( "line3" ) << QVariant( QLine() ) << QLineF();
    QTest::newRow( "line4" ) << QVariant( QLine(1, 2, 3, 4) ) << QLineF(1, 2, 3, 4);
}

void tst_QVariant::toLineF()
{
    QFETCH( QVariant, value );
    QFETCH( QLineF, result );
    QVERIFY( value.isValid() );
    EXEC_DEPRECATED_CALL(QVERIFY( value.canConvert( QVariant::LineF ) );)
    QVERIFY( value.canConvert(QMetaType::fromType<QLineF>()) );

    QLineF i = value.toLineF();
    QCOMPARE( i, result );
}

void tst_QVariant::toPoint_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QPoint>("result");
    QTest::newRow( "pointf1" ) << QVariant( QPointF(4, 2) ) << QPoint(4, 2);
    QTest::newRow( "pointf2" ) << QVariant( QPointF(0, 0) ) << QPoint(0, 0);
    QTest::newRow( "pointf3" ) << QVariant( QPointF(-4, -2) ) << QPoint(-4, -2);
    QTest::newRow( "pointf4" ) << QVariant( QPointF() ) << QPoint();
    QTest::newRow( "pointf5" ) << QVariant( QPointF(-4.2f, -2.3f) ) << QPoint(-4, -2);
}

void tst_QVariant::toPoint()
{
    QFETCH( QVariant, value );
    QFETCH( QPoint, result );
    QVERIFY( value.isValid() );
    EXEC_DEPRECATED_CALL(QVERIFY( value.canConvert( QVariant::Point ) );)
    QVERIFY( value.canConvert(QMetaType::fromType<QPoint>()) );
    QPoint i = value.toPoint();
    QCOMPARE( i, result );
}

void tst_QVariant::toRect_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QRect>("result");
    QTest::newRow( "rectf1" ) << QVariant(QRectF(1, 2, 3, 4)) << QRect(1, 2, 3, 4);
    QTest::newRow( "rectf2" ) << QVariant(QRectF(0, 0, 0, 0)) << QRect(0, 0, 0, 0);
    QTest::newRow( "rectf3" ) << QVariant(QRectF(-1, -2, -3, -4)) << QRect(-1, -2, -3, -4);
    QTest::newRow( "rectf4" ) << QVariant(QRectF(-1.3f, 0, 3.9f, -4.0)) << QRect(-1, 0, 4, -4);
    QTest::newRow( "rectf5" ) << QVariant(QRectF()) << QRect();
}

void tst_QVariant::toRect()
{
    QFETCH( QVariant, value );
    QFETCH( QRect, result );
    QVERIFY( value.isValid() );
    EXEC_DEPRECATED_CALL(QVERIFY( value.canConvert( QVariant::Rect ) );)
    QVERIFY( value.canConvert(QMetaType::fromType<QRect>()) );
    QRect i = value.toRect();
    QCOMPARE( i, result );
}

void tst_QVariant::toChar_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QChar>("result");
    QTest::newRow( "longlong" ) << QVariant(qlonglong('6')) << QChar('6');
    QTest::newRow( "ulonglong" ) << QVariant(qulonglong('7')) << QChar('7');
}

void tst_QVariant::toChar()
{
    QFETCH( QVariant, value );
    QFETCH( QChar, result );
    QVERIFY( value.isValid() );
    EXEC_DEPRECATED_CALL(QVERIFY( value.canConvert( QVariant::Char ) );)
    QVERIFY( value.canConvert(QMetaType::fromType<QChar>()) );

    QChar i = value.toChar();
    QCOMPARE( i, result );
}

void tst_QVariant::toBool_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<bool>("result");

    QTest::newRow( "int0" ) << QVariant( 0 ) << false;
    QTest::newRow( "int1" ) << QVariant( 123 ) << true;
    QTest::newRow( "uint0" ) << QVariant( 0u ) << false;
    QTest::newRow( "uint1" ) << QVariant( 123u ) << true;
    QTest::newRow( "double0" ) << QVariant( 0.0 ) << false;
    QTest::newRow( "float0" ) << QVariant( 0.0f ) << false;
    QTest::newRow( "float16_0" ) << QVariant::fromValue( qfloat16() ) << false;
    QTest::newRow( "double1" ) << QVariant( 3.1415927 ) << true;
    QTest::newRow( "float1" ) << QVariant( 3.1415927f ) << true;
    QTest::newRow( "float16_1" ) << QVariant::fromValue( qfloat16(3.1415927f) ) << true;
    QTest::newRow( "string0" ) << QVariant( QString("3") ) << true;
    QTest::newRow( "string1" ) << QVariant( QString("true") ) << true;
    QTest::newRow( "string2" ) << QVariant( QString("0") ) << false;
    QTest::newRow( "string3" ) << QVariant( QString("fAlSe") ) << false;
    QTest::newRow( "longlong0" ) << QVariant( (qlonglong)0 ) << false;
    QTest::newRow( "longlong1" ) << QVariant( (qlonglong)1 ) << true;
    QTest::newRow( "ulonglong0" ) << QVariant( (qulonglong)0 ) << false;
    QTest::newRow( "ulonglong1" ) << QVariant( (qulonglong)1 ) << true;
    QTest::newRow( "QChar" ) << QVariant(QChar('a')) << true;
    QTest::newRow( "Null_QChar" ) << QVariant(QChar(0)) << false;
    QTest::newRow("QJsonValue(true)") << QVariant(QJsonValue(true)) << true;
    QTest::newRow("QJsonValue(false)") << QVariant(QJsonValue(false)) << false;
    QTest::newRow("QCborValue(true)") << QVariant::fromValue(QCborValue(true)) << true;
    QTest::newRow("QCborValue(false)") << QVariant::fromValue(QCborValue(false)) << false;
}

void tst_QVariant::toBool()
{
    QFETCH( QVariant, value );
    QFETCH( bool, result );
    QVERIFY( value.isValid() );
    EXEC_DEPRECATED_CALL(QVERIFY( value.canConvert( QVariant::Bool ) );)
    QVERIFY( value.canConvert(QMetaType::fromType<bool>()) );

    bool i = value.toBool();
    QCOMPARE( i, result );

    QVERIFY(value.convert(QMetaType::fromType<bool>()));
    QCOMPARE(value.toBool(), result);
}

void tst_QVariant::toPointF_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QPointF>("result");

    QTest::newRow( "QPoint" ) << QVariant( QPointF( 19, 84) ) << QPointF( 19, 84 );
}

void tst_QVariant::toPointF()
{
    QFETCH( QVariant, value );
    QFETCH( QPointF, result );
    QVERIFY( value.isValid() );
    EXEC_DEPRECATED_CALL(QVERIFY( value.canConvert( QVariant::PointF ) );)
    QVERIFY( value.canConvert(QMetaType::fromType<QPointF>()) );
    QPointF d = value.toPointF();
    QCOMPARE( d, result );
}

void tst_QVariant::toRectF_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QRectF>("result");

    QRect r( 1, 9, 8, 4 );
    QRectF rf( 1.0, 9.0, 8.0, 4.0 );
    QTest::newRow( "QRect" ) << QVariant( r ) << rf;
}

void tst_QVariant::toRectF()
{
    QFETCH( QVariant, value );
    QFETCH( QRectF, result );
    QVERIFY( value.isValid() );
    EXEC_DEPRECATED_CALL(QVERIFY( value.canConvert( QVariant::RectF ) );)
    QVERIFY( value.canConvert(QMetaType::fromType<QRectF>()) );
    QRectF d = value.toRectF();
    QCOMPARE( d, result );
}

void tst_QVariant::toDouble_data()
{
    addNumberConversions<double>();
}

void tst_QVariant::toDouble()
{
    checkNumberConversions(&QVariant::toDouble);
}

void tst_QVariant::toFloat_data()
{
    addNumberConversions<float>();
}

void tst_QVariant::toFloat()
{
    checkNumberConversions(&QVariant::toFloat);
}

void tst_QVariant::toFloat16_data()
{
    addNumberConversions<qfloat16>();
}

void tst_QVariant::toFloat16()
{
    checkNumberConversions<qfloat16>(nullptr);
}

void tst_QVariant::toSChar_data()
{
    addNumberConversions<signed char>();
}

void tst_QVariant::toSChar()
{
    checkNumberConversions<signed char>();
}

void tst_QVariant::toUChar_data()
{
    addNumberConversions<uchar>();
}

void tst_QVariant::toUChar()
{
    checkNumberConversions<uchar>();
}

void tst_QVariant::toShort_data()
{
    addNumberConversions<short>();
}

void tst_QVariant::toShort()
{
    checkNumberConversions<short>();
}

void tst_QVariant::toUShort_data()
{
    addNumberConversions<ushort>();
}

void tst_QVariant::toUShort()
{
    checkNumberConversions<ushort>();
}

void tst_QVariant::toLong_data()
{
    addNumberConversions<long>();
}

void tst_QVariant::toLong()
{
    checkNumberConversions<long>();
}

void tst_QVariant::toULong_data()
{
    addNumberConversions<ulong>();
}

void tst_QVariant::toULong()
{
    checkNumberConversions<ulong>();
}

void tst_QVariant::toLongLong_data()
{
    addNumberConversions<qlonglong>();

}

void tst_QVariant::toLongLong()
{
    checkNumberConversions(&QVariant::toLongLong);
}

void tst_QVariant::toULongLong_data()
{
    addNumberConversions<qulonglong>();
}

void tst_QVariant::toULongLong()
{
    checkNumberConversions(&QVariant::toULongLong);
}

void tst_QVariant::toByteArray_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QByteArray>("result");

    QByteArray ba(5, ' ');
    ba[0] = 'T';
    ba[1] = 'e';
    ba[2] = 's';
    ba[3] = 't';
    ba[4] = '\0';

    QByteArray variantBa = ba;

    QTest::newRow( "qbytearray" ) << QVariant( variantBa ) << ba;
    QTest::newRow( "int" ) << QVariant( -123 ) << QByteArray( "-123" );
    QTest::newRow( "uint" ) << QVariant( (uint)123 ) << QByteArray( "123" );
    QTest::newRow( "double" ) << QVariant( 123.456 ) << QByteArray( "123.456" );

    // Conversion from float to double adds bits of which the double-to-string converter doesn't
    // know they're insignificant
    QTest::newRow( "float" ) << QVariant( 123.456f ) << QByteArray( "123.45600128173828" );

    QTest::newRow( "longlong" ) << QVariant( (qlonglong)34 ) << QByteArray( "34" );
    QTest::newRow( "ulonglong" ) << QVariant( (qulonglong)34 ) << QByteArray( "34" );
    QTest::newRow( "nullptr" ) << QVariant::fromValue(nullptr) << QByteArray();
}

void tst_QVariant::toByteArray()
{
    QFETCH( QVariant, value );
    QFETCH( QByteArray, result );
    QVERIFY( value.isValid() );
    EXEC_DEPRECATED_CALL(QVERIFY( value.canConvert( QVariant::ByteArray ) );)
    QVERIFY( value.canConvert(QMetaType::fromType<QByteArray>()) );
    QByteArray ba = value.toByteArray();
    QCOMPARE( ba.isNull(), result.isNull() );
    QCOMPARE( ba, result );

    QVERIFY( value.convert(QMetaType::fromType<QByteArray>()) );
    QCOMPARE( value.toByteArray(), result );
}

void tst_QVariant::toString_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("result");

    QTest::newRow( "qstring" ) << QVariant( QString( "Test" ) ) << QString( "Test" );
    QTest::newRow( "charstar" ) << QVariant(QLatin1String("Test")) << QString("Test");
    QTest::newRow( "qbytearray") << QVariant( QByteArray( "Test\0" ) ) << QString( "Test" );
    QTest::newRow( "int" ) << QVariant( -123 ) << QString( "-123" );
    QTest::newRow( "uint" ) << QVariant( (uint)123 ) << QString( "123" );
    QTest::newRow( "double" ) << QVariant( 123.456 ) << QString( "123.456" );

    // Conversion from float to double adds bits of which the double-to-string converter doesn't
    // know they're insignificant
    QTest::newRow( "float" ) << QVariant( 123.456f ) << QString( "123.45600128173828" );

    QTest::newRow( "bool" ) << QVariant( true ) << QString( "true" );
    QTest::newRow( "qdate" ) << QVariant( QDate( 2002, 1, 1 ) ) << QString( "2002-01-01" );
    QTest::newRow( "qtime" ) << QVariant( QTime( 12, 34, 56 ) ) << QString( "12:34:56.000" );
    QTest::newRow( "qtime-with-ms" ) << QVariant( QTime( 12, 34, 56, 789 ) ) << QString( "12:34:56.789" );
    QTest::newRow( "qdatetime" ) << QVariant( QDateTime( QDate( 2002, 1, 1 ), QTime( 12, 34, 56, 789 ) ) ) << QString( "2002-01-01T12:34:56.789" );
    QTest::newRow( "llong" ) << QVariant( (qlonglong)Q_INT64_C(123456789012) ) <<
        QString( "123456789012" );
    QTest::newRow("QJsonValue") << QVariant(QJsonValue(QString("hello"))) << QString("hello");
    QTest::newRow("QJsonValue(Null)") << QVariant(QJsonValue(QJsonValue::Null)) << QString();
    QTest::newRow("nullptr") << QVariant::fromValue(nullptr) << QString();
}

void tst_QVariant::toString()
{
    QFETCH( QVariant, value );
    QFETCH( QString, result );
    QVERIFY( value.isValid() );
    EXEC_DEPRECATED_CALL(QVERIFY( value.canConvert( QVariant::String ) );)
    QVERIFY( value.canConvert(QMetaType::fromType<QString>()) );
    QString str = value.toString();
    QCOMPARE( str.isNull(), result.isNull() );
    QCOMPARE( str, result );

    QVERIFY( value.convert(QMetaType::fromType<QString>()) );
    QCOMPARE( value.toString(), result );
}

void tst_QVariant::toDate_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QDate>("result");

    QTest::newRow( "qdate" ) << QVariant( QDate( 2002, 10, 10 ) ) << QDate( 2002, 10, 10 );
    QTest::newRow( "qdatetime" ) << QVariant( QDateTime( QDate( 2002, 10, 10 ), QTime( 12, 34, 56 ) ) ) << QDate( 2002, 10, 10 );
    QTest::newRow( "qstring" ) << QVariant( QString( "2002-10-10" ) ) << QDate( 2002, 10, 10 );
}

void tst_QVariant::toDate()
{
    QFETCH( QVariant, value );
    QFETCH( QDate, result );
    QVERIFY( value.isValid() );
    EXEC_DEPRECATED_CALL(QVERIFY( value.canConvert( QVariant::Date ) );)
    QVERIFY( value.canConvert(QMetaType::fromType<QDate>()) );
    QCOMPARE( value.toDate(), result );
}

void tst_QVariant::toTime_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QTime>("result");

    QTest::newRow( "qtime" ) << QVariant( QTime( 12, 34, 56 ) ) << QTime( 12, 34, 56 );
    QTest::newRow( "qdatetime" ) << QVariant( QDateTime( QDate( 2002, 10, 10 ), QTime( 12, 34, 56 ) ) ) << QTime( 12, 34, 56 );
    QTest::newRow( "qstring" ) << QVariant( QString( "12:34:56" ) ) << QTime( 12, 34, 56 );
    QTest::newRow( "qstring-with-ms" ) << QVariant( QString( "12:34:56.789" ) ) << QTime( 12, 34, 56, 789 );
}

void tst_QVariant::toTime()
{
    QFETCH( QVariant, value );
    QFETCH( QTime, result );
    QVERIFY( value.isValid() );
    EXEC_DEPRECATED_CALL(QVERIFY( value.canConvert( QVariant::Time ) );)
    QVERIFY( value.canConvert(QMetaType::fromType<QTime>()) );
    QCOMPARE( value.toTime(), result );
}

void tst_QVariant::toDateTime_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QDateTime>("result");

    QTest::newRow( "qdatetime" ) << QVariant( QDateTime( QDate( 2002, 10, 10 ), QTime( 12, 34, 56 ) ) )
        << QDateTime( QDate( 2002, 10, 10 ), QTime( 12, 34, 56 ) );
    QTest::newRow( "qdate" ) << QVariant( QDate( 2002, 10, 10 ) ) << QDateTime( QDate( 2002, 10, 10 ), QTime( 0, 0, 0 ) );
    QTest::newRow( "qstring" ) << QVariant( QString( "2002-10-10T12:34:56" ) ) << QDateTime( QDate( 2002, 10, 10 ), QTime( 12, 34, 56 ) );
    QTest::newRow("qstring-utc")
        << QVariant(QString("2002-10-10T12:34:56Z"))
        << QDateTime(QDate(2002, 10, 10), QTime(12, 34, 56), QTimeZone::UTC);
    QTest::newRow( "qstring-with-ms" ) << QVariant( QString( "2002-10-10T12:34:56.789" ) )
                                       << QDateTime( QDate( 2002, 10, 10 ), QTime( 12, 34, 56, 789 ) );
}

void tst_QVariant::toDateTime()
{
    QFETCH( QVariant, value );
    QFETCH( QDateTime, result );
    QVERIFY( value.isValid() );
    EXEC_DEPRECATED_CALL(QVERIFY( value.canConvert( QVariant::DateTime ) );)
    QVERIFY( value.canConvert(QMetaType::fromType<QDateTime>()) );
    QCOMPARE( value.toDateTime(), result );
}

#undef EXEC_DEPRECATED_CALL

void tst_QVariant::toLocale()
{
    QVariant variant;
    QLocale loc = variant.toLocale();
    variant = QLocale::system();
    loc = variant.toLocale();
}

void tst_QVariant::toRegularExpression()
{
    QVariant variant;
    QRegularExpression re = variant.toRegularExpression();
    QCOMPARE(re, QRegularExpression());

    variant = QRegularExpression("abc.*def");
    re = variant.toRegularExpression();
    QCOMPARE(re, QRegularExpression("abc.*def"));

    variant = QVariant::fromValue(QRegularExpression("[ab]\\w+"));
    re = variant.value<QRegularExpression>();
    QCOMPARE(re, QRegularExpression("[ab]\\w+"));
}

struct CustomStreamableClass
{
    int i;
    bool operator==(const CustomStreamableClass& other) const
    {
        return i == other.i;
    }
};

QDataStream &operator<<(QDataStream &out, const CustomStreamableClass &myObj)
{
    return out << myObj.i;
}

QDataStream &operator>>(QDataStream &in, CustomStreamableClass &myObj)
{
    return in >> myObj.i;
}
Q_DECLARE_METATYPE(CustomStreamableClass);

void tst_QVariant::writeToReadFromDataStream_data()
{
    QTest::addColumn<QVariant>("writeVariant");
    QTest::addColumn<bool>("isNull");
    {
        QVariantList valuelist;
        valuelist << QVariant( 1 ) << QVariant( QString("Two") ) << QVariant( 3.45 );
        QVariant var(valuelist);
        QTest::newRow( "list_valid" ) << var << false;
    }

    QTest::newRow( "invalid" ) << QVariant() << true;
    QTest::newRow( "bitarray_invalid" ) << QVariant(QMetaType::fromType<QBitArray>()) << true;
    QTest::newRow( "bitarray_empty" ) << QVariant( QBitArray() ) << false;
    QBitArray bitarray( 3 );
    bitarray[0] = 0;
    bitarray[1] = 1;
    bitarray[2] = 0;
    QTest::newRow( "bitarray_valid" ) << QVariant( bitarray ) << false;
    QTest::newRow( "bytearray_invalid" ) << QVariant(QMetaType::fromType<QByteArray>()) << true;
    QTest::newRow( "bytearray_empty" ) << QVariant( QByteArray() ) << false;
    QTest::newRow( "int_invalid") << QVariant(QMetaType::fromType<int>()) << true;
    QByteArray bytearray(5, ' ');
    bytearray[0] = 'T';
    bytearray[1] = 'e';
    bytearray[2] = 's';
    bytearray[3] = 't';
    bytearray[4] = '\0';
    QTest::newRow( "bytearray_valid" ) << QVariant( bytearray ) << false;
    QTest::newRow( "date_invalid" ) << QVariant(QMetaType::fromType<QDate>()) << true;
    QTest::newRow( "date_empty" ) << QVariant( QDate() ) << false;
    QTest::newRow( "date_valid" ) << QVariant( QDate( 2002, 07, 06 ) ) << false;
    QTest::newRow( "datetime_invalid" ) << QVariant(QMetaType::fromType<QDateTime>()) << true;
    QTest::newRow( "datetime_empty" ) << QVariant( QDateTime() ) << false;
    QTest::newRow( "datetime_valid" ) << QVariant( QDateTime( QDate( 2002, 07, 06 ), QTime( 14, 0, 0 ) ) ) << false;
    QTest::newRow( "double_valid" ) << QVariant( 123.456 ) << false;
    QTest::newRow( "float_valid" ) << QVariant( 123.456f ) << false;
    QTest::newRow( "int_valid" ) << QVariant( -123 ) << false;
    QVariantMap vMap;
    vMap.insert( "int", QVariant( 1 ) );
    vMap.insert( "string", QVariant( QString("Two") ) );
    vMap.insert( "double", QVariant( 3.45 ) );
    vMap.insert( "float", QVariant( 3.45f ) );
    QTest::newRow( "map_valid" ) << QVariant( vMap ) << false;
    QTest::newRow( "point_invalid" ) << QVariant(QMetaType::fromType<QPoint>()) << true;
    QTest::newRow( "point_empty" ) << QVariant::fromValue( QPoint() ) << false;
    QTest::newRow( "point_valid" ) << QVariant::fromValue( QPoint( 10, 10 ) ) << false;
    QTest::newRow( "rect_invalid" ) << QVariant(QMetaType::fromType<QRect>()) << true;
    QTest::newRow( "rect_empty" ) << QVariant( QRect() ) << false;
    QTest::newRow( "rect_valid" ) << QVariant( QRect( 10, 10, 20, 20 ) ) << false;
    QTest::newRow( "size_invalid" ) << QVariant(QMetaType::fromType<QSize>()) << true;
    QTest::newRow( "size_empty" ) << QVariant( QSize( 0, 0 ) ) << false;
    QTest::newRow( "size_valid" ) << QVariant( QSize( 10, 10 ) ) << false;
    QTest::newRow( "string_invalid" ) << QVariant(QMetaType::fromType<QString>()) << true;
    QTest::newRow( "string_empty" ) << QVariant( QString() ) << false;
    QTest::newRow( "string_valid" ) << QVariant( QString( "Test" ) ) << false;
    QStringList stringlist;
    stringlist << "One" << "Two" << "Three";
    QTest::newRow( "stringlist_valid" ) << QVariant( stringlist ) << false;
    QTest::newRow( "time_invalid" ) << QVariant(QMetaType::fromType<QTime>()) << true;
    QTest::newRow( "time_empty" ) << QVariant( QTime() ) << false;
    QTest::newRow( "time_valid" ) << QVariant( QTime( 14, 0, 0 ) ) << false;
    QTest::newRow( "uint_valid" ) << QVariant( (uint)123 ) << false;
    QTest::newRow( "qchar" ) << QVariant(QChar('a')) << false;
    QTest::newRow( "qchar_null" ) << QVariant(QChar(0)) << false;
    QTest::newRow( "regularexpression" ) << QVariant(QRegularExpression("abc.*def")) << false;
    QTest::newRow( "regularexpression_empty" ) << QVariant(QRegularExpression()) << false;

    // types known to QMetaType, but not part of QVariant::Type
    QTest::newRow("QMetaType::Long invalid") << QVariant(QMetaType::fromType<long>(), nullptr) << true;
    long longInt = -1l;
    QTest::newRow("QMetaType::Long") << QVariant(QMetaType::fromType<long>(), &longInt) << false;
    QTest::newRow("QMetaType::Short invalid") << QVariant(QMetaType::fromType<short>(), nullptr) << true;
    short shortInt = 1;
    QTest::newRow("QMetaType::Short") << QVariant(QMetaType::fromType<short>(), &shortInt) << false;
    QTest::newRow("QMetaType::Char invalid") << QVariant(QMetaType::fromType<QChar>(), nullptr) << true;
    char ch = 'c';
    QTest::newRow("QMetaType::Char") << QVariant(QMetaType::fromType<char>(), &ch) << false;
    QTest::newRow("QMetaType::ULong invalid") << QVariant(QMetaType::fromType<ulong>(), nullptr) << true;
    ulong ulongInt = 1ul;
    QTest::newRow("QMetaType::ULong") << QVariant(QMetaType::fromType<ulong>(), &ulongInt) << false;
    QTest::newRow("QMetaType::UShort invalid") << QVariant(QMetaType::fromType<ushort>(), nullptr) << true;
    ushort ushortInt = 1u;
    QTest::newRow("QMetaType::UShort") << QVariant(QMetaType::fromType<ushort>(), &ushortInt) << false;
    QTest::newRow("QMetaType::UChar invalid") << QVariant(QMetaType::fromType<uchar>(), nullptr) << true;
    uchar uch = 0xf0;
    QTest::newRow("QMetaType::UChar") << QVariant(QMetaType::fromType<uchar>(), &uch) << false;
    QTest::newRow("QMetaType::Float invalid") << QVariant(QMetaType::fromType<float>(), nullptr) << true;
    float f = 1.234f;
    QTest::newRow("QMetaType::Float") << QVariant(QMetaType::fromType<float>(), &f) << false;
    CustomStreamableClass custom = {123};
    QTest::newRow("Custom type") << QVariant::fromValue(custom) << false;
}

void tst_QVariant::writeToReadFromDataStream()
{
    QFETCH( QVariant, writeVariant );
    QFETCH( bool, isNull );
    QByteArray data;

    QDataStream writeStream( &data, QIODevice::WriteOnly );
    writeStream << writeVariant;

    QVariant readVariant;
    QDataStream readStream( &data, QIODevice::ReadOnly );
    readStream >> readVariant;
    QVERIFY( readVariant.isNull() == isNull );
    // Best way to confirm the readVariant contains the same data?
    // Since only a few won't match since the serial numbers are different
    // I won't bother adding another bool in the data test.
    const int writeType = writeVariant.userType();
    if (writeType == qMetaTypeId<CustomStreamableClass>()) {
        QCOMPARE(qvariant_cast<CustomStreamableClass>(readVariant),
                 qvariant_cast<CustomStreamableClass>(writeVariant));
    } else if ( writeType != QMetaType::UnknownType && writeType != QMetaType::QBitmap
                && writeType != QMetaType::QPixmap && writeType != QMetaType::QImage) {
        switch (writeType) {
        default:
            QCOMPARE( readVariant, writeVariant );
            break;

        // compare types know by QMetaType but not QVariant (QVariant::operator==() knows nothing about them)
        case QMetaType::Long:
            QCOMPARE(qvariant_cast<long>(readVariant), qvariant_cast<long>(writeVariant));
            break;
        case QMetaType::ULong:
            QCOMPARE(qvariant_cast<ulong>(readVariant), qvariant_cast<ulong>(writeVariant));
            break;
        case QMetaType::Short:
            QCOMPARE(qvariant_cast<short>(readVariant), qvariant_cast<short>(writeVariant));
            break;
        case QMetaType::UShort:
            QCOMPARE(qvariant_cast<ushort>(readVariant), qvariant_cast<ushort>(writeVariant));
            break;
        case QMetaType::Char:
            QCOMPARE(qvariant_cast<char>(readVariant), qvariant_cast<char>(writeVariant));
            break;
        case QMetaType::UChar:
            QCOMPARE(qvariant_cast<uchar>(readVariant), qvariant_cast<uchar>(writeVariant));
            break;
        case QMetaType::Float:
            {
                // the uninitialized float can be NaN (observed on Windows Mobile 5 ARMv4i)
                float readFloat = qvariant_cast<float>(readVariant);
                float writtenFloat = qvariant_cast<float>(writeVariant);
                QCOMPARE(qIsNaN(readFloat), qIsNaN(writtenFloat));
                if (!qIsNaN(readFloat))
                    QCOMPARE(readFloat, writtenFloat);
            }
            break;
        }
    }
}

void tst_QVariant::writeToReadFromOldDataStream()
{
    QVariant writeVariant = QString("hello");
    QByteArray data;

    QDataStream writeStream(&data, QIODevice::WriteOnly);
    writeStream.setVersion(QDataStream::Qt_2_1);
    writeStream << writeVariant;

    QVariant readVariant;
    QDataStream readStream(&data, QIODevice::ReadOnly);
    readStream.setVersion(QDataStream::Qt_2_1);
    readStream >> readVariant;

    QCOMPARE(writeVariant.userType(), readVariant.userType());
    QCOMPARE(writeVariant, readVariant);
}

void tst_QVariant::checkDataStream()
{
    const int typeId = QMetaType::LastCoreType + 1;
    QVERIFY(!QMetaType::isRegistered(typeId));

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Trying to construct an instance of an invalid type"));
    QByteArray settingsHex("000000");
    settingsHex.append(QByteArray::number(typeId, 16));
    settingsHex.append("ffffffffff");
    const QByteArray settings = QByteArray::fromHex(settingsHex);
    QDataStream in(settings);
    QVariant v;
    in >> v;
    // the line below has been left out for now since the data stream
    // is not necessarily considered corrupt when an invalid QVariant is
    // constructed. However, it might be worth considering changing that behavior
    // in the future.
//    QCOMPARE(in.status(), QDataStream::ReadCorruptData);
    QCOMPARE(v.typeId(), QMetaType::UnknownType);
}

void tst_QVariant::operator_eq_eq_data()
{
    QTest::addColumn<QVariant>("left");
    QTest::addColumn<QVariant>("right");
    QTest::addColumn<bool>("equal"); // left == right ?

    QVariant inv;
    QVariant i0( int(0) );
    QVariant i1( int(1) );
    // Invalid
    QTest::newRow( "invinv" ) << inv << inv << true;
    // Int
    QTest::newRow( "int1int1" ) << i1 << i1 << true;
    QTest::newRow( "int1int0" ) << i1 << i0 << false;
    QTest::newRow( "nullint" ) << i0 << QVariant(QMetaType::fromType<int>()) << true;

    // LongLong and ULongLong
    QVariant ll1( (qlonglong)1 );
    QVariant lln2( (qlonglong)-2 );
    QVariant ull1( (qulonglong)1 );
    QVariant ull3( (qulonglong)3 );
    QTest::newRow( "ll1ll1" ) << ll1 << ll1 << true;
    QTest::newRow( "ll1lln2" ) << ll1 << lln2 << false;
    QTest::newRow( "ll1ull1" ) << ull1 << ull1 << true;
    QTest::newRow( "ll1i1" ) << ull1 << i1 << true;
    QTest::newRow( "ull1ull1" ) << ull1 << ull1 << true;
    QTest::newRow( "ull1i1" ) << ull1 << ull1 << true;

    QVariant mInt(-42);
    QVariant mIntString(QByteArray("-42"));
    QVariant mIntQString(QString("-42"));

    QVariant mIntZero(0);
    QVariant mIntStringZero(QByteArray("0"));
    QVariant mIntQStringZero(QString("0"));

    QVariant mUInt(42u);
    QVariant mUIntString(QByteArray("42"));
    QVariant mUIntQString(QString("42"));

    QVariant mDouble(42.11);
#ifdef QT_NO_DOUBLECONVERSION
    // Without libdouble-conversion we don't get the shortest possible representation.
    QVariant mDoubleString(QByteArray("42.109999999999999"));
    QVariant mDoubleQString(QByteArray("42.109999999999999"));
#else
    // You cannot fool the double-to-string conversion into producing insignificant digits with
    // libdouble-conversion. You can, of course, add insignificant digits to the string and fool
    // the double-to-double comparison after converting the string to a double.
    QVariant mDoubleString(QByteArray("42.11"));
    QVariant mDoubleQString(QString("42.11"));
#endif

    // Float-to-double conversion produces insignificant extra bits.
    QVariant mFloat(42.11f);
#ifdef QT_NO_DOUBLECONVERSION
    // The trailing '2' is not significant, but snprintf doesn't know this.
    QVariant mFloatString(QByteArray("42.110000610351562"));
    QVariant mFloatQString(QString("42.110000610351562"));
#else
    QVariant mFloatString(QByteArray("42.11000061035156"));
    QVariant mFloatQString(QString("42.11000061035156"));
#endif

    QVariant mLongLong((qlonglong)-42);
    QVariant mLongLongString(QByteArray("-42"));
    QVariant mLongLongQString(QString("-42"));

    QVariant mULongLong((qulonglong)42);
    QVariant mULongLongString(QByteArray("42"));
    QVariant mULongLongQString(QString("42"));

    QVariant mBool(false);
    QVariant mBoolString(QByteArray("false"));
    QVariant mBoolQString(QString("false"));

    QVariant mTextString(QByteArray("foobar"));
    QVariant mTextQString(QString("foobar"));

    QTest::newRow( "double_int" ) << QVariant(42.0) << QVariant(42) << true;
    QTest::newRow( "float_int" ) << QVariant(42.f) << QVariant(42) << true;
    QTest::newRow( "mInt_mIntString" ) << mInt << mIntString << false;
    QTest::newRow( "mIntString_mInt" ) << mIntString << mInt << false;
    QTest::newRow( "mInt_mIntQString" ) << mInt << mIntQString << true;
    QTest::newRow( "mIntQString_mInt" ) << mIntQString << mInt << true;

    QTest::newRow( "mIntZero_mIntStringZero" ) << mIntZero << mIntStringZero << false;
    QTest::newRow( "mIntStringZero_mIntZero" ) << mIntStringZero << mIntZero << false;
    QTest::newRow( "mIntZero_mIntQStringZero" ) << mIntZero << mIntQStringZero << true;
    QTest::newRow( "mIntQStringZero_mIntZero" ) << mIntQStringZero << mIntZero << true;

    QTest::newRow( "mInt_mTextString" ) << mInt << mTextString << false;
    QTest::newRow( "mTextString_mInt" ) << mTextString << mInt << false;
    QTest::newRow( "mInt_mTextQString" ) << mInt << mTextQString << false;
    QTest::newRow( "mTextQString_mInt" ) << mTextQString << mInt << false;

    QTest::newRow( "mIntZero_mTextString" ) << mIntZero << mTextString << false;
    QTest::newRow( "mTextString_mIntZero" ) << mTextString << mIntZero << false;
    QTest::newRow( "mIntZero_mTextQString" ) << mIntZero << mTextQString << false;
    QTest::newRow( "mTextQString_mIntZero" ) << mTextQString << mIntZero << false;

    QTest::newRow( "mUInt_mUIntString" ) << mUInt << mUIntString << false;
    QTest::newRow( "mUIntString_mUInt" ) << mUIntString << mUInt << false;
    QTest::newRow( "mUInt_mUIntQString" ) << mUInt << mUIntQString << true;
    QTest::newRow( "mUIntQString_mUInt" ) << mUIntQString << mUInt << true;

    QTest::newRow( "mDouble_mDoubleString" ) << mDouble << mDoubleString << false;
    QTest::newRow( "mDoubleString_mDouble" ) << mDoubleString << mDouble << false;
    QTest::newRow( "mDouble_mDoubleQString" ) << mDouble << mDoubleQString << true;
    QTest::newRow( "mDoubleQString_mDouble" ) << mDoubleQString << mDouble << true;

    QTest::newRow( "mDouble_mTextString" ) << mDouble << mTextString << false;
    QTest::newRow( "mTextString_mDouble" ) << mTextString << mDouble << false;
    QTest::newRow( "mDouble_mTextQString" ) << mDouble << mTextQString << false;
    QTest::newRow( "mTextQString_mDouble" ) << mTextQString << mDouble << false;

    QTest::newRow( "mFloat_mFloatString" ) << mFloat << mFloatString << false;
    QTest::newRow( "mFloatString_mFloat" ) << mFloatString << mFloat << false;
    QTest::newRow( "mFloat_mFloatQString" ) << mFloat << mFloatQString << true;
    QTest::newRow( "mFloatQString_mFloat" ) << mFloatQString << mFloat << true;

    QTest::newRow( "mLongLong_mLongLongString" ) << mLongLong << mLongLongString << false;
    QTest::newRow( "mLongLongString_mLongLong" ) << mLongLongString << mLongLong << false;
    QTest::newRow( "mLongLong_mLongLongQString" ) << mLongLong << mLongLongQString << true;
    QTest::newRow( "mLongLongQString_mLongLong" ) << mLongLongQString << mLongLong << true;

    QTest::newRow( "mULongLong_mULongLongString" ) << mULongLong << mULongLongString << false;
    QTest::newRow( "mULongLongString_mULongLong" ) << mULongLongString << mULongLong << false;
    QTest::newRow( "mULongLong_mULongLongQString" ) << mULongLong << mULongLongQString << true;
    QTest::newRow( "mULongLongQString_mULongLong" ) << mULongLongQString << mULongLong << true;

    QTest::newRow( "mBool_mBoolString" ) << mBool << mBoolString << false;
    QTest::newRow( "mBoolString_mBool" ) << mBoolString << mBool << false;
    QTest::newRow( "mBool_mBoolQString" ) << mBool << mBoolQString << true;
    QTest::newRow( "mBoolQString_mBool" ) << mBoolQString << mBool << true;

    QTest::newRow("ba2qstring") << QVariant(QLatin1String("hallo")) << QVariant(QString("hallo")) << true;
    QTest::newRow("qstring2ba") << QVariant(QString("hallo")) << QVariant(QLatin1String("hallo")) << true;
    QTest::newRow("char_char") << QVariant(QChar('a')) << QVariant(QChar('a')) << true;
    QTest::newRow("char_char2") << QVariant(QChar('a')) << QVariant(QChar('b')) << false;

    QTest::newRow("invalidConversion") << QVariant(QString("bubu")) << QVariant() << false;
    QTest::newRow("invalidConversionR") << QVariant() << QVariant(QString("bubu")) << false;
    // ### many other combinations missing

    {
        QUuid uuid(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
        QTest::newRow("uuidstring") << QVariant(uuid) << QVariant(uuid.toString()) << false;
        QTest::newRow("stringuuid") << QVariant(uuid.toString()) << QVariant(uuid) << false;
        QTest::newRow("uuidbytearray") << QVariant(uuid) << QVariant(uuid.toByteArray()) << false;
        QTest::newRow("bytearrayuuid") << QVariant(uuid.toByteArray()) << QVariant(uuid) << false;
    }

    {
        QMap<QString, QVariant> map1;
        map1.insert( "X", 1 );

        QMap<QString, QVariant> map2;
        map2.insert( "Y", 1 );

        QTest::newRow("TwoItemsInEqual") << QVariant(map1) << QVariant(map2) << false;

    }

    {
        QMap<QString, QVariant> map1;
        map1.insert( "X", 1 );

        QMap<QString, QVariant> map2;
        map2.insert( "X", 1 );

        QTest::newRow("TwoItemsEqual") << QVariant(map1) << QVariant(map2) << true;
    }

    {
        QMap<QString, QVariant> map1;
        map1.insert( "X", 1 );

        QMap<QString, QVariant> map2;

        QTest::newRow("PopulatedEmptyMap") << QVariant(map1) << QVariant(map2) << false;
    }

    {
        QMap<QString, QVariant> map1;

        QMap<QString, QVariant> map2;
        map2.insert( "X", 1 );

        QTest::newRow("EmptyPopulatedMap") << QVariant(map1) << QVariant(map2) << false;
    }

    {
        QMap<QString, QVariant> map1;
        map1.insert( "X", 1 );
        map1.insert( "Y", 1 );

        QMap<QString, QVariant> map2;
        map2.insert( "X", 1 );

        QTest::newRow("FirstLarger") << QVariant(map1) << QVariant(map2) << false;
    }

    {
        QMap<QString, QVariant> map1;
        map1.insert( "X", 1 );

        QMap<QString, QVariant> map2;
        map2.insert( "X", 1 );
        map2.insert( "Y", 1 );

        QTest::newRow("SecondLarger") << QVariant(map1) << QVariant(map2) << false;
    }

    // same thing with hash
    {
        QHash<QString, QVariant> hash1;
        hash1.insert( "X", 1 );

        QHash<QString, QVariant> hash2;
        hash2.insert( "Y", 1 );

        QTest::newRow("HashTwoItemsInEqual") << QVariant(hash1) << QVariant(hash2) << false;

    }

    {
        QHash<QString, QVariant> hash1;
        hash1.insert( "X", 1 );

        QHash<QString, QVariant> hash2;
        hash2.insert( "X", 1 );

        QTest::newRow("HashTwoItemsEqual") << QVariant(hash1) << QVariant(hash2) << true;
    }

    {
        QHash<QString, QVariant> hash1;
        hash1.insert( "X", 1 );

        QHash<QString, QVariant> hash2;

        QTest::newRow("HashPopulatedEmptyHash") << QVariant(hash1) << QVariant(hash2) << false;
    }

    {
        QHash<QString, QVariant> hash1;

        QHash<QString, QVariant> hash2;
        hash2.insert( "X", 1 );

        QTest::newRow("EmptyPopulatedHash") << QVariant(hash1) << QVariant(hash2) << false;
    }

    {
        QHash<QString, QVariant> hash1;
        hash1.insert( "X", 1 );
        hash1.insert( "Y", 1 );

        QHash<QString, QVariant> hash2;
        hash2.insert( "X", 1 );

        QTest::newRow("HashFirstLarger") << QVariant(hash1) << QVariant(hash2) << false;
    }

    {
        QHash<QString, QVariant> hash1;
        hash1.insert( "X", 1 );

        QHash<QString, QVariant> hash2;
        hash2.insert( "X", 1 );
        hash2.insert( "Y", 1 );

        QTest::newRow("HashSecondLarger") << QVariant(hash1) << QVariant(hash2) << false;
    }
}

void tst_QVariant::operator_eq_eq()
{
    QFETCH( QVariant, left );
    QFETCH( QVariant, right );
    QFETCH( bool, equal );
    QT_TEST_EQUALITY_OPS(left, right, equal);
}

#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
void tst_QVariant::typeName_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<QByteArray>("res");
    QTest::newRow("0") << int(QVariant::Invalid) << QByteArray("");
    QTest::newRow("1") << int(QVariant::Map) << QByteArray("QVariantMap");
    QTest::newRow("2") << int(QVariant::List) << QByteArray("QVariantList");
    QTest::newRow("3") << int(QVariant::String) << QByteArray("QString");
    QTest::newRow("4") << int(QVariant::StringList) << QByteArray("QStringList");
    QTest::newRow("5") << int(QVariant::Font) << QByteArray("QFont");
    QTest::newRow("6") << int(QVariant::Pixmap) << QByteArray("QPixmap");
    QTest::newRow("7") << int(QVariant::Brush) << QByteArray("QBrush");
    QTest::newRow("8") << int(QVariant::Rect) << QByteArray("QRect");
    QTest::newRow("9") << int(QVariant::Size) << QByteArray("QSize");
    QTest::newRow("10") << int(QVariant::Color) << QByteArray("QColor");
    QTest::newRow("11") << int(QVariant::Palette) << QByteArray("QPalette");
    QTest::newRow("12") << int(QVariant::Point) << QByteArray("QPoint");
    QTest::newRow("13") << int(QVariant::Image) << QByteArray("QImage");
    QTest::newRow("14") << int(QVariant::Int) << QByteArray("int");
    QTest::newRow("15") << int(QVariant::UInt) << QByteArray("uint");
    QTest::newRow("16") << int(QVariant::Bool) << QByteArray("bool");
    QTest::newRow("17") << int(QVariant::Double) << QByteArray("double");
    QTest::newRow("18") << int(QMetaType::Float) << QByteArray("float");
    QTest::newRow("19") << int(QVariant::Polygon) << QByteArray("QPolygon");
    QTest::newRow("20") << int(QVariant::Region) << QByteArray("QRegion");
    QTest::newRow("21") << int(QVariant::Bitmap) << QByteArray("QBitmap");
    QTest::newRow("22") << int(QVariant::Cursor) << QByteArray("QCursor");
    // The test below doesn't work as long as we don't link against widgets
//    QTest::newRow("23") << int(QVariant::SizePolicy) << QByteArray("QSizePolicy");
    QTest::newRow("24") << int(QVariant::Date) << QByteArray("QDate");
    QTest::newRow("25") << int(QVariant::Time) << QByteArray("QTime");
    QTest::newRow("26") << int(QVariant::DateTime) << QByteArray("QDateTime");
    QTest::newRow("27") << int(QVariant::ByteArray) << QByteArray("QByteArray");
    QTest::newRow("28") << int(QVariant::BitArray) << QByteArray("QBitArray");
    QTest::newRow("29") << int(QVariant::KeySequence) << QByteArray("QKeySequence");
    QTest::newRow("30") << int(QVariant::Pen) << QByteArray("QPen");
    QTest::newRow("31") << int(QVariant::LongLong) << QByteArray("qlonglong");
    QTest::newRow("32") << int(QVariant::ULongLong) << QByteArray("qulonglong");
    QTest::newRow("33") << int(QVariant::Char) << QByteArray("QChar");
    QTest::newRow("34") << int(QVariant::Url) << QByteArray("QUrl");
    QTest::newRow("35") << int(QVariant::TextLength) << QByteArray("QTextLength");
    QTest::newRow("36") << int(QVariant::TextFormat) << QByteArray("QTextFormat");
    QTest::newRow("37") << int(QVariant::Locale) << QByteArray("QLocale");
    QTest::newRow("38") << int(QVariant::LineF) << QByteArray("QLineF");
    QTest::newRow("39") << int(QVariant::RectF) << QByteArray("QRectF");
    QTest::newRow("40") << int(QVariant::PointF) << QByteArray("QPointF");
    QTest::newRow("44") << int(QVariant::Transform) << QByteArray("QTransform");
    QTest::newRow("45") << int(QVariant::Hash) << QByteArray("QVariantHash");
    QTest::newRow("46") << int(QVariant::Matrix4x4) << QByteArray("QMatrix4x4");
    QTest::newRow("47") << int(QVariant::Vector2D) << QByteArray("QVector2D");
    QTest::newRow("48") << int(QVariant::Vector3D) << QByteArray("QVector3D");
    QTest::newRow("49") << int(QVariant::Vector4D) << QByteArray("QVector4D");
    QTest::newRow("50") << int(QVariant::Quaternion) << QByteArray("QQuaternion");
    QTest::newRow("51") << int(QVariant::RegularExpression) << QByteArray("QRegularExpression");
}

void tst_QVariant::typeName()
{
    QFETCH( int, type );
    QFETCH( QByteArray, res );
    QCOMPARE(QString::fromLatin1(QVariant::typeToName((QVariant::Type)type)),
            QString::fromLatin1(res.constData()));
}

// test nameToType as well
void tst_QVariant::typeToName()
{
    QVariant v;
    QCOMPARE( QVariant::typeToName( v.type() ), (const char*)0 ); // Invalid
    // assumes that QVariant::Type contains consecutive values

    int max = QVariant::LastGuiType;
    for (int t = 1; t <= max; ++t) {
        if (!QMetaType::isRegistered(t)) {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
                                     "^Trying to construct an instance of an invalid type"));
        }
        const char *n = QVariant::typeToName( (QVariant::Type)t );
        if (n)
            QCOMPARE( int(QVariant::nameToType( n )), t );
    }

    QCOMPARE(QVariant::typeToName(QVariant::Int), "int");
    // not documented but we return 0 if the type is out of range
    // by testing this we catch cases where QVariant is extended
    // but type_map is not updated accordingly
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
                             "^Trying to construct an instance of an invalid type"));
    QCOMPARE(QVariant::typeToName(QVariant::Type(max + 1)), (const char *)nullptr);
    // invalid type names
    QVERIFY( QVariant::nameToType( 0 ) == QVariant::Invalid );
    QVERIFY( QVariant::nameToType( "" ) == QVariant::Invalid );
    QVERIFY( QVariant::nameToType( "foo" ) == QVariant::Invalid );

    QCOMPARE(QVariant::nameToType("UserType"), QVariant::Invalid);

    // We don't support these old (Qt3) types anymore.
    QCOMPARE(QVariant::nameToType("QIconSet"), QVariant::Invalid);
    QCOMPARE(QVariant::nameToType("Q3CString"), QVariant::Invalid);
    QCOMPARE(QVariant::nameToType("Q_LLONG"), QVariant::Invalid);
    QCOMPARE(QVariant::nameToType("Q_ULLONG"), QVariant::Invalid);
}
QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 0)

void tst_QVariant::streamInvalidVariant()
{
    int writeX = 1;
    int writeY = 2;
    int readX;
    int readY;
    QVariant writeVariant;
    QVariant readVariant;

    QVERIFY( writeVariant.typeId() == QMetaType::UnknownType );

    QByteArray data;
    QDataStream writeStream( &data, QIODevice::WriteOnly );
    writeStream << writeX << writeVariant << writeY;

    QDataStream readStream( &data, QIODevice::ReadOnly );
    readStream >> readX >> readVariant >> readY;

    QVERIFY( readX == writeX );
    // Two invalid QVariant's aren't necessarily the same, so == will
    // return false if one is invalid, so check the type() instead
    QVERIFY( readVariant.typeId() == QMetaType::UnknownType );
    QVERIFY( readY == writeY );
}

static int instanceCount = 0;

struct MyType
{
    MyType(int n = 0, const char *t=0): number(n), text(t)
    {
        ++instanceCount;
    }
    MyType(const MyType &other)
        : number(other.number), text(other.text)
    {
        ++instanceCount;
    }
    MyType &operator=(const MyType &other)
    {
        number = other.number;
        text = other.text;
        return *this;
    }
    ~MyType()
    {
        --instanceCount;
    }
    int number;
    const char *text;
};
bool operator==(const MyType &a, const MyType &b) { return a.number == b.number && a.text == b.text; }
static_assert(QTypeTraits::has_operator_equal_v<MyType>);

Q_DECLARE_METATYPE(MyType)
Q_DECLARE_METATYPE(MyType*)

void tst_QVariant::userType()
{
    {
        MyType data(1, "eins");
        MyType data2(2, "zwei");

        {
            QVariant userVar;
            userVar.setValue(data);

            QVERIFY(QMetaType::fromName("MyType").isValid());
            QCOMPARE(QMetaType::fromName("MyType"), QMetaType::fromType<MyType>());
            QVERIFY(userVar.typeId() > QMetaType::User);
            QCOMPARE(userVar.userType(), qMetaTypeId<MyType>());
            QCOMPARE(userVar.typeName(), "MyType");
            QVERIFY(!userVar.isNull());
            QVERIFY(!userVar.canConvert<QString>());

            QVariant userVar2(userVar);
            QCOMPARE(userVar, userVar2);

            userVar2.setValue(data2);
            QVERIFY(userVar != userVar2);

            const MyType *varData = static_cast<const MyType *>(userVar.constData());
            QVERIFY(varData);
            QCOMPARE(varData->number, data.number);
            QCOMPARE(varData->text, data.text);

            QVariant userVar3;
            userVar3.setValue(data2);

            userVar3 = userVar2;
            QCOMPARE(userVar2, userVar3);
        }
        // At this point all QVariants got destroyed but we have 2 MyType instances.
        QCOMPARE(instanceCount, 2);
        {
            QVariant userVar;
            userVar.setValue(&data);

            QVERIFY(userVar.typeId() > QMetaType::User);
            QCOMPARE(userVar.userType(), qMetaTypeId<MyType*>());
            QCOMPARE(userVar.typeName(), "MyType*");
            QVERIFY(!userVar.isNull());
            QVERIFY(!userVar.canConvert<QString>());

            QVariant userVar2(userVar);
            QCOMPARE(userVar, userVar2);

            userVar2.setValue(&data2);
            QVERIFY(userVar != userVar2);

            MyType * const*varData = reinterpret_cast<MyType *const *>(userVar.constData());
            QVERIFY(varData);
            QCOMPARE(*varData, &data);

            QVariant userVar3;
            userVar3.setValue(&data2);

            /* This check is correct now. userVar2 contains a pointer to data2 and so
             * does userVar3. */
            QCOMPARE(userVar2, userVar3);

            userVar3 = userVar2;
            QCOMPARE(userVar2, userVar3);
        }

        QCOMPARE(instanceCount, 2);
        QVariant myCarrier;
        myCarrier.setValue(data);
        QCOMPARE(instanceCount, 3);
        {
            QVariant second = myCarrier;
            QCOMPARE(instanceCount, 3);
            second.detach();
            QCOMPARE(instanceCount, 4);
        }
        QCOMPARE(instanceCount, 3);

        MyType data3(0, "null");
        data3 = qvariant_cast<MyType>(myCarrier);
        QCOMPARE(data3.number, 1);
        QCOMPARE(data3.text, (const char *)"eins");
#ifndef Q_CC_SUN
        QCOMPARE(instanceCount, 4);
#endif

    }

    {
        const MyType data(3, "drei");
        QVariant myCarrier;

        myCarrier.setValue(data);
        QCOMPARE(myCarrier.typeName(), "MyType");

        const MyType data2 = qvariant_cast<MyType>(myCarrier);
        QCOMPARE(data2.number, 3);
        QCOMPARE(data2.text, (const char *)"drei");
    }

    {
        short s = 42;
        QVariant myCarrier;

        myCarrier.setValue(s);
        QCOMPARE((int)qvariant_cast<short>(myCarrier), 42);
    }

    {
        qlonglong ll = Q_INT64_C(42);
        QVariant myCarrier;

        myCarrier.setValue(ll);
        QCOMPARE(qvariant_cast<int>(myCarrier), 42);
    }

    // At this point all QVariants got destroyed and MyType objects too.
    QCOMPARE(instanceCount, 0);
}

struct MyTypePOD
{
    int a;
    int b;
};
Q_DECLARE_METATYPE(MyTypePOD)

void tst_QVariant::podUserType()
{
    MyTypePOD pod;
    pod.a = 10;
    pod.b = 20;

    // one of these two must register the type
    // (QVariant::fromValue calls QMetaType::fromType)
    QVariant pod_as_variant = QVariant::fromValue(pod);
    QMetaType mt = QMetaType::fromType<MyTypePOD>();
    QCOMPARE(pod_as_variant.metaType(), mt);
    QCOMPARE(pod_as_variant.metaType().name(), mt.name());
    QCOMPARE(QMetaType::fromName(mt.name()), mt);
    QCOMPARE_NE(pod_as_variant.typeId(), 0);

    MyTypePOD pod2 = qvariant_cast<MyTypePOD>(pod_as_variant);

    QCOMPARE(pod.a, pod2.a);
    QCOMPARE(pod.b, pod2.b);

    pod_as_variant.setValue(pod);
    pod2 = qvariant_cast<MyTypePOD>(pod_as_variant);

    QCOMPARE(pod.a, pod2.a);
    QCOMPARE(pod.b, pod2.b);
}

void tst_QVariant::basicUserType()
{
    QVariant v;
    {
        int i = 7;
        v = QVariant(QMetaType::fromType<int>(), &i);
    }
    QCOMPARE(v.typeId(), QMetaType::Int);
    QCOMPARE(v.toInt(), 7);

    {
        QString s("foo");
        v = QVariant(QMetaType::fromType<QString>(), &s);
    }
    QCOMPARE(v.typeId(), QMetaType::QString);
    QCOMPARE(v.toString(), QString("foo"));

    {
        double d = 4.4;
        v = QVariant(QMetaType::fromType<double>(), &d);
    }
    QCOMPARE(v.typeId(), QMetaType::Double);
    QCOMPARE(v.toDouble(), 4.4);

    {
        float f = 4.5f;
        v = QVariant(QMetaType::fromType<float>(), &f);
    }
    QCOMPARE(v.userType(), int(QMetaType::Float));
    QCOMPARE(v.toDouble(), 4.5);

    {
        QByteArray ba("bar");
        v = QVariant(QMetaType::fromType<QByteArray>(), &ba);
    }
    QCOMPARE(v.typeId(), QMetaType::QByteArray);
    QCOMPARE(v.toByteArray(), QByteArray("bar"));
}

void tst_QVariant::data()
{
    QVariant v;

    QVariant i = 1;
    QVariant d = 1.12;
    QVariant f = 1.12f;
    QVariant ll = (qlonglong)2;
    QVariant ull = (qulonglong)3;
    QVariant s(QString("hallo"));
    QVariant r(QRect(1,2,3,4));

    v = i;
    QVERIFY(v.data());
    QCOMPARE(*static_cast<int *>(v.data()), i.toInt());

    v = d;
    QVERIFY(v.data());
    QCOMPARE(*static_cast<double *>(v.data()), d.toDouble());

    v = f;
    QVERIFY(v.data());
    QCOMPARE(*static_cast<float *>(v.data()), qvariant_cast<float>(v));

    v = ll;
    QVERIFY(v.data());
    QCOMPARE(*static_cast<qlonglong *>(v.data()), ll.toLongLong());

    v = ull;
    QVERIFY(v.data());
    QCOMPARE(*static_cast<qulonglong *>(v.data()), ull.toULongLong());

    v = s;
    QVERIFY(v.data());
    QCOMPARE(*static_cast<QString *>(v.data()), s.toString());

    v = r;
    QVERIFY(v.data());
    QCOMPARE(*static_cast<QRect *>(v.data()), r.toRect());
}

void tst_QVariant::constData()
{
    QVariant v;

    int i = 1;
    double d = 1.12;
    float f = 1.12f;
    qlonglong ll = 2;
    qulonglong ull = 3;
    QString s("hallo");
    QRect r(1,2,3,4);

    v = QVariant(i);
    QVERIFY(v.constData());
    QCOMPARE(*static_cast<const int *>(v.constData()), i);

    v = QVariant(d);
    QVERIFY(v.constData());
    QCOMPARE(*static_cast<const double *>(v.constData()), d);

    v = QVariant(f);
    QVERIFY(v.constData());
    QCOMPARE(*static_cast<const float *>(v.constData()), f);

    v = QVariant(ll);
    QVERIFY(v.constData());
    QCOMPARE(*static_cast<const qlonglong *>(v.constData()), ll);

    v = QVariant(ull);
    QVERIFY(v.constData());
    QCOMPARE(*static_cast<const qulonglong *>(v.constData()), ull);

    v = QVariant(s);
    QVERIFY(v.constData());
    QCOMPARE(*static_cast<const QString *>(v.constData()), s);

    v = QVariant(r);
    QVERIFY(v.constData());
    QCOMPARE(*static_cast<const QRect *>(v.constData()), r);
}

struct Foo
{
    Foo(): i(0) {}
    int i;
};

Q_DECLARE_METATYPE(Foo)

void tst_QVariant::variant_to()
{
    QVariant v1(4.2);
    QVariant v2(5);

    QVariant v3;
    QVariant v4;

    QStringList sl;
    sl << QLatin1String("blah");

    v3.setValue(sl);

    Foo foo;
    foo.i = 42;

    v4.setValue(foo);

    QCOMPARE(qvariant_cast<double>(v1), 4.2);
    QCOMPARE(qvariant_cast<float>(v1), 4.2f);
    QCOMPARE(qvariant_cast<int>(v2), 5);
    QCOMPARE(qvariant_cast<QStringList>(v3), sl);
    QCOMPARE(qvariant_cast<QString>(v3), QString::fromLatin1("blah"));

    QCOMPARE(qvariant_cast<Foo>(v4).i, 42);

    QVariant v5;
    QCOMPARE(qvariant_cast<Foo>(v5).i, 0);

    QCOMPARE(qvariant_cast<int>(v1), 4);

    QVariant n = QVariant::fromValue<short>(42);
    QCOMPARE(qvariant_cast<int>(n), 42);
    QCOMPARE(qvariant_cast<uint>(n), 42u);
    QCOMPARE(qvariant_cast<double>(n), 42.0);
    QCOMPARE(qvariant_cast<float>(n), 42.f);
    QCOMPARE(qvariant_cast<short>(n), short(42));
    QCOMPARE(qvariant_cast<ushort>(n), ushort(42));

    n = QVariant::fromValue(43l);
    QCOMPARE(qvariant_cast<int>(n), 43);
    QCOMPARE(qvariant_cast<uint>(n), 43u);
    QCOMPARE(qvariant_cast<double>(n), 43.0);
    QCOMPARE(qvariant_cast<float>(n), 43.f);
    QCOMPARE(qvariant_cast<long>(n), 43l);

    n = QLatin1String("44");
    QCOMPARE(qvariant_cast<int>(n), 44);
    QCOMPARE(qvariant_cast<ulong>(n), 44ul);
    QCOMPARE(qvariant_cast<float>(n), 44.0f);

    QCOMPARE(QVariant::fromValue(0.25f).toDouble(), 0.25);
}

struct Blah { int i; };

QDataStream& operator>>(QDataStream& s, Blah& c)
{ return (s >> c.i); }

QDataStream& operator<<(QDataStream& s, const Blah& c)
{ return (s << c.i); }

void tst_QVariant::saveLoadCustomTypes()
{
    QByteArray data;

    Blah i = { 42 };
    auto tp = QMetaType::fromType<Blah>();
    QVariant v = QVariant(tp, &i);

    QCOMPARE(v.userType(), tp.id());
    QVERIFY(v.typeId() > QMetaType::User);
    {
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream << v;
    }

    v = QVariant();

    {
        QDataStream stream(data);
        stream >> v;
    }

    QCOMPARE(int(v.userType()), QMetaType::fromName("Blah").id());
    int value = *(int*)v.constData();
    QCOMPARE(value, 42);
}

void tst_QVariant::url()
{
    QString str("http://qt-project.org");
    QUrl url(str);

    QVariant v(url); //built with a QUrl

    QVariant v2 = v;

    QVariant v3(str); //built with a QString

    QCOMPARE(v2.toUrl(), url);
    QVERIFY(v3.canConvert<QUrl>());
    QCOMPARE(v2.toUrl(), v3.toUrl());

    QVERIFY(v2.canConvert<QString>());
    QCOMPARE(v2.toString(), str);
    QCOMPARE(v3.toString(), str);
}

void tst_QVariant::variantMap()
{
    QMap<QString, QVariant> map;
    map["test"] = 42;

    QVariant v = map;
    QVariantMap map2 = qvariant_cast<QVariantMap>(v);
    QCOMPARE(map2.value("test").toInt(), 42);
    QCOMPARE(map2, map);

    map2 = v.toMap();
    QCOMPARE(map2.value("test").toInt(), 42);
    QCOMPARE(map2, map);

    QVariant v2 = QVariant(QMetaType::fromType<QVariantMap>(), &map);
    QCOMPARE(qvariant_cast<QVariantMap>(v2).value("test").toInt(), 42);

    QVariant v3 = QVariant(QMetaType::fromType<QMap<QString, QVariant>>(), &map);
    QCOMPARE(qvariant_cast<QVariantMap>(v3).value("test").toInt(), 42);

    QHash<QString, QVariant> hash;
    hash["test"] = 42;
    QCOMPARE(hash, v.toHash());
}

void tst_QVariant::variantHash()
{
    QHash<QString, QVariant> hash;
    hash["test"] = 42;

    QVariant v = hash;
    QVariantHash hash2 = qvariant_cast<QVariantHash>(v);
    QCOMPARE(hash2.value("test").toInt(), 42);
    QCOMPARE(hash2, hash);

    hash2 = v.toHash();
    QCOMPARE(hash2.value("test").toInt(), 42);
    QCOMPARE(hash2, hash);

    QVariant v2 = QVariant(QMetaType::fromType<QVariantHash>(), &hash);
    QCOMPARE(qvariant_cast<QVariantHash>(v2).value("test").toInt(), 42);

    QVariant v3 = QVariant(QMetaType::fromType<QHash<QString, QVariant>>(), &hash);
    QCOMPARE(qvariant_cast<QVariantHash>(v3).value("test").toInt(), 42);

    QMap<QString, QVariant> map;
    map["test"] = 42;
    QCOMPARE(map, v.toMap());
}

class CustomQObject : public QObject {
    Q_OBJECT
public:
    CustomQObject(QObject *parent = nullptr) : QObject(parent) {}
};
Q_DECLARE_METATYPE(CustomQObject*)

class CustomNonQObject { };
Q_DECLARE_METATYPE(CustomNonQObject)
Q_DECLARE_METATYPE(CustomNonQObject*)

void tst_QVariant::cleanupTestCase()
{
    delete customNonQObjectPointer;
    qDeleteAll(objectPointerTestData);
}

void tst_QVariant::qvariant_cast_QObject_data()
{
    QTest::addColumn<QVariant>("data");
    QTest::addColumn<bool>("success");
    QTest::addColumn<bool>("isNull");
    QObject *obj = new QObject;
    obj->setObjectName(QString::fromLatin1("Hello"));
    QTest::newRow("from QObject") << QVariant(QMetaType::fromType<QObject*>(), &obj) << true << false;
    QTest::newRow("from QObject2") << QVariant::fromValue(obj) << true << false;
    QTest::newRow("from String") << QVariant(QLatin1String("1, 2, 3")) << false << false;
    QTest::newRow("from int") << QVariant((int) 123) << false << false;
    CustomQObject *customObject = new CustomQObject(this);
    customObject->setObjectName(QString::fromLatin1("Hello"));
    QTest::newRow("from Derived QObject") << QVariant::fromValue(customObject) << true << false;
    QTest::newRow("from custom Object") << QVariant::fromValue(CustomNonQObject()) << false << false;

    // Deleted in cleanupTestCase.
    customNonQObjectPointer = new CustomNonQObject;
    QTest::newRow("from custom ObjectStar") << QVariant::fromValue(customNonQObjectPointer) << false << false;

    // Deleted in cleanupTestCase.
    objectPointerTestData.push_back(obj);
    objectPointerTestData.push_back(customObject);

    QTest::newRow("null QObject") << QVariant::fromValue<QObject*>(0) << true << true;
    QTest::newRow("null derived QObject") << QVariant::fromValue<CustomQObject*>(0) << true << true;
    QTest::newRow("null custom object") << QVariant::fromValue<CustomNonQObject*>(0) << false << true;
    QTest::newRow("zero int") << QVariant::fromValue<int>(0) << false << false;
}

void tst_QVariant::qvariant_cast_QObject()
{
    QFETCH(QVariant, data);
    QFETCH(bool, success);
    QFETCH(bool, isNull);

    QObject *o = qvariant_cast<QObject *>(data);
    QCOMPARE(o != nullptr, success && !isNull);
    if (success) {
        if (!isNull)
            QCOMPARE(o->objectName(), QString::fromLatin1("Hello"));
        QVERIFY(data.canConvert<QObject*>());
        QVERIFY(data.canConvert(QMetaType::fromType<QObject*>()));
        QVERIFY(data.canConvert(QMetaType(::qMetaTypeId<QObject*>())));
        QCOMPARE(data.value<QObject*>() == nullptr, isNull);
        QVERIFY(data.convert(QMetaType::fromType<QObject*>()));
        QCOMPARE(data.userType(), int(QMetaType::QObjectStar));
    } else {
        QVERIFY(!data.canConvert<QObject*>());
        QVERIFY(!data.canConvert(QMetaType::fromType<QObject*>()));
        QVERIFY(!data.canConvert(QMetaType(::qMetaTypeId<QObject*>())));
        QVERIFY(!data.value<QObject*>());
        QVERIFY(data.userType() != QMetaType::QObjectStar);
        QVERIFY(!data.convert(QMetaType::fromType<QObject*>()));
    }
}

class CustomQObjectDerived : public CustomQObject {
    Q_OBJECT
public:
    CustomQObjectDerived(QObject *parent = nullptr) : CustomQObject(parent) {}
};
Q_DECLARE_METATYPE(CustomQObjectDerived*)

class CustomQObjectDerivedNoMetaType : public CustomQObject {
    Q_OBJECT
public:
    CustomQObjectDerivedNoMetaType(QObject *parent = nullptr) : CustomQObject(parent) {}
};

void tst_QVariant::qvariant_cast_QObject_derived()
{
    {
        CustomQObjectDerivedNoMetaType *object = new CustomQObjectDerivedNoMetaType(this);
        QVariant data = QVariant::fromValue(object);
        QCOMPARE(data.userType(), qMetaTypeId<CustomQObjectDerivedNoMetaType*>());
        QCOMPARE(data.value<QObject *>(), object);
        QCOMPARE(data.value<CustomQObjectDerivedNoMetaType *>(), object);
        QCOMPARE(data.value<CustomQObject *>(), object);
    }
    {
        CustomQObjectDerived *object = new CustomQObjectDerived(this);
        QVariant data = QVariant::fromValue(object);

        QCOMPARE(data.userType(), qMetaTypeId<CustomQObjectDerived*>());

        QCOMPARE(data.value<QObject *>(), object);
        QCOMPARE(data.value<CustomQObjectDerived *>(), object);
        QCOMPARE(data.value<CustomQObject *>(), object);
    }
    {
        QObject *object = new CustomQObjectDerivedNoMetaType(this);
        QVariant data = QVariant::fromValue(object);
        QVERIFY(data.canConvert<CustomQObjectDerivedNoMetaType*>());
        QVERIFY(data.convert(QMetaType(qMetaTypeId<CustomQObjectDerivedNoMetaType*>())));
        QCOMPARE(data.value<CustomQObjectDerivedNoMetaType*>(), object);
        QCOMPARE(data.isNull(), false);
    }
}

struct QObjectWrapper
{
    explicit QObjectWrapper(QObject *o = nullptr) : obj(o) {}

    QObject* getObject() const {
        return obj;
    }
private:
    QObject *obj;
};

Q_DECLARE_METATYPE(QObjectWrapper)

struct Converter
{
  Converter() {}

  QObject* operator()(const QObjectWrapper &f) const
  {
      return f.getObject();
  }
};

namespace MyNS {

template<typename T>
class SmartPointer
{
    T* pointer;
public:
    typedef T element_type;
    explicit SmartPointer(T *t = nullptr)
      : pointer(t)
    {
    }

    T* operator->() const { return pointer; }
};

template<typename T>
struct SequentialContainer
{
  typedef T value_type;
  typedef const T* const_iterator;
  T t;
  const_iterator begin() const { return &t; }
  const_iterator end() const { return &t + 1; }
};

template<typename T, typename U>
struct AssociativeContainer : public std::map<T, U>
{
};

}

Q_DECLARE_SMART_POINTER_METATYPE(MyNS::SmartPointer)

Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE(MyNS::SequentialContainer)
Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE(MyNS::AssociativeContainer)

// Test that explicit declaration does not degrade features.
Q_DECLARE_METATYPE(MyNS::SmartPointer<int>)
Q_DECLARE_METATYPE(MyNS::SmartPointer<QIODevice>)
Q_DECLARE_METATYPE(QSharedPointer<QIODevice>)

void tst_QVariant::qvariant_cast_QObject_wrapper()
{
    QMetaType::registerConverter<QObjectWrapper, QObject*>(&QObjectWrapper::getObject);

    CustomQObjectDerived *object = new CustomQObjectDerived(this);
    QObjectWrapper wrapper(object);
    QVariant v = QVariant::fromValue(wrapper);
    QCOMPARE(v.value<QObject*>(), object);
    v.convert(QMetaType(qMetaTypeId<QObject*>()));
    QCOMPARE(v.value<QObject*>(), object);

    MyNS::SequentialContainer<int> sc;
    sc.t = 47;
    MyNS::AssociativeContainer<int, short> ac;

    QVariant::fromValue(sc);
    QVariant::fromValue(ac);

    {
        QFile *f = new QFile(this);
        MyNS::SmartPointer<QFile> sp(f);
        QVariant spVar = QVariant::fromValue(sp);
        QVERIFY(spVar.canConvert<QObject*>());
        QCOMPARE(f, spVar.value<QObject*>());
    }
    {
        QFile *f = new QFile(this);
        QPointer<QFile> sp(f);
        QVariant spVar = QVariant::fromValue(sp);
        QVERIFY(spVar.canConvert<QObject*>());
        QCOMPARE(f, spVar.value<QObject*>());
    }
    {
        QFile *f = new QFile(this);
        QSharedPointer<QObject> sp(f);
        QWeakPointer<QObject> wp = sp;
        QVariant wpVar = QVariant::fromValue(wp);
        QVERIFY(wpVar.canConvert<QObject*>());
        QCOMPARE(f, wpVar.value<QObject*>());
    }
    {
        QFile *f = new QFile(this);
        QSharedPointer<QFile> sp(f);
        QWeakPointer<QFile> wp = sp.toWeakRef();
        QVariant wpVar = QVariant::fromValue(wp);
        QVERIFY(wpVar.canConvert<QObject*>());
        QCOMPARE(f, wpVar.value<QObject*>());
    }
    {
        QFile *f = new QFile(this);
        QSharedPointer<QFile> sp(f);
        QVariant spVar = QVariant::fromValue(sp);
        QVERIFY(spVar.canConvert<QObject*>());
        QCOMPARE(f, spVar.value<QObject*>());
    }
    {
        QIODevice *f = new QFile(this);
        MyNS::SmartPointer<QIODevice> sp(f);
        QVariant spVar = QVariant::fromValue(sp);
        QVERIFY(spVar.canConvert<QObject*>());
        QCOMPARE(f, spVar.value<QObject*>());
    }
    {
        QIODevice *f = new QFile(this);
        QSharedPointer<QIODevice> sp(f);
        QVariant spVar = QVariant::fromValue(sp);
        QVERIFY(spVar.canConvert<QObject*>());
        QCOMPARE(f, spVar.value<QObject*>());
    }

    // Compile tests:
    qRegisterMetaType<MyNS::SmartPointer<int> >();
    // Not declared as a metatype:
    qRegisterMetaType<MyNS::SmartPointer<double> >("MyNS::SmartPointer<double>");
}

void tst_QVariant::qvariant_cast_QSharedPointerQObject()
{
    // ensure no problems between this form and the auto-registering in QVariant::fromValue
    qRegisterMetaType<QSharedPointer<QObject> >("QSharedPointer<QObject>");

    QObject *rawptr = new QObject;
    QSharedPointer<QObject> strong(rawptr);
    QWeakPointer<QObject> weak(strong);
    QPointer<QObject> qptr(rawptr);

    QVariant v = QVariant::fromValue(strong);
    QCOMPARE(v.value<QSharedPointer<QObject> >(), strong);

    // clear our QSP; the copy inside the variant should keep the object alive
    strong.clear();

    // check that the object didn't get deleted
    QVERIFY(!weak.isNull());
    QVERIFY(!qptr.isNull());

    strong = qvariant_cast<QSharedPointer<QObject> >(v);
    QCOMPARE(strong.data(), rawptr);
    QVERIFY(strong == weak);

    // now really delete the object and verify
    strong.clear();
    v.clear();
    QVERIFY(weak.isNull());
    QVERIFY(qptr.isNull());

    // compile test:
    // QVariant::fromValue has already called this function
    qRegisterMetaType<QSharedPointer<QObject> >();
}

void tst_QVariant::qvariant_cast_const()
{
    int i = 42;
    QVariant v = QVariant::fromValue(&i);
    QVariant vConst = QVariant::fromValue(const_cast<const int*>(&i));
    QCOMPARE(v.value<int *>(), &i);
    QCOMPARE(v.value<const int *>(), &i);
    QCOMPARE(vConst.value<int *>(), nullptr);
    QCOMPARE(vConst.value<const int *>(), &i);
}

void tst_QVariant::qvariant_cast_QTransform()
{
    QTransform t{1, 2, 3, 4, 5, 6, 7, 8, 9};
    // this basically checks that GCC doesn't emit -Warray-bound
    const auto t2 = qvariant_cast<QTransform>(QVariant::fromValue(t));
    QCOMPARE(t, t2);
}

void tst_QVariant::convertToQUint8() const
{
    /* qint8. */
    {
        const qint8 anInt = 32;

        /* QVariant(int) gets invoked here so the QVariant has nothing with qint8 to do.
         * It's of type QVariant::Int. */
        const QVariant v0 = anInt;

        QVERIFY(v0.canConvert<qint8>());
        QCOMPARE(int(qvariant_cast<qint8>(v0)), 32);
        QCOMPARE(int(v0.toInt()), 32);
        QCOMPARE(v0.toString(), QString("32"));

        QCOMPARE(int(qvariant_cast<qlonglong>(v0)), 32);
        QCOMPARE(int(qvariant_cast<char>(v0)),      32);
        QCOMPARE(int(qvariant_cast<short>(v0)),     32);
        QCOMPARE(int(qvariant_cast<long>(v0)),      32);
        QCOMPARE(int(qvariant_cast<float>(v0)),     32);
        QCOMPARE(int(qvariant_cast<double>(v0)),    32);
    }

    /* quint8. */
    {
        const quint8 anInt = 32;
        const QVariant v0 = anInt;

        QVERIFY(v0.canConvert<quint8>());
        QCOMPARE(int(qvariant_cast<quint8>(v0)), 32);
        QCOMPARE(int(v0.toUInt()), 32);
        QCOMPARE(v0.toString(), QString("32"));
    }

    /* qint16. */
    {
        const qint16 anInt = 32;
        const QVariant v0 = anInt;

        QVERIFY(v0.canConvert<qint16>());
        QCOMPARE(int(qvariant_cast<qint16>(v0)), 32);
        QCOMPARE(int(v0.toInt()), 32);
        QCOMPARE(v0.toString(), QString("32"));
    }

    /* quint16. */
    {
        const quint16 anInt = 32;
        const QVariant v0 = anInt;

        QVERIFY(v0.canConvert<quint16>());
        QCOMPARE(int(qvariant_cast<quint16>(v0)), 32);
        QCOMPARE(int(v0.toUInt()), 32);
        QCOMPARE(v0.toString(), QString("32"));
    }
}

void tst_QVariant::compareCompiles() const
{
    QTestPrivate::testEqualityOperatorsCompile<QVariant>();
}

void tst_QVariant::compareNumerics_data() const
{
    QTest::addColumn<QVariant>("v1");
    QTest::addColumn<QVariant>("v2");
    QTest::addColumn<QPartialOrdering>("result");

    QTest::addRow("invalid-invalid")
            << QVariant() << QVariant() << QPartialOrdering::Unordered;

    static const auto asString = [](const QVariant &v) {
        if (v.isNull())
            return QStringLiteral("null");
        if (v.metaType().flags() & QMetaType::IsEnumeration)
            return v.metaType().flags() & QMetaType::IsUnsignedEnumeration ?
                        QString::number(v.toULongLong()) :
                        QString::number(v.toLongLong());
        switch (v.typeId()) {
        case QMetaType::Char16:
            return QString::number(qvariant_cast<char16_t>(v));
        case QMetaType::Char32:
            return QString::number(qvariant_cast<char32_t>(v));
        case QMetaType::Char:
        case QMetaType::UChar:
            return QString::number(v.toUInt());
        case QMetaType::SChar:
            return QString::number(v.toInt());
        }
        return v.toString();
    };

    auto addCompareToInvalid = [](auto value) {
        QVariant v = QVariant::fromValue(value);
        QTest::addRow("invalid-%s(%s)", v.typeName(), qPrintable(asString(v)))
                << QVariant() << v << QPartialOrdering::Unordered;
        QTest::addRow("%s(%s)-invalid", v.typeName(), qPrintable(asString(v)))
                << v << QVariant() << QPartialOrdering::Unordered;
    };
    addCompareToInvalid(false);
    addCompareToInvalid(true);
    addCompareToInvalid(char(0));
    addCompareToInvalid(qint8(0));
    addCompareToInvalid(quint8(0));
    addCompareToInvalid(short(0));
    addCompareToInvalid(ushort(0));
    addCompareToInvalid(int(0));
    addCompareToInvalid(uint(0));
    addCompareToInvalid(long(0));
    addCompareToInvalid(ulong(0));
    addCompareToInvalid(qint64(0));
    addCompareToInvalid(quint64(0));
    addCompareToInvalid(qfloat16(0.f));
    addCompareToInvalid(0.f);
    addCompareToInvalid(0.0);
    addCompareToInvalid(QCborSimpleType{});

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wsign-compare")
QT_WARNING_DISABLE_GCC("-Wsign-compare")
QT_WARNING_DISABLE_MSVC(4018)   // '<': signed/unsigned mismatch
    static const auto addComparePairWithResult = [](auto value1, auto value2, QPartialOrdering order) {
        QVariant v1 = QVariant::fromValue(value1);
        QVariant v2 = QVariant::fromValue(value2);
        QTest::addRow("%s(%s)-%s(%s)", v1.typeName(), qPrintable(asString(v1)),
                      v2.typeName(), qPrintable(asString(v2)))
                << v1 << v2 << order;
    };

    static const auto addComparePair = [](auto value1, auto value2) {
        QPartialOrdering order = QPartialOrdering::Unordered;
        if (value1 == value2)
            order = QPartialOrdering::Equivalent;
        else if (value1 < value2)
            order = QPartialOrdering::Less;
        else if (value1 > value2)
            order = QPartialOrdering::Greater;
        addComparePairWithResult(value1, value2, order);
    };
QT_WARNING_POP

    // homogeneous first
    static const auto addList = [](auto list) {
        for (auto v1 : list)
            for (auto v2 : list)
                addComparePair(v1, v2);
    };

    auto addSingleType = [](auto zero) {
        using T = decltype(zero);
        T one = T(zero + 1);
        T min = std::numeric_limits<T>::min();
        T max = std::numeric_limits<T>::max();
        T mid = T(max / 2 + 1);
        if (min != zero)
            addList(std::array{zero, one, min, mid, max});
        else
            addList(std::array{zero, one, mid, max});
    };
    addList(std::array{ false, true });
    addList(std::array{ QCborSimpleType{}, QCborSimpleType::False, QCborSimpleType(0xff) });
    addSingleType(char(0));
    addSingleType(char16_t(0));
    addSingleType(char32_t(0));
    addSingleType(qint8(0));
    addSingleType(quint8(0));
    addSingleType(qint16(0));
    addSingleType(quint16(0));
    addSingleType(qint32(0));
    addSingleType(quint32(0));
    addSingleType(qint64(0));
    addSingleType(quint64(0));
    addSingleType(qfloat16(0.f));
    addSingleType(0.f);
    addSingleType(0.0);
    addList(std::array{ EnumTest_Enum0{}, EnumTest_Enum0_value, EnumTest_Enum0_negValue });
    addList(std::array{ EnumTest_Enum1{}, EnumTest_Enum1_value, EnumTest_Enum1_bigValue });
    addList(std::array{ EnumTest_Enum7{}, EnumTest_Enum7::EnumTest_Enum7_value, EnumTest_Enum7::ensureSignedEnum7 });
    addList(std::array{ Qt::AlignRight|Qt::AlignHCenter, Qt::AlignCenter|Qt::AlignVCenter });

    // heterogeneous
    addComparePair(char(0), qint8(-127));
    addComparePair(char(127), qint8(127));
    addComparePair(char(127), quint8(127));
    addComparePair(qint8(-1), quint8(255));
    addComparePair(char16_t(256), qint8(-1));
    addComparePair(char16_t(256), short(-1));
    addComparePair(char16_t(256), int(-1));
    addComparePair(char32_t(256), int(-1));
    addComparePair(0U, -1);
    addComparePair(~0U, -1);
    addComparePair(Q_UINT64_C(0), -1);
    addComparePair(~Q_UINT64_C(0), -1);
    addComparePair(Q_UINT64_C(0), Q_INT64_C(-1));
    addComparePair(~Q_UINT64_C(0), Q_INT64_C(-1));
    addComparePair(INT_MAX, uint(INT_MAX));
    addComparePair(INT_MAX, qint64(INT_MAX) + 1);
    addComparePair(INT_MAX, UINT_MAX);
    addComparePair(INT_MAX, qint64(UINT_MAX));
    addComparePair(INT_MAX, qint64(UINT_MAX) + 1);
    addComparePair(INT_MAX, quint64(UINT_MAX));
    addComparePair(INT_MAX, quint64(UINT_MAX) + 1);
    addComparePair(INT_MAX, LONG_MIN);
    addComparePair(INT_MAX, LONG_MAX);
    addComparePair(INT_MAX, LLONG_MIN);
    addComparePair(INT_MAX, LLONG_MAX);
    addComparePair(INT_MIN, uint(INT_MIN));
    addComparePair(INT_MIN, uint(INT_MIN) + 1);
    addComparePair(INT_MIN + 1, uint(INT_MIN));
    addComparePair(INT_MIN + 1, uint(INT_MIN) + 1);
    addComparePair(INT_MIN, qint64(INT_MIN) - 1);
    addComparePair(INT_MIN + 1, qint64(INT_MIN) + 1);
    addComparePair(INT_MIN + 1, qint64(INT_MIN) - 1);
    addComparePair(INT_MIN, UINT_MAX);
    addComparePair(INT_MIN, qint64(UINT_MAX));
    addComparePair(INT_MIN, qint64(UINT_MAX) + 1);
    addComparePair(INT_MIN, quint64(UINT_MAX));
    addComparePair(INT_MIN, quint64(UINT_MAX) + 1);
    addComparePair(UINT_MAX, qint64(UINT_MAX) + 1);
    addComparePair(UINT_MAX, quint64(UINT_MAX) + 1);
    addComparePair(UINT_MAX, qint64(INT_MIN) - 1);
    addComparePair(UINT_MAX, quint64(INT_MIN) + 1);
    addComparePair(LLONG_MAX, quint64(LLONG_MAX));
    addComparePair(LLONG_MAX, quint64(LLONG_MAX) + 1);
    addComparePair(LLONG_MIN, quint64(LLONG_MAX));
    addComparePair(LLONG_MIN, quint64(LLONG_MAX) + 1);
    addComparePair(LLONG_MIN, quint64(LLONG_MIN) + 1);
    addComparePair(LLONG_MIN + 1, quint64(LLONG_MIN) + 1);
    addComparePair(LLONG_MIN, LLONG_MAX - 1);
    // addComparePair(LLONG_MIN, LLONG_MAX); // already added by addSingleType()

    // floating point
    addComparePair(qfloat16(0.f), 0);
    addComparePair(qfloat16(0.f), 0U);
    addComparePair(qfloat16(0.f), Q_INT64_C(0));
    addComparePair(qfloat16(0.f), Q_UINT64_C(0));
    addComparePair(qfloat16(0.f), 0.f);
    addComparePair(qfloat16(0.f), 1.f);
    addComparePair(qfloat16(0.f), 0.);
    addComparePair(qfloat16(0.f), 1.);
    addComparePair(qfloat16(1 << 11), 1 << 11);
    addComparePair(qfloat16(1 << 11) - 1, (1 << 11) - 1);
    addComparePair(-qfloat16(1 << 11), 1 << 11);
    addComparePair(-qfloat16(1 << 11) + 1, -(1 << 11) + 1);
    addComparePair(std::numeric_limits<qfloat16>::infinity(), qInf());
    addComparePair(std::numeric_limits<qfloat16>::infinity(), -qInf());
    addComparePair(std::numeric_limits<qfloat16>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());

    addComparePair(0.f, 0);
    addComparePair(0.f, 0U);
    addComparePair(0.f, Q_INT64_C(0));
    addComparePair(0.f, Q_UINT64_C(0));
    addComparePair(0.f, 0.);
    addComparePair(0.f, 1.);
    addComparePair(float(1 << 24), 1 << 24);
    addComparePair(float(1 << 24) - 1, (1 << 24) - 1);
    addComparePair(-float(1 << 24), 1 << 24);
    addComparePair(-float(1 << 24) + 1, -(1 << 24) + 1);
    addComparePair(HUGE_VALF, qInf());
    addComparePair(HUGE_VALF, -qInf());
    addComparePair(std::numeric_limits<float>::quiet_NaN(), qQNaN());
    if (sizeof(qreal) == sizeof(double)) {
        addComparePair(std::numeric_limits<float>::min(), std::numeric_limits<double>::min());
        addComparePair(std::numeric_limits<float>::min(), std::numeric_limits<double>::max());
        addComparePair(std::numeric_limits<float>::max(), std::numeric_limits<double>::min());
        addComparePair(std::numeric_limits<float>::max(), std::numeric_limits<double>::max());
        addComparePair(double(Q_INT64_C(1) << 53), Q_INT64_C(1) << 53);
        addComparePair(double(Q_INT64_C(1) << 53) - 1, (Q_INT64_C(1) << 53) - 1);
        addComparePair(-double(Q_INT64_C(1) << 53), Q_INT64_C(1) << 53);
        addComparePair(-double(Q_INT64_C(1) << 53) + 1, (Q_INT64_C(1) << 53) + 1);
    }

    // enums vs integers
    addComparePair(EnumTest_Enum0_value, 0);
    addComparePair(EnumTest_Enum0_value, 0U);
    addComparePair(EnumTest_Enum0_value, 0LL);
    addComparePair(EnumTest_Enum0_value, 0ULL);
    addComparePair(EnumTest_Enum0_value, int(EnumTest_Enum0_value));
    addComparePair(EnumTest_Enum0_value, qint64(EnumTest_Enum0_value));
    addComparePair(EnumTest_Enum0_value, quint64(EnumTest_Enum0_value));
    addComparePair(EnumTest_Enum0_negValue, int(EnumTest_Enum0_value));
    addComparePair(EnumTest_Enum0_negValue, qint64(EnumTest_Enum0_value));
    addComparePair(EnumTest_Enum0_negValue, quint64(EnumTest_Enum0_value));
    addComparePair(EnumTest_Enum0_negValue, int(EnumTest_Enum0_negValue));
    addComparePair(EnumTest_Enum0_negValue, qint64(EnumTest_Enum0_negValue));
    addComparePair(EnumTest_Enum0_negValue, quint64(EnumTest_Enum0_negValue));

    addComparePair(EnumTest_Enum1_value, 0);
    addComparePair(EnumTest_Enum1_value, 0U);
    addComparePair(EnumTest_Enum1_value, 0LL);
    addComparePair(EnumTest_Enum1_value, 0ULL);
    addComparePair(EnumTest_Enum1_value, int(EnumTest_Enum1_value));
    addComparePair(EnumTest_Enum1_value, qint64(EnumTest_Enum1_value));
    addComparePair(EnumTest_Enum1_value, quint64(EnumTest_Enum1_value));
    addComparePair(EnumTest_Enum1_bigValue, int(EnumTest_Enum1_value));
    addComparePair(EnumTest_Enum1_bigValue, qint64(EnumTest_Enum1_value));
    addComparePair(EnumTest_Enum1_bigValue, quint64(EnumTest_Enum1_value));
    addComparePair(EnumTest_Enum1_bigValue, int(EnumTest_Enum1_bigValue));
    addComparePair(EnumTest_Enum1_bigValue, qint64(EnumTest_Enum1_bigValue));
    addComparePair(EnumTest_Enum1_bigValue, quint64(EnumTest_Enum1_bigValue));

    addComparePair(EnumTest_Enum3_value, 0);
    addComparePair(EnumTest_Enum3_value, 0U);
    addComparePair(EnumTest_Enum3_value, 0LL);
    addComparePair(EnumTest_Enum3_value, 0ULL);
    addComparePair(EnumTest_Enum3_value, int(EnumTest_Enum3_value));
    addComparePair(EnumTest_Enum3_value, qint64(EnumTest_Enum3_value));
    addComparePair(EnumTest_Enum3_value, quint64(EnumTest_Enum3_value));
    addComparePair(EnumTest_Enum3_bigValue, int(EnumTest_Enum3_value));
    addComparePair(EnumTest_Enum3_bigValue, qint64(EnumTest_Enum3_value));
    addComparePair(EnumTest_Enum3_bigValue, quint64(EnumTest_Enum3_value));
    addComparePair(EnumTest_Enum3_bigValue, int(EnumTest_Enum3_bigValue));
    addComparePair(EnumTest_Enum3_bigValue, qint64(EnumTest_Enum3_bigValue));
    addComparePair(EnumTest_Enum3_bigValue, quint64(EnumTest_Enum3_bigValue));

    // enums of different types always compare as unordered
    addComparePairWithResult(EnumTest_Enum0_value, EnumTest_Enum1_value, QPartialOrdering::Unordered);
}

void tst_QVariant::compareNumerics() const
{
    QFETCH(QVariant, v1);
    QFETCH(QVariant, v2);
    QFETCH(QPartialOrdering, result);
    QCOMPARE(QVariant::compare(v1, v2), result);

    QEXPECT_FAIL("invalid-invalid", "needs fixing", Abort);
    QT_TEST_EQUALITY_OPS(v1, v2, is_eq(result));
}

void tst_QVariant::comparePointers() const
{
    class NonQObjectClass {};
    const std::array<NonQObjectClass, 2> arr{ NonQObjectClass{}, NonQObjectClass{} };

    const QVariant nonObjV1 = QVariant::fromValue<const void*>(&arr[0]);
    const QVariant nonObjV2 = QVariant::fromValue<const void*>(&arr[1]);

    Qt::partial_ordering expectedOrdering = Qt::partial_ordering::equivalent;
    QCOMPARE(QVariant::compare(nonObjV1, nonObjV1), expectedOrdering);
    QT_TEST_EQUALITY_OPS(nonObjV1, nonObjV1, is_eq(expectedOrdering));

    expectedOrdering = Qt::partial_ordering::less;
    QCOMPARE(QVariant::compare(nonObjV1, nonObjV2), expectedOrdering);
    QT_TEST_EQUALITY_OPS(nonObjV1, nonObjV2, is_eq(expectedOrdering));

    class QObjectClass : public QObject
    {
    public:
        QObjectClass(QObject *parent = nullptr) : QObject(parent) {}
    };
    const QObjectClass c1;
    const QObjectClass c2;

    const QVariant objV1 = QVariant::fromValue(&c1);
    const QVariant objV2 = QVariant::fromValue(&c2);
    QT_TEST_EQUALITY_OPS(objV1, objV1, true);
    QT_TEST_EQUALITY_OPS(objV1, objV2, false);
}

struct Data {};
Q_DECLARE_METATYPE(Data*)

void tst_QVariant::voidStar() const
{
    char c;
    void *p1 = &c;
    void *p2 = p1;

    QVariant v1, v2;
    v1 = QVariant::fromValue(p1);
    v2 = v1;
    QCOMPARE(v1, v2);

    v2 = QVariant::fromValue(p2);
    QCOMPARE(v1, v2);

    p2 = nullptr;
    v2 = QVariant::fromValue(p2);
    QVERIFY(v1 != v2);
}

void tst_QVariant::dataStar() const
{
    qRegisterMetaType<Data*>();
    Data *p1 = new Data;

    QVariant v1 = QVariant::fromValue(p1);
    QCOMPARE(v1.userType(), qMetaTypeId<Data*>());
    QCOMPARE(qvariant_cast<Data*>(v1), p1);

    QVariant v2 = v1;
    QCOMPARE(v1, v2);

    v2 = QVariant::fromValue(p1);
    QCOMPARE(v1, v2);
    delete p1;
}

void tst_QVariant::canConvertQStringList() const
{
    QFETCH(bool, canConvert);
    QFETCH(QStringList, input);
    QFETCH(QString, result);

    QVariant v(input);

    QCOMPARE(v.canConvert<QString>(), canConvert);
    QCOMPARE(v.toString(), result);
}

void tst_QVariant::canConvertQStringList_data() const
{
    QTest::addColumn<bool>("canConvert");
    QTest::addColumn<QStringList>("input");
    QTest::addColumn<QString>("result");

    QTest::newRow("An empty list") << true << QStringList() << QString();
    QTest::newRow("A single item") << true << QStringList(QLatin1String("foo")) << QString::fromLatin1("foo");
    QTest::newRow("A single, but empty item") << true << QStringList(QString()) << QString();

    QStringList l;
    l << "a" << "b";

    QTest::newRow("Two items") << true << l << QString();

    l << "c";
    QTest::newRow("Three items") << true << l << QString();
}

template<typename T> void convertMetaType()
{
    QVERIFY(QVariant::fromValue<T>(10).isValid());
    QVERIFY(QVariant::fromValue<T>(10).template canConvert<int>());
    QCOMPARE(QVariant::fromValue<T>(10).toInt(), 10);
    QCOMPARE(QVariant::fromValue<T>(10), QVariant::fromValue<T>(10));
}

#define CONVERT_META_TYPE(Type) \
    convertMetaType<Type>(); \
    if (QTest::currentTestFailed()) \
        QFAIL("convertMetaType<" #Type "> failed");

void tst_QVariant::canConvertMetaTypeToInt() const
{
    CONVERT_META_TYPE(long);
    CONVERT_META_TYPE(short);
    CONVERT_META_TYPE(short);
    CONVERT_META_TYPE(unsigned short);
    CONVERT_META_TYPE(ushort);
    CONVERT_META_TYPE(ulong);
    CONVERT_META_TYPE(unsigned long);
    CONVERT_META_TYPE(uchar);
    CONVERT_META_TYPE(unsigned char);
    CONVERT_META_TYPE(char);
    CONVERT_META_TYPE(uint);
    CONVERT_META_TYPE(unsigned int);
}

#undef CONVERT_META_TYPE

/*!
 These calls should not produce any warnings.
 */
void tst_QVariant::variantToDateTimeWithoutWarnings() const
{
    {
        const QVariant variant(QLatin1String("An invalid QDateTime string"));
        const QDateTime dateTime(variant.toDateTime());
        QVERIFY(!dateTime.isValid());
    }

    {
        QVariant v1(QLatin1String("xyz"));
        v1.convert(QMetaType::fromType<QDateTime>());

        QVariant v2(QLatin1String("xyz"));
        QDateTime dt1(v2.toDateTime());

        const QVariant v3(QLatin1String("xyz"));
        const QDateTime dt2(v3.toDateTime());
    }
}

void tst_QVariant::invalidDateTime() const
{
    QVariant variant(QString::fromLatin1("Invalid date time string"));
    QVERIFY(!variant.toDateTime().isValid());
    QVERIFY(!variant.convert(QMetaType::fromType<QDateTime>()));
}

struct MyClass
{
    MyClass() : myValue(0) {}
    int myValue;
};

Q_DECLARE_METATYPE( MyClass )

void tst_QVariant::loadUnknownUserType()
{
    qRegisterMetaType<MyClass>("MyClass");
    QTest::ignoreMessage(QtWarningMsg, "QVariant::load: unable to load type "
                         + QByteArray::number(qMetaTypeId<MyClass>()) +".");
    char data[] = {0, QMetaType::User >> 16, char(QMetaType::User >> 8) , char(QMetaType::User), 0, 0, 0, 0, 8, 'M', 'y', 'C', 'l', 'a', 's', 's', 0};

    QByteArray ba(data, sizeof(data));
    QDataStream ds(&ba, QIODevice::ReadOnly);
    QVariant var;
    var.load(ds);
    QCOMPARE(ds.status(), QDataStream::ReadCorruptData);
}

void tst_QVariant::loadBrokenUserType()
{
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Trying to construct an instance of an invalid type"));
    char data[] = {0, 0, 0, 127, 0 };

    QByteArray ba(data, sizeof(data));
    QDataStream ds(&ba, QIODevice::ReadOnly);
    QVariant var;
    var.load(ds);
    QCOMPARE(ds.status(), QDataStream::Ok);
}

void tst_QVariant::invalidDate() const
{
    QString foo("Hello");
    QVariant variant(foo);
    QVERIFY(!variant.convert(QMetaType::fromType<QDate>()));

    variant = foo;
    QVERIFY(!variant.convert(QMetaType::fromType<QDateTime>()));

    variant = foo;
    QVERIFY(!variant.convert(QMetaType::fromType<QTime>()));

    variant = foo;
    QVERIFY(!variant.convert(QMetaType::fromType<int>()));

    variant = foo;
    QVERIFY(!variant.convert(QMetaType::fromType<double>()));

    variant = foo;
    QVERIFY(!variant.convert(QMetaType::fromType<float>()));
}

struct WontCompare
{
    int x;
};
Q_DECLARE_METATYPE(WontCompare);

struct WillCompare
{
    int x;

    friend bool operator==(const WillCompare &a, const WillCompare &b)
    { return a.x == b.x; }
    friend bool operator<(const WillCompare &a, const WillCompare &b)
    { return a.x < b.x; }
};
Q_DECLARE_METATYPE(WillCompare);

void tst_QVariant::compareCustomTypes_data() const
{
    QTest::addColumn<QVariant>("v1");
    QTest::addColumn<QVariant>("v2");
    QTest::addColumn<Qt::partial_ordering>("expectedOrdering");

    QTest::newRow("same_uncomparable")
            << QVariant::fromValue(WontCompare{0})
            << QVariant::fromValue(WontCompare{0})
            << Qt::partial_ordering::unordered;

    QTest::newRow("same_comparable")
            << QVariant::fromValue(WillCompare{0})
            << QVariant::fromValue(WillCompare{0})
            << Qt::partial_ordering::equivalent;

    QTest::newRow("different_comparable")
            << QVariant::fromValue(WillCompare{1})
            << QVariant::fromValue(WillCompare{0})
            << Qt::partial_ordering::greater;

    QTest::newRow("qdatetime_vs_comparable")
            << QVariant::fromValue(QDateTime::currentDateTimeUtc())
            << QVariant::fromValue(WillCompare{0})
            << Qt::partial_ordering::unordered;
}

void tst_QVariant::compareCustomTypes() const
{
    QFETCH(const QVariant, v1);
    QFETCH(const QVariant, v2);
    QFETCH(const Qt::partial_ordering, expectedOrdering);

    QCOMPARE(QVariant::compare(v1, v2), expectedOrdering);
    QT_TEST_EQUALITY_OPS(v1, v2, is_eq(expectedOrdering));
}
void tst_QVariant::timeToDateTime() const
{
    const QVariant val(QTime::currentTime());
    QVERIFY(!val.canConvert<QDateTime>());
    QVERIFY(!val.toDateTime().isValid());
}

struct CustomComparable
{
    CustomComparable(int value = 0) : myValue(value) {}
    int myValue;

    bool operator==(const CustomComparable &other) const
    { return other.myValue == myValue; }
};

Q_DECLARE_METATYPE(CustomComparable)

void tst_QVariant::copyingUserTypes() const
{
    QVariant var;
    QVariant varCopy;
    const CustomComparable userType = CustomComparable(42);
    var.setValue(userType);
    varCopy = var;

    const CustomComparable copiedType = qvariant_cast<CustomComparable>(varCopy);
    QCOMPARE(copiedType, userType);
    QCOMPARE(copiedType.myValue, 42);
}


struct NonQObjectBase {};
struct NonQObjectDerived : NonQObjectBase {};

void tst_QVariant::valueClassHierarchyConversion() const
{

   {
      // QVariant allows downcasting
      QScopedPointer<CustomQObjectDerived> derived {new CustomQObjectDerived};
      QVariant var = QVariant::fromValue(derived.get());
      CustomQObject *object = var.value<CustomQObject *>();
      QVERIFY(object);
   }
   {
      // QVariant supports upcasting to the correct dynamic type for QObjects
      QScopedPointer<CustomQObjectDerived> derived {new CustomQObjectDerived};
      QVariant var = QVariant::fromValue<CustomQObject *>(derived.get());
      CustomQObjectDerived *object = var.value<CustomQObjectDerived *>();
      QVERIFY(object);
   }
   {
      // QVariant forbids unsafe upcasting
      QScopedPointer<CustomQObject> base {new CustomQObject};
      QVariant var = QVariant::fromValue(base.get());
      CustomQObjectDerived *object = var.value<CustomQObjectDerived *>();
      QVERIFY(!object);
   }
   {
      // QVariant does not support upcastingfor non-QObjects
      QScopedPointer<NonQObjectDerived> derived {new NonQObjectDerived};
      QVariant var = QVariant::fromValue<NonQObjectBase *>(derived.get());
      NonQObjectDerived *object = var.value<NonQObjectDerived *>();
      QVERIFY(!object);
   }
}

void tst_QVariant::convertBoolToByteArray() const
{
    QFETCH(QByteArray, input);
    QFETCH(bool, canConvert);
    QFETCH(bool, value);

    const QVariant variant(input);

    QCOMPARE(variant.canConvert<bool>(), canConvert);

    if(canConvert) {
        /* Just call this function so we run the code path. */
        QCOMPARE(variant.toBool(), value);
    }
}

void tst_QVariant::convertBoolToByteArray_data() const
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<bool>("canConvert");
    QTest::addColumn<bool>("value");

    QTest::newRow("false")
        << QByteArray("false")
        << true
        << false;

    QTest::newRow("FALSE")
        << QByteArray("FALSE")
        << true
        << false;

    QTest::newRow("falSE")
        << QByteArray("FALSE")
        << true
        << false;

    QTest::newRow("")
        << QByteArray("")
        << true
        << false;

    QTest::newRow("null QByteArray")
        << QByteArray()
        << true
        << false;

    QTest::newRow("any-content")
        << QByteArray("any-content")
        << true
        << true;

    QTest::newRow("true")
        << QByteArray("true")
        << true
        << true;

    QTest::newRow("TRUE")
        << QByteArray("TRUE")
        << true
        << true;

    QTest::newRow("trUE")
        << QByteArray("trUE")
        << true
        << true;
}

void tst_QVariant::convertByteArrayToBool() const
{
    QFETCH(bool, input);
    QFETCH(QByteArray, output);

    const QVariant variant(input);
    QCOMPARE(variant.typeId(), QMetaType::Bool);
    QCOMPARE(variant.toBool(), input);
    QVERIFY(variant.canConvert<bool>());

    QCOMPARE(variant.toByteArray(), output);
}

void tst_QVariant::convertByteArrayToBool_data() const
{
    QTest::addColumn<bool>("input");
    QTest::addColumn<QByteArray>("output");

    QTest::newRow("false")
        << false
        << QByteArray("false");

    QTest::newRow("true")
        << true
        << QByteArray("true");
}

void tst_QVariant::convertIterables() const
{
    {
        QStringList list;
        list.append("Hello");
        QCOMPARE(QVariant::fromValue(list).value<QVariantList>().size(), list.size());
    }
    {
        QByteArrayList list;
        list.append("Hello");
        QCOMPARE(QVariant::fromValue(list).value<QVariantList>().size(), list.size());
    }
    {
        QVariantList list;
        list.append("World");
        QCOMPARE(QVariant::fromValue(list).value<QVariantList>().size(), list.size());
    }
    {
        QMap<QString, int> map;
        map.insert("3", 4);
        QCOMPARE(QVariant::fromValue(map).value<QVariantHash>().size(), map.size());
        QCOMPARE(QVariant::fromValue(map).value<QVariantMap>().size(), map.size());

        map.insert("4", 5);
        QCOMPARE(QVariant::fromValue(map).value<QVariantHash>().size(), map.size());
        QCOMPARE(QVariant::fromValue(map).value<QVariantMap>().size(), map.size());
    }
    {
        QVariantMap map;
        map.insert("3", 4);
        QCOMPARE(QVariant::fromValue(map).value<QVariantHash>().size(), map.size());
        QCOMPARE(QVariant::fromValue(map).value<QVariantMap>().size(), map.size());

        map.insert("4", 5);
        QCOMPARE(QVariant::fromValue(map).value<QVariantHash>().size(), map.size());
        QCOMPARE(QVariant::fromValue(map).value<QVariantMap>().size(), map.size());
    }
    {
        QHash<QString, int> hash;
        hash.insert("3", 4);
        QCOMPARE(QVariant::fromValue(hash).value<QVariantHash>().size(), hash.size());
        QCOMPARE(QVariant::fromValue(hash).value<QVariantMap>().size(), hash.size());

        hash.insert("4", 5);
        QCOMPARE(QVariant::fromValue(hash).value<QVariantHash>().size(), hash.size());
        QCOMPARE(QVariant::fromValue(hash).value<QVariantMap>().size(), hash.size());
    }
    {
        QVariantHash hash;
        hash.insert("3", 4);
        QCOMPARE(QVariant::fromValue(hash).value<QVariantHash>().size(), hash.size());
        QCOMPARE(QVariant::fromValue(hash).value<QVariantMap>().size(), hash.size());

        hash.insert("4", 5);
        QCOMPARE(QVariant::fromValue(hash).value<QVariantHash>().size(), hash.size());
        QCOMPARE(QVariant::fromValue(hash).value<QVariantMap>().size(), hash.size());
    }
}

struct Derived : QObject
{
    Q_OBJECT
};

void tst_QVariant::convertConstNonConst() const
{
    Derived *derived = new Derived;
    QObject *obj = derived;
    QObject const *unrelatedConstObj = new QObject;
    auto cleanUp = qScopeGuard([&] {
        delete unrelatedConstObj;
        delete derived;
    });
    QObject const *constObj = obj;
    Derived const *constDerived = derived;
    QCOMPARE(QVariant::fromValue(constObj).value<QObject *>(), obj);
    QCOMPARE(QVariant::fromValue(obj).value<QObject const *>(), constObj);

    QCOMPARE(QVariant::fromValue(constDerived).value<QObject *>(), derived);
    QCOMPARE(QVariant::fromValue(derived).value<QObject const *>(), derived);

    QObject const *derivedAsConstObject = derived;
    // up cast and remove const is possible, albeit dangerous
    QCOMPARE(QVariant::fromValue(derivedAsConstObject).value<Derived *>(), derived);
    QCOMPARE(QVariant::fromValue(unrelatedConstObj).value<Derived *>(), nullptr);
}

/*!
  We verify that:
    1. Converting the string "9.9" to int fails. This is the behavior of
       toLongLong() and hence also QVariant, since it uses it.
    2. Converting the QVariant containing the double 9.9 to int works.

  Rationale: "9.9" is not a valid int. However, doubles are by definition not
  ints and therefore it makes more sense to perform conversion for those.
*/
void tst_QVariant::toIntFromQString() const
{
    QVariant first("9.9");
    bool ok;
    QCOMPARE(first.toInt(&ok), 0);
    QVERIFY(!ok);

    QCOMPARE(QString("9.9").toLongLong(&ok), qlonglong(0));
    QVERIFY(!ok);

    QVariant v(9.9);
    QCOMPARE(v.toInt(&ok), 10);
    QVERIFY(ok);
}

/*!
  We verify that:
    1. Conversion from (64 bit) double to int works (no overflow).
    2. Same conversion works for QVariant::convert.

  Rationale: if 2147483630 is set in float and then converted to int,
  there will be overflow and the result will be -2147483648.
*/
void tst_QVariant::toIntFromDouble() const
{
    double d = 2147483630;  // max int 2147483647
    QCOMPARE((int)d, 2147483630);

    QVariant var(d);
    QVERIFY(var.canConvert<int>());

    bool ok;
    int result = var.toInt(&ok);

    QVERIFY( ok == true );
    QCOMPARE(result, 2147483630);
}

void tst_QVariant::fpStringRoundtrip_data() const
{
    QTest::addColumn<QVariant>("number");

    QTest::newRow("float") << QVariant(1 + FLT_EPSILON);
    QTest::newRow("double") << QVariant(1 + DBL_EPSILON);
}

void tst_QVariant::fpStringRoundtrip() const
{
    QFETCH(QVariant, number);

    QVariant converted = number;
    QVERIFY(converted.convert(QMetaType::fromType<QString>()));
    QVERIFY(converted.convert(QMetaType(number.typeId())));
    QCOMPARE(converted, number);

    converted = number;
    QVERIFY(converted.convert(QMetaType::fromType<QByteArray>()));
    QVERIFY(converted.convert(QMetaType(number.typeId())));
    QCOMPARE(converted, number);
}

void tst_QVariant::numericalConvert_data()
{
    QTest::addColumn<QVariant>("v");
    QTest::addColumn<bool>("isInteger");
    QTest::newRow("float") << QVariant(float(5.3)) << false;
    QTest::newRow("double") << QVariant(double(5.3)) << false;
    QTest::newRow("qreal") << QVariant(qreal(5.3)) << false;
    QTest::newRow("int") << QVariant(int(5)) << true;
    QTest::newRow("uint") << QVariant(uint(5)) << true;
    QTest::newRow("short") << QVariant(short(5)) << true;
    QTest::newRow("longlong") << QVariant(quint64(5)) << true;
    QTest::newRow("long") << QVariant::fromValue(long(5)) << true;
    QTest::newRow("stringint") << QVariant(QString::fromLatin1("5")) << true;
    QTest::newRow("string") << QVariant(QString::fromLatin1("5.30000019")) << false;
}

void tst_QVariant::numericalConvert()
{
    QFETCH(QVariant, v);
    QFETCH(bool, isInteger);
    double num = isInteger ? 5 : 5.3;

    QCOMPARE(v.toFloat() , float(num));
    QCOMPARE(float(v.toReal()) , float(num));
    QCOMPARE(float(v.toDouble()) , float(num));
    if (isInteger) {
        QCOMPARE(v.toInt() , int(num));
        QCOMPARE(v.toUInt() , uint(num));
        QCOMPARE(v.toULongLong() , quint64(num));
        QCOMPARE(v.value<ulong>() , ulong(num));
        QCOMPARE(v.value<ushort>() , ushort(num));
    }
    switch (v.userType())
    {
    case QMetaType::Double:
        QCOMPARE(v.toString() , QString::number(num, 'g', QLocale::FloatingPointShortest));
        break;
    case QMetaType::Float:
        QCOMPARE(v.toString() ,
                 QString::number(float(num), 'g', QLocale::FloatingPointShortest));
        break;
    }
}


template<class T> void playWithVariant(const T &orig, bool isNull, const QString &toString, double toDouble, bool toBool)
{
    QVariant v = QVariant::fromValue(orig);
    QVERIFY(v.isValid());
    QCOMPARE(v.isNull(), isNull);
    QCOMPARE(v.toString(), toString);
    QCOMPARE(v.toDouble(), toDouble);
    QCOMPARE(v.toBool(), toBool);
    QCOMPARE(qvariant_cast<T>(v), orig);

    {
        QVariant v2 = v;
        if (QTypeInfo<T>::isRelocatable) {
            // Type is movable so standard comparison algorithm in QVariant should work
            // In a custom type QVariant is not aware of ==operator so it won't be called,
            // which may cause problems especially visible when using a not-movable type
            QCOMPARE(v2, v);
        }
        QVERIFY(v2.isValid());
        QCOMPARE(v2.isNull(), isNull);
        QCOMPARE(v2.toString(), toString);
        QCOMPARE(v2.toDouble(), toDouble);
        QCOMPARE(v2.toBool(), toBool);
        QCOMPARE(qvariant_cast<T>(v2), orig);

        QVariant v3;
        v = QVariant();
        QCOMPARE(v3, v);
        v = v2;
        if (QTypeInfo<T>::isRelocatable) {
            // Type is movable so standard comparison algorithm in QVariant should work
            // In a custom type QVariant is not aware of ==operator so it won't be called,
            // which may cause problems especially visible when using a not-movable type
            QCOMPARE(v2, v);
        }
        QCOMPARE(qvariant_cast<T>(v2), qvariant_cast<T>(v));
        QCOMPARE(v2.toString(), toString);
        v3 = QVariant::fromValue(orig);

        QVERIFY(v3.isValid());
        QCOMPARE(v3.isNull(), isNull);
        QCOMPARE(v3.toString(), toString);
        QCOMPARE(v3.toDouble(), toDouble);
        QCOMPARE(v3.toBool(), toBool);
        QCOMPARE(qvariant_cast<T>(v3), qvariant_cast<T>(v));
    }

    QVERIFY(v.isValid());
    QCOMPARE(v.isNull(), isNull);
    QCOMPARE(v.toString(), toString);
    QCOMPARE(v.toDouble(), toDouble);
    QCOMPARE(v.toBool(), toBool);
    QCOMPARE(qvariant_cast<T>(v), orig);

    if (qMetaTypeId<T>() != qMetaTypeId<QVariant>())
        QCOMPARE(v.userType(), qMetaTypeId<T>());
}

#define PLAY_WITH_VARIANT(Orig, IsNull, ToString, ToDouble, ToBool) \
    playWithVariant(Orig, IsNull, ToString, ToDouble, ToBool);\
    if (QTest::currentTestFailed())\
        QFAIL("playWithVariant failed");

struct MyPrimitive
{
    char x, y;
    bool operator==(const MyPrimitive &o) const
    {
        return x == o.x && y == o.y;
    }
};

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(MyPrimitive, Q_PRIMITIVE_TYPE);
QT_END_NAMESPACE

struct MyData
{
    void *ptr;
    MyData() : ptr(this) {}
    ~MyData()
    {
        if (ptr != this) qWarning("%s: object has moved", Q_FUNC_INFO);
    }
    MyData(const MyData& o) : ptr(this)
    {
        if (o.ptr != &o) qWarning("%s: other object has moved", Q_FUNC_INFO);
    }
    MyData &operator=(const MyData &o)
    {
        if (ptr != this) qWarning("%s: object has moved", Q_FUNC_INFO);
        if (o.ptr != &o) qWarning("%s: other object has moved", Q_FUNC_INFO);
        return *this;
    }
    bool operator==(const MyData &o) const
    {
        if (ptr != this) qWarning("%s: object has moved", Q_FUNC_INFO);
        if (o.ptr != &o) qWarning("%s: other object has moved", Q_FUNC_INFO);
        return true;
    }
};

struct MyMovable
{
    static int count;
    int v;
    MyMovable() { v = count++; }
    ~MyMovable() { count--; }
    MyMovable(const MyMovable &o) : v(o.v) { count++; }
    MyMovable &operator=(const MyMovable &o) { v = o.v; return *this; }

    bool operator==(const MyMovable &o) const
    {
        return v == o.v;
    }
};

int MyMovable::count  = 0;

struct MyNotMovable
{
    static int count;
    MyNotMovable *that;
    MyNotMovable() : that(this) { count++; }
    ~MyNotMovable() { QCOMPARE(that, this);  count--; }
    MyNotMovable(const MyNotMovable &o) : that(this) { QCOMPARE(o.that, &o); count++; }
    MyNotMovable &operator=(const MyNotMovable &o) {
        bool ok = that == this && o.that == &o;
        if (!ok) qFatal("MyNotMovable has been moved");
        return *this;
    }

    //PLAY_WITH_VARIANT test that they are equal, but never that they are not equal
    // so it would be fine just to always return true
    bool operator==(const MyNotMovable &o) const
    {
        bool ok = that == this && o.that == &o;
        if (!ok) qFatal("MyNotMovable has been moved");
        return ok;
    }
    // Make it too big to store it in the variant itself
    void *dummy[4];
};

int MyNotMovable::count  = 0;

struct MyShared : QSharedData {
    MyMovable movable;
};

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(MyMovable, Q_RELOCATABLE_TYPE);
QT_END_NAMESPACE

Q_DECLARE_METATYPE(MyPrimitive)
Q_DECLARE_METATYPE(MyData)
Q_DECLARE_METATYPE(MyMovable)
Q_DECLARE_METATYPE(MyNotMovable)
Q_DECLARE_METATYPE(MyPrimitive *)
Q_DECLARE_METATYPE(MyData *)
Q_DECLARE_METATYPE(MyMovable *)
Q_DECLARE_METATYPE(MyNotMovable *)
Q_DECLARE_METATYPE(QSharedDataPointer<MyShared>)

void tst_QVariant::setValue()
{
    MyNotMovable t; //we just take a value so that we're sure that it will be shared
    QVariant v1 = QVariant::fromValue(t);
    QVERIFY( v1.isDetached() );
    QVariant v2 = v1;
    QVERIFY( !v1.isDetached() );
    QVERIFY( !v2.isDetached() );

    v2.setValue(3); //set an integer value

    QVERIFY( v1.isDetached() );
    QVERIFY( v2.isDetached() );
}

void tst_QVariant::moreCustomTypes()
{
    {
        QList<QSize> listSize;
        PLAY_WITH_VARIANT(listSize, false, QString(), 0, false);
        listSize << QSize(4,5) << QSize(89,23) << QSize(5,6);
        PLAY_WITH_VARIANT(listSize, false, QString(), 0, false);
    }

    {
        QString str;
        PLAY_WITH_VARIANT(str, false, QString(), 0, false);
        str = QString::fromLatin1("123456789.123");
        PLAY_WITH_VARIANT(str, false, str, 123456789.123, true);
    }

    {
        QSize size;
        PLAY_WITH_VARIANT(size, false, QString(), 0, false);
        PLAY_WITH_VARIANT(QSize(45,78), false, QString(), 0, false);
    }

    {
        MyData d;
        PLAY_WITH_VARIANT(d, false, QString(), 0, false);
        PLAY_WITH_VARIANT(&d, false, QString(), 0, false);
        QList<MyData> l;
        PLAY_WITH_VARIANT(l, false, QString(), 0, false);
        l << MyData() << MyData();
        PLAY_WITH_VARIANT(l, false, QString(), 0, false);
    }

    {
        MyPrimitive d = { 4, 5 };
        PLAY_WITH_VARIANT(d, false, QString(), 0, false);
        PLAY_WITH_VARIANT(&d, false, QString(), 0, false);
        QList<MyPrimitive> l;
        PLAY_WITH_VARIANT(l, false, QString(), 0, false);
        l << d;
        PLAY_WITH_VARIANT(l, false, QString(), 0, false);
    }

    {
        MyMovable d;
        PLAY_WITH_VARIANT(d, false, QString(), 0, false);
        PLAY_WITH_VARIANT(&d, false, QString(), 0, false);
        QList<MyMovable> l;
        PLAY_WITH_VARIANT(l, false, QString(), 0, false);
        l << MyMovable() << d;
        PLAY_WITH_VARIANT(l, false, QString(), 0, false);
    }
    QCOMPARE(MyMovable::count, 0);

    QCOMPARE(MyNotMovable::count, 0);
    {
        MyNotMovable d;
        PLAY_WITH_VARIANT(d, false, QString(), 0, false);
        PLAY_WITH_VARIANT(&d, false, QString(), 0, false);
        QList<MyNotMovable> l;
        PLAY_WITH_VARIANT(l, false, QString(), 0, false);
        l << MyNotMovable() << d;
        PLAY_WITH_VARIANT(l, false, QString(), 0, false);
    }
    QCOMPARE(MyNotMovable::count, 0);

    {
#ifdef QT_NO_DOUBLECONVERSION
        // snprintf cannot do "shortest" conversion and always adds noise.
        PLAY_WITH_VARIANT(12.12, false, "12.119999999999999", 12.12, true);
#else
        // Double can be printed exactly with libdouble-conversion
        PLAY_WITH_VARIANT(12.12, false, "12.12", 12.12, true);
#endif

        // Float is converted to double, adding insignificant bits
        PLAY_WITH_VARIANT(12.12f, false, "12.119999885559082", 12.12f, true);

        PLAY_WITH_VARIANT(quint16(14), false, "14", 14, true);
        PLAY_WITH_VARIANT( qint16(15), false, "15", 15, true);
        PLAY_WITH_VARIANT(quint32(16), false, "16", 16, true);
        PLAY_WITH_VARIANT( qint32(17), false, "17", 17, true);
        PLAY_WITH_VARIANT(quint64(18), false, "18", 18, true);
        PLAY_WITH_VARIANT( qint64(19), false, "19", 19, true);
        PLAY_WITH_VARIANT( qint16(-13), false, "-13", -13, true);
        PLAY_WITH_VARIANT( qint32(-14), false, "-14", -14, true);
        PLAY_WITH_VARIANT( qint64(-15), false, "-15", -15, true);
        PLAY_WITH_VARIANT(quint64(0), false, "0", 0, false);
        PLAY_WITH_VARIANT( true, false, "true", 1, true);
        PLAY_WITH_VARIANT( false, false, "false", 0, false);

        PLAY_WITH_VARIANT(QString("hello\n"), false, "hello\n", 0, true);
    }

    {
        int i = 5;
        PLAY_WITH_VARIANT((void *)(&i), false, QString(), 0, false);
        PLAY_WITH_VARIANT((void *)(0), true, QString(), 0, false);
    }

    {
        QVariant v1 = QVariant::fromValue(5);
        QVariant v2 = QVariant::fromValue(5.0);
        QVariant v3 = QVariant::fromValue(quint16(5));
        QVariant v4 = 5;
        QVariant v5 = QVariant::fromValue(MyPrimitive());
        QVariant v6 = QVariant::fromValue(MyMovable());
        QVariant v7 = QVariant::fromValue(MyData());
        PLAY_WITH_VARIANT(v1, false, "5", 5, true);
        PLAY_WITH_VARIANT(v2, false, "5", 5, true);
        PLAY_WITH_VARIANT(v3, false, "5", 5, true);
        PLAY_WITH_VARIANT(v4, false, "5", 5, true);

        PLAY_WITH_VARIANT(v5, false, QString(), 0, false);
    }

    QCOMPARE(MyMovable::count, 0);
    {
        QSharedDataPointer<MyShared> d(new MyShared);
        PLAY_WITH_VARIANT(d, false, QString(), 0, false);
    }
    QCOMPARE(MyMovable::count, 0);

    {
        QList<QList<int> > data;
        PLAY_WITH_VARIANT(data, false, QString(), 0, false);
        data << (QList<int>() << 42);
        PLAY_WITH_VARIANT(data, false, QString(), 0, false);
    }

    {
        QList<QList<int> > data;
        PLAY_WITH_VARIANT(data, false, QString(), 0, false);
        data << (QList<int>() << 42);
        PLAY_WITH_VARIANT(data, false, QString(), 0, false);
    }

    {
        QList<QSet<int> > data;
        PLAY_WITH_VARIANT(data, false, QString(), 0, false);
        data << (QSet<int>() << 42);
        PLAY_WITH_VARIANT(data, false, QString(), 0, false);
    }
}

void tst_QVariant::movabilityTest()
{
    // This test checks if QVariant is movable even if an internal data is not movable.
    QVERIFY(!MyNotMovable::count);
    {
        QVariant variant = QVariant::fromValue(MyNotMovable());
        QVERIFY(MyNotMovable::count);

        // prepare destination memory space to which variant will be moved
        QVariant buffer[1];
        QCOMPARE(buffer[0].typeId(), QMetaType::UnknownType);
        buffer[0].~QVariant();

        memcpy(static_cast<void *>(buffer), static_cast<void *>(&variant), sizeof(QVariant));
        QVERIFY(buffer[0].typeId() > QMetaType::User);
        QCOMPARE(buffer[0].userType(), qMetaTypeId<MyNotMovable>());
        MyNotMovable tmp(buffer[0].value<MyNotMovable>());

        new (&variant) QVariant();
    }
    QVERIFY(!MyNotMovable::count);
}

void tst_QVariant::variantInVariant()
{
    QVariant var1 = 5;
    QCOMPARE(var1.typeId(), QMetaType::Int);
    QVariant var2 = var1;
    QCOMPARE(var2, var1);
    QCOMPARE(var2.typeId(), QMetaType::Int);
    QVariant var3 = QVariant::fromValue(var1);
    QCOMPARE(var3, var1);
    QCOMPARE(var3.typeId(), QMetaType::Int);
    QVariant var4 = qvariant_cast<QVariant>(var1);
    QCOMPARE(var4, var1);
    QCOMPARE(var4.typeId(), QMetaType::Int);
    QVariant var5;
    var5 = var1;
    QCOMPARE(var5, var1);
    QCOMPARE(var5.typeId(), QMetaType::Int);
    QVariant var6;
    var6.setValue(var1);
    QCOMPARE(var6, var1);
    QCOMPARE(var6.typeId(), QMetaType::Int);

    QCOMPARE(QVariant::fromValue(var1), QVariant::fromValue(var2));
    QCOMPARE(qvariant_cast<QVariant>(var3), QVariant::fromValue(var4));
    QCOMPARE(qvariant_cast<QVariant>(var5), qvariant_cast<QVariant>(var6));

    QString str("hello");
    QVariant var8 = qvariant_cast<QVariant>(QVariant::fromValue(QVariant::fromValue(str)));
    QCOMPARE(var8.typeId(), QMetaType::QString);
    QCOMPARE(qvariant_cast<QString>(QVariant(qvariant_cast<QVariant>(var8))), str);

    QVariant var9(QMetaType::fromType<QVariant>(), &var1);
    QCOMPARE(var9.userType(), qMetaTypeId<QVariant>());
    QCOMPARE(qvariant_cast<QVariant>(var9), var1);
}

struct Convertible {
    double d;
    operator int() const { return (int)d; }
    operator double() const { return d; }
    operator QString() const { return QString::number(d); }
};

Q_DECLARE_METATYPE(Convertible);

struct BigConvertible {
    double d;
    double dummy[sizeof(QVariant) / sizeof(double)];
    operator int() const { return (int)d; }
    operator double() const { return d; }
    operator QString() const { return QString::number(d); }
};

Q_DECLARE_METATYPE(BigConvertible);
static_assert(sizeof(BigConvertible) > sizeof(QVariant));

void tst_QVariant::userConversion()
{
    {
        QVERIFY(!(QMetaType::hasRegisteredConverterFunction<int, Convertible>()));
        QVERIFY(!(QMetaType::hasRegisteredConverterFunction<double, Convertible>()));
        QVERIFY(!(QMetaType::hasRegisteredConverterFunction<QString, Convertible>()));

        Convertible c = { 123 };
        QVariant v = QVariant::fromValue(c);

        bool ok;
        v.toInt(&ok);
        QVERIFY(!ok);

        v.toDouble(&ok);
        QVERIFY(!ok);

        QString s = v.toString();
        QVERIFY(s.isEmpty());

        QMetaType::registerConverter<Convertible, int>();
        QMetaType::registerConverter<Convertible, double>();
        QMetaType::registerConverter<Convertible, QString>();

        int i = v.toInt(&ok);
        QVERIFY(ok);
        QCOMPARE(i, 123);

        double d = v.toDouble(&ok);
        QVERIFY(ok);
        QCOMPARE(d, 123.);

        s = v.toString();
        QCOMPARE(s, QString::fromLatin1("123"));
    }

    {
        QVERIFY(!(QMetaType::hasRegisteredConverterFunction<int, BigConvertible>()));
        QVERIFY(!(QMetaType::hasRegisteredConverterFunction<double, BigConvertible>()));
        QVERIFY(!(QMetaType::hasRegisteredConverterFunction<QString, BigConvertible>()));

        BigConvertible c = { 123, { 0, 0 } };
        QVariant v = QVariant::fromValue(c);

        bool ok;
        v.toInt(&ok);
        QVERIFY(!ok);

        v.toDouble(&ok);
        QVERIFY(!ok);

        QString s = v.toString();
        QVERIFY(s.isEmpty());

        QMetaType::registerConverter<BigConvertible, int>();
        QMetaType::registerConverter<BigConvertible, double>();
        QMetaType::registerConverter<BigConvertible, QString>();

        int i = v.toInt(&ok);
        QVERIFY(ok);
        QCOMPARE(i, 123);

        double d = v.toDouble(&ok);
        QVERIFY(ok);
        QCOMPARE(d, 123.);

        s = v.toString();
        QCOMPARE(s, QString::fromLatin1("123"));
    }
}

void tst_QVariant::modelIndexConversion()
{
    QVariant modelIndexVariant = QModelIndex();
    QVERIFY(modelIndexVariant.canConvert<QPersistentModelIndex>());
    QVERIFY(modelIndexVariant.convert(QMetaType::fromType<QPersistentModelIndex>()));
    QCOMPARE(modelIndexVariant.typeId(), QMetaType::QPersistentModelIndex);
    QVERIFY(modelIndexVariant.canConvert(QMetaType::fromType<QModelIndex>()));
    QVERIFY(modelIndexVariant.convert(QMetaType::fromType<QModelIndex>()));
    QCOMPARE(modelIndexVariant.typeId(), QMetaType::QModelIndex);
}

class Forward;
Q_DECLARE_OPAQUE_POINTER(Forward*)
Q_DECLARE_METATYPE(Forward*)

void tst_QVariant::forwardDeclare()
{
    Forward *f = nullptr;
    QVariant v = QVariant::fromValue(f);
    QCOMPARE(qvariant_cast<Forward*>(v), f);
}

void tst_QVariant::loadQt5Stream_data()
{
    dataStream_data(QDataStream::Qt_5_0);
}

void tst_QVariant::loadQt5Stream()
{
    loadQVariantFromDataStream(QDataStream::Qt_5_0);
}

void tst_QVariant::saveQt5Stream_data()
{
    dataStream_data(QDataStream::Qt_5_0);
}

void tst_QVariant::saveQt5Stream()
{
    saveQVariantFromDataStream(QDataStream::Qt_5_0);
}

void tst_QVariant::loadQt4Stream_data()
{
    dataStream_data(QDataStream::Qt_4_9);
}

void tst_QVariant::loadQt4Stream()
{
    loadQVariantFromDataStream(QDataStream::Qt_4_9);
}

void tst_QVariant::saveQt4Stream_data()
{
    dataStream_data(QDataStream::Qt_4_9);
}

void tst_QVariant::saveQt4Stream()
{
    saveQVariantFromDataStream(QDataStream::Qt_4_9);
}

void tst_QVariant::dataStream_data(QDataStream::Version version)
{
    QTest::addColumn<QString>("fileName");

    QString path;
    switch (version) {
    case QDataStream::Qt_4_9:
        path = QString::fromLatin1("qt4.9");
        break;
    case QDataStream::Qt_5_0:
        path = QString::fromLatin1("qt5.0");
        break;
    default:
        Q_UNIMPLEMENTED();
    }

    path = path.prepend(":/stream/").append("/");
    QDir dir(path);
    uint i = 0;
    const auto entries = dir.entryInfoList(QStringList{u"*.bin"_s});
    for (const QFileInfo &fileInfo : entries) {
        QTest::newRow((path + fileInfo.fileName()).toLatin1()) << fileInfo.filePath();
        i += 1;
    }
    QVERIFY(i > 10);
}

void tst_QVariant::loadQVariantFromDataStream(QDataStream::Version version)
{
    QFETCH(QString, fileName);

    QFile file(fileName);
    QVERIFY(file.open(QIODevice::ReadOnly));

    QDataStream stream(&file);
    stream.setVersion(version);

    QString typeName;
    QVariant loadedVariant;
    stream >> typeName >> loadedVariant;

    const int id = QMetaType::fromName(typeName.toLatin1()).id();
    if (id == QMetaType::Void) {
        // Void type is not supported by QVariant
        return;
    }

    QVariant constructedVariant {QMetaType(id)};
    QCOMPARE(constructedVariant.userType(), id);
    QCOMPARE(QMetaType(loadedVariant.userType()).name(), typeName.toLatin1().constData());
    QCOMPARE(loadedVariant.userType(), constructedVariant.userType());
}

void tst_QVariant::saveQVariantFromDataStream(QDataStream::Version version)
{
    QFETCH(QString, fileName);

    QFile file(fileName);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QDataStream dataFileStream(&file);

    QString typeName;
    dataFileStream >> typeName;
    QByteArray data = file.readAll();
    const int id = QMetaType::fromName(typeName.toLatin1()).id();
    if (id == QMetaType::Void) {
        // Void type is not supported by QVariant
        return;
    }

    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QDataStream stream(&buffer);
    stream.setVersion(version);

    QVariant constructedVariant {QMetaType(id)};
    QCOMPARE(constructedVariant.userType(), id);
    stream << constructedVariant;

    // We are testing QVariant there is no point in testing full array.
    QCOMPARE(buffer.data().left(5), data.left(5));

    buffer.seek(0);
    QVariant recunstructedVariant;
    stream >> recunstructedVariant;
    QCOMPARE(recunstructedVariant.userType(), constructedVariant.userType());
}

void tst_QVariant::debugStream_data()
{
    QTest::addColumn<QVariant>("variant");
    QTest::addColumn<int>("typeId");
    for (int id = 0; id < QMetaType::LastCoreType + 1; ++id) {
        if (id && !QMetaType::isRegistered(id)) {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
                                     "^Trying to construct an instance of an invalid type"));
        }
        const char *tagName = QMetaType(id).name();
        if (tagName && id != QMetaType::Void)
            QTest::newRow(tagName) << QVariant(QMetaType(id)) << id;
    }
    QTest::newRow("QBitArray(111)") << QVariant(QBitArray(3, true)) << qMetaTypeId<QBitArray>();
    QTest::newRow("CustomStreamableClass") << QVariant(QMetaType::fromType<CustomStreamableClass>(), 0) << qMetaTypeId<CustomStreamableClass>();
    QTest::newRow("MyClass") << QVariant(QMetaType::fromType<MyClass>(), 0) << qMetaTypeId<MyClass>();
    QTest::newRow("InvalidVariant") << QVariant() << int(QMetaType::UnknownType);
    QTest::newRow("CustomQObject") << QVariant::fromValue(this) << qMetaTypeId<tst_QVariant*>();
}

void tst_QVariant::debugStream()
{
    QFETCH(QVariant, variant);
    QFETCH(int, typeId);

    MessageHandler msgHandler(typeId);
    qDebug() << variant;
    QVERIFY(msgHandler.testPassed());
}

#if QT_DEPRECATED_SINCE(6, 0)
struct MessageHandlerType : public MessageHandler
{
    MessageHandlerType(const int typeId)
        : MessageHandler(typeId, handler)
    {}
    static void handler(QtMsgType, const QMessageLogContext &, const QString &msg)
    {
        // Format itself is not important, but basic data as a type name should be included in the output
        ok = msg.startsWith("QVariant::");
        QVERIFY2(ok, (QString::fromLatin1("Message is not started correctly: '") + msg + '\'').toLatin1().constData());
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
        ok &= (currentId == QMetaType::UnknownType
                ? msg.contains("Invalid")
                : msg.contains(QMetaType::typeName(currentId)));
QT_WARNING_POP
        QVERIFY2(ok, (QString::fromLatin1("Message doesn't contain type name: '") + msg + '\'').toLatin1().constData());
    }
};

void tst_QVariant::debugStreamType_data()
{
    debugStream_data();
}

void tst_QVariant::debugStreamType()
{
    QFETCH(QVariant, variant);
    QFETCH(int, typeId);

    MessageHandlerType msgHandler(typeId);
    QT_IGNORE_DEPRECATIONS(qDebug() << QVariant::Type(typeId);)
    QVERIFY(msgHandler.testPassed());
}
#endif // QT_DEPRECATED_SINCE(6, 0)

void tst_QVariant::implicitConstruction()
{
    // This is a compile-time test
    QVariant v;

#define FOR_EACH_CORE_CLASS(F) \
    F(Char) \
    F(String) \
    F(StringList) \
    F(ByteArray) \
    F(BitArray) \
    F(Date) \
    F(Time) \
    F(DateTime) \
    F(Url) \
    F(Locale) \
    F(Rect) \
    F(RectF) \
    F(Size) \
    F(SizeF) \
    F(Line) \
    F(LineF) \
    F(Point) \
    F(PointF) \
    F(EasingCurve) \
    F(Uuid) \
    F(ModelIndex) \
    F(PersistentModelIndex) \
    F(RegularExpression) \
    F(JsonValue) \
    F(JsonObject) \
    F(JsonArray) \
    F(JsonDocument) \

#define CONSTRUCT(TYPE) \
    { \
        Q##TYPE t; \
        v = t; \
        t = v.to##TYPE(); \
        QVERIFY(true); \
    }

    FOR_EACH_CORE_CLASS(CONSTRUCT)

#undef CONSTRUCT
#undef FOR_EACH_CORE_CLASS
}

void tst_QVariant::saveInvalid_data()
{
    QTest::addColumn<unsigned>("version");
    for (unsigned version = QDataStream::Qt_5_0; version > QDataStream::Qt_1_0; --version)
        QTest::newRow(QString::number(version).toUtf8()) << version;
}

void tst_QVariant::saveInvalid()
{
    QFETCH(unsigned, version);

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(version);
    stream << QVariant();
    QCOMPARE(stream.status(), QDataStream::Ok);
    QVERIFY(data.size() >= 4);
    QCOMPARE(int(data.constData()[0]), 0);
    QCOMPARE(int(data.constData()[1]), 0);
    QCOMPARE(int(data.constData()[2]), 0);
    QCOMPARE(int(data.constData()[3]), 0);
}

void tst_QVariant::saveNewBuiltinWithOldStream()
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_3_1);
    stream << QVariant::fromValue<QJsonValue>(123); // QJsonValue class was introduced in Qt5
    QCOMPARE(stream.status(), QDataStream::Ok);
    QVERIFY(data.size() >= 4);
    QCOMPARE(int(data.constData()[0]), 0);
    QCOMPARE(int(data.constData()[1]), 0);
    QCOMPARE(int(data.constData()[2]), 0);
    QCOMPARE(int(data.constData()[3]), 0);
}

template<typename Container, typename Value_Type = typename Container::value_type>
struct ContainerAPI
{
    static void insert(Container &container, typename Container::value_type value)
    {
        container.push_back(value);
    }

    static bool compare(const QVariant &variant, typename Container::value_type value)
    {
        return variant.value<typename Container::value_type>() == value;
    }
    static bool compare(QVariant variant, const QVariant &value)
    {
        return variant == value;
    }
};

template<typename Container>
struct ContainerAPI<Container, QVariant>
{
    static void insert(Container &container, int value)
    {
        container.push_back(QVariant::fromValue(value));
    }

    static bool compare(QVariant variant, const QVariant &value)
    {
        return variant == value;
    }
};

template<typename Container>
struct ContainerAPI<Container, QString>
{
    static void insert(Container &container, int value)
    {
        container.push_back(QString::number(value));
    }

    static bool compare(const QVariant &variant, QString value)
    {
        return variant.value<QString>() == value;
    }
    static bool compare(QVariant variant, const QVariant &value)
    {
        return variant == value;
    }
};

template<typename Container>
struct ContainerAPI<Container, QByteArray>
{
    static void insert(Container &container, int value)
    {
        container.push_back(QByteArray::number(value));
    }

    static bool compare(const QVariant &variant, QByteArray value)
    {
        return variant.value<QByteArray>() == value;
    }
    static bool compare(QVariant variant, const QVariant &value)
    {
        return variant == value;
    }
};

template<typename Container>
struct ContainerAPI<Container, QChar>
{
    static void insert(Container &container, int value)
    {
        container.push_back(QChar::fromLatin1(char(value) + '0'));
    }

    static bool compare(const QVariant &variant, QChar value)
    {
        return variant.value<QChar>() == value;
    }
    static bool compare(QVariant variant, const QVariant &value)
    {
        return variant == value;
    }
};

template<typename Container>
struct ContainerAPI<Container, char>
{
    static void insert(Container &container, int value)
    {
        container.push_back(char(value) + '0');
    }

    static bool compare(const QVariant &variant, char value)
    {
        return variant.value<char>() == value;
    }
    static bool compare(QVariant variant, const QVariant &value)
    {
        return variant == value;
    }
};

#ifdef __has_include
# if __has_include(<forward_list>)
# define TEST_FORWARD_LIST
# include <forward_list>

Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE(std::forward_list)

// Test that explicit declaration does not degrade features.
Q_DECLARE_METATYPE(std::forward_list<int>)

template<typename Value_Type>
struct ContainerAPI<std::forward_list<Value_Type> >
{
    static void insert(std::forward_list<Value_Type> &container, Value_Type value)
    {
        container.push_front(value);
    }
    static bool compare(const QVariant &variant, Value_Type value)
    {
        return variant.value<Value_Type>() == value;
    }
    static bool compare(QVariant variant, const QVariant &value)
    {
        return variant == value;
    }
};

template<>
struct ContainerAPI<std::forward_list<QVariant> >
{
    static void insert(std::forward_list<QVariant> &container, int value)
    {
        container.push_front(QVariant::fromValue(value));
    }

    static bool compare(QVariant variant, const QVariant &value)
    {
        return variant == value;
    }
};

template<>
struct ContainerAPI<std::forward_list<QString> >
{
    static void insert(std::forward_list<QString> &container, int value)
    {
        container.push_front(QString::number(value));
    }
    static bool compare(const QVariant &variant, QString value)
    {
        return variant.value<QString>() == value;
    }
    static bool compare(QVariant variant, const QVariant &value)
    {
        return variant == value;
    }
};
# endif // __has_include(<forward_list>)
#endif // __has_include

template<typename Container>
struct KeyGetter
{
    static const typename Container::key_type & get(const typename Container::const_iterator &it)
    {
        return it.key();
    }
    static const typename Container::mapped_type & value(const typename Container::const_iterator &it)
    {
        return it.value();
    }
};

template<typename T, typename U>
struct KeyGetter<std::map<T, U> >
{
    static const T & get(const typename std::map<T, U>::const_iterator &it)
    {
        return it->first;
    }
    static const U & value(const typename std::map<T, U>::const_iterator &it)
    {
        return it->second;
    }
};


typedef std::unordered_map<int, bool> StdUnorderedMap_int_bool;

Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE(std::unordered_map)

Q_DECLARE_METATYPE(StdUnorderedMap_int_bool)

template<typename T, typename U>
struct KeyGetter<std::unordered_map<T, U> >
{
    static const T & get(const typename std::unordered_map<T, U>::const_iterator &it)
    {
        return it->first;
    }
    static const U & value(const typename std::unordered_map<T, U>::const_iterator &it)
    {
        return it->second;
    }
};

template<typename Iterator>
void sortIterable(QSequentialIterable *iterable)
{
    std::sort(Iterator(iterable->mutableBegin()), Iterator(iterable->mutableEnd()),
              [&](const QVariant &a, const QVariant &b) {
        return a.toInt() < b.toInt();
    });
}

template<typename Container>
static void testSequentialIteration()
{
    int numSeen = 0;
    Container sequence;
    ContainerAPI<Container>::insert(sequence, 1);
    ContainerAPI<Container>::insert(sequence, 2);
    ContainerAPI<Container>::insert(sequence, 3);

    QVariant listVariant = QVariant::fromValue(sequence);
    QVERIFY(listVariant.canConvert<QVariantList>());
    QVariantList varList = listVariant.value<QVariantList>();
    QCOMPARE(varList.size(), (int)std::distance(sequence.begin(), sequence.end()));
    QSequentialIterable listIter = listVariant.view<QSequentialIterable>();
    QCOMPARE(varList.size(), listIter.size());

    typename Container::iterator containerIter = sequence.begin();
    const typename Container::iterator containerEnd = sequence.end();
    for (int i = 0; i < listIter.size(); ++i, ++containerIter, ++numSeen)
    {
        QVERIFY(ContainerAPI<Container >::compare(listIter.at(i), *containerIter));
        QVERIFY(ContainerAPI<Container >::compare(listIter.at(i), varList.at(i)));
    }
    QCOMPARE(numSeen, (int)std::distance(sequence.begin(), sequence.end()));
    QCOMPARE(containerIter, containerEnd);

    numSeen = 0;
    containerIter = sequence.begin();
    for (QVariant v : listIter) {
        QVERIFY(ContainerAPI<Container>::compare(v, *containerIter));
        QVERIFY(ContainerAPI<Container>::compare(v, varList.at(numSeen)));
        ++containerIter;
        ++numSeen;
    }
    QCOMPARE(numSeen, (int)std::distance(sequence.begin(), sequence.end()));

    auto compareLists = [&]() {
        int numSeen = 0;
        auto varList = listVariant.value<QVariantList>();
        auto varIter = varList.begin();
        for (const QVariant &v : std::as_const(listIter)) {
            QVERIFY(ContainerAPI<Container>::compare(v, *varIter));
            ++varIter;
            ++numSeen;
        }
        QCOMPARE(varIter, varList.end());
        numSeen = 0;
        auto constVarIter = varList.constBegin();
        for (QVariant v : listIter) {
            QVERIFY(ContainerAPI<Container>::compare(v, *constVarIter));
            ++constVarIter;
            ++numSeen;
        }
        QCOMPARE(numSeen, (int)std::distance(varList.begin(), varList.end()));
    };
    compareLists();

    QVariant first = listIter.at(0);
    QVariant second = listIter.at(1);
    QVariant third = listIter.at(2);
    compareLists();
    listIter.addValue(third);
    compareLists();
    listIter.addValue(second);
    compareLists();
    listIter.addValue(first);
    compareLists();

    QCOMPARE(listIter.size(), 6);

    if (listIter.canRandomAccessIterate())
        sortIterable<QSequentialIterable::RandomAccessIterator>(&listIter);
    else if (listIter.canReverseIterate())
        sortIterable<QSequentialIterable::BidirectionalIterator>(&listIter);
    else if (listIter.canForwardIterate())
        return; // std::sort cannot sort with only forward iterators.
    else
        QFAIL("The container has no meaningful iterators");

    compareLists();
    QCOMPARE(listIter.size(), 6);
    QCOMPARE(listIter.at(0), first);
    QCOMPARE(listIter.at(1), first);
    QCOMPARE(listIter.at(2), second);
    QCOMPARE(listIter.at(3), second);
    QCOMPARE(listIter.at(4), third);
    QCOMPARE(listIter.at(5), third);

    if (listIter.metaContainer().canRemoveValue()) {
        listIter.removeValue();
        compareLists();
        QCOMPARE(listIter.size(), 5);
        QCOMPARE(listIter.at(0), first);
        QCOMPARE(listIter.at(1), first);
        QCOMPARE(listIter.at(2), second);
        QCOMPARE(listIter.at(3), second);
        QCOMPARE(listIter.at(4), third);
    } else {
        // QString and QByteArray have no pop_back or pop_front and it's unclear what other
        // method we should use to remove an item.
        QVERIFY((std::is_same_v<Container, QString> || std::is_same_v<Container, QByteArray>));
    }

    auto i = listIter.mutableBegin();
    QVERIFY(i != listIter.mutableEnd());

    *i = QStringLiteral("17");
    if (listIter.metaContainer().valueMetaType() == QMetaType::fromType<int>())
        QCOMPARE(listIter.at(0).toInt(), 17);
    else if (listIter.metaContainer().valueMetaType() == QMetaType::fromType<bool>())
        QCOMPARE(listIter.at(0).toBool(), false);

    *i = QStringLiteral("true");
    if (listIter.metaContainer().valueMetaType() == QMetaType::fromType<int>())
        QCOMPARE(listIter.at(0).toInt(), 0);
    else if (listIter.metaContainer().valueMetaType() == QMetaType::fromType<bool>())
        QCOMPARE(listIter.at(0).toBool(), true);
}

template<typename Container>
static void testAssociativeIteration()
{
    using Key = typename Container::key_type;
    using Mapped = typename Container::mapped_type;

    int numSeen = 0;
    Container mapping;
    mapping[5] = true;
    mapping[15] = false;

    QVariant mappingVariant = QVariant::fromValue(mapping);
    QVariantMap varMap = mappingVariant.value<QVariantMap>();
    QVariantMap varHash = mappingVariant.value<QVariantMap>();
    QAssociativeIterable mappingIter = mappingVariant.view<QAssociativeIterable>();

    typename Container::const_iterator containerIter = mapping.begin();
    const typename Container::const_iterator containerEnd = mapping.end();
    for ( ;containerIter != containerEnd; ++containerIter, ++numSeen)
    {
        Mapped expected = KeyGetter<Container>::value(containerIter);
        Key key = KeyGetter<Container>::get(containerIter);
        Mapped actual = qvariant_cast<Mapped>(mappingIter.value(key));
        QCOMPARE(qvariant_cast<Mapped>(varMap.value(QString::number(key))), expected);
        QCOMPARE(qvariant_cast<Mapped>(varHash.value(QString::number(key))), expected);
        QCOMPARE(actual, expected);
        const QAssociativeIterable::const_iterator it = mappingIter.find(key);
        QVERIFY(it != mappingIter.end());
        QCOMPARE(it.value().value<Mapped>(), expected);
    }
    QCOMPARE(numSeen, (int)std::distance(mapping.begin(), mapping.end()));
    QCOMPARE(containerIter, containerEnd);
    QVERIFY(mappingIter.find(10) == mappingIter.end());

    auto i = mappingIter.mutableFind(QStringLiteral("nonono"));
    QCOMPARE(i, mappingIter.mutableEnd());
    i = mappingIter.mutableFind(QStringLiteral("5"));
    QVERIFY(i != mappingIter.mutableEnd());

    *i = QStringLiteral("17");

    if (mappingIter.metaContainer().mappedMetaType() == QMetaType::fromType<int>())
        QCOMPARE(mappingIter.value(5).toInt(), 17);
    else if (mappingIter.metaContainer().mappedMetaType() == QMetaType::fromType<bool>())
        QCOMPARE(mappingIter.value(5).toBool(), true);

    *i = QStringLiteral("true");
    if (mappingIter.metaContainer().mappedMetaType() == QMetaType::fromType<int>())
        QCOMPARE(mappingIter.value(5).toInt(), 0);
    else if (mappingIter.metaContainer().mappedMetaType() == QMetaType::fromType<bool>())
        QCOMPARE(mappingIter.value(5).toBool(), true);

    QVERIFY(mappingIter.containsKey("5"));
    mappingIter.removeKey(QStringLiteral("5"));
    QCOMPARE(mappingIter.find(5), mappingIter.end());

    mappingIter.setValue(5, 44);
    if (mappingIter.metaContainer().mappedMetaType() == QMetaType::fromType<int>())
        QCOMPARE(mappingIter.value(5).toInt(), 44);
    else if (mappingIter.metaContainer().mappedMetaType() == QMetaType::fromType<bool>())
        QCOMPARE(mappingIter.value(5).toBool(), true);

    // Test that find() does not coerce
    auto container = Container();
    container[0] = true;

    QVariant containerVariant = QVariant::fromValue(container);
    QAssociativeIterable iter = containerVariant.value<QAssociativeIterable>();
    auto f = iter.constFind(QStringLiteral("anything"));
    QCOMPARE(f, iter.constEnd());
}

void tst_QVariant::iterateSequentialContainerElements_data()
{
    QTest::addColumn<QFunctionPointer>("testFunction");
#define ADD(T)  QTest::newRow(#T) << &testSequentialIteration<T>
    ADD(QQueue<int>);
    ADD(QQueue<QVariant>);
    ADD(QQueue<QString>);
    ADD(QList<int>);
    ADD(QList<QVariant>);
    ADD(QList<QString>);
    ADD(QList<QByteArray>);
    ADD(QStack<int>);
    ADD(QStack<QVariant>);
    ADD(QStack<QString>);
    ADD(std::vector<int>);
    ADD(std::vector<QVariant>);
    ADD(std::vector<QString>);
    ADD(std::list<int>);
    ADD(std::list<QVariant>);
    ADD(std::list<QString>);
    ADD(QStringList);
    ADD(QByteArrayList);
    ADD(QString);
    ADD(QByteArray);

#ifdef TEST_FORWARD_LIST
    ADD(std::forward_list<int>);
    ADD(std::forward_list<QVariant>);
    ADD(std::forward_list<QString>);
#endif
#undef ADD
}

void tst_QVariant::iterateAssociativeContainerElements_data()
{
    QTest::addColumn<QFunctionPointer>("testFunction");
#define ADD(C, K, V)  QTest::newRow(#C #K #V) << &testAssociativeIteration<C<K, V>>;
    ADD(QHash, int, bool);
    ADD(QHash, int, int);
    ADD(QMap, int, bool);
    ADD(std::map, int, bool);
    ADD(std::unordered_map, int, bool);
#undef ADD
}

void tst_QVariant::iterateContainerElements()
{
    {
        QVariantList ints;
        ints << 1 << 2 << 3;
        QVariant var = QVariant::fromValue(ints);
        QSequentialIterable iter = var.value<QSequentialIterable>();
        QSequentialIterable::const_iterator it = iter.begin();
        QSequentialIterable::const_iterator end = iter.end();
        QCOMPARE(ints.at(1), *(it + 1));
        int i = 0;
        for ( ; it != end; ++it, ++i) {
            QCOMPARE(ints.at(i), *it);
        }

        it = iter.begin();

        QVariantList intsCopy;
        intsCopy << *(it++);
        intsCopy << *(it++);
        intsCopy << *(it++);
        QCOMPARE(ints, intsCopy);
    }

    {
        QMap<int, QString> mapping;
        mapping.insert(1, "one");
        mapping.insert(2, "two");
        mapping.insert(3, "three");
        QVariant var = QVariant::fromValue(mapping);
        QAssociativeIterable iter = var.value<QAssociativeIterable>();
        QAssociativeIterable::const_iterator it = iter.begin();
        QAssociativeIterable::const_iterator end = iter.end();
        QCOMPARE(*(++mapping.begin()), (*(it + 1)).toString());
        int i = 0;
        for ( ; it != end; ++it, ++i) {
            QCOMPARE(*(std::next(mapping.begin(), i)), (*it).toString());
        }

        QVariantList nums;
        nums << "one" << "two" << "three";

        it = iter.begin();

        QVariantList numsCopy;
        numsCopy << *(it++);
        numsCopy << *(it++);
        numsCopy << *(it++);
        QCOMPARE(nums, numsCopy);
    }

    {
        auto container = QVariantMap();

        container["one"] = 1;

        auto containerVariant = QVariant::fromValue(container);
        auto iter = containerVariant.value<QAssociativeIterable>();
        auto value = iter.value("one");
        QCOMPARE(value, QVariant(1));

        auto f = iter.constFind("one");
        QCOMPARE(*f, QVariant(1));
    }
}

template <typename Pair> static void testVariantPairElements()
{
    QFETCH(std::function<void(void *)>, makeValue);
    Pair p;
    makeValue(&p);
    QVariant v = QVariant::fromValue(p);

    QVERIFY(v.canConvert<QVariantPair>());
    QVariantPair pi = v.value<QVariantPair>();
    QCOMPARE(pi.first, QVariant::fromValue(p.first));
    QCOMPARE(pi.second, QVariant::fromValue(p.second));
}

void tst_QVariant::pairElements_data()
{
    QTest::addColumn<QFunctionPointer>("testFunction");
    QTest::addColumn<std::function<void(void *)>>("makeValue");

    static auto makeString = [](auto &&value) -> QString {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
            return QString::number(value);
        } else if constexpr (std::is_same_v<T, QVariant>) {
            return value.toString();
        } else {
            return value;
        }
    };
    auto addRow = [](auto &&first, auto &&second) {
        using Pair = std::pair<std::decay_t<decltype(first)>, std::decay_t<decltype(second)>>;
        std::function<void(void *)> makeValue = [=](void *pair) {
            *static_cast<Pair *>(pair) = Pair{first, second};
        };

        QTest::addRow("%s", qPrintable(makeString(first) + u',' + makeString(second)))
                << &testVariantPairElements<Pair> << makeValue;
    };

    addRow(4, 5);
    addRow(QStringLiteral("one"), QStringLiteral("two"));
    addRow(QVariant(4), QVariant(5));
    addRow(QVariant(41), 65);
    addRow(41, QVariant(15));
}

template <auto value> static void testVariantEnum()
{
    using Enum = decltype(value);
    auto canLosslesslyConvert = [=](auto zero) {
        return sizeof(value) <= sizeof(zero) ||
                value == Enum(decltype(zero)(qToUnderlying(value)));
    };
    bool losslessConvertToInt = canLosslesslyConvert(int{});

    QVariant var = QVariant::fromValue(value);
    QCOMPARE(var.userType(), qMetaTypeId<Enum>());

    QVERIFY(var.canConvert<Enum>());
    QVERIFY(var.canConvert<int>());
    QVERIFY(var.canConvert<unsigned int>());
    QVERIFY(var.canConvert<short>());
    QVERIFY(var.canConvert<unsigned short>());
    QVERIFY(var.canConvert<qint64>());
    QVERIFY(var.canConvert<quint64>());

    QCOMPARE(var.value<Enum>(), value);
    QCOMPARE(var.value<int>(), static_cast<int>(value));
    QCOMPARE(var.value<uint>(), static_cast<uint>(value));
    QCOMPARE(var.value<short>(), static_cast<short>(value));
    QCOMPARE(var.value<unsigned short>(), static_cast<unsigned short>(value));
    QCOMPARE(var.value<qint64>(), static_cast<qint64>(value));
    QCOMPARE(var.value<quint64>(), static_cast<quint64>(value));

    QVariant var2 = var;
    QVERIFY(var2.convert(QMetaType::fromType<int>()));
    QCOMPARE(var2.value<int>(), static_cast<int>(value));

    QVariant strVar = QString::number(qToUnderlying(value));
    QVariant baVar = QByteArray::number(qToUnderlying(value));
    QCOMPARE(strVar.value<Enum>(), value);
    QCOMPARE(baVar.value<Enum>(), value);
    QCOMPARE(var.value<QString>(), strVar);
    QCOMPARE(var.value<QByteArray>(), baVar);

    // unary + to silence gcc warning
    if (losslessConvertToInt) {
        int intValue = static_cast<int>(value);
        QVariant intVar = intValue;
        QVERIFY(intVar.canConvert<Enum>());
        QCOMPARE(intVar.value<Enum>(), value);
    }
    qint64 longValue = static_cast<qint64>(value);
    QVERIFY(QVariant(longValue).canConvert<Enum>());
    QCOMPARE(QVariant(longValue).value<Enum>(), value);

    auto value2 = Enum(qToUnderlying(value) + 1);
    var2 = QVariant::fromValue(value2);
    QCOMPARE_EQ(var, var);
    QCOMPARE_NE(var, var2);
    QCOMPARE(QVariant::compare(var, var), QPartialOrdering::Equivalent);
    QCOMPARE(QVariant::compare(var, var2), QPartialOrdering::Less);
    QCOMPARE(QVariant::compare(var2, var), QPartialOrdering::Greater);

    QCOMPARE_EQ(var, static_cast<qint64>(value));
    QCOMPARE_EQ(var, static_cast<quint64>(value));
    QCOMPARE_EQ(static_cast<qint64>(value), var);
    QCOMPARE_EQ(static_cast<quint64>(value), var);
    QCOMPARE_NE(var2, static_cast<qint64>(value));
    QCOMPARE_NE(var2, static_cast<quint64>(value));
    QCOMPARE_NE(static_cast<qint64>(value), var2);
    QCOMPARE_NE(static_cast<quint64>(value), var2);

    if (losslessConvertToInt) {
        QCOMPARE_EQ(var, int(value));
        QCOMPARE_EQ(int(value), var);
        QCOMPARE_NE(var2, int(value));
        QCOMPARE_NE(int(value), var2);
    }
    if (canLosslesslyConvert(uint{})) {
        QCOMPARE_EQ(var, uint(value));
        QCOMPARE_EQ(uint(value), var);
        QCOMPARE_NE(var2, uint(value));
        QCOMPARE_NE(uint(value), var2);
    }
    if (canLosslesslyConvert(short{})) {
        QCOMPARE_EQ(var, short(value));
        QCOMPARE_EQ(short(value), var);
        QCOMPARE_NE(var2, short(value));
        QCOMPARE_NE(short(value), var2);
    }
    if (canLosslesslyConvert(ushort{})) {
        QCOMPARE_EQ(var, ushort(value));
        QCOMPARE_EQ(ushort(value), var);
        QCOMPARE_NE(var2, ushort(value));
        QCOMPARE_NE(ushort(value), var2);
    }
    if (canLosslesslyConvert(char{})) {
        QCOMPARE_EQ(var, char(value));
        QCOMPARE_EQ(char(value), var);
        QCOMPARE_NE(var2, char(value));
        QCOMPARE_NE(char(value), var2);
    }
    if (canLosslesslyConvert(uchar{})) {
        QCOMPARE_EQ(var, uchar(value));
        QCOMPARE_EQ(uchar(value), var);
        QCOMPARE_NE(var2, uchar(value));
        QCOMPARE_NE(uchar(value), var2);
    }
    if (canLosslesslyConvert(qint8{})) {
        QCOMPARE_EQ(var, qint8(value));
        QCOMPARE_EQ(qint8(value), var);
        QCOMPARE_NE(var2, qint8(value));
        QCOMPARE_NE(qint8(value), var2);
    }

    // string compares too (of the values in decimal)
    QCOMPARE_EQ(var, QString::number(qToUnderlying(value)));
    QCOMPARE_EQ(QString::number(qToUnderlying(value)), var);
    QCOMPARE_NE(var, QString::number(qToUnderlying(value2)));
    QCOMPARE_NE(QString::number(qToUnderlying(value2)), var);
}

void tst_QVariant::enums_data()
{
    QTest::addColumn<QFunctionPointer>("testFunction");

#define ADD(V)      QTest::newRow(#V) << &testVariantEnum<V>
    ADD(EnumTest_Enum0_value);
    ADD(EnumTest_Enum0_negValue);
    ADD(EnumTest_Enum1_value);
    ADD(EnumTest_Enum1_bigValue);
    ADD(EnumTest_Enum3::EnumTest_Enum3_value);
    ADD(EnumTest_Enum3::EnumTest_Enum3_bigValue);
    ADD(EnumTest_Enum4::EnumTest_Enum4_value);
    ADD(EnumTest_Enum4::EnumTest_Enum4_bigValue);
    ADD(EnumTest_Enum5::EnumTest_Enum5_value);
    ADD(EnumTest_Enum6::EnumTest_Enum6_value);
    ADD(EnumTest_Enum7::EnumTest_Enum7_value);
    ADD(EnumTest_Enum8::EnumTest_Enum8_value);
    ADD(EnumTest_Enum3::EnumTest_Enum3_value);
#undef ADD
}

// ### C++20: this would be easier if QFlags were a structural type
template <typename Enum, auto Value> static void testVariantMetaEnum()
{
    Enum value(Value);
    QFETCH(QString, string);

    QVariant var = QVariant::fromValue(value);
    QVERIFY(var.canConvert<QString>());
    QVERIFY(var.canConvert<QByteArray>());

    QCOMPARE(var.value<QString>(), string);
    QCOMPARE(var.value<QByteArray>(), string.toLatin1());

    QVariant strVar = string;
    QVERIFY(strVar.canConvert<Enum>());
    QCOMPARE(strVar.value<Enum>(), value);
    strVar = string.toLatin1();
    QVERIFY(strVar.canConvert<Enum>());
    QCOMPARE(strVar.value<Enum>(), value);
}

void tst_QVariant::metaEnums_data()
{
    QTest::addColumn<QFunctionPointer>("testFunction");
    QTest::addColumn<QString>("string");

#define METAENUMS_TEST(Value) \
    QTest::newRow(#Value) << &testVariantMetaEnum<decltype(Value), Value> << #Value;

    METAENUMS_TEST(MetaEnumTest_Enum0_value);
    METAENUMS_TEST(MetaEnumTest_Enum1_value);
    METAENUMS_TEST(MetaEnumTest_Enum1_bigValue);
    METAENUMS_TEST(MetaEnumTest_Enum3_value);
    METAENUMS_TEST(MetaEnumTest_Enum3_bigValue);
    METAENUMS_TEST(MetaEnumTest_Enum3_bigNegValue);
    METAENUMS_TEST(MetaEnumTest_Enum4_value);
    METAENUMS_TEST(MetaEnumTest_Enum4_bigValue);
    METAENUMS_TEST(MetaEnumTest_Enum5_value);
    METAENUMS_TEST(MetaEnumTest_Enum6_value);
    METAENUMS_TEST(MetaEnumTest_Enum8_value);
    { using namespace Qt; METAENUMS_TEST(RichText); }
#undef METAENUMS_TEST

    QTest::newRow("AlignBottom")
            << &testVariantMetaEnum<Qt::Alignment, Qt::AlignBottom> << "AlignBottom";

    constexpr auto AlignHCenterBottom = Qt::AlignmentFlag((Qt::AlignHCenter | Qt::AlignBottom).toInt());
    QTest::newRow("AlignHCenter|AlignBottom")
            << &testVariantMetaEnum<Qt::Alignment, AlignHCenterBottom> << "AlignHCenter|AlignBottom";
}

void tst_QVariant::nullConvert()
{
    // null variant with no initialized value
    QVariant nullVar {QMetaType::fromType<QString>()};
    QVERIFY(nullVar.isValid());
    QVERIFY(nullVar.isNull());
    // We can not convert a variant with no value
    QVERIFY(!nullVar.convert(QMetaType::fromType<QUrl>()));
    QCOMPARE(nullVar.typeId(), QMetaType::QUrl);
    QVERIFY(nullVar.isNull());
}

void tst_QVariant::accessSequentialContainerKey()
{
    QString nameResult;

    {
    QMap<QString, QObject*> mapping;
    QString name = QString::fromLatin1("Seven");
    mapping.insert(name, nullptr);

    QVariant variant = QVariant::fromValue(mapping);

    QAssociativeIterable iterable = variant.value<QAssociativeIterable>();
    QAssociativeIterable::const_iterator iit = iterable.begin();
    const QAssociativeIterable::const_iterator end = iterable.end();
    for ( ; iit != end; ++iit) {
        nameResult += iit.key().toString();
    }
    } // Destroy mapping
    // Regression test for QTBUG-52246 - no memory corruption/double deletion
    // of the string key.

    QCOMPARE(nameResult, QStringLiteral("Seven"));
}

void tst_QVariant::shouldDeleteVariantDataWorksForSequential()
{
    QCOMPARE(instanceCount, 0);
    {
        QtMetaContainerPrivate::QMetaSequenceInterface metaSequence {};
        metaSequence.iteratorCapabilities = QtMetaContainerPrivate::RandomAccessCapability
                | QtMetaContainerPrivate::BiDirectionalCapability
                | QtMetaContainerPrivate::ForwardCapability
                | QtMetaContainerPrivate::InputCapability;

        metaSequence.sizeFn = [](const void *) { return qsizetype(1); };
        metaSequence.createConstIteratorFn =
                [](const void *, QtMetaContainerPrivate::QMetaSequenceInterface::Position) -> void* {
            return nullptr;
        };
        metaSequence.addValueFn = [](void *, const void *,
                QtMetaContainerPrivate::QMetaSequenceInterface::Position) {};
        metaSequence.advanceConstIteratorFn = [](void *, qsizetype) {};
        metaSequence.destroyConstIteratorFn = [](const void *){};
        metaSequence.compareConstIteratorFn = [](const void *, const void *) {
            return true; // all iterators are nullptr
        };
        metaSequence.copyConstIteratorFn = [](void *, const void *){};
        metaSequence.diffConstIteratorFn = [](const void *, const void *) -> qsizetype  {
            return 0;
        };
        metaSequence.valueAtIndexFn = [](const void *, qsizetype, void *dataPtr) -> void {
            MyType mytype {1, "eins"};
            *static_cast<MyType *>(dataPtr) = mytype;
        };
        metaSequence.valueAtConstIteratorFn = [](const void *, void *dataPtr) -> void {
            MyType mytype {2, "zwei"};
            *static_cast<MyType *>(dataPtr) = mytype;
        };
        metaSequence.valueMetaType = QtPrivate::qMetaTypeInterfaceForType<MyType>();

        QSequentialIterable iterable(QMetaSequence(&metaSequence), nullptr);
        QVariant value1 = iterable.at(0);
        QVERIFY(value1.canConvert<MyType>());
        QCOMPARE(value1.value<MyType>().number, 1);
        QVariant value2 = *iterable.begin();
        QVERIFY(value2.canConvert<MyType>());
        QCOMPARE(value2.value<MyType>().number, 2);
    }
    QCOMPARE(instanceCount, 0);
}

void tst_QVariant::shouldDeleteVariantDataWorksForAssociative()
{
    QCOMPARE(instanceCount, 0);
    {
        QtMetaContainerPrivate::QMetaAssociationInterface iterator {};

        iterator.sizeFn = [](const void *) -> qsizetype {return 1;};
        iterator.mappedMetaType = QtPrivate::qMetaTypeInterfaceForType<MyType>();
        iterator.keyMetaType = QtPrivate::qMetaTypeInterfaceForType<MyType>();
        iterator.createConstIteratorFn = [](
                const void *, QtMetaContainerPrivate::QMetaContainerInterface::Position) -> void * {
            return nullptr;
        };
        iterator.advanceConstIteratorFn = [](void *, qsizetype) {};
        iterator.destroyConstIteratorFn = [](const void *){};
        iterator.compareConstIteratorFn = [](const void *, const void *) {
            return true; /*all iterators are nullptr*/
        };
        iterator.createConstIteratorAtKeyFn = [](const void *, const void *) -> void * {
            return reinterpret_cast<void *>(quintptr(42));
        };
        iterator.copyConstIteratorFn = [](void *, const void *) {};
        iterator.diffConstIteratorFn = [](const void *, const void *) -> qsizetype { return 0; };
        iterator.keyAtConstIteratorFn = [](const void *iterator, void *dataPtr) -> void {
            MyType mytype {1, "key"};
            if (reinterpret_cast<quintptr>(iterator) == 42) {
                mytype.number = 42;
                mytype.text = "find_key";
            }
            *static_cast<MyType *>(dataPtr) = mytype;
        };
        iterator.mappedAtConstIteratorFn = [](const void *iterator, void *dataPtr) -> void {
            MyType mytype {2, "value"};
            if (reinterpret_cast<quintptr>(iterator) == 42) {
                mytype.number = 42;
                mytype.text = "find_value";
            }
            *static_cast<MyType *>(dataPtr) = mytype;
        };
        QAssociativeIterable iterable(QMetaAssociation(&iterator), nullptr);
        auto it = iterable.begin();
        QVariant value1 = it.key();
        QVERIFY(value1.canConvert<MyType>());
        QCOMPARE(value1.value<MyType>().number, 1);
        QCOMPARE(value1.value<MyType>().text, "key");
        QVariant value2 = it.value();
        QVERIFY(value2.canConvert<MyType>());
        QCOMPARE(value2.value<MyType>().number, 2);
        auto findIt = iterable.find(QVariant::fromValue(MyType {}));
        value1 = findIt.key();
        QCOMPARE(value1.value<MyType>().number, 42);
        QCOMPARE(value1.value<MyType>().text, "find_key");
        value2 = findIt.value();
        QCOMPARE(value2.value<MyType>().number, 42);
        QCOMPARE(value2.value<MyType>().text, "find_value");
    }
    QCOMPARE(instanceCount, 0);
}

void tst_QVariant::fromStdVariant()
{
#define CHECK_EQUAL(lhs, rhs, type) do { \
        QCOMPARE(lhs.typeId(), rhs.typeId()); \
        if (lhs.isNull()) { \
            QVERIFY(rhs.isNull()); \
        } else { \
            QVERIFY(!rhs.isNull()); \
            QCOMPARE(get< type >(lhs), get< type >(rhs)); \
        } \
    } while (false)

    {
        typedef std::variant<int, bool> intorbool_t;
        intorbool_t stdvar = 5;
        QVariant qvar = QVariant::fromStdVariant(stdvar);
        QVERIFY(!qvar.isNull());
        QCOMPARE(qvar.typeId(), QMetaType::Int);
        QCOMPARE(qvar.value<int>(), std::get<int>(stdvar));
        {
            const auto qv2 = QVariant::fromStdVariant(std::move(stdvar));
            CHECK_EQUAL(qv2, qvar, int);
        }

        stdvar = true;
        qvar = QVariant::fromStdVariant(stdvar);
        QVERIFY(!qvar.isNull());
        QCOMPARE(qvar.typeId(), QMetaType::Bool);
        QCOMPARE(qvar.value<bool>(), std::get<bool>(stdvar));
        {
            const auto qv2 = QVariant::fromStdVariant(std::move(stdvar));
            CHECK_EQUAL(qv2, qvar, bool);
        }
    }
    {
        std::variant<std::monostate, int> stdvar;
        QVariant qvar = QVariant::fromStdVariant(stdvar);
        QVERIFY(!qvar.isValid());
        {
            const auto qv2 = QVariant::fromStdVariant(std::move(stdvar));
            CHECK_EQUAL(qv2, qvar, int); // fake type, they're empty
        }
        stdvar = -4;
        qvar = QVariant::fromStdVariant(stdvar);
        QVERIFY(!qvar.isNull());
        QCOMPARE(qvar.typeId(), QMetaType::Int);
        QCOMPARE(qvar.value<int>(), std::get<int>(stdvar));
        {
            const auto qv2 = QVariant::fromStdVariant(std::move(stdvar));
            CHECK_EQUAL(qv2, qvar, int);
        }
    }
    {
        std::variant<int, bool, QChar> stdvar = QChar::fromLatin1(' ');
        QVariant qvar = QVariant::fromStdVariant(stdvar);
        QVERIFY(!qvar.isNull());
        QCOMPARE(qvar.typeId(), QMetaType::QChar);
        QCOMPARE(qvar.value<QChar>(), std::get<QChar>(stdvar));
        {
            const auto qv2 = QVariant::fromStdVariant(std::move(stdvar));
            CHECK_EQUAL(qv2, qvar, QChar);
        }
    }
    // rvalue fromStdVariant() actually moves:
    {
        const auto foo = u"foo"_s;
        std::variant<QString, QByteArray> stdvar = foo;
        QVariant qvar = QVariant::fromStdVariant(std::move(stdvar));
        const auto ps = get_if<QString>(&stdvar);
        QVERIFY(ps);
        QVERIFY(ps->isNull()); // QString was moved from
        QVERIFY(!qvar.isNull());
        QCOMPARE(qvar.typeId(), QMetaType::QString);
        QCOMPARE(get<QString>(qvar), foo);
    }

#undef CHECK_EQUAL
}

void tst_QVariant::qt4UuidDataStream()
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_4_8);
    QUuid source(0x12345678,0x1234,0x1234,0x12,0x23,0x34,0x45,0x56,0x67,0x78,0x89);
    stream << QVariant::fromValue(source);
    const QByteArray qt4Data = QByteArray::fromHex("0000007f000000000651557569640012345678123412341223344556677889");
    QCOMPARE(data, qt4Data);

    QDataStream input(&data, QIODevice::ReadOnly);
    input.setVersion(QDataStream::Qt_4_8);
    QVariant result;
    input >> result;
    QCOMPARE(result.value<QUuid>(), source);
}

void tst_QVariant::sequentialIterableEndianessSanityCheck()
{
    namespace QMTP = QtMetaContainerPrivate;
    QMTP::IteratorCapabilities oldIteratorCaps
            = QMTP::InputCapability | QMTP::ForwardCapability
            | QMTP::BiDirectionalCapability | QMTP::RandomAccessCapability;
    QMTP::QMetaSequenceInterface seqImpl {};
    QCOMPARE(seqImpl.revision, 0u);
    memcpy(&seqImpl.iteratorCapabilities, &oldIteratorCaps, sizeof(oldIteratorCaps));
    QCOMPARE(seqImpl.revision, 0u);
}

void tst_QVariant::sequentialIterableAppend()
{
    {
        QList<int> container { 1, 2 };
        auto variant = QVariant::fromValue(container);
        QVERIFY(variant.canConvert<QSequentialIterable>());
        QSequentialIterable asIterable = variant.view<QSequentialIterable>();
        const int i = 3, j = 4;
        void *mutableIterable = asIterable.mutableIterable();
        asIterable.metaContainer().addValueAtEnd(mutableIterable, &i);
        asIterable.metaContainer().addValueAtEnd(mutableIterable, &j);
        QCOMPARE(variant.value<QList<int>>(), QList<int> ({ 1, 2, 3, 4 }));

        asIterable.metaContainer().addValueAtBegin(mutableIterable, &i);
        asIterable.metaContainer().addValueAtBegin(mutableIterable, &j);
        QCOMPARE(variant.value<QList<int>>(), QList<int> ({ 4, 3, 1, 2, 3, 4 }));

        asIterable.metaContainer().removeValueAtBegin(mutableIterable);
        QCOMPARE(variant.value<QList<int>>(), QList<int> ({ 3, 1, 2, 3, 4 }));
        asIterable.metaContainer().removeValueAtEnd(mutableIterable);
        QCOMPARE(variant.value<QList<int>>(), QList<int> ({ 3, 1, 2, 3 }));
    }
    {
        QSet<QByteArray> container { QByteArray{"hello"}, QByteArray{"world"} };
        auto variant = QVariant::fromValue(std::move(container));
        QVERIFY(variant.canConvert<QSequentialIterable>());
        QSequentialIterable asIterable = variant.view<QSequentialIterable>();
        QByteArray qba1 {"goodbye"};
        QByteArray qba2 { "moon" };
        void *mutableIterable = asIterable.mutableIterable();
        asIterable.metaContainer().addValue(mutableIterable, &qba1);
        asIterable.metaContainer().addValue(mutableIterable, &qba2);
        QSet<QByteArray> reference { "hello", "world", "goodbye", "moon" };
        QCOMPARE(variant.value<QSet<QByteArray>>(), reference);
        asIterable.metaContainer().addValue(mutableIterable, &qba1);
        asIterable.metaContainer().addValue(mutableIterable, &qba2);
        QCOMPARE(variant.value<QSet<QByteArray>>(), reference);
    }
}

void tst_QVariant::preferDirectConversionOverInterfaces()
{
    using namespace QtMetaTypePrivate;
    bool calledCorrectConverter = false;
    QMetaType::registerConverter<MyType, QSequentialIterable>([](const MyType &) {
        return QSequentialIterable {};
    });
    QMetaType::registerConverter<MyType, QVariantList>([&calledCorrectConverter](const MyType &) {
        calledCorrectConverter = true;
        return QVariantList {};
    });
    QMetaType::registerConverter<MyType, QAssociativeIterable>([](const MyType &) {
        return QAssociativeIterable {};
    });
    QMetaType::registerConverter<MyType, QVariantHash>([&calledCorrectConverter](const MyType &) {
        calledCorrectConverter = true;
        return QVariantHash {};
    });
    QMetaType::registerConverter<MyType, QVariantMap>([&calledCorrectConverter](const MyType &) {
        calledCorrectConverter = true;
        return QVariantMap {};
    });
    auto holder = QVariant::fromValue(MyType {});

    QVERIFY(holder.canConvert<QSequentialIterable>());
    QVERIFY(holder.canConvert<QVariantList>());
    QVERIFY(holder.canConvert<QAssociativeIterable>());
    QVERIFY(holder.canConvert<QVariantHash>());
    QVERIFY(holder.canConvert<QVariantMap>());

    holder.value<QVariantList>();
    QVERIFY(calledCorrectConverter);
    calledCorrectConverter = false;

    holder.value<QVariantHash>();
    QVERIFY(calledCorrectConverter);
    calledCorrectConverter = false;

    holder.value<QVariantMap>();
    QVERIFY(calledCorrectConverter);
}

struct MyTypeView
{
    MyType *data;
};

void tst_QVariant::mutableView()
{
    bool calledView = false;
    const bool success = QMetaType::registerMutableView<MyType, MyTypeView>([&](MyType &data) {
        calledView = true;
        return MyTypeView { &data };
    });
    QVERIFY(success);

    QTest::ignoreMessage(
                QtWarningMsg,
                "Mutable view on type already registered from type MyType to type MyTypeView");
    const bool shouldFail = QMetaType::registerMutableView<MyType, MyTypeView>([&](MyType &) {
        return MyTypeView { nullptr };
    });
    QVERIFY(!shouldFail);

    auto original = QVariant::fromValue(MyType {});

    QVERIFY(original.canView<MyTypeView>());
    QVERIFY(!original.canConvert<MyTypeView>());

    MyTypeView view = original.view<MyTypeView>();
    QVERIFY(calledView);
    const char *txt = "lll";
    view.data->number = 113;
    view.data->text = txt;

    MyType extracted = original.view<MyType>();
    QCOMPARE(extracted.number, 0);
    QCOMPARE(extracted.text, nullptr);
}

template<typename T>
void tst_QVariant::canViewAndView_ReturnFalseAndDefault_WhenConvertingBetweenPointerAndValue_impl(
        const QByteArray &typeName)
{
    T instance{};

    // Value -> Pointer
    QVariant value = QVariant::fromValue(instance);
    QVERIFY2(!value.canView<T *>(), typeName);
    QCOMPARE(value.view<T *>(), nullptr); // Expect default constructed pointer

    // Pointer -> Value
    QVariant pointer = QVariant::fromValue(&instance);
    QVERIFY2(!pointer.canView<T>(), typeName);
    QCOMPARE(pointer.view<T>(), T{}); // Expect default constructed. Note: Weak test since instance
                                      // is default constructed, but we detect data corruption
}

void tst_QVariant::canViewAndView_ReturnFalseAndDefault_WhenConvertingBetweenPointerAndValue()
{
#define ADD_TEST_IMPL(typeName, typeNameId, realType)                                         \
    canViewAndView_ReturnFalseAndDefault_WhenConvertingBetweenPointerAndValue_impl<realType>( \
            #typeName);

    // Add tests for static primitive types
    QT_FOR_EACH_STATIC_PRIMITIVE_NON_VOID_TYPE(ADD_TEST_IMPL)

    // Add tests for static core types
    QT_FOR_EACH_STATIC_CORE_CLASS(ADD_TEST_IMPL)
#undef ADD_TEST_IMPL
}

struct MoveTester
{
    bool wasMoved = false;
    MoveTester() = default;
    MoveTester(const MoveTester &) {}; // non-trivial on purpose
    MoveTester(MoveTester &&other) { other.wasMoved = true; }
    MoveTester& operator=(const MoveTester &) = default;
    MoveTester& operator=(MoveTester &&other) {other.wasMoved = true; return *this;}
};

void tst_QVariant::moveOperations()
{
    {
        QString str = "Hello";
        auto v = QVariant::fromValue(str);
        QVariant v2(std::move(v));
        QCOMPARE(v2.toString(), str);

        v = QVariant::fromValue(str);
        v2 = std::move(v);
        QCOMPARE(v2.toString(), str);
    }

    std::list<int> list;
    auto v = QVariant::fromValue(list);
    QVariant v2(std::move(v));
    QVERIFY(v2.value<std::list<int>>() == list);

    v = QVariant::fromValue(list);
    v2 = std::move(v);
    QVERIFY(v2.value<std::list<int>>() == list);

    {
        MoveTester tester;
        QVariant::fromValue(tester);
        QVERIFY(!tester.wasMoved);
        QVariant::fromValue(std::move(tester));
        QVERIFY(tester.wasMoved);
    }
    {
        const MoveTester tester;
        QVariant::fromValue(std::move(tester));
        QVERIFY(!tester.wasMoved); // we don't want to move from const variables
    }
    {
        QVariant var(std::in_place_type<MoveTester>);
        const auto p = get_if<MoveTester>(&var);
        QVERIFY(p);
        auto &tester = *p;
        QVERIFY(!tester.wasMoved);
        [[maybe_unused]] auto copy = var.value<MoveTester>();
        QVERIFY(!tester.wasMoved);
        [[maybe_unused]] auto moved = std::move(var).value<MoveTester>();
        QVERIFY(tester.wasMoved);
    }
}

class NoMetaObject : public QObject {};
void tst_QVariant::equalsWithoutMetaObject()
{
    using T = NoMetaObject*;
    QtPrivate::QMetaTypeInterface d = {
        /*.revision=*/ 0,
        /*.alignment=*/ alignof(T),
        /*.size=*/ sizeof(T),
        /*.flags=*/ QtPrivate::QMetaTypeForType<T>::flags(),
        /*.typeId=*/ 0,
        /*.metaObject=*/ nullptr, // on purpose.
        /*.name=*/ "NoMetaObject*",
        /*.defaultCtr=*/ [](const QtPrivate::QMetaTypeInterface *, void *addr) {
            new (addr) T();
        },
        /*.copyCtr=*/ [](const QtPrivate::QMetaTypeInterface *, void *addr, const void *other) {
            new (addr) T(*reinterpret_cast<const T *>(other));
        },
        /*.moveCtr=*/ [](const QtPrivate::QMetaTypeInterface *, void *addr, void *other) {
            new (addr) T(std::move(*reinterpret_cast<T *>(other)));
        },
        /*.dtor=*/ [](const QtPrivate::QMetaTypeInterface *, void *addr) {
            reinterpret_cast<T *>(addr)->~T();
        },
        /*.equals*/ nullptr,
        /*.lessThan*/ nullptr,
        /*.debugStream=*/ nullptr,
        /*.dataStreamOut=*/ nullptr,
        /*.dataStreamIn=*/ nullptr,
        /*.legacyRegisterOp=*/ nullptr
    };

    QMetaType noMetaObjectMetaType(&d);
    QMetaType qobjectMetaType = QMetaType::fromType<tst_QVariant*>();

    QVERIFY(noMetaObjectMetaType.flags() & QMetaType::PointerToQObject);
    QVERIFY(qobjectMetaType.flags() & QMetaType::PointerToQObject);

    QVariant noMetaObjectVariant(noMetaObjectMetaType, nullptr);
    QVariant qobjectVariant(qobjectMetaType, nullptr);

    // Shouldn't crash
    QVERIFY(noMetaObjectVariant != qobjectVariant);
    QVERIFY(qobjectVariant != noMetaObjectVariant);
}

struct NonDefaultConstructible
{
   NonDefaultConstructible(int i) :i(i) {}
   int i;
   friend bool operator==(NonDefaultConstructible l, NonDefaultConstructible r)
   { return l.i == r.i; }
};

template <> char *QTest::toString<NonDefaultConstructible>(const NonDefaultConstructible &ndc)
{
    return qstrdup('{' + QByteArray::number(ndc.i) + '}');
}

struct Indestructible
{
    Indestructible() {}
    Indestructible(const Indestructible &) {}
    Indestructible &operator=(const Indestructible &) { return *this; }
private:
    ~Indestructible() {}
};

struct NotCopyable
{
    NotCopyable() = default;
    NotCopyable(const NotCopyable&) = delete;
    NotCopyable &operator=(const NotCopyable &) = delete;
};

void tst_QVariant::constructFromIncompatibleMetaType_data()
{
    QTest::addColumn<QMetaType>("type");
    auto addRow = [](QMetaType meta) {
        QTest::newRow(meta.name()) << meta;
    };
    addRow(QMetaType::fromType<void>());
    addRow(QMetaType::fromType<NonDefaultConstructible>());
    addRow(QMetaType::fromType<QObject>());
    addRow(QMetaType::fromType<Indestructible>());
    addRow(QMetaType::fromType<NotCopyable>());
}

void tst_QVariant::constructFromIncompatibleMetaType()
{
   QFETCH(QMetaType, type);
   const auto anticipate = [type]() {
       // In that case, we run into a different condition (size == 0), and do not warn
       if (type == QMetaType::fromType<NonDefaultConstructible>()) {
           QTest::ignoreMessage(QtWarningMsg,
                                "QVariant: Cannot create type 'NonDefaultConstructible' without a "
                                "default constructor");
       } else if (type != QMetaType::fromType<void>()) {
           QTest::ignoreMessage(
               QtWarningMsg,
               "QVariant: Provided metatype for '" + QByteArray(type.name()) +
               "' does not support destruction and copy construction");
       }
   };
   anticipate();
   QVariant var(type, nullptr);
   QVERIFY(!var.isValid());
   QVERIFY(!var.metaType().isValid());

   anticipate();
   QVariant regular(1.0);
   QVERIFY(!var.canView(type));
   QVERIFY(!var.canConvert(type));
   QVERIFY(!QVariant(regular).convert(type));
}

void tst_QVariant::constructFromQtLT65MetaType()
{
   auto qsizeIface = QtPrivate::qMetaTypeInterfaceForType<QSize>();

   QtPrivate::QMetaTypeInterface qsize64Iface = {
       /*revision*/0,
       8,
       8,
       QMetaType::NeedsConstruction | QMetaType::NeedsDestruction,
       0,
       qsizeIface->metaObjectFn,
       "FakeQSize",
       qsizeIface->defaultCtr,
       qsizeIface->copyCtr,
       qsizeIface->moveCtr,
       /*dtor =*/  nullptr,
       qsizeIface->equals,
       qsizeIface->lessThan,
       qsizeIface->debugStream,
       qsizeIface->dataStreamOut,
       qsizeIface->dataStreamIn,
       /*legacyregop =*/ nullptr
   };
   QVariant var{ QMetaType(&qsize64Iface) };
   QVERIFY(var.isValid());
}

void tst_QVariant::copyNonDefaultConstructible()
{
    NonDefaultConstructible ndc(42);
    QVariant var = QVariant::fromValue(ndc);
    QVERIFY(var.isDetached());
    QCOMPARE(var.metaType(), QMetaType::fromType<NonDefaultConstructible>());
    QVERIFY(var.constData() != &ndc);

    // qvariant_cast<T> and QVariant::value<T> don't compile
    QCOMPARE(get<NonDefaultConstructible>(std::as_const(var)), ndc);

    QVariant var2 = var;
    var2.detach();      // force another copy
    QVERIFY(var2.isDetached());
    QVERIFY(var2.constData() != var.constData());
    QCOMPARE(get<NonDefaultConstructible>(std::as_const(var2)),
             get<NonDefaultConstructible>(std::as_const(var)));
    QCOMPARE(var2, var);
}

void tst_QVariant::inplaceConstruct()
{
    {
        NonDefaultConstructible ndc(42);
        QVariant var(std::in_place_type<NonDefaultConstructible>, 42);
        QVERIFY(get_if<NonDefaultConstructible>(&var));
        QCOMPARE(get<NonDefaultConstructible>(var), ndc);
    }

    {
        std::vector<int> vec {1, 2, 3, 4};
        QVariant var(std::in_place_type<std::vector<int>>, {1, 2, 3, 4});
        QVERIFY(get_if<std::vector<int>>(&var));
        QCOMPARE(get<std::vector<int>>(var), vec);
    }
}

struct LargerThanInternalQVariantStorage {
    char data[6 * sizeof(void *)];
};

struct alignas(256) LargerThanInternalQVariantStorageOveraligned {
    char data[6 * sizeof(void *)];
};

struct alignas(128) SmallerAlignmentEvenLargerSize {
    char data[17 * sizeof(void *)];
};

void tst_QVariant::emplace()
{
    {
        // can emplace non default constructible + can emplace on null variant
        NonDefaultConstructible ndc(42);
        QVariant var;
        var.emplace<NonDefaultConstructible>(42);
        QVERIFY(get_if<NonDefaultConstructible>(&var));
        QCOMPARE(get<NonDefaultConstructible>(var), ndc);
    }
    {
        // can emplace using ctor taking initializer_list
        QVariant var;
        var.emplace<std::vector<int>>({0, 1, 2, 3, 4});
        auto vecPtr = get_if<std::vector<int>>(&var);
        QVERIFY(vecPtr);
        QCOMPARE(vecPtr->size(), 5U);
        for (int i = 0; i < 5; ++i)
            QCOMPARE(vecPtr->at(size_t(i)), i);
    }
    // prequisites for the test
    QCOMPARE_LE(sizeof(std::vector<int>), sizeof(std::string));
    QCOMPARE(alignof(std::vector<int>), alignof(std::string));
    {
        // emplace can reuse storage
        auto var = QVariant::fromValue(std::string{});
        QVERIFY(var.data_ptr().is_shared);
        auto data = var.constData();
        std::vector<int> &vec = var.emplace<std::vector<int>>(3, 42);
        /* alignment is the same, so the pointer is exactly the same;
           no offset change */
        auto expected = std::vector<int>{42, 42, 42};
        QCOMPARE(get_if<std::vector<int>>(&var), &vec);
        QCOMPARE(get<std::vector<int>>(var), expected);
        QCOMPARE(var.constData(), data);
    }
    {
        // emplace can't reuse storage if the variant is shared
        auto var = QVariant::fromValue(std::string{});
        [[maybe_unused]] QVariant causesSharing = var;
        QVERIFY(var.data_ptr().is_shared);
        auto data = var.constData();
        var.emplace<std::vector<int>>(3, 42);
        auto expected = std::vector<int>{42, 42, 42};
        QVERIFY(get_if<std::vector<int>>(&var));
        QCOMPARE(get<std::vector<int>>(var), expected);
        QCOMPARE_NE(var.constData(), data);
    }
    {
        // emplace puts element into the correct place - non-shared
        QVERIFY(QVariant::Private::canUseInternalSpace(QMetaType::fromType<QString>().iface()));
        QVariant var;
        var.emplace<QString>(QChar('x'));
        QVERIFY(!var.data_ptr().is_shared);
    }
    {
        // emplace puts element into the correct place - shared
        QVERIFY(!QVariant::Private::canUseInternalSpace(QMetaType::fromType<std::string>().iface()));
        QVariant var;
        var.emplace<std::string>(42, 'x');
        QVERIFY(var.data_ptr().is_shared);
    }
    {
        // emplace does not reuse the storage if alignment is too large
        auto iface = QMetaType::fromType<LargerThanInternalQVariantStorage>().iface();
        QVERIFY(!QVariant::Private::canUseInternalSpace(iface));
        auto var = QVariant::fromValue(LargerThanInternalQVariantStorage{});
        auto data = var.constData();
        var.emplace<LargerThanInternalQVariantStorageOveraligned>();
        QCOMPARE_NE(var.constData(), data);
    }
    {
        // emplace does reuse the storage if new alignment and size are together small enough
        auto iface = QMetaType::fromType<LargerThanInternalQVariantStorageOveraligned>().iface();
        QVERIFY(!QVariant::Private::canUseInternalSpace(iface));
        auto var = QVariant::fromValue(LargerThanInternalQVariantStorageOveraligned{});
        auto data = var.constData();
        var.emplace<SmallerAlignmentEvenLargerSize>();
        // no exact match below - the alignment is after all different
        QCOMPARE_LE(quintptr(var.constData()), quintptr(data));
        QCOMPARE_LE(quintptr(var.constData()),
                    quintptr(data) + sizeof(LargerThanInternalQVariantStorageOveraligned));
    }
}

void tst_QVariant::getIf_NonDefaultConstructible()
{
    getIf_impl(NonDefaultConstructible{42});
}

void tst_QVariant::getIfSpecial()
{
    QVariant v{QString{}}; // used to be a null QVariant in Qt 5
    QCOMPARE_NE(get_if<QString>(&v), nullptr); // not anymore...
}

void tst_QVariant::get_NonDefaultConstructible()
{
    get_impl(NonDefaultConstructible{42});
}

template <typename T>
T mutate(const T &t) { return t + t; }
template <>
QTransform mutate(const QTransform &t)
{
    return t * 2;
}
template <>
NonDefaultConstructible mutate(const NonDefaultConstructible &t)
{
    return NonDefaultConstructible{t.i + t.i};
}

template <typename T>
QVariant make_null_QVariant_of_type()
{
    return QVariant(QMetaType::fromType<T>());
}

template <typename T>
void tst_QVariant::getIf_impl(T t) const
{
    QVariant v = QVariant::fromValue(t);

    QVariant null;
    QVERIFY(null.isNull());

    [[maybe_unused]]
    QVariant nulT;
    if constexpr (std::is_default_constructible_v<T>) {
        // typed null QVariants don't work with non-default-constuctable types
        nulT = make_null_QVariant_of_type<T>();
        QVERIFY(nulT.isNull());
    }

    QVariant date = QDate(2023, 3, 3);
    static_assert(!std::is_same_v<T, QDate>);

    // for behavioral comparison:
    StdVariant stdn = {}, stdv = t;

    // returns nullptr on type mismatch:
    {
        // const
        QCOMPARE_EQ(get_if<T>(&std::as_const(stdn)), nullptr);
        QCOMPARE_EQ(get_if<T>(&std::as_const(date)), nullptr);
        // mutable
        QCOMPARE_EQ(get_if<T>(&stdn), nullptr);
        QCOMPARE_EQ(get_if<T>(&date), nullptr);
    }

    // returns nullptr on null variant (QVariant only):
    {
        QCOMPARE_EQ(get_if<T>(&std::as_const(null)), nullptr);
        QCOMPARE_EQ(get_if<T>(&null), nullptr);
        if constexpr (std::is_default_constructible_v<T>) {
            // const access return nullptr
            QCOMPARE_EQ(get_if<T>(&std::as_const(nulT)), nullptr);
            // but mutable access makes typed null QVariants non-null (like data())
            QCOMPARE_NE(get_if<T>(&nulT), nullptr);
            QVERIFY(!nulT.isNull());
            nulT = make_null_QVariant_of_type<T>(); // reset to null state
        }
    }

    // const access:
    {
        auto ps = get_if<T>(&std::as_const(stdv));
        static_assert(std::is_same_v<decltype(ps), const T*>);
        QCOMPARE_NE(ps, nullptr);
        QCOMPARE_EQ(*ps, t);

        auto pv = get_if<T>(&std::as_const(v));
        static_assert(std::is_same_v<decltype(ps), const T*>);
        QCOMPARE_NE(pv, nullptr);
        QCOMPARE_EQ(*pv, t);
    }

    // mutable access:
    {
        T t2 = mutate(t);

        auto ps = get_if<T>(&stdv);
        static_assert(std::is_same_v<decltype(ps), T*>);
        QCOMPARE_NE(ps, nullptr);
        QCOMPARE_EQ(*ps, t);
        *ps = t2;
        auto ps2 = get_if<T>(&stdv);
        QCOMPARE_NE(ps2, nullptr);
        QCOMPARE_EQ(*ps2, t2);

        auto pv = get_if<T>(&v);
        static_assert(std::is_same_v<decltype(pv), T*>);
        QCOMPARE_NE(pv, nullptr);
        QCOMPARE_EQ(*pv, t);
        *pv = t2;
        auto pv2 = get_if<T>(&v);
        QCOMPARE_NE(pv2, nullptr);
        QCOMPARE_EQ(*pv2, t2);

        // typed null QVariants become non-null (data() behavior):
        if constexpr (std::is_default_constructible_v<T>) {
            QVERIFY(nulT.isNull());
            auto pn = get_if<T>(&nulT);
            QVERIFY(!nulT.isNull());
            static_assert(std::is_same_v<decltype(pn), T*>);
            QCOMPARE_NE(pn, nullptr);
            QCOMPARE_EQ(*pn, T{});
            *pn = t2;
            auto pn2 = get_if<T>(&nulT);
            QCOMPARE_NE(pn2, nullptr);
            QCOMPARE_EQ(*pn2, t2);
        }
    }
}

template <typename T>
void tst_QVariant::get_impl(T t) const
{
    QVariant v = QVariant::fromValue(t);

    // for behavioral comparison:
    StdVariant stdv = t;

    #define FOR_EACH_CVREF(op) \
        op(/*unadorned*/, &&) \
        op(&, &) \
        op(&&, &&) \
        op(const, const &&) \
        op(const &, const &) \
        op(const &&, const &&) \
        /* end */


    #define CHECK_RETURN_TYPE_OF(Variant, cvref_in, cvref_out) \
        static_assert(std::is_same_v< \
                decltype(get<T>(std::declval<Variant cvref_in >())), \
                T cvref_out \
            >); \
        /* end */
    #define CHECK_RETURN_TYPE(cvref_in, cvref_out) \
        CHECK_RETURN_TYPE_OF(StdVariant, cvref_in, cvref_out) \
        CHECK_RETURN_TYPE_OF(QVariant,   cvref_in, cvref_out) \
        /* end */
    FOR_EACH_CVREF(CHECK_RETURN_TYPE)
    #undef CHECK_RETURN_TYPE

    #undef FOR_EACH_CVREF

    // const access:
    {
        auto &&rs = get<T>(std::as_const(stdv));
        QCOMPARE_EQ(rs, t);

        auto &&rv = get<T>(std::as_const(v));
        QCOMPARE_EQ(rv, t);
    }

    // mutable access:
    {
        T t2 = mutate(t);

        auto &&rs = get<T>(stdv);
        QCOMPARE_EQ(rs, t);
        rs = t2;
        auto &&rs2 = get<T>(stdv);
        QCOMPARE_EQ(rs2, t2);

        auto &&rv = get<T>(v);
        QCOMPARE_EQ(rv, t);
        rv = t2;
        auto &&rv2 = get<T>(v);
        QCOMPARE_EQ(rv2, t2);
    }
}

QTEST_MAIN(tst_QVariant)
#include "tst_qvariant.moc"
