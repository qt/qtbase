/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
** Copyright (C) 2016 Intel Corporation.
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

#include <qvariant.h>
#include <qbitarray.h>
#include <qbytearraylist.h>
#include <qdatetime.h>
#include <qmap.h>
#include <qiodevice.h>
#include <qurl.h>
#include <qlocale.h>
#include <qdebug.h>
#include <qjsondocument.h>
#include <quuid.h>

#include <limits.h>
#include <float.h>
#include <cmath>

#include <QLinkedList>
#include <QRegularExpression>
#include <QDir>
#include <QBuffer>
#include "qnumeric.h"

#include <private/qlocale_p.h>
#include "tst_qvariant_common.h"

class CustomNonQObject;

#if defined(Q_COMPILER_CLASS_ENUM)
#define ENUM_SIZE(X) : X
#else
#define ENUM_SIZE(X)
#endif

class tst_QVariant : public QObject
{
    Q_OBJECT

public:
    tst_QVariant(QObject *parent = 0)
      : QObject(parent), customNonQObjectPointer(0)
    {

    }


    enum MetaEnumTest_Enum0 { MetaEnumTest_Enum0_dummy = 2, MetaEnumTest_Enum0_value = 42, MetaEnsureSignedEnum0 = -1 };
    Q_ENUM(MetaEnumTest_Enum0)
    enum MetaEnumTest_Enum1 { MetaEnumTest_Enum1_value = 42, MetaEnumTest_Enum1_bigValue = (Q_INT64_C(1) << 33) + 50 };
    Q_ENUM(MetaEnumTest_Enum1)

    enum MetaEnumTest_Enum3 ENUM_SIZE(qint64) { MetaEnumTest_Enum3_value = -47, MetaEnumTest_Enum3_bigValue = (Q_INT64_C(1) << 56) + 5  };
    Q_ENUM(MetaEnumTest_Enum3)
    enum MetaEnumTest_Enum4 ENUM_SIZE(quint64) { MetaEnumTest_Enum4_value = 47, MetaEnumTest_Enum4_bigValue = (Q_INT64_C(1) << 52) + 45 };
    Q_ENUM(MetaEnumTest_Enum4)
    enum MetaEnumTest_Enum5 ENUM_SIZE(uint) { MetaEnumTest_Enum5_value = 47 };
    Q_ENUM(MetaEnumTest_Enum5)
    enum MetaEnumTest_Enum6 ENUM_SIZE(uchar) { MetaEnumTest_Enum6_value = 47 };
    Q_ENUM(MetaEnumTest_Enum6)
    enum MetaEnumTest_Enum8 ENUM_SIZE(short) { MetaEnumTest_Enum8_value = 47 };
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

    void toPointF_data();
    void toPointF();

    void toRectF_data();
    void toRectF();

    void qvariant_cast_QObject_data();
    void qvariant_cast_QObject();
    void qvariant_cast_QObject_derived();
    void qvariant_cast_QObject_wrapper();
    void qvariant_cast_QSharedPointerQObject();

    void toLocale();

    void toRegExp();
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
    void operator_eq_eq_rhs();
    void compareNumbers_data() const;
    void compareNumbers() const;

    void typeName_data();
    void typeName();
    void typeToName();

    void streamInvalidVariant();

    void podUserType();

    void data();
    void constData();

    void saveLoadCustomTypes();

    void variantMap();
    void variantHash();

    void convertToQUint8() const;
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
    void compareCustomTypes() const;
    void timeToDateTime() const;
    void copyingUserTypes() const;
    void convertBoolToByteArray() const;
    void convertBoolToByteArray_data() const;
    void convertByteArrayToBool() const;
    void convertByteArrayToBool_data() const;
    void convertIterables() const;
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
    void debugStreamType_data();
    void debugStreamType();

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

    void iterateContainerElements();
    void pairElements();

    void enums();
    void metaEnums();
    void compareSanity_data();
    void compareSanity();

    void accessSequentialContainerKey();

private:
    void dataStream_data(QDataStream::Version version);
    void loadQVariantFromDataStream(QDataStream::Version version);
    void saveQVariantFromDataStream(QDataStream::Version version);

    CustomNonQObject *customNonQObjectPointer;
    QVector<QObject*> objectPointerTestData;
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

    QVariant var3(QVariant::String);
    QCOMPARE(var3.typeName(), "QString");
    QVERIFY(var3.isNull());
    QVERIFY(var3.isValid());

    QVariant var4(QVariant::Invalid);
    QCOMPARE(var4.type(), QVariant::Invalid);
    QVERIFY(var4.isNull());
    QVERIFY(!var4.isValid());

    QVariant var5(QLatin1String("hallo"));
    QCOMPARE(var5.type(), QVariant::String);
    QCOMPARE(var5.typeName(), "QString");

    QVariant var6(qlonglong(0));
    QCOMPARE(var6.type(), QVariant::LongLong);
    QCOMPARE(var6.typeName(), "qlonglong");

    QVariant var7 = 5;
    QVERIFY(var7.isValid());
    QVERIFY(!var7.isNull());
    QVariant var8;
    var8.setValue<int>(5);
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
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Trying to construct an instance of an invalid type, type id:"));
        QVariant variant(static_cast<QVariant::Type>(typeId));
        QVERIFY(!variant.isValid());
        QCOMPARE(variant.userType(), int(QMetaType::UnknownType));
    }
    {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Trying to construct an instance of an invalid type, type id:"));
        QVariant variant(typeId, /* copy */ 0);
        QVERIFY(!variant.isValid());
        QCOMPARE(variant.userType(), int(QMetaType::UnknownType));
    }
}

void tst_QVariant::copy_constructor()
{
    QVariant var7(QVariant::Int);
    QVariant var8(var7);
    QCOMPARE(var8.type(), QVariant::Int);
    QVERIFY(var8.isNull());
}

void tst_QVariant::isNull()
{
    QVariant var;
    QVERIFY( var.isNull() );

    QString str1;
    QVariant var1( str1 );
    QVERIFY( var1.isNull() );

    QVariant var2( QString::null );
    QVERIFY( var2.isNull() );

    QVariant var3( QString( "blah" ) );
    QVERIFY( !var3.isNull() );

    QVariant var4( 0 );
    QVERIFY( !var4.isNull() );

    QVariant var5 = QString();
    QVERIFY( var5.isNull() );

    QVariant var6( QString( "blah" ) );
    QVERIFY( !var6.isNull() );
    var6 = QVariant();
    QVERIFY( var6.isNull() );
    var6.convert( QVariant::String );
    QVERIFY( var6.isNull() );
    QVariant varLL( (qlonglong)0 );
    QVERIFY( !varLL.isNull() );
    QVariant var7(QString::null);
    QVERIFY(var7.isNull());
}

void tst_QVariant::swap()
{
    QVariant v1 = 1, v2 = 2.0;
    v1.swap(v2);
    QCOMPARE(v1.type(),QVariant::Double);
    QCOMPARE(v1.toDouble(),2.0);
    QCOMPARE(v2.type(),QVariant::Int);
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
        << var << N << N << Y << N << Y << Y << N << N << N << Y << N << N << Y << N << N << N << Y << N << N << N << N << N << N << N << N << N << Y << N << N << Y << Y;
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
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N;
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
        << var << N << N << Y << N << Y << Y << N << Y << Y << Y << Y << N << Y << N << Y << N << Y << N << N << N << N << N << N << N << N << N << Y << Y << Y << Y << Y;
   var = QVariant(QStringList("entry"));
    QTest::newRow("StringList")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << N << N << N << N << N << N << N << N << N << Y << Y << N << N << N;
    var = QVariant(QTime());
    QTest::newRow("Time")
        << var << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << N << Y << N << Y << N << N;
    var = QVariant((uint)1);
    QTest::newRow("UInt")
        << var << N << N << Y << N << Y << N << N << N << N << Y << N << N << Y << N << N << N << Y << N << N << N << N << N << N << N << N << N << Y << N << N << Y << Y;
    var = QVariant((int)1);
    QTest::newRow("Int")
        << var << N << N << Y << N << Y << N << N << N << N << Y << N << N << Y << N << Y << N << Y << N << N << N << N << N << N << N << N << N << Y << N << N << Y << Y;
    var = QVariant((qulonglong)1);
    QTest::newRow("ULongLong")
        << var << N << N << Y << N << Y << N << N << N << N << Y << N << N << Y << N << N << N << Y << N << N << N << N << N << N << N << N << N << Y << N << N << Y << Y;
    var = QVariant::fromValue('a');
    QTest::newRow("Char")
        << var << N << N << Y << N << Y << N << N << N << N << Y << N << N << Y << N << N << N << Y << N << N << N << N << N << N << N << N << N << Y << N << N << Y << Y;
    var = QVariant::fromValue<signed char>(-1);
    QTest::newRow("SChar")
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

    TST_QVARIANT_CANCONVERT_COMPARE_DATA

    // Invalid type ids
    QCOMPARE(val.canConvert(-1), false);
    QCOMPARE(val.canConvert(-23), false);
    QCOMPARE(val.canConvert(-23876), false);
    QCOMPARE(val.canConvert(23876), false);
}

void tst_QVariant::toInt_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<int>("result");
    QTest::addColumn<bool>("valueOK");

    QTest::newRow( "invalid" ) << QVariant()  << 0 << false;
    QTest::newRow( "int" ) << QVariant( 123 ) << 123 << true;
    QTest::newRow( "char" ) << QVariant::fromValue('a') << int('a') << true;
    signed char signedChar = -13;
    QTest::newRow( "signed char" ) << QVariant::fromValue(signedChar) << -13 << true;
    QTest::newRow( "double" ) << QVariant( 3.1415927 ) << 3 << true;
    QTest::newRow( "float" ) << QVariant( 3.1415927f ) << 3 << true;
    QTest::newRow( "uint" ) << QVariant( 123u ) << 123 << true;
    QTest::newRow( "int-string" ) << QVariant( QString("123") ) << 123 << true;
    QTest::newRow( "string" ) << QVariant( QString("Unicode String") ) << 0 << false;
    QTest::newRow( "longlong0" ) << QVariant( (qlonglong)34 ) << 34 << true;
    QTest::newRow( "longlong1" ) << QVariant( intMax1 ) << (int)INT_MIN << true;
    QTest::newRow( "ulonglong0" ) << QVariant( (qulonglong)34 ) << 34 << true;
    QTest::newRow( "ulonglong1" ) << QVariant( uintMax1 ) << 0 << true;
    QTest::newRow( "signedint" ) << QVariant( -123 ) << -123 << true;
    QTest::newRow( "signeddouble" ) << QVariant( -3.1415927 ) << -3 << true;
    QTest::newRow( "signedfloat" ) << QVariant( -3.1415927f ) << -3 << true;
    QTest::newRow( "signedint-string" ) << QVariant( QString("-123") ) << -123 << true;
    QTest::newRow( "signedlonglong0" ) << QVariant( (qlonglong)-34 ) << -34 << true;
    QTest::newRow( "QChar" ) << QVariant(QChar('a')) << int('a') << true;
    QByteArray bytearray(4, ' ');
    bytearray[0] = 'T';
    bytearray[1] = 'e';
    bytearray[2] = 's';
    bytearray[3] = 't';
    QTest::newRow( "QByteArray1" ) << QVariant( bytearray ) << 0 << false;
    bytearray[0] = '4';
    bytearray[1] = '5';
    bytearray[2] = '0';
    bytearray[3] = '0';
    QTest::newRow( "QByteArray2" ) << QVariant( bytearray ) << 4500 << true;
    QTest::newRow("int-QJsonValue") << QVariant(QJsonValue(321)) << 321 << true;
    QTest::newRow("undefined-QJsonValue") << QVariant(QJsonValue(QJsonValue::Undefined)) << 0 << false;
}

void tst_QVariant::toInt()
{
    QFETCH( QVariant, value );
    QFETCH( int, result );
    QFETCH( bool, valueOK );
    QVERIFY( value.isValid() == value.canConvert( QVariant::Int ) );
    bool ok;
    int i = value.toInt( &ok );
    QCOMPARE( i, result );
    QVERIFY( ok == valueOK );
}

void tst_QVariant::toUInt_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<uint>("result");
    QTest::addColumn<bool>("valueOK");

    QTest::newRow( "int" ) << QVariant( 123 ) << (uint)123 << true;
    QTest::newRow( "char" ) << QVariant::fromValue('a') << uint('a') << true;
    signed char signedChar = 12;
    QTest::newRow( "signed char" ) << QVariant::fromValue(signedChar) << uint(12) << true;
    QTest::newRow( "double" ) << QVariant( 3.1415927 ) << (uint)3 << true;
    QTest::newRow( "float" ) << QVariant( 3.1415927f ) << (uint)3 << true;
    QTest::newRow( "uint" ) << QVariant( 123u ) << (uint)123 << true;
    QTest::newRow( "int-string" ) << QVariant( QString("123") ) << (uint)123 << true;
    QTest::newRow( "string" ) << QVariant( QString("Unicode String") ) << (uint)0 << false;
    QTest::newRow( "string2" ) << QVariant( QString("4") ) << (uint)4 << true;
    QTest::newRow( "longlong0" ) << QVariant( (qlonglong)34 ) << (uint)34 << true;
    QTest::newRow( "longlong1" ) << QVariant( intMax1 ) << (uint)INT_MIN << true;
    QTest::newRow( "ulonglong0" ) << QVariant( (qulonglong)34 ) << (uint)34 << true;
    QTest::newRow( "ulonglong1" ) << QVariant( uintMax1 ) << (uint)0 << true;
    QTest::newRow( "negativeint" ) << QVariant( -123 ) << (uint)-123 << true;
    QTest::newRow( "negativedouble" ) << QVariant( -3.1415927 ) << (uint)-3 << true;
    QTest::newRow( "negativefloat" ) << QVariant( -3.1415927f ) << (uint)-3 << true;
    QTest::newRow( "negativeint-string" ) << QVariant( QString("-123") ) << (uint)0 << false;
    QTest::newRow( "negativelonglong0" ) << QVariant( (qlonglong)-34 ) << (uint)-34 << true;
    QTest::newRow( "QChar" ) << QVariant(QChar('a')) << uint('a') << true;
    QByteArray bytearray(4, ' ');
    bytearray[0] = '4';
    bytearray[1] = '3';
    bytearray[2] = '2';
    bytearray[3] = '1';
    QTest::newRow( "QByteArray" ) << QVariant( bytearray ) << (uint)4321 << true;
    QTest::newRow("int-QJsonValue") << QVariant(QJsonValue(321)) << (uint)321 << true;
    QTest::newRow("null-QJsonValue") << QVariant(QJsonValue(QJsonValue::Null)) << (uint)0 << false;
}

void tst_QVariant::toUInt()
{
    QFETCH( QVariant, value );
    QFETCH( uint, result );
    QFETCH( bool, valueOK );
    QVERIFY( value.isValid() );
    QVERIFY( value.canConvert( QVariant::UInt ) );

    bool ok;
    uint i = value.toUInt( &ok );
    QVERIFY( ok == valueOK );
    QCOMPARE( i, result );
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
    QVERIFY( value.canConvert( QVariant::Size ) );

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
    QVERIFY( value.canConvert( QVariant::SizeF ) );

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
    QVERIFY( value.canConvert( QVariant::Line ) );

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
    QVERIFY( value.canConvert( QVariant::LineF ) );

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
    QVERIFY( value.canConvert( QVariant::Point ) );
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
    QVERIFY( value.canConvert( QVariant::Rect ) );
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
    QVERIFY( value.canConvert( QVariant::Char ) );

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
    QTest::newRow( "double1" ) << QVariant( 3.1415927 ) << true;
    QTest::newRow( "float1" ) << QVariant( 3.1415927f ) << true;
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
}

void tst_QVariant::toBool()
{
    QFETCH( QVariant, value );
    QFETCH( bool, result );
    QVERIFY( value.isValid() );
    QVERIFY( value.canConvert( QVariant::Bool ) );

    bool i = value.toBool();
    QCOMPARE( i, result );
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
    QVERIFY( value.canConvert( QVariant::PointF ) );
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
    QVERIFY( value.canConvert( QVariant::RectF ) );
    QRectF d = value.toRectF();
    QCOMPARE( d, result );
}

void tst_QVariant::toDouble_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<double>("result");
    QTest::addColumn<bool>("valueOK");

    QByteArray bytearray(4, ' ');
    bytearray[0] = '3';
    bytearray[1] = '2';
    bytearray[2] = '.';
    bytearray[3] = '1';
    QTest::newRow( "bytearray" ) << QVariant( bytearray ) << 32.1 << true;
    QTest::newRow("double-QJsonValue") << QVariant(QJsonValue(32.1)) << 32.1 << true;
    QTest::newRow("null-QJsonValue") << QVariant(QJsonValue(QJsonValue::Null)) << 0.0 << false;
}

void tst_QVariant::toDouble()
{
    QFETCH( QVariant, value );
    QFETCH( double, result );
    QFETCH( bool, valueOK );
    QVERIFY( value.isValid() );
    QVERIFY( value.canConvert( QVariant::Double ) );
    bool ok;
    double d = value.toDouble( &ok );
    QCOMPARE( d, result );
    QVERIFY( ok == valueOK );
}

void tst_QVariant::toFloat_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<float>("result");
    QTest::addColumn<bool>("valueOK");

    QByteArray bytearray(4, ' ');
    bytearray[0] = '3';
    bytearray[1] = '2';
    bytearray[2] = '.';
    bytearray[3] = '1';
    QTest::newRow("QByteArray") << QVariant(bytearray) << float(32.1) << true;
    QTest::newRow("double-QJsonValue") << QVariant(QJsonValue(32.1)) << float(32.1) << true;
    QTest::newRow("undefined-QJsonValue") << QVariant(QJsonValue(QJsonValue::Undefined)) << float(0.0) << false;
}

void tst_QVariant::toFloat()
{
    QFETCH(QVariant, value );
    QFETCH(float, result);
    QFETCH(bool, valueOK);
    QVERIFY(value.isValid());
    QVERIFY(value.canConvert(QMetaType::Float));
    bool ok;
    float d = value.toFloat(&ok);
    QCOMPARE(d, result);
    QCOMPARE(ok, valueOK);
}

void tst_QVariant::toLongLong_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<qlonglong>("result");
    QTest::addColumn<bool>("valueOK");

    QTest::newRow( "int0" ) << QVariant( 123 ) << (qlonglong)123 << true;
    QTest::newRow( "double" ) << QVariant( 3.1415927 ) << (qlonglong)3 << true;
    QTest::newRow( "float" ) << QVariant( 3.1415927f ) << (qlonglong)3 << true;
    QTest::newRow( "uint" ) << QVariant( 123u ) << (qlonglong)123 << true;
    QTest::newRow( "int-string" ) << QVariant( QString("123") )
                               << (qlonglong)123 << true;
    QTest::newRow( "string" ) << QVariant( QString("Unicode fun") ) << (qlonglong)0
                           << false;
    QTest::newRow( "longlong" ) << QVariant( intMax1 ) << intMax1 << true;
    QTest::newRow( "ulonglong" ) << QVariant( uintMax1 ) << (qlonglong)uintMax1 << true;
    QTest::newRow( "QChar" ) << QVariant(QChar('a')) << qlonglong('a') << true;
    QByteArray bytearray(4, ' ');
    bytearray[0] = '3';
    bytearray[1] = '2';
    bytearray[2] = '0';
    bytearray[3] = '0';
    QTest::newRow( "QByteArray" ) << QVariant( bytearray ) << (qlonglong) 3200 << true;
    QTest::newRow("int-QJsonValue") << QVariant(QJsonValue(321)) << (qlonglong)321 << true;
    QTest::newRow("string-QJsonValue") << QVariant(QJsonValue(QString("string"))) << (qlonglong)0 << false;

    qint64 value64 = (Q_INT64_C(12) << 35) + 8;
    QTest::newRow("qint64") << QVariant::fromValue(value64) << qlonglong(value64) << true;
    QTest::newRow("-qint64") << QVariant::fromValue(-value64) << qlonglong(-value64) << true;
    QTest::newRow("long") << QVariant::fromValue(long(464646)) << qlonglong(464646)  << true;
    QTest::newRow("LONG_MAX") << QVariant::fromValue( LONG_MAX ) << qlonglong(LONG_MAX)  << true;
    QTest::newRow("LONG_MIN") << QVariant::fromValue( LONG_MIN ) << qlonglong(LONG_MIN)  << true;

    QTest::newRow( "short" ) << QVariant(short(12)) << qlonglong(12) << true;
    QTest::newRow( "-short" ) << QVariant(short(-24)) << qlonglong(-24) << true;
    QTest::newRow( "ushort" ) << QVariant(ushort(15)) << qlonglong(15) << true;
}

void tst_QVariant::toLongLong()
{
    QFETCH( QVariant, value );
    QFETCH( qlonglong, result );
    QFETCH( bool, valueOK );
    QVERIFY( value.isValid() );
    QVERIFY( value.canConvert( QVariant::LongLong ) );
    bool ok;
    qlonglong ll = value.toLongLong( &ok );
    QCOMPARE( ll, result );
    QVERIFY( ok == valueOK );
}

void tst_QVariant::toULongLong_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<qulonglong>("result");
    QTest::addColumn<bool>("valueOK");

    QTest::newRow( "int0" ) << QVariant( 123 ) << (qulonglong)123 << true;
    QTest::newRow( "double" ) << QVariant( 3.1415927 ) << (qulonglong)3 << true;
    QTest::newRow( "float" ) << QVariant( 3.1415927f ) << (qulonglong)3 << true;
    QTest::newRow( "uint" ) << QVariant( 123u ) << (qulonglong)123 << true;
    QTest::newRow( "int-string" ) << QVariant( QString("123") )
                               << (qulonglong)123 << true;
    QTest::newRow( "string" ) << QVariant( QString("Unicode fun") ) << (qulonglong)0
                           << false;
    QTest::newRow( "ulonglong-string" ) << QVariant( QString("18446744073709551615") )
                                     << Q_UINT64_C(18446744073709551615)
                                     << true;
    QTest::newRow( "bytaa-string" ) << QVariant( QString("18446744073709551615") )
                                     << Q_UINT64_C(18446744073709551615)
                                     << true;
    QTest::newRow( "longlong" ) << QVariant( intMax1 ) << (qulonglong)intMax1 << true;
    QTest::newRow( "ulonglong" ) << QVariant( uintMax1 ) << uintMax1 << true;
    QTest::newRow( "QChar" ) << QVariant(QChar('a')) << qulonglong('a') << true;
    QByteArray bytearray(4, ' ');
    bytearray[0] = '3';
    bytearray[1] = '2';
    bytearray[2] = '0';
    bytearray[3] = '1';
    QTest::newRow( "QByteArray" ) << QVariant( bytearray ) << (qulonglong) 3201 << true;
    QTest::newRow("int-QJsonValue") << QVariant(QJsonValue(321)) << (qulonglong)321 << true;
    QTest::newRow("bool-QJsonValue") << QVariant(QJsonValue(true)) << (qulonglong)0 << false;

    quint64 value64 = (Q_INT64_C(12) << 35) + 8;
    QTest::newRow("qint64") << QVariant::fromValue(value64) << qulonglong(value64) << true;
    QTest::newRow("long") << QVariant::fromValue(long(464646)) << qulonglong(464646)  << true;
    QTest::newRow("LONG_MAX") << QVariant::fromValue( LONG_MAX ) << qulonglong(LONG_MAX)  << true;
    QTest::newRow("ULONG_MAX") << QVariant::fromValue( ULONG_MAX ) << qulonglong(ULONG_MAX)  << true;
    QTest::newRow( "short" ) << QVariant(short(12)) << qulonglong(12) << true;
    QTest::newRow( "-short" ) << QVariant(short(-24)) << qulonglong(-24) << true;
    QTest::newRow( "ushort" ) << QVariant(ushort(15)) << qulonglong(15) << true;
}

void tst_QVariant::toULongLong()
{
    QFETCH( QVariant, value );
    QFETCH( qulonglong, result );
    QFETCH( bool, valueOK );
    QVERIFY( value.isValid() );
    QVERIFY( value.canConvert( QVariant::ULongLong ) );
    bool ok;
    qulonglong ll = value.toULongLong( &ok );
    QCOMPARE( ll, result );
    QVERIFY( ok == valueOK );
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
}

void tst_QVariant::toByteArray()
{
    QFETCH( QVariant, value );
    QFETCH( QByteArray, result );
    QVERIFY( value.isValid() );
    QVERIFY( value.canConvert( QVariant::ByteArray ) );
    QByteArray ba = value.toByteArray();
    QCOMPARE( ba, result );
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
    QTest::newRow( "qtime" ) << QVariant( QTime( 12, 34, 56 ) ) << QString( "12:34:56" );
    QTest::newRow( "qdatetime" ) << QVariant( QDateTime( QDate( 2002, 1, 1 ), QTime( 12, 34, 56 ) ) ) << QString( "2002-01-01T12:34:56" );
    QTest::newRow( "llong" ) << QVariant( (qlonglong)Q_INT64_C(123456789012) ) <<
        QString( "123456789012" );
    QTest::newRow("QJsonValue") << QVariant(QJsonValue(QString("hello"))) << QString("hello");
    QTest::newRow("QJsonValue(Null)") << QVariant(QJsonValue(QJsonValue::Null)) << QString();
}

void tst_QVariant::toString()
{
    QFETCH( QVariant, value );
    QFETCH( QString, result );
    QVERIFY( value.isValid() );
    QVERIFY( value.canConvert( QVariant::String ) );
    QString str = value.toString();
    QCOMPARE( str, result );
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
    QVERIFY( value.canConvert( QVariant::Date ) );
    QCOMPARE( value.toDate(), result );
}

void tst_QVariant::toTime_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QTime>("result");

    QTest::newRow( "qtime" ) << QVariant( QTime( 12, 34, 56 ) ) << QTime( 12, 34, 56 );
    QTest::newRow( "qdatetime" ) << QVariant( QDateTime( QDate( 2002, 10, 10 ), QTime( 12, 34, 56 ) ) ) << QTime( 12, 34, 56 );
    QTest::newRow( "qstring" ) << QVariant( QString( "12:34:56" ) ) << QTime( 12, 34, 56 );
}

void tst_QVariant::toTime()
{
    QFETCH( QVariant, value );
    QFETCH( QTime, result );
    QVERIFY( value.isValid() );
    QVERIFY( value.canConvert( QVariant::Time ) );
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
}

void tst_QVariant::toDateTime()
{
    QFETCH( QVariant, value );
    QFETCH( QDateTime, result );
    QVERIFY( value.isValid() );
    QVERIFY( value.canConvert( QVariant::DateTime ) );
    QCOMPARE( value.toDateTime(), result );
}

void tst_QVariant::toLocale()
{
    QVariant variant;
    QLocale loc = variant.toLocale();
    variant = QLocale::system();
    loc = variant.toLocale();
}

void tst_QVariant::toRegExp()
{
    QVariant variant;
    QRegExp rx = variant.toRegExp();
    variant = QRegExp("foo");
    rx = variant.toRegExp();
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
Q_DECLARE_METATYPE(CustomStreamableClass);

QDataStream &operator<<(QDataStream &out, const CustomStreamableClass &myObj)
{
    return out << myObj.i;
}

QDataStream &operator>>(QDataStream &in, CustomStreamableClass &myObj)
{
    return in >> myObj.i;
}

void tst_QVariant::writeToReadFromDataStream_data()
{
    qRegisterMetaTypeStreamOperators<CustomStreamableClass>();

    QTest::addColumn<QVariant>("writeVariant");
    QTest::addColumn<bool>("isNull");
    {
        QVariantList valuelist;
        valuelist << QVariant( 1 ) << QVariant( QString("Two") ) << QVariant( 3.45 );
        QVariant var(valuelist);
        QTest::newRow( "list_valid" ) << var << false;
    }

    QTest::newRow( "invalid" ) << QVariant() << true;
    QTest::newRow( "bitarray_invalid" ) << QVariant( QBitArray() ) << true;
    QBitArray bitarray( 3 );
    bitarray[0] = 0;
    bitarray[1] = 1;
    bitarray[2] = 0;
    QTest::newRow( "bitarray_valid" ) << QVariant( bitarray ) << false;
    QTest::newRow( "bytearray_invalid" ) << QVariant( QByteArray() ) << true;
    QTest::newRow( "int_invalid") << QVariant(QVariant::Int) << true;
    QByteArray bytearray(5, ' ');
    bytearray[0] = 'T';
    bytearray[1] = 'e';
    bytearray[2] = 's';
    bytearray[3] = 't';
    bytearray[4] = '\0';
    QTest::newRow( "bytearray_valid" ) << QVariant( bytearray ) << false;
    QTest::newRow( "date_invalid" ) << QVariant( QDate() ) << true;
    QTest::newRow( "date_valid" ) << QVariant( QDate( 2002, 07, 06 ) ) << false;
    QTest::newRow( "datetime_invalid" ) << QVariant( QDateTime() ) << true;
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
    QTest::newRow( "point_invalid" ) << QVariant::fromValue( QPoint() ) << true;
    QTest::newRow( "point_valid" ) << QVariant::fromValue( QPoint( 10, 10 ) ) << false;
    QTest::newRow( "rect_invalid" ) << QVariant( QRect() ) << true;
    QTest::newRow( "rect_valid" ) << QVariant( QRect( 10, 10, 20, 20 ) ) << false;
    QTest::newRow( "size_invalid" ) << QVariant( QSize( 0, 0 ) ) << true;
    QTest::newRow( "size_valid" ) << QVariant( QSize( 10, 10 ) ) << false;
    QTest::newRow( "string_invalid" ) << QVariant( QString() ) << true;
    QTest::newRow( "string_valid" ) << QVariant( QString( "Test" ) ) << false;
    QStringList stringlist;
    stringlist << "One" << "Two" << "Three";
    QTest::newRow( "stringlist_valid" ) << QVariant( stringlist ) << false;
    QTest::newRow( "time_invalid" ) << QVariant( QTime() ) << true;
    QTest::newRow( "time_valid" ) << QVariant( QTime( 14, 0, 0 ) ) << false;
    QTest::newRow( "uint_valid" ) << QVariant( (uint)123 ) << false;
    QTest::newRow( "qchar" ) << QVariant(QChar('a')) << false;
    QTest::newRow( "qchar_null" ) << QVariant(QChar(0)) << true;
    QTest::newRow( "regexp" ) << QVariant(QRegExp("foo", Qt::CaseInsensitive)) << false;
    QTest::newRow( "regexp_empty" ) << QVariant(QRegExp()) << false;
    QTest::newRow( "regularexpression" ) << QVariant(QRegularExpression("abc.*def")) << false;
    QTest::newRow( "regularexpression_empty" ) << QVariant(QRegularExpression()) << false;

    // types known to QMetaType, but not part of QVariant::Type
    QTest::newRow("QMetaType::Long invalid") << QVariant(QMetaType::Long, (void *) 0) << false;
    long longInt = -1l;
    QTest::newRow("QMetaType::Long") << QVariant(QMetaType::Long, &longInt) << false;
    QTest::newRow("QMetaType::Short invalid") << QVariant(QMetaType::Short, (void *) 0) << false;
    short shortInt = 1;
    QTest::newRow("QMetaType::Short") << QVariant(QMetaType::Short, &shortInt) << false;
    QTest::newRow("QMetaType::Char invalid") << QVariant(QMetaType::Char, (void *) 0) << false;
    char ch = 'c';
    QTest::newRow("QMetaType::Char") << QVariant(QMetaType::Char, &ch) << false;
    QTest::newRow("QMetaType::ULong invalid") << QVariant(QMetaType::ULong, (void *) 0) << false;
    ulong ulongInt = 1ul;
    QTest::newRow("QMetaType::ULong") << QVariant(QMetaType::ULong, &ulongInt) << false;
    QTest::newRow("QMetaType::UShort invalid") << QVariant(QMetaType::UShort, (void *) 0) << false;
    ushort ushortInt = 1u;
    QTest::newRow("QMetaType::UShort") << QVariant(QMetaType::UShort, &ushortInt) << false;
    QTest::newRow("QMetaType::UChar invalid") << QVariant(QMetaType::UChar, (void *) 0) << false;
    uchar uch = 0xf0;
    QTest::newRow("QMetaType::UChar") << QVariant(QMetaType::UChar, &uch) << false;
    QTest::newRow("QMetaType::Float invalid") << QVariant(QMetaType::Float, (void *) 0) << false;
    float f = 1.234f;
    QTest::newRow("QMetaType::Float") << QVariant(QMetaType::Float, &f) << false;
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
    if (writeType == qMetaTypeId<CustomStreamableClass>())
        QCOMPARE(qvariant_cast<CustomStreamableClass>(readVariant), qvariant_cast<CustomStreamableClass>(writeVariant));
    else if ( writeType != QVariant::Invalid && writeType != QVariant::Bitmap && writeType != QVariant::Pixmap
        && writeType != QVariant::Image) {
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

    QByteArray errorMessage("Trying to construct an instance of an invalid type, type id: ");
    errorMessage.append(QString::number(typeId, 10));

    QTest::ignoreMessage(QtWarningMsg, errorMessage.constData());
    QByteArray settingsHex("000000");
    settingsHex.append(QString::number(typeId, 16));
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
    QCOMPARE(v.type(), QVariant::Invalid);
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
    QTest::newRow( "nullint" ) << i0 << QVariant(QVariant::Int) << true;

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

    QTest::newRow( "double_int" ) << QVariant(42.0) << QVariant(42) << true;
    QTest::newRow( "float_int" ) << QVariant(42.f) << QVariant(42) << true;
    QTest::newRow( "mInt_mIntString" ) << mInt << mIntString << true;
    QTest::newRow( "mIntString_mInt" ) << mIntString << mInt << true;
    QTest::newRow( "mInt_mIntQString" ) << mInt << mIntQString << true;
    QTest::newRow( "mIntQString_mInt" ) << mIntQString << mInt << true;

    QTest::newRow( "mUInt_mUIntString" ) << mUInt << mUIntString << true;
    QTest::newRow( "mUIntString_mUInt" ) << mUIntString << mUInt << true;
    QTest::newRow( "mUInt_mUIntQString" ) << mUInt << mUIntQString << true;
    QTest::newRow( "mUIntQString_mUInt" ) << mUIntQString << mUInt << true;

    QTest::newRow( "mDouble_mDoubleString" ) << mDouble << mDoubleString << true;
    QTest::newRow( "mDoubleString_mDouble" ) << mDoubleString << mDouble << true;
    QTest::newRow( "mDouble_mDoubleQString" ) << mDouble << mDoubleQString << true;
    QTest::newRow( "mDoubleQString_mDouble" ) << mDoubleQString << mDouble << true;

    QTest::newRow( "mFloat_mFloatString" ) << mFloat << mFloatString << true;
    QTest::newRow( "mFloatString_mFloat" ) << mFloatString << mFloat << true;
    QTest::newRow( "mFloat_mFloatQString" ) << mFloat << mFloatQString << true;
    QTest::newRow( "mFloatQString_mFloat" ) << mFloatQString << mFloat << true;

    QTest::newRow( "mLongLong_mLongLongString" ) << mLongLong << mLongLongString << true;
    QTest::newRow( "mLongLongString_mLongLong" ) << mLongLongString << mLongLong << true;
    QTest::newRow( "mLongLong_mLongLongQString" ) << mLongLong << mLongLongQString << true;
    QTest::newRow( "mLongLongQString_mLongLong" ) << mLongLongQString << mLongLong << true;

    QTest::newRow( "mULongLong_mULongLongString" ) << mULongLong << mULongLongString << true;
    QTest::newRow( "mULongLongString_mULongLong" ) << mULongLongString << mULongLong << true;
    QTest::newRow( "mULongLong_mULongLongQString" ) << mULongLong << mULongLongQString << true;
    QTest::newRow( "mULongLongQString_mULongLong" ) << mULongLongQString << mULongLong << true;

    QTest::newRow( "mBool_mBoolString" ) << mBool << mBoolString << true;
    QTest::newRow( "mBoolString_mBool" ) << mBoolString << mBool << true;
    QTest::newRow( "mBool_mBoolQString" ) << mBool << mBoolQString << true;
    QTest::newRow( "mBoolQString_mBool" ) << mBoolQString << mBool << true;

    QTest::newRow("ba2qstring") << QVariant(QLatin1String("hallo")) << QVariant(QString("hallo")) << true;
    QTest::newRow("qstring2ba") << QVariant(QString("hallo")) << QVariant(QLatin1String("hallo")) << true;
    QTest::newRow("char_char") << QVariant(QChar('a')) << QVariant(QChar('a')) << true;
    QTest::newRow("char_char2") << QVariant(QChar('a')) << QVariant(QChar('b')) << false;

    QTest::newRow("invalidConversion") << QVariant(QString("bubu")) << QVariant(0) << false;
    QTest::newRow("invalidConversionR") << QVariant(0) << QVariant(QString("bubu")) << false;
    // ### many other combinations missing

    {
        // QUuid can convert to QString, but not the opposite
        QUuid uuid(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
        QTest::newRow("uuidstring") << QVariant(uuid) << QVariant(uuid.toString()) << true;
        QTest::newRow("stringuuid") << QVariant(uuid.toString()) << QVariant(uuid) << true;
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
    QCOMPARE( left == right, equal );
}

void tst_QVariant::operator_eq_eq_rhs()
{
    QVariant v = 42;

    QVERIFY(v == 42);
    QVERIFY(42 == v);

#if 0
    /* This should _not_ compile */
    QStringList list;
    QDateTime dt;

    QVERIFY(dt == list);
#endif
}

void tst_QVariant::compareNumbers_data() const
{
    typedef signed char schar;
    QTest::addColumn<QVariant>("v1");
    QTest::addColumn<QVariant>("v2");
    QTest::addColumn<int>("expected");

    // sanity checking: same types
    QTest::newRow("bool1") << QVariant(false) << QVariant(false) << 0;
    QTest::newRow("bool2") << QVariant(true) << QVariant(true) << 0;
    QTest::newRow("bool3") << QVariant(false) << QVariant(true) << -1;
    QTest::newRow("bool4") << QVariant(true) << QVariant(false) << +1;

    QTest::newRow("char1") << qVariantFromValue(char(0)) << qVariantFromValue(char(0)) << 0;
    QTest::newRow("char2") << qVariantFromValue(CHAR_MAX) << qVariantFromValue(CHAR_MAX) << 0;
    QTest::newRow("char3") << qVariantFromValue(CHAR_MIN) << qVariantFromValue(CHAR_MIN) << 0;
    QTest::newRow("char4") << qVariantFromValue(CHAR_MIN) << qVariantFromValue(CHAR_MAX) << -1;
    QTest::newRow("char5") << qVariantFromValue(CHAR_MAX) << qVariantFromValue(CHAR_MIN) << +1;

    QTest::newRow("schar1") << qVariantFromValue(schar(0)) << qVariantFromValue(schar(0)) << 0;
    QTest::newRow("schar2") << qVariantFromValue(SCHAR_MAX) << qVariantFromValue(SCHAR_MAX) << 0;
    QTest::newRow("schar3") << qVariantFromValue(SCHAR_MIN) << qVariantFromValue(SCHAR_MIN) << 0;
    QTest::newRow("schar4") << qVariantFromValue(SCHAR_MIN) << qVariantFromValue(SCHAR_MAX) << -1;
    QTest::newRow("schar5") << qVariantFromValue(SCHAR_MAX) << qVariantFromValue(SCHAR_MIN) << +1;

    QTest::newRow("uchar1") << qVariantFromValue(uchar(0)) << qVariantFromValue(uchar(0)) << 0;
    QTest::newRow("uchar2") << qVariantFromValue(UCHAR_MAX) << qVariantFromValue(UCHAR_MAX) << 0;
    QTest::newRow("uchar3") << qVariantFromValue(uchar(0)) << qVariantFromValue(UCHAR_MAX) << -1;
    QTest::newRow("uchar4") << qVariantFromValue(UCHAR_MAX) << qVariantFromValue(uchar(0)) << +1;

    QTest::newRow("short1") << qVariantFromValue(short(0)) << qVariantFromValue(short(0)) << 0;
    QTest::newRow("short2") << qVariantFromValue(SHRT_MAX) << qVariantFromValue(SHRT_MAX) << 0;
    QTest::newRow("short3") << qVariantFromValue(SHRT_MIN) << qVariantFromValue(SHRT_MIN) << 0;
    QTest::newRow("short4") << qVariantFromValue(SHRT_MIN) << qVariantFromValue(SHRT_MAX) << -1;
    QTest::newRow("short5") << qVariantFromValue(SHRT_MAX) << qVariantFromValue(SHRT_MIN) << +1;

    QTest::newRow("ushort1") << qVariantFromValue(ushort(0)) << qVariantFromValue(ushort(0)) << 0;
    QTest::newRow("ushort2") << qVariantFromValue(USHRT_MAX) << qVariantFromValue(USHRT_MAX) << 0;
    QTest::newRow("ushort3") << qVariantFromValue(ushort(0)) << qVariantFromValue(USHRT_MAX) << -1;
    QTest::newRow("ushort4") << qVariantFromValue(USHRT_MAX) << qVariantFromValue(ushort(0)) << +1;

    QTest::newRow("int1") << qVariantFromValue(int(0)) << qVariantFromValue(int(0)) << 0;
    QTest::newRow("int2") << qVariantFromValue(INT_MAX) << qVariantFromValue(INT_MAX) << 0;
    QTest::newRow("int3") << qVariantFromValue(INT_MIN) << qVariantFromValue(INT_MIN) << 0;
    QTest::newRow("int4") << qVariantFromValue(INT_MIN) << qVariantFromValue(INT_MAX) << -1;
    QTest::newRow("int5") << qVariantFromValue(INT_MAX) << qVariantFromValue(INT_MIN) << +1;

    QTest::newRow("uint1") << qVariantFromValue(uint(0)) << qVariantFromValue(uint(0)) << 0;
    QTest::newRow("uint2") << qVariantFromValue(UINT_MAX) << qVariantFromValue(UINT_MAX) << 0;
    QTest::newRow("uint3") << qVariantFromValue(uint(0)) << qVariantFromValue(UINT_MAX) << -1;
    QTest::newRow("uint4") << qVariantFromValue(UINT_MAX) << qVariantFromValue(uint(0)) << +1;

    QTest::newRow("long1") << qVariantFromValue(long(0)) << qVariantFromValue(long(0)) << 0;
    QTest::newRow("long2") << qVariantFromValue(LONG_MAX) << qVariantFromValue(LONG_MAX) << 0;
    QTest::newRow("long3") << qVariantFromValue(LONG_MIN) << qVariantFromValue(LONG_MIN) << 0;
    QTest::newRow("long4") << qVariantFromValue(LONG_MIN) << qVariantFromValue(LONG_MAX) << -1;
    QTest::newRow("long5") << qVariantFromValue(LONG_MAX) << qVariantFromValue(LONG_MIN) << +1;

    QTest::newRow("ulong1") << qVariantFromValue(ulong(0)) << qVariantFromValue(ulong(0)) << 0;
    QTest::newRow("ulong2") << qVariantFromValue(ULONG_MAX) << qVariantFromValue(ULONG_MAX) << 0;
    QTest::newRow("ulong3") << qVariantFromValue(ulong(0)) << qVariantFromValue(ULONG_MAX) << -1;
    QTest::newRow("ulong4") << qVariantFromValue(ULONG_MAX) << qVariantFromValue(ulong(0)) << +1;

    QTest::newRow("llong1") << qVariantFromValue(qlonglong(0)) << qVariantFromValue(qlonglong(0)) << 0;
    QTest::newRow("llong2") << qVariantFromValue(LLONG_MAX) << qVariantFromValue(LLONG_MAX) << 0;
    QTest::newRow("llong3") << qVariantFromValue(LLONG_MIN) << qVariantFromValue(LLONG_MIN) << 0;
    QTest::newRow("llong4") << qVariantFromValue(LLONG_MIN) << qVariantFromValue(LLONG_MAX) << -1;
    QTest::newRow("llong5") << qVariantFromValue(LLONG_MAX) << qVariantFromValue(LLONG_MIN) << +1;

    QTest::newRow("ullong1") << qVariantFromValue(qulonglong(0)) << qVariantFromValue(qulonglong(0)) << 0;
    QTest::newRow("ullong2") << qVariantFromValue(ULLONG_MAX) << qVariantFromValue(ULLONG_MAX) << 0;
    QTest::newRow("ullong3") << qVariantFromValue(qulonglong(0)) << qVariantFromValue(ULLONG_MAX) << -1;
    QTest::newRow("ullong4") << qVariantFromValue(ULLONG_MAX) << qVariantFromValue(qulonglong(0)) << +1;

    QTest::newRow("float1") << qVariantFromValue(0.f) << qVariantFromValue(0.f) << 0;
    QTest::newRow("float2") << qVariantFromValue(-1.f) << qVariantFromValue(0.f) << -1;
    QTest::newRow("float3") << qVariantFromValue(0.f) << qVariantFromValue(-1.f) << +1;
    QTest::newRow("float4") << qVariantFromValue(-float(qInf())) << qVariantFromValue(0.f) << -1;
    QTest::newRow("float5") << qVariantFromValue(0.f) << qVariantFromValue(-float(qInf())) << +1;
    QTest::newRow("float6") << qVariantFromValue(-float(qInf())) << qVariantFromValue(-float(qInf())) << 0;
    QTest::newRow("float7") << qVariantFromValue(float(qInf())) << qVariantFromValue(float(qInf())) << 0;

    QTest::newRow("double1") << qVariantFromValue(0.) << qVariantFromValue(0.) << 0;
    QTest::newRow("double2") << qVariantFromValue(-1.) << qVariantFromValue(0.) << -1;
    QTest::newRow("double3") << qVariantFromValue(0.) << qVariantFromValue(-1.) << +1;
    QTest::newRow("double4") << qVariantFromValue(-qInf()) << qVariantFromValue(0.) << -1;
    QTest::newRow("double5") << qVariantFromValue(0.) << qVariantFromValue(-qInf()) << +1;
    QTest::newRow("double6") << qVariantFromValue(-double(qInf())) << qVariantFromValue(-qInf()) << 0;
    QTest::newRow("double7") << qVariantFromValue(qInf()) << qVariantFromValue(qInf()) << 0;

    // mixed comparisons
    // fp + fp
    QTest::newRow("float+double1") << qVariantFromValue(0.f) << qVariantFromValue(0.) << 0;
    QTest::newRow("float+double2") << qVariantFromValue(-1.f) << qVariantFromValue(0.) << -1;
    QTest::newRow("float+double3") << qVariantFromValue(0.f) << qVariantFromValue(-1.) << +1;
    QTest::newRow("float+double4") << qVariantFromValue(-float(qInf())) << qVariantFromValue(0.) << -1;
    QTest::newRow("float+double5") << qVariantFromValue(0.f) << qVariantFromValue(-qInf()) << +1;
    QTest::newRow("float+double6") << qVariantFromValue(-float(qInf())) << qVariantFromValue(qInf()) << 0;
    QTest::newRow("float+double7") << qVariantFromValue(float(qInf())) << qVariantFromValue(qInf()) << 0;

    // fp + int
    QTest::newRow("float+int1") << qVariantFromValue(0.f) << qVariantFromValue(0) << 0;
    QTest::newRow("double+int1") << qVariantFromValue(0.) << qVariantFromValue(0) << 0;
    QTest::newRow("float+int2") << qVariantFromValue(-1.f) << qVariantFromValue(0) << -1;
    QTest::newRow("double+int2") << qVariantFromValue(-1.) << qVariantFromValue(0) << -1;
    QTest::newRow("float+int3") << qVariantFromValue(0.f) << qVariantFromValue(-1) << +1;
    QTest::newRow("double+int3") << qVariantFromValue(0.) << qVariantFromValue(-1) << +1;
    QTest::newRow("float+int4") << qVariantFromValue(1.5f) << qVariantFromValue(1) << +1;
    QTest::newRow("double+int4") << qVariantFromValue(1.5) << qVariantFromValue(1) << +1;
    QTest::newRow("double+int5") << qVariantFromValue(qInf()) << qVariantFromValue(1) << +1;

    // fp + uint
    QTest::newRow("float+uint1") << qVariantFromValue(0.f) << qVariantFromValue(0U) << 0;
    QTest::newRow("double+uint1") << qVariantFromValue(0.) << qVariantFromValue(0U) << 0;
    QTest::newRow("float+uint2") << qVariantFromValue(-1.f) << qVariantFromValue(0U) << -1;
    QTest::newRow("double+uint2") << qVariantFromValue(-1.) << qVariantFromValue(0U) << -1;
    QTest::newRow("float+uint3") << qVariantFromValue(0.f) << qVariantFromValue(1U) << -1;
    QTest::newRow("double+uint3") << qVariantFromValue(0.) << qVariantFromValue(1U) << -1;
    QTest::newRow("float+uint4") << qVariantFromValue(1.5f) << qVariantFromValue(1U) << +1;
    QTest::newRow("double+uint4") << qVariantFromValue(1.5) << qVariantFromValue(1U) << +1;

    // lower ranked + int
    QTest::newRow("bool+int1") << qVariantFromValue(false) << qVariantFromValue(0) << 0;
    QTest::newRow("bool+int2") << qVariantFromValue(false) << qVariantFromValue(1) << -1;
    QTest::newRow("bool+int3") << qVariantFromValue(true) << qVariantFromValue(0) << +1;
    QTest::newRow("bool+int4") << qVariantFromValue(true) << qVariantFromValue(1) << 0;
    QTest::newRow("bool+int5") << qVariantFromValue(true) << qVariantFromValue(2) << -1;

    QTest::newRow("char+int1") << qVariantFromValue(char(0)) << qVariantFromValue(0) << 0;
    QTest::newRow("char+int2") << qVariantFromValue(char(0)) << qVariantFromValue(1) << -1;
    QTest::newRow("char+int3") << qVariantFromValue(char(1)) << qVariantFromValue(0) << +1;
    QTest::newRow("char+int4") << qVariantFromValue(char(1)) << qVariantFromValue(1) << 0;
    if (std::numeric_limits<char>::is_signed) {
        QTest::newRow("char+int5") << qVariantFromValue(char(-1)) << qVariantFromValue(0) << -1;
        QTest::newRow("char+int6") << qVariantFromValue(char(-1)) << qVariantFromValue(-1) << 0;
    }

    QTest::newRow("schar+int1") << qVariantFromValue(schar(0)) << qVariantFromValue(0) << 0;
    QTest::newRow("schar+int2") << qVariantFromValue(schar(0)) << qVariantFromValue(1) << -1;
    QTest::newRow("schar+int3") << qVariantFromValue(schar(1)) << qVariantFromValue(0) << +1;
    QTest::newRow("schar+int4") << qVariantFromValue(schar(1)) << qVariantFromValue(1) << 0;
    QTest::newRow("schar+int5") << qVariantFromValue(schar(-1)) << qVariantFromValue(0) << -1;
    QTest::newRow("schar+int6") << qVariantFromValue(schar(-1)) << qVariantFromValue(-1) << 0;

    QTest::newRow("uchar+int1") << qVariantFromValue(uchar(0)) << qVariantFromValue(0) << 0;
    QTest::newRow("uchar+int2") << qVariantFromValue(uchar(0)) << qVariantFromValue(1) << -1;
    QTest::newRow("uchar+int3") << qVariantFromValue(uchar(1)) << qVariantFromValue(0) << +1;
    QTest::newRow("uchar+int4") << qVariantFromValue(uchar(1)) << qVariantFromValue(1) << 0;

    QTest::newRow("short+int1") << qVariantFromValue(short(0)) << qVariantFromValue(0) << 0;
    QTest::newRow("short+int2") << qVariantFromValue(short(0)) << qVariantFromValue(1) << -1;
    QTest::newRow("short+int3") << qVariantFromValue(short(1)) << qVariantFromValue(0) << +1;
    QTest::newRow("short+int4") << qVariantFromValue(short(1)) << qVariantFromValue(1) << 0;
    QTest::newRow("short+int5") << qVariantFromValue(short(-1)) << qVariantFromValue(0) << -1;
    QTest::newRow("short+int6") << qVariantFromValue(short(-1)) << qVariantFromValue(-1) << 0;

    QTest::newRow("ushort+int1") << qVariantFromValue(ushort(0)) << qVariantFromValue(0) << 0;
    QTest::newRow("ushort+int2") << qVariantFromValue(ushort(0)) << qVariantFromValue(1) << -1;
    QTest::newRow("ushort+int3") << qVariantFromValue(ushort(1)) << qVariantFromValue(0) << +1;
    QTest::newRow("ushort+int4") << qVariantFromValue(ushort(1)) << qVariantFromValue(1) << 0;

    // lower ranked + uint (without sign change)
    QTest::newRow("bool+uint1") << qVariantFromValue(false) << qVariantFromValue(0U) << 0;
    QTest::newRow("bool+uint2") << qVariantFromValue(false) << qVariantFromValue(1U) << -1;
    QTest::newRow("bool+uint3") << qVariantFromValue(true) << qVariantFromValue(0U) << +1;
    QTest::newRow("bool+uint4") << qVariantFromValue(true) << qVariantFromValue(1U) << 0;
    QTest::newRow("bool+uint5") << qVariantFromValue(true) << qVariantFromValue(2U) << -1;

    QTest::newRow("char+uint1") << qVariantFromValue(char(0)) << qVariantFromValue(0U) << 0;
    QTest::newRow("char+uint2") << qVariantFromValue(char(0)) << qVariantFromValue(1U) << -1;
    QTest::newRow("char+uint3") << qVariantFromValue(char(1)) << qVariantFromValue(0U) << +1;
    QTest::newRow("char+uint4") << qVariantFromValue(char(1)) << qVariantFromValue(1U) << 0;

    QTest::newRow("schar+uint1") << qVariantFromValue(schar(0)) << qVariantFromValue(0U) << 0;
    QTest::newRow("schar+uint2") << qVariantFromValue(schar(0)) << qVariantFromValue(1U) << -1;
    QTest::newRow("schar+uint3") << qVariantFromValue(schar(1)) << qVariantFromValue(0U) << +1;
    QTest::newRow("schar+uint4") << qVariantFromValue(schar(1)) << qVariantFromValue(1U) << 0;

    QTest::newRow("uchar+uint1") << qVariantFromValue(uchar(0)) << qVariantFromValue(0U) << 0;
    QTest::newRow("uchar+uint2") << qVariantFromValue(uchar(0)) << qVariantFromValue(1U) << -1;
    QTest::newRow("uchar+uint3") << qVariantFromValue(uchar(1)) << qVariantFromValue(0U) << +1;
    QTest::newRow("uchar+uint4") << qVariantFromValue(uchar(1)) << qVariantFromValue(1U) << 0;

    QTest::newRow("short+uint1") << qVariantFromValue(short(0)) << qVariantFromValue(0U) << 0;
    QTest::newRow("short+uint2") << qVariantFromValue(short(0)) << qVariantFromValue(1U) << -1;
    QTest::newRow("short+uint3") << qVariantFromValue(short(1)) << qVariantFromValue(0U) << +1;
    QTest::newRow("short+uint4") << qVariantFromValue(short(1)) << qVariantFromValue(1U) << 0;

    QTest::newRow("ushort+uint1") << qVariantFromValue(ushort(0)) << qVariantFromValue(0U) << 0;
    QTest::newRow("ushort+uint2") << qVariantFromValue(ushort(0)) << qVariantFromValue(1U) << -1;
    QTest::newRow("ushort+uint3") << qVariantFromValue(ushort(1)) << qVariantFromValue(0U) << +1;
    QTest::newRow("ushort+uint4") << qVariantFromValue(ushort(1)) << qVariantFromValue(1U) << 0;

    // int + qlonglong
    QTest::newRow("int+qlonglong1") << qVariantFromValue(0) << qVariantFromValue(Q_INT64_C(0)) << 0;
    QTest::newRow("int+qlonglong2") << qVariantFromValue(1) << qVariantFromValue(Q_INT64_C(0)) << +1;
    QTest::newRow("int+qlonglong3") << qVariantFromValue(0) << qVariantFromValue(Q_INT64_C(1)) << -1;
    QTest::newRow("int+qlonglong4") << qVariantFromValue(1) << qVariantFromValue(Q_INT64_C(1)) << 0;
    QTest::newRow("int+qlonglong5") << qVariantFromValue(0) << qVariantFromValue(Q_INT64_C(-1)) << +1;
    QTest::newRow("int+qlonglong6") << qVariantFromValue(-1) << qVariantFromValue(Q_INT64_C(0)) << -1;
    QTest::newRow("int+qlonglong7") << qVariantFromValue(-1) << qVariantFromValue(Q_INT64_C(-1)) << 0;

    // uint + qulonglong
    QTest::newRow("uint+qulonglong1") << qVariantFromValue(0U) << qVariantFromValue(Q_UINT64_C(0)) << 0;
    QTest::newRow("uint+qulonglong2") << qVariantFromValue(1U) << qVariantFromValue(Q_UINT64_C(0)) << +1;
    QTest::newRow("uint+qulonglong3") << qVariantFromValue(0U) << qVariantFromValue(Q_UINT64_C(1)) << -1;
    QTest::newRow("uint+qulonglong4") << qVariantFromValue(1U) << qVariantFromValue(Q_UINT64_C(1)) << 0;

    // int + uint (without sign change)
    QTest::newRow("int+uint1") << qVariantFromValue(0) << qVariantFromValue(0U) << 0;
    QTest::newRow("int+uint2") << qVariantFromValue(1) << qVariantFromValue(0U) << +1;
    QTest::newRow("int+uint3") << qVariantFromValue(0) << qVariantFromValue(1U) << -1;
    QTest::newRow("int+uint4") << qVariantFromValue(1) << qVariantFromValue(1U) << 0;

    // uint + qlonglong
    QTest::newRow("uint+qlonglong1") << qVariantFromValue(0U) << qVariantFromValue(Q_INT64_C(0)) << 0;
    QTest::newRow("uint+qlonglong2") << qVariantFromValue(1U) << qVariantFromValue(Q_INT64_C(0)) << +1;
    QTest::newRow("uint+qlonglong3") << qVariantFromValue(0U) << qVariantFromValue(Q_INT64_C(1)) << -1;
    QTest::newRow("uint+qlonglong4") << qVariantFromValue(1U) << qVariantFromValue(Q_INT64_C(1)) << 0;
    QTest::newRow("uint+qlonglong5") << qVariantFromValue(0U) << qVariantFromValue(Q_INT64_C(-1)) << +1;

    // boundary conditions
    QTest::newRow("charmax+intmax") << qVariantFromValue(CHAR_MAX) << qVariantFromValue(INT_MAX) << -1;
    QTest::newRow("charmax+uintmax") << qVariantFromValue(CHAR_MAX) << qVariantFromValue(UINT_MAX) << -1;
    QTest::newRow("scharmax+intmax") << qVariantFromValue(SCHAR_MAX) << qVariantFromValue(INT_MAX) << -1;
    QTest::newRow("scharmax+uintmax") << qVariantFromValue(SCHAR_MAX) << qVariantFromValue(UINT_MAX) << -1;
    QTest::newRow("ucharmax+intmax") << qVariantFromValue(UCHAR_MAX) << qVariantFromValue(INT_MAX) << -1;
    QTest::newRow("ucharmax+uintmax") << qVariantFromValue(UCHAR_MAX) << qVariantFromValue(UINT_MAX) << -1;
    QTest::newRow("shortmax+intmax") << qVariantFromValue(SHRT_MAX) << qVariantFromValue(INT_MAX) << -1;
    QTest::newRow("shortmax+uintmax") << qVariantFromValue(SHRT_MAX) << qVariantFromValue(UINT_MAX) << -1;
    QTest::newRow("ushortmax+intmax") << qVariantFromValue(USHRT_MAX) << qVariantFromValue(INT_MAX) << -1;
    QTest::newRow("ushortmax+uintmax") << qVariantFromValue(USHRT_MAX) << qVariantFromValue(UINT_MAX) << -1;

    QTest::newRow("intmin+qlonglongmin") << qVariantFromValue(INT_MIN) << qVariantFromValue(LLONG_MIN) << +1;
    QTest::newRow("intmax+uintmax") << qVariantFromValue(INT_MAX) << qVariantFromValue(UINT_MAX) << -1;
    QTest::newRow("intmax+qlonglongmax") << qVariantFromValue(INT_MAX) << qVariantFromValue(LLONG_MAX) << -1;
    QTest::newRow("uintmax+qlonglongmax") << qVariantFromValue(UINT_MAX) << qVariantFromValue(LLONG_MAX) << -1;
    QTest::newRow("intmax+qulonglongmax") << qVariantFromValue(INT_MAX) << qVariantFromValue(ULLONG_MAX) << -1;
    QTest::newRow("qlonglongmax+qulonglongmax") << qVariantFromValue(LLONG_MAX) << qVariantFromValue(ULLONG_MAX) << -1;
    QTest::newRow("uintmax+qlonglongmin") << qVariantFromValue(UINT_MAX) << qVariantFromValue(LLONG_MIN) << +1;

    // check for no sign-extension issues
    QTest::newRow("ushortmax+intzero") << qVariantFromValue(USHRT_MAX) << qVariantFromValue(0) << +1;
    QTest::newRow("ushortmax+qlonglongzero") << qVariantFromValue(USHRT_MAX) << qVariantFromValue(Q_INT64_C(0)) << +1;
    QTest::newRow("uintmax+qlonglongzero") << qVariantFromValue(UINT_MAX) << qVariantFromValue(Q_INT64_C(0)) << +1;

    // sign changes
    // the tests below check that a signed negative number sign-changes to a non-zero unsigned number and that
    // signed -1 sign-changes to unsigned maximum (all bits set, ~0). This works on two's complement machines
    // (all that Qt supports), and would also work on one's complement.
    if (std::numeric_limits<char>::is_signed) {
        QTest::newRow("signchange-char+uint") << qVariantFromValue(char(-1)) << qVariantFromValue(0U) << +1;
        QTest::newRow("signchange-char+uintmax") << qVariantFromValue(char(-1)) << qVariantFromValue(UINT_MAX) << 0;
        QTest::newRow("signchange-charmin+uint") << qVariantFromValue(CHAR_MIN) << qVariantFromValue(0U) << +1;
        QTest::newRow("signchange-char+qulonglong") << qVariantFromValue(char(-1)) << qVariantFromValue(Q_UINT64_C(0)) << +1;
        QTest::newRow("signchange-char+qulonglongmax") << qVariantFromValue(char(-1)) << qVariantFromValue(ULLONG_MAX) << 0;
        QTest::newRow("signchange-charmin+qulonglong") << qVariantFromValue(CHAR_MIN) << qVariantFromValue(Q_UINT64_C(0)) << +1;
    }
    QTest::newRow("signchange-schar+uint") << qVariantFromValue(schar(-1)) << qVariantFromValue(0U) << +1;
    QTest::newRow("signchange-schar+uintmax") << qVariantFromValue(schar(-1)) << qVariantFromValue(UINT_MAX) << 0;
    QTest::newRow("signchange-scharmin+uint") << qVariantFromValue(SCHAR_MIN) << qVariantFromValue(0U) << +1;
    QTest::newRow("signchange-schar+qulonglong") << qVariantFromValue(schar(-1)) << qVariantFromValue(Q_UINT64_C(0)) << +1;
    QTest::newRow("signchange-schar+qulonglongmax") << qVariantFromValue(schar(-1)) << qVariantFromValue(ULLONG_MAX) << 0;
    QTest::newRow("signchange-scharmin+qulonglong") << qVariantFromValue(SCHAR_MIN) << qVariantFromValue(Q_UINT64_C(0)) << +1;
    QTest::newRow("signchange-short+uint") << qVariantFromValue(short(-1)) << qVariantFromValue(0U) << +1;
    QTest::newRow("signchange-short+uintmax") << qVariantFromValue(short(-1)) << qVariantFromValue(UINT_MAX) << 0;
    QTest::newRow("signchange-shortmin+uint") << qVariantFromValue(SHRT_MIN) << qVariantFromValue(0U) << +1;
    QTest::newRow("signchange-short+qulonglong") << qVariantFromValue(short(-1)) << qVariantFromValue(Q_UINT64_C(0)) << +1;
    QTest::newRow("signchange-short+qulonglongmax") << qVariantFromValue(short(-1)) << qVariantFromValue(ULLONG_MAX) << 0;
    QTest::newRow("signchange-shortmin+qulonglong") << qVariantFromValue(SHRT_MIN) << qVariantFromValue(Q_UINT64_C(0)) << +1;
    QTest::newRow("signchange-int+uint") << qVariantFromValue(-1) << qVariantFromValue(0U) << +1;
    QTest::newRow("signchange-int+uintmax") << qVariantFromValue(-1) << qVariantFromValue(UINT_MAX) << 0;
    QTest::newRow("signchange-intmin+uint") << qVariantFromValue(INT_MIN) << qVariantFromValue(0U) << +1;
    QTest::newRow("signchange-int+qulonglong") << qVariantFromValue(-1) << qVariantFromValue(Q_UINT64_C(0)) << +1;
    QTest::newRow("signchange-int+qulonglongmax") << qVariantFromValue(-1) << qVariantFromValue(ULLONG_MAX) << 0;
    QTest::newRow("signchange-intmin+qulonglong") << qVariantFromValue(INT_MIN) << qVariantFromValue(Q_UINT64_C(0)) << +1;
    // no qlonglong+uint, since that should promote to qlonglong and then the comparison is signed (tested above)
    QTest::newRow("signchange-qlonglong+qulonglong") << qVariantFromValue(Q_INT64_C(-1)) << qVariantFromValue(Q_UINT64_C(0)) << +1;
    QTest::newRow("signchange-qlonglong+qulonglongmax") << qVariantFromValue(Q_INT64_C(-1)) << qVariantFromValue(ULLONG_MAX) << 0;
    QTest::newRow("signchange-qlonglongmin+qulonglong") << qVariantFromValue(LLONG_MIN) << qVariantFromValue(Q_UINT64_C(0)) << +1;
}

void tst_QVariant::compareNumbers() const
{
    QFETCH(QVariant, v1);
    QFETCH(QVariant, v2);
    QFETCH(int, expected);

    if (expected == -1) {
        QVERIFY(v1 < v2);
        QVERIFY(v1 <= v2);
        QVERIFY(!(v1 == v2));
        QVERIFY(!(v1 > v2));
        QVERIFY(!(v1 >= v2));

        QVERIFY(!(v2 < v1));
        QVERIFY(!(v2 <= v1));
        QVERIFY(!(v2 == v1));
        QVERIFY(v2 >= v1);
        QVERIFY(v2 > v1);
    } else if (expected == 0) {
        QVERIFY(!(v1 < v2));
        QVERIFY(v1 <= v2);
        QCOMPARE(v1, v2);
        QVERIFY(!(v1 > v2));
        QVERIFY(v1 >= v2);

        QVERIFY(!(v2 < v1));
        QVERIFY(v2 <= v1);
        QCOMPARE(v2, v1);
        QVERIFY(v2 >= v1);
        QVERIFY(!(v2 > v1));
    } else {
        QVERIFY(!(v1 < v2));
        QVERIFY(!(v1 <= v2));
        QVERIFY(!(v1 == v2));
        QVERIFY(v1 > v2);
        QVERIFY(v1 >= v2);

        QVERIFY(v2 < v1);
        QVERIFY(v2 <= v1);
        QVERIFY(!(v2 == v1));
        QVERIFY(!(v2 >= v1));
        QVERIFY(!(v2 > v1));
    }
}

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
    QTest::newRow("23") << int(QVariant::SizePolicy) << QByteArray("QSizePolicy");
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
    QTest::newRow("41") << int(QVariant::RegExp) << QByteArray("QRegExp");
    QTest::newRow("43") << int(QVariant::Matrix) << QByteArray("QMatrix");
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
    for ( int t = 1; t <= max; t++ ) {
        const char *n = QVariant::typeToName( (QVariant::Type)t );
        if (n)
            QCOMPARE( int(QVariant::nameToType( n )), t );

    }
    QCOMPARE(QVariant::typeToName(QVariant::Int), "int");
    // not documented but we return 0 if the type is out of range
    // by testing this we catch cases where QVariant is extended
    // but type_map is not updated accordingly
    QCOMPARE( QVariant::typeToName( QVariant::Type(max+1) ), (char*)0 );
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

void tst_QVariant::streamInvalidVariant()
{
    int writeX = 1;
    int writeY = 2;
    int readX;
    int readY;
    QVariant writeVariant;
    QVariant readVariant;

    QVERIFY( writeVariant.type() == QVariant::Invalid );

    QByteArray data;
    QDataStream writeStream( &data, QIODevice::WriteOnly );
    writeStream << writeX << writeVariant << writeY;

    QDataStream readStream( &data, QIODevice::ReadOnly );
    readStream >> readX >> readVariant >> readY;

    QVERIFY( readX == writeX );
    // Two invalid QVariant's aren't necessarily the same, so == will
    // return false if one is invalid, so check the type() instead
    QVERIFY( readVariant.type() == QVariant::Invalid );
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
    ~MyType()
    {
        --instanceCount;
    }
    int number;
    const char *text;
};

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

            QCOMPARE(userVar.type(), QVariant::UserType);
            QCOMPARE(userVar.userType(), qMetaTypeId<MyType>());
            QCOMPARE(userVar.typeName(), "MyType");
            QVERIFY(!userVar.isNull());
            QVERIFY(!userVar.canConvert(QVariant::String));

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

            QCOMPARE(userVar.type(), QVariant::UserType);
            QCOMPARE(userVar.userType(), qMetaTypeId<MyType*>());
            QCOMPARE(userVar.typeName(), "MyType*");
            QVERIFY(!userVar.isNull());
            QVERIFY(!userVar.canConvert(QVariant::String));

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

    QVariant pod_as_variant = QVariant::fromValue(pod);
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
        v = QVariant(QMetaType::Int, &i);
    }
    QCOMPARE(v.type(), QVariant::Int);
    QCOMPARE(v.toInt(), 7);

    {
        QString s("foo");
        v = QVariant(QMetaType::QString, &s);
    }
    QCOMPARE(v.type(), QVariant::String);
    QCOMPARE(v.toString(), QString("foo"));

    {
        double d = 4.4;
        v = QVariant(QMetaType::Double, &d);
    }
    QCOMPARE(v.type(), QVariant::Double);
    QCOMPARE(v.toDouble(), 4.4);

    {
        float f = 4.5f;
        v = QVariant(QMetaType::Float, &f);
    }
    QCOMPARE(v.userType(), int(QMetaType::Float));
    QCOMPARE(v.toDouble(), 4.5);

    {
        QByteArray ba("bar");
        v = QVariant(QMetaType::QByteArray, &ba);
    }
    QCOMPARE(v.type(), QVariant::ByteArray);
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
    int tp = qRegisterMetaType<Blah>("Blah");
    QVariant v = QVariant(tp, &i);

    qRegisterMetaTypeStreamOperators<Blah>("Blah");

    QCOMPARE(v.userType(), tp);
    QCOMPARE(v.type(), QVariant::UserType);
    {
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream << v;
    }

    v = QVariant();

    {
        QDataStream stream(data);
        stream >> v;
    }

    QCOMPARE(int(v.userType()), QMetaType::type("Blah"));
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

    QVariant v2 = QVariant(QMetaType::type("QVariantMap"), &map);
    QCOMPARE(qvariant_cast<QVariantMap>(v2).value("test").toInt(), 42);

    QVariant v3 = QVariant(QMetaType::type("QMap<QString, QVariant>"), &map);
    QCOMPARE(qvariant_cast<QVariantMap>(v3).value("test").toInt(), 42);

    // multi-keys
    map.insertMulti("test", 47);
    v = map;
    map2 = qvariant_cast<QVariantMap>(v);
    QCOMPARE(map2, map);
    map2 = v.toMap();
    QCOMPARE(map2, map);
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

    QVariant v2 = QVariant(QMetaType::type("QVariantHash"), &hash);
    QCOMPARE(qvariant_cast<QVariantHash>(v2).value("test").toInt(), 42);

    QVariant v3 = QVariant(QMetaType::type("QHash<QString, QVariant>"), &hash);
    QCOMPARE(qvariant_cast<QVariantHash>(v3).value("test").toInt(), 42);

    // multi-keys
    hash.insertMulti("test", 47);
    v = hash;
    hash2 = qvariant_cast<QVariantHash>(v);
    QCOMPARE(hash2, hash);
    hash2 = v.toHash();
    QCOMPARE(hash2, hash);
}

class CustomQObject : public QObject {
    Q_OBJECT
public:
    CustomQObject(QObject *parent = 0) : QObject(parent) {}
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
    QTest::newRow("from QObject") << QVariant(QMetaType::QObjectStar, &obj) << true << false;
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
    QTest::newRow("null int") << QVariant::fromValue<int>(0) << false << true;
}

void tst_QVariant::qvariant_cast_QObject()
{
    QFETCH(QVariant, data);
    QFETCH(bool, success);
    QFETCH(bool, isNull);

    QObject *o = qvariant_cast<QObject *>(data);
    QCOMPARE(o != 0, success && !isNull);
    if (success) {
        if (!isNull)
            QCOMPARE(o->objectName(), QString::fromLatin1("Hello"));
        QVERIFY(data.canConvert<QObject*>());
        QVERIFY(data.canConvert(QMetaType::QObjectStar));
        QVERIFY(data.canConvert(::qMetaTypeId<QObject*>()));
        QCOMPARE(data.value<QObject*>() == 0, isNull);
        QVERIFY(data.convert(QMetaType::QObjectStar));
        QCOMPARE(data.userType(), int(QMetaType::QObjectStar));
    } else {
        QVERIFY(!data.canConvert<QObject*>());
        QVERIFY(!data.canConvert(QMetaType::QObjectStar));
        QVERIFY(!data.canConvert(::qMetaTypeId<QObject*>()));
        QVERIFY(!data.value<QObject*>());
        QVERIFY(!data.convert(QMetaType::QObjectStar));
        QVERIFY(data.userType() != QMetaType::QObjectStar);
    }
}

class CustomQObjectDerived : public CustomQObject {
    Q_OBJECT
public:
    CustomQObjectDerived(QObject *parent = 0) : CustomQObject(parent) {}
};
Q_DECLARE_METATYPE(CustomQObjectDerived*)

class CustomQObjectDerivedNoMetaType : public CustomQObject {
    Q_OBJECT
public:
    CustomQObjectDerivedNoMetaType(QObject *parent = 0) : CustomQObject(parent) {}
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
}

struct QObjectWrapper
{
    explicit QObjectWrapper(QObject *o = 0) : obj(o) {}

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
    explicit SmartPointer(T *t = 0)
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
    v.convert(qMetaTypeId<QObject*>());
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
        QWeakPointer<QFile> sp(f);
        QVariant spVar = QVariant::fromValue(sp);
        QVERIFY(spVar.canConvert<QObject*>());
        QCOMPARE(f, spVar.value<QObject*>());
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

void tst_QVariant::comparePointers() const
{
    class MyClass
    {
    };

    MyClass myClass;

    QVariant v  = QVariant::fromValue<void *>(&myClass);
    QVariant v2 = QVariant::fromValue<void *>(&myClass);

    QCOMPARE(v, v2);
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

    p2 = 0;
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

    QCOMPARE(v.canConvert(QVariant::String), canConvert);
    QCOMPARE(v.toString(), result);
}

void tst_QVariant::canConvertQStringList_data() const
{
    QTest::addColumn<bool>("canConvert");
    QTest::addColumn<QStringList>("input");
    QTest::addColumn<QString>("result");

    QTest::newRow("An empty list") << false << QStringList() << QString();
    QTest::newRow("A single item") << true << QStringList(QLatin1String("foo")) << QString::fromLatin1("foo");
    QTest::newRow("A single, but empty item") << true << QStringList(QString()) << QString();

    QStringList l;
    l << "a" << "b";

    QTest::newRow("Two items") << false << l << QString();

    l << "c";
    QTest::newRow("Three items") << false << l << QString();
}

template<typename T> void convertMetaType()
{
    QVERIFY(QVariant::fromValue<T>(10).isValid());
    QVERIFY(QVariant::fromValue<T>(10).canConvert(QVariant::Int));
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
        v1.convert(QVariant::DateTime);

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
    QVERIFY(!variant.convert(QVariant::DateTime));
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
    char data[] = {0, 0, QMetaType::User >> 8 , char(QMetaType::User), 0, 0, 0, 0, 8, 'M', 'y', 'C', 'l', 'a', 's', 's', 0};

    QByteArray ba(data, sizeof(data));
    QDataStream ds(&ba, QIODevice::ReadOnly);
    QVariant var;
    var.load(ds);
    QCOMPARE(ds.status(), QDataStream::ReadCorruptData);
}

void tst_QVariant::loadBrokenUserType()
{
    QTest::ignoreMessage(QtWarningMsg, "Trying to construct an instance of an invalid type, type id: 127");
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
    QVERIFY(!variant.convert(QVariant::Date));

    variant = foo;
    QVERIFY(!variant.convert(QVariant::DateTime));

    variant = foo;
    QVERIFY(!variant.convert(QVariant::Time));

    variant = foo;
    QVERIFY(!variant.convert(QVariant::Int));

    variant = foo;
    QVERIFY(!variant.convert(QVariant::Double));

    variant = foo;
    QVERIFY(!variant.convert(QVariant::Type(QMetaType::Float)));
}

struct WontCompare
{
    int x,y,z,q,w,e,r,t;
};
Q_DECLARE_METATYPE(WontCompare);

void tst_QVariant::compareCustomTypes() const
{
    qRegisterMetaType<WontCompare>("WontCompare");

    WontCompare f1;
    f1.x = 0;
    const QVariant variant1(QVariant::fromValue(f1));

    WontCompare f2;
    f2.x = 0;
    const QVariant variant2(QVariant::fromValue(f2));

    /* We compare pointers. */
    QVERIFY(variant1 != variant2);
    QCOMPARE(variant1, variant1);
    QCOMPARE(variant2, variant2);
}

void tst_QVariant::timeToDateTime() const
{
    const QVariant val(QTime::currentTime());
    QVERIFY(!val.canConvert(QVariant::DateTime));
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
    QCOMPARE(variant.type(), QVariant::Bool);
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
        QCOMPARE(QVariant::fromValue(list).value<QVariantList>().count(), list.count());
    }
    {
        QByteArrayList list;
        list.append("Hello");
        QCOMPARE(QVariant::fromValue(list).value<QVariantList>().count(), list.count());
    }
    {
        QVariantList list;
        list.append("World");
        QCOMPARE(QVariant::fromValue(list).value<QVariantList>().count(), list.count());
    }
    {
        QMap<QString, int> map;
        map.insert("3", 4);
        QCOMPARE(QVariant::fromValue(map).value<QVariantHash>().count(), map.count());
        QCOMPARE(QVariant::fromValue(map).value<QVariantMap>().count(), map.count());

        map.insertMulti("3", 5);
        QCOMPARE(QVariant::fromValue(map).value<QVariantHash>().count(), map.count());
        QCOMPARE(QVariant::fromValue(map).value<QVariantMap>().count(), map.count());
    }
    {
        QVariantMap map;
        map.insert("3", 4);
        QCOMPARE(QVariant::fromValue(map).value<QVariantHash>().count(), map.count());
        QCOMPARE(QVariant::fromValue(map).value<QVariantMap>().count(), map.count());

        map.insertMulti("3", 5);
        QCOMPARE(QVariant::fromValue(map).value<QVariantHash>().count(), map.count());
        QCOMPARE(QVariant::fromValue(map).value<QVariantMap>().count(), map.count());
    }
    {
        QHash<QString, int> hash;
        hash.insert("3", 4);
        QCOMPARE(QVariant::fromValue(hash).value<QVariantHash>().count(), hash.count());
        QCOMPARE(QVariant::fromValue(hash).value<QVariantMap>().count(), hash.count());

        hash.insertMulti("3", 5);
        QCOMPARE(QVariant::fromValue(hash).value<QVariantHash>().count(), hash.count());
        QCOMPARE(QVariant::fromValue(hash).value<QVariantMap>().count(), hash.count());
    }
    {
        QVariantHash hash;
        hash.insert("3", 4);
        QCOMPARE(QVariant::fromValue(hash).value<QVariantHash>().count(), hash.count());
        QCOMPARE(QVariant::fromValue(hash).value<QVariantMap>().count(), hash.count());

        hash.insertMulti("3", 5);
        QCOMPARE(QVariant::fromValue(hash).value<QVariantHash>().count(), hash.count());
        QCOMPARE(QVariant::fromValue(hash).value<QVariantMap>().count(), hash.count());
    }
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
    QVERIFY( var.canConvert( QVariant::Int ) );

    bool ok;
    int result = var.toInt(&ok);

    QVERIFY( ok == true );
    QCOMPARE(result, 2147483630);
}

void tst_QVariant::setValue()
{
    QJsonDocument t; //we just take a value so that we're sure that it will be shared
    QVariant v1 = QVariant::fromValue(t);
    QVERIFY( v1.isDetached() );
    QVariant v2 = v1;
    QVERIFY( !v1.isDetached() );
    QVERIFY( !v2.isDetached() );

    v2.setValue(3); //set an integer value

    QVERIFY( v1.isDetached() );
    QVERIFY( v2.isDetached() );
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
    QVERIFY(converted.convert(QVariant::String));
    QVERIFY(converted.convert(number.type()));
    QCOMPARE(converted, number);

    converted = number;
    QVERIFY(converted.convert(QVariant::ByteArray));
    QVERIFY(converted.convert(number.type()));
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
    case QVariant::Double:
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
        if (!(QTypeInfo<T>::isStatic && QTypeInfo<T>::isComplex)) {
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
        if (!(QTypeInfo<T>::isStatic && QTypeInfo<T>::isComplex)) {
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

    if (qMetaTypeId<T>() != qMetaTypeId<QVariant>()) {
        QCOMPARE(v.userType(), qMetaTypeId<T>());
        QCOMPARE(QVariant::typeToName(QVariant::Type(v.userType())), QMetaType::typeName(qMetaTypeId<T>()));
    }
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
};

int MyNotMovable::count  = 0;

struct MyShared : QSharedData {
    MyMovable movable;
};

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(MyMovable, Q_MOVABLE_TYPE);
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
        PLAY_WITH_VARIANT(str, true, QString(), 0, false);
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

        PLAY_WITH_VARIANT('a', false, "a", 'a', true);
        PLAY_WITH_VARIANT((unsigned char)('a'), false, "a", 'a', true);
        PLAY_WITH_VARIANT( quint8(12), false, "\xc", 12, true);
        PLAY_WITH_VARIANT(  qint8(13), false, "\xd", 13, true);
        PLAY_WITH_VARIANT(quint16(14), false, "14", 14, true);
        PLAY_WITH_VARIANT( qint16(15), false, "15", 15, true);
        PLAY_WITH_VARIANT(quint32(16), false, "16", 16, true);
        PLAY_WITH_VARIANT( qint32(17), false, "17", 17, true);
        PLAY_WITH_VARIANT(quint64(18), false, "18", 18, true);
        PLAY_WITH_VARIANT( qint64(19), false, "19", 19, true);
        PLAY_WITH_VARIANT(  qint8(-12), false, QLatin1String("\xf4"), -12, true); // qint8 is char, so it gets converted via QChar
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
        PLAY_WITH_VARIANT((void *)(0), false, QString(), 0, false);
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
        QList<QVector<int> > data;
        PLAY_WITH_VARIANT(data, false, QString(), 0, false);
        data << (QVector<int>() << 42);
        PLAY_WITH_VARIANT(data, false, QString(), 0, false);
    }

    {
        QList<QSet<int> > data;
        PLAY_WITH_VARIANT(data, false, QString(), 0, false);
        data << (QSet<int>() << 42);
        PLAY_WITH_VARIANT(data, false, QString(), 0, false);
    }

    {
        QList<QLinkedList<int> > data;
        PLAY_WITH_VARIANT(data, false, QString(), 0, false);
        data << (QLinkedList<int>() << 42);
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
        QCOMPARE(buffer[0].type(), QVariant::Invalid);
        buffer[0].~QVariant();

        memcpy(buffer, &variant, sizeof(QVariant));
        QCOMPARE(buffer[0].type(), QVariant::UserType);
        QCOMPARE(buffer[0].userType(), qMetaTypeId<MyNotMovable>());
        MyNotMovable tmp(buffer[0].value<MyNotMovable>());

        new (&variant) QVariant();
    }
    QVERIFY(!MyNotMovable::count);
}

void tst_QVariant::variantInVariant()
{
    QVariant var1 = 5;
    QCOMPARE(var1.type(), QVariant::Int);
    QVariant var2 = var1;
    QCOMPARE(var2, var1);
    QCOMPARE(var2.type(), QVariant::Int);
    QVariant var3 = QVariant::fromValue(var1);
    QCOMPARE(var3, var1);
    QCOMPARE(var3.type(), QVariant::Int);
    QVariant var4 = qvariant_cast<QVariant>(var1);
    QCOMPARE(var4, var1);
    QCOMPARE(var4.type(), QVariant::Int);
    QVariant var5;
    var5 = var1;
    QCOMPARE(var5, var1);
    QCOMPARE(var5.type(), QVariant::Int);
    QVariant var6;
    var6.setValue(var1);
    QCOMPARE(var6, var1);
    QCOMPARE(var6.type(), QVariant::Int);

    QCOMPARE(QVariant::fromValue(var1), QVariant::fromValue(var2));
    QCOMPARE(qvariant_cast<QVariant>(var3), QVariant::fromValue(var4));
    QCOMPARE(qvariant_cast<QVariant>(var5), qvariant_cast<QVariant>(var6));

    QString str("hello");
    QVariant var8 = qvariant_cast<QVariant>(QVariant::fromValue(QVariant::fromValue(str)));
    QCOMPARE((int)var8.type(), (int)QVariant::String);
    QCOMPARE(qvariant_cast<QString>(QVariant(qvariant_cast<QVariant>(var8))), str);

    QVariant var9(qMetaTypeId<QVariant>(), &var1);
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
    double dummy;
    double dummy2;
    operator int() const { return (int)d; }
    operator double() const { return d; }
    operator QString() const { return QString::number(d); }
};

Q_DECLARE_METATYPE(BigConvertible);
Q_STATIC_ASSERT(sizeof(BigConvertible) > sizeof(QVariant));

void tst_QVariant::userConversion()
{
    {
        QVERIFY(!(QMetaType::hasRegisteredConverterFunction<int, Convertible>()));
        QVERIFY(!(QMetaType::hasRegisteredConverterFunction<double, Convertible>()));
        QVERIFY(!(QMetaType::hasRegisteredConverterFunction<QString, Convertible>()));

        Convertible c = { 123 };
        QVariant v = qVariantFromValue(c);

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

        BigConvertible c = { 123, 0, 0 };
        QVariant v = qVariantFromValue(c);

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
    QVERIFY(modelIndexVariant.canConvert(QMetaType::QPersistentModelIndex));
    QVERIFY(modelIndexVariant.convert(QMetaType::QPersistentModelIndex));
    QCOMPARE(modelIndexVariant.type(), QVariant::PersistentModelIndex);
    QVERIFY(modelIndexVariant.canConvert(QMetaType::QModelIndex));
    QVERIFY(modelIndexVariant.convert(QMetaType::QModelIndex));
    QCOMPARE(modelIndexVariant.type(), QVariant::ModelIndex);
}

class Forward;
Q_DECLARE_OPAQUE_POINTER(Forward*)
Q_DECLARE_METATYPE(Forward*)

void tst_QVariant::forwardDeclare()
{
    Forward *f = 0;
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
    foreach (const QFileInfo &fileInfo, dir.entryInfoList(QStringList() << "*.bin")) {
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

    const int id = QMetaType::type(typeName.toLatin1());
    if (id == QMetaType::Void) {
        // Void type is not supported by QVariant
        return;
    }

    QVariant constructedVariant(static_cast<QVariant::Type>(id));
    QCOMPARE(constructedVariant.userType(), id);
    QCOMPARE(QMetaType::typeName(loadedVariant.userType()), typeName.toLatin1().constData());
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
    const int id = QMetaType::type(typeName.toLatin1());
    if (id == QMetaType::Void) {
        // Void type is not supported by QVariant
        return;
    }

    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QDataStream stream(&buffer);
    stream.setVersion(version);

    QVariant constructedVariant(static_cast<QVariant::Type>(id));
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
        const char *tagName = QMetaType::typeName(id);
        if (!tagName)
            continue;
        if (id != QMetaType::Void) {
            QTest::newRow(tagName) << QVariant(static_cast<QVariant::Type>(id)) << id;
        }
    }
    QTest::newRow("QBitArray(111)") << QVariant(QBitArray(3, true)) << qMetaTypeId<QBitArray>();
    QTest::newRow("CustomStreamableClass") << QVariant(qMetaTypeId<CustomStreamableClass>(), 0) << qMetaTypeId<CustomStreamableClass>();
    QTest::newRow("MyClass") << QVariant(qMetaTypeId<MyClass>(), 0) << qMetaTypeId<MyClass>();
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
        ok &= (currentId == QMetaType::UnknownType
                ? msg.contains("Invalid")
                : msg.contains(QMetaType::typeName(currentId)));
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
    qDebug() << QVariant::Type(typeId);
    QVERIFY(msgHandler.testPassed());
}

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
    F(RegExp) \
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


// We have no built-in defines to check the stdlib features.
// #define TEST_UNORDERED_MAP

#ifdef TEST_UNORDERED_MAP
#include <unordered_map>
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
#endif

void tst_QVariant::iterateContainerElements()
{
#ifdef Q_COMPILER_RANGE_FOR

#define TEST_RANGE_FOR(CONTAINER) \
        numSeen = 0; \
        containerIter = intList.begin(); \
        for (QVariant v : listIter) { \
            QVERIFY(ContainerAPI<CONTAINER >::compare(v, *containerIter)); \
            QVERIFY(ContainerAPI<CONTAINER >::compare(v, varList.at(numSeen))); \
            ++containerIter; \
            ++numSeen; \
        } \
        QCOMPARE(numSeen, (int)std::distance(intList.begin(), intList.end()));

#else

#define TEST_RANGE_FOR(CONTAINER)

#endif

#define TEST_SEQUENTIAL_ITERATION_ON_FULL_NAME(CONTAINER) \
    { \
        int numSeen = 0; \
        CONTAINER intList; \
        ContainerAPI<CONTAINER >::insert(intList, 1); \
        ContainerAPI<CONTAINER >::insert(intList, 2); \
        ContainerAPI<CONTAINER >::insert(intList, 3); \
        \
        QVariant listVariant = QVariant::fromValue(intList); \
        QVERIFY(listVariant.canConvert<QVariantList>()); \
        QVariantList varList = listVariant.value<QVariantList>(); \
        QCOMPARE(varList.size(), (int)std::distance(intList.begin(), intList.end())); \
        QSequentialIterable listIter = listVariant.value<QSequentialIterable>(); \
        QCOMPARE(varList.size(), listIter.size()); \
        \
        CONTAINER::iterator containerIter = intList.begin(); \
        const CONTAINER::iterator containerEnd = intList.end(); \
        for (int i = 0; i < listIter.size(); ++i, ++containerIter, ++numSeen) \
        { \
            QVERIFY(ContainerAPI<CONTAINER >::compare(listIter.at(i), *containerIter)); \
            QVERIFY(ContainerAPI<CONTAINER >::compare(listIter.at(i), varList.at(i))); \
        } \
        QCOMPARE(numSeen, (int)std::distance(intList.begin(), intList.end())); \
        QCOMPARE(containerIter, containerEnd); \
        \
        containerIter = intList.begin(); \
        numSeen = 0; \
        Q_FOREACH (const QVariant &v, listIter) { \
            QVERIFY(ContainerAPI<CONTAINER >::compare(v, *containerIter)); \
            QVERIFY(ContainerAPI<CONTAINER >::compare(v, varList.at(numSeen))); \
            ++containerIter; \
            ++numSeen; \
        } \
        QCOMPARE(numSeen, (int)std::distance(intList.begin(), intList.end())); \
        TEST_RANGE_FOR(CONTAINER) \
    }

#define TEST_SEQUENTIAL_ITERATION(CONTAINER, VALUE_TYPE) \
    TEST_SEQUENTIAL_ITERATION_ON_FULL_NAME(CONTAINER<VALUE_TYPE > )


    TEST_SEQUENTIAL_ITERATION(QVector, int)
    TEST_SEQUENTIAL_ITERATION(QVector, QVariant)
    TEST_SEQUENTIAL_ITERATION(QVector, QString)
    TEST_SEQUENTIAL_ITERATION(QQueue, int)
    TEST_SEQUENTIAL_ITERATION(QQueue, QVariant)
    TEST_SEQUENTIAL_ITERATION(QQueue, QString)
    TEST_SEQUENTIAL_ITERATION(QList, int)
    TEST_SEQUENTIAL_ITERATION(QList, QVariant)
    TEST_SEQUENTIAL_ITERATION(QList, QString)
    TEST_SEQUENTIAL_ITERATION(QList, QByteArray)
    TEST_SEQUENTIAL_ITERATION(QStack, int)
    TEST_SEQUENTIAL_ITERATION(QStack, QVariant)
    TEST_SEQUENTIAL_ITERATION(QStack, QString)
    TEST_SEQUENTIAL_ITERATION(std::vector, int)
    TEST_SEQUENTIAL_ITERATION(std::vector, QVariant)
    TEST_SEQUENTIAL_ITERATION(std::vector, QString)
    TEST_SEQUENTIAL_ITERATION(std::list, int)
    TEST_SEQUENTIAL_ITERATION(std::list, QVariant)
    TEST_SEQUENTIAL_ITERATION(std::list, QString)
    TEST_SEQUENTIAL_ITERATION_ON_FULL_NAME(QStringList)
    TEST_SEQUENTIAL_ITERATION_ON_FULL_NAME(QByteArrayList)

#ifdef TEST_FORWARD_LIST
    TEST_SEQUENTIAL_ITERATION(std::forward_list, int)
    TEST_SEQUENTIAL_ITERATION(std::forward_list, QVariant)
    TEST_SEQUENTIAL_ITERATION(std::forward_list, QString)
#endif

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

#define TEST_ASSOCIATIVE_ITERATION(CONTAINER, KEY_TYPE, MAPPED_TYPE) \
    { \
        int numSeen = 0; \
        CONTAINER<KEY_TYPE, MAPPED_TYPE> mapping; \
        mapping[5] = true; \
        mapping[15] = false; \
        \
        QVariant mappingVariant = QVariant::fromValue(mapping); \
        QVariantMap varMap = mappingVariant.value<QVariantMap>(); \
        QVariantMap varHash = mappingVariant.value<QVariantMap>(); \
        QAssociativeIterable mappingIter = mappingVariant.value<QAssociativeIterable>(); \
        \
        CONTAINER<KEY_TYPE, MAPPED_TYPE>::const_iterator containerIter = mapping.begin(); \
        const CONTAINER<KEY_TYPE, MAPPED_TYPE>::const_iterator containerEnd = mapping.end(); \
        for ( ; containerIter != containerEnd; ++containerIter, ++numSeen) \
        { \
            MAPPED_TYPE expected = KeyGetter<CONTAINER<KEY_TYPE, MAPPED_TYPE> >::value(containerIter); \
            KEY_TYPE key = KeyGetter<CONTAINER<KEY_TYPE, MAPPED_TYPE> >::get(containerIter); \
            MAPPED_TYPE actual = mappingIter.value(key).value<MAPPED_TYPE >(); \
            QCOMPARE(varMap.value(QString::number(key)).value<MAPPED_TYPE>(), expected); \
            QCOMPARE(varHash.value(QString::number(key)).value<MAPPED_TYPE>(), expected); \
            QCOMPARE(actual, expected); \
            const QAssociativeIterable::const_iterator it = mappingIter.find(key); \
            QVERIFY(it != mappingIter.end()); \
            QCOMPARE(it.value().value<MAPPED_TYPE>(), expected); \
        } \
        QCOMPARE(numSeen, (int)std::distance(mapping.begin(), mapping.end())); \
        QCOMPARE(containerIter, containerEnd); \
        QVERIFY(mappingIter.find(10) == mappingIter.end()); \
    }

    TEST_ASSOCIATIVE_ITERATION(QHash, int, bool)
    TEST_ASSOCIATIVE_ITERATION(QMap, int, bool)
    TEST_ASSOCIATIVE_ITERATION(std::map, int, bool)
#ifdef TEST_UNORDERED_MAP
    TEST_ASSOCIATIVE_ITERATION(std::unordered_map, int, bool)
#endif

    {
        QMap<int, QString> mapping;
        mapping.insert(1, "one");
        mapping.insert(2, "two");
        mapping.insert(3, "three");
        QVariant var = QVariant::fromValue(mapping);
        QAssociativeIterable iter = var.value<QAssociativeIterable>();
        QAssociativeIterable::const_iterator it = iter.begin();
        QAssociativeIterable::const_iterator end = iter.end();
        QCOMPARE(*(mapping.begin() + 1), (*(it + 1)).toString());
        int i = 0;
        for ( ; it != end; ++it, ++i) {
            QCOMPARE(*(mapping.begin() + i), (*it).toString());
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
}

void tst_QVariant::pairElements()
{
    typedef QPair<QVariant, QVariant> QVariantPair;

#define TEST_PAIR_ELEMENT_ACCESS(PAIR, T1, T2, VALUE1, VALUE2) \
    { \
    PAIR<T1, T2> p(VALUE1, VALUE2); \
    QVariant v = QVariant::fromValue(p); \
    \
    QVERIFY(v.canConvert<QVariantPair>()); \
    QVariantPair pi = v.value<QVariantPair>(); \
    QCOMPARE(pi.first, QVariant::fromValue(VALUE1)); \
    QCOMPARE(pi.second, QVariant::fromValue(VALUE2)); \
    }

    TEST_PAIR_ELEMENT_ACCESS(QPair, int, int, 4, 5)
    TEST_PAIR_ELEMENT_ACCESS(std::pair, int, int, 4, 5)
    TEST_PAIR_ELEMENT_ACCESS(QPair, QString, QString, QStringLiteral("one"), QStringLiteral("two"))
    TEST_PAIR_ELEMENT_ACCESS(std::pair, QString, QString, QStringLiteral("one"), QStringLiteral("two"))
    TEST_PAIR_ELEMENT_ACCESS(QPair, QVariant, QVariant, 4, 5)
    TEST_PAIR_ELEMENT_ACCESS(std::pair, QVariant, QVariant, 4, 5)
    TEST_PAIR_ELEMENT_ACCESS(QPair, QVariant, int, 41, 15)
    TEST_PAIR_ELEMENT_ACCESS(std::pair, QVariant, int, 34, 65)
    TEST_PAIR_ELEMENT_ACCESS(QPair, int, QVariant, 24, 25)
    TEST_PAIR_ELEMENT_ACCESS(std::pair, int, QVariant, 44, 15)
}

enum EnumTest_Enum0 { EnumTest_Enum0_value = 42, EnumTest_Enum0_negValue = -8 };
Q_DECLARE_METATYPE(EnumTest_Enum0)
enum EnumTest_Enum1 { EnumTest_Enum1_value = 42, EnumTest_Enum1_bigValue = (Q_INT64_C(1) << 33) + 50 };
Q_DECLARE_METATYPE(EnumTest_Enum1)

#if defined(Q_COMPILER_CLASS_ENUM)
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
#endif

template<typename Enum> void testVariant(Enum value, bool *ok)
{
    *ok = false;
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
    if (sizeof(value) < 8 && static_cast<qint64>(value) < 0) {
        QEXPECT_FAIL("", "The metatype system don't store the sign of enums", Continue);
        // The value is stored internaly with 32 bit. When asked to convert it to 64 bit unsigned,
        // we consider that the value was unsigned, so we don't extent the bit signs
    }
    QCOMPARE(var.value<quint64>(), static_cast<quint64>(value));

    QVariant var2 = var;
    QVERIFY(var2.convert(QMetaType::Int));
    QCOMPARE(var2.value<int>(), static_cast<int>(value));

    if (static_cast<qint64>(value) <= INT_MAX) {
        int intValue = static_cast<int>(value);
        QVariant intVar = intValue;
        QVERIFY(intVar.canConvert<Enum>());
        QCOMPARE(intVar.value<Enum>(), value);
    }
    qint64 longValue = static_cast<qint64>(value);
    QVERIFY(QVariant(longValue).canConvert<Enum>());
    QCOMPARE(QVariant(longValue).value<Enum>(), value);

    *ok = true;
}

void tst_QVariant::enums()
{
    bool ok = false;
    testVariant(EnumTest_Enum0_value, &ok);
    QVERIFY(ok);
    testVariant(EnumTest_Enum0_negValue, &ok);
    QVERIFY(ok);
    testVariant(EnumTest_Enum1_value, &ok);
    QVERIFY(ok);
    testVariant(EnumTest_Enum1_bigValue, &ok);
    QVERIFY(ok);
#if defined(Q_COMPILER_CLASS_ENUM)
    testVariant(EnumTest_Enum3::EnumTest_Enum3_value, &ok);
    QVERIFY(ok);
    testVariant(EnumTest_Enum3::EnumTest_Enum3_bigValue, &ok);
    QVERIFY(ok);
    testVariant(EnumTest_Enum4::EnumTest_Enum4_value, &ok);
    QVERIFY(ok);
    testVariant(EnumTest_Enum4::EnumTest_Enum4_bigValue, &ok);
    QVERIFY(ok);
    testVariant(EnumTest_Enum5::EnumTest_Enum5_value, &ok);
    QVERIFY(ok);
    testVariant(EnumTest_Enum6::EnumTest_Enum6_value, &ok);
    QVERIFY(ok);
    testVariant(EnumTest_Enum7::EnumTest_Enum7_value, &ok);
    QVERIFY(ok);
    testVariant(EnumTest_Enum8::EnumTest_Enum8_value, &ok);
    QVERIFY(ok);
    testVariant(EnumTest_Enum3::EnumTest_Enum3_value, &ok);
    QVERIFY(ok);
#endif
}

template<typename Enum> void testVariantMeta(Enum value, bool *ok, const char *string)
{
    testVariant<Enum>(value, ok);
    QVERIFY(ok);
    *ok = false;

    QVariant var = QVariant::fromValue(value);
    QVERIFY(var.canConvert<QString>());
    QVERIFY(var.canConvert<QByteArray>());

    QCOMPARE(var.value<QString>(), QString::fromLatin1(string));
    QCOMPARE(var.value<QByteArray>(), QByteArray(string));

    QVariant strVar = QString::fromLatin1(string);
    QVERIFY(strVar.canConvert<Enum>());
    if (value > INT_MAX) {
        QEXPECT_FAIL("", "QMetaEnum api uses 'int' as return type  QTBUG-27451", Abort);
        *ok = true;
    }
    QCOMPARE(strVar.value<Enum>(), value);
    strVar = QByteArray(string);
    QVERIFY(strVar.canConvert<Enum>());
    QCOMPARE(strVar.value<Enum>(), value);
    *ok = true;
}

void tst_QVariant::metaEnums()
{
    bool ok = false;
#define METAENUMS_TEST(Value) \
    testVariantMeta(Value, &ok, #Value); QVERIFY(ok)

    METAENUMS_TEST(MetaEnumTest_Enum0_value);
    METAENUMS_TEST(MetaEnumTest_Enum1_value);
    METAENUMS_TEST(MetaEnumTest_Enum1_bigValue);
    METAENUMS_TEST(MetaEnumTest_Enum3_value);
    METAENUMS_TEST(MetaEnumTest_Enum3_bigValue);
    METAENUMS_TEST(MetaEnumTest_Enum4_value);
    METAENUMS_TEST(MetaEnumTest_Enum4_bigValue);
    METAENUMS_TEST(MetaEnumTest_Enum5_value);
    METAENUMS_TEST(MetaEnumTest_Enum6_value);
    METAENUMS_TEST(MetaEnumTest_Enum8_value);
}

void tst_QVariant::compareSanity_data()
{
    QTest::addColumn<QVariant>("value1");
    QTest::addColumn<QVariant>("value2");

    QTest::newRow( "int <>/== QUrl" ) << QVariant( 97 ) << QVariant(QUrl("a"));
    QTest::newRow( "int <>/== QChar" ) << QVariant( 97 ) << QVariant(QChar('a'));
    QTest::newRow( "int <>/== QString" ) << QVariant( 97 ) << QVariant(QString("a"));
    QTest::newRow( "QUrl <>/== QChar" ) << QVariant(QUrl("a")) << QVariant(QChar('a'));
    QTest::newRow( "QUrl <>/== QString" ) << QVariant(QUrl("a")) << QVariant(QString("a"));
    QTest::newRow( "QChar <>/== QString" ) << QVariant(QChar('a')) << QVariant(QString("a"));
}

void tst_QVariant::compareSanity()
{
    QFETCH(QVariant, value1);
    QFETCH(QVariant, value2);

    if (value1 == value2) {
        QVERIFY(!(value1 < value2) && !(value1 > value2));
    } else {
        QVERIFY(value1 != value2);
        QVERIFY((value1 < value2) || (value1 > value2));
    }
}

void tst_QVariant::accessSequentialContainerKey()
{
    QString nameResult;

    {
    QMap<QString, QObject*> mapping;
    QString name = QString::fromLatin1("Seven");
    mapping.insert(name, Q_NULLPTR);

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


QTEST_MAIN(tst_QVariant)
#include "tst_qvariant.moc"
