// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

#include <qsqlfield.h>
#include <qvariant.h>
#include <qsqlfield.h>

class tst_QSqlField : public QObject
{
Q_OBJECT

public:
    tst_QSqlField();
    virtual ~tst_QSqlField();


public slots:
    void init();
    void cleanup();
private slots:
    void getSetCheck();
    void type();
    void setValue_data();
    void setValue();
    void setReadOnly();
    void setNull();
    void setName_data();
    void setName();
    void operator_Equal();
    void operator_Assign();
    void name_data();
    void name();
    void isReadOnly();
    void isNull();
    void clear_data();
    void clear();
    void setTableName_data();
    void setTableName();
    void moveSemantics();
};

// Testing get/set functions
void tst_QSqlField::getSetCheck()
{
    QSqlField obj1;
    // RequiredStatus QSqlField::requiredStatus()
    // void QSqlField::setRequiredStatus(RequiredStatus)
    obj1.setRequiredStatus(QSqlField::RequiredStatus(QSqlField::Unknown));
    QCOMPARE(QSqlField::RequiredStatus(QSqlField::Unknown), obj1.requiredStatus());
    obj1.setRequiredStatus(QSqlField::RequiredStatus(QSqlField::Optional));
    QCOMPARE(QSqlField::RequiredStatus(QSqlField::Optional), obj1.requiredStatus());
    obj1.setRequiredStatus(QSqlField::RequiredStatus(QSqlField::Required));
    QCOMPARE(QSqlField::RequiredStatus(QSqlField::Required), obj1.requiredStatus());

    // int QSqlField::length()
    // void QSqlField::setLength(int)
    obj1.setLength(0);
    QCOMPARE(0, obj1.length());
    obj1.setLength(INT_MIN);
    QCOMPARE(INT_MIN, obj1.length());
    obj1.setLength(INT_MAX);
    QCOMPARE(INT_MAX, obj1.length());

    // int QSqlField::precision()
    // void QSqlField::setPrecision(int)
    obj1.setPrecision(0);
    QCOMPARE(0, obj1.precision());
    obj1.setPrecision(INT_MIN);
    QCOMPARE(INT_MIN, obj1.precision());
    obj1.setPrecision(INT_MAX);
    QCOMPARE(INT_MAX, obj1.precision());
}

tst_QSqlField::tst_QSqlField()
{
}

tst_QSqlField::~tst_QSqlField()
{

}

void tst_QSqlField::init()
{
// TODO: Add initialization code here.
// This will be executed immediately before each test is run.
}

void tst_QSqlField::cleanup()
{
// TODO: Add cleanup code here.
// This will be executed immediately after each test is run.
}

void tst_QSqlField::clear_data()
{
    QTest::addColumn<int>("val");
    QTest::addColumn<bool>("bval");
    QTest::addColumn<QString>("strVal");
    QTest::addColumn<double>("fval");

    //next we fill it with data
    QTest::newRow( "data0" ) << (int)5 << true << QString("Hallo") << (double)0;
    QTest::newRow( "data1" )  << -5 << false << QString("NULL") << (double)-4;
    QTest::newRow( "data2" )  << 0 << false << QString("0") << (double)0;
}

void tst_QSqlField::clear()
{
    QSqlField field( "Testfield", QMetaType(QMetaType::Int) );
    QFETCH( int, val );
    field.setValue( val );
    field.setReadOnly(true);
    field.clear();
    QVERIFY( field.value() == val );
    QVERIFY( !field.isNull() );

    QSqlField bfield( "Testfield", QMetaType(QMetaType::Bool) );
    QFETCH( bool, bval );
    bfield.setValue( QVariant(bval) );
    bfield.setReadOnly(true);
    bfield.clear();

    QVERIFY( bfield.value() == QVariant(bval) );
    QVERIFY( !bfield.isNull() );

    QSqlField ffield( "Testfield", QMetaType(QMetaType::Double) );
    QFETCH( double, fval );
    ffield.setValue( fval );
    ffield.setReadOnly(true);
    ffield.clear();
    QVERIFY( ffield.value() == fval );
    QVERIFY( !ffield.isNull() );

    QSqlField sfield( "Testfield", QMetaType(QMetaType::QString) );
    QFETCH( QString, strVal );
    sfield.setValue( strVal );
    sfield.setReadOnly(true);
    sfield.clear();
    QVERIFY( sfield.value() == strVal );
    QVERIFY( !sfield.isNull() );
}

void tst_QSqlField::isNull()
{
    QSqlField field( "test", QMetaType(QMetaType::QString) );
    QVERIFY( field.isNull() );
}

void tst_QSqlField::isReadOnly()
{
    QSqlField field( "test", QMetaType(QMetaType::QString) );
    QVERIFY( !field.isReadOnly() );
    field.setReadOnly( true );
    QVERIFY( field.isReadOnly() );
    field.setReadOnly( false );
    QVERIFY( !field.isReadOnly() );
}

void tst_QSqlField::name_data()
{
    QTest::addColumn<QString>("val");

    //next we fill it with data
    QTest::newRow( "data0" )  << QString("test");
    QTest::newRow( "data1" )  << QString("Harry");
    QTest::newRow( "data2" )  << QString("");
}

void tst_QSqlField::name()
{
    QSqlField field( "test", QMetaType(QMetaType::QString) );
    QFETCH( QString, val );
    QCOMPARE(field.name(), QLatin1String("test"));
    field.setName( val );
    QCOMPARE(field.name(), val);
}

void tst_QSqlField::operator_Assign()
{
    QSqlField field1( "test", QMetaType(QMetaType::QString) );
    field1.setValue( "Harry" );
    field1.setReadOnly( true );
    QSqlField field2 = field1;
    QVERIFY( field1 == field2 );
    QSqlField field3( "test", QMetaType(QMetaType::Double) );
    field3.clear();
    field1 = field3;
    QVERIFY( field1 == field3 );
    QSqlField field4("test", QMetaType(QMetaType::QString), "ATable");
    field1 = field4;
    QVERIFY(field1 == field4);
}

void tst_QSqlField::operator_Equal()
{
    QSqlField field1( "test", QMetaType(QMetaType::QString) );
    QSqlField field2( "test2", QMetaType(QMetaType::QString) );
    QSqlField field3( "test", QMetaType(QMetaType::Int) );
    QSqlField field4("test", QMetaType(QMetaType::QString), QString("ATable"));
    QSqlField field5("test2", QMetaType(QMetaType::QString), QString("ATable"));
    QSqlField field6("test", QMetaType(QMetaType::QString), QString("BTable"));

    QVERIFY( !(field1 == field2) );
    QVERIFY( !(field1 == field3) );
    QVERIFY(field1 != field4);
    QVERIFY(field1 != field5);
    QVERIFY(field1 != field6);
    QVERIFY(field4 != field5);
    QVERIFY(field4 != field6);

    field2.setName( "test" );
    QVERIFY( field1 == field2 );
    QVERIFY( field1 == field2 );
    field1.setValue( "Harry" );
    QVERIFY( !(field1 == field2) );
    field2.setValue( "Harry" );
    QVERIFY( field1 == field2 );
    field1.setReadOnly( true );
    QVERIFY( !(field1 == field2) );
    field2.setReadOnly( true );
    QVERIFY( field1 == field2 );
    field4.setTableName("BTable");
    QCOMPARE(field4, field6);
    field6.setName("test3");
    QVERIFY(field4 != field6);
}

void tst_QSqlField::setName_data()
{
    QTest::addColumn<QString>("val");

    //next we fill it with data
    QTest::newRow( "data0" )  << QString("test");
    QTest::newRow( "data1" )  << QString("Harry");
    QTest::newRow( "data2" )  << QString("");
}

void tst_QSqlField::setName()
{
    QSqlField field( "test", QMetaType(QMetaType::QString) );
    QFETCH( QString, val );
    QCOMPARE(field.name(), QLatin1String("test"));
    field.setName( val );
    QCOMPARE(field.name(), val);
}

void tst_QSqlField::setNull()
{
    QSqlField field( "test", QMetaType(QMetaType::QString) );
    field.setValue( "test" );
    field.clear();
    QVERIFY( field.value() == QVariant().toString() );
    QVERIFY( field.isNull() );
}

void tst_QSqlField::setReadOnly()
{
    QSqlField field( "test", QMetaType(QMetaType::QString) );
    field.setValue( "test" );
    field.setReadOnly( true );
    field.setValue( "Harry" );
    QCOMPARE(field.value().toString(), QLatin1String("test"));
    field.clear();
    QCOMPARE(field.value().toString(), QLatin1String("test"));
    QVERIFY( !field.isNull() );
    field.clear();
    QCOMPARE(field.value().toString(), QLatin1String("test"));
    QVERIFY( !field.isNull() );
    field.setReadOnly( false );
    field.setValue( "Harry" );
    QCOMPARE(field.value().toString(), QLatin1String("Harry"));
    field.clear();
    QVERIFY( field.value() == QVariant().toString() );
    QVERIFY( field.isNull() );
}

void tst_QSqlField::setValue_data()
{
    QTest::addColumn<int>("ival");
    QTest::addColumn<bool>("bval");
    QTest::addColumn<double>("dval");
    QTest::addColumn<QString>("sval");

    //next we fill it with data
    QTest::newRow( "data0" )  << 0 << false << (double)223.232 << QString("");
    QTest::newRow( "data1" )  << 123 << true << (double)-232.232 << QString("Harry");
    QTest::newRow( "data2" )  << -123 << false << (double)232222.323223233338 << QString("Woipertinger");
}

void tst_QSqlField::setValue()
{
    QSqlField field1 ( "test", QMetaType(QMetaType::Int) );
    QSqlField field2 ( "test", QMetaType(QMetaType::QString) );
    QSqlField field3 ( "test", QMetaType(QMetaType::Bool) );
    QSqlField field4 ( "test", QMetaType(QMetaType::Double) );
    field1.clear();
    QFETCH( int, ival );
    QFETCH( QString, sval );
    QFETCH( double, dval );
    QFETCH( bool, bval );
    field1.setValue( ival );
    QCOMPARE( field1.value().toInt(), ival );
    // setValue should also reset the NULL flag
    QVERIFY( !field1.isNull() );

    field2.setValue( sval );
    QCOMPARE( field2.value().toString(), sval );
    field3.setValue( QVariant( bval) );
    QVERIFY( field3.value().toBool() == bval );
    field4.setValue( dval );
    QCOMPARE( field4.value().toDouble(), dval );
    field4.setReadOnly( true );
    field4.setValue( "Something_that's_not_a_double" );
    QCOMPARE( field4.value().toDouble(), dval );
}

void tst_QSqlField::type()
{
    QSqlField field1( "string", QMetaType(QMetaType::QString) );
    QSqlField field2( "string", QMetaType(QMetaType::Bool) );
    QSqlField field3( "string", QMetaType(QMetaType::Double) );
    QVERIFY( field1.metaType() == QMetaType(QMetaType::QString) );
    QVERIFY( field2.metaType() == QMetaType(QMetaType::Bool) );
    QVERIFY( field3.metaType() == QMetaType(QMetaType::Double) );
}

void tst_QSqlField::setTableName_data()
{
    QTest::addColumn<QString>("tableName");

    QTest::newRow("data0") << QString("");
    QTest::newRow("data1") << QString("tbl");
}

void tst_QSqlField::setTableName()
{
    QSqlField field("test", QMetaType(QMetaType::QString), "test");
    QFETCH(QString, tableName);
    QCOMPARE(field.tableName(), QLatin1String("test"));
    field.setTableName(tableName);
    QCOMPARE(field.tableName(), tableName);
}

void tst_QSqlField::moveSemantics()
{
    QSqlField field("test", QMetaType(QMetaType::QString), "testTable");
    QSqlField empty;
    field.setValue("string");
    auto moved = std::move(field);
    // `field` is now partially-formed

    // moving transfers state:
    QCOMPARE(moved.value().toString(), QLatin1String("string"));

    // moved-from objects can be assigned-to:
    field = empty;
    QVERIFY(field.value().isNull());

    // moved-from object can be destroyed:
    moved = std::move(field);
}

QTEST_MAIN(tst_QSqlField)
#include "tst_qsqlfield.moc"
