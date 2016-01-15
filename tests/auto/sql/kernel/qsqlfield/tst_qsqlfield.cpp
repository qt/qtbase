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


#include <QtTest/QtTest>

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
    QSqlField field( "Testfield", QVariant::Int );
    QFETCH( int, val );
    field.setValue( val );
    field.setReadOnly(true);
    field.clear();
    QVERIFY( field.value() == val );
    QVERIFY( !field.isNull() );

    QSqlField bfield( "Testfield", QVariant::Bool );
    QFETCH( bool, bval );
    bfield.setValue( QVariant(bval) );
    bfield.setReadOnly(true);
    bfield.clear();

    QVERIFY( bfield.value() == QVariant(bval) );
    QVERIFY( !bfield.isNull() );

    QSqlField ffield( "Testfield", QVariant::Double );
    QFETCH( double, fval );
    ffield.setValue( fval );
    ffield.setReadOnly(true);
    ffield.clear();
    QVERIFY( ffield.value() == fval );
    QVERIFY( !ffield.isNull() );

    QSqlField sfield( "Testfield", QVariant::String );
    QFETCH( QString, strVal );
    sfield.setValue( strVal );
    sfield.setReadOnly(true);
    sfield.clear();
    QVERIFY( sfield.value() == strVal );
    QVERIFY( !sfield.isNull() );
}

void tst_QSqlField::isNull()
{
    QSqlField field( "test", QVariant::String );
    QVERIFY( field.isNull() );
}

void tst_QSqlField::isReadOnly()
{
    QSqlField field( "test", QVariant::String );
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
    QSqlField field( "test", QVariant::String );
    QFETCH( QString, val );
    QCOMPARE(field.name(), QLatin1String("test"));
    field.setName( val );
    QCOMPARE(field.name(), val);
}

void tst_QSqlField::operator_Assign()
{
    QSqlField field1( "test", QVariant::String );
    field1.setValue( "Harry" );
    field1.setReadOnly( true );
    QSqlField field2 = field1;
    QVERIFY( field1 == field2 );
    QSqlField field3( "test", QVariant::Double );
    field3.clear();
    field1 = field3;
    QVERIFY( field1 == field3 );
}

void tst_QSqlField::operator_Equal()
{
    QSqlField field1( "test", QVariant::String );
    QSqlField field2( "test2", QVariant::String );
    QSqlField field3( "test", QVariant::Int );
    QVERIFY( !(field1 == field2) );
    QVERIFY( !(field1 == field3) );
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
    QSqlField field( "test", QVariant::String );
    QFETCH( QString, val );
    QCOMPARE(field.name(), QLatin1String("test"));
    field.setName( val );
    QCOMPARE(field.name(), val);
}

void tst_QSqlField::setNull()
{
    QSqlField field( "test", QVariant::String );
    field.setValue( "test" );
    field.clear();
    QVERIFY( field.value() == QVariant().toString() );
    QVERIFY( field.isNull() );
}

void tst_QSqlField::setReadOnly()
{
    QSqlField field( "test", QVariant::String );
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
    QSqlField field1 ( "test", QVariant::Int );
    QSqlField field2 ( "test", QVariant::String );
    QSqlField field3 ( "test", QVariant::Bool );
    QSqlField field4 ( "test", QVariant::Double );
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
    QSqlField field1( "string", QVariant::String );
    QSqlField field2( "string", QVariant::Bool );
    QSqlField field3( "string", QVariant::Double );
    QVERIFY( field1.type() == QVariant::String );
    QVERIFY( field2.type() == QVariant::Bool );
    QVERIFY( field3.type() == QVariant::Double );
}

QTEST_MAIN(tst_QSqlField)
#include "tst_qsqlfield.moc"
