// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QtSql/QtSql>

#include <QtCore/QDateTime>
#include <QtCore/QTimeZone>

#include <numeric>

#include "../qsqldatabase/tst_databases.h"

using namespace Qt::StringLiterals;

QString qtest;

class tst_QSqlQuery : public QObject
{
    Q_OBJECT

public:
    tst_QSqlQuery();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

private slots:
    void value_data() { generic_data(); }
    void value();
    void isValid_data() { generic_data(); }
    void isValid();
    void isActive_data() { generic_data(); }
    void isActive();
    void isSelect_data() { generic_data(); }
    void isSelect();
    void numRowsAffected_data() { generic_data(); }
    void numRowsAffected();
    void size_data() { generic_data(); }
    void size();
    void isNull_data() { generic_data(); }
    void isNull();
    void writeNull_data() { generic_data(); }
    void writeNull();
    void query_exec_data() { generic_data(); }
    void query_exec();
    void execErrorRecovery_data() { generic_data(); }
    void execErrorRecovery();
    void first_data() { generic_data(); }
    void first();
    void next_data() { generic_data(); }
    void next();
    void prev_data() { generic_data(); }
    void prev();
    void last_data() { generic_data(); }
    void last();
    void seek_data() { generic_data(); }
    void seek();
    void record_data() { generic_data(); }
    void record();
    void record_sqlite_data() { generic_data("QSQLITE"); }
    void record_sqlite();
    void finish_data() { generic_data(); }
    void finish();
    void sqlite_finish_data() { generic_data("QSQLITE"); }
    void sqlite_finish();
    void nextResult_data() { generic_data(); }
    void nextResult();

    // forwardOnly mode need special treatment
    void forwardOnly_data() { generic_data(); }
    void forwardOnly();
    void forwardOnlyMultipleResultSet_data() { generic_data(); }
    void forwardOnlyMultipleResultSet();
    void psql_forwardOnlyQueryResultsLost_data() { generic_data("QPSQL"); }
    void psql_forwardOnlyQueryResultsLost();

    // Bug-specific tests:
    void oci_nullBlob_data() { generic_data("QOCI"); }
    void oci_nullBlob();
    void blob_data() { generic_data(); }
    void blob();
    void oci_rawField_data() { generic_data("QOCI"); }
    void oci_rawField();
    void precision_data() { generic_data(); }
    void precision();
    void nullResult_data() { generic_data(); }
    void nullResult();
    void joins_data() { generic_data(); }
    void joins();
    void outValues_data() { generic_data(); }
    void outValues();
    void char1Select_data() { generic_data(); }
    void char1Select();
    void char1SelectUnicode_data() { generic_data(); }
    void char1SelectUnicode();
    void synonyms_data() { generic_data(); }
    void synonyms();
    void oraOutValues_data() { generic_data("QOCI"); }
    void oraOutValues();
    void mysql_outValues_data() { generic_data("QMYSQL"); }
    void mysql_outValues();
    void oraClob_data() { generic_data("QOCI"); }
    void oraClob();
    void oraClobBatch_data() { generic_data("QOCI"); }
    void oraClobBatch();
    void oraLong_data() { generic_data("QOCI"); }
    void oraLong();
    void oraOCINumber_data() { generic_data("QOCI"); }
    void oraOCINumber();
    void outValuesDB2_data() { generic_data("QDB2"); }
    void outValuesDB2();
    void storedProceduresIBase_data() {generic_data("QIBASE"); }
    void storedProceduresIBase();
    void oraRowId_data() { generic_data("QOCI"); }
    void oraRowId();
    void prepare_bind_exec_data() { generic_data(); }
    void prepare_bind_exec();
    void prepared_select_data() { generic_data(); }
    void prepared_select();
    void sqlServerLongStrings_data() { generic_data(); }
    void sqlServerLongStrings();
    void invalidQuery_data() { generic_data(); }
    void invalidQuery();
    void batchExec_data() { generic_data(); }
    void batchExec();
    void QTBUG_43874_data() { generic_data(); }
    void QTBUG_43874();
    void oraArrayBind_data() { generic_data("QOCI"); }
    void oraArrayBind();
    void lastInsertId_data() { generic_data(); }
    void lastInsertId();
    void lastQuery_data() { generic_data(); }
    void lastQuery();
    void lastQueryTwoQueries_data() { generic_data(); }
    void lastQueryTwoQueries();
    void bindBool_data() { generic_data(); }
    void bindBool();
    void psql_bindWithDoubleColonCastOperator_data() { generic_data("QPSQL"); }
    void psql_bindWithDoubleColonCastOperator();
    void psql_specialFloatValues_data() { generic_data("QPSQL"); }
    void psql_specialFloatValues();
    void queryOnInvalidDatabase_data() { generic_data(); }
    void queryOnInvalidDatabase();
    void createQueryOnClosedDatabase_data() { generic_data(); }
    void createQueryOnClosedDatabase();
    void seekForwardOnlyQuery_data() { generic_data(); }
    void seekForwardOnlyQuery();
    void reExecutePreparedForwardOnlyQuery_data() { generic_data(); }
    void reExecutePreparedForwardOnlyQuery();
    void blobsPreparedQuery_data() { generic_data(); }
    void blobsPreparedQuery();
    void emptyTableNavigate_data() { generic_data(); }
    void emptyTableNavigate();
    void timeStampParsing_data() { generic_data(); }
    void timeStampParsing();
    void sqliteVirtualTable_data() { generic_data("QSQLITE"); }
    void sqliteVirtualTable();
    void mysql_timeType_data() { generic_data("QMYSQL"); }
    void mysql_timeType();
    void ibase_executeBlock_data() { generic_data("QIBASE"); }
    void ibase_executeBlock();

    void task_217003_data() { generic_data(); }
    void task_217003();

    void task_250026_data() { generic_data("QODBC"); }
    void task_250026();
    void crashQueryOnCloseDatabase();

    void testNaN_data() { generic_data("QPSQL"); }
    void testNaN();

    void psqlNumericMetadata_data() { generic_data("QPSQL"); }
    void psqlNumericMetadata();

    void sqlServerReturn0_data() { generic_data(); }
    void sqlServerReturn0();

    void QTBUG_551_data() { generic_data("QOCI"); }
    void QTBUG_551();

    void QTBUG_5251_data() { generic_data("QPSQL"); }
    void QTBUG_5251();
    void QTBUG_6421_data() { generic_data("QOCI"); }
    void QTBUG_6421();
    void QTBUG_6618_data() { generic_data("QODBC"); }
    void QTBUG_6618();
    void QTBUG_6852_data() { generic_data("QMYSQL"); }
    void QTBUG_6852();
    void QTBUG_5765_data() { generic_data("QMYSQL"); }
    void QTBUG_5765();
    void QTBUG_12186_data() { generic_data("QSQLITE"); }
    void QTBUG_12186();
    void QTBUG_14132_data() { generic_data("QOCI"); }
    void QTBUG_14132();
    void QTBUG_18435_data() { generic_data("QODBC"); }
    void QTBUG_18435();
    void QTBUG_21884_data() { generic_data("QSQLITE"); }
    void QTBUG_21884();
    void QTBUG_16967_data() { generic_data("QSQLITE"); }
    void QTBUG_16967(); // clean close
    void QTBUG_23895_data() { generic_data("QSQLITE"); }
    void QTBUG_23895(); // sqlite boolean type
    void QTBUG_14904_data() { generic_data("QSQLITE"); }
    void QTBUG_14904();

    void QTBUG_2192_data() { generic_data(); }
    void QTBUG_2192();

    void QTBUG_36211_data() { generic_data("QPSQL"); }
    void QTBUG_36211();

    void QTBUG_53969_data() { generic_data("QMYSQL"); }
    void QTBUG_53969();

    void gisPointDatatype_data() { generic_data("QMYSQL"); }
    void gisPointDatatype();

    void sqlite_constraint_data() { generic_data("QSQLITE"); }
    void sqlite_constraint();

    void sqlite_real_data() { generic_data("QSQLITE"); }
    void sqlite_real();

    void prepared_query_json_row_data() { generic_data(); }
    void prepared_query_json_row();

    void aggregateFunctionTypes_data() { generic_data(); }
    void aggregateFunctionTypes();

    void integralTypesMysql_data() { generic_data("QMYSQL"); }
    void integralTypesMysql();

    void QTBUG_57138_data() { generic_data("QSQLITE"); }
    void QTBUG_57138();

    void QTBUG_73286_data() { generic_data("QODBC"); }
    void QTBUG_73286();

    void insertVarChar1_data() { generic_data("QODBC"); }
    void insertVarChar1();

    void dateTime_data();
    void dateTime();

    void ibaseArray_data() { generic_data("QIBASE"); }
    void ibaseArray();

    void ibaseDateTimeWithTZ_data();
    void ibaseDateTimeWithTZ();
    void ibaseTimeStampTzArray_data() { generic_data("QIBASE"); }
    void ibaseTimeStampTzArray();

    // Double addDatabase() with same name leaves system in a state that breaks
    // invalidQuery() if run later; so put this one last !
    void prematureExec_data() { generic_data(); }
    void prematureExec();
private:
    // returns all database connections
    void generic_data(const QString &engine=QString());
    void dropTestTables(QSqlDatabase db);
    void createTestTables(QSqlDatabase db);
    void populateTestTables(QSqlDatabase db);

    tst_Databases dbs;
};

tst_QSqlQuery::tst_QSqlQuery()
{
    static QSqlDatabase static_qtest_db = QSqlDatabase();
    qtest = qTableName("qtest", __FILE__, static_qtest_db);
}

void tst_QSqlQuery::initTestCase()
{
    QVERIFY(dbs.open());

    for (const QString &name : std::as_const(dbs.dbNames)) {
        QSqlDatabase db = QSqlDatabase::database(name);
        CHECK_DATABASE(db);
        dropTestTables(db); // in case of leftovers
        createTestTables(db);
        populateTestTables(db);
    }
}

void tst_QSqlQuery::cleanupTestCase()
{
    for (const QString &name : std::as_const(dbs.dbNames)) {
        QSqlDatabase db = QSqlDatabase::database(name);
        CHECK_DATABASE(db);
        dropTestTables(db);
    }

    dbs.close();
}

void tst_QSqlQuery::cleanup()
{
    if (QTest::currentTestFunction() == QLatin1String("crashQueryOnCloseDatabase"))
        return;
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    if (QTest::currentTestFunction() == QLatin1String("numRowsAffected")
            || QTest::currentTestFunction() == QLatin1String("transactions")
            || QTest::currentTestFunction() == QLatin1String("size")
            || QTest::currentTestFunction() == QLatin1String("isActive")
            || QTest::currentTestFunction() == QLatin1String("lastInsertId")) {
        populateTestTables(db);
    }

    if (QTest::currentTestFailed() && (tst_Databases::getDatabaseType(db) == QSqlDriver::Oracle
                                       || db.driverName().startsWith("QODBC"))) {
        // Since Oracle ODBC totally craps out on error, we init again:
        db.close();
        db.open();
    }
}

void tst_QSqlQuery::generic_data(const QString &engine)
{
    if (dbs.fillTestTable(engine))
        return;

    if (engine.isEmpty())
        QSKIP("No database drivers are available in this Qt configuration");

    QSKIP(qPrintable(QLatin1String("No database drivers of type %1 "
                                   "are available in this Qt configuration").arg(engine)));
}

void tst_QSqlQuery::dropTestTables(QSqlDatabase db)
{
    QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    QStringList tablenames;
    // Drop all the table in case a testcase failed:
    tablenames <<  qtest
               << qTableName("qtest_null", __FILE__, db)
               << qTableName("tst_record", __FILE__, db);

    tst_Databases::safeDropTables(db, tablenames);

    if (dbType == QSqlDriver::Oracle) {
        QSqlQuery q(db);
        q.exec("DROP PACKAGE " + qTableName("pkg", __FILE__, db));
    }
}

void tst_QSqlQuery::createTestTables(QSqlDatabase db)
{
    QSqlQuery q(db);
    QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    if (dbType == QSqlDriver::MySqlServer)
        // ### stupid workaround until we find a way to hardcode this
        // in the MySQL server startup script
        q.exec("set table_type=innodb");
    else if (dbType == QSqlDriver::PostgreSQL)
        QVERIFY_SQL(q, exec("set client_min_messages='warning'"));

    if (dbType == QSqlDriver::PostgreSQL) {
        QVERIFY_SQL(q, exec(QLatin1String(
                                "create table %1 (id serial NOT NULL, t_varchar varchar(20), "
                                "t_char char(20), primary key(id))").arg(qtest)));
    } else {
        QVERIFY_SQL(q, exec(QLatin1String(
                                "create table %1 (id int %2 NOT NULL, t_varchar varchar(20), "
                                "t_char char(20), primary key(id))")
                                .arg(qtest, tst_Databases::autoFieldName(db))));
    }

    QLatin1String creator(dbType == QSqlDriver::MSSqlServer || dbType == QSqlDriver::Sybase
                          ? "create table %1 (id int null, t_varchar varchar(20) null)"
                          : "create table %1 (id int, t_varchar varchar(20))");
    QVERIFY_SQL(q, exec(creator.arg(qTableName("qtest_null", __FILE__, db))));
}

void tst_QSqlQuery::populateTestTables(QSqlDatabase db)
{
    QSqlQuery q(db);
    const QString qtest_null(qTableName("qtest_null", __FILE__, db));
    q.exec("delete from " + qtest);
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (1, 'VarChar1', 'Char1')")
                        .arg(qtest)));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (2, 'VarChar2', 'Char2')")
                        .arg(qtest)));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (3, 'VarChar3', 'Char3')")
                        .arg(qtest)));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (4, 'VarChar4', 'Char4')")
                        .arg(qtest)));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (5, 'VarChar5', 'Char5')")
                        .arg(qtest)));

    q.exec("delete from " + qtest_null);
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (0, NULL)").arg(qtest_null)));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (1, 'n')").arg(qtest_null)));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (2, 'i')").arg(qtest_null)));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (3, NULL)").arg(qtest_null)));
}

// There were problems with char fields of size 1
void tst_QSqlQuery::char1Select()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    {
        TableScope ts(db, "char1Select", __FILE__);
        QSqlQuery q(db);
        QVERIFY_SQL(q, exec(QLatin1String("create table %1 (id char(1))").arg(ts.tableName())));
        QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values ('a')").arg(ts.tableName())));
        QVERIFY_SQL(q, exec("select * from " + ts.tableName()));
        QVERIFY(q.next());
        if (tst_Databases::getDatabaseType(db) == QSqlDriver::Interbase)
            QCOMPARE(q.value(0).toString().left(1), u"a");
        else
            QCOMPARE(q.value(0).toString(), u"a");

        QVERIFY(!q.next());
    }
}

void tst_QSqlQuery::char1SelectUnicode()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    if (dbType == QSqlDriver::DB2)
        QSKIP("Needs someone with more Unicode knowledge than I have to fix");

    if (!db.driver()->hasFeature(QSqlDriver::Unicode))
        QSKIP("Database not unicode capable");

    QString uniStr(QChar(0x0915)); // DEVANAGARI LETTER KA
    TableScope ts(db, "char1SU", __FILE__);
    QSqlQuery q(db);
    QLatin1String createQuery;

    switch (dbType) {
    case QSqlDriver::MimerSQL:
    case QSqlDriver::MSSqlServer:
        createQuery = QLatin1String("create table %1(id nchar(1))");
        break;
    case QSqlDriver::DB2:
    case QSqlDriver::Oracle:
    case QSqlDriver::PostgreSQL:
        createQuery = QLatin1String("create table %1 (id char(3))");
        break;
    case QSqlDriver::Interbase:
        createQuery = QLatin1String("create table %1 (id char(1) character set unicode_fss)");
        break;
    case QSqlDriver::MySqlServer:
        createQuery = QLatin1String("create table %1 (id char(1)) default character set 'utf8'");
        break;
    default:
        createQuery = QLatin1String("create table %1 (id char(1))");
        break;
    }

    QVERIFY_SQL(q, exec(createQuery.arg(ts.tableName())));
    QVERIFY_SQL(q, prepare(QLatin1String("insert into %1 values(?)").arg(ts.tableName())));

    q.bindValue(0, uniStr);
    QVERIFY_SQL(q, exec());
    QVERIFY_SQL(q, exec("select * from " + ts.tableName()));

    QVERIFY(q.next());
    if (!q.value(0).toString().isEmpty())
        QCOMPARE(q.value(0).toString()[0].unicode(), uniStr[0].unicode());

    QCOMPARE(q.value(0).toString().trimmed(), uniStr);
    QVERIFY(!q.next());
}

void tst_QSqlQuery::oraRowId()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "oraRowId", __FILE__);
    const auto &oraRowId = ts.tableName();

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec("select rowid from " + qtest));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).metaType().id(), QMetaType::QString);
    QVERIFY(!q.value(0).toString().isEmpty());

    QVERIFY_SQL(q, exec(QLatin1String("create table %1 (id char(1))").arg(oraRowId)));

    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values('a')").arg(oraRowId)));
    QVariant v1 = q.lastInsertId();
    QVERIFY(v1.isValid());

    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values('b')").arg(oraRowId)));
    QVariant v2 = q.lastInsertId();
    QVERIFY(v2.isValid());

    QVERIFY_SQL(q, prepare(QLatin1String("select * from %1 where rowid = ?").arg(oraRowId)));
    q.addBindValue(v1);
    QVERIFY_SQL(q, exec());
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toString(), u"a");

    q.addBindValue(v2);
    QVERIFY_SQL(q, exec());
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toString(), u"b");
}

void tst_QSqlQuery::mysql_outValues()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QString hello(qTableName("hello", __FILE__, db));
    ProcScope ps(db, "qtestproc", __FILE__);

    QSqlQuery q(db);

    q.exec("drop function " + hello);

    QVERIFY_SQL(q, exec(QLatin1String(
                            "create function %1 (s char(20)) returns varchar(50) "
                            "READS SQL DATA return concat('Hello ', s)").arg(hello)));

    QVERIFY_SQL(q, exec(QLatin1String("select %1('world')").arg(hello)));
    QVERIFY_SQL(q, next());

    QCOMPARE(q.value(0).toString(), u"Hello world");

    QVERIFY_SQL(q, prepare(QLatin1String("select %1('harald')").arg(hello)));
    QVERIFY_SQL(q, exec());
    QVERIFY_SQL(q, next());

    QCOMPARE(q.value(0).toString(), u"Hello harald");

    QVERIFY_SQL(q, exec("drop function " + hello));

    QVERIFY_SQL(q, exec(QLatin1String("create procedure %1 () BEGIN "
                                      "select * from %2 order by id; END").arg(ps.name(), qtest)));
    QVERIFY_SQL(q, exec(QLatin1String("call %1()").arg(ps.name())));
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(1).toString(), u"VarChar1");

    QVERIFY_SQL(q, exec("drop procedure " + ps.name()));
    QVERIFY_SQL(q, exec(QLatin1String("create procedure %1 (OUT param1 INT) "
                                      "BEGIN set param1 = 42; END").arg(ps.name())));

    QVERIFY_SQL(q, exec(QLatin1String("call %1 (@out)").arg(ps.name())));
    QVERIFY_SQL(q, exec("select @out"));
    QCOMPARE(q.record().fieldName(0), u"@out");
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 42);
}

void tst_QSqlQuery::bindBool()
{
    // QTBUG-27763: bool value got converted to int 127 by mysql driver because
    // sizeof(bool) < sizeof(int). The problem was the way the bool value from
    // the application was handled. For our purposes here, it doesn't matter
    // whether the column type is BOOLEAN or INT. All DBMSs have INT, and this
    // usually works for this test. Postresql is an exception because its INT
    // type does not accept BOOLEAN values and its BOOLEAN columns do not accept
    // INT values.
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    QSqlQuery q(db);
    QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    const QString tableName(qTableName("bindBool", __FILE__, db));

    q.exec("DROP TABLE " + tableName);
    const bool useBooleanType = (dbType == QSqlDriver::PostgreSQL || dbType == QSqlDriver::Interbase);
    QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %2 (id INT, flag %1 NOT NULL, PRIMARY KEY(id))")
                        .arg(QLatin1String(useBooleanType ? "BOOLEAN" : "INT"), tableName)));

    for (int i = 0; i < 2; ++i) {
        bool flag = i;
        q.prepare(QLatin1String("INSERT INTO %1 (id, flag) VALUES(:id, :flag)").arg(tableName));
        q.bindValue(":id", i);
        q.bindValue(":flag", flag);
        QVERIFY_SQL(q, exec());
    }

    QVERIFY_SQL(q, exec(QLatin1String("SELECT id, flag FROM %1 ORDER BY id").arg(tableName)));
    for (int i = 0; i < 2; ++i) {
        bool flag = i;
        QVERIFY_SQL(q, next());
        QCOMPARE(q.value(0).toInt(), i);
        QCOMPARE(q.value(1).toBool(), flag);
    }
    QVERIFY_SQL(q, prepare(QLatin1String(
                               "SELECT flag FROM %1 WHERE flag = :filter").arg(tableName)));
    const bool filter = true;
    q.bindValue(":filter", filter);
    QVERIFY_SQL(q, exec());
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toBool(), filter);
    QFAIL_SQL(q, next());
    QVERIFY_SQL(q, exec("DROP TABLE " + tableName));
}

void tst_QSqlQuery::oraOutValues()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QString tst_outValues(qTableName("tst_outValues", __FILE__, db));

    if (!db.driver()->hasFeature(QSqlDriver::PreparedQueries))
        QSKIP("Test requires prepared query support");

    QSqlQuery q(db);

    q.setForwardOnly(true);

    /*** outvalue int ***/
    QVERIFY_SQL(q, exec(QLatin1String("create or replace procedure %1(x out int) is\n"
                                      "begin\n"
                                      "    x := 42;\n"
                                      "end;\n").arg(tst_outValues)));
    QVERIFY(q.prepare(QLatin1String("call %1(?)").arg(tst_outValues)));
    q.addBindValue(0, QSql::Out);
    QVERIFY_SQL(q, exec());
    QCOMPARE(q.boundValue(0).toInt(), 42);

    // Bind a null value, make sure the OCI driver resets the null flag:
    q.addBindValue(QVariant(QMetaType(QMetaType::Int)), QSql::Out);
    QVERIFY_SQL(q, exec());
    QCOMPARE(q.boundValue(0).toInt(), 42);
    QVERIFY(!q.boundValue(0).isNull());

    /*** outvalue varchar ***/
    QVERIFY_SQL(q, exec(QLatin1String(
                            "create or replace procedure %1(x out varchar) is\n"
                            "begin\n"
                            "    x := 'blah';\n"
                            "end;\n").arg(tst_outValues)));
    QVERIFY(q.prepare(QLatin1String("call %1(?)").arg(tst_outValues)));
    QString s1("12345");
    s1.reserve(512);
    q.addBindValue(s1, QSql::Out);
    QVERIFY_SQL(q, exec());
    QCOMPARE(q.boundValue(0).toString(), u"blah");

    /*** in/outvalue numeric ***/
    QVERIFY_SQL(q, exec(QLatin1String(
                            "create or replace procedure %1(x in out numeric) is\n"
                            "begin\n"
                            "    x := x + 10;\n"
                            "end;\n").arg(tst_outValues)));
    QVERIFY(q.prepare(QLatin1String("call %1(?)").arg(tst_outValues)));
    q.addBindValue(10, QSql::Out);
    QVERIFY_SQL(q, exec());
    QCOMPARE(q.boundValue(0).toInt(), 20);

    /*** in/outvalue varchar ***/
    QVERIFY_SQL(q, exec(QLatin1String(
                            "create or replace procedure %1(x in out varchar) is\n"
                            "begin\n"
                            "    x := 'homer';\n"
                            "end;\n").arg(tst_outValues)));
    QVERIFY(q.prepare(QLatin1String("call %1(?)").arg(tst_outValues)));
    q.addBindValue(u"maggy"_s, QSql::Out);
    QVERIFY_SQL(q, exec());
    QCOMPARE(q.boundValue(0).toString(), u"homer");

    /*** in/outvalue varchar ***/
    QVERIFY_SQL(q, exec(QLatin1String(
                            "create or replace procedure %1(x in out varchar) is\n"
                            "begin\n"
                            "    x := NULL;\n"
                            "end;\n").arg(tst_outValues)));
    QVERIFY(q.prepare(QLatin1String("call %1(?)").arg(tst_outValues)));
    q.addBindValue(u"maggy"_s, QSql::Out);
    QVERIFY_SQL(q, exec());
    QVERIFY(q.boundValue(0).isNull());

    /*** in/outvalue int ***/
    QVERIFY_SQL(q, exec(QLatin1String(
                            "create or replace procedure %1(x in out int) is\n"
                            "begin\n"
                            "    x := NULL;\n"
                            "end;\n").arg(tst_outValues)));
    QVERIFY(q.prepare(QLatin1String("call %1(?)").arg(tst_outValues)));
    q.addBindValue(42, QSql::Out);
    QVERIFY_SQL(q, exec());
    QVERIFY(q.boundValue(0).isNull());

    /*** in/outvalue varchar ***/
    QVERIFY_SQL(q, exec(QLatin1String(
                            "create or replace procedure %1(x in varchar, y out varchar) is\n"
                            "begin\n"
                            "    y := x||'bubulalakikikokololo';\n"
                            "end;\n").arg(tst_outValues)));
    QVERIFY(q.prepare(QLatin1String("call %1(?, ?)").arg(tst_outValues)));
    q.addBindValue(u"fifi"_s, QSql::In);
    QString out;
    out.reserve(50);
    q.addBindValue(out, QSql::Out);
    QVERIFY_SQL(q, exec());
    QCOMPARE(q.boundValue(1).toString(), u"fifibubulalakikikokololo");

    /*** in/outvalue date ***/
    QVERIFY_SQL(q, exec(QLatin1String(
                            "create or replace procedure %1(x in date, y out date) is\n"
                            "begin\n"
                            "    y := x;\n"
                            "end;\n").arg(tst_outValues)));
    QVERIFY(q.prepare(QLatin1String("call %1(?, ?)").arg(tst_outValues)));
    const QDate date = QDate::currentDate();
    q.addBindValue(date, QSql::In);
    q.addBindValue(QVariant(QDate()), QSql::Out);
    QVERIFY_SQL(q, exec());
    QCOMPARE(q.boundValue(1).toDate(), date);

    /*** in/outvalue timestamp ***/
    QVERIFY_SQL(q, exec(QLatin1String(
                            "create or replace procedure %1(x in timestamp, y out timestamp) is\n"
                            "begin\n"
                            "    y := x;\n"
                            "end;\n").arg(tst_outValues)));
    QVERIFY(q.prepare(QLatin1String("call %1(?, ?)").arg(tst_outValues)));
    const QDateTime dt = QDateTime::currentDateTime();
    q.addBindValue(dt, QSql::In);
    q.addBindValue(QVariant(QMetaType(QMetaType::QDateTime)), QSql::Out);
    QVERIFY_SQL(q, exec());
    QCOMPARE(q.boundValue(1).toDateTime(), dt);
}

void tst_QSqlQuery::oraClob()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "clobby", __FILE__);
    const auto &clobby = ts.tableName();

    QSqlQuery q(db);

    // Simple short string:
    QVERIFY_SQL(q, exec(QLatin1String("create table %1(id int primary key, cl clob, bl blob)")
                        .arg(clobby)));
    QVERIFY_SQL(q, prepare(QLatin1String("insert into %1 (id, cl, bl) values(?, ?, ?)")
                           .arg(clobby)));
    q.addBindValue(1);
    q.addBindValue("bubu");
    q.addBindValue("bubu"_ba);
    QVERIFY_SQL(q, exec());

    QVERIFY_SQL(q, exec(QLatin1String("select bl, cl from %1 where id = 1").arg(clobby)));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toString(), u"bubu");
    QCOMPARE(q.value(1).toString(), u"bubu");

    // Simple short string with binding:
    QVERIFY_SQL(q, prepare(QLatin1String("insert into %1 (id, cl, bl) values(?, ?, ?)")
                           .arg(clobby)));
    q.addBindValue(2);
    q.addBindValue(u"lala"_s, QSql::Binary);
    q.addBindValue("lala"_ba, QSql::Binary);
    QVERIFY_SQL(q, exec());

    QVERIFY_SQL(q, exec(QLatin1String("select bl, cl from %1 where id = 2").arg(clobby)));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toString(), u"lala");
    QCOMPARE(q.value(1).toString(), u"lala");

    // Loooong string:
    const QString loong(25000, QLatin1Char('A'));
    QVERIFY_SQL(q, prepare(QLatin1String("insert into %1 (id, cl, bl) values(?, ?, ?)")
                           .arg(clobby)));
    q.addBindValue(3);
    q.addBindValue(loong, QSql::Binary);
    q.addBindValue(loong.toLatin1(), QSql::Binary);
    QVERIFY_SQL(q, exec());

    QVERIFY_SQL(q, exec(QLatin1String("select bl, cl from %1 where id = 3").arg(clobby)));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toString().size(), loong.size());
    QVERIFY(q.value(0).toString() == loong); // Deliberately not QCOMPARE() as too long
    QCOMPARE(q.value(1).toByteArray().size(), loong.toLatin1().size());
    QVERIFY(q.value(1).toByteArray() == loong.toLatin1()); // ditto
}

void tst_QSqlQuery::oraClobBatch()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QString clobBatch(qTableName("clobBatch", __FILE__, db));
    tst_Databases::safeDropTables(db, { clobBatch });
    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QLatin1String("create table %1(cl clob)").arg(clobBatch)));

    const QString longString(USHRT_MAX + 1, QLatin1Char('A'));
    QVERIFY_SQL(q, prepare(QLatin1String("insert into %1 (cl) values(:cl)").arg(clobBatch)));
    const QVariantList vars = { longString };
    q.addBindValue(vars);
    QVERIFY_SQL(q, execBatch());

    QVERIFY_SQL(q, exec("select cl from " + clobBatch));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toString().size(), longString.size());
    QVERIFY(q.value(0).toString() == longString); // As above. deliberately not QCOMPARE().
}

void tst_QSqlQuery::storedProceduresIBase()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    ProcScope ps(db, "TESTPROC", __FILE__);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QLatin1String("create procedure %1 RETURNS (x integer, y varchar(20)) "
                                      "AS BEGIN "
                                      "  x = 42; "
                                      "  y = 'Hello Anders'; "
                                      "END").arg(ps.name())));
    QVERIFY_SQL(q, prepare("execute procedure " + ps.name()));
    QVERIFY_SQL(q, exec());

    // Check for a valid result set:
    QSqlRecord rec = q.record();
    QCOMPARE(rec.count(), 2);
    QCOMPARE(rec.fieldName(0).toUpper(), u"X");
    QCOMPARE(rec.fieldName(1).toUpper(), u"Y");

    // The first next shall suceed:
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 42);
    QCOMPARE(q.value(1).toString(), u"Hello Anders");

    // The second next shall fail:
    QVERIFY(!q.next());
}

void tst_QSqlQuery::outValuesDB2()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (!db.driver()->hasFeature(QSqlDriver::PreparedQueries))
        QSKIP("Test requires prepared query support");

    ProcScope ps(db, "tst_outValues", __FILE__);

    QSqlQuery q(db);
    q.setForwardOnly(true);

    QVERIFY_SQL(q, exec(QLatin1String("CREATE PROCEDURE %1 "
                                      "(OUT x int, OUT x2 double, OUT x3 char(20))\n"
                                      "LANGUAGE SQL\n"
                                      "P1: BEGIN\n"
                                      " SET x = 42;\n"
                                      " SET x2 = 4.2;\n"
                                      " SET x3 = 'Homer';\n"
                                      "END P1").arg(ps.name())));

    QVERIFY_SQL(q, prepare(QLatin1String("call %1(?, ?, ?)").arg(ps.name())));

    q.addBindValue(0, QSql::Out);
    q.addBindValue(0.0, QSql::Out);
    q.addBindValue("Simpson", QSql::Out);

    QVERIFY_SQL(q, exec());

    QCOMPARE(q.boundValue(0).toInt(), 42);
    QCOMPARE(q.boundValue(1).toDouble(), 4.2);
    QCOMPARE(q.boundValue(2).toString().trimmed(), u"Homer");
}

void tst_QSqlQuery::outValues()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (!db.driver()->hasFeature(QSqlDriver::PreparedQueries))
        QSKIP("Test requires prepared query support");

    ProcScope ps(db, "tst_outValues", __FILE__);
    QSqlQuery q(db);

    q.setForwardOnly(true);
    QLatin1String creator, caller;
    switch (tst_Databases::getDatabaseType(db)) {
    case QSqlDriver::Oracle:
        creator = QLatin1String("create or replace procedure %1(x out int) is\n"
                                "begin\n"
                                "    x := 42;\n"
                                "end;\n");
        caller = QLatin1String("call %1(?)");
        break;
    case QSqlDriver::DB2:
        creator = QLatin1String("CREATE PROCEDURE %1 (OUT x int)\n"
                                "LANGUAGE SQL\n"
                                "P1: BEGIN\n"
                                " SET x = 42;\n"
                                "END P1");
        caller = QLatin1String("call %1(?)");
        break;
    case QSqlDriver::MSSqlServer:
        creator = QLatin1String("create procedure %1 (@x int out) as\n"
                                "begin\n"
                                "    set @x = 42\n"
                                "end\n");
        caller = QLatin1String("{call %1(?)}");
        break;
    default:
        QSKIP("Don't know how to create a stored procedure for this database server, "
              "please fix this test");
    }
    QVERIFY_SQL(q, exec(creator.arg(ps.name())));
    QVERIFY(q.prepare(caller.arg(ps.name())));

    q.addBindValue(0, QSql::Out);

    QVERIFY_SQL(q, exec());

    QCOMPARE(q.boundValue(0).toInt(), 42);
}

void tst_QSqlQuery::blob()
{
    constexpr int BLOBSIZE = 1024 * 10;
    constexpr int BLOBCOUNT = 2;

    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (!db.driver()->hasFeature(QSqlDriver::BLOB))
        QSKIP("DBMS not BLOB capable");

    // Don't make it too big otherwise sybase and mysql will complain:
    QByteArray ba(BLOBSIZE, Qt::Uninitialized);
    for (int i = 0; i < ba.size(); ++i)
        ba[i] = i % 256;

    TableScope ts(db, "qtest_blob", __FILE__);
    QSqlQuery q(db);
    q.setForwardOnly(true);

    QVERIFY_SQL(q, exec(QLatin1String("create table %1 (id int not null primary key, t_blob %2)")
                        .arg(ts.tableName(), tst_Databases::blobTypeName(db, BLOBSIZE))));

    QVERIFY_SQL(q, prepare(QLatin1String("insert into %1 (id, t_blob) values (?, ?)")
                           .arg(ts.tableName())));

    for (int i = 0; i < BLOBCOUNT; ++i) {
        q.addBindValue(i);
        q.addBindValue(ba);
        QVERIFY_SQL(q, exec());
    }

    QVERIFY_SQL(q, exec("select * from " + ts.tableName()));

    for (int i = 0; i < BLOBCOUNT; ++i) {
        QVERIFY(q.next());
        QByteArray res = q.value(1).toByteArray();
        QVERIFY2(res.size() >= ba.size(),
                 qPrintable(QString::asprintf(
                                "array sizes differ, expected (at least) %" PRIdQSIZETYPE
                                ", got %" PRIdQSIZETYPE, ba.size(), res.size())));

        for (qsizetype i2 = 0; i2 < ba.size(); ++i2) {
            if (res[i2] != ba[i2]) {
                QFAIL(qPrintable(QString::asprintf(
                                     "ByteArrays differ at position %lld, expected %hhu, got %hhu",
                                     i2, ba[i2], res[i2])));
            }
        }
    }
}

void tst_QSqlQuery::value()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QLatin1String("select id, t_varchar, t_char from %1 order by id")
                        .arg(qtest)));

    for (int i = 1; q.next(); ++i) {
        QCOMPARE(q.value(0).toInt(), i);
        QCOMPARE(q.value("id").toInt(), i);
        auto istring = QString::number(i);

        if (dbType == QSqlDriver::Interbase)
            QVERIFY(q.value(1).toString().startsWith("VarChar" + istring));
        else if (q.value(1).toString().endsWith(QLatin1Char(' ')))
            QCOMPARE(q.value(1).toString(), QLatin1String("VarChar%1            ").arg(istring));
        else
            QCOMPARE(q.value(1).toString(), "VarChar" + istring);

        if (dbType == QSqlDriver::Interbase)
            QVERIFY(q.value(2).toString().startsWith("Char" + istring));
        else if (q.value(2).toString().endsWith(QLatin1Char(' ')))
            QCOMPARE(q.value(2).toString(), QLatin1String("Char%1               ").arg(istring));
        else
            QCOMPARE(q.value(2).toString(), "Char" + istring);
    }
}

#define SETUP_RECORD_TABLE \
    do { \
        QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %1 (id integer, extra varchar(50))") \
                            .arg(tst_record))); \
        for (int i = 0; i < 3; ++i) \
            QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 VALUES(%2, 'extra%2')") \
                                .arg(tst_record).arg(i))); \
    } while (0)

#define CHECK_RECORD \
    do { \
        QVERIFY_SQL(q, exec(QLatin1String( \
                                "select %1.id, %1.t_varchar, %1.t_char, %2.id, %2.extra " \
                                "from %1, %2 where %1.id = %2.id order by %1.id") \
                            .arg(lowerQTest, tst_record))); \
        QCOMPARE(q.record().fieldName(0).toLower(), u"id"); \
        QCOMPARE(q.record().field(0).tableName().toLower(), lowerQTest); \
        QCOMPARE(q.record().fieldName(1).toLower(), u"t_varchar"); \
        QCOMPARE(q.record().field(1).tableName().toLower(), lowerQTest); \
        QCOMPARE(q.record().fieldName(2).toLower(), u"t_char"); \
        QCOMPARE(q.record().field(2).tableName().toLower(), lowerQTest); \
        QCOMPARE(q.record().fieldName(3).toLower(), u"id"); \
        QCOMPARE(q.record().field(3).tableName().toLower(), tst_record); \
        QCOMPARE(q.record().fieldName(4).toLower(), u"extra"); \
        QCOMPARE(q.record().field(4).tableName().toLower(), tst_record); \
    } while (0)

void tst_QSqlQuery::record()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QVERIFY(q.record().isEmpty());
    QVERIFY_SQL(q, exec(QLatin1String("select id, t_varchar, t_char from %1 order by id")
                        .arg(qtest)));
    QCOMPARE(q.record().fieldName(0).toLower(), u"id");
    QCOMPARE(q.record().fieldName(1).toLower(), u"t_varchar");
    QCOMPARE(q.record().fieldName(2).toLower(), u"t_char");
    QCOMPARE(q.record().value(0), QVariant(q.record().field(0).metaType()));
    QCOMPARE(q.record().value(1), QVariant(q.record().field(1).metaType()));
    QCOMPARE(q.record().value(2), QVariant(q.record().field(2).metaType()));

    QVERIFY(q.next());
    QVERIFY(q.next());
    QCOMPARE(q.record().fieldName(0).toLower(), u"id");
    QCOMPARE(q.value(0).toInt(), 2);

    if (tst_Databases::getDatabaseType(db) == QSqlDriver::Oracle
        || tst_Databases::getDatabaseType(db) == QSqlDriver::MimerSQL)
        QSKIP("Getting the tablename is not supported in Oracle and Mimer SQL");

    const auto lowerQTest = qtest.toLower();
    for (int i = 0; i < 3; ++i)
        QCOMPARE(q.record().field(i).tableName().toLower(), lowerQTest);
    q.clear();
    const auto tst_record = qTableName("tst_record", __FILE__, db, false).toLower();
    SETUP_RECORD_TABLE;
    CHECK_RECORD;
    q.clear();

    // Recreate the tables, in a different order:
    const QStringList tables = { qtest, tst_record, qTableName("qtest_null", __FILE__, db) };
    tst_Databases::safeDropTables(db, tables);
    SETUP_RECORD_TABLE;
    createTestTables(db);
    populateTestTables(db);
    CHECK_RECORD;
}

void tst_QSqlQuery::isValid()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QVERIFY(!q.isValid());
    QVERIFY_SQL(q, exec("select * from " + qtest));
    QVERIFY(q.first());
    QVERIFY(q.isValid());
}

void tst_QSqlQuery::isActive()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QVERIFY(!q.isActive());
    QVERIFY_SQL(q, exec("select * from " + qtest));
    QVERIFY(q.isActive());
    QVERIFY(q.last());

    // Access is stupid enough to let you scroll over boundaries:
    if (!tst_Databases::isMSAccess(db))
        QVERIFY(!q.next());
    QVERIFY(q.isActive());

    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (41, 'VarChar41', 'Char41')")
                        .arg(qtest)));
    QVERIFY(q.isActive());

    QVERIFY_SQL(q, exec(QLatin1String("update %1 set id = 42 where id = 41").arg(qtest)));
    QVERIFY(q.isActive());

    QVERIFY_SQL(q, exec(QLatin1String("delete from %1 where id = 42").arg(qtest)));
    QVERIFY(q.isActive());

    QVERIFY_SQL(q, exec(QLatin1String("delete from %1 where id = 42").arg(qtest)));
    QVERIFY(q.isActive());
}

void tst_QSqlQuery::numRowsAffected()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QCOMPARE(q.numRowsAffected(), -1);

    QVERIFY_SQL(q, exec("select * from " + qtest));

    int i = 0;
    while (q.next())
        ++i;

    if (q.numRowsAffected() == -1 || q.numRowsAffected() == 0)
        QSKIP("Database doesn't support numRowsAffected");

    // Value is undefined for SELECT, this check is just here for curiosity:
    if (q.numRowsAffected() != -1 && q.numRowsAffected() != 0 && q.numRowsAffected() != i)
        qDebug("Expected numRowsAffected to be -1, 0 or %d, got %d", i, q.numRowsAffected());

    QVERIFY_SQL(q, exec(QLatin1String("update %1 set id = 100 where id = 1").arg(qtest)));

    QCOMPARE(q.numRowsAffected(), 1);
    QCOMPARE(q.numRowsAffected(), 1); // yes, we check twice

    QVERIFY_SQL(q, exec(QLatin1String("update %1 set id = id + 100").arg(qtest)));
    QCOMPARE(q.numRowsAffected(), i);
    QCOMPARE(q.numRowsAffected(), i); // yes, we check twice

    QVERIFY_SQL(q, prepare(QLatin1String("update %1 set id = id + :newid").arg(qtest)));
    q.bindValue(":newid", 100);
    QVERIFY_SQL(q, exec());
    QCOMPARE(q.numRowsAffected(), i);
    QCOMPARE(q.numRowsAffected(), i); // yes, we check twice

    QVERIFY_SQL(q, prepare(QLatin1String("update %1 set id = id + :newid where NOT(1 = 1)")
                           .arg(qtest)));
    q.bindValue(":newid", 100);
    QVERIFY_SQL(q, exec());
    QCOMPARE(q.numRowsAffected(), 0);
    QCOMPARE(q.numRowsAffected(), 0); // yes, we check twice

    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (42000, 'homer', 'marge')")
                        .arg(qtest)));
    QCOMPARE(q.numRowsAffected(), 1);
    QCOMPARE(q.numRowsAffected(), 1); // yes, we check twice

    QSqlQuery q2(db);
    QVERIFY_SQL(q2, exec(QLatin1String("insert into %1 values (42001, 'homer', 'marge')")
                         .arg(qtest)));
    QCOMPARE(q2.numRowsAffected(), 1);
    QCOMPARE(q2.numRowsAffected(), 1); // yes, we check twice
}

void tst_QSqlQuery::size()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QCOMPARE(q.size(), -1);

    QVERIFY_SQL(q, exec("select * from " + qtest));

    int i = 0;
    while (q.next())
        ++i;

    if (db.driver()->hasFeature(QSqlDriver::QuerySize)) {
        QCOMPARE(q.size(), i);
        QCOMPARE(q.size(), i); // yes, twice
    } else {
        QCOMPARE(q.size(), -1);
        QCOMPARE(q.size(), -1); // yes, twice
    }

    QSqlQuery q2("select * from " + qtest, db);

    if (db.driver()->hasFeature(QSqlDriver::QuerySize))
        QCOMPARE(q.size(), i);
    else
        QCOMPARE(q.size(), -1);

    q2.clear();

    QVERIFY_SQL(q, exec(QLatin1String("update %1 set id = 100 where id = 1").arg(qtest)));
    QCOMPARE(q.size(), -1);
    QCOMPARE(q.size(), -1); // yes, twice
}

void tst_QSqlQuery::isSelect()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec("select * from " + qtest));
    QVERIFY(q.isSelect());

    QVERIFY_SQL(q, exec(QLatin1String("update %1 set id = 1 where id = 1").arg(qtest)));
    QVERIFY(!q.isSelect());
}

void tst_QSqlQuery::first()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY_SQL(q, exec("select * from " + qtest));
    QVERIFY(q.last());
    QVERIFY_SQL(q, first());
    QCOMPARE(q.at(), 0);
}

void tst_QSqlQuery::next()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY_SQL(q, exec("select * from " + qtest));
    QVERIFY(q.first());
    QVERIFY(q.next());
    QCOMPARE(q.at(), 1);
}

void tst_QSqlQuery::prev()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY_SQL(q, exec("select * from " + qtest));
    QVERIFY(q.first());
    QVERIFY(q.next());
    QVERIFY(q.previous());
    QCOMPARE(q.at(), 0);
}

void tst_QSqlQuery::last()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY_SQL(q, exec("select * from " + qtest));

    int i = 0;
    while (q.next())
        ++i;

    QCOMPARE(q.at(), QSql::AfterLastRow);
    QVERIFY(q.last());
    QSet<int> validReturns(QSet<int>() << -1 << i - 1);
    QVERIFY(validReturns.contains(q.at()));

    QSqlQuery q2("select * from " + qtest, db);
    QVERIFY(q2.last());
    QVERIFY(validReturns.contains(q.at()));
}

void tst_QSqlQuery::seek()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    QSqlQuery q(db);
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY_SQL(q, exec(QLatin1String("select id from %1 order by id").arg(qtest)));

    // NB! The order of the calls below are important!
    QVERIFY(q.last());
    QVERIFY(!q.seek(QSql::BeforeFirstRow));
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY(q.seek(0));
    QCOMPARE(q.at(), 0);
    QCOMPARE(q.value(0).toInt(), 1);

    QVERIFY(q.seek(1));
    QCOMPARE(q.at(), 1);
    QCOMPARE(q.value(0).toInt(), 2);

    QVERIFY(q.seek(3));
    QCOMPARE(q.at(), 3);
    QCOMPARE(q.value(0).toInt(), 4);

    QVERIFY(q.seek(-2, true));
    QCOMPARE(q.at(), 1);
    QVERIFY(q.seek(0));
    QCOMPARE(q.at(), 0);
    QCOMPARE(q.value(0).toInt(), 1);

    QVERIFY(!q.seek(QSql::BeforeFirstRow));
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY(q.seek(1, true));
    QCOMPARE(q.at(), 0);
    QCOMPARE(q.value(0).toInt(), 1);

    qint32 count = 1;
    while (q.next())
        ++count;

    QCOMPARE(q.at(), QSql::AfterLastRow);

    if (!q.isForwardOnly()) {
        QVERIFY(q.seek(-1, true));
        QCOMPARE(q.at(), count - 1);
        QCOMPARE(q.value(0).toInt(), count);
    } else {
        QVERIFY(!q.seek(-1, true));
        QCOMPARE(q.at(), QSql::AfterLastRow);
    }
}

void tst_QSqlQuery::seekForwardOnlyQuery()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    q.setForwardOnly(false);
    QVERIFY(!q.isForwardOnly());

    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY_SQL(q, exec(QLatin1String("select id from %1 order by id").arg(qtest)));

    QSqlRecord rec;

    // NB! The order of the calls below are important!
    QVERIFY(q.seek(0));
    QCOMPARE(q.at(), 0);
    rec = q.record();
    QCOMPARE(rec.value(0).toInt(), 1);

    QVERIFY(q.seek(1));
    QCOMPARE(q.at(), 1);
    rec = q.record();
    QCOMPARE(rec.value(0).toInt(), 2);

    // Make a jump!
    QVERIFY(q.seek(3));
    QCOMPARE(q.at(), 3);
    rec = q.record();
    QCOMPARE(rec.value(0).toInt(), 4);

    // Last record in result set
    QVERIFY(q.seek(4));
    QCOMPARE(q.at(), 4);
    rec = q.record();
    QCOMPARE(rec.value(0).toInt(), 5);
}

void tst_QSqlQuery::forwardOnly()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    q.setForwardOnly(true);
    QVERIFY(q.isForwardOnly());
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY_SQL(q, exec(QLatin1String("select * from %1 order by id").arg(qtest)));
    if (!q.isForwardOnly())
        QSKIP("DBMS doesn't support forward-only queries");
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY(q.first());
    QCOMPARE(q.at(), 0);
    QCOMPARE(q.value(0).toInt(), 1);
    QVERIFY(q.next());
    QCOMPARE(q.at(), 1);
    QCOMPARE(q.value(0).toInt(), 2);
    QVERIFY(q.next());
    QCOMPARE(q.at(), 2);
    QCOMPARE(q.value(0).toInt(), 3);

    // Let's make some mistakes to see how robust it is:
    QTest::ignoreMessage(QtWarningMsg,
                         "QSqlQuery::seek: cannot seek backwards in a forward only query");
    QVERIFY(!q.first());
    QCOMPARE(q.at(), 2);
    QCOMPARE(q.value(0).toInt(), 3);
    QTest::ignoreMessage(QtWarningMsg,
                         "QSqlQuery::seek: cannot seek backwards in a forward only query");
    QVERIFY(!q.previous());
    QCOMPARE(q.at(), 2);
    QCOMPARE(q.value(0).toInt(), 3);
    QVERIFY(q.next());
    QCOMPARE(q.at(), 3);
    QCOMPARE(q.value(0).toInt(), 4);

    QVERIFY_SQL(q, exec("select * from " + qtest));

    int i = 0;
    while (q.next())
        ++i;

    QCOMPARE(q.at(), QSql::AfterLastRow);

    QVERIFY_SQL(q, exec(QLatin1String("select * from %1 order by id").arg(qtest)));
    QVERIFY(q.isForwardOnly());
    QCOMPARE(q.at(), QSql::BeforeFirstRow);

    QVERIFY_SQL(q, seek(3));
    QCOMPARE(q.at(), 3);
    QCOMPARE(q.value(0).toInt(), 4);

    QTest::ignoreMessage(QtWarningMsg,
                         "QSqlQuery::seek: cannot seek backwards in a forward only query");
    QVERIFY(!q.seek(0));
    QCOMPARE(q.value(0).toInt(), 4);
    QCOMPARE(q.at(), 3);

    QVERIFY(q.last());
    QCOMPARE(q.at(), i - 1);

    QTest::ignoreMessage(QtWarningMsg,
                         "QSqlQuery::seek: cannot seek backwards in a forward only query");
    QVERIFY(!q.first());
    QCOMPARE(q.at(), i - 1);

    QVERIFY(!q.next());
    QCOMPARE(q.at(), QSql::AfterLastRow);
}

void tst_QSqlQuery::forwardOnlyMultipleResultSet()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (!db.driver()->hasFeature(QSqlDriver::MultipleResultSets))
        QSKIP("DBMS doesn't support multiple result sets");

    QSqlQuery q(db);
    q.setForwardOnly(true);
    QVERIFY_SQL(q, exec(QLatin1String(
                            "select id, t_varchar from %1 order by id;"         // 1.
                            "select id, t_varchar, t_char from %1 where id<4 order by id;"  // 2.
                            "update %1 set t_varchar='VarChar555' where id=5;"  // 3.
                            "select * from %1 order by id;"                     // 4.
                            "select * from %1 where id=5 order by id;"          // 5.
                            "select * from %1 where id=-1 order by id;"         // 6.
                            "select * from %1 order by id")                     // 7.
                        .arg(qtest)));

    if (!q.isForwardOnly())
        QSKIP("DBMS doesn't support forward-only queries");

    // 1. Result set with 2 columns and 5 rows
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY(q.isActive());
    QVERIFY(q.isSelect());

    // Test record() of first result set
    QSqlRecord record = q.record();
    QCOMPARE(record.count(), 2);
    QCOMPARE(record.indexOf("id"), 0);
    QCOMPARE(record.indexOf("t_varchar"), 1);
    // tableName() is not available in forward-only mode of QPSQL
    if (tst_Databases::getDatabaseType(db) != QSqlDriver::PostgreSQL) {
        // BUG: This fails for Microsoft SQL Server 2016 (QODBC), need fix:
        QCOMPARE(record.field(0).tableName(), qtest);
        QCOMPARE(record.field(1).tableName(), qtest);
    }

    // Test navigation
    QVERIFY(q.first());
    QCOMPARE(q.at(), 0);
    QCOMPARE(q.value(0).toInt(), 1);

    QVERIFY(q.next());
    QCOMPARE(q.at(), 1);
    QCOMPARE(q.value(0).toInt(), 2);

    QVERIFY(q.seek(3));
    QCOMPARE(q.at(), 3);
    QCOMPARE(q.value(0).toInt(), 4);

    QTest::ignoreMessage(QtWarningMsg,
                         "QSqlQuery::seek: cannot seek backwards in a forward only query");
    QVERIFY(!q.first());
    QCOMPARE(q.at(), 3);
    QCOMPARE(q.value(0).toInt(), 4);

    QTest::ignoreMessage(QtWarningMsg,
                         "QSqlQuery::seek: cannot seek backwards in a forward only query");
    QVERIFY(!q.previous());
    QCOMPARE(q.at(), 3);
    QCOMPARE(q.value(0).toInt(), 4);

    QTest::ignoreMessage(QtWarningMsg,
                         "QSqlQuery::seek: cannot seek backwards in a forward only query");
    QVERIFY(!q.seek(1));
    QCOMPARE(q.at(), 3);
    QCOMPARE(q.value(0).toInt(), 4);

    QVERIFY(q.last());
    QCOMPARE(q.at(), 4);

    // Try move after last row
    QVERIFY(!q.next());
    QCOMPARE(q.at(), QSql::AfterLastRow);
    QVERIFY(q.isActive());

    // 2. Result set with 3 columns and 3 rows
    QVERIFY(q.nextResult());
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY(q.isActive());
    QVERIFY(q.isSelect());

    // Test record() of second result set
    record = q.record();
    QCOMPARE(record.count(), 3);
    QCOMPARE(record.indexOf("id"), 0);
    QCOMPARE(record.indexOf("t_varchar"), 1);
    QCOMPARE(record.indexOf("t_char"), 2);

    // Test iteration
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    int index = 0;
    while (q.next()) {
        QCOMPARE(q.at(), index);
        QCOMPARE(q.value(0).toInt(), index + 1);
        ++index;
    }
    QCOMPARE(q.at(), QSql::AfterLastRow);
    QCOMPARE(index, 3);

    // 3. Update statement
    QVERIFY(q.nextResult());
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY(q.isActive());
    QVERIFY(!q.isSelect());
    QCOMPARE(q.numRowsAffected(), 1);

    // 4. Result set with 5 rows
    QVERIFY(q.nextResult());
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY(q.isActive());
    QVERIFY(q.isSelect());

    // Test forward seek(n)
    QVERIFY(q.seek(2));
    QCOMPARE(q.at(), 2);
    QCOMPARE(q.value(0).toInt(), 3);

    // Test value(string)
    QCOMPARE(q.value("id").toInt(), 3);
    QCOMPARE(q.value("t_varchar").toString(), "VarChar3");
    QCOMPARE(q.value("t_char").toString().trimmed(), "Char3");

    // Next 2 rows of current result set will be
    // discarded by next call of nextResult()

    // 5. Result set with 1 row
    QVERIFY(q.nextResult());
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY(q.isActive());
    QVERIFY(q.isSelect());
    QVERIFY(q.first());
    QCOMPARE(q.at(), 0);
    QCOMPARE(q.value(0).toInt(), 5);
    QVERIFY(!q.next());
    QCOMPARE(q.at(), QSql::AfterLastRow);

    // 6. Result set without rows
    QVERIFY(q.nextResult());
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY(q.isActive());
    QVERIFY(q.isSelect());
    QVERIFY(!q.next());

    // 7. Result set with 5 rows
    QVERIFY(q.nextResult());
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY(q.isActive());
    QVERIFY(q.isSelect());

    // Just skip it, so we move after last result set.
    QVERIFY(!q.nextResult());
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY(!q.isActive());

    // See if we can execute another query
    QVERIFY_SQL(q, exec(QLatin1String("select id from %1 where id=5").arg(qtest)));
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY(q.isActive());
    QVERIFY(q.isSelect());
    QVERIFY(q.first());
    QCOMPARE(q.at(), 0);
    QCOMPARE(q.value("id").toInt(), 5);
    QCOMPARE(q.record().count(), 1);
    QCOMPARE(q.record().indexOf("id"), 0);
}

void tst_QSqlQuery::psql_forwardOnlyQueryResultsLost()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q1(db);
    q1.setForwardOnly(true);
    QVERIFY_SQL(q1, exec(QLatin1String("select * from %1 where id<=3 order by id").arg(qtest)));
    if (!q1.isForwardOnly())
        QSKIP("DBMS doesn't support forward-only queries");

    // Read first row of q1
    QVERIFY(q1.next());
    QCOMPARE(q1.at(), 0);
    QCOMPARE(q1.value(0).toInt(), 1);

    // Executing another query on the same db connection
    // will cause the query results of q1 to be lost.
    QSqlQuery q2(db);
    q2.setForwardOnly(true);
    QVERIFY_SQL(q2, exec(QLatin1String("select * from %1 where id>3 order by id").arg(qtest)));

    QTest::ignoreMessage(QtWarningMsg, "QPSQLDriver::getResult: Query results lost - "
                                       "probably discarded on executing another SQL query.");

    // Reading next row of q1 will not possible.
    QVERIFY(!q1.next());
    QCOMPARE(q1.at(), QSql::AfterLastRow);
    QVERIFY(q1.lastError().type() != QSqlError::NoError);

    // See if we can read rows from q2
    QVERIFY(q2.seek(1));
    QCOMPARE(q2.at(), 1);
    QCOMPARE(q2.value(0).toInt(), 5);
}

void tst_QSqlQuery::query_exec()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QVERIFY(!q.isValid());
    QVERIFY(!q.isActive());
    QVERIFY_SQL(q, exec("select * from " + qtest));
    QVERIFY(q.isActive());
    QVERIFY(q.next());
    QVERIFY(q.isValid());
}

void tst_QSqlQuery::isNull()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QLatin1String("select id, t_varchar from %1 order by id")
                        .arg(qTableName("qtest_null", __FILE__, db))));
    QVERIFY(q.next());
    QVERIFY(!q.isNull(0));
    QVERIFY(!q.isNull("id"));
    QVERIFY(q.isNull(1));
    QVERIFY(q.isNull("t_varchar"));
    QCOMPARE(q.value(0).toInt(), 0);
    QCOMPARE(q.value(1).toString(), QString());
    QVERIFY(!q.value(0).isNull());
    QVERIFY(q.value(1).isNull());

    QVERIFY(q.next());
    QVERIFY(!q.isNull(0));
    QVERIFY(!q.isNull("id"));
    QVERIFY(!q.isNull(1));
    QVERIFY(!q.isNull("t_varchar"));

    // For a non existent field, it should be returning true.
    QVERIFY(q.isNull(2));
    QTest::ignoreMessage(QtWarningMsg, "QSqlQuery::isNull: unknown field name 'unknown'");
    QVERIFY(q.isNull("unknown"));
}

void tst_QSqlQuery::writeNull()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);

    // The test data table is already used, so use a local hash to exercise the various
    // cases from the QSqlResultPrivate::isVariantNull helper. Only PostgreSQL and Mimer SQL
    // supports QUuid.
    QMultiHash<QString, QVariant> nullableTypes = {
        { "varchar(20)", u"not null"_s },
        { "varchar(20)", "not null"_ba },
        { tst_Databases::dateTimeTypeName(db), QDateTime::currentDateTime() },
        { tst_Databases::dateTypeName(db), QDate::currentDate() },
        { tst_Databases::timeTypeName(db), QTime::currentTime() },
    };
    if (dbType == QSqlDriver::PostgreSQL)
        nullableTypes["uuid"] = QUuid::createUuid();
    if (dbType == QSqlDriver::MimerSQL)
        nullableTypes["builtin.uuid"] = QUuid::createUuid();

    // Helper to count rows with null values in the data column.
    // Since QSqlDriver::QuerySize might not be supported, we have to count anyway
    const auto countRowsWithNull = [&](QSqlQuery &q, const QString &tableName){
        q.exec(QLatin1String("select id, data from %1 where data is null").arg(tableName));
        int size = 0;
        while (q.next())
            ++size;
        return size;
    };

    for (const auto &nullableType : nullableTypes.keys()) {
        TableScope ts(db, "qtest_writenull", __FILE__);
        QSqlQuery q(db);

        const QVariant nonNullValue = nullableTypes.value(nullableType);
        // some useful diagnostic output in case of any test failure
        auto errorHandler = qScopeGuard([&]{
            qWarning() << "Test failure for data type" << nonNullValue.metaType().name();
            q.exec("select id, data from " + ts.tableName());
            while (q.next())
                qWarning() << q.value(0) << q.value(1);
        });
        QString createQuery = QLatin1String("create table %3 (id int, data %1%2)")
            .arg(nullableType,
                 dbType == QSqlDriver::MSSqlServer || dbType == QSqlDriver::Sybase ? " null" : "",
                 ts.tableName());
        QVERIFY_SQL(q, exec(createQuery));

        int expectedNullCount = 0;
        // Verify that inserting a non-null value works:
        QVERIFY_SQL(q, prepare(QLatin1String("insert into %1 values(:id, :data)").arg(ts.tableName())));
        q.bindValue(":id", expectedNullCount);
        q.bindValue(":data", nonNullValue);
        QVERIFY_SQL(q, exec());
        QCOMPARE(countRowsWithNull(q, ts.tableName()), expectedNullCount);

        // Verify that inserting using a null QVariant produces a null entry in the database:
        QVERIFY_SQL(q, prepare(QLatin1String("insert into %1 values(:id, :data)").arg(ts.tableName())));
        q.bindValue(":id", ++expectedNullCount);
        q.bindValue(":data", QVariant());
        QVERIFY_SQL(q, exec());
        QCOMPARE(countRowsWithNull(q, ts.tableName()), expectedNullCount);

        // Verify that writing a null-value (but not a null-variant) produces a
        // null entry in the database:
        const QMetaType nullableMetaType = nullableTypes.value(nullableType).metaType();
        // Creating a QVariant with meta type and nullptr does create a null-QVariant. We want
        // to explicitly create a non-null variant, so we have to pass in a default-constructed
        // value as well (and make sure that the default value is also destroyed again,
        // which is clumsy to do using std::unique_ptr with a custom deleter, so use another
        // scope guard).
        void *defaultData = nullableMetaType.create();
        const auto defaultTypeDeleter = qScopeGuard([&]{ nullableMetaType.destroy(defaultData); });
        const QVariant nullValueVariant(nullableMetaType, defaultData);
        QVERIFY(!nullValueVariant.isNull());

        QVERIFY_SQL(q, prepare(QLatin1String("insert into %1 values(:id, :data)").arg(ts.tableName())));
        q.bindValue(":id", ++expectedNullCount);
        q.bindValue(":data", nullValueVariant);
        QVERIFY_SQL(q, exec());
        QCOMPARE(countRowsWithNull(q, ts.tableName()), expectedNullCount);

        // All tests passed for this type if we got here, so don't print diagnostics:
        errorHandler.dismiss();
    }
}

// Oracle-specific NULL BLOB test:
void tst_QSqlQuery::oci_nullBlob()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "qtest_nullblob", __FILE__);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QLatin1String("create table %1 (id int primary key, bb blob)")
                        .arg(ts.tableName())));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (0, EMPTY_BLOB())")
                        .arg(ts.tableName())));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (1, NULL)").arg(ts.tableName())));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (2, 'aabbcc00112233445566')")
                        .arg(ts.tableName())));
    // Necessary otherwise Oracle will bombard you with internal errors:
    q.setForwardOnly(true);
    QVERIFY_SQL(q, exec(QLatin1String("select * from %1 order by id").arg(ts.tableName())));

    QVERIFY(q.next());
    QVERIFY(q.value(1).toByteArray().isEmpty());
    QVERIFY(!q.isNull(1));

    QVERIFY(q.next());
    QVERIFY(q.value(1).toByteArray().isEmpty());
    QVERIFY(q.isNull(1));

    QVERIFY(q.next());
    QCOMPARE(q.value(1).toByteArray().size(), qsizetype(10));
    QVERIFY(!q.isNull(1));
}

/* Oracle-specific RAW field test */
void tst_QSqlQuery::oci_rawField()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "qtest_rawtest", __FILE__);

    QSqlQuery q(db);
    q.setForwardOnly(true);
    QVERIFY_SQL(q, exec(QLatin1String("create table %1 (id int, col raw(20))").arg(ts.tableName())));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (0, NULL)").arg(ts.tableName())));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (1, '00aa1100ddeeff')")
                        .arg(ts.tableName())));
    QVERIFY_SQL(q, exec(QLatin1String("select col from %1 order by id").arg(ts.tableName())));
    QVERIFY(q.next());
    QVERIFY(q.isNull(0));
    QVERIFY(q.value(0).toByteArray().isEmpty());
    QVERIFY(q.next());
    QVERIFY(!q.isNull(0));
    QCOMPARE(q.value(0).toByteArray().size(), qsizetype(7));
}

// Test whether we can fetch values with more than DOUBLE precision
// note that SQLite highest precision is that of a double, although
// you can define field with higher precision:
void tst_QSqlQuery::precision()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    if (dbType == QSqlDriver::Interbase)
        QSKIP("DB unable to store high precision");

    const auto tidier = qScopeGuard([db, oldPrecision = db.driver()->numericalPrecisionPolicy()]() {
        db.driver()->setNumericalPrecisionPolicy(oldPrecision);
    });

    db.driver()->setNumericalPrecisionPolicy(QSql::HighPrecision);
    TableScope ts(db, "qtest_precision", __FILE__);
    static const QLatin1String precStr("1.2345678901234567891");

    {
        // need a new scope for SQLITE
        QSqlQuery q(db);

        QVERIFY_SQL(q, exec(QLatin1String(tst_Databases::isMSAccess(db)
                                          ? "CREATE TABLE %1 (col1 number)"
                                          : "CREATE TABLE %1 (col1 numeric(21, 20))")
                            .arg(ts.tableName())));

        QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 (col1) VALUES (%2)")
                            .arg(ts.tableName(), precStr)));
        QVERIFY_SQL(q, exec("SELECT * FROM " + ts.tableName()));
        QVERIFY(q.next());
        const QString val = q.value(0).toString();
        if (!val.startsWith(precStr)) {
            int i = 0;
            while (i < val.size() && precStr[i] != 0 && precStr[i] == val[i].toLatin1())
                ++i;

            // TDS has crappy precisions by default
            if (dbType == QSqlDriver::Sybase) {
                if (i < 18)
                    qWarning("TDS didn't return the right precision");
            } else {
                qWarning() << tst_Databases::dbToString(db) << "didn't return the right precision ("
                    << i << "out of 21)," << val;
            }
        }
    } // SQLITE scope
}

void tst_QSqlQuery::nullResult()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QLatin1String("select * from %1 where id > 50000").arg(qtest)));

    if (q.driver()->hasFeature(QSqlDriver::QuerySize))
        QCOMPARE(q.size(), 0);

    QVERIFY(!q.next());
    QVERIFY(!q.first());
    QVERIFY(!q.last());
    QVERIFY(!q.previous());
    QVERIFY(!q.seek(10));
    QVERIFY(!q.seek(0));
}

void tst_QSqlQuery::joins()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);

    if (dbType == QSqlDriver::Oracle || dbType == QSqlDriver::Sybase
            || dbType == QSqlDriver::Interbase || db.driverName().startsWith("QODBC")) {
        // Oracle broken beyond recognition - cannot outer join on more than one table:
        QSKIP("DBMS cannot understand standard SQL");
    }
    TableScope j1(db, "qtestj1", __FILE__);
    TableScope j2(db, "qtestj2", __FILE__);

    QSqlQuery q(db);

    QVERIFY_SQL(q, exec(QLatin1String("create table %1 (id1 int, id2 int)").arg(j1.tableName())));
    QVERIFY_SQL(q, exec(QLatin1String("create table %1 (id int, name varchar(20))").arg(j2.tableName())));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (1, 1)").arg(j1.tableName())));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (1, 2)").arg(j1.tableName())));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values(1, 'trenton')").arg(j2.tableName())));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values(2, 'marius')").arg(j2.tableName())));

    QVERIFY_SQL(q, exec(QLatin1String(
                            "select qtestj1.id1, qtestj1.id2, qtestj2.id, qtestj2.name, "
                            "qtestj3.id, qtestj3.name from %1 qtestj1 left outer join %2 qtestj2 "
                            "on (qtestj1.id1 = qtestj2.id) left outer join %2 as qtestj3 "
                            "on (qtestj1.id2 = qtestj3.id)").arg(j1.tableName(), j2.tableName())));

    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 1);
    QCOMPARE(q.value(1).toInt(), 1);
    QCOMPARE(q.value(2).toInt(), 1);
    QCOMPARE(q.value(3).toString(), u"trenton");
    QCOMPARE(q.value(4).toInt(), 1);
    QCOMPARE(q.value(5).toString(), u"trenton");

    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 1);
    QCOMPARE(q.value(1).toInt(), 2);
    QCOMPARE(q.value(2).toInt(), 1);
    QCOMPARE(q.value(3).toString(), u"trenton");
    QCOMPARE(q.value(4).toInt(), 2);
    QCOMPARE(q.value(5).toString(), u"marius");
}

void tst_QSqlQuery::synonyms()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QLatin1String(
                            "select a.id, a.t_char, a.t_varchar from %1 a where a.id = 1")
                        .arg(qtest)));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 1);
    QCOMPARE(q.value(1).toString().trimmed(), u"Char1");
    QCOMPARE(q.value(2).toString().trimmed(), u"VarChar1");

    QSqlRecord rec = q.record();
    QCOMPARE(rec.count(), qsizetype(3));
    QCOMPARE(rec.field(0).name().toLower(), u"id");
    QCOMPARE(rec.field(1).name().toLower(), u"t_char");
    QCOMPARE(rec.field(2).name().toLower(), u"t_varchar");
}

// It doesn't make sense to split this into several tests
void tst_QSqlQuery::prepare_bind_exec()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    TableScope ts(db, "qtest_prepare", __FILE__);
    const auto &qtest_prepare = ts.tableName();

    if (dbType == QSqlDriver::DB2)
        QSKIP("Needs someone with more Unicode knowledge than I have to fix");

    { // New scope for SQLITE:
        static const QString utf8str = QString::fromUtf8("काचं शक्नोम्यत्तुम् । नोपहिनस्ति माम् ॥");

        static const QString values[6] = {
            u"Harry"_s, u"Trond"_s, u"Mark"_s, u"Ma?rk"_s, u"?"_s, u":id"_s
        };

        bool useUnicode = db.driver()->hasFeature(QSqlDriver::Unicode);

        QSqlQuery q(db);
        QLatin1String createQuery;
        QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
        if (dbType == QSqlDriver::PostgreSQL)
            QVERIFY_SQL(q, exec("set client_min_messages='warning'"));

        switch (dbType) {
        case QSqlDriver::MSSqlServer:
        case QSqlDriver::Sybase:
        case QSqlDriver::MimerSQL:
            createQuery = QLatin1String("create table %1 (id int primary key, "
                                        "name nvarchar(200) null, name2 nvarchar(200) null)");
            break;
        case QSqlDriver::MySqlServer:
            if (useUnicode) {
                createQuery = QLatin1String("create table %1 (id int not null primary key, "
                                            "name varchar(200) character set utf8, "
                                            "name2 varchar(200) character set utf8)");
                break;
            }
            Q_FALLTHROUGH();
        default:
            createQuery = QLatin1String("create table %1 (id int not null primary key, "
                                        "name varchar(200), name2 varchar(200))");
            break;
        }

        QVERIFY_SQL(q, exec(createQuery.arg(qtest_prepare)));
        QVERIFY(q.prepare(QLatin1String("insert into %1 (id, name) values (:id, :name)")
                          .arg(qtest_prepare)));
        for (int i = 0; i < 6; ++i) {
            q.bindValue(":name", values[i]);
            q.bindValue(":id", i);
            QVERIFY_SQL(q, exec());
            const QVariantList m = q.boundValues();
            QCOMPARE(m.size(), qsizetype(2));
            QCOMPARE(m.at(0).toInt(), i);
            QCOMPARE(m.at(1).toString(), values[i]);
            const QStringList n = q.boundValueNames();
            QCOMPARE(n.size(), 2);
            QCOMPARE(n.at(0), ":id");
            QCOMPARE(n.at(1), ":name");
            QCOMPARE(q.boundValueName(0), ":id");
            QCOMPARE(q.boundValueName(1), ":name");
        }

        q.bindValue(":id", 8);
        QVERIFY_SQL(q, exec());

        if (useUnicode) {
            q.bindValue(":id", 7);
            q.bindValue(":name", utf8str);
            QVERIFY_SQL(q, exec());
        }

        QVERIFY_SQL(q, exec(QLatin1String("SELECT * FROM %1 order by id").arg(qtest_prepare)));
        for (int i = 0; i < 6; ++i) {
            QVERIFY(q.next());
            QCOMPARE(q.value(0).toInt(), i);
            QCOMPARE(q.value(1).toString().trimmed(), values[i]);
        }

        if (useUnicode) {
            QVERIFY_SQL(q, next());
            QCOMPARE(q.value(0).toInt(), 7);
            QCOMPARE(q.value(1).toString(), utf8str);
        }

        QVERIFY_SQL(q, next());
        QCOMPARE(q.value(0).toInt(), 8);
        QCOMPARE(q.value(1).toString(), values[5]);

        QVERIFY(q.prepare(QLatin1String("insert into %1 (id, name) values (:id, 'Bart')")
                          .arg(qtest_prepare)));
        q.bindValue(":id", 99);
        QVERIFY_SQL(q, exec());
        q.bindValue(":id", 100);
        QVERIFY_SQL(q, exec());
        QVERIFY(q.exec(QLatin1String("select * from %1 where id > 98 order by id")
                       .arg(qtest_prepare)));

        for (int i = 99; i <= 100; ++i) {
            QVERIFY(q.next());
            QCOMPARE(q.value(0).toInt(), i);
            QCOMPARE(q.value(1).toString().trimmed(), u"Bart");
        }

        /*** SELECT stuff ***/
        QVERIFY(q.prepare(QLatin1String("select * from %1 where id = :id").arg(qtest_prepare)));

        for (int i = 0; i < 6; ++i) {
            q.bindValue(":id", i);
            QVERIFY_SQL(q, exec());
            QVERIFY_SQL(q, next());
            QCOMPARE(q.value(0).toInt(), i);
            QCOMPARE(q.value(1).toString().trimmed(), values[i]);
            QSqlRecord rInf = q.record();
            QCOMPARE(rInf.count(), qsizetype(3));
            QCOMPARE(rInf.field(0).name().toUpper(), u"ID");
            QCOMPARE(rInf.field(1).name().toUpper(), u"NAME");
            QVERIFY(!q.next());
        }

        QVERIFY_SQL(q, exec("DELETE FROM " + qtest_prepare));

        /* Below we test QSqlQuery::boundValues() with position arguments.
           Due to the fact that the name of a positional argument is not
           specified by the Qt docs, we test that the QList contains the correct
           values in the same order as QSqlResult::boundValueName returns since
           it should be in insertion order (i.e. field order). */
        QVERIFY(q.prepare(QLatin1String("insert into %1 (id, name) values (?, ?)")
                          .arg(qtest_prepare)));
        q.bindValue(0, 0);
        q.bindValue(1, values[0]);
        QCOMPARE(q.boundValues().size(), 2);
        QCOMPARE(q.boundValues().at(0).toInt(), 0);
        QCOMPARE(q.boundValues().at(1).toString(), values[0]);
        QVERIFY_SQL(q, exec());
        QCOMPARE(q.boundValues().size(), 2);
        QCOMPARE(q.boundValues().at(0).toInt(), 0);
        QCOMPARE(q.boundValues().at(1).toString(), values[0]);

        q.addBindValue(1);
        q.addBindValue(values[1]);
        QCOMPARE(q.boundValues().size(), 2);
        QCOMPARE(q.boundValues().at(0).toInt(), 1);
        QCOMPARE(q.boundValues().at(1).toString(), values[1]);
        QVERIFY_SQL(q, exec());
        QCOMPARE(q.boundValues().size(), 2);
        QCOMPARE(q.boundValues().at(0).toInt(), 1);
        QCOMPARE(q.boundValues().at(1).toString(), values[1]);

        q.addBindValue(2);
        q.addBindValue(values[2]);
        QCOMPARE(q.boundValues().size(), 2);
        QCOMPARE(q.boundValues().at(0).toInt(), 2);
        QCOMPARE(q.boundValues().at(1).toString(), values[2]);
        QVERIFY_SQL(q, exec());
        QCOMPARE(q.boundValues().size(), 2);
        QCOMPARE(q.boundValues().at(0).toInt(), 2);
        QCOMPARE(q.boundValues().at(1).toString(), values[2]);

        q.addBindValue(3);
        q.addBindValue(values[3]);
        QCOMPARE(q.boundValues().size(), 2);
        QCOMPARE(q.boundValues().at(0).toInt(), 3);
        QCOMPARE(q.boundValues().at(1).toString(), values[3]);
        QVERIFY_SQL(q, exec());
        QCOMPARE(q.boundValues().size(), 2);
        QCOMPARE(q.boundValues().at(0).toInt(), 3);
        QCOMPARE(q.boundValues().at(1).toString(), values[3]);

        q.addBindValue(4);
        q.addBindValue(values[4]);
        QCOMPARE(q.boundValues().size(), 2);
        QCOMPARE(q.boundValues().at(0).toInt(), 4);
        QCOMPARE(q.boundValues().at(1).toString(), values[4]);
        QVERIFY_SQL(q, exec());
        QCOMPARE(q.boundValues().size(), 2);
        QCOMPARE(q.boundValues().at(0).toInt(), 4);
        QCOMPARE(q.boundValues().at(1).toString(), values[4]);

        q.bindValue(1, values[5]);
        q.bindValue(0, 5);
        QCOMPARE(q.boundValues().size(), 2);
        QCOMPARE(q.boundValues().at(0).toInt(), 5);
        QCOMPARE(q.boundValues().at(1).toString(), values[5]);
        QVERIFY_SQL(q, exec());
        QCOMPARE(q.boundValues().size(), 2);
        QCOMPARE(q.boundValues().at(0).toInt(), 5);
        QCOMPARE(q.boundValues().at(1).toString(), values[5]);

        q.bindValue(0, 6);
        q.bindValue(1, QString());
        QCOMPARE(q.boundValues().size(), 2);
        QCOMPARE(q.boundValues().at(0).toInt(), 6);
        QCOMPARE(q.boundValues().at(1).toString(), QString());
        QVERIFY_SQL(q, exec());
        QCOMPARE(q.boundValues().size(), 2);
        QCOMPARE(q.boundValues().at(0).toInt(), 6);
        QCOMPARE(q.boundValues().at(1).toString(), QString());

        if (db.driver()->hasFeature(QSqlDriver::Unicode)) {
            q.bindValue(0, 7);
            q.bindValue(1, utf8str);
            QCOMPARE(q.boundValues().at(0).toInt(), 7);
            QCOMPARE(q.boundValues().at(1).toString(), utf8str);
            QVERIFY_SQL(q, exec());
            QCOMPARE(q.boundValues().at(0).toInt(), 7);
            QCOMPARE(q.boundValues().at(1).toString(), utf8str);
        }

        // Test binding more placeholders than the query contains placeholders
        q.addBindValue(8);
        q.addBindValue(9);
        q.addBindValue(10);
        QCOMPARE(q.boundValues().size(), 3);
        QCOMPARE(q.boundValues().at(0).toInt(), 8);
        QCOMPARE(q.boundValues().at(1).toInt(), 9);
        QCOMPARE(q.boundValues().at(2).toInt(), 10);
        QFAIL_SQL(q, exec());

        QVERIFY_SQL(q, exec(QLatin1String("SELECT * FROM %1 order by id").arg(qtest_prepare)));
        for (int i = 0; i < 6; ++i) {
            QVERIFY(q.next());
            QCOMPARE(q.value(0).toInt(), i);
            QCOMPARE(q.value(1).toString().trimmed(), values[i]);
        }

        QVERIFY(q.next());

        QCOMPARE(q.value(0).toInt(), 6);
        QVERIFY(q.value(1).toString().isEmpty());

        if (useUnicode) {
            QVERIFY(q.next());
            QCOMPARE(q.value(0).toInt(), 7);
            QCOMPARE(q.value(1).toString(), utf8str);
        }

        QVERIFY(q.prepare(QLatin1String("insert into %1 (id, name) values (?, 'Bart')")
                          .arg(qtest_prepare)));
        q.bindValue(0, 99);
        QVERIFY_SQL(q, exec());
        q.addBindValue(100);
        QVERIFY_SQL(q, exec());

        QVERIFY(q.exec(QLatin1String("select * from %1 where id > 98 order by id")
                       .arg(qtest_prepare)));
        for (int i = 99; i <= 100; ++i) {
            QVERIFY(q.next());
            QCOMPARE(q.value(0).toInt(), i);
            QCOMPARE(q.value(1).toString().trimmed(), u"Bart");
        }

        // Insert a duplicate id and make sure the db bails out:
        QVERIFY(q.prepare(QLatin1String("insert into %1 (id, name) values (?, ?)")
                          .arg(qtest_prepare)));
        q.addBindValue( 99 );
        q.addBindValue(u"something silly"_s);

        QVERIFY(!q.exec());
        QVERIFY(q.lastError().isValid());
        QVERIFY(!q.isActive());

        QVERIFY(q.prepare(QLatin1String(
                              "insert into %1 (id, name, name2) values (:id, :name, :name)")
                          .arg(qtest_prepare)));
        for (int i = 101; i < 103; ++i) {
            q.bindValue(":id", i);
            q.bindValue(":name", "name");
            QVERIFY(q.exec());
        }

        // Test for QTBUG-6420
        QVERIFY(q.exec(QLatin1String("select * from %1 where id > 100 order by id")
                       .arg(qtest_prepare)));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 101);
        QCOMPARE(q.value(1).toString(), u"name");
        QCOMPARE(q.value(2).toString(), u"name");

        // Test that duplicated named placeholders before the next unique one
        // works correctly - QTBUG-65150
        QVERIFY(q.prepare(QLatin1String("insert into %1 (id, name, name2) values (:id, :id, :name)")
                          .arg(qtest_prepare)));
        for (int i = 104; i < 106; ++i) {
            q.bindValue(":id", i);
            q.bindValue(":name", "name");
            QVERIFY(q.exec());
        }
        QVERIFY(q.exec(QLatin1String("select * from %1 where id > 103 order by id")
                       .arg(qtest_prepare)));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 104);
        QCOMPARE(q.value(1).toString(), u"104");
        QCOMPARE(q.value(2).toString(), u"name");

        // Test that duplicated named placeholders in any order
        QVERIFY(q.prepare(QLatin1String("insert into %1 (id, name, name2) values (:id, :name, :id)")
                          .arg(qtest_prepare)));
        for (int i = 107; i < 109; ++i) {
            q.bindValue(":id", i);
            q.bindValue(":name", "name");
            QVERIFY(q.exec());
        }
        QVERIFY(q.exec(QLatin1String("select * from %1 where id > 106 order by id")
                       .arg(qtest_prepare)));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 107);
        QCOMPARE(q.value(1).toString(), u"name");
        QCOMPARE(q.value(2).toString(), u"107");

        // Test just duplicated placeholders
        QVERIFY(q.prepare(QLatin1String(
                              "insert into %1 (id, name, name2) values (110, :name, :name)")
                          .arg(qtest_prepare)));
        q.bindValue(":name", "name");
        QVERIFY_SQL(q, exec());
        QVERIFY(q.exec(QLatin1String("select * from %1 where id > 109 order by id")
                       .arg(qtest_prepare)));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 110);
        QCOMPARE(q.value(1).toString(), u"name");
        QCOMPARE(q.value(2).toString(), u"name");
    } // End of SQLite scope.
}

void tst_QSqlQuery::prepared_select()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QString query = QLatin1String(
        "select a.id, a.t_char, a.t_varchar from %1 a where a.id = ?").arg(qtest);

    QSqlQuery q(db);
    QVERIFY_SQL(q, prepare(query));

    q.bindValue(0, 1);
    QVERIFY_SQL(q, exec());
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 1);

    q.bindValue(0, 2);
    QVERIFY_SQL(q, exec());
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 2);

    q.bindValue(0, 3);
    QVERIFY_SQL(q, exec());
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 3);

    QVERIFY_SQL(q, prepare(query));
    QCOMPARE(q.at(), QSql::BeforeFirstRow);
    QVERIFY(!q.first());
}

void tst_QSqlQuery::sqlServerLongStrings()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    if (tst_Databases::getDatabaseType(db) != QSqlDriver::MSSqlServer)
        QSKIP("Test is specific to SQL Server");

    TableScope ts(db, "qtest_longstr", __FILE__);
    QSqlQuery q(db);

    QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %1 (id int primary key, longstring ntext)")
                        .arg(ts.tableName())));
    QVERIFY_SQL(q, prepare(QLatin1String("INSERT INTO %1 VALUES (?, ?)").arg(ts.tableName())));

    q.addBindValue(0);
    q.addBindValue(u"bubu"_s);
    QVERIFY_SQL(q, exec());

    const QString testStr(85000, QLatin1Char('a'));

    q.addBindValue(1);
    q.addBindValue(testStr);
    QVERIFY_SQL(q, exec());
    QVERIFY_SQL(q, exec("select * from " + ts.tableName()));

    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 0);
    QCOMPARE(q.value(1).toString(), u"bubu");

    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 1);
    QCOMPARE(q.value(1).toString(), testStr);
}

void tst_QSqlQuery::invalidQuery()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    QSqlQuery q(db);

    QVERIFY(!q.exec());

    QVERIFY(!q.exec("blahfasel"));
    QVERIFY(q.lastError().type() != QSqlError::NoError);
    QVERIFY(!q.next());
    QVERIFY(!q.isActive());

    if (dbType != QSqlDriver::Oracle && dbType != QSqlDriver::DB2
        && !db.driverName().startsWith("QODBC")) {
        // Oracle and DB2 just prepare everything without complaining:
        if (db.driver()->hasFeature(QSqlDriver::PreparedQueries))
            QVERIFY(!q.prepare("blahfasel"));
    }

    QVERIFY(!q.exec());
    QVERIFY(!q.isActive());
    QVERIFY(!q.next());
}

void tst_QSqlQuery::batchExec()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    TableScope ts(db, "qtest_batch", __FILE__);
    const auto &tableName = ts.tableName();
    const auto dbType = tst_Databases::getDatabaseType(db);
    QLatin1String timeStampString(dbType == QSqlDriver::Interbase ? "TIMESTAMP" : "TIMESTAMP (3)");

    QVERIFY_SQL(q, exec(QLatin1String(
                            "create table %2 (id int, name varchar(20), dt date, "
                            "num numeric(8, 4), dtstamp %1, extraId int, extraName varchar(20))")
                        .arg(timeStampString, tableName)));

    const QVariantList intCol = { 1, 2, QVariant(QMetaType(QMetaType::Int)) };
    const QVariantList charCol = { u"harald"_s, u"boris"_s,
                                   QVariant(QMetaType(QMetaType::QString)) };
    const QDateTime currentDateTime = QDateTime(QDateTime::currentDateTime());
    const QVariantList dateCol = { currentDateTime.date(), currentDateTime.date().addDays(-1),
                                   QVariant(QMetaType(QMetaType::QDate)) };
    const QVariantList numCol = { 2.3, 3.4, QVariant(QMetaType(QMetaType::Double)) };
    const QVariantList timeStampCol = { currentDateTime, currentDateTime.addDays(-1),
                                        QVariant(QMetaType(QMetaType::QDateTime)) };

    // Test with positional placeholders
    QVERIFY_SQL(q, prepare(QLatin1String("insert into %1 "
                                         "(id, name, dt, num, dtstamp, extraId, extraName) "
                                         "values (?, ?, ?, ?, ?, ?, ?)").arg(tableName)));
    q.addBindValue(intCol);
    q.addBindValue(charCol);
    q.addBindValue(dateCol);
    q.addBindValue(numCol);
    q.addBindValue(timeStampCol);
    q.addBindValue(intCol);
    q.addBindValue(charCol);

    QVERIFY_SQL(q, execBatch());
    QVERIFY_SQL(q, exec(QLatin1String("select id, name, dt, num, dtstamp, extraId, extraName from ")
                        + tableName));

    for (int i = 0; i < intCol.size(); ++i) {
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), intCol.at(i));
        QCOMPARE(q.value(1).toString(), charCol.at(i));
        QCOMPARE(q.value(2).toDate(), dateCol.at(i));
        QVERIFY(qFuzzyCompare(q.value(3).toDouble(), numCol.at(i).toDouble()));
        if (tst_Databases::getDatabaseType(db) == QSqlDriver::MySqlServer
            && timeStampCol.at(i).isNull()) {
            QEXPECT_FAIL("", "This appears to be a bug in MySQL as it converts null datetimes to "
                             "the current datetime for a timestamp field", Continue);
        }
        QCOMPARE(q.value(4).toDateTime(), timeStampCol.at(i));
        QCOMPARE(q.value(5).toInt(), intCol.at(i));
        QCOMPARE(q.value(6).toString(), charCol.at(i));
    }

    // Empty table ready for retesting with duplicated named placeholders
    QVERIFY_SQL(q, exec(QLatin1String("delete from ") + tableName));
    QVERIFY_SQL(q, prepare(QLatin1String(
                               "insert into %1 (id, name, dt, num, dtstamp, extraId, extraName) "
                               "values (:id, :name, :dt, :num, :dtstamp, :id, :name)")
                           .arg(tableName)));
    q.bindValue(":id", intCol);
    q.bindValue(":name", charCol);
    q.bindValue(":dt", dateCol);
    q.bindValue(":num", numCol);
    q.bindValue(":dtstamp", timeStampCol);

    QVERIFY_SQL(q, execBatch());
    QVERIFY_SQL(q, exec(QLatin1String("select id, name, dt, num, dtstamp, extraId, extraName from ")
                        + tableName));

    for (int i = 0; i < intCol.size(); ++i) {
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), intCol.at(i));
        QCOMPARE(q.value(1).toString(), charCol.at(i));
        QCOMPARE(q.value(2).toDate(), dateCol.at(i));
        QVERIFY(qFuzzyCompare(q.value(3).toDouble(), numCol.at(i).toDouble()));
        if (tst_Databases::getDatabaseType(db) == QSqlDriver::MySqlServer
            && timeStampCol.at(i).isNull()) {
            QEXPECT_FAIL("", "This appears to be a bug in MySQL as it converts null datetimes to "
                             "the current datetime for a timestamp field", Continue);
        }
        QCOMPARE(q.value(4).toDateTime(), timeStampCol.at(i));
        QCOMPARE(q.value(5).toInt(), intCol.at(i));
        QCOMPARE(q.value(6).toString(), charCol.at(i));
    }

    // Only test the prepared stored procedure approach where the driver has support
    // for batch operations as this will not work without it.
    // Currently Mimer SQL cannot use output parameters with procedures in batch operations.
    if (dbType != QSqlDriver::MimerSQL && db.driver()->hasFeature(QSqlDriver::BatchOperations)) {
        const QString procName = qTableName("qtest_batch_proc", __FILE__, db);
        QVERIFY_SQL(q, exec(QLatin1String(
                                "create or replace procedure %1 (x in timestamp, y out timestamp) "
                                "is\n"
                                "begin\n"
                                "    y := x;\n"
                                "end;\n").arg(procName)));
        QVERIFY(q.prepare(QLatin1String("call %1(?, ?)").arg(procName)));
        q.addBindValue(timeStampCol, QSql::In);
        QVariantList emptyDateTimes;
        emptyDateTimes.reserve(timeStampCol.size());
        for (int i = 0; i < timeStampCol.size(); ++i)
            emptyDateTimes << QVariant(QDateTime());
        q.addBindValue(emptyDateTimes, QSql::Out);
        QVERIFY_SQL(q, execBatch());
        QCOMPARE(q.boundValue(1).toList(), timeStampCol);
    }
}

void tst_QSqlQuery::QTBUG_43874()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "bug43874", __FILE__);

    QSqlQuery q(db);

    QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %1 (id INT)").arg(ts.tableName())));
    QVERIFY_SQL(q, prepare(QLatin1String("INSERT INTO %1 (id) VALUES (?)").arg(ts.tableName())));

    for (int i = 0; i < 2; ++i) {
        const QVariantList ids = { i };
        q.addBindValue(ids);
        QVERIFY_SQL(q, execBatch());
    }
    QVERIFY_SQL(q, exec(QLatin1String("SELECT id FROM %1 ORDER BY id").arg(ts.tableName())));

    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 0);

    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 1);
}

void tst_QSqlQuery::oraArrayBind()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (!db.driver()->hasFeature(QSqlDriver::BatchOperations))
        QSKIP("Database can't do BatchOperations");

    QSqlQuery q(db);

    QVERIFY_SQL(q, exec("CREATE OR REPLACE PACKAGE ora_array_test "
                          "IS TYPE names_type IS "
                          "TABLE OF VARCHAR(64) NOT NULL INDEX BY BINARY_INTEGER; "
                        "names_tab names_type; "
                        "PROCEDURE set_name(name_in IN VARCHAR2, row_in in INTEGER); "
                        "PROCEDURE get_name(row_in IN INTEGER, str_out OUT VARCHAR2); "
                        "PROCEDURE get_table(tbl OUT names_type); "
                        "PROCEDURE set_table(tbl IN names_type); "
                        "END ora_array_test; "));

    QVERIFY_SQL(q, exec("CREATE OR REPLACE PACKAGE BODY ora_array_test "
                        "IS "
                        "PROCEDURE set_name(name_in IN VARCHAR2, row_in in INTEGER) "
                        "IS "
                        "BEGIN "
                        "names_tab(row_in) := name_in; "
                        "END set_name; "

                        "PROCEDURE get_name(row_in IN INTEGER, str_out OUT VARCHAR2) "
                        "IS "
                        "BEGIN "
                        "str_out := names_tab(row_in); "
                        "END get_name; "

                        "PROCEDURE get_table(tbl OUT names_type) "
                        "IS "
                        "BEGIN "
                        "tbl:=names_tab; "
                        "END get_table; "

                        "PROCEDURE set_table(tbl IN names_type) "
                        "IS "
                        "BEGIN "
                        "names_tab := tbl; "
                        "END set_table; "
                        "END ora_array_test; "));

    QVariantList list = { u"lorem"_s, u"ipsum"_s, u"dolor"_s, u"sit"_s, u"amet"_s };

    QVERIFY_SQL(q, prepare("BEGIN "
                           "ora_array_test.set_table(?); "
                           "END;"));
    q.bindValue(0, list, QSql::In);
    QVERIFY_SQL(q, execBatch(QSqlQuery::ValuesAsColumns));
    QVERIFY_SQL(q, prepare("BEGIN "
                           "ora_array_test.get_table(?); "
                           "END;"));

    list.clear();
    list.resize(5, QString(64, ' '));

    q.bindValue(0, list, QSql::Out);

    QVERIFY_SQL(q, execBatch(QSqlQuery::ValuesAsColumns));

    const QVariantList out_list = q.boundValue(0).toList();

    QCOMPARE(out_list.at(0).toString(), u"lorem");
    QCOMPARE(out_list.at(1).toString(), u"ipsum");
    QCOMPARE(out_list.at(2).toString(), u"dolor");
    QCOMPARE(out_list.at(3).toString(), u"sit");
    QCOMPARE(out_list.at(4).toString(), u"amet");

    QVERIFY_SQL(q, exec("DROP PACKAGE ora_array_test"));
}

/* Tests that QSqlDatabase::record() and QSqlQuery::record() return the same
   thing - otherwise our models get confused.
*/
void tst_QSqlQuery::record_sqlite()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    TableScope ts(db, "record_sqlite");
    QSqlQuery q(db);

    QVERIFY_SQL(q, exec(QLatin1String(
                            "create table %1(id integer primary key, name varchar, title int)")
                        .arg(ts.tableName())));

    QSqlRecord rec = db.record(ts.tableName());

    QCOMPARE(rec.count(), 3);
    QCOMPARE(rec.field(0).metaType().id(), QMetaType::Int);
    QCOMPARE(rec.field(1).metaType().id(), QMetaType::QString);
    QCOMPARE(rec.field(2).metaType().id(), QMetaType::Int);

    // Important - select from an empty table:
    QVERIFY_SQL(q, exec("select id, name, title from " + ts.tableName()));

    rec = q.record();
    QCOMPARE(rec.count(), 3);
    QCOMPARE(rec.field(0).metaType().id(), QMetaType::Int);
    QCOMPARE(rec.field(1).metaType().id(), QMetaType::QString);
    QCOMPARE(rec.field(2).metaType().id(), QMetaType::Int);
}

void tst_QSqlQuery::oraLong()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QString aLotOfText(127000, QLatin1Char('H'));
    TableScope ts(db, "qtest_longstr", __FILE__);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QLatin1String("create table %1 (id int primary key, astr long)")
                        .arg(ts.tableName())));
    QVERIFY_SQL(q, prepare(QLatin1String("insert into %1 (id, astr) values (?, ?)")
                           .arg(ts.tableName())));
    q.addBindValue(1);
    q.addBindValue(aLotOfText);
    QVERIFY_SQL(q, exec());

    QVERIFY_SQL(q, exec("select id,astr from " + ts.tableName()));

    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 1);
    QCOMPARE(q.value(1).toString(), aLotOfText);
}

void tst_QSqlQuery::execErrorRecovery()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    TableScope ts(db, "qtest_exerr", __FILE__);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QLatin1String("create table %1 (id int not null primary key)").arg(ts.tableName())));
    QVERIFY_SQL(q, prepare(QLatin1String("insert into %1 values (?)").arg(ts.tableName())));

    q.addBindValue(1);
    QVERIFY_SQL(q, exec());

    q.addBindValue(1); // Binding the same pkey - should fail.
    QVERIFY(!q.exec());

    q.addBindValue(2); // This should work again.
    QVERIFY_SQL(q, exec());
}

void tst_QSqlQuery::prematureExec()
{
    QFETCH(QString, dbName);
    // We only want the engine name, for addDatabase():
    int cut = dbName.indexOf(QChar('@'));
    if (cut < 0)
        QSKIP("Failed to parse database type out of name");
    dbName.truncate(cut);
    cut = dbName.indexOf(QChar('_'));
    if (cut >= 0)
        dbName = dbName.sliced(cut + 1);

    const auto tidier = qScopeGuard([dbName]() { QSqlDatabase::removeDatabase(dbName); });
    // Note: destruction of db needs to happen before we call removeDatabase.
    auto db = QSqlDatabase::addDatabase(dbName);
    QSqlQuery q(db);

    QTest::ignoreMessage(QtWarningMsg,
                         "QSqlDatabasePrivate::removeDatabase: connection "
                         "'qt_sql_default_connection' is still in use, all "
                         "queries will cease to work.");
    QTest::ignoreMessage(QtWarningMsg,
                         "QSqlDatabasePrivate::addDatabase: duplicate connection name "
                         "'qt_sql_default_connection', old connection removed.");
    auto otherDb = QSqlDatabase::addDatabase(dbName);

    QTest::ignoreMessage(QtWarningMsg, "QSqlQuery::exec: called before driver has been set up");
    // QTBUG-100037: shouldn't crash !
    QVERIFY(!q.exec("select stuff from TheVoid"));
}

void tst_QSqlQuery::lastInsertId()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (!db.driver()->hasFeature(QSqlDriver::LastInsertId))
        QSKIP("Database doesn't support lastInsertId");

    QSqlQuery q(db);

    // PostgreSQL >= 8.1 relies on lastval() which does not work if a value is
    // manually inserted to the serial field, so we create a table specifically
    if (tst_Databases::getDatabaseType(db) == QSqlDriver::PostgreSQL) {
        TableScope ts(db, "tst_lastInsertId", __FILE__);
        QVERIFY_SQL(q, exec(QLatin1String("create table %1 (id serial not null, t_varchar "
                                          "varchar(20), t_char char(20), primary key(id))")
                            .arg(ts.tableName())));
        QVERIFY_SQL(q, exec(QLatin1String("insert into %1 (t_varchar, t_char) values "
                                          "('VarChar41', 'Char41')").arg(ts.tableName())));
    } else {
        QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (41, 'VarChar41', 'Char41')")
                            .arg(qtest)));
    }
    QVERIFY(q.lastInsertId().isValid());
}

void tst_QSqlQuery::lastQuery()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QString sql = "select * from " + qtest;
    QVERIFY_SQL(q, exec(sql));
    QCOMPARE(q.lastQuery(), sql);
    QCOMPARE(q.executedQuery(), sql);
}

void tst_QSqlQuery::lastQueryTwoQueries()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);

    QString sql = QLatin1String("select * from ") + qtest;
    QVERIFY_SQL(q, exec(sql));
    QCOMPARE(q.lastQuery(), sql);
    QCOMPARE(q.executedQuery(), sql);

    sql = QLatin1String("select id from ") + qtest;
    QVERIFY_SQL(q, exec(sql));
    QCOMPARE(q.lastQuery(), sql);
    QCOMPARE(q.executedQuery(), sql);
}

void tst_QSqlQuery::psql_bindWithDoubleColonCastOperator()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    TableScope ts(db, "bindtest", __FILE__);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QLatin1String(
                            "create table %1 (id1 int, id2 int, id3 int, fld1 int, fld2 int)")
                        .arg(ts.tableName())));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 values (1, 2, 3, 10, 5)").arg(ts.tableName())));

    // Insert tableName last to let the other %-tokens' numbering match what they're replaced with:
    const auto queryTemplate = QLatin1String("select sum((fld1 - fld2)::int) from %4 where "
                                             "id1 = %1 and id2 =%2 and id3=%3");
    const QString query = queryTemplate.arg(":myid1", ":myid2", ":myid3", ts.tableName());
    QVERIFY_SQL(q, prepare(query));
    q.bindValue(":myid1", 1);
    q.bindValue(":myid2", 2);
    q.bindValue(":myid3", 3);

    QVERIFY_SQL(q, exec());
    QVERIFY_SQL(q, next());

    // The positional placeholders are converted to named placeholders in executedQuery()
    const QString expected = db.driver()->hasFeature(QSqlDriver::PreparedQueries)
        ? query : queryTemplate.arg("1", "2", "3", ts.tableName());
    QCOMPARE(q.executedQuery(), expected);
}

void tst_QSqlQuery::psql_specialFloatValues()
{
    if (!std::numeric_limits<float>::has_quiet_NaN)
        QSKIP("Platform does not have quiet_NaN");
    if (!std::numeric_limits<float>::has_infinity)
        QSKIP("Platform does not have infinity");

    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);

    CHECK_DATABASE(db);
    QSqlQuery query(db);
    TableScope ts(db, "floattest", __FILE__);
    QVERIFY_SQL(query, exec(QLatin1String("create table %1 (value float)").arg(ts.tableName())));
    QVERIFY_SQL(query, prepare(QLatin1String("insert into %1 values(:value)").arg(ts.tableName())));

    const QVariant data[] = {
        QVariant(double(42.42)),
        QVariant(std::numeric_limits<double>::quiet_NaN()),
        QVariant(std::numeric_limits<double>::infinity()),
        QVariant(float(42.42)),
        QVariant(std::numeric_limits<float>::quiet_NaN()),
        QVariant(std::numeric_limits<float>::infinity()),
    };

    for (const QVariant &v : data) {
        query.bindValue(":value", v);
        QVERIFY_SQL(query, exec());
    }
}

/* For task 157397: Using QSqlQuery with an invalid QSqlDatabase
   does not set the last error of the query.
   This test function will output some warnings, that's ok.
*/
void tst_QSqlQuery::queryOnInvalidDatabase()
{
    {
        const auto tidier = qScopeGuard([]() {
            QSqlDatabase::removeDatabase("invalidConnection");
        });
        // Note: destruction of db needs to happen before we call removeDatabase.
        QTest::ignoreMessage(QtWarningMsg, "QSqlDatabase: INVALID driver not loaded");
#if QT_CONFIG(regularexpression)
        QTest::ignoreMessage(QtWarningMsg,
                             QRegularExpression("QSqlDatabase: available drivers: "));
#endif
        QSqlDatabase db = QSqlDatabase::addDatabase("INVALID", "invalidConnection");
        QVERIFY(db.lastError().isValid());

        QTest::ignoreMessage(QtWarningMsg, "QSqlQuery::exec: database not open");
        QSqlQuery query("SELECT 1 AS ID", db);
        QVERIFY(query.lastError().isValid());
    }

    {
        QSqlDatabase db = QSqlDatabase::database("this connection does not exist");
        QTest::ignoreMessage(QtWarningMsg, "QSqlQuery::exec: database not open");
        QSqlQuery query("SELECT 1 AS ID", db);
        QVERIFY(query.lastError().isValid());
    }
}

/* For task 159138: Error on instantiating a sql-query before explicitly
   opening the database. This is something we don't support, so this isn't
   really a bug. However some of the drivers are nice enough to support it.
*/
void tst_QSqlQuery::createQueryOnClosedDatabase()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    // Only supported by these drivers

    if (dbType != QSqlDriver::PostgreSQL && dbType != QSqlDriver::Oracle
            && dbType != QSqlDriver::MySqlServer && dbType != QSqlDriver::DB2) {
        QSKIP("Test is specific for PostgreSQL, Oracle, MySql and DB2");
    }

    db.close();

    QSqlQuery q(db);
    db.open();
    QVERIFY_SQL(q, exec(QLatin1String("select * from %1 where id = 1").arg(qtest)));

    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 1);
    QCOMPARE(q.value(1).toString().trimmed(), u"VarChar1");
    QCOMPARE(q.value(2).toString().trimmed(), u"Char1");

    db.close();
    QVERIFY2(!q.exec(QLatin1String("select * from %1 where id = 1").arg(qtest)),
             "This can't happen! The query should not have been executed!");
}

void tst_QSqlQuery::reExecutePreparedForwardOnlyQuery()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    q.setForwardOnly(true);

    QVERIFY_SQL(q, prepare(QLatin1String("SELECT id, t_varchar, t_char FROM %1 WHERE id = :id")
                           .arg(qtest)));
    q.bindValue(":id", 1);
    QVERIFY_SQL(q, exec());

    // Do something, like iterate over the result, or skip to the end
    QVERIFY_SQL(q, last());

    QVERIFY_SQL(q, exec());
    /* This was broken with SQLite because the cache size was set to 0 in the 2nd execute.
       When forwardOnly is set we don't cahce the entire result, but we do cache the current row
       but this requires the cache size to be equal to the column count.
    */
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 1);
    QCOMPARE(q.value(1).toString().trimmed(), u"VarChar1");
    QCOMPARE(q.value(2).toString().trimmed(), u"Char1");
}

void tst_QSqlQuery::finish()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QVERIFY_SQL(q, prepare(QLatin1String("SELECT id FROM %1 WHERE id = ?").arg(qtest)));

    int id = 4;
    q.bindValue(0, id);
    QVERIFY_SQL(q, exec());
    QVERIFY(q.isActive());
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), id);

    q.finish();
    QVERIFY(!q.isActive()); // Query is now inactive, but ...
    QCOMPARE(q.boundValue(0).toInt(), id); // bound values are retained.

    QVERIFY_SQL(q, exec()); // No prepare needed.
    QVERIFY(q.isActive());
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), id);

    q.finish();
    QVERIFY(!q.isActive());

    QVERIFY_SQL(q, exec(QLatin1String("SELECT id FROM %1 WHERE id = 1").arg(qtest)));
    QVERIFY(q.isActive());
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 1);
    QCOMPARE(q.record().count(), 1);
}

void tst_QSqlQuery::sqlite_finish()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (db.databaseName().startsWith(':'))
        QSKIP("This test requires a database on the filesystem, not in-memory");

    {
        const auto tidier = qScopeGuard([]() {
            QSqlDatabase::removeDatabase("sqlite_finish_sqlite");
        });
        // Note: destruction of db2 needs to happen before we call removeDatabase.
        QSqlDatabase db2 = QSqlDatabase::addDatabase("QSQLITE", "sqlite_finish_sqlite");
        db2.setDatabaseName(db.databaseName());
        QVERIFY_SQL(db2, open());

        TableScope ts(db, "qtest_lockedtable", __FILE__);
        const auto &tableName = ts.tableName();
        QSqlQuery q(db);

        q.exec(QLatin1String("CREATE TABLE %1 (pk_id INTEGER PRIMARY KEY, whatever TEXT)")
               .arg(tableName));
        q.exec(QLatin1String("INSERT INTO %1 values(1, 'whatever')").arg(tableName));
        q.exec(QLatin1String("INSERT INTO %1 values(2, 'whatever more')").arg(tableName));

        // This creates a read-lock in the database
        QVERIFY_SQL(q, exec(QLatin1String("SELECT * FROM %1 WHERE pk_id = 1 or pk_id = 2")
                            .arg(tableName)));
        QVERIFY_SQL(q, next());

        // The DELETE will fail because of the read-lock
        QSqlQuery q2(db2);
        QVERIFY(!q2.exec(QLatin1String("DELETE FROM %1 WHERE pk_id=2").arg(tableName)));
        QCOMPARE(q2.numRowsAffected(), -1);

        // The DELETE will succeed now because finish() removes the lock
        q.finish();
        QVERIFY_SQL(q2, exec(QLatin1String("DELETE FROM %1 WHERE pk_id=2").arg(tableName)));
        QCOMPARE(q2.numRowsAffected(), 1);
    }
}

void tst_QSqlQuery::nextResult()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    if (!db.driver()->hasFeature(QSqlDriver::MultipleResultSets))
        QSKIP("DBMS does not support multiple result sets");

    QSqlQuery q(db);
    TableScope ts(db, "more_results", __FILE__);
    const auto &tableName = ts.tableName();

    QVERIFY_SQL(q, exec(QLatin1String(
                            "CREATE TABLE %1 (id integer, text varchar(20), "
                            "num numeric(6, 3), empty varchar(10));").arg(tableName)));

    QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 VALUES(1, 'one', 1.1, '');").arg(tableName)));
    QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 VALUES(2, 'two', 2.2, '');").arg(tableName)));
    QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 VALUES(3, 'three', 3.3, '');")
                        .arg(tableName)));
    QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 VALUES(4, 'four', 4.4, '');")
                        .arg(tableName)));

    const QString tstStrings[] = { u"one"_s, u"two"_s, u"three"_s, u"four"_s };

    // Query that returns only one result set, nothing special about this
    QVERIFY_SQL(q, exec(QLatin1String("SELECT * FROM %1;").arg(tableName)));

    QVERIFY(q.next());                // Move to first row of the result set

    QVERIFY(!q.nextResult());         // No more result sets are available

    QVERIFY(!q.isActive());           // So the query is no longer active

    QVERIFY(!q.next());               // ... and no data is available as the call

    // Attempting nextResult() discarded the result set.

    // Query that returns two result sets (batch sql)
    // When working with multiple result sets SQL Server insists on non-scrollable cursors
    if (db.driverName().startsWith("QODBC"))
        q.setForwardOnly(true);

    QVERIFY_SQL(q, exec(QLatin1String("SELECT id FROM %1; SELECT text, num FROM %1;")
                        .arg(tableName)));

    QCOMPARE(q.record().count(), 1);  // Check that the meta data is as expected
    QCOMPARE(q.record().field(0).name().toUpper(), u"ID");
    QCOMPARE(q.record().field(0).metaType().id(), QMetaType::Int);

    QVERIFY(q.nextResult());          // Discards first result set and move to the next
    QCOMPARE(q.record().count(), 2);  // New meta data should be available

    QCOMPARE(q.record().field(0).name().toUpper(), u"TEXT");
    QCOMPARE(q.record().field(0).metaType().id(), QMetaType::QString);

    QCOMPARE(q.record().field(1).name().toUpper(), u"NUM");
    QCOMPARE(q.record().field(1).metaType().id(), QMetaType::Double);

    QVERIFY(q.next());                    // Move to first row of the second result set
    QFAIL_SQL(q, nextResult()); // No more result sets after this
    QVERIFY(!q.isActive());               // So the query is no longer active
    QVERIFY(!q.next());                   // ... and no data is available as the call to

    // nextResult() discarded the result set

    // Query that returns one result set, a count of affected rows and then another result set
    QVERIFY_SQL(q, exec(QLatin1String("SELECT id, text, num, empty FROM %1 WHERE id <= 3; "
                                      "UPDATE %1 SET empty = 'Yatta!'; "
                                      "SELECT id, empty FROM %1 WHERE id <=2;").arg(tableName)));

    // Check result set returned by first statement
    QVERIFY(q.isSelect());            // The first statement is a select

    for (int i = 0; i < 3; ++i) {
        QVERIFY_SQL(q, next());
        QCOMPARE(q.value(0).toInt(), 1 + i);
        QCOMPARE(q.value(1).toString(), tstStrings[i]);
        QCOMPARE(q.value(2).toDouble(), 1.1 * (i + 1));
        QVERIFY(q.value(3).toString().isEmpty());
    }

    QVERIFY_SQL(q, nextResult());

    QVERIFY(!q.isSelect());           // The second statement isn't a SELECT
    QVERIFY(!q.next());               // ... so no result set is available
    QCOMPARE(q.numRowsAffected(), 4); // 4 rows was affected by the UPDATE

    // Check result set returned by third statement
    QVERIFY_SQL(q, nextResult());
    QVERIFY(q.isSelect());            // The third statement is a SELECT

    for (int i = 0; i < 2; ++i) {
        QVERIFY_SQL(q, next());
        QCOMPARE(q.value(0).toInt(), 1 + i);
        QCOMPARE(q.value(1).toString(), u"Yatta!");
    }

    // Stored procedure with multiple result sets
    const QString procName(qTableName("proc_more_res", __FILE__, db));

    auto dropProc = [&]() {
        q.exec(QLatin1String(dbType == QSqlDriver::PostgreSQL
                             ? "DROP FUNCTION %1(refcursor, refcursor);"
                             : "DROP PROCEDURE %1;").arg(procName));
    };
    dropProc(); // To make sure it's not there before we start.

    QLatin1String creator;
    switch (dbType) {
    case QSqlDriver::MySqlServer:
        creator = QLatin1String("CREATE PROCEDURE %1()\n"
                                "BEGIN\n"
                                " SELECT id, text FROM %2;\n"
                                " SELECT empty, num, text, id FROM %2;\n"
                                "END");
        break;
    case QSqlDriver::DB2:
        creator = QLatin1String("CREATE PROCEDURE %1()\n"
                                "RESULT SETS 2\n"
                                "LANGUAGE SQL\n"
                                "p1:BEGIN\n"
                                " DECLARE cursor1 CURSOR WITH RETURN FOR "
                                  "SELECT id, text FROM %2;\n"
                                " DECLARE cursor2 CURSOR WITH RETURN FOR "
                                  "SELECT empty, num, text, id FROM %2;\n"
                                " OPEN cursor1;\n"
                                " OPEN cursor2;\n"
                                "END p1");
        break;
    case QSqlDriver::PostgreSQL:
        creator = QLatin1String("CREATE FUNCTION %1(ref1 refcursor, ref2 refcursor)\n"
                                "RETURNS SETOF refcursor AS $$\n"
                                "BEGIN\n"
                                " OPEN ref1 FOR SELECT id, text FROM %2;\n"
                                " RETURN NEXT ref1;\n"
                                " OPEN ref2 FOR SELECT empty, num, text, id FROM %2;\n"
                                " RETURN NEXT ref2;\n"
                                "END;\n"
                                "$$ LANGUAGE plpgsql");
        break;
    default:
        creator = QLatin1String("CREATE PROCEDURE %1\n"
                                "AS\n"
                                "SELECT id, text FROM %2\n"
                                "SELECT empty, num, text, id FROM %2");
        break;
    }
    QVERIFY_SQL(q, exec(creator.arg(procName, tableName)));
    const auto tidier = qScopeGuard(dropProc);

    QLatin1String caller;
    switch (dbType) {
    case QSqlDriver::MySqlServer:
    case QSqlDriver::DB2:
        q.setForwardOnly(true);
        caller = QLatin1String("CALL %1()");
        break;
    case QSqlDriver::PostgreSQL:
        // Returning multiple result sets from PostgreSQL stored procedure:
        // http://sqlines.com/postgresql/how-to/return_result_set_from_stored_procedure
        caller = QLatin1String("BEGIN;"
                               " SELECT %1('cur1', 'cur2');"
                               " FETCH ALL IN cur1;"
                               " FETCH ALL IN cur2;"
                               "COMMIT;");
        break;
    default:
        caller = QLatin1String("EXEC %1");
        break;
    }
    QVERIFY_SQL(q, exec(caller.arg(procName)));

    if (dbType == QSqlDriver::PostgreSQL) {
        // First result set - start of transaction
        QVERIFY(!q.isSelect());
        QCOMPARE(q.numRowsAffected(), 0);
        QVERIFY(q.nextResult());
        // Second result set contains cursor names
        QVERIFY(q.isSelect());
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toString(), "cur1");
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toString(), "cur2");
        QVERIFY(q.nextResult());
    }

    for (int i = 0; i < 4; ++i) {
        QVERIFY_SQL(q, next());
        QCOMPARE(q.value(0).toInt(), i + 1);
        QCOMPARE(q.value(1).toString(), tstStrings[i]);
    }

    QVERIFY_SQL(q, nextResult());
    QVERIFY_SQL(q, isActive());

    for (int i = 0; i < 4; ++i) {
        QVERIFY_SQL(q, next());
        QCOMPARE(q.value(0).toString(), u"Yatta!");
        QCOMPARE(q.value(1).toDouble(), 1.1 * (1 + i));
        QCOMPARE(q.value(2).toString(), tstStrings[i]);
        QCOMPARE(q.value(3).toInt(), 1 + i);
    }

    // MySQL also counts the CALL itself as a result
    if (dbType == QSqlDriver::MySqlServer) {
        QVERIFY(q.nextResult());
        QVERIFY(!q.isSelect());           // ... but it's not a select
        // ... and no rows are affected (at least not with this procedure):
        QCOMPARE(q.numRowsAffected(), 0);
    }
    if (dbType == QSqlDriver::PostgreSQL) {
        // Last result set - commit transaction
        QVERIFY(q.nextResult());
        QVERIFY(!q.isSelect());
        QCOMPARE(q.numRowsAffected(), 0);
    }

    QVERIFY(!q.nextResult());
    QVERIFY(!q.isActive());
}


// For task 190311. Problem: Truncation happens on the 2nd execution if that BLOB is larger
// than the BLOB on the 1st execution. This was only for MySQL, but the test is general
// enough to be run with all backends.
void tst_QSqlQuery::blobsPreparedQuery()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (!db.driver()->hasFeature(QSqlDriver::BLOB)
        || !db.driver()->hasFeature(QSqlDriver::PreparedQueries)) {
        QSKIP("DBMS does not support BLOBs or prepared queries");
    }

    TableScope ts(db, "blobstest", __FILE__);

    QSqlQuery q(db);
    q.setForwardOnly(true); // This is needed to make the test work with DB2.
    QString shortBLOB("abc");
    QString longerBLOB("abcdefghijklmnopqrstuvxyz¿äëïöü¡  ");

    // In PostgreSQL a BLOB is not called a BLOB, but a BYTEA! :-)
    // ... and in SQL Server it can be called a lot, but IMAGE will do.
    QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    const QLatin1String typeName(dbType == QSqlDriver::PostgreSQL ? "BYTEA"
                                 : dbType == QSqlDriver::MSSqlServer ? "IMAGE" : "BLOB");

    QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %1(id INTEGER, data %2)")
                        .arg(ts.tableName(), typeName)));
    q.prepare(QLatin1String("INSERT INTO %1(id, data) VALUES(:id, :data)").arg(ts.tableName()));
    q.bindValue(":id", 1);
    q.bindValue(":data", shortBLOB);
    QVERIFY_SQL(q, exec());

    q.bindValue(":id", 2);
    q.bindValue(":data", longerBLOB);
    QVERIFY_SQL(q, exec());

    // Two executions and result sets
    q.prepare(QLatin1String("SELECT data FROM %1 WHERE id = ?").arg(ts.tableName()));
    q.bindValue(0, QVariant(1));
    QVERIFY_SQL(q, exec());
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toString(), shortBLOB);

    q.bindValue(0, QVariant(2));
    QVERIFY_SQL(q, exec());
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toString().toUtf8(), longerBLOB.toUtf8());

    // Only one execution and result set
    q.prepare(QLatin1String("SELECT id, data FROM %1 ORDER BY id").arg(ts.tableName()));
    QVERIFY_SQL(q, exec());
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(1).toString(), shortBLOB);
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(1).toString(), longerBLOB);
}

// There were problems with navigating past the end of a table returning an error on mysql
void tst_QSqlQuery::emptyTableNavigate()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    {
        TableScope ts(db, "qtest_empty", __FILE__);
        QSqlQuery q(db);
        q.exec("drop table " + ts.tableName());
        QVERIFY_SQL(q, exec(QLatin1String("create table %1 (id char(10))").arg(ts.tableName())));
        QVERIFY_SQL(q, prepare("select * from " + ts.tableName()));
        QVERIFY_SQL(q, exec());
        QVERIFY(!q.next());
        QVERIFY(!q.lastError().isValid());
    }
}

void tst_QSqlQuery::timeStampParsing()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "timeStampParsing", __FILE__);
    QSqlQuery q(db);
    QLatin1String creator;
    switch (tst_Databases::getDatabaseType(db)) {
    case QSqlDriver::PostgreSQL:
        creator = QLatin1String("CREATE TABLE %1(id serial NOT NULL, "
                                "datefield timestamp, primary key(id));");
        break;
    case QSqlDriver::MySqlServer:
        creator = QLatin1String("CREATE TABLE %1(id integer NOT NULL AUTO_INCREMENT, "
                                "datefield timestamp, primary key(id));");
        break;
    case QSqlDriver::Interbase:
        // Since there is no auto-increment feature in Interbase we allow it to be null
        creator = QLatin1String("CREATE TABLE %1(id integer, datefield timestamp);");
        break;
    case QSqlDriver::MimerSQL:
        creator = QLatin1String("CREATE UNIQUE SEQUENCE timeStampParsing_seq");
        QVERIFY_SQL(q, exec(creator));
        creator = QLatin1String("CREATE TABLE %1(id integer NOT NULL default next value "
                                "for timeStampParsing_seq, "
                                "datefield timestamp, primary key(id));");
        break;
    default:
        creator = QLatin1String("CREATE TABLE %1("
                                "\"id\" integer NOT NULL PRIMARY KEY AUTOINCREMENT, "
                                "\"datefield\" timestamp);");
        break;
    }
    QVERIFY_SQL(q, exec(creator.arg(ts.tableName())));
    QLatin1String currentTimestamp;
    if (tst_Databases::getDatabaseType(db) == QSqlDriver::MimerSQL)
        currentTimestamp = QLatin1String("localtimestamp");
    else
        currentTimestamp = QLatin1String("current_timestamp");
    QVERIFY_SQL(q,
                exec(QLatin1String("INSERT INTO %1 (datefield) VALUES (%2);")
                             .arg(ts.tableName())
                             .arg(currentTimestamp)));
    QVERIFY_SQL(q, exec(QLatin1String("SELECT * FROM ") + ts.tableName()));
    while (q.next())
        QVERIFY(q.value(1).toDateTime().isValid());
}

void tst_QSqlQuery::task_217003()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "Planet", __FILE__);
    const auto &planets = ts.tableName();

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QLatin1String("create table %1 (Name varchar(20))").arg(planets)));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 VALUES ('Mercury')").arg(planets)));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 VALUES ('Venus')").arg(planets)));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 VALUES ('Earth')").arg(planets)));
    QVERIFY_SQL(q, exec(QLatin1String("insert into %1 VALUES ('Mars')").arg(planets)));

    QVERIFY_SQL(q, exec("SELECT Name FROM " + planets));
    QVERIFY_SQL(q, seek(3));
    QCOMPARE(q.value(0).toString(), u"Mars");
    QVERIFY_SQL(q, seek(1));
    QCOMPARE(q.value(0).toString(), u"Venus");
    QVERIFY_SQL(q, exec("SELECT Name FROM " + planets));
    QVERIFY_SQL(q, seek(3));
    QCOMPARE(q.value(0).toString(), u"Mars");
    QVERIFY_SQL(q, seek(0));
    QCOMPARE(q.value(0).toString(), u"Mercury");
    QVERIFY_SQL(q, seek(1));
    QCOMPARE(q.value(0).toString(), u"Venus");
}

void tst_QSqlQuery::task_250026()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "task_250026", __FILE__);

    QSqlQuery q(db);
    if (!q.exec(QLatin1String("create table %1 (longfield varchar(1100))").arg(ts.tableName()))) {
        qDebug() << "Error" << q.lastError();
        QSKIP("Db doesn't support \"1100\" as a size for fields");
    }

    const QString data258(258, QLatin1Char('A'));
    const QString data1026(1026, QLatin1Char('A'));
    QVERIFY_SQL(q, prepare(QLatin1String("insert into %1(longfield) VALUES (:longfield)")
                           .arg(ts.tableName())));
    q.bindValue(":longfield", data258);
    QVERIFY_SQL(q, exec());
    q.bindValue(":longfield", data1026);
    QVERIFY_SQL(q, exec());
    QVERIFY_SQL(q, exec("select * from " + ts.tableName()));
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toString().size(), data258.size());
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toString().size(), data1026.size());
}

void tst_QSqlQuery::crashQueryOnCloseDatabase()
{
    for (const auto &dbName : std::as_const(dbs.dbNames)) {
        const auto tidier = qScopeGuard([]() { QSqlDatabase::removeDatabase("crashTest"); });
        // Note: destruction of clonedDb needs to happen before we call removeDatabase.
        QSqlDatabase clonedDb = QSqlDatabase::cloneDatabase(
              QSqlDatabase::database(dbName), "crashTest");
        qDebug() << "Testing crash in sqlquery dtor for driver" << clonedDb.driverName();
        QVERIFY(clonedDb.open());
        QSqlQuery q(clonedDb);
        clonedDb.close();
    }
}

void tst_QSqlQuery::testNaN()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "testNaN", __FILE__);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QLatin1String(
                            "CREATE TABLE %1(dbl1 double precision,dbl2 double precision) "
                            "without oids;").arg(ts.tableName())));
    const QString queryString =
        QLatin1String("INSERT INTO %1(dbl1, dbl2) VALUES(?,?)").arg(ts.tableName());

    const double nan = qQNaN();
    QVERIFY_SQL(q, prepare(queryString));
    q.bindValue(0, nan);
    q.bindValue(1, nan);
    QVERIFY_SQL(q, exec());
}

void tst_QSqlQuery::psqlNumericMetadata()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec("SELECT 1::bit, '10101010000111101101'::varbit, "
                              "'10101111011'::varbit(15), '22222.20'::numeric(16,2), "
                              "'333333'::numeric(18), '444444'::numeric"));
    QVERIFY_SQL(q, next());
    QSqlRecord r = q.record();
    QSqlField f;

    f = r.field(0);
    QCOMPARE(f.length(), 1);
    QCOMPARE(f.precision(), -1);

    f = r.field(1);
    QCOMPARE(f.length(), -1);
    QCOMPARE(f.precision(), -1);

    f = r.field(2);
    QCOMPARE(f.length(), 15);
    QCOMPARE(f.precision(), -1);

    f = r.field(3);
    QCOMPARE(f.length(), 16);
    QCOMPARE(f.precision(), 2);

    f = r.field(4);
    QCOMPARE(f.length(), 18);
    QCOMPARE(f.precision(), 0);

    f = r.field(5);
    QCOMPARE(f.length(), -1);
    QCOMPARE(f.precision(), -1);
}

void tst_QSqlQuery::sqlServerReturn0()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    if (tst_Databases::getDatabaseType(db) != QSqlDriver::MSSqlServer)
        QSKIP("Test is specific to SQL Server");

    ProcScope ps(db, "test141895_proc", __FILE__);
    TableScope ts(db, "test141895", __FILE__);
    const auto &tableName = ts.tableName();
    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %1 (id integer)").arg(tableName)));
    QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 (id) VALUES (1)").arg(tableName)));
    QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 (id) VALUES (2)").arg(tableName)));
    QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 (id) VALUES (2)").arg(tableName)));
    QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 (id) VALUES (3)").arg(tableName)));
    QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 (id) VALUES (1)").arg(tableName)));
    QVERIFY_SQL(q, exec(QLatin1String("CREATE PROCEDURE %1 AS "
                                      "SELECT * FROM %2 WHERE ID = 2 "
                                      "RETURN 0").arg(ps.name(), ts.tableName())));

    QVERIFY_SQL(q, exec(QLatin1String("{CALL %1}").arg(ps.name())));
    QVERIFY_SQL(q, next());
}

void tst_QSqlQuery::QTBUG_551()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    QSqlQuery q(db);
    const QString pkgname(qTableName("pkg", __FILE__, db));
    QVERIFY_SQL(q, exec(QLatin1String(
                            "CREATE OR REPLACE PACKAGE %1 IS \n\n"
                            "TYPE IntType IS TABLE OF INTEGER      INDEX BY BINARY_INTEGER;\n"
                            "TYPE VCType  IS TABLE OF VARCHAR2(60) INDEX BY BINARY_INTEGER;\n"
                            "PROCEDURE P (Inp IN IntType,  Outp OUT VCType);\n"
                            "END %1;").arg(pkgname)));

    QVERIFY_SQL(q, exec(QLatin1String("CREATE OR REPLACE PACKAGE BODY %1 IS\n"
                                      "PROCEDURE P (Inp IN IntType,  Outp OUT VCType)\n"
                                      " IS\n"
                                      " BEGIN\n"
                                      "  Outp(1) := '1. Value is ' ||TO_CHAR(Inp(1));\n"
                                      "  Outp(2) := '2. Value is ' ||TO_CHAR(Inp(2));\n"
                                      "  Outp(3) := '3. Value is ' ||TO_CHAR(Inp(3));\n"
                                      " END p;\n"
                                      "END %1;").arg(pkgname)));

    q.prepare(QLatin1String("begin %1.p(:inp, :outp); end;").arg(pkgname));

    QString text;
    text.reserve(60);

    // loading arrays
    QVariantList inLst, outLst;
    for (int count = 0; count < 3; ++count) {
        inLst << count;
        outLst << text;
    }

    q.bindValue(":inp", inLst);
    q.bindValue(":outp", outLst, QSql::Out);

    QVERIFY_SQL(q, execBatch(QSqlQuery::ValuesAsColumns));
    const auto res_outLst = qvariant_cast<QVariantList>(q.boundValues().at(1));

    QCOMPARE(res_outLst[0].toString(), QLatin1String("1. Value is 0"));
    QCOMPARE(res_outLst[1].toString(), QLatin1String("2. Value is 1"));
    QCOMPARE(res_outLst[2].toString(), QLatin1String("3. Value is 2"));
}

void tst_QSqlQuery::QTBUG_12186()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);

    // Make sure that query.boundValues() returns the values in the right order
    // even for more than 16 placeholders:
    QSqlQuery query(db);
    query.prepare("INSERT INTO person (col1, col2, col3, col4, col5, col6, col7, col8, col9, "
                  "col10, col11, col12, col13, col14, col15, col16, col17, col18) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    const QList<QVariant> values = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
    for (const QVariant &v : values)
        query.bindValue(v.toInt(), v);

    QCOMPARE(query.boundValues(), values);
}

void tst_QSqlQuery::QTBUG_14132()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    QSqlQuery q(db);
    const QString procedureName(qTableName("procedure", __FILE__, db));
    QVERIFY_SQL(q, exec(QLatin1String("CREATE OR REPLACE PROCEDURE %1 (outStr OUT varchar2)\n"
                                      "is\n"
                                      "begin\n"
                                      " outStr := 'OUTSTRING'; \n"
                                      "end;").arg(procedureName)));
    QString placeholder = "XXXXXXXXX";
    QVERIFY(q.prepare(QLatin1String("CALL %1(?)").arg(procedureName)));
    q.addBindValue(placeholder, QSql::Out);
    QVERIFY_SQL(q, exec());
    QCOMPARE(q.boundValue(0).toString(), QLatin1String("OUTSTRING"));
}

void tst_QSqlQuery::QTBUG_18435()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    if (dbType != QSqlDriver::MSSqlServer || !db.driverName().startsWith("QODBC"))
        QSKIP("Test is specific to SQL Server");

    ProcScope ps(db, "qtbug_18435_proc", __FILE__);
    QSqlQuery q(db);

    const QString stmt = QLatin1String("CREATE PROCEDURE %1 @key nvarchar(50) OUTPUT AS\n"
                                       "BEGIN\n"
                                       "  SET NOCOUNT ON\n"
                                       "  SET @key = 'TEST'\n"
                                       "END\n").arg(ps.name());

    QVERIFY_SQL(q, exec(stmt));
    QVERIFY_SQL(q, prepare(QLatin1String("{CALL %1(?)}").arg(ps.name())));
    const QString testStr = "0123";
    q.bindValue(0, testStr, QSql::Out);
    QVERIFY_SQL(q, exec());
    QCOMPARE(q.boundValue(0).toString(), QLatin1String("TEST"));
}

void tst_QSqlQuery::QTBUG_5251()
{
    // Since QSqlTableModel will escape the identifiers, we need to escape them
    // for databases that are case sensitive.
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "timetest", __FILE__);
    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %1 (t TIME)").arg(ts.tableName())));
    QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 VALUES ('1:2:3.666')").arg(ts.tableName())));

    QSqlTableModel timetestModel(0, db);
    timetestModel.setEditStrategy(QSqlTableModel::OnManualSubmit);
    timetestModel.setTable(ts.tableName());
    QVERIFY_SQL(timetestModel, select());

    QCOMPARE(timetestModel.record(0).field(0).value().toTime().toString("HH:mm:ss.zzz"),
             u"01:02:03.666");
    QVERIFY_SQL(timetestModel, setData(timetestModel.index(0, 0), QTime(0, 12, 34, 500)));
    QCOMPARE(timetestModel.record(0).field(0).value().toTime().toString("HH:mm:ss.zzz"),
             u"00:12:34.500");
    QVERIFY_SQL(timetestModel, submitAll());
    QCOMPARE(timetestModel.record(0).field(0).value().toTime().toString("HH:mm:ss.zzz"),
             u"00:12:34.500");

    QVERIFY_SQL(q, exec(QLatin1String("UPDATE %1 SET t = '0:11:22.33'").arg(ts.tableName())));
    QVERIFY_SQL(timetestModel, select());
    QCOMPARE(timetestModel.record(0).field(0).value().toTime().toString("HH:mm:ss.zzz"),
             u"00:11:22.330");
}

void tst_QSqlQuery::QTBUG_6421()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QString tableName(qTableName("bug6421", __FILE__, db).toUpper());
    TableScope ts(db, tableName);

    QSqlQuery q(db);

    QVERIFY_SQL(q, exec(QLatin1String(
                            "create table %1(COL1 char(10), COL2 char(10), COL3 char(10))")
                        .arg(tableName)));
    QVERIFY_SQL(q, exec(QLatin1String("create index INDEX1 on %1 (COL1 desc)").arg(tableName)));
    QVERIFY_SQL(q, exec(QLatin1String("create index INDEX2 on %1 (COL2 desc)").arg(tableName)));
    QVERIFY_SQL(q, exec(QLatin1String("create index INDEX3 on %1 (COL3 desc)").arg(tableName)));
    q.setForwardOnly(true);
    QVERIFY_SQL(q, exec(QLatin1String("select COLUMN_EXPRESSION from ALL_IND_EXPRESSIONS "
                                      "where TABLE_NAME='%1'")
                        .arg(tableName)));
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toString(), QLatin1String("\"COL1\""));
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toString(), QLatin1String("\"COL2\""));
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toString(), QLatin1String("\"COL3\""));
}

void tst_QSqlQuery::QTBUG_6618()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    if (tst_Databases::getDatabaseType(db) != QSqlDriver::MSSqlServer)
        QSKIP("Test is specific to SQL Server");

    ProcScope ps(db, "tst_raiseError", __FILE__);
    QSqlQuery q(db);
    QString errorString;
    for (int i = 0; i < 110; ++i)
        errorString += "reallylong";
    errorString += " error";
    QVERIFY_SQL(q, exec(QLatin1String("create procedure %1 as\n"
                                      "begin\n"
                                      "    raiserror('%2', 16, 1)\n"
                                      "end\n").arg(ps.name(), errorString)));
    q.exec(QLatin1String("{call %1}").arg(ps.name()));
    QVERIFY(q.lastError().text().contains(errorString));
}

void tst_QSqlQuery::QTBUG_6852()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "bug6852", __FILE__);
    const auto &tableName = ts.tableName();
    ProcScope ps(db, "bug6852_proc", __FILE__);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %1(\n"
                                      "MainKey INT NOT NULL,\n"
                                      "OtherTextCol VARCHAR(45) NOT NULL,\n"
                                      "PRIMARY KEY(`MainKey`))").arg(tableName)));
    QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 VALUES(0, \"Disabled\")").arg(tableName)));
    QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 VALUES(5, \"Error Only\")").arg(tableName)));
    QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 VALUES(10, \"Enabled\")").arg(tableName)));
    QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 VALUES(15, \"Always\")").arg(tableName)));
    QVERIFY_SQL(q, exec(QLatin1String("CREATE PROCEDURE %1()\n"
                                      "READS SQL DATA\n"
                                      "BEGIN\n"
                                      "  SET @st = 'SELECT MainKey, OtherTextCol from %2';\n"
                                      "  PREPARE stmt from @st;\n"
                                      "  EXECUTE stmt;\n"
                                      "END;").arg(ps.name(), tableName)));

    QVERIFY_SQL(q, exec(QLatin1String("CALL %1()").arg(ps.name())));
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 0);
    QCOMPARE(q.value(1).toString(), QLatin1String("Disabled"));
}

void tst_QSqlQuery::QTBUG_5765()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "bug5765", __FILE__);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %1(testval TINYINT(1) DEFAULT 0)")
                        .arg(ts.tableName())));
    q.prepare(QLatin1String("INSERT INTO %1 SET testval = :VALUE").arg(ts.tableName()));
    q.bindValue(":VALUE", 1);
    QVERIFY_SQL(q, exec());
    q.bindValue(":VALUE", 12);
    QVERIFY_SQL(q, exec());
    q.bindValue(":VALUE", 123);
    QVERIFY_SQL(q, exec());
    QString sql = "select testval from " + ts.tableName();
    QVERIFY_SQL(q, exec(sql));
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 1);
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 12);
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 123);
    QVERIFY_SQL(q, prepare(sql));
    QVERIFY_SQL(q, exec());
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 1);
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 12);
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 123);
}

/* Test multiple statements in one execution.
   SQLite driver doesn't support that. If more than one statement is given, the
   exec or prepare function return failure to the client.
*/
void tst_QSqlQuery::QTBUG_21884()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "bug21884", __FILE__);
    const auto &tableName = ts.tableName();

    QSqlQuery q(db);

    {
        const QString good[] = {
            QLatin1String("create table %1(id integer primary key, note string)").arg(tableName),
            QLatin1String("select * from %1;").arg(tableName),
            QLatin1String("select * from %1;  \t\n\r").arg(tableName),
            QLatin1String("drop table %1").arg(tableName)
        };

        for (const QString &st : good)
            QVERIFY_SQL(q, exec(st));

        for (const QString &st : good) {
            QVERIFY_SQL(q, prepare(st));
            QVERIFY_SQL(q, exec());
        }
    }

    {
        const QString bad[] = {
            QLatin1String("create table %1(id integer primary key); select * from ").arg(tableName),
            QLatin1String("create table %1(id integer primary key); syntax error!;").arg(tableName),
            QLatin1String("create table %1(id integer primary key);;").arg(tableName),
            QLatin1String("create table %1(id integer primary key);\'\"\a\b\b\v").arg(tableName)
        };

        QLatin1String shouldFail("the statement is expected to fail! %1");
        for (const QString &st : bad) {
            QVERIFY2(!q.prepare(st), qPrintable(shouldFail.arg(st)));
            QVERIFY2(!q.exec(st), qPrintable(shouldFail.arg(st)));
        }
    }
}

/* Test SQLite driver close function. SQLite driver should close cleanly even if
   there is still outstanding prepared statement.
*/
void tst_QSqlQuery::QTBUG_16967()
{
    QSqlQuery q2;
    QFETCH(QString, dbName);
    {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        CHECK_DATABASE(db);
        db.close();
        QCOMPARE(db.lastError().type(), QSqlError::NoError);
    }
    {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        CHECK_DATABASE(db);
        QSqlQuery q(db);
        q2 = QSqlQuery(q.lastQuery(), db);
        q.prepare("CREATE TABLE t1 (id INTEGER PRIMARY KEY, str TEXT);");
        db.close();
        QCOMPARE(db.lastError().type(), QSqlError::NoError);
    }
    {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        CHECK_DATABASE(db);
        QSqlQuery q(db);
        q2 = QSqlQuery(q.lastQuery(), db);
        q2.prepare("CREATE TABLE t1 (id INTEGER PRIMARY KEY, str TEXT);");
        q2.exec();
        db.close();
        QCOMPARE(db.lastError().type(), QSqlError::NoError);
    }
    {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        CHECK_DATABASE(db);
        QSqlQuery q(db);
        q2 = QSqlQuery(q.lastQuery(), db);
        q.exec("INSERT INTO t1 (id, str) VALUES(1, \"test1\");");
        db.close();
        QCOMPARE(db.lastError().type(), QSqlError::NoError);
    }
    {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        CHECK_DATABASE(db);
        QSqlQuery q(db);
        q2 = QSqlQuery(q.lastQuery(), db);
        q.exec("SELECT * FROM t1;");
        db.close();
        QCOMPARE(db.lastError().type(), QSqlError::NoError);
    }
}

/* In SQLite, when a boolean value is bound to a placeholder, it should be
   converted into integer 0/1 rather than text "false"/"true". According to
   documentation, SQLite does not have a separate Boolean storage class.
   Instead, Boolean values are stored as integers.
*/
void tst_QSqlQuery::QTBUG_23895()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "bug23895", __FILE__);
    const auto &tableName = ts.tableName();

    QSqlQuery q(db);

    q.prepare(QLatin1String("create table %1(id integer primary key, val1 bool, val2 boolean)")
              .arg(tableName));
    QVERIFY_SQL(q, exec());
    q.prepare(QLatin1String("insert into %1(id, val1, val2) values(?, ?, ?);").arg(tableName));
    q.addBindValue(1);
    q.addBindValue(true);
    q.addBindValue(false);
    QVERIFY_SQL(q, exec());

    QString sql = "select * from " + tableName;
    QVERIFY_SQL(q, exec(sql));
    QVERIFY_SQL(q, next());

    QCOMPARE(q.record().field(0).metaType().id(), QMetaType::Int);
    QCOMPARE(q.value(0).metaType().id(), QMetaType::LongLong);
    QCOMPARE(q.value(0).toInt(), 1);
    QCOMPARE(q.record().field(1).metaType().id(), QMetaType::Bool);
    QCOMPARE(q.value(1).metaType().id(), QMetaType::LongLong);
    QVERIFY(q.value(1).toBool());
    QCOMPARE(q.record().field(2).metaType().id(), QMetaType::Bool);
    QCOMPARE(q.value(2).metaType().id(), QMetaType::LongLong);
    QVERIFY(!q.value(2).toBool());

    q.prepare(QLatin1String("insert into %1(id, val1, val2) values(?, ?, ?);").arg(tableName));
    q.addBindValue(2);
    q.addBindValue(false);
    q.addBindValue(false);
    QVERIFY_SQL(q, exec());

    sql = QLatin1String("select * from %1 where val1").arg(tableName);
    QVERIFY_SQL(q, exec(sql));
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 1);
    QVERIFY(!q.next());

    sql = QLatin1String("select * from %1 where not val2").arg(tableName);
    QVERIFY_SQL(q, exec(sql));
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 1);
    QVERIFY_SQL(q, next());
    QCOMPARE(q.value(0).toInt(), 2);
    QVERIFY(!q.next());
}

// Test for aliases with dots:
void tst_QSqlQuery::QTBUG_14904()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "bug14904", __FILE__);

    QSqlQuery q(db);
    q.prepare(QLatin1String("create table %1(val1 bool)").arg(ts.tableName()));
    QVERIFY_SQL(q, exec());
    q.prepare(QLatin1String("insert into %1(val1) values(?);").arg(ts.tableName()));
    q.addBindValue(true);
    QVERIFY_SQL(q, exec());

    QString sql = "select val1 AS value1 from " + ts.tableName();
    QVERIFY_SQL(q, exec(sql));
    QVERIFY_SQL(q, next());

    QCOMPARE(q.record().indexOf("value1"), 0);
    QCOMPARE(q.record().field(0).metaType().id(), QMetaType::Bool);
    QVERIFY(q.value(0).toBool());

    sql = "select val1 AS 'value.one' from " + ts.tableName();
    QVERIFY_SQL(q, exec(sql));
    QVERIFY_SQL(q, next());
    QCOMPARE(q.record().indexOf("value.one"), 0);  // Was -1 before bug fix.
    QCOMPARE(q.record().field(0).metaType().id(), QMetaType::Bool);
    QVERIFY(q.value(0).toBool());
}

void tst_QSqlQuery::QTBUG_2192()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    {
        TableScope ts(db, "bug2192", __FILE__);
        QSqlQuery q(db);
        QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %1 (dt %2)")
                            .arg(ts.tableName(), tst_Databases::dateTimeTypeName(db))));

        QDateTime dt = QDateTime(QDate(2012, 7, 4), QTime(23, 59, 59, 999));
        QVERIFY_SQL(q, prepare(QLatin1String("INSERT INTO %1 (dt) VALUES (?)").arg(ts.tableName())));
        q.bindValue(0, dt);
        QVERIFY_SQL(q, exec());

        QVERIFY_SQL(q, exec("SELECT dt FROM " + ts.tableName()));
        QVERIFY_SQL(q, next());

        // Check if retrieved value preserves reported precision
        int precision = qMax(0, q.record().field("dt").precision());
        qint64 diff = qAbs(q.value(0).toDateTime().msecsTo(dt));
        qint64 keep = qMin(1000LL, qRound64(qPow(10.0, precision)));
        QVERIFY(diff <= 1000 - keep);
    }
}

void tst_QSqlQuery::QTBUG_36211()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    if (tst_Databases::getDatabaseType(db) == QSqlDriver::PostgreSQL) {
        TableScope ts(db, "bug36211", __FILE__);
        QSqlQuery q(db);
        QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %1 (dtwtz timestamptz, dtwotz timestamp)")
                            .arg(ts.tableName())));

#if QT_CONFIG(timezone)
        QTimeZone l_tzBrazil("America/Sao_Paulo");
        QTimeZone l_tzChina("Asia/Shanghai");
        QVERIFY(l_tzBrazil.isValid());
        QVERIFY(l_tzChina.isValid());
        QDateTime dt = QDateTime(QDate(2014, 10, 30), QTime(14, 12, 02, 357));
        QVERIFY_SQL(q, prepare(QLatin1String("INSERT INTO %1 (dtwtz, dtwotz) VALUES (:dt, :dt)")
                               .arg(ts.tableName())));
        q.bindValue(":dt", dt);
        QVERIFY_SQL(q, exec());
        q.bindValue(":dt", dt.toTimeZone(l_tzBrazil));
        QVERIFY_SQL(q, exec());
        q.bindValue(":dt", dt.toTimeZone(l_tzChina));
        QVERIFY_SQL(q, exec());

        QVERIFY_SQL(q, exec("SELECT dtwtz, dtwotz FROM " + ts.tableName()));

        for (int i = 0; i < 3; ++i) {
            QVERIFY_SQL(q, next());

            for (int j = 0; j < 2; ++j) {
                // Check if retrieved value preserves reported precision
                int precision = qMax(0, q.record().field(j).precision());
                qint64 diff = qAbs(q.value(j).toDateTime().msecsTo(dt));
                qint64 keep = qMin(1000LL, qRound64(qPow(10.0, precision)));
                QVERIFY(diff <= 1000 - keep);
            }
        }
#endif
    }
}

void tst_QSqlQuery::QTBUG_53969()
{
    QFETCH(QString, dbName);
    const QList<int> values = { 10, 20, 127, 128, 1 };
    QList<int> tableValues;
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    tableValues.reserve(values.size());
    if (tst_Databases::getDatabaseType(db) == QSqlDriver::MySqlServer) {
        TableScope ts(db, "bug53969", __FILE__);
        QSqlQuery q(db);
        QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %1 (id INT AUTO_INCREMENT PRIMARY KEY, "
                                          "test_number TINYINT(3) UNSIGNED)")
                            .arg(ts.tableName())));

        QVERIFY_SQL(q, prepare(QLatin1String("INSERT INTO %1 (test_number) VALUES (:value)")
                               .arg(ts.tableName())));

        for (int value : values) {
            q.bindValue(":value", value);
            QVERIFY_SQL(q, exec());
        }

        QVERIFY_SQL(q, prepare("SELECT test_number FROM " + ts.tableName()));
        QVERIFY_SQL(q, exec());

        while (q.next()) {
            bool ok;
            tableValues.push_back(q.value(0).toUInt(&ok));
            QVERIFY(ok);
        }
        QCOMPARE(tableValues, values);
    }
}

void tst_QSqlQuery::gisPointDatatype()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "qtbug72140", __FILE__);

    QSqlQuery sqlQuery(db);
    QVERIFY(sqlQuery.exec(QLatin1String(
                              "CREATE TABLE %1 (`lonlat_point` POINT NULL) ENGINE = InnoDB;")
                          .arg(ts.tableName())));
    QVERIFY(sqlQuery.exec(QLatin1String(
                              "INSERT INTO %1(lonlat_point) VALUES(ST_GeomFromText('POINT(1 1)'));")
                          .arg(ts.tableName())));
    QVERIFY(sqlQuery.exec(QLatin1String("SELECT * FROM %1;").arg(ts.tableName())));
    QCOMPARE(sqlQuery.record().field(0).metaType().id(), QMetaType::QByteArray);
    QVERIFY(sqlQuery.next());
}

void tst_QSqlQuery::oraOCINumber()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "qtest_oraOCINumber", __FILE__);

    QSqlQuery q(db);
    q.setForwardOnly(true);
    QVERIFY_SQL(q, exec(QLatin1String("create table %1 (col1 number(20), col2 number(20))")
                        .arg(ts.tableName())));
    QVERIFY(q.prepare(QLatin1String("insert into %1 values (?, ?)").arg(ts.tableName())));

    const QVariantList col1Values = {
        qulonglong(1), qulonglong(0), qulonglong(INT_MAX), qulonglong(UINT_MAX),
        qulonglong(LONG_MAX), qulonglong(ULONG_MAX), qulonglong(LLONG_MAX),
        qulonglong(ULLONG_MAX)
    };
    const QVariantList col2Values = {
        qlonglong(1), qlonglong(0), qlonglong(-1), qlonglong(LONG_MAX), qlonglong(LONG_MIN),
        qlonglong(ULONG_MAX), qlonglong(LLONG_MAX), qlonglong(LLONG_MIN)
    };

    q.addBindValue(col1Values);
    q.addBindValue(col2Values);
    QVERIFY(q.execBatch());
    QVERIFY(q.prepare(QLatin1String(
                          "select * from %1 where col1 = :bindValue0 AND col2 = :bindValue1")
                      .arg(ts.tableName())));

    q.bindValue(":bindValue0", qulonglong(1), QSql::InOut);
    q.bindValue(":bindValue1", qlonglong(1), QSql::InOut);

    QVERIFY_SQL(q, exec());
    QVERIFY(q.next());
    QCOMPARE(q.boundValue(0).toULongLong(), qulonglong(1));
    QCOMPARE(q.boundValue(1).toLongLong(), qlonglong(1));

    q.bindValue(":bindValue0", qulonglong(0), QSql::InOut);
    q.bindValue(":bindValue1", qlonglong(0), QSql::InOut);
    QVERIFY_SQL(q, exec());

    QVERIFY(q.next());
    QCOMPARE(q.boundValue(0).toULongLong(), qulonglong(0));
    QCOMPARE(q.boundValue(1).toLongLong(), qlonglong(0));

    q.bindValue(":bindValue0", qulonglong(INT_MAX), QSql::InOut);
    q.bindValue(":bindValue1", qlonglong(-1), QSql::InOut);
    QVERIFY_SQL(q, exec());

    QVERIFY(q.next());
    QCOMPARE(q.boundValue(0).toULongLong(), qulonglong(INT_MAX));
    QCOMPARE(q.boundValue(1).toLongLong(), qlonglong(-1));

    q.bindValue(":bindValue0", qulonglong(UINT_MAX), QSql::InOut);
    q.bindValue(":bindValue1", qlonglong(LONG_MAX), QSql::InOut);
    QVERIFY_SQL(q, exec());

    QVERIFY(q.next());
    QCOMPARE(q.boundValue(0).toULongLong(), qulonglong(UINT_MAX));
    QCOMPARE(q.boundValue(1).toLongLong(), qlonglong(LONG_MAX));

    q.bindValue(":bindValue0", qulonglong(LONG_MAX), QSql::InOut);
    q.bindValue(":bindValue1", qlonglong(LONG_MIN), QSql::InOut);
    QVERIFY_SQL(q, exec());

    QVERIFY(q.next());
    QCOMPARE(q.boundValue(0).toULongLong(), qulonglong(LONG_MAX));
    QCOMPARE(q.boundValue(1).toLongLong(), qlonglong(LONG_MIN));

    q.bindValue(":bindValue0", qulonglong(ULONG_MAX), QSql::InOut);
    q.bindValue(":bindValue1", qlonglong(ULONG_MAX), QSql::InOut);
    QVERIFY_SQL(q, exec());

    QVERIFY(q.next());
    QCOMPARE(q.boundValue(0).toULongLong(), qulonglong(ULONG_MAX));
    QCOMPARE(q.boundValue(1).toLongLong(), qlonglong(ULONG_MAX));

    q.bindValue(":bindValue0", qulonglong(LLONG_MAX), QSql::InOut);
    q.bindValue(":bindValue1", qlonglong(LLONG_MAX), QSql::InOut);
    QVERIFY_SQL(q, exec());

    QVERIFY(q.next());
    QCOMPARE(q.boundValue(0).toULongLong(), qulonglong(LLONG_MAX));
    QCOMPARE(q.boundValue(1).toLongLong(), qlonglong(LLONG_MAX));

    q.bindValue(":bindValue0", qulonglong(ULLONG_MAX), QSql::InOut);
    q.bindValue(":bindValue1", qlonglong(LLONG_MIN), QSql::InOut);
    QVERIFY_SQL(q, exec());

    QVERIFY(q.next());
    QCOMPARE(q.boundValue(0).toULongLong(), qulonglong(ULLONG_MAX));
    QCOMPARE(q.boundValue(1).toLongLong(), qlonglong(LLONG_MIN));
}

void tst_QSqlQuery::sqlite_constraint()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    const QString trigger(qTableName("test_constraint", __FILE__, db));

    QVERIFY_SQL(q, exec(QLatin1String("CREATE TEMP TRIGGER %1 BEFORE DELETE ON %2\n"
                                      "FOR EACH ROW\n"
                                      "BEGIN\n"
                                      "  SELECT RAISE(ABORT, 'Raised Abort successfully');\n"
                                      "END;").arg(trigger, qtest)));

    QVERIFY(!q.exec("DELETE FROM " + qtest));
    QCOMPARE(q.lastError().databaseText(), QLatin1String("Raised Abort successfully"));
}

void tst_QSqlQuery::sqlite_real()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "sqliterealtype", __FILE__);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %1 (id INTEGER, realVal REAL)")
                        .arg(ts.tableName())));
    QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 (id, realVal) VALUES (1, 2.3)")
                        .arg(ts.tableName())));
    QVERIFY_SQL(q, exec("SELECT realVal FROM " + ts.tableName()));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toDouble(), 2.3);
    QCOMPARE(q.record().field(0).metaType().id(), QMetaType::Double);

    q.prepare(QLatin1String("INSERT INTO %1 (id, realVal) VALUES (?, ?)").arg(ts.tableName()));
    QVariant var((double)5.6);
    q.addBindValue(4);
    q.addBindValue(var);
    QVERIFY_SQL(q, exec());

    QVERIFY_SQL(q, exec(QLatin1String("SELECT realVal FROM %1 WHERE ID=4").arg(ts.tableName())));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toDouble(), 5.6);
}

void tst_QSqlQuery::prepared_query_json_row()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    if (tst_Databases::getDatabaseType(db) != QSqlDriver::MySqlServer &&
        tst_Databases::getDatabaseType(db) != QSqlDriver::PostgreSQL) {
        QSKIP("PostgreSQL / MySQL specific test");
    }

    TableScope ts(db, "tableWithJsonRow", __FILE__);
    QSqlQuery q(db);
    const QLatin1String vals[] = {QLatin1String("{\"certificateNumber\": \"CERT-001\"}"),
                                  QLatin1String("{\"certificateNumber\": \"CERT-002\"}")};
    QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %1 (id INTEGER, value JSON)").arg(ts.tableName())));
    for (const QLatin1String &json : vals) {
        QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 (id, value) VALUES (1, '%2')")
                            .arg(ts.tableName(), json)));
    }

    QVERIFY_SQL(q, prepare(QLatin1String("SELECT id, value FROM %1 WHERE id = ?").arg(ts.tableName())));
    q.addBindValue(1);
    QVERIFY_SQL(q, exec());

    size_t iCount = 0;
    while (q.next()) {
        QVERIFY(iCount < sizeof(vals));
        const int id = q.value(0).toInt();
        const QByteArray json = q.value(1).toByteArray();
        QCOMPARE(id, 1);
        QCOMPARE(json, vals[iCount].data());
        ++iCount;
    }
}

void tst_QSqlQuery::aggregateFunctionTypes()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    int intType = QMetaType::Int;
    int sumType = intType;
    int countType = intType;
    // QPSQL uses LongLong for manipulation of integers
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    if (dbType == QSqlDriver::PostgreSQL || dbType == QSqlDriver::Interbase
        || dbType == QSqlDriver::MimerSQL) {
        sumType = countType = QMetaType::LongLong;
    } else if (dbType == QSqlDriver::Oracle) {
        intType = sumType = countType = QMetaType::Double;
    } else if (dbType == QSqlDriver::MySqlServer) {
        sumType = QMetaType::Double;
        countType = QMetaType::LongLong;
    }
    {
        TableScope ts(db, "numericFunctionsWithIntValues", __FILE__);
        const auto &tableName = ts.tableName();

        QSqlQuery q(db);
        QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %1 (id INTEGER)").arg(tableName)));

        // First test without any entries
        QVERIFY_SQL(q, exec("SELECT SUM(id) FROM " + tableName));
        QVERIFY(q.next());
        if (dbType == QSqlDriver::SQLite)
            QCOMPARE(q.record().field(0).metaType().id(), QMetaType::UnknownType);
        else
            QCOMPARE(q.record().field(0).metaType().id(), sumType);

        QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 (id) VALUES (1)").arg(tableName)));
        QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 (id) VALUES (2)").arg(tableName)));

        QVERIFY_SQL(q, exec("SELECT SUM(id) FROM " + tableName));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 3);
        QCOMPARE(q.record().field(0).metaType().id(), sumType);

        QVERIFY_SQL(q, exec("SELECT AVG(id) FROM " + tableName));
        QVERIFY(q.next());
        if (dbType == QSqlDriver::SQLite || dbType == QSqlDriver::PostgreSQL
            || dbType == QSqlDriver::MySqlServer || dbType == QSqlDriver::Oracle) {
            QCOMPARE(q.value(0).toDouble(), 1.5);
            QCOMPARE(q.record().field(0).metaType().id(), QMetaType::Double);
        } else {
            QCOMPARE(q.value(0).toInt(), 1);
            QCOMPARE(q.record().field(0).metaType().id(),
                     dbType == QSqlDriver::Interbase ? QMetaType::LongLong : QMetaType::Int);
        }

        QVERIFY_SQL(q, exec("SELECT COUNT(id) FROM " + tableName));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 2);
        QCOMPARE(q.record().field(0).metaType().id(), countType);

        QVERIFY_SQL(q, exec("SELECT MIN(id) FROM " + tableName));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 1);
        QCOMPARE(q.record().field(0).metaType().id(), intType);

        QVERIFY_SQL(q, exec("SELECT MAX(id) FROM " + tableName));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 2);
        QCOMPARE(q.record().field(0).metaType().id(), intType);
    }
    {
        TableScope ts(db, "numericFunctionsWithDoubleValues", __FILE__);
        const auto &tableName = ts.tableName();

        QSqlQuery q(db);
        QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %1 (id REAL)").arg(tableName)));

        // First test without any entries
        QVERIFY_SQL(q, exec("SELECT SUM(id) FROM " + tableName));
        QVERIFY(q.next());
        if (dbType == QSqlDriver::SQLite)
            QCOMPARE(q.record().field(0).metaType().id(), QMetaType::UnknownType);
        else
            QCOMPARE(q.record().field(0).metaType().id(), QMetaType::Double);

        QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 (id) VALUES (1.5)").arg(tableName)));
        QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 (id) VALUES (2.5)").arg(tableName)));

        QVERIFY_SQL(q, exec("SELECT SUM(id) FROM " + tableName));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toDouble(), 4.0);
        QCOMPARE(q.record().field(0).metaType().id(), QMetaType::Double);

        QVERIFY_SQL(q, exec("SELECT AVG(id) FROM " + tableName));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toDouble(), 2.0);
        QCOMPARE(q.record().field(0).metaType().id(), QMetaType::Double);

        QVERIFY_SQL(q, exec("SELECT COUNT(id) FROM " + tableName));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 2);
        QCOMPARE(q.record().field(0).metaType().id(), countType);

        QVERIFY_SQL(q, exec("SELECT MIN(id) FROM " + tableName));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toDouble(), 1.5);
        QCOMPARE(q.record().field(0).metaType().id(), QMetaType::Double);

        QVERIFY_SQL(q, exec("SELECT MAX(id) FROM " + tableName));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toDouble(), 2.5);
        QCOMPARE(q.record().field(0).metaType().id(), QMetaType::Double);

        QString field = "id";

        // PSQL does not have the round() function with real type
        if (dbType == QSqlDriver::PostgreSQL)
            field += "::NUMERIC";

        QVERIFY_SQL(q, exec(QLatin1String("SELECT ROUND(%1, 1) FROM %2 WHERE id=1.5")
                            .arg(field, tableName)));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toDouble(), 1.5);
        QCOMPARE(q.record().field(0).metaType().id(), QMetaType::Double);

        QVERIFY_SQL(q, exec(QLatin1String("SELECT ROUND(%1, 0) FROM %2 WHERE id=2.5")
                            .arg(field, tableName)));
        QVERIFY(q.next());
        if (dbType == QSqlDriver::MySqlServer)
            QCOMPARE(q.value(0).toDouble(), 2.0);
        else
            QCOMPARE(q.value(0).toDouble(), 3.0);
        QCOMPARE(q.record().field(0).metaType().id(), QMetaType::Double);
    }
    {
        TableScope ts(db, "stringFunctions", __FILE__);
        const auto &tableName = ts.tableName();

        QSqlQuery q(db);
        QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %1 (id INTEGER, txt VARCHAR(50))")
                            .arg(tableName)));

        QVERIFY_SQL(q, exec("SELECT MAX(txt) FROM " + tableName));
        QVERIFY(q.next());
        if (dbType == QSqlDriver::SQLite)
            QCOMPARE(q.record().field(0).metaType().id(), QMetaType::UnknownType);
        else
            QCOMPARE(q.record().field(0).metaType().id(), QMetaType::QString);

        QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 (id, txt) VALUES (1, 'lower')")
                            .arg(tableName)));
        QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 (id, txt) VALUES (2, 'upper')")
                            .arg(tableName)));

        QVERIFY_SQL(q, exec("SELECT MAX(txt) FROM " + tableName));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toString(), QLatin1String("upper"));
        QCOMPARE(q.record().field(0).metaType().id(), QMetaType::QString);
    }
}

template<typename T>
void runIntegralTypesMysqlTest(QSqlDatabase &db, const QString &tableName, const QString &type,
                               bool withPreparedStatement, const QList<T> &values)
{
    QList<QVariant> variantValues;
    variantValues.reserve(values.size());
    TableScope ts(db, tableName);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %2 (id %1)").arg(type, tableName)));

    if (withPreparedStatement) {
        QVERIFY_SQL(q, prepare(QLatin1String("INSERT INTO %1 (id) VALUES (?)").arg(tableName)));
    }
    for (int i = 0; i < values.size(); ++i) {
        const T v = values.at(i);
        if (withPreparedStatement) {
            q.bindValue(0, v);
            QVERIFY_SQL(q, exec());
        } else {
            QVERIFY_SQL(q, exec(QLatin1String("INSERT INTO %1 (id) VALUES (%2)")
                                .arg(tableName, QString::number(v))));
        }
        variantValues.append(QVariant::fromValue(v));
    }

    // Ensure we can read them back properly:
    if (withPreparedStatement) {
        QVERIFY_SQL(q, prepare("SELECT id FROM " + tableName));
        QVERIFY_SQL(q, exec());
    } else {
        QVERIFY_SQL(q, exec("SELECT id FROM " + tableName));
    }
    QList<T> actualValues;
    QList<QVariant> actualVariantValues;
    actualValues.reserve(values.size());
    while (q.next()) {
        QVariant value = q.value(0);
        actualVariantValues << value;
        actualValues << value.value<T>();
        QVERIFY(actualVariantValues.last().metaType().id() != qMetaTypeId<char>());
        QVERIFY(actualVariantValues.last().metaType().id() != qMetaTypeId<signed char>());
        QVERIFY(actualVariantValues.last().metaType().id() != qMetaTypeId<unsigned char>());
    }
    QCOMPARE(actualValues, values);
    QCOMPARE(actualVariantValues, variantValues);
}

template<typename T>
void runIntegralTypesMysqlTest(QSqlDatabase &db, const QString &tableName,
                               const QString &type, const bool withPreparedStatement,
                               const T min = std::numeric_limits<T>::min(),
                               const T max = std::numeric_limits<T>::max())
{
    // Insert some values:
    constexpr int steps = 20;
    const T increment = (max / steps - min / steps);
    QList<T> values;
    values.reserve(steps);
    T v = min;
    for (int i = 0; i < steps; ++i, v += increment)
        values.append(v);
    runIntegralTypesMysqlTest(db, tableName, type, withPreparedStatement, values);
}

void tst_QSqlQuery::integralTypesMysql()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    const QList<bool> boolValues = QList<bool>() << false << true;
    for (int i = 0; i < 2; ++i) {
        const bool withPreparedStatement = (i == 1);
        runIntegralTypesMysqlTest<bool>(db, "tinyInt1Test", "TINYINT(1)",
                                        withPreparedStatement, boolValues);
        runIntegralTypesMysqlTest<bool>(db, "unsignedTinyInt1Test", "TINYINT(1) UNSIGNED",
                                        withPreparedStatement, boolValues);
        runIntegralTypesMysqlTest<qint8>(db, "tinyIntTest", "TINYINT", withPreparedStatement);
        runIntegralTypesMysqlTest<quint8>(db, "unsignedTinyIntTest", "TINYINT UNSIGNED",
                                          withPreparedStatement);
        runIntegralTypesMysqlTest<qint16>(db, "smallIntTest", "SMALLINT", withPreparedStatement);
        runIntegralTypesMysqlTest<quint16>(db, "unsignedSmallIntTest", "SMALLINT UNSIGNED",
                                           withPreparedStatement);
        runIntegralTypesMysqlTest<qint32>(db, "mediumIntTest", "MEDIUMINT", withPreparedStatement,
                                          -(1 << 23), (1 << 23) - 1);
        runIntegralTypesMysqlTest<quint32>(db, "unsignedMediumIntTest", "MEDIUMINT UNSIGNED",
                                           withPreparedStatement, 0, (1 << 24) - 1);
        runIntegralTypesMysqlTest<qint32>(db, "intTest", "INT", withPreparedStatement);
        runIntegralTypesMysqlTest<quint32>(db, "unsignedIntTest", "INT UNSIGNED",
                                           withPreparedStatement);
        runIntegralTypesMysqlTest<qint64>(db, "bigIntTest", "BIGINT", withPreparedStatement);
        runIntegralTypesMysqlTest<quint64>(db, "unsignedBigIntTest", "BIGINT UNSIGNED",
                                           withPreparedStatement);
        runIntegralTypesMysqlTest<quint64>(db, "bitmask_7", "BIT(7)", withPreparedStatement, 0,
                                           (1LL << 7) - 1);
        runIntegralTypesMysqlTest<quint64>(db, "bitmask_31", "BIT(31)", withPreparedStatement, 0,
                                           (1LL << 31) - 1);
        runIntegralTypesMysqlTest<quint64>(db, "bitmask_33", "BIT(33)", withPreparedStatement, 0,
                                           (1LL << 33) - 1);
        runIntegralTypesMysqlTest<quint64>(db, "bitmask_64", "BIT(64)", withPreparedStatement);
    }
}

void tst_QSqlQuery::QTBUG_57138()
{
    const QDateTime utc(QDate(2150, 1, 5), QTime(14, 0, 0, 123), QTimeZone::UTC);
    const QDateTime localtime(QDate(2150, 1, 5), QTime(14, 0, 0, 123));
    const QDateTime tzoffset(QDate(2150, 1, 5), QTime(14, 0, 0, 123),
                             QTimeZone::fromSecondsAheadOfUtc(3600));

    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "qtbug57138", __FILE__);

    QSqlQuery create(db);

    QVERIFY_SQL(create, exec(QLatin1String(
                                 "create table %1 (id int, dt_utc datetime, dt_lt datetime, "
                                 "dt_tzoffset datetime)").arg(ts.tableName())));
    QVERIFY_SQL(create, prepare(QLatin1String("insert into %1 (id, dt_utc, dt_lt, dt_tzoffset) "
                                              "values (?, ?, ?, ?)").arg(ts.tableName())));

    create.addBindValue(0);
    create.addBindValue(utc);
    create.addBindValue(localtime);
    create.addBindValue(tzoffset);
    QVERIFY_SQL(create, exec());

    QSqlQuery q(db);
    q.prepare(QLatin1String("SELECT dt_utc, dt_lt, dt_tzoffset FROM %1 WHERE id = ?")
              .arg(ts.tableName()));
    q.addBindValue(0);

    QVERIFY_SQL(q, exec());
    QVERIFY(q.next());

    QCOMPARE(q.value(0).toDateTime(), utc);
    QCOMPARE(q.value(1).toDateTime(), localtime);
    QCOMPARE(q.value(2).toDateTime(), tzoffset);
}

void tst_QSqlQuery::QTBUG_73286()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "qtbug73286", __FILE__);

    QSqlQuery create(db);
    QVERIFY_SQL(create, exec(QLatin1String(
                                 "create table %1 (dec2 decimal(4,2), dec0 decimal(20,0), "
                                 "dec3 decimal(20,3))").arg(ts.tableName())));
    QVERIFY_SQL(create, prepare(QLatin1String(
                                    "insert into %1 (dec2, dec0, dec3) values (?, ?, ?)")
                                .arg(ts.tableName())));

    create.addBindValue("99.99");
    create.addBindValue("12345678901234567890");
    create.addBindValue("12345678901234567.890");

    QVERIFY_SQL(create, exec());

    QSqlQuery q(db);
    q.prepare("SELECT dec2, dec0, dec3 FROM " + ts.tableName());
    q.setNumericalPrecisionPolicy(QSql::HighPrecision);

    QVERIFY_SQL(q, exec());
    QVERIFY(q.next());

    QCOMPARE(q.value(0).toString(), "99.99");
    QCOMPARE(q.value(1).toString(), "12345678901234567890");
    QCOMPARE(q.value(2).toString(), "12345678901234567.890");
}

void tst_QSqlQuery::insertVarChar1()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    TableScope ts(db, "testtable", __FILE__);
    QVERIFY_SQL(q, exec(QLatin1String("CREATE TABLE %1 (smallcol VARCHAR(1))").arg(ts.tableName())));
    QVERIFY_SQL(q, prepare(QLatin1String("INSERT INTO %1 (smallcol) VALUES (?)").arg(ts.tableName())));
    QSqlField smallCol("smallcol");
    smallCol.setValue(QVariant(QString(QChar('F'))));
    q.bindValue(0, smallCol.value());
    QVERIFY_SQL(q, exec());
}

void tst_QSqlQuery::dateTime_data()
{
    if (dbs.dbNames.isEmpty())
        QSKIP("No database drivers are available in this Qt configuration");

    QTest::addColumn<QString>("dbName");
    QTest::addColumn<QString>("tableName");
    QTest::addColumn<QString>("createTableString");
    QTest::addColumn<QList<QDateTime> >("initialDateTimes");
    QTest::addColumn<QList<QDateTime> >("expectedDateTimes");

#if QT_CONFIG(timezone)
    // Using time zones which are highly unlikely to be the same as the testing
    // machine's one as it could pass as a result despite it.
    // +8.5 hours from UTC to North Korea
    const QTimeZone afterUTCTimeZone(30600);
    // -8 hours from UTC to Belize
    const QTimeZone beforeUTCTimeZone(-28800);
    const QTimeZone utcTimeZone("UTC");

    const QDateTime dtWithAfterTZ(QDate(2015, 5, 18), QTime(4, 26, 30, 500), afterUTCTimeZone);
    const QDateTime dtWithBeforeTZ(QDate(2015, 5, 18), QTime(4, 26, 30, 500), beforeUTCTimeZone);
    const QDateTime dtWithUTCTZ(QDate(2015, 5, 18), QTime(4, 26, 30, 500), utcTimeZone);
#endif
    const QDateTime dt(QDate(2015, 5, 18), QTime(4, 26, 30));
    const QDateTime dtWithMS(QDate(2015, 5, 18), QTime(4, 26, 30, 500));
    const QList<QDateTime> dateTimes = {
        dt, dtWithMS,
#if QT_CONFIG(timezone)
        dtWithAfterTZ, dtWithBeforeTZ, dtWithUTCTZ
#endif
    };
    const QList<QDateTime> expectedDateTimesLocalTZ = {
        dt, dtWithMS,
#if QT_CONFIG(timezone)
        dtWithAfterTZ.toLocalTime(), dtWithBeforeTZ.toLocalTime(), dtWithUTCTZ.toLocalTime()
#endif
    };
    const QList<QDateTime> expectedTimeStampDateTimes = {
        dt, dtWithMS,
#if QT_CONFIG(timezone)
        dtWithMS, dtWithMS, dtWithMS
#endif
    };
    const QList<QDateTime> expectedDateTimes = {
        dt, dt,
#if QT_CONFIG(timezone)
        dt, dt, dt
#endif
    };

    for (const QString &dbName : std::as_const(dbs.dbNames)) {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        if (!db.isValid())
            continue;
        const QString tableNameTSWithTimeZone(qTableName("dateTimeTSWithTimeZone", __FILE__, db));
        const QString tableNameTS(qTableName("dateTimeTS", __FILE__, db));
        const QString tableNameDate(qTableName("dateTimeDate", __FILE__, db));
        QTest::newRow(QString(dbName + " timestamp with time zone").toLatin1())
                        << dbName << tableNameTSWithTimeZone
                        << u" (dt TIMESTAMP WITH TIME ZONE)"_s
                        << dateTimes << dateTimes;
        QTest::newRow(QString(dbName + " timestamp with local time zone").toLatin1())
                        << dbName << tableNameTSWithTimeZone
                        << u" (dt TIMESTAMP WITH LOCAL TIME ZONE)"_s
                        << dateTimes << expectedDateTimesLocalTZ;
        QTest::newRow(QString(dbName + "timestamp").toLatin1())
                        << dbName << tableNameTS << u" (dt TIMESTAMP(3))"_s
                        << dateTimes << expectedTimeStampDateTimes;
        QTest::newRow(QString(dbName + "date").toLatin1())
                        << dbName << tableNameDate << u" (dt DATE)"_s
                        << dateTimes << expectedDateTimes;
    }
}

void tst_QSqlQuery::dateTime()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    if (tst_Databases::getDatabaseType(db) != QSqlDriver::Oracle)
        QSKIP("Implemented only for Oracle");

    QFETCH(QString, tableName);
    QFETCH(QString, createTableString);
    QFETCH(QList<QDateTime>, initialDateTimes);
    QFETCH(QList<QDateTime>, expectedDateTimes);

    TableScope ts(db, tableName);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec("CREATE TABLE " + tableName + createTableString));
    for (const QDateTime &dt : std::as_const(initialDateTimes)) {
        QVERIFY_SQL(q, prepare(QLatin1String("INSERT INTO %1 values(:dt)").arg(tableName)));
        q.bindValue(":dt", dt);
        QVERIFY_SQL(q, exec());
    }
    QVERIFY_SQL(q, exec("SELECT * FROM " + tableName));
    for (const QDateTime &dt : std::as_const(expectedDateTimes)) {
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toDateTime(), dt);
    }
}

void tst_QSqlQuery::ibaseDateTimeWithTZ_data()
{
    if (dbs.dbNames.isEmpty())
        QSKIP("No database drivers are available in this Qt configuration");

    QTest::addColumn<QString>("dbName");
    QTest::addColumn<QString>("tableName");
    QTest::addColumn<QList<QDateTime> >("initialDateTimes");
    QTest::addColumn<QList<QDateTime> >("expectedDateTimes");

#if QT_CONFIG(timezone)
    const QTimeZone afterUTCTimeZone("Asia/Hong_Kong");
    const QTimeZone beforeUTCTimeZone("America/Los_Angeles");
    const QDateTime dtWithAfterTZ(QDate(2015, 5, 18), QTime(4, 26, 30, 500), afterUTCTimeZone);
    const QDateTime dtWithBeforeTZ(QDate(2015, 5, 18), QTime(4, 26, 30, 500), beforeUTCTimeZone);
    const QTimeZone utcTimeZone("UTC");
    const QDateTime dtWithUTCTZ(QDate(2015, 5, 18), QTime(4, 26, 30, 500), utcTimeZone);
#endif // QT_CONFIG(timezone)
    const QDateTime dtLocalTZ(QDateTime::currentDateTime());

    const QList<QDateTime> dateTimes = {
#if QT_CONFIG(timezone)
        dtWithAfterTZ,
        dtWithBeforeTZ,
        dtWithUTCTZ,
#endif // QT_CONFIG(timezone)
        dtLocalTZ
    };

    for (const QString &dbName : std::as_const(dbs.dbNames)) {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        if (!db.isValid())
            continue;

        const QString tableNameTSWithTimeZone(qTableName("dateTimeTSWithTZ", __FILE__, db));

        QTest::newRow(QString(dbName + " timestamp with time zone").toLatin1())
                        << dbName
                        << tableNameTSWithTimeZone
                        << dateTimes
                        << dateTimes;
    }
}

void tst_QSqlQuery::ibaseDateTimeWithTZ()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    if (tst_Databases::getDatabaseType(db) != QSqlDriver::Interbase)
        QSKIP("Implemented only for Interbase");

    if (tst_Databases::getIbaseEngineVersion(db).majorVersion() < 4)
        QSKIP("Time zone support only implemented for firebird engine version 4 and greater");

    QFETCH(QString, tableName);
    TableScope ts(db, tableName);

    QSqlQuery q(db);
    QVERIFY_SQL(q, exec(QString("CREATE TABLE " + tableName + "(dt timestamp with time zone)")));

    QFETCH(QList<QDateTime>, initialDateTimes);
    QFETCH(QList<QDateTime>, expectedDateTimes);

    for (const QDateTime &dt : std::as_const(initialDateTimes)) {
        QVERIFY_SQL(q, prepare(QLatin1String("INSERT INTO %1 values(:dt)").arg(tableName)));
        q.bindValue(":dt", dt);
        QVERIFY_SQL(q, exec());
    }
    QVERIFY_SQL(q, exec("SELECT * FROM " + tableName));
    for (const QDateTime &dt : std::as_const(expectedDateTimes)) {
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toDateTime(), dt);
    }
}

void tst_QSqlQuery::sqliteVirtualTable()
{
    // Virtual tables can behave differently when it comes to prepared
    // queries, so we need to check these explicitly
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const auto tableName = qTableName("sqliteVirtual", __FILE__, db);
    QSqlQuery qry(db);
    QVERIFY_SQL(qry, exec(QLatin1String("create virtual table %1 using fts3(id, name)")
                          .arg(tableName)));

    // Delibrately malform the query to try and provoke a potential crash situation
    QVERIFY_SQL(qry, prepare(QLatin1String("select * from %1 where name match '?'")
                             .arg(tableName)));
    qry.addBindValue("Andy");
    QVERIFY(!qry.exec());

    QVERIFY_SQL(qry, prepare(QLatin1String("insert into %1(id, name) VALUES (?, ?)")
                             .arg(tableName)));
    qry.addBindValue(1);
    qry.addBindValue("Andy");
    QVERIFY_SQL(qry, exec());

    QVERIFY_SQL(qry, exec("select * from " + tableName));
    QVERIFY(qry.next());
    QCOMPARE(qry.value(0).toInt(), 1);
    QCOMPARE(qry.value(1).toString(), "Andy");

    QVERIFY_SQL(qry, prepare(QLatin1String("insert into %1(id, name) values (:id, :name)")
                             .arg(tableName)));
    qry.bindValue(":id", 2);
    qry.bindValue(":name", "Peter");
    QVERIFY_SQL(qry, exec());

    QVERIFY_SQL(qry, prepare(QLatin1String("select * from %1 where name match ?").arg(tableName)));
    qry.addBindValue("Peter");
    QVERIFY_SQL(qry, exec());
    QVERIFY(qry.next());
    QCOMPARE(qry.value(0).toInt(), 2);
    QCOMPARE(qry.value(1).toString(), "Peter");
}

void tst_QSqlQuery::mysql_timeType()
{
    // The TIME data type is different to the standard with MySQL as it has a range of
    // '-838:59:59' to '838:59:59'.
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    TableScope ts(db, "mysqlTimeType", __FILE__);
    const auto &tableName = ts.tableName();
    QSqlQuery qry(db);
    QVERIFY_SQL(qry, exec(QLatin1String("create table %1 (t time(6))").arg(tableName)));

    // MySQL will convert days into hours and add them together so 17 days 11 hours becomes 419 hours
    const QString timeData[] = {
        u"-838:59:59.000000"_s, u"-123:45:56.789"_s, u"000:00:00.0"_s, u"123:45:56.789"_s,
        u"838:59:59.000000"_s, u"15:50"_s, u"12"_s, u"1213"_s, u"0 1:2:3"_s, u"17 11:22:33"_s
    };
    const QString resultTimeData[] =  {
        u"-838:59:59.000000"_s, u"-123:45:56.789000"_s, u"00:00:00.000000"_s,
        u"123:45:56.789000"_s, u"838:59:59.000000"_s, u"15:50:00.000000"_s,
        u"00:00:12.000000"_s, u"00:12:13.000000"_s, u"01:02:03.000000"_s, u"419:22:33.000000"_s
    };
    for (const QString &time : timeData) {
        QVERIFY_SQL(qry, exec(QLatin1String("insert into %2 (t) VALUES ('%1')")
                                            .arg(time, tableName)));
    }

    QVERIFY_SQL(qry, exec("select * from " + tableName));
    for (const QString &time : resultTimeData) {
        QVERIFY(qry.next());
        QCOMPARE(qry.value(0).toString(), time);
    }

    QVERIFY_SQL(qry, exec("delete from " + tableName));
    for (const QString &time : timeData) {
        QVERIFY_SQL(qry, prepare(QLatin1String("insert into %1 (t) VALUES (:time)")
                                 .arg(tableName)));
        qry.bindValue(0, time);
        QVERIFY_SQL(qry, exec());
    }
    QVERIFY_SQL(qry, exec("select * from " + tableName));
    for (const QString &time : resultTimeData) {
        QVERIFY(qry.next());
        QCOMPARE(qry.value(0).toString(), time);
    }

    QVERIFY_SQL(qry, exec("delete from " + tableName));
    const QTime qTimeBasedData[] = {
        QTime(), QTime(1, 2, 3, 4), QTime(0, 0, 0, 0), QTime(23, 59, 59, 999)
    };
    for (const QTime &time : qTimeBasedData) {
        QVERIFY_SQL(qry, prepare(QLatin1String("insert into %1 (t) VALUES (:time)")
                                 .arg(tableName)));
        qry.bindValue(0, time);
        QVERIFY_SQL(qry, exec());
    }
    QVERIFY_SQL(qry, exec("select * from " + tableName));
    for (const QTime &time : qTimeBasedData) {
        QVERIFY(qry.next());
        QCOMPARE(qry.value(0).toTime(), time);
    }
}

void tst_QSqlQuery::ibaseArray()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    TableScope ts(db, "ibasearray", __FILE__);
    QSqlQuery qry(db);
    QVERIFY_SQL(qry, exec(QLatin1String(
                              "create table %1 (intData int[0:4], longData bigint[5], "
                              "charData varchar(255)[5], boolData boolean[2])").arg(ts.tableName())));
    QVERIFY_SQL(qry, prepare(QLatin1String("insert into %1 (intData, longData, charData, boolData)"
                                           " values(?, ?, ?, ?)").arg(ts.tableName())));
    const auto intArray = QVariant{QVariantList{1, 2, 3, 4711, 815}};
    const auto charArray = QVariant{QVariantList{"AAA", "BBB", "CCC", "DDD", "EEE"}};
    const auto boolArray = QVariant{QVariantList{true, false}};
    qry.bindValue(0, intArray);
    qry.bindValue(1, intArray);
    qry.bindValue(2, charArray);
    qry.bindValue(3, boolArray);
    QVERIFY_SQL(qry, exec());
    QVERIFY_SQL(qry, exec("select * from " + ts.tableName()));
    QVERIFY(qry.next());
    QCOMPARE(qry.value(0).toList(), intArray.toList());
    QCOMPARE(qry.value(1).toList(), intArray.toList());
    QCOMPARE(qry.value(2).toList(), charArray.toList());
    QCOMPARE(qry.value(3).toList(), boolArray.toList());
}

void tst_QSqlQuery::ibaseTimeStampTzArray()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (tst_Databases::getIbaseEngineVersion(db).majorVersion() < 4)
        QSKIP("Time zone support only implemented for firebird engine version 4 and greater");

    TableScope ts(db, "ibasetstzarray", __FILE__);
    QSqlQuery qry(db);
    QVERIFY_SQL(qry, exec(QLatin1String(
                              "create table %1 (timeStampData timestamp with time zone[0:4])").arg(ts.tableName())));
    QVERIFY_SQL(qry, prepare(QLatin1String("insert into %1 (timeStampData)"
                                           " values(?)").arg(ts.tableName())));
#if QT_CONFIG(timezone)
    const QDateTime dtWithAfterTZ(QDate(2015, 5, 18), QTime(4, 26, 30, 500), QTimeZone("Asia/Hong_Kong"));
    const QDateTime dtWithBeforeTZ(QDate(2015, 5, 18), QTime(4, 26, 30, 500), QTimeZone("America/Los_Angeles"));
    const QDateTime dtWithUTCTZ(QDate(2015, 5, 18), QTime(4, 26, 30, 500), QTimeZone("UTC"));
    const QDateTime dtLocalTZ(QDateTime::currentDateTime());
    const QDateTime dtWithMETTZ(QDate(2015, 5, 18), QTime(4, 26, 30, 500), QTimeZone("MET"));


    const auto timeStampData = QVariant{QVariantList{dtWithAfterTZ,
                                                     dtWithBeforeTZ,
                                                     dtWithUTCTZ,
                                                     dtLocalTZ,
                                                     dtWithMETTZ}};
    qry.bindValue(0, timeStampData);
    QVERIFY_SQL(qry, exec());
    QVERIFY_SQL(qry, exec("select * from " + ts.tableName()));
    QVERIFY(qry.next());
    QCOMPARE(qry.value(0).toList(), timeStampData.toList());
#endif // QT_CONFIG(timezone)
}

void tst_QSqlQuery::ibase_executeBlock()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    QSqlQuery qry(db);
    QVERIFY_SQL(qry, prepare("execute block (x double precision = ?, y double precision = ?) "
                             "returns (total double precision) "
                             "as "
                             "begin "
                             "total = :x + :y; "
                             "suspend; "
                             "end"));
    qry.bindValue(0, 2);
    qry.bindValue(1, 2);
    QVERIFY_SQL(qry, exec());
    QVERIFY(qry.next());
    QCOMPARE(qry.value(0).toInt(), 4);
}

QTEST_MAIN(tst_QSqlQuery)
#include "tst_qsqlquery.moc"
