/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <qsqldatabase.h>
#include <qsqlquery.h>
#include <qsqldriver.h>
#include <qsqlrecord.h>
#include <qsqlfield.h>
#include <qsqlindex.h>
#include <qregexp.h>
#include <qvariant.h>
#include <qdatetime.h>
#include <qdebug.h>

#include "tst_databases.h"

Q_DECLARE_METATYPE(QSqlDriver::NotificationSource)

QT_FORWARD_DECLARE_CLASS(QSqlDatabase)
struct FieldDef;

class tst_QSqlDatabase : public QObject
{
    Q_OBJECT

public:
    tst_QSqlDatabase();
    virtual ~tst_QSqlDatabase();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
    void record_data() { generic_data(); }
    //void record();
    void open_data() { generic_data(); }
    void open();
    void tables_data() { generic_data(); }
    void tables();
    void oci_tables_data() { generic_data("QOCI"); }
    void oci_tables();
    void transaction_data() { generic_data(); }
    void transaction();
    void eventNotification_data() { generic_data(); }
    void eventNotification();
    void addDatabase();
    void errorReporting_data();
    void errorReporting();

    //database specific tests
    void recordMySQL_data() { generic_data("QMYSQL"); }
    void recordMySQL();
    void recordPSQL_data() { generic_data("QPSQL"); }
    void recordPSQL();
    void recordOCI_data() { generic_data("QOCI"); }
    void recordOCI();
    void recordTDS_data() { generic_data("QTDS"); }
    void recordTDS();
    void recordDB2_data() { generic_data("QDB2"); }
    void recordDB2();
    void recordSQLite_data() { generic_data("QSQLITE"); }
    void recordSQLite();
    void recordAccess_data() { generic_data("QODBC"); }
    void recordAccess();
    void recordSQLServer_data() { generic_data("QODBC"); }
    void recordSQLServer();
    void recordIBase_data() {generic_data("QIBASE"); }
    void recordIBase();

    void eventNotificationIBase_data() { generic_data("QIBASE"); }
    void eventNotificationIBase();
    void eventNotificationPSQL_data() { generic_data("QPSQL"); }
    void eventNotificationPSQL();

    //database specific 64 bit integer test
    void bigIntField_data() { generic_data(); }
    void bigIntField();

    // general tests
    void getConnectionName_data() { generic_data(); }
    void getConnectionName(); // For task 129992

    //problem specific tests
    void alterTable_data() { generic_data(); }
    void alterTable();
    void caseSensivity_data() { generic_data(); }
    void caseSensivity();
    void noEscapedFieldNamesInRecord_data() { generic_data(); }
    void noEscapedFieldNamesInRecord();
    void whitespaceInIdentifiers_data() { generic_data(); }
    void whitespaceInIdentifiers();
    void formatValueTrimStrings_data() { generic_data(); }
    void formatValueTrimStrings();
    void precisionPolicy_data() { generic_data(); }
    void precisionPolicy();

    void db2_valueCacheUpdate_data() { generic_data("QDB2"); }
    void db2_valueCacheUpdate();

    void psql_schemas_data() { generic_data("QPSQL"); }
    void psql_schemas();
    void psql_escapedIdentifiers_data() { generic_data("QPSQL"); }
    void psql_escapedIdentifiers();
    void psql_escapeBytea_data() { generic_data("QPSQL"); }
    void psql_escapeBytea();
    void psql_bug249059_data() { generic_data("QPSQL"); }
    void psql_bug249059();

    void mysqlOdbc_unsignedIntegers_data() { generic_data(); }
    void mysqlOdbc_unsignedIntegers();
    void mysql_multiselect_data() { generic_data("QMYSQL"); }
    void mysql_multiselect();  // For task 144331
    void mysql_savepointtest_data() { generic_data("QMYSQL"); }
    void mysql_savepointtest();

    void accessOdbc_strings_data() { generic_data(); }
    void accessOdbc_strings();

    void ibase_numericFields_data() { generic_data("QIBASE"); }
    void ibase_numericFields(); // For task 125053
    void ibase_fetchBlobs_data() { generic_data("QIBASE"); }
    void ibase_fetchBlobs(); // For task 143471
    void ibase_useCustomCharset_data() { generic_data("QIBASE"); }
    void ibase_useCustomCharset(); // For task 134608
    void ibase_procWithoutReturnValues_data() { generic_data("QIBASE"); } // For task 165423
    void ibase_procWithoutReturnValues();
    void ibase_procWithReturnValues_data() { generic_data("QIBASE"); } // For task 177530
    void ibase_procWithReturnValues();

    void odbc_reopenDatabase_data() { generic_data("QODBC"); }
    void odbc_reopenDatabase();
    void odbc_uniqueidentifier_data() { generic_data("QODBC"); }
    void odbc_uniqueidentifier(); // For task 141822
    void odbc_uintfield_data() { generic_data("QODBC"); }
    void odbc_uintfield();
    void odbc_bindBoolean_data() { generic_data("QODBC"); }
    void odbc_bindBoolean();
    void odbc_testqGetString_data() { generic_data("QODBC"); }
    void odbc_testqGetString();

    void oci_serverDetach_data() { generic_data("QOCI"); }
    void oci_serverDetach(); // For task 154518
    void oci_xmltypeSupport_data() { generic_data("QOCI"); }
    void oci_xmltypeSupport();
    void oci_fieldLength_data() { generic_data("QOCI"); }
    void oci_fieldLength();
    void oci_synonymstest_data() { generic_data("QOCI"); }
    void oci_synonymstest();

    void sqlite_bindAndFetchUInt_data() { generic_data("QSQLITE"); }
    void sqlite_bindAndFetchUInt();

    void sqlStatementUseIsNull_189093_data() { generic_data(); }
    void sqlStatementUseIsNull_189093();

    void sqlite_enable_cache_mode_data() { generic_data("QSQLITE"); }
    void sqlite_enable_cache_mode();

private:
    void createTestTables(QSqlDatabase db);
    void dropTestTables(QSqlDatabase db);
    void populateTestTables(QSqlDatabase db);
    void generic_data(const QString &engine=QString());

    void testRecord(const FieldDef fieldDefs[], const QSqlRecord& inf, QSqlDatabase db);
    void commonFieldTest(const FieldDef fieldDefs[], QSqlDatabase, const int);

    tst_Databases dbs;
};

// number of records to be inserted per testfunction
static const int ITERATION_COUNT = 2;

//helper class for database specific tests
struct FieldDef {
    FieldDef(QString tn = QString(),
          QVariant::Type t = QVariant::Invalid,
          QVariant v = QVariant(),
          bool nl = true):
        typeName(tn), type(t), val(v), nullable(nl) {}

    QString fieldName() const
    {
        QString rt = typeName;
        rt.replace(QRegExp("\\s"), QString("_"));
        int i = rt.indexOf("(");
        if (i == -1)
            i = rt.length();
        if (i > 20)
            i = 20;
        return "t_" + rt.left(i);
    }
    QString typeName;
    QVariant::Type type;
    QVariant val;
    bool nullable;
};

// creates a table out of the FieldDefs and returns the number of fields
// excluding the primary key field
static int createFieldTable(const FieldDef fieldDefs[], QSqlDatabase db)
{
    QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);
    const QString tableName = qTableName("qtestfields", __FILE__, db);
    tst_Databases::safeDropTable(db, tableName);
    QSqlQuery q(db);
    // construct a create table statement consisting of all fieldtypes
    QString qs = "create table " + tableName;
    QString autoName = tst_Databases::autoFieldName(db);
    if (tst_Databases::isMSAccess(db))
        qs.append(" (id int not null");
    else if (dbType == QSqlDriverPrivate::PostgreSQL)
        qs.append(" (id serial not null");
    else
        qs.append(QString("(id integer not null %1 primary key").arg(autoName));

    int i = 0;
    for (i = 0; !fieldDefs[ i ].typeName.isNull(); ++i) {
        qs += QString(",\n %1 %2").arg(fieldDefs[ i ].fieldName()).arg(fieldDefs[ i ].typeName);
        if ((dbType == QSqlDriverPrivate::Sybase || dbType == QSqlDriverPrivate::MSSqlServer) && fieldDefs[i].nullable)
            qs += " null";
    }

    if (tst_Databases::isMSAccess(db))
        qs.append(",\n primary key (id)");

    qs += ')';
    if (!q.exec(qs)) {
        qDebug() << "Creation of Table failed:" << tst_Databases::printError(q.lastError(), db);
        qDebug() << "Query: " << qs;
        return -1;
    }
    return i;
}

tst_QSqlDatabase::tst_QSqlDatabase()
{
}

tst_QSqlDatabase::~tst_QSqlDatabase()
{
}

void tst_QSqlDatabase::createTestTables(QSqlDatabase db)
{
    if (!db.isValid())
    return;
    const QString tableName = qTableName("qtest", __FILE__, db);
    QSqlQuery q(db);
    QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);
    if (dbType == QSqlDriverPrivate::MySqlServer) {
        // ### stupid workaround until we find a way to hardcode this
        // in the MySQL server startup script
        q.exec("set table_type=innodb");
    } else if (dbType == QSqlDriverPrivate::MSSqlServer) {
        QVERIFY_SQL(q, exec("SET ANSI_DEFAULTS ON"));
        QVERIFY_SQL(q, exec("SET IMPLICIT_TRANSACTIONS OFF"));
    } else if (dbType == QSqlDriverPrivate::PostgreSQL) {
        QVERIFY_SQL( q, exec("set client_min_messages='warning'"));
    }
    // please never ever change this table; otherwise fix all tests ;)
    if (tst_Databases::isMSAccess(db)) {
        QVERIFY_SQL(q, exec("create table " + tableName +
                   " (id int not null, t_varchar varchar(40) not null, t_char char(40), "
                   "t_numeric number, primary key (id, t_varchar))"));
    } else {
        QVERIFY_SQL(q, exec("create table " + tableName +
               " (id integer not null, t_varchar varchar(40) not null, "
               "t_char char(40), t_numeric numeric(6, 3), primary key (id, t_varchar))"));
    }

    if (testWhiteSpaceNames(db.driverName())) {
        QString qry = "create table "
            + db.driver()->escapeIdentifier(tableName + " test", QSqlDriver::TableName)
            + '('
            + db.driver()->escapeIdentifier(QLatin1String("test test"), QSqlDriver::FieldName)
            + " int not null primary key)";
        QVERIFY_SQL(q, exec(qry));
    }
}

void tst_QSqlDatabase::dropTestTables(QSqlDatabase db)
{
    if (!db.isValid())
        return;

    QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);
    if (dbType == QSqlDriverPrivate::PostgreSQL) {
        QSqlQuery q(db);
        QVERIFY_SQL( q, exec("set client_min_messages='warning'"));
    }

    // drop the view first, otherwise we'll get dependency problems
    tst_Databases::safeDropViews(db, QStringList() << qTableName("qtest_view", __FILE__, db) << qTableName("qtest_view2", __FILE__, db));
    const QString qtestTable = qTableName("qtest", __FILE__, db);
    QStringList tableNames;
    tableNames << qtestTable
            << qTableName("qtestfields", __FILE__, db)
            << qTableName("qtestalter", __FILE__, db)
            << qTableName("qtest_temp", __FILE__, db)
            << qTableName("qtest_bigint", __FILE__, db)
            << qTableName("qtest_xmltype", __FILE__, db)
            << qTableName("latin1table", __FILE__, db)
            << qTableName("qtest_sqlguid", __FILE__, db)
            << qTableName("batable", __FILE__, db)
            << qTableName("qtest_prec", __FILE__, db)
            << qTableName("uint", __FILE__, db)
            << qTableName("strings", __FILE__, db)
            << qTableName("numericfields", __FILE__, db)
            << qTableName("qtest_ibaseblobs", __FILE__, db)
            << qTableName("qtestBindBool", __FILE__, db)
            << qTableName("testqGetString", __FILE__, db)
            << qTableName("qtest_sqlguid", __FILE__, db)
            << qTableName("uint_table", __FILE__, db)
            << qTableName("uint_test", __FILE__, db)
            << qTableName("bug_249059", __FILE__, db);

    QSqlQuery q(0, db);
    if (dbType == QSqlDriverPrivate::PostgreSQL) {
        q.exec("drop schema " + qTableName("qtestschema", __FILE__, db) + " cascade");
        q.exec("drop schema " + qTableName("qtestScHeMa", __FILE__, db) + " cascade");
    }

    if (testWhiteSpaceNames(db.driverName()))
        tableNames <<  db.driver()->escapeIdentifier(qtestTable + " test", QSqlDriver::TableName);

    tst_Databases::safeDropTables(db, tableNames);

    if (dbType == QSqlDriverPrivate::Oracle) {
        q.exec("drop user "+qTableName("CREATOR", __FILE__, db)+ " cascade");
        q.exec("drop user "+qTableName("APPUSER", __FILE__, db) + " cascade");
        q.exec("DROP TABLE sys."+qTableName("mypassword", __FILE__, db));

    }
}

void tst_QSqlDatabase::populateTestTables(QSqlDatabase db)
{
    if (!db.isValid())
        return;
    QSqlQuery q(db);
    const QString qtest(qTableName("qtest", __FILE__, db));

    q.exec("delete from " + qtest); //non-fatal
    QVERIFY_SQL(q, exec("insert into " + qtest + " (id, t_varchar, t_char, t_numeric) values (0, 'VarChar0', 'Char0', 1.1)"));
    QVERIFY_SQL(q, exec("insert into " + qtest + " (id, t_varchar, t_char, t_numeric) values (1, 'VarChar1', 'Char1', 2.2)"));
    QVERIFY_SQL(q, exec("insert into " + qtest + " (id, t_varchar, t_char, t_numeric) values (2, 'VarChar2', 'Char2', 3.3)"));
    QVERIFY_SQL(q, exec("insert into " + qtest + " (id, t_varchar, t_char, t_numeric) values (3, 'VarChar3', 'Char3', 4.4)"));
    QVERIFY_SQL(q, exec("insert into " + qtest + " (id, t_varchar, t_char, t_numeric) values (4, 'VarChar4', NULL, NULL)"));
}

void tst_QSqlDatabase::initTestCase()
{
    qRegisterMetaType<QSqlDriver::NotificationSource>("QSqlDriver::NotificationSource");
    QVERIFY(dbs.open());

    for (QStringList::ConstIterator it = dbs.dbNames.begin(); it != dbs.dbNames.end(); ++it) {
        QSqlDatabase db = QSqlDatabase::database((*it));
        CHECK_DATABASE(db);
        dropTestTables(db); //in case of leftovers
        createTestTables(db);
        populateTestTables(db);
    }
}

void tst_QSqlDatabase::cleanupTestCase()
{
    for (QStringList::ConstIterator it = dbs.dbNames.begin(); it != dbs.dbNames.end(); ++it) {
        QSqlDatabase db = QSqlDatabase::database((*it));
        CHECK_DATABASE(db);
        dropTestTables(db);
    }

    dbs.close();
}

void tst_QSqlDatabase::init()
{
}

void tst_QSqlDatabase::cleanup()
{
}

void tst_QSqlDatabase::generic_data(const QString& engine)
{
    if ( dbs.fillTestTable(engine) == 0 ) {
        if(engine.isEmpty())
           QSKIP( "No database drivers are available in this Qt configuration");
        else
           QSKIP( (QString("No database drivers of type %1 are available in this Qt configuration").arg(engine)).toLocal8Bit());
    }
}

void tst_QSqlDatabase::addDatabase()
{
    QTest::ignoreMessage(QtWarningMsg, "QSqlDatabase: BLAH_FOO_NONEXISTENT_DRIVER driver not loaded");
    QTest::ignoreMessage(QtWarningMsg, qPrintable("QSqlDatabase: available drivers: " + QSqlDatabase::drivers().join(QLatin1Char(' '))));
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("BLAH_FOO_NONEXISTENT_DRIVER",
                                                    "INVALID_CONNECTION");
        QVERIFY(!db.isValid());
    }
    QVERIFY(QSqlDatabase::contains("INVALID_CONNECTION"));
    QSqlDatabase::removeDatabase("INVALID_CONNECTION");
    QVERIFY(!QSqlDatabase::contains("INVALID_CONNECTION"));
}

void tst_QSqlDatabase::errorReporting_data()
{
    QTest::addColumn<QString>("driver");

    QTest::newRow("QTDS") << QString::fromLatin1("QTDS");
    QTest::newRow("QTDS7") << QString::fromLatin1("QTDS7");
}

void tst_QSqlDatabase::errorReporting()
{
    QFETCH(QString, driver);

    if (!QSqlDatabase::drivers().contains(driver))
        QSKIP(QString::fromLatin1("Database driver %1 not available").arg(driver).toLocal8Bit().constData());

    const QString dbName = QLatin1String("errorReportingDb-") + driver;
    QSqlDatabase db = QSqlDatabase::addDatabase(driver, dbName);

    db.setHostName(QLatin1String("127.0.0.1"));
    db.setDatabaseName(QLatin1String("NonExistantDatabase"));
    db.setUserName(QLatin1String("InvalidUser"));
    db.setPassword(QLatin1String("IncorrectPassword"));

    QVERIFY(!db.open());

    db = QSqlDatabase();

    QSqlDatabase::removeDatabase(dbName);
}

void tst_QSqlDatabase::open()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    int i;
    for (i = 0; i < 10; ++i) {
        db.close();
        QVERIFY(!db.isOpen());
        QVERIFY_SQL(db, open());
        QVERIFY(db.isOpen());
        QVERIFY(!db.isOpenError());
    }

    QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);
    if (dbType == QSqlDriverPrivate::SQLite && db.databaseName() == ":memory:") {
        // tables in in-memory databases don't survive an open/close
        createTestTables(db);
        populateTestTables(db);
    }
}

void tst_QSqlDatabase::tables()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);

    const QString qtest(qTableName("qtest", __FILE__, db)), qtest_view(qTableName("qtest_view", __FILE__, db)), temp_tab(qTableName("test_tab", __FILE__, db));

    bool views = true;
    bool tempTables = false;

    QSqlQuery q(db);
    if ( db.driverName().startsWith( "QMYSQL" ) && tst_Databases::getMySqlVersion( db ).section( QChar('.'), 0, 0 ).toInt()<5 )
        QSKIP( "Test requires MySQL >= 5.0");


    if (!q.exec("CREATE VIEW " + qtest_view + " as select * from " + qtest)) {
        qDebug("DBMS '%s' cannot handle VIEWs: %s",
               qPrintable(tst_Databases::dbToString(db)),
               qPrintable(tst_Databases::printError(q.lastError())));
        views = false;
    }

    if (db.driverName().startsWith("QSQLITE3")) {
        QVERIFY_SQL(q, exec("CREATE TEMPORARY TABLE " + temp_tab + " (id int)"));
        tempTables = true;
    }

    QStringList tables = db.tables(QSql::Tables);
    QVERIFY(tables.contains(qtest, Qt::CaseInsensitive));
    QVERIFY(!tables.contains("sql_features", Qt::CaseInsensitive)); //check for postgres 7.4 internal tables
    if (views) {
        QVERIFY(!tables.contains(qtest_view, Qt::CaseInsensitive));
    }
    if (tempTables)
        QVERIFY(tables.contains(temp_tab, Qt::CaseInsensitive));

    tables = db.tables(QSql::Views);
    if (views) {
        if(!tables.contains(qtest_view, Qt::CaseInsensitive))
            qDebug() << "failed to find" << qtest_view << "in" << tables;
        QVERIFY(tables.contains(qtest_view, Qt::CaseInsensitive));
    }
    if (tempTables)
        QVERIFY(!tables.contains(temp_tab, Qt::CaseInsensitive));
    QVERIFY(!tables.contains(qtest, Qt::CaseInsensitive));

    tables = db.tables(QSql::SystemTables);
    QVERIFY(!tables.contains(qtest, Qt::CaseInsensitive));
    QVERIFY(!tables.contains(qtest_view, Qt::CaseInsensitive));
    QVERIFY(!tables.contains(temp_tab, Qt::CaseInsensitive));

    tables = db.tables(QSql::AllTables);
    if (views)
        QVERIFY(tables.contains(qtest_view, Qt::CaseInsensitive));
    if (tempTables)
        QVERIFY(tables.contains(temp_tab, Qt::CaseInsensitive));
    QVERIFY(tables.contains(qtest, Qt::CaseInsensitive));

    if (dbType == QSqlDriverPrivate::PostgreSQL)
        QVERIFY(tables.contains(qtest + " test"));
}

void tst_QSqlDatabase::whitespaceInIdentifiers()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);

    if (testWhiteSpaceNames(db.driverName())) {
        const QString tableName(qTableName("qtest", __FILE__, db) + " test");
        QVERIFY(db.tables().contains(tableName, Qt::CaseInsensitive));

        QSqlRecord rec = db.record(db.driver()->escapeIdentifier(tableName, QSqlDriver::TableName));
        QCOMPARE(rec.count(), 1);
        QCOMPARE(rec.fieldName(0), QString("test test"));
        if (dbType == QSqlDriverPrivate::Oracle)
            QCOMPARE(rec.field(0).type(), QVariant::Double);
        else
            QCOMPARE(rec.field(0).type(), QVariant::Int);

        QSqlIndex idx = db.primaryIndex(db.driver()->escapeIdentifier(tableName, QSqlDriver::TableName));
        QCOMPARE(idx.count(), 1);
        QCOMPARE(idx.fieldName(0), QString("test test"));
        if (dbType == QSqlDriverPrivate::Oracle)
            QCOMPARE(idx.field(0).type(), QVariant::Double);
        else
            QCOMPARE(idx.field(0).type(), QVariant::Int);
    } else {
        QSKIP("DBMS does not support whitespaces in identifiers");
    }
}

void tst_QSqlDatabase::alterTable()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QString qtestalter(qTableName("qtestalter", __FILE__, db));

    QSqlQuery q(db);

    QVERIFY_SQL(q, exec("create table " + qtestalter + " (F1 char(20), F2 char(20), F3 char(20))"));
    QSqlRecord rec = db.record(qtestalter);
    QCOMPARE((int)rec.count(), 3);

    int i;
    for (i = 0; i < 3; ++i) {
        QCOMPARE(rec.field(i).name().toUpper(), QString("F%1").arg(i + 1));
    }

    if (!q.exec("alter table " + qtestalter + " drop column F2")) {
        QSKIP("DBMS doesn't support dropping columns in ALTER TABLE statement");
    }

    rec = db.record(qtestalter);

    QCOMPARE((int)rec.count(), 2);

    QCOMPARE(rec.field(0).name().toUpper(), QString("F1"));
    QCOMPARE(rec.field(1).name().toUpper(), QString("F3"));

    q.exec("select * from " + qtestalter);
}

#if 0
// this is the general test that should work on all databases.
// unfortunately no DBMS supports SQL 92/ 99 so the general
// test is more or less a joke. Please write a test for each
// database plugin (see recordOCI and so on). Use this test
// as a template.
void tst_QSqlDatabase::record()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    static const FieldDef fieldDefs[] = {
        FieldDef("char(20)", QVariant::String,         QString("blah1"), false),
        FieldDef("varchar(20)", QVariant::String,      QString("blah2"), false),
        FieldDef()
    };

    const int fieldCount = createFieldTable(fieldDefs, db);
    QVERIFY(fieldCount > 0);

    commonFieldTest(fieldDefs, db, fieldCount);
}
#endif

void tst_QSqlDatabase::testRecord(const FieldDef fieldDefs[], const QSqlRecord& inf, QSqlDatabase db)
{
    int i = 0;
    if (!tst_Databases::autoFieldName(db).isEmpty()) // Currently only MySQL is tested
        QVERIFY2(inf.field(i).isAutoValue(), qPrintable(inf.field(i).name() + " should be reporting as an autovalue"));
    for (i = 0; !fieldDefs[ i ].typeName.isNull(); ++i) {
        QCOMPARE(inf.field(i+1).name().toUpper(), fieldDefs[ i ].fieldName().toUpper());
        if (inf.field(i+1).type() != fieldDefs[ i ].type) {
            QFAIL(qPrintable(QString(" Expected: '%1' Received: '%2' for field %3 in testRecord").arg(
              QVariant::typeToName(fieldDefs[ i ].type)).arg(
            QVariant::typeToName(inf.field(i+1).type())).arg(
              fieldDefs[ i ].fieldName())));
        }
        QVERIFY(!inf.field(i+1).isAutoValue());

//      qDebug(QString(" field: %1 type: %2 variant type: %3").arg(fieldDefs[ i ].fieldName()).arg(QVariant::typeToName(inf.field(i+1)->type())).arg(QVariant::typeToName(inf.field(i+1)->value().type())));
    }
}

// non-dbms specific tests
void tst_QSqlDatabase::commonFieldTest(const FieldDef fieldDefs[], QSqlDatabase db, const int fieldCount)
{
    CHECK_DATABASE(db);
    const QString tableName = qTableName("qtestfields", __FILE__, db);
    QSqlRecord rec = db.record(tableName);
    QCOMPARE((int)rec.count(), fieldCount+1);
    testRecord(fieldDefs, rec, db);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec("select * from " + tableName));
}

void tst_QSqlDatabase::recordTDS()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    static const FieldDef fieldDefs[] = {
        FieldDef("tinyint", QVariant::Int,              255),
        FieldDef("smallint", QVariant::Int,             32767),
        FieldDef("int", QVariant::Int,                  2147483647),
        FieldDef("numeric(10,9)", QVariant::Double,     1.23456789),
        FieldDef("decimal(10,9)", QVariant::Double,     1.23456789),
        FieldDef("float(4)", QVariant::Double,          1.23456789),
        FieldDef("double precision", QVariant::Double,  1.23456789),
        FieldDef("real", QVariant::Double,              1.23456789),
        FieldDef("smallmoney", QVariant::Double,        100.42),
        FieldDef("money", QVariant::Double,             200.42),
        // accuracy is that of a minute
        FieldDef("smalldatetime", QVariant::DateTime,   QDateTime(QDate::currentDate(), QTime(1, 2, 0, 0))),
        // accuracy is that of a second
        FieldDef("datetime", QVariant::DateTime,        QDateTime(QDate::currentDate(), QTime(1, 2, 3, 0))),
        FieldDef("char(20)", QVariant::String,          "blah1"),
        FieldDef("varchar(20)", QVariant::String,       "blah2"),
        FieldDef("nchar(20)", QVariant::String,         "blah3"),
        FieldDef("nvarchar(20)", QVariant::String,      "blah4"),
        FieldDef("text", QVariant::String,              "blah5"),
        FieldDef("bit", QVariant::Int,                  1, false),

        FieldDef()
    };

    const int fieldCount = createFieldTable(fieldDefs, db);
    QVERIFY(fieldCount > 0);

    commonFieldTest(fieldDefs, db, fieldCount);
}

void tst_QSqlDatabase::recordOCI()
{
    bool hasTimeStamp = false;

    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    // runtime check for Oracle version since V8 doesn't support TIMESTAMPs
    if (tst_Databases::getOraVersion(db) >= 9)
        hasTimeStamp = true;

    FieldDef tsdef;
    FieldDef tstzdef;
    FieldDef tsltzdef;
    FieldDef intytm;
    FieldDef intdts;

    static const QDateTime dt(QDate::currentDate(), QTime(1, 2, 3, 0));

    if (hasTimeStamp) {
        tsdef = FieldDef("timestamp", QVariant::DateTime,  dt);
        tstzdef = FieldDef("timestamp with time zone", QVariant::DateTime, dt);
        tsltzdef = FieldDef("timestamp with local time zone", QVariant::DateTime, dt);
        intytm = FieldDef("interval year to month", QVariant::String, QString("+01-01"));
        intdts = FieldDef("interval day to second", QVariant::String, QString("+01 00:00:01.000000"));
    }

    const FieldDef fieldDefs[] = {
        FieldDef("char(20)", QVariant::String,          QString("blah1")),
        FieldDef("varchar(20)", QVariant::String,       QString("blah2")),
        FieldDef("nchar(20)", QVariant::String,         QString("blah3")),
        FieldDef("nvarchar2(20)", QVariant::String,     QString("blah4")),
        FieldDef("number(10,5)", QVariant::Double,      1.1234567),
        FieldDef("date", QVariant::DateTime,            dt),
        FieldDef("long raw", QVariant::ByteArray,       QByteArray("blah5")),
        FieldDef("raw(2000)", QVariant::ByteArray,      QByteArray("blah6"), false),
        FieldDef("blob", QVariant::ByteArray,           QByteArray("blah7")),
        FieldDef("clob", QVariant::String,             QString("blah8")),
        FieldDef("nclob", QVariant::String,            QString("blah9")),
//        FieldDef("bfile", QVariant::ByteArray,         QByteArray("blah10")),

        intytm,
        intdts,
        tsdef,
        tstzdef,
        tsltzdef,
        FieldDef()
    };

    const int fieldCount = createFieldTable(fieldDefs, db);
    QVERIFY(fieldCount > 0);

    commonFieldTest(fieldDefs, db, fieldCount);

    // some additional tests
    const QString tableName = qTableName("qtestfields", __FILE__, db);
    QSqlRecord rec = db.record(tableName);
    QCOMPARE(rec.field("T_NUMBER").length(), 10);
    QCOMPARE(rec.field("T_NUMBER").precision(), 5);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec("SELECT * FROM " + tableName));
    rec = q.record();
    QCOMPARE(rec.field("T_NUMBER").length(), 10);
    QCOMPARE(rec.field("T_NUMBER").precision(), 5);
}

void tst_QSqlDatabase::recordPSQL()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    FieldDef byteadef;
    if (db.driver()->hasFeature(QSqlDriver::BLOB))
        byteadef = FieldDef("bytea", QVariant::ByteArray, QByteArray("bl\\ah"));
    static FieldDef fieldDefs[] = {
        FieldDef("bigint", QVariant::LongLong,       Q_INT64_C(9223372036854775807)),
        FieldDef("bigserial", QVariant::LongLong,    100, false),
        FieldDef("bit", QVariant::String,            "1"), // a bit in postgres is a bit-string
        FieldDef("box", QVariant::String,            "(5,6),(1,2)"),
        FieldDef("char(20)", QVariant::String,       "blah5678901234567890"),
        FieldDef("varchar(20)", QVariant::String,    "blah5678901234567890"),
        FieldDef("cidr", QVariant::String,           "12.123.0.0/24"),
        FieldDef("circle", QVariant::String,         "<(1,2),3>"),
        FieldDef("date", QVariant::Date,             QDate::currentDate()),
        FieldDef("float8", QVariant::Double,         1.12345678912),
        FieldDef("inet", QVariant::String,           "12.123.12.23"),
        FieldDef("integer", QVariant::Int,           2147483647),
        FieldDef("interval", QVariant::String,       "1 day 12:59:10"),
//      LOL... you can create a "line" datatype in PostgreSQL <= 7.2.x but
//      as soon as you want to insert data you get a "not implemented yet" error
//      FieldDef("line", QVariant::Polygon, QPolygon(QRect(1, 2, 3, 4))),
        FieldDef("lseg", QVariant::String,           "[(1,1),(2,2)]"),
        FieldDef("macaddr", QVariant::String,        "08:00:2b:01:02:03"),
        FieldDef("money", QVariant::String,          "$12.23"),
        FieldDef("numeric", QVariant::Double,        1.2345678912),
        FieldDef("path", QVariant::String,           "((1,2),(3,2),(3,5),(1,5))"),
        FieldDef("point", QVariant::String,          "(1,2)"),
        FieldDef("polygon", QVariant::String,        "((1,2),(3,2),(3,5),(1,5))"),
        FieldDef("real", QVariant::Double,           1.1234),
        FieldDef("smallint", QVariant::Int,          32767),
        FieldDef("serial", QVariant::Int,            100, false),
        FieldDef("text", QVariant::String,           "blah"),
        FieldDef("time(6)", QVariant::Time,          QTime(1, 2, 3)),
        FieldDef("timetz", QVariant::Time,           QTime(1, 2, 3)),
        FieldDef("timestamp(6)", QVariant::DateTime, QDateTime::currentDateTime()),
        FieldDef("timestamptz", QVariant::DateTime,  QDateTime::currentDateTime()),
        byteadef,

        FieldDef()
    };

    QSqlQuery q(db);

    QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);
    if (dbType == QSqlDriverPrivate::PostgreSQL)
        QVERIFY_SQL( q, exec("set client_min_messages='warning'"));
    const QString tableName = qTableName("qtestfields", __FILE__, db);
    q.exec("drop sequence " + tableName + "_t_bigserial_seq");
    q.exec("drop sequence " + tableName + "_t_serial_seq");
    // older psql cut off the table name
    q.exec("drop sequence " + tableName + "_t_bigserial_seq");
    q.exec("drop sequence " + tableName + "_t_serial_seq");

    const int fieldCount = createFieldTable(fieldDefs, db);
    QVERIFY(fieldCount > 0);

    commonFieldTest(fieldDefs, db, fieldCount);
    for (int i = 0; i < ITERATION_COUNT; ++i) {
        // increase serial values
        for (int i2 = 0; !fieldDefs[ i2 ].typeName.isNull(); ++i2) {
            if (fieldDefs[ i2 ].typeName == "serial" ||
                fieldDefs[ i2 ].typeName == "bigserial") {

                FieldDef def = fieldDefs[ i2 ];
                def.val = def.val.toInt() + 1;
                fieldDefs[ i2 ] = def;
            }
        }
    }
}

void tst_QSqlDatabase::recordMySQL()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    FieldDef bin10, varbin10;
    int major = tst_Databases::getMySqlVersion( db ).section( QChar('.'), 0, 0 ).toInt();
    int minor = tst_Databases::getMySqlVersion( db ).section( QChar('.'), 1, 1 ).toInt();
    int revision = tst_Databases::getMySqlVersion( db ).section( QChar('.'), 2, 2 ).toInt();
    int vernum = (major << 16) + (minor << 8) + revision;

    /* The below is broken in mysql below 5.0.15
        see http://dev.mysql.com/doc/refman/5.0/en/binary-varbinary.html
        specifically: Before MySQL 5.0.15, the pad value is space. Values are right-padded
        with space on insert, and trailing spaces are removed on select.
    */
    if( vernum >= ((5 << 16) + 15) ) {
        bin10 = FieldDef("binary(10)", QVariant::ByteArray, QString("123abc    "));
        varbin10 = FieldDef("varbinary(10)", QVariant::ByteArray, QString("123abcv   "));
    }

    static QDateTime dt(QDate::currentDate(), QTime(1, 2, 3, 0));
    static const FieldDef fieldDefs[] = {
        FieldDef("tinyint", QVariant::Int,               127),
        FieldDef("tinyint unsigned", QVariant::UInt,     255),
        FieldDef("smallint", QVariant::Int,              32767),
        FieldDef("smallint unsigned", QVariant::UInt,    65535),
        FieldDef("mediumint", QVariant::Int,             8388607),
        FieldDef("mediumint unsigned", QVariant::UInt,   16777215),
        FieldDef("integer", QVariant::Int,               2147483647),
        FieldDef("integer unsigned", QVariant::UInt,     4294967295u),
        FieldDef("bigint", QVariant::LongLong,           Q_INT64_C(9223372036854775807)),
        FieldDef("bigint unsigned", QVariant::ULongLong, Q_UINT64_C(18446744073709551615)),
        FieldDef("float", QVariant::Double,              1.12345),
        FieldDef("double", QVariant::Double,             1.123456789),
        FieldDef("decimal(10, 9)", QVariant::Double,     1.123456789),
        FieldDef("numeric(5, 2)", QVariant::Double,      123.67),
        FieldDef("date", QVariant::Date,                 QDate::currentDate()),
        FieldDef("datetime", QVariant::DateTime,         dt),
        FieldDef("timestamp", QVariant::DateTime,        dt, false),
        FieldDef("time", QVariant::Time,                 dt.time()),
        FieldDef("year", QVariant::Int,                  2003),
        FieldDef("char(20)", QVariant::String,           "Blah"),
        FieldDef("varchar(20)", QVariant::String,        "BlahBlah"),
        FieldDef("tinytext", QVariant::String,           QString("blah5")),
        FieldDef("text", QVariant::String,               QString("blah6")),
        FieldDef("mediumtext", QVariant::String,         QString("blah7")),
        FieldDef("longtext", QVariant::String,           QString("blah8")),
        // SET OF?

        FieldDef()
    };

    const int fieldCount = createFieldTable(fieldDefs, db);
    QVERIFY(fieldCount > 0);

    commonFieldTest(fieldDefs, db, fieldCount);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec("SELECT DATE_SUB(CURDATE(), INTERVAL 2 DAY)"));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toDateTime().date(), QDate::currentDate().addDays(-2));
}

void tst_QSqlDatabase::recordDB2()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    static const FieldDef fieldDefs[] = {
        FieldDef("char(20)", QVariant::String,         QString("Blah1")),
        FieldDef("varchar(20)", QVariant::String,      QString("Blah2")),
        FieldDef("long varchar", QVariant::String,     QString("Blah3")),
        // using BOOLEAN results in "SQL0486N  The BOOLEAN data type is currently only supported internally."
//X     FieldDef("boolean" , QVariant::Bool,           QVariant(true, 1)),
        FieldDef("smallint", QVariant::Int,            32767),
        FieldDef("integer", QVariant::Int,             2147483647),
        FieldDef("bigint", QVariant::LongLong,         Q_INT64_C(9223372036854775807)),
        FieldDef("real", QVariant::Double,             1.12345),
        FieldDef("double", QVariant::Double,           1.23456789),
        FieldDef("float", QVariant::Double,            1.23456789),
        FieldDef("decimal(10,9)", QVariant::Double,    1.234567891),
        FieldDef("numeric(10,9)", QVariant::Double,    1.234567891),
        FieldDef("date", QVariant::Date,               QDate::currentDate()),
        FieldDef("time", QVariant::Time,               QTime(1, 2, 3)),
        FieldDef("timestamp", QVariant::DateTime,      QDateTime::currentDateTime()),
//         FieldDef("graphic(20)", QVariant::String,       QString("Blah4")),
//         FieldDef("vargraphic(20)", QVariant::String,    QString("Blah5")),
//         FieldDef("long vargraphic", QVariant::String,   QString("Blah6")),
        //X FieldDef("datalink", QVariant::String,          QString("DLVALUE('Blah10')")),
        FieldDef()
    };

    const int fieldCount = createFieldTable(fieldDefs, db);
    QVERIFY(fieldCount > 0);

    commonFieldTest(fieldDefs, db, fieldCount);
}

void tst_QSqlDatabase::recordIBase()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    static const FieldDef fieldDefs[] = {
        FieldDef("char(20)", QVariant::String, QString("Blah1"), false),
        FieldDef("varchar(20)", QVariant::String, QString("Blah2")),
        FieldDef("smallint", QVariant::Int, 32767),
        FieldDef("float", QVariant::Double, 1.2345),
        FieldDef("double precision", QVariant::Double, 1.2345678),
        FieldDef("timestamp", QVariant::DateTime, QDateTime::currentDateTime()),
        FieldDef("time", QVariant::Time, QTime::currentTime()),
        FieldDef("decimal(18)", QVariant::LongLong, Q_INT64_C(9223372036854775807)),
        FieldDef("numeric(5,2)", QVariant::Double, 123.45),

        FieldDef()
    };

    const int fieldCount = createFieldTable(fieldDefs, db);
    QVERIFY(fieldCount > 0);

    commonFieldTest(fieldDefs, db, fieldCount);
}

void tst_QSqlDatabase::recordSQLite()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    static const FieldDef fieldDefs[] = {
        // The affinity of these fields are TEXT so SQLite should give us strings, not ints or doubles.
        FieldDef("char(20)", QVariant::String,          QString("123")),
        FieldDef("varchar(20)", QVariant::String,       QString("123.4")),
        FieldDef("clob", QVariant::String,              QString("123.45")),
        FieldDef("text", QVariant::String,              QString("123.456")),

        FieldDef("integer", QVariant::Int,              QVariant(13)),
        FieldDef("int", QVariant::Int,                  QVariant(12)),
        FieldDef("real", QVariant::Double,              QVariant(1.234567890123456)),

        FieldDef()
    };

    const int fieldCount = createFieldTable(fieldDefs, db);
    QVERIFY(fieldCount > 0);

    commonFieldTest(fieldDefs, db, fieldCount);
}

void tst_QSqlDatabase::recordSQLServer()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);
    if (dbType != QSqlDriverPrivate::MSSqlServer)
        QSKIP("SQL server specific test");

    // ### TODO: Add the rest of the fields
    static const FieldDef fieldDefs[] = {
        FieldDef("varchar(20)", QVariant::String, QString("Blah1")),
        FieldDef("bigint", QVariant::LongLong, 12345),
        FieldDef("int", QVariant::Int, 123456),
        FieldDef("tinyint", QVariant::UInt, 255),
        FieldDef("float", QVariant::Double, 1.12345),
        FieldDef("numeric(5,2)", QVariant::Double, 123.45),
        FieldDef("uniqueidentifier", QVariant::String,
            QString("AA7DF450-F119-11CD-8465-00AA00425D90")),

        FieldDef()
    };

    const int fieldCount = createFieldTable(fieldDefs, db);
    QVERIFY(fieldCount > 0);

    commonFieldTest(fieldDefs, db, fieldCount);
}

void tst_QSqlDatabase::recordAccess()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (!tst_Databases::isMSAccess(db))
        QSKIP("MS Access specific test");

    QString memo;
    for (int i = 0; i < 32; i++)
        memo.append("ABCDEFGH12345678abcdefgh12345678");

    // ### TODO: Add the rest of the fields
    static const FieldDef fieldDefs[] = {
    FieldDef("varchar(20)", QVariant::String, QString("Blah1")),
    FieldDef("single", QVariant::Double, 1.12345),
    FieldDef("double", QVariant::Double, 1.123456),
    FieldDef("byte", QVariant::UInt, 255),
    FieldDef("long", QVariant::Int, 2147483647),
        FieldDef("memo", QVariant::String, memo),
    FieldDef()
    };

    const int fieldCount = createFieldTable(fieldDefs, db);
    QVERIFY(fieldCount > 0);

    commonFieldTest(fieldDefs, db, fieldCount);
}

void tst_QSqlDatabase::transaction()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);
    const QString qtest(qTableName("qtest", __FILE__, db));

    if (!db.driver()->hasFeature(QSqlDriver::Transactions))
        QSKIP("DBMS not transaction capable");

    QVERIFY(db.transaction());

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec("insert into " + qtest + " values (40, 'VarChar40', 'Char40', 40.40)"));
    QVERIFY_SQL(q, exec("select * from " + qtest + " where id = 40"));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 40);
    q.clear();

    QVERIFY(db.commit());

    QVERIFY(db.transaction());
    QVERIFY_SQL(q, exec("select * from " + qtest + " where id = 40"));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 40);
    q.clear();
    QVERIFY(db.commit());

    QVERIFY(db.transaction());
    QVERIFY_SQL(q, exec("insert into " + qtest + " values (41, 'VarChar41', 'Char41', 41.41)"));
    QVERIFY_SQL(q, exec("select * from " + qtest + " where id = 41"));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 41);
    q.clear(); // for SQLite which does not allow any references on rows that shall be rolled back
    if (!db.rollback()) {
        if (dbType == QSqlDriverPrivate::MySqlServer)
            QSKIP("MySQL transaction failed: " + tst_Databases::printError(db.lastError()));
        else
            QFAIL("Could not rollback transaction: " + tst_Databases::printError(db.lastError()));
    }

    QVERIFY_SQL(q, exec("select * from " + qtest + " where id = 41"));
    QVERIFY(!q.next());

    populateTestTables(db);
}

void tst_QSqlDatabase::bigIntField()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);
    const QString qtest_bigint(qTableName("qtest_bigint", __FILE__, db));

    QSqlQuery q(db);
    q.setForwardOnly(true);

    if (dbType == QSqlDriverPrivate::Oracle)
        q.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);

    if (dbType == QSqlDriverPrivate::MySqlServer) {
        QVERIFY_SQL(q, exec("create table " + qtest_bigint + " (id int, t_s64bit bigint, t_u64bit bigint unsigned)"));
    } else if (dbType == QSqlDriverPrivate::PostgreSQL
                || dbType == QSqlDriverPrivate::DB2
                || dbType == QSqlDriverPrivate::MSSqlServer) {
        QVERIFY_SQL(q, exec("create table " + qtest_bigint + "(id int, t_s64bit bigint, t_u64bit bigint)"));
    } else if (dbType == QSqlDriverPrivate::Oracle) {
        QVERIFY_SQL(q, exec("create table " + qtest_bigint + " (id int, t_s64bit int, t_u64bit int)"));
    //} else if (dbType == QSqlDriverPrivate::Interbase) {
    //    QVERIFY_SQL(q, exec("create table " + qtest_bigint + " (id int, t_s64bit int64, t_u64bit int64)"));
    } else {
        QSKIP("no 64 bit integer support");
    }
    QVERIFY(q.prepare("insert into " + qtest_bigint + " values (?, ?, ?)"));
    qlonglong ll = Q_INT64_C(9223372036854775807);
    qulonglong ull = Q_UINT64_C(18446744073709551615);

    if (dbType == QSqlDriverPrivate::MySqlServer || dbType == QSqlDriverPrivate::Oracle) {
        q.bindValue(0, 0);
        q.bindValue(1, ll);
        q.bindValue(2, ull);
        QVERIFY_SQL(q, exec());
        q.bindValue(0, 1);
        q.bindValue(1, -ll);
        q.bindValue(2, ull);
        QVERIFY_SQL(q, exec());
    } else {
        // usinged bigint fields not supported - a cast is necessary
        q.bindValue(0, 0);
        q.bindValue(1, ll);
        q.bindValue(2, (qlonglong) ull);
        QVERIFY_SQL(q, exec());
        q.bindValue(0, 1);
        q.bindValue(1, -ll);
        q.bindValue(2,  (qlonglong) ull);
        QVERIFY_SQL(q, exec());
    }
    QVERIFY(q.exec("select * from " + qtest_bigint + " order by id"));
    QVERIFY(q.next());
    QCOMPARE(q.value(1).toDouble(), (double)ll);
    QCOMPARE(q.value(1).toLongLong(), ll);
    if (dbType == QSqlDriverPrivate::Oracle)
        QEXPECT_FAIL("", "Oracle driver lacks support for unsigned int64 types", Continue);
    QCOMPARE(q.value(2).toULongLong(), ull);
    QVERIFY(q.next());
    QCOMPARE(q.value(1).toLongLong(), -ll);
    if (dbType == QSqlDriverPrivate::Oracle)
        QEXPECT_FAIL("", "Oracle driver lacks support for unsigned int64 types", Continue);
    QCOMPARE(q.value(2).toULongLong(), ull);
}

void tst_QSqlDatabase::caseSensivity()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);

    bool cs = false;
    if (dbType == QSqlDriverPrivate::MySqlServer || dbType == QSqlDriverPrivate::SQLite || dbType == QSqlDriverPrivate::Sybase
        || dbType == QSqlDriverPrivate::MSSqlServer || db.driverName().startsWith("QODBC"))
    cs = true;

    QSqlRecord rec = db.record(qTableName("qtest", __FILE__, db));
    QVERIFY((int)rec.count() > 0);
    if (!cs) {
    rec = db.record(qTableName("QTEST", __FILE__, db).toUpper());
    QVERIFY((int)rec.count() > 0);
    rec = db.record(qTableName("qTesT", __FILE__, db));
    QVERIFY((int)rec.count() > 0);
    }

    rec = db.primaryIndex(qTableName("qtest", __FILE__, db));
    QVERIFY((int)rec.count() > 0);
    if (!cs) {
    rec = db.primaryIndex(qTableName("QTEST", __FILE__, db).toUpper());
    QVERIFY((int)rec.count() > 0);
    rec = db.primaryIndex(qTableName("qTesT", __FILE__, db));
    QVERIFY((int)rec.count() > 0);
    }
}

void tst_QSqlDatabase::noEscapedFieldNamesInRecord()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QString fieldname("t_varchar");
    QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);
    if (dbType == QSqlDriverPrivate::Oracle || dbType == QSqlDriverPrivate::Interbase || dbType == QSqlDriverPrivate::DB2)
        fieldname = fieldname.toUpper();

    QSqlQuery q(db);
    QString query = "SELECT " + db.driver()->escapeIdentifier(fieldname, QSqlDriver::FieldName) + " FROM " + qTableName("qtest", __FILE__, db);
    QVERIFY_SQL(q, exec(query));
    QCOMPARE(q.record().fieldName(0), fieldname);
}

void tst_QSqlDatabase::psql_schemas()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (!db.tables(QSql::SystemTables).contains("pg_namespace"))
        QSKIP("server does not support schemas");

    QSqlQuery q(db);
    QVERIFY_SQL( q, exec("set client_min_messages='warning'"));

    const QString schemaName = qTableName("qtestschema", __FILE__, db);
    QVERIFY_SQL(q, exec("CREATE SCHEMA " + schemaName));

    QString table = schemaName + '.' + qTableName("qtesttable", __FILE__, db);
    QVERIFY_SQL(q, exec("CREATE TABLE " + table + " (id int primary key, name varchar(20))"));

    QVERIFY(db.tables().contains(table));

    QSqlRecord rec = db.record(table);
    QCOMPARE(rec.count(), 2);
    QCOMPARE(rec.fieldName(0), QString("id"));
    QCOMPARE(rec.fieldName(1), QString("name"));

    QSqlIndex idx = db.primaryIndex(table);
    QCOMPARE(idx.count(), 1);
    QCOMPARE(idx.fieldName(0), QString("id"));
}

void tst_QSqlDatabase::psql_escapedIdentifiers()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    QSqlDriver* drv = db.driver();
    CHECK_DATABASE(db);

    if (!db.tables(QSql::SystemTables).contains("pg_namespace"))
        QSKIP("server does not support schemas");

    QSqlQuery q(db);
    QVERIFY_SQL( q, exec("set client_min_messages='warning'"));

    const QString schemaName(qTableName("qtestScHeMa", __FILE__, db)),
                  tableName(qTableName("qtest", __FILE__, db)),
                  field1Name(QLatin1String("fIeLdNaMe")),
                  field2Name(QLatin1String("ZuLu"));

    q.exec(QString("DROP SCHEMA \"%1\" CASCADE").arg(schemaName));
    QString createSchema = QString("CREATE SCHEMA \"%1\"").arg(schemaName);
    QVERIFY_SQL(q, exec(createSchema));
    QString createTable = QString("CREATE TABLE \"%1\".\"%2\" (\"%3\" int PRIMARY KEY, \"%4\" varchar(20))").arg(schemaName).arg(tableName).arg(field1Name).arg(field2Name);
    QVERIFY_SQL(q, exec(createTable));

    QVERIFY(db.tables().contains(schemaName + '.' + tableName, Qt::CaseSensitive));

    QSqlField fld1(field1Name, QVariant::Int);
    QSqlField fld2(field2Name, QVariant::String);
    QSqlRecord rec;
    rec.append(fld1);
    rec.append(fld2);

    QVERIFY_SQL(q, exec(drv->sqlStatement(QSqlDriver::SelectStatement, db.driver()->escapeIdentifier(schemaName, QSqlDriver::TableName) + '.' + db.driver()->escapeIdentifier(tableName, QSqlDriver::TableName), rec, false)));

    rec = q.record();
    QCOMPARE(rec.count(), 2);
    QCOMPARE(rec.fieldName(0), field1Name);
    QCOMPARE(rec.fieldName(1), field2Name);
    QCOMPARE(rec.field(0).type(), QVariant::Int);

    q.exec(QString("DROP SCHEMA \"%1\" CASCADE").arg(schemaName));
}

void tst_QSqlDatabase::psql_escapeBytea()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    const char dta[4] = {'\x71', '\x14', '\x32', '\x81'};
    QByteArray ba(dta, 4);

    QSqlQuery q(db);
    const QString tableName(qTableName("batable", __FILE__, db));
    QVERIFY_SQL(q, exec(QString("CREATE TABLE %1 (ba bytea)").arg(tableName)));

    QSqlQuery iq(db);
    QVERIFY_SQL(iq, prepare(QString("INSERT INTO %1 VALUES (?)").arg(tableName)));
    iq.bindValue(0, QVariant(ba));
    QVERIFY_SQL(iq, exec());

    QVERIFY_SQL(q, exec(QString("SELECT ba FROM %1").arg(tableName)));
    QVERIFY_SQL(q, next());

    QByteArray res = q.value(0).toByteArray();
    int i = 0;
    for (; i < ba.size(); ++i){
        if (ba[i] != res[i])
            break;
    }

    QCOMPARE(i, 4);
}

void tst_QSqlDatabase::psql_bug249059()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QString version=tst_Databases::getPSQLVersion( db );
    double ver=version.section(QChar::fromLatin1('.'),0,1).toDouble();
    if (ver < 7.3)
        QSKIP("Test requires PostgreSQL >= 7.3");

    QSqlQuery q(db);
    const QString tableName(qTableName("bug_249059", __FILE__, db));
    QVERIFY_SQL(q, exec(QString("CREATE TABLE %1 (dt timestamp, t time)").arg(tableName)));

    QSqlQuery iq(db);
    QVERIFY_SQL(iq, prepare(QString("INSERT INTO %1 VALUES (?, ?)").arg(tableName)));
    iq.bindValue(0, QVariant(QString("2001-09-09 04:05:06.789 -5:00")));
    iq.bindValue(1, QVariant(QString("04:05:06.789 -5:00")));
    QVERIFY_SQL(iq, exec());
    iq.bindValue(0, QVariant(QString("2001-09-09 04:05:06.789 +5:00")));
    iq.bindValue(1, QVariant(QString("04:05:06.789 +5:00")));
    QVERIFY_SQL(iq, exec());

    QVERIFY_SQL(q, exec(QString("SELECT dt, t FROM %1").arg(tableName)));
    QVERIFY_SQL(q, next());
    QDateTime dt1=q.value(0).toDateTime();
    QTime t1=q.value(1).toTime();
    QVERIFY_SQL(q, next());
    QDateTime dt2=q.value(0).toDateTime();
    QTime t2=q.value(1).toTime();

    // These will fail when timezone support is added, when that's the case, set the second record to 14:05:06.789 and it should work correctly
    QCOMPARE(dt1, dt2);
    QCOMPARE(t1, t2);
}

// This test should be rewritten to work with Oracle as well - or the Oracle driver
// should be fixed to make this test pass (handle overflows)
void tst_QSqlDatabase::precisionPolicy()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
//     DBMS_SPECIFIC(db, "QPSQL");

    QSqlQuery q(db);
    const QString tableName(qTableName("qtest_prec", __FILE__, db));
    if(!db.driver()->hasFeature(QSqlDriver::LowPrecisionNumbers))
        QSKIP("Driver or database doesn't support setting precision policy");

    // Create a test table with some data
    if(tst_Databases::isMSAccess(db))
        QVERIFY_SQL(q, exec(QString("CREATE TABLE %1 (id smallint, num number)").arg(tableName)));
    else
        QVERIFY_SQL(q, exec(QString("CREATE TABLE %1 (id smallint, num numeric(18,5))").arg(tableName)));
    QVERIFY_SQL(q, prepare(QString("INSERT INTO %1 VALUES (?, ?)").arg(tableName)));
    q.bindValue(0, 1);
    q.bindValue(1, 123);
    QVERIFY_SQL(q, exec());
    q.bindValue(0, 2);
    q.bindValue(1, 1850000000000.0001);
    QVERIFY_SQL(q, exec());

    // These are expected to pass
    q.setNumericalPrecisionPolicy(QSql::HighPrecision);
    QString query = QString("SELECT num FROM %1 WHERE id = 1").arg(tableName);
    QVERIFY_SQL(q, exec(query));
    QVERIFY_SQL(q, next());
    QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);
    if (dbType == QSqlDriverPrivate::SQLite)
        QEXPECT_FAIL("", "SQLite returns this value as determined by contents of the field, not the declaration", Continue);
    QCOMPARE(q.value(0).type(), QVariant::String);

    q.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);
    QVERIFY_SQL(q, exec(query));
    QVERIFY_SQL(q, next());
    if(q.value(0).type() != QVariant::LongLong)
        QEXPECT_FAIL("", "SQLite returns this value as determined by contents of the field, not the declaration", Continue);
    QCOMPARE(q.value(0).type(), QVariant::LongLong);
    QCOMPARE(q.value(0).toLongLong(), (qlonglong)123);

    q.setNumericalPrecisionPolicy(QSql::LowPrecisionInt32);
    QVERIFY_SQL(q, exec(query));
    QVERIFY_SQL(q, next());
    if (dbType == QSqlDriverPrivate::SQLite)
        QEXPECT_FAIL("", "SQLite returns this value as determined by contents of the field, not the declaration", Continue);
    QCOMPARE(q.value(0).type(), QVariant::Int);
    QCOMPARE(q.value(0).toInt(), 123);

    q.setNumericalPrecisionPolicy(QSql::LowPrecisionDouble);
    QVERIFY_SQL(q, exec(query));
    QVERIFY_SQL(q, next());
    if (dbType == QSqlDriverPrivate::SQLite)
        QEXPECT_FAIL("", "SQLite returns this value as determined by contents of the field, not the declaration", Continue);
    QCOMPARE(q.value(0).type(), QVariant::Double);
    QCOMPARE(q.value(0).toDouble(), (double)123);

    query = QString("SELECT num FROM %1 WHERE id = 2").arg(tableName);
    QVERIFY_SQL(q, exec(query));
    QVERIFY_SQL(q, next());
    if (dbType == QSqlDriverPrivate::SQLite)
        QEXPECT_FAIL("", "SQLite returns this value as determined by contents of the field, not the declaration", Continue);
    QCOMPARE(q.value(0).type(), QVariant::Double);
    QCOMPARE(q.value(0).toDouble(), QString("1850000000000.0001").toDouble());

    // Postgres returns invalid QVariants on overflow
    q.setNumericalPrecisionPolicy(QSql::HighPrecision);
    QVERIFY_SQL(q, exec(query));
    QVERIFY_SQL(q, next());
    if (dbType == QSqlDriverPrivate::SQLite)
        QEXPECT_FAIL("", "SQLite returns this value as determined by contents of the field, not the declaration", Continue);
    QCOMPARE(q.value(0).type(), QVariant::String);

    q.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);
    QEXPECT_FAIL("QOCI", "Oracle fails here, to retrieve next", Continue);
    QVERIFY_SQL(q, exec(query));
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).type(), QVariant::LongLong);

    QSql::NumericalPrecisionPolicy oldPrecision= db.numericalPrecisionPolicy();
    db.setNumericalPrecisionPolicy(QSql::LowPrecisionInt64);
    QSqlQuery q2(db);
    q2.exec(QString("SELECT num FROM %1 WHERE id = 2").arg(tableName));
    QVERIFY_SQL(q2, exec(query));
    QVERIFY_SQL(q2, next());
    QCOMPARE(q2.value(0).type(), QVariant::LongLong);
    db.setNumericalPrecisionPolicy(oldPrecision);
}

// This test needs a ODBC data source containing MYSQL in it's name
void tst_QSqlDatabase::mysqlOdbc_unsignedIntegers()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (tst_Databases::getDatabaseType(db) != QSqlDriverPrivate::MySqlServer || !db.driverName().startsWith("QODBC"))
       QSKIP("MySQL through ODBC-driver specific test");

    QSqlQuery q(db);
    const QString tableName(qTableName("uint", __FILE__, db));
    QVERIFY_SQL(q, exec(QString("CREATE TABLE %1 (foo integer(10) unsigned, bar integer(10))").arg(tableName)));
    QVERIFY_SQL(q, exec(QString("INSERT INTO %1 VALUES (-4000000000, -4000000000)").arg(tableName)));
    QVERIFY_SQL(q, exec(QString("INSERT INTO %1 VALUES (4000000000, 4000000000)").arg(tableName)));

    QVERIFY_SQL(q, exec(QString("SELECT foo, bar FROM %1").arg(tableName)));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toString(), QString("0"));
    QCOMPARE(q.value(1).toString(), QString("-2147483648"));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toString(), QString("4000000000"));
    QCOMPARE(q.value(1).toString(), QString("2147483647"));
}

void tst_QSqlDatabase::accessOdbc_strings()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (!tst_Databases::isMSAccess(db))
        QSKIP("MS Access specific test");

    QSqlQuery q(db);
    const QString tableName(qTableName("strings", __FILE__, db));
    QVERIFY_SQL(q, exec(QString("CREATE TABLE %1 (aStr memo, bStr memo, cStr memo, dStr memo"
            ", eStr memo, fStr memo, gStr memo, hStr memo)").arg(tableName)));

    QVERIFY_SQL(q, prepare(QString("INSERT INTO %1 VALUES (?, ?, ?, ?, ?, ?, ?, ?)").arg(tableName)));
    QString aStr, bStr, cStr, dStr, eStr, fStr, gStr, hStr;

    q.bindValue(0, aStr.fill('A', 32));
    q.bindValue(1, bStr.fill('B', 127));
    q.bindValue(2, cStr.fill('C', 128));
    q.bindValue(3, dStr.fill('D', 129));
    q.bindValue(4, eStr.fill('E', 254));
    q.bindValue(5, fStr.fill('F', 255));
    q.bindValue(6, gStr.fill('G', 256));
    q.bindValue(7, hStr.fill('H', 512));

    QVERIFY_SQL(q, exec());

    QVERIFY_SQL(q, exec(QString("SELECT aStr, bStr, cStr, dStr, eStr, fStr, gStr, hStr FROM %1").arg(tableName)));
    q.next();
    QCOMPARE(q.value(0).toString(), aStr);
    QCOMPARE(q.value(1).toString(), bStr);
    QCOMPARE(q.value(2).toString(), cStr);
    QCOMPARE(q.value(3).toString(), dStr);
    QCOMPARE(q.value(4).toString(), eStr);
    QCOMPARE(q.value(5).toString(), fStr);
    QCOMPARE(q.value(6).toString(), gStr);
    QCOMPARE(q.value(7).toString(), hStr);
}

// For task 125053
void tst_QSqlDatabase::ibase_numericFields()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    const QString tableName(qTableName("numericfields", __FILE__, db));
    QVERIFY_SQL(q, exec(QString("CREATE TABLE %1 (id int not null, num1 NUMERIC(2,1), "
        "num2 NUMERIC(5,2), num3 NUMERIC(10,3), "
        "num4 NUMERIC(18,4))").arg(tableName)));

    QVERIFY_SQL(q, exec(QString("INSERT INTO %1 VALUES (1, 1.1, 123.45, 1234567.123, 10203040506070.8090)").arg(tableName)));

    QVERIFY_SQL(q, prepare(QString("INSERT INTO %1 VALUES (?, ?, ?, ?, ?)").arg(tableName)));

    double num1 = 1.1;
    double num2 = 123.45;
    double num3 = 1234567.123;
    double num4 = 10203040506070.8090;

    q.bindValue(0, 2);
    q.bindValue(1, QVariant(num1));
    q.bindValue(2, QVariant(num2));
    q.bindValue(3, QVariant(num3));
    q.bindValue(4, QVariant(num4));
    QVERIFY_SQL(q, exec());

    QVERIFY_SQL(q, exec(QString("SELECT id, num1, num2, num3, num4 FROM %1").arg(tableName)));

    int id = 0;
    while (q.next()) {
        QCOMPARE(q.value(0).toInt(), ++id);
        QCOMPARE(q.value(1).toString(), QString("%1").arg(num1));
        QCOMPARE(q.value(2).toString(), QString("%1").arg(num2));
        QCOMPARE(QString("%1").arg(q.value(3).toDouble()), QString("%1").arg(num3));
        QCOMPARE(QString("%1").arg(q.value(4).toDouble()), QString("%1").arg(num4));
        QVERIFY(q.value(0).type() == QVariant::Int);
        QVERIFY(q.value(1).type() == QVariant::Double);
        QVERIFY(q.value(2).type() == QVariant::Double);
        QVERIFY(q.value(3).type() == QVariant::Double);
        QVERIFY(q.value(4).type() == QVariant::Double);

        QCOMPARE(q.record().field(1).length(), 2);
        QCOMPARE(q.record().field(1).precision(), 1);
        QCOMPARE(q.record().field(2).length(), 5);
        QCOMPARE(q.record().field(2).precision(), 2);
        QCOMPARE(q.record().field(3).length(), 10);
        QCOMPARE(q.record().field(3).precision(), 3);
        QCOMPARE(q.record().field(4).length(), 18);
        QCOMPARE(q.record().field(4).precision(), 4);
        QVERIFY(q.record().field(0).requiredStatus() == QSqlField::Required);
        QVERIFY(q.record().field(1).requiredStatus() == QSqlField::Optional);
    }

    QSqlRecord r = db.record(tableName);
    QVERIFY(r.field(0).type() == QVariant::Int);
    QVERIFY(r.field(1).type() == QVariant::Double);
    QVERIFY(r.field(2).type() == QVariant::Double);
    QVERIFY(r.field(3).type() == QVariant::Double);
    QVERIFY(r.field(4).type() == QVariant::Double);
    QCOMPARE(r.field(1).length(), 2);
    QCOMPARE(r.field(1).precision(), 1);
    QCOMPARE(r.field(2).length(), 5);
    QCOMPARE(r.field(2).precision(), 2);
    QCOMPARE(r.field(3).length(), 10);
    QCOMPARE(r.field(3).precision(), 3);
    QCOMPARE(r.field(4).length(), 18);
    QCOMPARE(r.field(4).precision(), 4);
    QVERIFY(r.field(0).requiredStatus() == QSqlField::Required);
    QVERIFY(r.field(1).requiredStatus() == QSqlField::Optional);
}

void tst_QSqlDatabase::ibase_fetchBlobs()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    const QString tableName(qTableName("qtest_ibaseblobs", __FILE__, db));
    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QString("CREATE TABLE %1 (blob1 BLOB segment size 256)").arg(tableName)));

    QVERIFY_SQL(q, prepare(QString("INSERT INTO %1 VALUES (?)").arg(tableName)));
    q.bindValue(0, QByteArray().fill('x', 1024));
    QVERIFY_SQL(q, exec());

    QVERIFY_SQL(q, prepare(QString("INSERT INTO %1 VALUES (?)").arg(tableName)));
    q.bindValue(0, QByteArray().fill('x', 16383));
    QVERIFY_SQL(q, exec());

    QVERIFY_SQL(q, prepare(QString("INSERT INTO %1 VALUES (?)").arg(tableName)));
    q.bindValue(0, QByteArray().fill('x', 17408));
    QVERIFY_SQL(q, exec());

    QVERIFY_SQL(q, exec(QString("SELECT * FROM %1").arg(tableName)));

    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toByteArray().size(), 1024);
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toByteArray().size(), 16383);
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toByteArray().size(), 17408);
}

void tst_QSqlDatabase::ibase_procWithoutReturnValues()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    const QString procName(qTableName("qtest_proc1", __FILE__, db));
    q.exec(QString("drop procedure %1").arg(procName));
    QVERIFY_SQL(q, exec("CREATE PROCEDURE " + procName + " (str VARCHAR(10))\nAS BEGIN\nstr='test';\nEND;"));
    QVERIFY_SQL(q, exec(QString("execute procedure %1('qtest')").arg(procName)));
    q.exec(QString("drop procedure %1").arg(procName));
}

void tst_QSqlDatabase::ibase_procWithReturnValues()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    const QString procName(qTableName("qtest_proc2", __FILE__, db));

    QSqlQuery q(db);
    q.exec(QString("drop procedure %1").arg(procName));
    QVERIFY_SQL(q, exec("CREATE PROCEDURE " + procName + " ("
                        "\nABC INTEGER)"
                        "\nRETURNS ("
                        "\nRESULT INTEGER)"
                        "\nAS"
                        "\nbegin"
                        "\nRESULT = 10 * ABC;"
                        "\nsuspend;"
                        "\nend"));

    // Interbase procedures can be executed in two ways: EXECUTE PROCEDURE or SELECT
    QVERIFY_SQL(q, exec(QString("execute procedure %1(123)").arg(procName)));
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 1230);
    QVERIFY_SQL(q, exec(QString("select result from %1(456)").arg(procName)));
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 4560);
    QVERIFY_SQL(q, prepare(QLatin1String("execute procedure ")+procName+QLatin1String("(?)")));
    q.bindValue(0, 123);
    QVERIFY_SQL(q, exec());
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 1230);
    q.bindValue(0, 456);
    QVERIFY_SQL(q, exec());
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 4560);

    q.exec(QString("drop procedure %1").arg(procName));
}

void tst_QSqlDatabase::formatValueTrimStrings()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    const QString tableName = qTableName("qtest", __FILE__, db);
    QVERIFY_SQL(q, exec(QString("INSERT INTO %1 (id, t_varchar, t_char) values (50, 'Trim Test ', 'Trim Test 2   ')").arg(tableName)));
    QVERIFY_SQL(q, exec(QString("INSERT INTO %1 (id, t_varchar, t_char) values (51, 'TrimTest', 'Trim Test 2')").arg(tableName)));
    QVERIFY_SQL(q, exec(QString("INSERT INTO %1 (id, t_varchar, t_char) values (52, ' ', '    ')").arg(tableName)));

    QVERIFY_SQL(q, exec(QString("SELECT t_varchar, t_char FROM %1 WHERE id >= 50 AND id <= 52 ORDER BY id").arg(tableName)));

    QVERIFY_SQL(q, next());

    QCOMPARE(db.driver()->formatValue(q.record().field(0), true), QString("'Trim Test'"));
    QCOMPARE(db.driver()->formatValue(q.record().field(1), true), QString("'Trim Test 2'"));

    QVERIFY_SQL(q, next());
    QCOMPARE(db.driver()->formatValue(q.record().field(0), true), QString("'TrimTest'"));
    QCOMPARE(db.driver()->formatValue(q.record().field(1), true), QString("'Trim Test 2'"));

    QVERIFY_SQL(q, next());
    QCOMPARE(db.driver()->formatValue(q.record().field(0), true), QString("''"));
    QCOMPARE(db.driver()->formatValue(q.record().field(1), true), QString("''"));

}

void tst_QSqlDatabase::odbc_reopenDatabase()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    const QString tableName = qTableName("qtest", __FILE__, db);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec("SELECT * from " + tableName));
    QVERIFY_SQL(q, next());
    db.open();
    QVERIFY_SQL(q, exec("SELECT * from " + tableName));
    QVERIFY_SQL(q, next());
    db.open();
}

void tst_QSqlDatabase::odbc_bindBoolean()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);
    if (dbType == QSqlDriverPrivate::MySqlServer)
        QSKIP("MySql has inconsistent behaviour of bit field type across versions.");

    QSqlQuery q(db);
    const QString tableName = qTableName("qtestBindBool", __FILE__, db);
    QVERIFY_SQL(q, exec("CREATE TABLE " + tableName + "(id int, boolvalue bit)"));

    // Bind and insert
    QVERIFY_SQL(q, prepare("INSERT INTO " + tableName + " VALUES(?, ?)"));
    q.bindValue(0, 1);
    q.bindValue(1, true);
    QVERIFY_SQL(q, exec());
    q.bindValue(0, 2);
    q.bindValue(1, false);
    QVERIFY_SQL(q, exec());

    // Retrive
    QVERIFY_SQL(q, exec("SELECT id, boolvalue FROM " + tableName + " ORDER BY id"));
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 1);
    QCOMPARE(q.value(1).toBool(), true);
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 2);
    QCOMPARE(q.value(1).toBool(), false);
}

void tst_QSqlDatabase::odbc_testqGetString()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QString testqGetString(qTableName("testqGetString", __FILE__, db));

    QSqlQuery q(db);
    QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);
    if (dbType == QSqlDriverPrivate::MSSqlServer)
        QVERIFY_SQL(q, exec("CREATE TABLE " + testqGetString + "(id int, vcvalue varchar(MAX))"));
    else if(tst_Databases::isMSAccess(db))
        QVERIFY_SQL(q, exec("CREATE TABLE " + testqGetString + "(id int, vcvalue memo)"));
    else
        QVERIFY_SQL(q, exec("CREATE TABLE " + testqGetString + "(id int, vcvalue varchar(65538))"));

    QString largeString;
    largeString.fill('A', 65536);

    // Bind and insert
    QVERIFY_SQL(q, prepare("INSERT INTO " + testqGetString + " VALUES(?, ?)"));
    q.bindValue(0, 1);
    q.bindValue(1, largeString);
    QVERIFY_SQL(q, exec());
    q.bindValue(0, 2);
    q.bindValue(1, largeString+QLatin1Char('B'));
    QVERIFY_SQL(q, exec());
    q.bindValue(0, 3);
    q.bindValue(1, largeString+QLatin1Char('B')+QLatin1Char('C'));
    QVERIFY_SQL(q, exec());

    // Retrive
    QVERIFY_SQL(q, exec("SELECT id, vcvalue FROM " + testqGetString + " ORDER BY id"));
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 1);
    QCOMPARE(q.value(1).toString().length(), 65536);
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 2);
    QCOMPARE(q.value(1).toString().length(), 65537);
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 3);
    QCOMPARE(q.value(1).toString().length(), 65538);
}


void tst_QSqlDatabase::mysql_multiselect()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QString qtest(qTableName("qtest", __FILE__, db));

    QSqlQuery q(db);
    QString version=tst_Databases::getMySqlVersion( db );
    double ver=version.section(QChar::fromLatin1('.'),0,1).toDouble();
    if (ver < 4.1)
        QSKIP("Test requires MySQL >= 4.1");

    QVERIFY_SQL(q, exec("SELECT * FROM " + qtest + "; SELECT * FROM " + qtest));
    QVERIFY_SQL(q, next());
    QVERIFY_SQL(q, exec("SELECT * FROM " + qtest + "; SELECT * FROM " + qtest));
    QVERIFY_SQL(q, next());
    QVERIFY_SQL(q, exec("SELECT * FROM " + qtest));
}

void tst_QSqlDatabase::ibase_useCustomCharset()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    QString nonlatin1string("��");

    db.close();
    db.setConnectOptions("ISC_DPB_LC_CTYPE=Latin1");
    db.open();

    const QString tableName(qTableName("latin1table", __FILE__, db));

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QString("CREATE TABLE %1(text VARCHAR(6) CHARACTER SET Latin1)").arg(tableName)));
    QVERIFY_SQL(q, prepare(QString("INSERT INTO %1 VALUES(?)").arg(tableName)));
    q.addBindValue(nonlatin1string);
    QVERIFY_SQL(q, exec());
    QVERIFY_SQL(q, exec(QString("SELECT text FROM %1").arg(tableName)));
    QVERIFY_SQL(q, next());
    QCOMPARE(toHex(q.value(0).toString()), toHex(nonlatin1string));
}

void tst_QSqlDatabase::oci_serverDetach()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    for (int i = 0; i < 2; i++) {
        db.close();
        if (db.open()) {
            QSqlQuery query(db);
            query.exec("SELECT 1 FROM DUAL");
            db.close();
        } else {
            QFAIL(tst_Databases::printError(db.lastError(), db));
        }
    }
    if(!db.open())
        qFatal("%s", qPrintable(tst_Databases::printError(db.lastError(), db)));
}

void tst_QSqlDatabase::oci_xmltypeSupport()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    const QString tableName(qTableName("qtest_xmltype", __FILE__, db));
    QString xml("<?xml version=\"1.0\"?>\n<TABLE_NAME>MY_TABLE</TABLE_NAME>\n");
    QSqlQuery q(db);

    // Embedding the XML in the statement
    if(!q.exec(QString("CREATE TABLE %1(xmldata xmltype)").arg(tableName)))
        QSKIP("This test requries xml type support");
    QVERIFY_SQL(q, exec(QString("INSERT INTO %1 values('%2')").arg(tableName).arg(xml)));
    QVERIFY_SQL(q, exec(QString("SELECT a.xmldata.getStringVal() FROM %1 a").arg(tableName)));
    QVERIFY_SQL(q, last());
    QCOMPARE(q.value(0).toString(), xml);

    // Binding the XML with a prepared statement
    QVERIFY_SQL(q, prepare(QString("INSERT INTO %1 values(?)").arg(tableName)));
    q.addBindValue(xml);
    QVERIFY_SQL(q, exec());
    QVERIFY_SQL(q, exec(QString("SELECT a.xmldata.getStringVal() FROM %1 a").arg(tableName)));
    QVERIFY_SQL(q, last());
    QCOMPARE(q.value(0).toString(), xml);
}


void tst_QSqlDatabase::oci_fieldLength()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    const QString tableName(qTableName("qtest", __FILE__, db));
    QSqlQuery q(db);

    QVERIFY_SQL(q, exec(QString("SELECT t_varchar, t_char FROM %1").arg(tableName)));
    QVERIFY_SQL(q, next());
    QCOMPARE(q.record().field(0).length(), 40);
    QCOMPARE(q.record().field(1).length(), 40);
}

void tst_QSqlDatabase::oci_synonymstest()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    const QString creator(qTableName("CREATOR", __FILE__, db)), appuser(qTableName("APPUSER", __FILE__, db)), table1(qTableName("TABLE1", __FILE__, db));
//     QVERIFY_SQL(q, exec("drop public synonym "+table1));
    QVERIFY_SQL(q, exec(QString("create user %1 identified by %2 default tablespace users temporary tablespace temp").arg(creator).arg(creator)));
    QVERIFY_SQL(q, exec(QString("grant CONNECT to %1").arg(creator)));
    QVERIFY_SQL(q, exec(QString("grant RESOURCE to %1").arg(creator)));
    QSqlDatabase db2=db.cloneDatabase(db, QLatin1String("oci_synonymstest"));
    db2.close();
    QVERIFY_SQL(db2, open(creator,creator));
    QSqlQuery q2(db2);
    QVERIFY_SQL(q2, exec(QString("create table %1(id int primary key)").arg(table1)));
    QVERIFY_SQL(q, exec(QString("create user %1 identified by %2 default tablespace users temporary tablespace temp").arg(appuser).arg(appuser)));
    QVERIFY_SQL(q, exec(QString("grant CREATE ANY SYNONYM to %1").arg(appuser)));
    QVERIFY_SQL(q, exec(QString("grant CONNECT to %1").arg(appuser)));
    QVERIFY_SQL(q2, exec(QString("grant select, insert, update, delete on %1 to %2").arg(table1).arg(appuser)));
    QSqlDatabase db3=db.cloneDatabase(db, QLatin1String("oci_synonymstest2"));
    db3.close();
    QVERIFY_SQL(db3, open(appuser,appuser));
    QSqlQuery q3(db3);
    QVERIFY_SQL(q3, exec("create synonym " + appuser + '.' + qTableName("synonyms", __FILE__, db) + " for " + creator + '.' + table1));
    QVERIFY_SQL(db3, tables().filter(qTableName("synonyms", __FILE__, db), Qt::CaseInsensitive).count() >= 1);
}


// This test isn't really necessary as SQL_GUID / uniqueidentifier is
// already tested in recordSQLServer().
void tst_QSqlDatabase::odbc_uniqueidentifier()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);
    if (dbType != QSqlDriverPrivate::MSSqlServer)
        QSKIP("SQL Server (ODBC) specific test");

    const QString tableName(qTableName("qtest_sqlguid", __FILE__, db));
    QString guid = QString("AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE");
    QString invalidGuid = QString("GAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE");

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QString("CREATE TABLE %1(id uniqueidentifier)").arg(tableName)));

    q.prepare(QString("INSERT INTO %1 VALUES(?)").arg(tableName));;
    q.addBindValue(guid);
    QVERIFY_SQL(q, exec());

    q.addBindValue(invalidGuid);
    QEXPECT_FAIL("", "The GUID string is required to be correctly formated!",
        Continue);
    QVERIFY_SQL(q, exec());

    QVERIFY_SQL(q, exec(QString("SELECT id FROM %1").arg(tableName)));
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toString(), guid);
}

void tst_QSqlDatabase::getConnectionName()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QCOMPARE(db.connectionName(), dbName);
    QSqlDatabase clone = QSqlDatabase::cloneDatabase(db, "clonedDatabase");
    QCOMPARE(clone.connectionName(), QString("clonedDatabase"));
    QTest::ignoreMessage(QtWarningMsg, "QSqlDatabasePrivate::removeDatabase: "
        "connection 'clonedDatabase' is still in use, all queries will cease to work.");
    QSqlDatabase::removeDatabase("clonedDatabase");
    QCOMPARE(clone.connectionName(), QString());
    QCOMPARE(db.connectionName(), dbName);
}

void tst_QSqlDatabase::odbc_uintfield()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    const QString tableName(qTableName("uint_table", __FILE__, db));
    unsigned int val = 4294967295U;

    QSqlQuery q(db);
    if ( tst_Databases::isMSAccess( db ) )
        QVERIFY_SQL(q, exec(QString("CREATE TABLE %1(num number)").arg(tableName)));
    else
        QVERIFY_SQL(q, exec(QString("CREATE TABLE %1(num numeric(10))").arg(tableName)));
    q.prepare(QString("INSERT INTO %1 VALUES(?)").arg(tableName));
    q.addBindValue(val);
    QVERIFY_SQL(q, exec());

    q.exec(QString("SELECT num FROM %1").arg(tableName));
    if (q.next())
        QCOMPARE(q.value(0).toUInt(), val);
}

void tst_QSqlDatabase::eventNotification()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlDriver *driver = db.driver();
    if (!driver->hasFeature(QSqlDriver::EventNotifications))
        QSKIP("DBMS doesn't support event notifications");

    // Not subscribed to any events yet
    QCOMPARE(driver->subscribedToNotifications().size(), 0);

    // Subscribe to "event_foo"
    QVERIFY_SQL(*driver, subscribeToNotification(QLatin1String("event_foo")));
    QCOMPARE(driver->subscribedToNotifications().size(), 1);
    QVERIFY(driver->subscribedToNotifications().contains("event_foo"));

    // Can't subscribe to the same event multiple times
    QVERIFY2(!driver->subscribeToNotification(QLatin1String("event_foo")), "Shouldn't be able to subscribe to event_foo twice");
    QCOMPARE(driver->subscribedToNotifications().size(), 1);

    // Unsubscribe from "event_foo"
    QVERIFY_SQL(*driver, unsubscribeFromNotification(QLatin1String("event_foo")));
    QCOMPARE(driver->subscribedToNotifications().size(), 0);

    // Re-subscribing to "event_foo" now is allowed
    QVERIFY_SQL(*driver, subscribeToNotification(QLatin1String("event_foo")));
    QCOMPARE(driver->subscribedToNotifications().size(), 1);

    // closing the connection causes automatically unsubscription from all events
    db.close();
    QCOMPARE(driver->subscribedToNotifications().size(), 0);

    // Can't subscribe to anything while database is closed
    QVERIFY2(!driver->subscribeToNotification(QLatin1String("event_foo")), "Shouldn't be able to subscribe to event_foo");
    QCOMPARE(driver->subscribedToNotifications().size(), 0);

    db.open();
}

void tst_QSqlDatabase::eventNotificationIBase()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    const QString procedureName(qTableName("posteventProc", __FILE__, db));
    QSqlDriver *driver=db.driver();
    QVERIFY_SQL(*driver, subscribeToNotification(procedureName));
    QTest::qWait(300);  // Interbase needs some time to call the driver callback.

    db.transaction();   // InterBase events are posted from within transactions.
    QSqlQuery q(db);
    q.exec(QString("DROP PROCEDURE %1").arg(procedureName));
    q.exec(QString("CREATE PROCEDURE %1\nAS BEGIN\nPOST_EVENT '%1';\nEND;").arg(procedureName));
    q.exec(QString("EXECUTE PROCEDURE %1").arg(procedureName));
    QSignalSpy spy(driver, SIGNAL(notification(QString)));
    db.commit();        // No notifications are posted until the transaction is committed.
    QTest::qWait(300);  // Interbase needs some time to post the notification and call the driver callback.
                        // This happends from another thread, and we have to process events in order for the
                        // event handler in the driver to be executed and emit the notification signal.

    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString() == procedureName);
    QVERIFY_SQL(*driver, unsubscribeFromNotification(procedureName));
    q.exec(QString("DROP PROCEDURE %1").arg(procedureName));
}

void tst_QSqlDatabase::eventNotificationPSQL()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery query(db);
    QString procedureName = qTableName("posteventProc", __FILE__, db);
    QString payload = "payload";
    QSqlDriver &driver=*(db.driver());
    QVERIFY_SQL(driver, subscribeToNotification(procedureName));
    QSignalSpy spy(db.driver(), SIGNAL(notification(QString,QSqlDriver::NotificationSource,QVariant)));
    query.exec(QString("NOTIFY \"%1\", '%2'").arg(procedureName).arg(payload));
    QCoreApplication::processEvents();
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), procedureName);
    QCOMPARE(qvariant_cast<QSqlDriver::NotificationSource>(arguments.at(1)), QSqlDriver::SelfSource);
    QCOMPARE(qvariant_cast<QVariant>(arguments.at(2)).toString(), payload);
    QVERIFY_SQL(driver, unsubscribeFromNotification(procedureName));
}

void tst_QSqlDatabase::sqlite_bindAndFetchUInt()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    if (db.driverName().startsWith("QSQLITE2"))
        QSKIP("SQLite3 specific test");

    QSqlQuery q(db);
    const QString tableName(qTableName("uint_test", __FILE__, db));
    QVERIFY_SQL(q, exec(QString("CREATE TABLE %1(uint_field UNSIGNED INTEGER)").arg(tableName)));
    QVERIFY_SQL(q, prepare(QString("INSERT INTO %1 VALUES(?)").arg(tableName)));
    q.addBindValue(4000000000U);
    QVERIFY_SQL(q, exec());
    QVERIFY_SQL(q, exec(QString("SELECT uint_field FROM %1").arg(tableName)));
    QVERIFY_SQL(q, next());

    // All integers in SQLite are signed, so even though we bound the value
    // as an UInt it will come back as a LongLong
    QCOMPARE(q.value(0).type(), QVariant::LongLong);
    QCOMPARE(q.value(0).toUInt(), 4000000000U);
}

void tst_QSqlDatabase::db2_valueCacheUpdate()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    const QString tableName(qTableName("qtest", __FILE__, db));
    QSqlQuery q(db);
    q.exec(QString("SELECT id, t_varchar, t_char, t_numeric FROM %1").arg(tableName));
    q.next();
    QVariant c4 = q.value(3);
    QVariant c3 = q.value(2);
    QVariant c2 = q.value(1);
    QVariant c1 = q.value(0);
    QCOMPARE(c4.toString(), q.value(3).toString());
    QCOMPARE(c3.toString(), q.value(2).toString());
    QCOMPARE(c2.toString(), q.value(1).toString());
    QCOMPARE(c1.toString(), q.value(0).toString());
}

void tst_QSqlDatabase::sqlStatementUseIsNull_189093()
{
    // NULL = NULL is unknown, the sqlStatment must use IS NULL
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    // select a record with NULL value
    QSqlQuery q(QString::null, db);
    QVERIFY_SQL(q, exec("select * from " + qTableName("qtest", __FILE__, db) + " where id = 4"));
    QVERIFY_SQL(q, next());

    QSqlDriver *driver = db.driver();
    QVERIFY(driver);

    QString preparedStatment = driver->sqlStatement(QSqlDriver::WhereStatement, QString("qtest"), q.record(), true);
    QCOMPARE(preparedStatment.count("IS NULL", Qt::CaseInsensitive), 2);

    QString statment = driver->sqlStatement(QSqlDriver::WhereStatement, QString("qtest"), q.record(), false);
    QCOMPARE(statment.count("IS NULL", Qt::CaseInsensitive), 2);
}

void tst_QSqlDatabase::mysql_savepointtest()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    if (tst_Databases::getMySqlVersion(db).section(QChar('.'), 0, 1).toDouble() < 4.1)
        QSKIP( "Test requires MySQL >= 4.1");

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec("begin"));
    QVERIFY_SQL(q, exec("insert into " + qTableName("qtest", __FILE__, db) + " VALUES (54, 'foo', 'foo', 54.54)"));
    QVERIFY_SQL(q, exec("savepoint foo"));
}

void tst_QSqlDatabase::oci_tables()
{
    QSKIP("Requires specific permissions to create a system table");
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    QSqlQuery q(db);
    const QString systemTableName("sys." + qTableName("mypassword", __FILE__, db).toUpper());
    QVERIFY_SQL(q, exec("CREATE TABLE "+systemTableName+"(name VARCHAR(20))"));
    QVERIFY(!db.tables().contains(systemTableName.toUpper()));
    QVERIFY(db.tables(QSql::SystemTables).contains(systemTableName.toUpper()));
}

void tst_QSqlDatabase::sqlite_enable_cache_mode()
{
    QFETCH(QString, dbName);
    if(dbName.endsWith(":memory:"))
        QSKIP( "cache mode is meaningless for :memory: databases");
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    db.close();
    db.setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
    QVERIFY_SQL(db, open());
    QSqlDatabase db2 = QSqlDatabase::cloneDatabase(db, dbName+":cachemodeconn2");
    db2.setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");
    QVERIFY_SQL(db2, open());
    QSqlQuery q(db), q2(db2);
    QVERIFY_SQL(q, exec("select * from " + qTableName("qtest", __FILE__, db)));
    QVERIFY_SQL(q2, exec("select * from " + qTableName("qtest", __FILE__, db)));
    db2.close();
}

QTEST_MAIN(tst_QSqlDatabase)
#include "tst_qsqldatabase.moc"
