/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <qapplication.h>
#include <qsqldatabase.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#ifdef QT3_SUPPORT
#include <q3sqlcursor.h>
#endif
#include <qsqlrecord.h>
#include <qsql.h>
#include <qsqlresult.h>
#include <qsqldriver.h>
#include <qdebug.h>
#include <private/qsqlnulldriver_p.h>

#include "../qsqldatabase/tst_databases.h"

//TESTED_FILES=

class tst_QSql : public QObject
{
Q_OBJECT

public:
    tst_QSql();
    virtual ~tst_QSql();


public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
    void open();
    void openInvalid();
    void registerSqlDriver();

    // problem specific tests
    void openErrorRecovery();
    void concurrentAccess();
    void basicDriverTest();
};

/****************** General Qt SQL Module tests *****************/

tst_QSql::tst_QSql()
{
}

tst_QSql::~tst_QSql()
{
}

void tst_QSql::initTestCase()
{
}

void tst_QSql::cleanupTestCase()
{
}

void tst_QSql::init()
{
}

void tst_QSql::cleanup()
{
}


// this is a very basic test for drivers that cannot create/delete tables
// it can be used while developing new drivers,
// it's original purpose is to test ODBC Text datasources that are basically
// to stupid to do anything more advanced than SELECT/INSERT/UPDATE/DELETE
// the datasource has to have a table called "qtest_basictest" consisting
// of a field "id"(integer) and "name"(char/varchar).
void tst_QSql::basicDriverTest()
{
    int argc = 0;
    QApplication app( argc, 0, false );
    tst_Databases dbs;
    dbs.open();

    foreach( const QString& dbName, dbs.dbNames )
    {
        QSqlDatabase db = QSqlDatabase::database( dbName );
        QVERIFY_SQL( db, isValid() );

        QStringList tables = db.tables();
        QString tableName;

        if ( tables.contains( "qtest_basictest.txt" ) )
            tableName = "qtest_basictest.txt";
        else if ( tables.contains( "qtest_basictest" ) )
            tableName = "qtest_basictest";
        else if ( tables.contains( "QTEST_BASICTEST" ) )
            tableName = "QTEST_BASICTEST";
        else {
            QVERIFY( 1 );
            continue;
        }

        qDebug( qPrintable( QLatin1String( "Testing: " ) + tst_Databases::dbToString( db ) ) );

        QSqlRecord rInf = db.record( tableName );
        QCOMPARE( rInf.count(), 2 );
        QCOMPARE( rInf.fieldName( 0 ).toLower(), QString( "id" ) );
        QCOMPARE( rInf.fieldName( 1 ).toLower(), QString( "name" ) );

#ifdef QT3_SUPPORT
        QSqlRecord* rec = 0;
        Q3SqlCursor cur( tableName, true, db );
        QVERIFY_SQL( cur, select() );
        QCOMPARE( cur.count(), 2 );
        QCOMPARE( cur.fieldName( 0 ).lower(), QString( "id" ) );
        QCOMPARE( cur.fieldName( 1 ).lower(), QString( "name" ) );

        rec = cur.primeDelete();
        rec->setGenerated( 0, false );
        rec->setGenerated( 1, false );
        QVERIFY_SQL( cur, del() );
        QVERIFY_SQL( cur, select() );
        QCOMPARE( cur.at(), int( QSql::BeforeFirst ) );
        QVERIFY( !cur.next() );
        rec = cur.primeInsert();
        rec->setValue( 0, 1 );
        rec->setValue( 1, QString( "Harry" ) );
        QVERIFY_SQL( cur, insert( false ) );
        rec = cur.primeInsert();
        rec->setValue( 0, 2 );
        rec->setValue( 1, QString( "Trond" ) );
        QVERIFY_SQL( cur, insert( true ) );
        QVERIFY_SQL( cur, select( cur.index( QString( "id" ) ) ) );
        QVERIFY_SQL( cur, next() );
        QCOMPARE( cur.value( 0 ).toInt(), 1 );
        QCOMPARE( cur.value( 1 ).toString().stripWhiteSpace(), QString( "Harry" ) );
        QVERIFY_SQL( cur, next() );
        QCOMPARE( cur.value( 0 ).toInt(), 2 );
        QCOMPARE( cur.value( 1 ).toString().stripWhiteSpace(), QString( "Trond" ) );
        QVERIFY( !cur.next() );
        QVERIFY_SQL( cur, first() );
        rec = cur.primeUpdate();
        rec->setValue( 1, QString( "Vohi" ) );
        QVERIFY_SQL( cur, update( true ) );
        QVERIFY_SQL( cur, select( "id = 1" ) );
        QVERIFY_SQL( cur, next() );
        QCOMPARE( cur.value( 0 ).toInt(), 1 );
        QCOMPARE( cur.value( 1 ).toString().stripWhiteSpace(), QString( "Vohi" ) );
#endif
    }

    dbs.close();
    QVERIFY( 1 ); // make sure the test doesn't fail if no database drivers are there
}

// make sure that the static stuff will be deleted
// when using multiple QApplication objects
void tst_QSql::open()
{
    int i;
    int argc = 0;
    int count = -1;
    for ( i = 0; i < 10; ++i ) {

	QApplication app( argc, 0, false );
	tst_Databases dbs;

	dbs.open();
	if ( count == -1 )
	    // first iteration: see how many dbs are open
	    count = (int) dbs.dbNames.count();
	else
	    // next iterations: make sure all are opened again
	    QCOMPARE( count, (int)dbs.dbNames.count() );
	dbs.close();
    }
}

void tst_QSql::openInvalid()
{
    QSqlDatabase db;
    QVERIFY(!db.open());

    QSqlDatabase db2 = QSqlDatabase::addDatabase("doesnt_exist_will_never_exist", "blah");
    QFAIL_SQL(db2, open());
}

void tst_QSql::concurrentAccess()
{
    int argc = 0;
    QApplication app( argc, 0, false );
    tst_Databases dbs;

    dbs.open();
    foreach ( const QString& dbName, dbs.dbNames ) {
	QSqlDatabase db = QSqlDatabase::database( dbName );
	QVERIFY( db.isValid() );
        if (tst_Databases::isMSAccess(db))
            continue;

	QSqlDatabase ndb = QSqlDatabase::addDatabase( db.driverName(), "tst_QSql::concurrentAccess" );
	ndb.setDatabaseName( db.databaseName() );
	ndb.setHostName( db.hostName() );
	ndb.setPort( db.port() );
	ndb.setUserName( db.userName() );
	ndb.setPassword( db.password() );
	QVERIFY_SQL( ndb, open() );

	QCOMPARE( db.tables(), ndb.tables() );
    }
    // no database servers installed - don't fail
    QVERIFY(1);
    dbs.close();
}

void tst_QSql::openErrorRecovery()
{
    int argc = 0;
    QApplication app( argc, 0, false );
    tst_Databases dbs;

    dbs.addDbs();
    if (dbs.dbNames.isEmpty())
        QSKIP("No database drivers installed", SkipAll);
    foreach ( const QString& dbName, dbs.dbNames ) {
	QSqlDatabase db = QSqlDatabase::database( dbName, false );
	CHECK_DATABASE( db );

	QString userName = db.userName();
	QString password = db.password();

	// force an open error
	if ( db.open( "dummy130977", "doesnt_exist" ) ) {
	    qDebug( qPrintable(QLatin1String("Promiscuous database server without access control - test skipped for ") +
		    tst_Databases::dbToString( db )) );
	    QVERIFY(1);
	    continue;
	}

	QFAIL_SQL( db, isOpen() );
	QVERIFY_SQL( db, isOpenError() );

	// now open it
	if ( !db.open( userName, password ) ) {
	    qDebug() << "Could not open Database " << tst_Databases::dbToString( db ) <<
		    ". Assuming DB is down, skipping... (Error: " << 
		    tst_Databases::printError( db.lastError() ) << ")";
	    continue;
	}
	QVERIFY_SQL( db, open( userName, password ) );
	QVERIFY_SQL( db, isOpen() );
	QFAIL_SQL( db, isOpenError() );
	db.close();
	QFAIL_SQL( db, isOpen() );

	// force another open error
	QFAIL_SQL( db, open( "dummy130977", "doesnt_exist" ) );
	QFAIL_SQL( db, isOpen() );
	QVERIFY_SQL( db, isOpenError() );
    }
}

void tst_QSql::registerSqlDriver()
{
    int argc = 0;
    QApplication app( argc, 0, false );

    QSqlDatabase::registerSqlDriver( "QSQLTESTDRIVER", new QSqlDriverCreator<QSqlNullDriver> );
    QVERIFY( QSqlDatabase::drivers().contains( "QSQLTESTDRIVER" ) );

    QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLTESTDRIVER" );
    QVERIFY( db.isValid() );

    QCOMPARE( db.tables(), QStringList() );
}

QTEST_APPLESS_MAIN(tst_QSql)
#include "tst_qsql.moc"
