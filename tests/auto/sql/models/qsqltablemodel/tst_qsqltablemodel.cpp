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
#include "../../kernel/qsqldatabase/tst_databases.h"
#include <QtSql>
#include <QtSql/private/qsqltablemodel_p.h>
#include <QThread>
#include <QElapsedTimer>

const QString test(qTableName("test", __FILE__, QSqlDatabase())),
                   test2(qTableName("test2", __FILE__, QSqlDatabase())),
                   test3(qTableName("test3", __FILE__, QSqlDatabase()));

// In order to catch when the warning message occurs, indicating that the database belongs to another
// thread, we have to install our own message handler. To ensure that the test reporting still happens
// as before, we call the originating one.
//
// For now, this is only called inside the modelInAnotherThread() test
QtMessageHandler oldHandler = nullptr;

void sqlTableModelMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (type == QtWarningMsg &&
        msg == "QSqlDatabasePrivate::database: requested database does not "
               "belong to the calling thread.") {
        QFAIL("Requested database does not belong to the calling thread.");
    }
    if (oldHandler)
        oldHandler(type, context, msg);
}

class tst_QSqlTableModel : public QObject
{
    Q_OBJECT

public:
    tst_QSqlTableModel();
    virtual ~tst_QSqlTableModel();


    void dropTestTables();
    void createTestTables();
    void recreateTestTables();
    void repopulateTestTables();

    tst_Databases dbs;

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:

    void select_data() { generic_data(); }
    void select();
    void selectRow_data() { generic_data(); }
    void selectRow();
    void selectRowOverride_data() { generic_data(); }
    void selectRowOverride();
    void insertColumns_data() { generic_data_with_strategies(); }
    void insertColumns();
    void submitAll_data() { generic_data(); }
    void submitAll();
    void setData_data()  { generic_data(); }
    void setData();
    void setRecord_data()  { generic_data(); }
    void setRecord();
    void setRecordReimpl_data()  { generic_data(); }
    void setRecordReimpl();
    void recordReimpl_data()  { generic_data(); }
    void recordReimpl();
    void insertRow_data() { generic_data_with_strategies(); }
    void insertRow();
    void insertRowFailure_data() { generic_data_with_strategies(); }
    void insertRowFailure();
    void insertRecord_data() { generic_data(); }
    void insertRecord();
    void insertMultiRecords_data() { generic_data(); }
    void insertMultiRecords();
    void insertWithAutoColumn_data() { generic_data_with_strategies("QSQLITE"); }
    void insertWithAutoColumn();
    void removeRow_data() { generic_data(); }
    void removeRow();
    void removeRows_data() { generic_data(); }
    void removeRows();
    void removeInsertedRow_data() { generic_data_with_strategies(); }
    void removeInsertedRow();
    void removeInsertedRows_data() { generic_data(); }
    void removeInsertedRows();
    void revert_data() { generic_data_with_strategies("QSQLITE"); }
    void revert();
    void isDirty_data() { generic_data_with_strategies(); }
    void isDirty();
    void setFilter_data() { generic_data(); }
    void setFilter();
    void setInvalidFilter_data() { generic_data(); }
    void setInvalidFilter();

    void emptyTable_data() { generic_data(); }
    void emptyTable();
    void tablesAndSchemas_data() { generic_data("QPSQL"); }
    void tablesAndSchemas();
    void whitespaceInIdentifiers_data() { generic_data(); }
    void whitespaceInIdentifiers();
    void primaryKeyOrder_data() { generic_data("QSQLITE"); }
    void primaryKeyOrder();

    void sqlite_bigTable_data() { generic_data("QSQLITE"); }
    void sqlite_bigTable();
    void modelInAnotherThread();

    // bug specific tests
    void insertRecordBeforeSelect_data() { generic_data(); }
    void insertRecordBeforeSelect();
    void submitAllOnInvalidTable_data() { generic_data(); }
    void submitAllOnInvalidTable();
    void insertRecordsInLoop_data() { generic_data(); }
    void insertRecordsInLoop();
    void sqlite_attachedDatabase_data() { generic_data("QSQLITE"); }
    void sqlite_attachedDatabase(); // For task 130799
    void tableModifyWithBlank_data() { generic_data(); }
    void tableModifyWithBlank(); // For mail task

    void removeColumnAndRow_data() { generic_data(); }
    void removeColumnAndRow(); // task 256032

    void insertBeforeDelete_data() { generic_data(); }
    void insertBeforeDelete();

    void invalidFilterAndHeaderData_data() { generic_data(); }
    void invalidFilterAndHeaderData(); //QTBUG-23879
private:
    void generic_data(const QString& engine=QString());
    void generic_data_with_strategies(const QString& engine=QString());
};

tst_QSqlTableModel::tst_QSqlTableModel()
{
    QVERIFY(dbs.open());
}

tst_QSqlTableModel::~tst_QSqlTableModel()
{
}

void tst_QSqlTableModel::dropTestTables()
{
    for (int i = 0; i < dbs.dbNames.count(); ++i) {
        QSqlDatabase db = QSqlDatabase::database(dbs.dbNames.at(i));
        QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
        QSqlQuery q(db);
        if (dbType == QSqlDriver::PostgreSQL)
            QVERIFY_SQL( q, exec("set client_min_messages='warning'"));

        QStringList tableNames;
        tableNames << test
                   << test2
                   << test3
                   << qTableName("test4", __FILE__, db)
                   << qTableName("emptytable", __FILE__, db)
                   << qTableName("bigtable", __FILE__, db)
                   << qTableName("foo", __FILE__, db)
                   << qTableName("pktest", __FILE__, db);
        if (testWhiteSpaceNames(db.driverName()))
            tableNames << qTableName("qtestw hitespace", db);

        tst_Databases::safeDropTables(db, tableNames);

        if (db.driverName().startsWith("QPSQL")) {
            q.exec("DROP SCHEMA " + qTableName("testschema", __FILE__, db) + " CASCADE");
        }
    }
}

void tst_QSqlTableModel::createTestTables()
{
    for (int i = 0; i < dbs.dbNames.count(); ++i) {
        QSqlDatabase db = QSqlDatabase::database(dbs.dbNames.at(i));
        QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
        QSqlQuery q(db);

        QVERIFY_SQL( q, exec("create table " + test + "(id int, name varchar(20), title int)"));

        QVERIFY_SQL( q, exec("create table " + test2 + "(id int, title varchar(20))"));

        QVERIFY_SQL( q, exec("create table " + test3 + "(id int, random varchar(20), randomtwo varchar(20))"));

        if (dbType != QSqlDriver::MSSqlServer)
            QVERIFY_SQL(q, exec("create table " + qTableName("test4", __FILE__, db) + "(column1 varchar(50), column2 varchar(50), column3 varchar(50))"));
        else
            QVERIFY_SQL(q, exec("create table " + qTableName("test4", __FILE__, db) + "(column1 varchar(50), column2 varchar(50) NULL, column3 varchar(50))"));


        QVERIFY_SQL(q, exec("create table " + qTableName("emptytable", __FILE__, db) + "(id int)"));

        if (testWhiteSpaceNames(db.driverName())) {
            QString qry = "create table " + qTableName("qtestw hitespace", db) + " ("+ db.driver()->escapeIdentifier("a field", QSqlDriver::FieldName) + " int)";
            QVERIFY_SQL( q, exec(qry));
        }

        QVERIFY_SQL(q, exec("create table " + qTableName("pktest", __FILE__, db) + "(id int not null primary key, a varchar(20))"));
    }
}

void tst_QSqlTableModel::repopulateTestTables()
{
    for (int i = 0; i < dbs.dbNames.count(); ++i) {
        QSqlDatabase db = QSqlDatabase::database(dbs.dbNames.at(i));
        QSqlQuery q(db);

        q.exec("delete from " + test);
        QVERIFY_SQL( q, exec("insert into " + test + " values(1, 'harry', 1)"));
        QVERIFY_SQL( q, exec("insert into " + test + " values(2, 'trond', 2)"));
        QVERIFY_SQL( q, exec("insert into " + test + " values(3, 'vohi', 3)"));

        q.exec("delete from " + test2);
        QVERIFY_SQL( q, exec("insert into " + test2 + " values(1, 'herr')"));
        QVERIFY_SQL( q, exec("insert into " + test2 + " values(2, 'mister')"));

        q.exec("delete from " + test3);
        QVERIFY_SQL( q, exec("insert into " + test3 + " values(1, 'foo', 'bar')"));
        QVERIFY_SQL( q, exec("insert into " + test3 + " values(2, 'baz', 'joe')"));
    }
}

void tst_QSqlTableModel::recreateTestTables()
{
    dropTestTables();
    createTestTables();
    repopulateTestTables();
}

void tst_QSqlTableModel::generic_data(const QString &engine)
{
    if ( dbs.fillTestTable(engine) == 0 ) {
        if (engine.isEmpty())
           QSKIP( "No database drivers are available in this Qt configuration");
        else
           QSKIP( (QString("No database drivers of type %1 are available in this Qt configuration").arg(engine)).toLocal8Bit());
    }
}

void tst_QSqlTableModel::generic_data_with_strategies(const QString &engine)
{
    if ( dbs.fillTestTableWithStrategies(engine) == 0 ) {
        if (engine.isEmpty())
           QSKIP( "No database drivers are available in this Qt configuration");
        else
           QSKIP( (QString("No database drivers of type %1 are available in this Qt configuration").arg(engine)).toLocal8Bit());
    }
}

void tst_QSqlTableModel::initTestCase()
{
    recreateTestTables();
}

void tst_QSqlTableModel::cleanupTestCase()
{
    dropTestTables();
    dbs.close();
}

void tst_QSqlTableModel::init()
{
}

void tst_QSqlTableModel::cleanup()
{
    recreateTestTables();
    if (oldHandler) {
        qInstallMessageHandler(oldHandler);
        oldHandler = nullptr;
    }
}

void tst_QSqlTableModel::select()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setTable(test);
    model.setSort(0, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.columnCount(), 3);

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 3)), QVariant());

    QCOMPARE(model.data(model.index(1, 0)).toInt(), 2);
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(1, 2)).toInt(), 2);
    QCOMPARE(model.data(model.index(1, 3)), QVariant());

    QCOMPARE(model.data(model.index(2, 0)).toInt(), 3);
    QCOMPARE(model.data(model.index(2, 1)).toString(), QString("vohi"));
    QCOMPARE(model.data(model.index(2, 2)).toInt(), 3);
    QCOMPARE(model.data(model.index(2, 3)), QVariant());

    QCOMPARE(model.data(model.index(3, 0)), QVariant());
    QCOMPARE(model.data(model.index(3, 1)), QVariant());
    QCOMPARE(model.data(model.index(3, 2)), QVariant());
    QCOMPARE(model.data(model.index(3, 3)), QVariant());
}

class SelectRowModel: public QSqlTableModel
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSqlTableModel)
public:
    SelectRowModel(QObject *parent, QSqlDatabase db): QSqlTableModel(parent, db) {}
    bool cacheEmpty() const
    {
        Q_D(const QSqlTableModel);
        return d->cache.isEmpty();
    }
};

void tst_QSqlTableModel::selectRow()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QString tbl = qTableName("pktest", __FILE__, db);
    QSqlQuery q(db);
    q.exec("DELETE FROM " + tbl);
    q.exec("INSERT INTO " + tbl + " (id, a) VALUES (0, 'a')");
    q.exec("INSERT INTO " + tbl + " (id, a) VALUES (1, 'b')");
    q.exec("INSERT INTO " + tbl + " (id, a) VALUES (2, 'c')");

    SelectRowModel model(0, db);
    model.setEditStrategy(QSqlTableModel::OnFieldChange);
    model.setTable(tbl);
    model.setSort(0, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.columnCount(), 2);

    QModelIndex idx = model.index(1, 1);

    // selectRow should not make the cache grow if there is no change.
    model.selectRow(1);
    QCOMPARE(model.data(idx).toString(), QString("b"));
    QVERIFY_SQL(model, cacheEmpty());

    // Check if selectRow() refreshes an unchanged row.
    // Row is not in cache yet.
    q.exec("UPDATE " + tbl + " SET a = 'Qt' WHERE id = 1");
    QCOMPARE(model.data(idx).toString(), QString("b"));
    model.selectRow(1);
    QCOMPARE(model.data(idx).toString(), QString("Qt"));

    // Check if selectRow() refreshes a changed row.
    // Row is already in the cache.
    model.setData(idx, QString("b"));
    QCOMPARE(model.data(idx).toString(), QString("b"));
    q.exec("UPDATE " + tbl + " SET a = 'Qt' WHERE id = 1");
    QCOMPARE(model.data(idx).toString(), QString("b"));
    model.selectRow(1);
    QCOMPARE(model.data(idx).toString(), QString("Qt"));

    q.exec("DELETE FROM " + tbl);
}

class SelectRowOverrideTestModel: public QSqlTableModel
{
    Q_OBJECT
public:
    SelectRowOverrideTestModel(QObject *parent, QSqlDatabase db):QSqlTableModel(parent, db) { }
    bool selectRow(int row)
    {
        Q_UNUSED(row)
        return select();
    }
};

void tst_QSqlTableModel::selectRowOverride()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QString tbl = qTableName("pktest", __FILE__, db);
    QSqlQuery q(db);
    q.exec("DELETE FROM " + tbl);
    q.exec("INSERT INTO " + tbl + " (id, a) VALUES (0, 'a')");
    q.exec("INSERT INTO " + tbl + " (id, a) VALUES (1, 'b')");
    q.exec("INSERT INTO " + tbl + " (id, a) VALUES (2, 'c')");

    SelectRowOverrideTestModel model(0, db);
    model.setEditStrategy(QSqlTableModel::OnFieldChange);
    model.setTable(tbl);
    model.setSort(0, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.columnCount(), 2);

    q.exec("UPDATE " + tbl + " SET a = 'Qt' WHERE id = 2");
    QModelIndex idx = model.index(1, 1);
    // overridden selectRow() should select() whole table and not crash
    model.setData(idx, QString("Qt"));

    // both rows should have changed
    QCOMPARE(model.data(idx).toString(), QString("Qt"));
    idx = model.index(2, 1);
    QCOMPARE(model.data(idx).toString(), QString("Qt"));

    q.exec("DELETE FROM " + tbl);
}

void tst_QSqlTableModel::insertColumns()
{
    // Just like the select test, with extra stuff
    QFETCH(QString, dbName);
    QFETCH(int, submitpolicy_i);
    QSqlTableModel::EditStrategy submitpolicy = (QSqlTableModel::EditStrategy) submitpolicy_i;
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setTable(test);
    model.setSort(0, Qt::AscendingOrder);
    model.setEditStrategy(submitpolicy);

    QVERIFY_SQL(model, select());

    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.columnCount(), 3);

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 3)), QVariant());

    QCOMPARE(model.data(model.index(1, 0)).toInt(), 2);
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(1, 2)).toInt(), 2);
    QCOMPARE(model.data(model.index(1, 3)), QVariant());

    QCOMPARE(model.data(model.index(2, 0)).toInt(), 3);
    QCOMPARE(model.data(model.index(2, 1)).toString(), QString("vohi"));
    QCOMPARE(model.data(model.index(2, 2)).toInt(), 3);
    QCOMPARE(model.data(model.index(2, 3)), QVariant());

    QCOMPARE(model.data(model.index(3, 0)), QVariant());
    QCOMPARE(model.data(model.index(3, 1)), QVariant());
    QCOMPARE(model.data(model.index(3, 2)), QVariant());
    QCOMPARE(model.data(model.index(3, 3)), QVariant());

    // Now add a column at 0 and 2
    model.insertColumn(0);
    model.insertColumn(2);

    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.columnCount(), 5);

    QCOMPARE(model.data(model.index(0, 0)), QVariant());
    QCOMPARE(model.data(model.index(0, 1)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 2)), QVariant());
    QCOMPARE(model.data(model.index(0, 3)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 4)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 5)), QVariant());

    QCOMPARE(model.data(model.index(1, 0)), QVariant());
    QCOMPARE(model.data(model.index(1, 1)).toInt(), 2);
    QCOMPARE(model.data(model.index(1, 2)), QVariant());
    QCOMPARE(model.data(model.index(1, 3)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(1, 4)).toInt(), 2);
    QCOMPARE(model.data(model.index(1, 5)), QVariant());

    QCOMPARE(model.data(model.index(2, 0)), QVariant());
    QCOMPARE(model.data(model.index(2, 1)).toInt(), 3);
    QCOMPARE(model.data(model.index(2, 2)), QVariant());
    QCOMPARE(model.data(model.index(2, 3)).toString(), QString("vohi"));
    QCOMPARE(model.data(model.index(2, 4)).toInt(), 3);
    QCOMPARE(model.data(model.index(2, 5)), QVariant());

    QCOMPARE(model.data(model.index(3, 0)), QVariant());
    QCOMPARE(model.data(model.index(3, 1)), QVariant());
    QCOMPARE(model.data(model.index(3, 2)), QVariant());
    QCOMPARE(model.data(model.index(3, 3)), QVariant());
    QCOMPARE(model.data(model.index(3, 4)), QVariant());
    QCOMPARE(model.data(model.index(3, 5)), QVariant());
}

void tst_QSqlTableModel::setData()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    model.setTable(test);
    model.setSort(0, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

    //  initial state
    QModelIndex idx = model.index(0, 0);
    QVariant val = model.data(idx);
    QVERIFY(val == int(1));
    QVERIFY(!val.isNull());
    QFAIL_SQL(model, isDirty());

    // change 1 to 0
    idx = model.index(0, 0);
    QVERIFY_SQL(model, setData(idx, int(0)));
    val = model.data(idx);
    QVERIFY(val == int(0));
    QVERIFY(!val.isNull());
    QVERIFY_SQL(model, isDirty(idx));
    QVERIFY_SQL(model, submitAll());

    // change 0 to NULL
    idx = model.index(0, 0);
    QVERIFY_SQL(model, setData(idx, QVariant(QVariant::Int)));
    val = model.data(idx);
    QCOMPARE(val, QVariant(QVariant::Int));
    QVERIFY(val.isNull());
    QVERIFY_SQL(model, isDirty(idx));
    QVERIFY_SQL(model, submitAll());

    // change NULL to 0
    idx = model.index(0, 0);
    QVERIFY_SQL(model, setData(idx, int(0)));
    val = model.data(idx);
    QVERIFY(val == int(0));
    QVERIFY(!val.isNull());
    QVERIFY_SQL(model, isDirty(idx));
    QVERIFY_SQL(model, submitAll());

    // ignore unchanged 0 to 0
    idx = model.index(0, 0);
    QVERIFY_SQL(model, setData(idx, int(0)));
    val = model.data(idx);
    QVERIFY(val == int(0));
    QVERIFY(!val.isNull());
    QFAIL_SQL(model, isDirty(idx));

    // pending INSERT
    QVERIFY_SQL(model, insertRow(0));
    // initial state
    idx = model.index(0, 0);
    QSqlRecord rec = model.record(0);
    QCOMPARE(rec.value(0), QVariant(QVariant::Int));
    QVERIFY(rec.isNull(0));
    QVERIFY(!rec.isGenerated(0));
    // unchanged value, but causes column to be included in INSERT
    QVERIFY_SQL(model, setData(idx, QVariant(QVariant::Int)));
    rec = model.record(0);
    QCOMPARE(rec.value(0), QVariant(QVariant::Int));
    QVERIFY(rec.isNull(0));
    QVERIFY(rec.isGenerated(0));
    QVERIFY_SQL(model, submitAll());
}

void tst_QSqlTableModel::setRecord()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QList<QSqlTableModel::EditStrategy> policies = QList<QSqlTableModel::EditStrategy>() << QSqlTableModel::OnFieldChange << QSqlTableModel::OnRowChange << QSqlTableModel::OnManualSubmit;

    QString Xsuffix;
    foreach( QSqlTableModel::EditStrategy submitpolicy, policies) {

        QSqlTableModel model(0, db);
        model.setEditStrategy((QSqlTableModel::EditStrategy)submitpolicy);
        model.setTable(test3);
        model.setSort(0, Qt::AscendingOrder);
        QVERIFY_SQL(model, select());

        for (int i = 0; i < model.rowCount(); ++i) {
            QSignalSpy spy(&model, SIGNAL(dataChanged(QModelIndex,QModelIndex)));

            QSqlRecord rec = model.record(i);
            rec.setValue(1, rec.value(1).toString() + 'X');
            rec.setValue(2, rec.value(2).toString() + 'X');
            QVERIFY(model.setRecord(i, rec));

            // dataChanged() emitted by setData() for each *changed* column
            if ((QSqlTableModel::EditStrategy)submitpolicy == QSqlTableModel::OnManualSubmit) {
                QCOMPARE(spy.count(), 2);
                QCOMPARE(spy.at(0).count(), 2);
                QCOMPARE(qvariant_cast<QModelIndex>(spy.at(0).at(0)), model.index(i, 1));
                QCOMPARE(qvariant_cast<QModelIndex>(spy.at(0).at(1)), model.index(i, 1));
                QCOMPARE(qvariant_cast<QModelIndex>(spy.at(1).at(0)), model.index(i, 2));
                QCOMPARE(qvariant_cast<QModelIndex>(spy.at(1).at(1)), model.index(i, 2));
                QVERIFY(model.submitAll());
            } else if ((QSqlTableModel::EditStrategy)submitpolicy == QSqlTableModel::OnRowChange && i == model.rowCount() -1)
                model.submit();
            else {
                if ((QSqlTableModel::EditStrategy)submitpolicy != QSqlTableModel::OnManualSubmit)
                    // dataChanged() also emitted by selectRow()
                    QCOMPARE(spy.count(), 3);
                else
                    QCOMPARE(spy.count(), 2);
                QCOMPARE(spy.at(0).count(), 2);
                QCOMPARE(qvariant_cast<QModelIndex>(spy.at(0).at(0)), model.index(i, 1));
                QCOMPARE(qvariant_cast<QModelIndex>(spy.at(0).at(1)), model.index(i, 1));
                QCOMPARE(qvariant_cast<QModelIndex>(spy.at(1).at(0)), model.index(i, 2));
                QCOMPARE(qvariant_cast<QModelIndex>(spy.at(1).at(1)), model.index(i, 2));
            }
        }

        Xsuffix.append('X');

        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("foo").append(Xsuffix));
        QCOMPARE(model.data(model.index(0, 2)).toString(), QString("bar").append(Xsuffix));
        QCOMPARE(model.data(model.index(1, 1)).toString(), QString("baz").append(Xsuffix));
        QCOMPARE(model.data(model.index(1, 2)).toString(), QString("joe").append(Xsuffix));
    }
}

class SetRecordReimplModel: public QSqlTableModel
{
    Q_OBJECT
public:
    SetRecordReimplModel(QObject *parent, QSqlDatabase db):QSqlTableModel(parent, db) {}
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole)
    {
        Q_UNUSED(value);
        return QSqlTableModel::setData(index, QString("Qt"), role);
    }
};

void tst_QSqlTableModel::setRecordReimpl()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    SetRecordReimplModel model(0, db);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    model.setTable(test3);
    model.setSort(0, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

    // make sure that a reimplemented setData() affects setRecord()
    QSqlRecord rec = model.record(0);
    rec.setValue(1, QString("x"));
    rec.setValue(2, QString("y"));
    QVERIFY(model.setRecord(0, rec));

    rec = model.record(0);
    QCOMPARE(rec.value(1).toString(), QString("Qt"));
    QCOMPARE(rec.value(2).toString(), QString("Qt"));
}

class RecordReimplModel: public QSqlTableModel
{
    Q_OBJECT
public:
    RecordReimplModel(QObject *parent, QSqlDatabase db):QSqlTableModel(parent, db) {}
    QVariant data(const QModelIndex &index, int role = Qt::EditRole) const
    {
        if (role == Qt::EditRole)
            return QString("Qt");
        else
            return QSqlTableModel::data(index, role);
    }
};

void tst_QSqlTableModel::recordReimpl()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    RecordReimplModel model(0, db);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    model.setTable(test3);
    model.setSort(0, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

    // make sure reimplemented data() affects record(row)
    QSqlRecord rec = model.record(0);
    QCOMPARE(rec.value(1).toString(), QString("Qt"));
    QCOMPARE(rec.value(2).toString(), QString("Qt"));

    // and also when the record is in the cache
    QVERIFY_SQL(model, setData(model.index(0, 1), QString("not Qt")));
    QVERIFY_SQL(model, setData(model.index(0, 2), QString("not Qt")));

    rec = model.record(0);
    QCOMPARE(rec.value(1).toString(), QString("Qt"));
    QCOMPARE(rec.value(2).toString(), QString("Qt"));
}

void tst_QSqlTableModel::insertRow()
{
    QFETCH(QString, dbName);
    QFETCH(int, submitpolicy_i);
    QSqlTableModel::EditStrategy submitpolicy = (QSqlTableModel::EditStrategy) submitpolicy_i;
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setEditStrategy(submitpolicy);
    model.setTable(test);
    model.setSort(0, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toInt(), 1);
    QCOMPARE(model.data(model.index(1, 0)).toInt(), 2);
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(1, 2)).toInt(), 2);
    QCOMPARE(model.data(model.index(2, 0)).toInt(), 3);
    QCOMPARE(model.data(model.index(2, 1)).toString(), QString("vohi"));
    QCOMPARE(model.data(model.index(2, 2)).toInt(), 3);

    QVERIFY(model.insertRow(2));

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toInt(), 1);
    QCOMPARE(model.data(model.index(1, 0)).toInt(), 2);
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(1, 2)).toInt(), 2);
    QCOMPARE(model.data(model.index(2, 0)).toInt(), 0);
    QCOMPARE(model.data(model.index(2, 1)).toString(), QString());
    QCOMPARE(model.data(model.index(2, 2)).toInt(), 0);
    QCOMPARE(model.data(model.index(3, 0)).toInt(), 3);
    QCOMPARE(model.data(model.index(3, 1)).toString(), QString("vohi"));
    QCOMPARE(model.data(model.index(3, 2)).toInt(), 3);

    QSqlRecord rec = model.record(1);
    rec.setValue(0, 42);
    rec.setValue(1, QString("francis"));

    // Setting record does not cause resort
    QVERIFY(model.setRecord(2, rec));

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toInt(), 1);
    QCOMPARE(model.data(model.index(1, 0)).toInt(), 2);
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(1, 2)).toInt(), 2);

    QCOMPARE(model.data(model.index(2, 0)).toInt(), 42);
    QCOMPARE(model.data(model.index(2, 1)).toString(), QString("francis"));
    QCOMPARE(model.data(model.index(2, 2)).toInt(), 2);
    QCOMPARE(model.data(model.index(3, 0)).toInt(), 3);
    QCOMPARE(model.data(model.index(3, 1)).toString(), QString("vohi"));
    QCOMPARE(model.data(model.index(3, 2)).toInt(), 3);

    QVERIFY(model.submitAll());

    if (submitpolicy == QSqlTableModel::OnManualSubmit) {
        // After the submit we should have the resorted view
        QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
        QCOMPARE(model.data(model.index(0, 2)).toInt(), 1);
        QCOMPARE(model.data(model.index(1, 0)).toInt(), 2);
        QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond"));
        QCOMPARE(model.data(model.index(1, 2)).toInt(), 2);
        QCOMPARE(model.data(model.index(2, 0)).toInt(), 3);
        QCOMPARE(model.data(model.index(2, 1)).toString(), QString("vohi"));
        QCOMPARE(model.data(model.index(2, 2)).toInt(), 3);
        QCOMPARE(model.data(model.index(3, 0)).toInt(), 42);
        QCOMPARE(model.data(model.index(3, 1)).toString(), QString("francis"));
        QCOMPARE(model.data(model.index(3, 2)).toInt(), 2);
    } else {
        // Submit does not select, therefore not resorted
        QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
        QCOMPARE(model.data(model.index(0, 2)).toInt(), 1);
        QCOMPARE(model.data(model.index(1, 0)).toInt(), 2);
        QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond"));
        QCOMPARE(model.data(model.index(1, 2)).toInt(), 2);
        QCOMPARE(model.data(model.index(2, 0)).toInt(), 42);
        QCOMPARE(model.data(model.index(2, 1)).toString(), QString("francis"));
        QCOMPARE(model.data(model.index(2, 2)).toInt(), 2);
        QCOMPARE(model.data(model.index(3, 0)).toInt(), 3);
        QCOMPARE(model.data(model.index(3, 1)).toString(), QString("vohi"));
        QCOMPARE(model.data(model.index(3, 2)).toInt(), 3);
    }

    QVERIFY(model.select());
    // After the select we should have the resorted view in all strategies
    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toInt(), 1);
    QCOMPARE(model.data(model.index(1, 0)).toInt(), 2);
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(1, 2)).toInt(), 2);
    QCOMPARE(model.data(model.index(2, 0)).toInt(), 3);
    QCOMPARE(model.data(model.index(2, 1)).toString(), QString("vohi"));
    QCOMPARE(model.data(model.index(2, 2)).toInt(), 3);
    QCOMPARE(model.data(model.index(3, 0)).toInt(), 42);
    QCOMPARE(model.data(model.index(3, 1)).toString(), QString("francis"));
    QCOMPARE(model.data(model.index(3, 2)).toInt(), 2);
}

void tst_QSqlTableModel::insertRowFailure()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    QFETCH(int, submitpolicy_i);
    QSqlTableModel::EditStrategy submitpolicy = (QSqlTableModel::EditStrategy) submitpolicy_i;
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setTable(qTableName("pktest", __FILE__, db));
    model.setEditStrategy(submitpolicy);

    QSqlRecord values = model.record();
    values.setValue(0, 42);
    values.setGenerated(0, true);
    values.setValue(1, QString("blah"));
    values.setGenerated(1, true);

    // populate 1 row
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    QVERIFY_SQL(model, insertRecord(0, values));
    QVERIFY_SQL(model, submitAll());
    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 0)).toInt(), 42);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("blah"));

    // primary key conflict will succeed in model but fail in database
    QVERIFY_SQL(model, insertRow(0));
    QVERIFY_SQL(model, setData(model.index(0, 0), 42));
    QVERIFY_SQL(model, setData(model.index(0, 1), "conflict"));
    QFAIL_SQL(model, submitAll());

    // failed insert is still cached
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("conflict"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("blah"));

    // cached insert affects subsequent operations
    values.setValue(1, QString("spam"));
    if (submitpolicy != QSqlTableModel::OnManualSubmit) {
        QFAIL_SQL(model, setData(model.index(1, 1), QString("eggs")));
        QCOMPARE(model.data(model.index(1, 1)).toString(), QString("blah"));
        QFAIL_SQL(model, setRecord(1, values));
        QCOMPARE(model.data(model.index(1, 1)).toString(), QString("blah"));
        QFAIL_SQL(model, insertRow(2));
        QCOMPARE(model.rowCount(), 2);
        QFAIL_SQL(model, removeRow(1));
        QCOMPARE(model.rowCount(), 2);
    } else {
        QVERIFY_SQL(model, setData(model.index(1, 1), QString("eggs")));
        QCOMPARE(model.data(model.index(1, 1)).toString(), QString("eggs"));
        QVERIFY_SQL(model, setRecord(1, values));
        QCOMPARE(model.data(model.index(1, 1)).toString(), QString("spam"));
        QVERIFY_SQL(model, insertRow(2));
        QCOMPARE(model.rowCount(), 3);
        QVERIFY_SQL(model, removeRow(1));
        QCOMPARE(model.rowCount(), 3);
    }

    // restore empty table
    model.revertAll();
    QVERIFY_SQL(model, removeRow(0));
    QVERIFY_SQL(model, submitAll());
    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 0);
}

void tst_QSqlTableModel::insertRecord()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    model.setTable(test);
    model.setSort(0, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

    QSqlRecord rec = model.record();
    rec.setValue(0, 42);
    rec.setValue(1, QString("vohi"));
    rec.setValue(2, 1);
    QVERIFY(model.insertRecord(1, rec));
    QCOMPARE(model.rowCount(), 4);

    QCOMPARE(model.data(model.index(1, 0)).toInt(), 42);
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("vohi"));
    QCOMPARE(model.data(model.index(1, 2)).toInt(), 1);

    model.revertAll();
    model.setEditStrategy(QSqlTableModel::OnRowChange);

    QVERIFY(model.insertRecord(-1, rec));

    QCOMPARE(model.data(model.index(3, 0)).toInt(), 42);
    QCOMPARE(model.data(model.index(3, 1)).toString(), QString("vohi"));
    QCOMPARE(model.data(model.index(3, 2)).toInt(), 1);
}

void tst_QSqlTableModel::insertMultiRecords()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    model.setTable(test);
    model.setSort(0, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.rowCount(), 3);

    QVERIFY(model.insertRow(2));

    QCOMPARE(model.data(model.index(2, 0)), QVariant(model.record().field(0).type()));
    QCOMPARE(model.data(model.index(2, 1)), QVariant(model.record().field(1).type()));
    QCOMPARE(model.data(model.index(2, 2)), QVariant(model.record().field(2).type()));

    QVERIFY(model.insertRow(3));
    QVERIFY(model.insertRow(0));

    QCOMPARE(model.data(model.index(5, 0)).toInt(), 3);
    QCOMPARE(model.data(model.index(5, 1)).toString(), QString("vohi"));
    QCOMPARE(model.data(model.index(5, 2)).toInt(), 3);

    QVERIFY(model.setData(model.index(0, 0), QVariant(42)));
    QVERIFY(model.setData(model.index(3, 0), QVariant(43)));
    QVERIFY(model.setData(model.index(4, 0), QVariant(44)));
    QVERIFY(model.setData(model.index(4, 1), QVariant(QLatin1String("gunnar"))));
    QVERIFY(model.setData(model.index(4, 2), QVariant(1)));

    QVERIFY(model.submitAll());
    model.clear();
    model.setTable(test);
    model.setSort(0, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(1, 0)).toInt(), 2);
    QCOMPARE(model.data(model.index(2, 0)).toInt(), 3);
    QCOMPARE(model.data(model.index(3, 0)).toInt(), 42);
    QCOMPARE(model.data(model.index(4, 0)).toInt(), 43);
    QCOMPARE(model.data(model.index(5, 0)).toInt(), 44);
    QCOMPARE(model.data(model.index(5, 1)).toString(), QString("gunnar"));
    QCOMPARE(model.data(model.index(5, 2)).toInt(), 1);
}

void tst_QSqlTableModel::insertWithAutoColumn()
{
    QFETCH(QString, dbName);
    QFETCH(int, submitpolicy_i);
    QSqlTableModel::EditStrategy submitpolicy = (QSqlTableModel::EditStrategy) submitpolicy_i;
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QString tbl = qTableName("autoColumnTest", __FILE__, db);
    QSqlQuery q(db);
    q.exec("DROP TABLE " + tbl);
    QVERIFY_SQL(q, exec("CREATE TABLE " + tbl + "(id INTEGER PRIMARY KEY AUTOINCREMENT, val TEXT)"));

    QSqlTableModel model(0, db);
    model.setTable(tbl);
    model.setSort(0, Qt::AscendingOrder);
    model.setEditStrategy(submitpolicy);

    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 0);

    // For insertRow/insertRows, we have to touch at least one column
    // or else the generated flag won't be set, which would lead to
    // an empty column list in the INSERT statement, which generally
    // does not work.
    if (submitpolicy != QSqlTableModel::OnManualSubmit) {
        for (int id = 1; id <= 2; ++id) {
            QVERIFY_SQL(model, insertRow(0));
            QVERIFY_SQL(model, setData(model.index(0, 1), QString("foo")));
            QVERIFY_SQL(model, submit());
            QCOMPARE(model.data(model.index(0, 0)).toInt(), id);
        }
    } else {
        QVERIFY_SQL(model, insertRows(0, 2));
        QVERIFY_SQL(model, setData(model.index(0, 1), QString("foo")));
        QVERIFY_SQL(model, setData(model.index(1, 1), QString("foo")));
    }

    QCOMPARE(model.rowCount(), 2);

    QSqlRecord rec = db.record(tbl);
    QVERIFY(rec.field(0).isAutoValue());
    rec.setGenerated(0, false);

    QVERIFY_SQL(model, insertRecord(0, rec));
    if (submitpolicy != QSqlTableModel::OnManualSubmit)
        QCOMPARE(model.data(model.index(0, 0)).toInt(), 3);

    QCOMPARE(model.rowCount(), 3);

    if (submitpolicy != QSqlTableModel::OnManualSubmit) {
        // Rows updated in original positions after previous submits.
        QCOMPARE(model.data(model.index(0, 0)).toInt(), 3);
        QCOMPARE(model.data(model.index(1, 0)).toInt(), 2);
        QCOMPARE(model.data(model.index(2, 0)).toInt(), 1);
    } else {
        // Manual submit is followed by requery.
        QVERIFY_SQL(model, submitAll());
        QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
        QCOMPARE(model.data(model.index(1, 0)).toInt(), 2);
        QCOMPARE(model.data(model.index(2, 0)).toInt(), 3);
    }

    QVERIFY_SQL(q, exec("DROP TABLE " + tbl));
}

void tst_QSqlTableModel::submitAll()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setTable(test);
    model.setSort(0, Qt::AscendingOrder);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    QVERIFY_SQL(model, select());

    QVERIFY(model.setData(model.index(0, 1), "harry2", Qt::EditRole));
    QVERIFY(model.setData(model.index(1, 1), "trond2", Qt::EditRole));

    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry2"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond2"));

    QVERIFY_SQL(model, submitAll());

    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry2"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond2"));

    QVERIFY(model.setData(model.index(0, 1), "harry", Qt::EditRole));
    QVERIFY(model.setData(model.index(1, 1), "trond", Qt::EditRole));

    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond"));

    QVERIFY_SQL(model, submitAll());

    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond"));
}

void tst_QSqlTableModel::removeRow()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setTable(test);
    model.setSort(0, Qt::AscendingOrder);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 3);

    // headerDataChanged must be emitted by the model since the row won't vanish until select
    qRegisterMetaType<Qt::Orientation>("Qt::Orientation");
    QSignalSpy headerDataChangedSpy(&model, SIGNAL(headerDataChanged(Qt::Orientation,int,int)));

    QVERIFY(model.removeRow(1));
    QCOMPARE(headerDataChangedSpy.count(), 1);
    QCOMPARE(*static_cast<const Qt::Orientation *>(headerDataChangedSpy.at(0).value(0).constData()), Qt::Vertical);
    QCOMPARE(headerDataChangedSpy.at(0).at(1).toInt(), 1);
    QCOMPARE(headerDataChangedSpy.at(0).at(2).toInt(), 1);
    QVERIFY(model.submitAll());
    QCOMPARE(model.rowCount(), 2);

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(1, 0)).toInt(), 3);
    model.clear();

    recreateTestTables();

    model.setTable(test);
    model.setEditStrategy(QSqlTableModel::OnRowChange);
    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 3);

    headerDataChangedSpy.clear();
    QVERIFY(model.removeRow(1));
    QCOMPARE(headerDataChangedSpy.count(), 1);
    QCOMPARE(model.rowCount(), 3);

    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 2);

    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("vohi"));
}

void tst_QSqlTableModel::removeRows()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setTable(test);
    model.setSort(0, Qt::AscendingOrder);
    model.setEditStrategy(QSqlTableModel::OnFieldChange);
    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 3);

    QSignalSpy beforeDeleteSpy(&model, SIGNAL(beforeDelete(int)));

    // Make sure wrong stuff is ok
    QVERIFY(!model.removeRows(-1,1)); // negative start
    QVERIFY(!model.removeRows(-1, 0)); // negative start, and zero count
    QVERIFY(!model.removeRows(1, 0)); // zero count
    QVERIFY(!model.removeRows(5, 1)); // past end (DOESN'T causes a beforeDelete to be emitted)
    QVERIFY(!model.removeRows(1, 0, model.index(2, 0))); // can't pass a valid modelindex
    QFAIL_SQL(model, removeRows(0, 2)); // more than 1 row on OnFieldChange

    QVERIFY_SQL(model, removeRows(0, 1));
    QVERIFY_SQL(model, removeRows(1, 1));
    QCOMPARE(beforeDeleteSpy.count(), 2);
    QCOMPARE(beforeDeleteSpy.at(0).at(0).toInt(), 0);
    QCOMPARE(beforeDeleteSpy.at(1).at(0).toInt(), 1);
    // deleted rows shown as empty until select
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString(""));
    QVERIFY(model.select());
    // deleted rows are gone
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("vohi"));
    model.clear();

    recreateTestTables();
    model.setTable(test);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 3);
    beforeDeleteSpy.clear();

    // When the edit strategy is OnManualSubmit the beforeDelete() signal
    // isn't emitted until submitAll() is called.

    QVERIFY(!model.removeRows(-1,1)); // negative start
    QVERIFY(!model.removeRows(-1, 0)); // negative start, and zero count
    QVERIFY(!model.removeRows(1, 0)); // zero count
    QVERIFY(!model.removeRows(5, 1)); // past end (DOESN'T cause a beforeDelete to be emitted)
    QVERIFY(!model.removeRows(1, 0, model.index(2, 0))); // can't pass a valid modelindex

    qRegisterMetaType<Qt::Orientation>("Qt::Orientation");
    QSignalSpy headerDataChangedSpy(&model, SIGNAL(headerDataChanged(Qt::Orientation,int,int)));
    QVERIFY(model.removeRows(0, 2, QModelIndex()));
    QCOMPARE(headerDataChangedSpy.count(), 2);
    QCOMPARE(headerDataChangedSpy.at(0).at(1).toInt(), 1);
    QCOMPARE(headerDataChangedSpy.at(0).at(2).toInt(), 1);
    QCOMPARE(headerDataChangedSpy.at(1).at(1).toInt(), 0);
    QCOMPARE(headerDataChangedSpy.at(1).at(2).toInt(), 0);
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(beforeDeleteSpy.count(), 0);
    QVERIFY(model.submitAll());
    QCOMPARE(beforeDeleteSpy.count(), 2);
    QCOMPARE(beforeDeleteSpy.at(0).at(0).toInt(), 0);
    QCOMPARE(beforeDeleteSpy.at(1).at(0).toInt(), 1);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("vohi"));
}

void tst_QSqlTableModel::removeInsertedRow()
{
    QFETCH(QString, dbName);
    QFETCH(int, submitpolicy_i);
    QSqlTableModel::EditStrategy submitpolicy = (QSqlTableModel::EditStrategy) submitpolicy_i;
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setTable(test);
    model.setSort(0, Qt::AscendingOrder);

    model.setEditStrategy(submitpolicy);
    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 3);

    QVERIFY(model.insertRow(1));
    QCOMPARE(model.rowCount(), 4);

    QVERIFY(model.removeRow(1));
    QCOMPARE(model.rowCount(), 3);

    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(2, 1)).toString(), QString("vohi"));

    // Now insert a row with a null, and check that removing it also works (QTBUG-15979 etc)
    model.insertRow(1);
    model.setData(model.index(1,0), 55);
    model.setData(model.index(1,1), QString("null columns"));
    model.setData(model.index(1,2), QVariant());

    model.submitAll();

    if (model.editStrategy() != QSqlTableModel::OnManualSubmit) {
        QCOMPARE(model.rowCount(), 4);
        QCOMPARE(model.data(model.index(1, 0)).toInt(), 55);
        QCOMPARE(model.data(model.index(1, 1)).toString(), QString("null columns"));
        QCOMPARE(model.data(model.index(1, 2)).isNull(), true);
        QVERIFY(model.select());
    }

    QCOMPARE(model.rowCount(), 4);
    QCOMPARE(model.data(model.index(3, 0)).toInt(), 55);
    QCOMPARE(model.data(model.index(3, 1)).toString(), QString("null columns"));
    QCOMPARE(model.data(model.index(3, 2)).isNull(), true);

    QVERIFY(model.removeRow(3));
    model.submitAll();

    if (model.editStrategy() != QSqlTableModel::OnManualSubmit) {
        QCOMPARE(model.rowCount(), 4);
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
        QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond"));
        QCOMPARE(model.data(model.index(2, 1)).toString(), QString("vohi"));
        QCOMPARE(model.data(model.index(3, 1)).toString(), QString(""));
        QVERIFY(model.select());
    }

    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(2, 1)).toString(), QString("vohi"));
}

void tst_QSqlTableModel::removeInsertedRows()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setTable(test);
    model.setSort(0, Qt::AscendingOrder);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit); // you can't insert more than one row otherwise
    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 3);

    // First put two empty rows, and remove them one by one
    QVERIFY(model.insertRows(1, 2));
    QCOMPARE(model.rowCount(), 5);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString());
    QCOMPARE(model.data(model.index(2, 1)).toString(), QString());
    QCOMPARE(model.data(model.index(3, 1)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(4, 1)).toString(), QString("vohi"));

    QVERIFY(model.removeRow(1));
    QCOMPARE(model.rowCount(), 4);

    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString());
    QCOMPARE(model.data(model.index(2, 1)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(3, 1)).toString(), QString("vohi"));

    QVERIFY(model.removeRow(1));
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(2, 1)).toString(), QString("vohi"));

    // Now put two empty rows, and remove them all at once
    QVERIFY(model.insertRows(1, 2));
    QCOMPARE(model.rowCount(), 5);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString());
    QCOMPARE(model.data(model.index(2, 1)).toString(), QString());
    QCOMPARE(model.data(model.index(3, 1)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(4, 1)).toString(), QString("vohi"));

    QVERIFY(model.removeRows(1, 2));
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(2, 1)).toString(), QString("vohi"));


    // Now put two empty rows, and remove one good and two empty
    QVERIFY(model.insertRows(1, 2));
    QCOMPARE(model.rowCount(), 5);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString());
    QCOMPARE(model.data(model.index(2, 1)).toString(), QString());
    QCOMPARE(model.data(model.index(3, 1)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(4, 1)).toString(), QString("vohi"));

    QVERIFY(model.removeRows(0, 3));
    QVERIFY(model.submitAll()); // otherwise the remove of the real row doesn't work

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("vohi"));

    // Reset back again
    model.clear();
    recreateTestTables();
    model.setTable(test);
    model.setSort(0, Qt::AscendingOrder);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit); // you can't insert more than one row otherwise
    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 3);

    // Now two empty and one good
    QVERIFY(model.insertRows(1, 2));
    QCOMPARE(model.rowCount(), 5);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString());
    QCOMPARE(model.data(model.index(2, 1)).toString(), QString());
    QCOMPARE(model.data(model.index(3, 1)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(4, 1)).toString(), QString("vohi"));

    QVERIFY(model.removeRows(1, 3));
    QVERIFY(model.submitAll());
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("vohi"));

    // Reset back again
    model.clear();
    recreateTestTables();
    model.setTable(test);
    model.setSort(0, Qt::AscendingOrder);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit); // you can't insert more than one row otherwise
    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 3);

    // one empty, one good, one empty
    QVERIFY(model.insertRows(1, 1));
    QVERIFY(model.insertRows(3, 1));
    QCOMPARE(model.rowCount(), 5);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString());
    QCOMPARE(model.data(model.index(2, 1)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(3, 1)).toString(), QString());
    QCOMPARE(model.data(model.index(4, 1)).toString(), QString("vohi"));

    QVERIFY(model.removeRows(1, 3));
    QVERIFY(model.submitAll());
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("vohi"));
}

void tst_QSqlTableModel::revert()
{
    QFETCH(QString, dbName);
    QFETCH(int, submitpolicy_i);
    QSqlTableModel::EditStrategy submitpolicy = (QSqlTableModel::EditStrategy) submitpolicy_i;
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QString tblA = qTableName("revertATest", __FILE__, db);
    QString tblB = qTableName("revertBTest", __FILE__, db);
    QSqlQuery q(db);
    q.exec("PRAGMA foreign_keys = ON;");
    q.exec("DROP TABLE " + tblB);
    q.exec("DROP TABLE " + tblA);
    QVERIFY_SQL(q, exec("CREATE TABLE " + tblA + "(a INT PRIMARY KEY)"));
    QVERIFY_SQL(q, exec("CREATE TABLE " + tblB + "(b INT PRIMARY KEY, FOREIGN KEY (b) REFERENCES " + tblA + " (a))"));
    QVERIFY_SQL(q, exec("INSERT INTO " + tblA + "(a) VALUES (1)"));
    QVERIFY_SQL(q, exec("INSERT INTO " + tblB + "(b) VALUES (1)"));
    if (q.exec("UPDATE " + tblA + " SET a = -1"))
        QSKIP("database does not enforce foreign key constraints, skipping test");

    QSqlTableModel model(0, db);
    model.setTable(tblA);
    model.setSort(0, Qt::AscendingOrder);
    model.setEditStrategy(submitpolicy);

    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 1);
    QFAIL_SQL(model, isDirty());

    // don't crash if there is no change
    model.revert();

    // UPDATE
    // invalid value makes submit fail leaving pending update in cache
    const QModelIndex idx = model.index(0, 0);
    if (submitpolicy == QSqlTableModel::OnFieldChange)
        QFAIL_SQL(model, setData(idx, int(-1)));
    else
        QVERIFY_SQL(model, setData(idx, int(-1)));
    QVERIFY_SQL(model, isDirty(idx));
    model.revert();
    if (submitpolicy != QSqlTableModel::OnManualSubmit)
        QFAIL_SQL(model, isDirty(idx));
    else
        QVERIFY_SQL(model, isDirty(idx));

    // INSERT
    QVERIFY_SQL(model, select());
    // insertRow() does not submit leaving pending insert in cache
    QVERIFY_SQL(model, insertRow(0));
    QCOMPARE(model.rowCount(), 2);
    QVERIFY_SQL(model, isDirty());
    model.revert();
    if (submitpolicy != QSqlTableModel::OnManualSubmit)
        QFAIL_SQL(model, isDirty());
    else
        QVERIFY_SQL(model, isDirty());

    // DELETE
    QVERIFY_SQL(model, select());
    // foreign key makes submit fail leaving pending delete in cache
    if (submitpolicy == QSqlTableModel::OnManualSubmit)
        QVERIFY_SQL(model, removeRow(0));
    else
        QFAIL_SQL(model, removeRow(0));
    QVERIFY_SQL(model, isDirty());
    model.revert();
    if (submitpolicy != QSqlTableModel::OnManualSubmit)
        QFAIL_SQL(model, isDirty());
    else
        QVERIFY_SQL(model, isDirty());

    q.exec("DROP TABLE " + tblB);
    q.exec("DROP TABLE " + tblA);
}

void tst_QSqlTableModel::isDirty()
{
    QFETCH(QString, dbName);
    QFETCH(int, submitpolicy_i);
    QSqlTableModel::EditStrategy submitpolicy = (QSqlTableModel::EditStrategy) submitpolicy_i;
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setEditStrategy(submitpolicy);
    model.setTable(test);
    QFAIL_SQL(model, isDirty());

    model.setSort(0, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());
    QFAIL_SQL(model, isDirty());

    // check that setting the current value does not add to the cache
    {
        QModelIndex i = model.index(0, 1);
        QVariant v = model.data(i, Qt::EditRole);
        QVERIFY_SQL(model, setData(i, v));
        QFAIL_SQL(model, isDirty());
    }

    if (submitpolicy != QSqlTableModel::OnFieldChange) {
        // setData() followed by revertAll()
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
        QVERIFY_SQL(model, setData(model.index(0, 1), QString("sam i am")));
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("sam i am"));
        QVERIFY_SQL(model, isDirty());
        QVERIFY_SQL(model, isDirty(model.index(0, 1)));
        model.revertAll();
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
        QFAIL_SQL(model, isDirty());
        QFAIL_SQL(model, isDirty(model.index(0, 1)));

        // setData() followed by select(), which clears changes
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
        QVERIFY_SQL(model, setData(model.index(0, 1), QString("sam i am")));
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("sam i am"));
        QVERIFY_SQL(model, isDirty());
        QVERIFY_SQL(model, isDirty(model.index(0, 1)));
        QVERIFY_SQL(model, select());
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
        QFAIL_SQL(model, isDirty());
        QFAIL_SQL(model, isDirty(model.index(0, 1)));
    }

    if (submitpolicy == QSqlTableModel::OnRowChange) {
        // dirty row must block change on other rows
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
        QVERIFY(model.rowCount() > 1);
        QVERIFY_SQL(model, setData(model.index(0, 1), QString("sam i am")));
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("sam i am"));
        QVERIFY_SQL(model, isDirty());
        QVERIFY_SQL(model, isDirty(model.index(0, 1)));
        QVERIFY(!(model.flags(model.index(1, 1)) & Qt::ItemIsEditable));
        QFAIL_SQL(model, setData(model.index(1, 1), QString("sam i am")));
        QFAIL_SQL(model, setRecord(1, model.record(1)));
        QFAIL_SQL(model, insertRow(1));
        QFAIL_SQL(model, removeRow(1));
        QFAIL_SQL(model, isDirty(model.index(1, 1)));

        model.revertAll();
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
        QFAIL_SQL(model, isDirty());
        QFAIL_SQL(model, isDirty(model.index(0, 1)));
    }

    // setData() followed by submitAll()
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QVERIFY_SQL(model, setData(model.index(0, 1), QString("sam i am")));
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("sam i am"));
    if (submitpolicy != QSqlTableModel::OnFieldChange) {
        QVERIFY_SQL(model, isDirty());
        QVERIFY_SQL(model, isDirty(model.index(0, 1)));
    }
    QVERIFY_SQL(model, submitAll());
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("sam i am"));
    QFAIL_SQL(model, isDirty());
    QFAIL_SQL(model, isDirty(model.index(0, 1)));
    // check status after refreshing underlying query
    QVERIFY_SQL(model, select());
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("sam i am"));
    QFAIL_SQL(model, isDirty());
    QFAIL_SQL(model, isDirty(model.index(0, 1)));
    //restore original state
    QVERIFY_SQL(model, setData(model.index(0, 1), QString("harry")));
    QVERIFY_SQL(model, submitAll());
    QVERIFY_SQL(model, select());
    QFAIL_SQL(model, isDirty());
    QFAIL_SQL(model, isDirty(model.index(0, 1)));

    QSqlRecord newvals = model.record(0);
    newvals.setValue(1, QString("sam i am"));
    newvals.setGenerated(1, true);
    if (submitpolicy == QSqlTableModel::OnManualSubmit) {
        // setRecord() followed by revertAll()
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
        QVERIFY_SQL(model, setRecord(0, newvals));
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("sam i am"));
        QVERIFY_SQL(model, isDirty());
        QVERIFY_SQL(model, isDirty(model.index(0, 1)));
        model.revertAll();
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
        QFAIL_SQL(model, isDirty());
        QFAIL_SQL(model, isDirty(model.index(0, 1)));

        // setRecord() followed by select(), which clears changes
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
        QVERIFY_SQL(model, setRecord(0, newvals));
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("sam i am"));
        QVERIFY_SQL(model, isDirty());
        QVERIFY_SQL(model, isDirty(model.index(0, 1)));
        QVERIFY_SQL(model, select());
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
        QFAIL_SQL(model, isDirty());
        QFAIL_SQL(model, isDirty(model.index(0, 1)));
    }

    // setRecord() followed by submitAll()
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QVERIFY_SQL(model, setRecord(0, newvals));
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("sam i am"));
    if (submitpolicy == QSqlTableModel::OnManualSubmit) {
        QVERIFY_SQL(model, isDirty());
        QVERIFY_SQL(model, isDirty(model.index(0, 1)));
    }
    QVERIFY_SQL(model, submitAll());
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("sam i am"));
    QFAIL_SQL(model, isDirty());
    QFAIL_SQL(model, isDirty(model.index(0, 1)));
    // check status after refreshing underlying query
    QVERIFY_SQL(model, select());
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("sam i am"));
    QFAIL_SQL(model, isDirty());
    QFAIL_SQL(model, isDirty(model.index(0, 1)));
    //restore original state
    QVERIFY_SQL(model, setData(model.index(0, 1), QString("harry")));
    QVERIFY_SQL(model, submitAll());
    QVERIFY_SQL(model, select());
    QFAIL_SQL(model, isDirty());
    QFAIL_SQL(model, isDirty(model.index(0, 1)));

    // insertRow()
    QVERIFY_SQL(model, insertRow(0));
    QVERIFY_SQL(model, isDirty());
    QVERIFY_SQL(model, isDirty(model.index(0, 1)));
    model.revertAll();
    QFAIL_SQL(model, isDirty());
    QFAIL_SQL(model, isDirty(model.index(0, 1)));
    QVERIFY_SQL(model, select());
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QFAIL_SQL(model, isDirty());
    QFAIL_SQL(model, isDirty(model.index(0, 1)));

    // removeRow()
    QSqlRecord saved_rec = model.record(0);
    QVERIFY_SQL(model, removeRow(0));
    if (submitpolicy == QSqlTableModel::OnManualSubmit) {
        QVERIFY_SQL(model, isDirty());
        QVERIFY_SQL(model, isDirty(model.index(0, 1)));
    }
    QVERIFY_SQL(model, submitAll());
    QFAIL_SQL(model, isDirty());
    QFAIL_SQL(model, isDirty(model.index(0, 1)));
    QVERIFY_SQL(model, select());
    QFAIL_SQL(model, isDirty());
    QFAIL_SQL(model, isDirty(model.index(0, 1)));
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("trond"));

    // insertRecord(), put back the removed row
    for (int i = saved_rec.count() - 1; i >= 0; --i)
        saved_rec.setGenerated(i, true);
    QVERIFY_SQL(model, insertRecord(0, saved_rec));
    if (submitpolicy == QSqlTableModel::OnManualSubmit) {
        QVERIFY_SQL(model, isDirty());
        QVERIFY_SQL(model, isDirty(model.index(0, 1)));
    }
    QVERIFY_SQL(model, submitAll());
    QFAIL_SQL(model, isDirty());
    QFAIL_SQL(model, isDirty(model.index(0, 1)));
    QVERIFY_SQL(model, select());
    QFAIL_SQL(model, isDirty());
    QFAIL_SQL(model, isDirty(model.index(0, 1)));
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
}

void tst_QSqlTableModel::emptyTable()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.columnCount(), 0);

    model.setTable(qTableName("emptytable", __FILE__, db));
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.columnCount(), 1);

    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.columnCount(), 1);

    // QTBUG-29108: check correct horizontal header for empty query with pending insert
    QCOMPARE(model.headerData(0, Qt::Horizontal).toString(), QString("id"));
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    model.insertRow(0);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.headerData(0, Qt::Horizontal).toString(), QString("id"));
    model.revertAll();
}

void tst_QSqlTableModel::tablesAndSchemas()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    q.exec("DROP SCHEMA " + qTableName("testschema", __FILE__, db) + " CASCADE");
    QVERIFY_SQL( q, exec("create schema " + qTableName("testschema", __FILE__, db)));
    QString tableName = qTableName("testschema", __FILE__, db) + '.' + qTableName("testtable", __FILE__, db);
    QVERIFY_SQL( q, exec("create table " + tableName + "(id int)"));
    QVERIFY_SQL( q, exec("insert into " + tableName + " values(1)"));
    QVERIFY_SQL( q, exec("insert into " + tableName + " values(2)"));

    QSqlTableModel model(0, db);
    model.setTable(tableName);
    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.columnCount(), 1);
}

void tst_QSqlTableModel::whitespaceInIdentifiers()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (!testWhiteSpaceNames(db.driverName()))
        QSKIP("DBMS doesn't support whitespaces in identifiers");

    QString tableName = qTableName("qtestw hitespace", db);

    QSqlTableModel model(0, db);
    model.setTable(tableName);
    QVERIFY_SQL(model, select());
}

void tst_QSqlTableModel::primaryKeyOrder()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);

    QSqlQuery q(db);

    if (dbType == QSqlDriver::PostgreSQL)
        QVERIFY_SQL( q, exec("set client_min_messages='warning'"));

    QVERIFY_SQL(q, exec("create table " + qTableName("foo", __FILE__, db) + "(a varchar(20), id int not null primary key, b varchar(20))"));

    QSqlTableModel model(0, db);
    model.setTable(qTableName("foo", __FILE__, db));

    QSqlIndex pk = model.primaryKey();
    QCOMPARE(pk.count(), 1);
    QCOMPARE(pk.fieldName(0), QLatin1String("id"));

    QVERIFY(model.insertRow(0));
    QVERIFY(model.setData(model.index(0, 0), "hello"));
    QVERIFY(model.setData(model.index(0, 1), 42));
    QVERIFY(model.setData(model.index(0, 2), "blah"));
    QVERIFY_SQL(model, submitAll());

    QVERIFY(model.setData(model.index(0, 1), 43));
    QVERIFY_SQL(model, submitAll());

    QCOMPARE(model.data(model.index(0, 1)).toInt(), 43);
}

void tst_QSqlTableModel::setInvalidFilter()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    // set an invalid filter, make sure it fails
    QSqlTableModel model(0, db);
    model.setTable(test);
    model.setFilter("blahfahsel");

    QCOMPARE(model.filter(), QString("blahfahsel"));
    QVERIFY(!model.select());

    // set a valid filter later, make sure if passes
    model.setFilter("id = 1");
    QVERIFY_SQL(model, select());
}

void tst_QSqlTableModel::setFilter()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setTable(test);
    model.setFilter("id = 1");
    QCOMPARE(model.filter(), QString("id = 1"));
    QVERIFY_SQL(model, select());

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);

    QSignalSpy modelAboutToBeResetSpy(&model, SIGNAL(modelAboutToBeReset()));
    QSignalSpy modelResetSpy(&model, SIGNAL(modelReset()));
    model.setFilter("id = 2");

    // check the signals
    QCOMPARE(modelAboutToBeResetSpy.count(), 1);
    QCOMPARE(modelResetSpy.count(), 1);

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 0)).toInt(), 2);
}

void tst_QSqlTableModel::sqlite_bigTable()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QString bigtable(qTableName("bigtable", __FILE__, db));

    bool hasTransactions = db.driver()->hasFeature(QSqlDriver::Transactions);
    if (hasTransactions) QVERIFY(db.transaction());
    QSqlQuery q(db);
    QVERIFY_SQL( q, exec("create table "+bigtable+"(id int primary key, name varchar)"));
    QVERIFY_SQL( q, prepare("insert into "+bigtable+"(id, name) values (?, ?)"));
    QElapsedTimer timing;
    timing.start();
    for (int i = 0; i < 10000; ++i) {
        q.addBindValue(i);
        q.addBindValue(QString::number(i));
        if (i % 1000 == 0 && timing.elapsed() > 5000)
            qDebug() << i << "records written";
        QVERIFY_SQL( q, exec());
    }
    q.clear();
    if (hasTransactions) QVERIFY(db.commit());

    QSqlTableModel model(0, db);
    model.setTable(bigtable);
    QVERIFY_SQL(model, select());

    QSqlRecord rec = model.record();
    rec.setValue("id", 424242);
    rec.setValue("name", "Guillaume");
    QVERIFY_SQL(model, insertRecord(-1, rec));

    model.clear();
}

// For task 118547: couldn't insert records unless select()
// had first been called.
void tst_QSqlTableModel::insertRecordBeforeSelect()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setTable(test);
    QCOMPARE(model.lastError().type(), QSqlError::NoError);

    QSqlRecord buffer = model.record();
    buffer.setValue("id", 13);
    buffer.setValue("name", QString("The Lion King"));
    buffer.setValue("title", 0);
    QVERIFY_SQL(model, insertRecord(-1, buffer));

    buffer.setValue("id", 26);
    buffer.setValue("name", QString("T. Leary"));
    buffer.setValue("title", 0);
    QVERIFY_SQL(model, insertRecord(1, buffer));

    if (model.editStrategy() != QSqlTableModel::OnManualSubmit) {
        QCOMPARE(model.rowCount(), 2);
        QVERIFY_SQL(model, select());
    }

    int rowCount = model.rowCount();
    model.clear();
    QCOMPARE(model.rowCount(), 0);

    QSqlTableModel model2(0, db);
    model2.setTable(test);
    QVERIFY_SQL(model2, select());
    QCOMPARE(model2.rowCount(), rowCount);
}

// For task 118547: set errors if table doesn't exist and if records
// are inserted and submitted on a non-existing table.
void tst_QSqlTableModel::submitAllOnInvalidTable()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);

    // setTable returns a void, so the error can only be caught by
    // manually checking lastError(). ### Qt5: This should be changed!
    model.setTable(qTableName("invalidTable", __FILE__, db));
    QCOMPARE(model.lastError().type(), QSqlError::StatementError);

    // This will give us an empty record which is expected behavior
    QSqlRecord buffer = model.record();
    buffer.setValue("bogus", 1000);
    buffer.setValue("bogus2", QString("I will go nowhere!"));

    // Inserting the record into the *model* will work (OnManualSubmit)
    QVERIFY_SQL(model, insertRecord(-1, buffer));

    // The submit and select shall fail because the table doesn't exist
    QEXPECT_FAIL("", "The table doesn't exist: submitAll() shall fail",
        Continue);
    QVERIFY_SQL(model, submitAll());
    QEXPECT_FAIL("", "The table doesn't exist: select() shall fail",
        Continue);
    QVERIFY_SQL(model, select());
}

// For task 147575: the rowsRemoved signal emitted from the model was lying
void tst_QSqlTableModel::insertRecordsInLoop()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setTable(test);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    model.select();

    QSqlRecord record = model.record();
    record.setValue(0, 10);
    record.setValue(1, "Testman");
    record.setValue(2, 1);

    QSignalSpy modelAboutToBeResetSpy(&model, SIGNAL(modelAboutToBeReset()));
    QSignalSpy modelResetSpy(&model, SIGNAL(modelReset()));
    QSignalSpy spyRowsInserted(&model, SIGNAL(rowsInserted(QModelIndex,int,int)));
    for (int i = 0; i < 10; i++) {
        QVERIFY(model.insertRecord(model.rowCount(), record));
        QCOMPARE(spyRowsInserted.at(i).at(1).toInt(), i+3); // The table already contains three rows
        QCOMPARE(spyRowsInserted.at(i).at(2).toInt(), i+3);
    }
    model.submitAll(); // submitAll() calls select() which clears and repopulates the table

    // model emits reset signals
    QCOMPARE(modelAboutToBeResetSpy.count(), 1);
    QCOMPARE(modelResetSpy.count(), 1);

    QCOMPARE(model.rowCount(), 13);
    QCOMPARE(model.columnCount(), 3);
}

void tst_QSqlTableModel::sqlite_attachedDatabase()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    if(db.databaseName() == ":memory:")
        QSKIP(":memory: database, skipping test");

    QSqlDatabase attachedDb = QSqlDatabase::cloneDatabase(db, db.driverName() + QLatin1String("attached"));
    attachedDb.setDatabaseName(db.databaseName()+QLatin1String("attached.dat"));
    QVERIFY_SQL(attachedDb, open());
    QSqlQuery q(attachedDb);
    tst_Databases::safeDropTables(attachedDb, QStringList() << "atest" << "atest2");
    QVERIFY_SQL( q, exec("CREATE TABLE atest(id int, text varchar(20))"));
    QVERIFY_SQL( q, exec("CREATE TABLE atest2(id int, text varchar(20))"));
    QVERIFY_SQL( q, exec("INSERT INTO atest VALUES(1, 'attached-atest')"));
    QVERIFY_SQL( q, exec("INSERT INTO atest2 VALUES(2, 'attached-atest2')"));

    QSqlQuery q2(db);
    tst_Databases::safeDropTable(db, "atest");
    QVERIFY_SQL(q2, exec("CREATE TABLE atest(id int, text varchar(20))"));
    QVERIFY_SQL(q2, exec("INSERT INTO atest VALUES(3, 'main')"));
    QVERIFY_SQL(q2, exec("ATTACH DATABASE \""+attachedDb.databaseName()+"\" as adb"));

    // This should query the table in the attached database (schema supplied)
    QSqlTableModel model(0, db);
    model.setTable("adb.atest");
    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 0), Qt::DisplayRole).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1), Qt::DisplayRole).toString(), QLatin1String("attached-atest"));

    // This should query the table in the attached database (unique tablename)
    model.setTable("atest2");
    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 0), Qt::DisplayRole).toInt(), 2);
    QCOMPARE(model.data(model.index(0, 1), Qt::DisplayRole).toString(), QLatin1String("attached-atest2"));

    // This should query the table in the main database (tables in main db has 1st priority)
    model.setTable("atest");
    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0, 0), Qt::DisplayRole).toInt(), 3);
    QCOMPARE(model.data(model.index(0, 1), Qt::DisplayRole).toString(), QLatin1String("main"));
    attachedDb.close();
}


void tst_QSqlTableModel::tableModifyWithBlank()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setTable(qTableName("test4", __FILE__, db));
    model.select();

    //generate a time stamp for the test. Add one second to the current time to make sure
    //it is different than the QSqlQuery test.
    QString timeString=QDateTime::currentDateTime().addSecs(1).toString(Qt::ISODate);

    //insert a new row, with column0 being the timestamp.
    //Should be equivalent to QSqlQuery INSERT INTO... command)
    QVERIFY_SQL(model, insertRow(0));
    QVERIFY_SQL(model, setData(model.index(0,0),timeString));
    QVERIFY_SQL(model, submitAll());

    //set a filter on the table so the only record we get is the one we just made
    //I could just do another setData command, but I want to make sure the TableModel
    //matches exactly what is stored in the database
    model.setFilter("column1='" + timeString + QLatin1Char('\'')); //filter to get just the newly entered row
    QVERIFY_SQL(model, select());

    //Make sure we only get one record, and that it is the one we just made
    QCOMPARE(model.rowCount(), 1); //verify only one entry
    QCOMPARE(model.record(0).value(0).toString(), timeString); //verify correct record

    //At this point we know that the initial value (timestamp) was succsefully stored in the database
    //Attempt to modify the data in the new record
    //equivalent to query.exec("update test set column3="... command in direct test
    //set the data in the first column to "col1ModelData"
    QVERIFY_SQL(model, setData(model.index(0,1), "col1ModelData"));

    //do a quick check to make sure that the setData command properly set the value in the model
    QCOMPARE(model.record(0).value(1).toString(), QLatin1String("col1ModelData"));

    //submit the changed data to the database
    //This is where I have been getting errors.
    QVERIFY_SQL(model, submitAll());

    //make sure the model has the most current data for our record
    QVERIFY_SQL(model, select());

    //verify that our new record was the only record returned
    QCOMPARE(model.rowCount(), 1);

    //And that the record returned is, in fact, our test record.
    QCOMPARE(model.record(0).value(0).toString(), timeString);

    //Make sure the value of the first column matches what we set it to previously.
    QCOMPARE(model.record(0).value(1).toString(), QLatin1String("col1ModelData"));
}

void tst_QSqlTableModel::removeColumnAndRow()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setTable(test);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.columnCount(), 3);

    QVERIFY(model.removeColumn(0));
    QVERIFY(model.removeRow(0));
    QVERIFY(model.submitAll());
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.columnCount(), 2);

    // check with another table because the model has been modified
    // but not the sql table
    QSqlTableModel model2(0, db);
    model2.setTable(test);
    QVERIFY_SQL(model2, select());
    QCOMPARE(model2.rowCount(), 2);
    QCOMPARE(model2.columnCount(), 3);
}

void tst_QSqlTableModel::insertBeforeDelete()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlQuery q(db);
    QVERIFY_SQL( q, exec("insert into " + test + " values(9, 'andrew', 9)"));
    QVERIFY_SQL( q, exec("insert into " + test + " values(10, 'justin', 10)"));

    QSqlTableModel model(0, db);
    model.setTable(test);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    QVERIFY_SQL(model, select());

    QSqlRecord rec = model.record();
    rec.setValue(0, 4);
    rec.setValue(1, QString("bill"));
    rec.setValue(2, 4);
    QVERIFY_SQL(model, insertRecord(4, rec));

    QVERIFY_SQL(model, removeRow(5));
    QVERIFY_SQL(model, submitAll());
    QCOMPARE(model.rowCount(), 5);
}

void tst_QSqlTableModel::invalidFilterAndHeaderData()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlTableModel model(0, db);
    model.setTable(test);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    QVERIFY_SQL(model, select());
    QVERIFY_SQL(model, setHeaderData(0, Qt::Horizontal, "id"));
    QVERIFY_SQL(model, setHeaderData(1, Qt::Horizontal, "name"));
    QVERIFY_SQL(model, setHeaderData(2, Qt::Horizontal, "title"));

    model.setFilter("some nonsense");

    QVariant v = model.headerData(0, Qt::Horizontal, Qt::SizeHintRole);
    QVERIFY(!v.isValid());
}

class SqlThread : public QThread
{
public:
    SqlThread() : QThread() {}
    void run()
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "non-default-connection");
        QSqlTableModel stm(nullptr, db);
        isDone = true;
    }
    bool isDone = false;
};

void tst_QSqlTableModel::modelInAnotherThread()
{
    oldHandler = qInstallMessageHandler(sqlTableModelMessageHandler);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    CHECK_DATABASE(db);
    SqlThread t;
    t.start();
    QTRY_VERIFY(t.isDone);
    QVERIFY(t.isFinished());
}

QTEST_MAIN(tst_QSqlTableModel)
#include "tst_qsqltablemodel.moc"
