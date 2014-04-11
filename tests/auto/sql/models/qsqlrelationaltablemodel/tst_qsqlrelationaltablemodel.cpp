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
#include <QtSql/QtSql>

#include "../../kernel/qsqldatabase/tst_databases.h"

const QString reltest1(qTableName("reltest1", __FILE__, QSqlDatabase())),
        reltest2(qTableName("reltest2", __FILE__, QSqlDatabase())),
        reltest3(qTableName("reltest3", __FILE__, QSqlDatabase())),
        reltest4(qTableName("reltest4", __FILE__, QSqlDatabase())),
        reltest5(qTableName("reltest5", __FILE__, QSqlDatabase()));

class tst_QSqlRelationalTableModel : public QObject
{
    Q_OBJECT

public:
    void recreateTestTables(QSqlDatabase);

    tst_Databases dbs;

public slots:
    void initTestCase_data();
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void data();
    void setData();
    void multipleRelation();
    void insertRecord();
    void setRecord();
    void insertWithStrategies();
    void removeColumn();
    void filter();
    void sort();
    void revert();

    void clearDisplayValuesCache();
    void insertRecordDuplicateFieldNames();
    void invalidData();
    void relationModel();
    void casing();
    void escapedRelations();
    void escapedTableName();
    void whiteSpaceInIdentifiers();
    void psqlSchemaTest();
    void selectAfterUpdate();
    void relationOnFirstColumn();

private:
    void dropTestTables( QSqlDatabase db );
};


void tst_QSqlRelationalTableModel::initTestCase_data()
{
    QVERIFY(dbs.open());
    if (dbs.fillTestTable() == 0)
        QSKIP("No database drivers are available in this Qt configuration");
}

void tst_QSqlRelationalTableModel::recreateTestTables(QSqlDatabase db)
{
    dropTestTables(db);

    QSqlQuery q(db);
    QVERIFY_SQL( q, exec("create table " + reltest1 +
            " (id int not null primary key, name varchar(20), title_key int, another_title_key int)"));
    QVERIFY_SQL( q, exec("insert into " + reltest1 + " values(1, 'harry', 1, 2)"));
    QVERIFY_SQL( q, exec("insert into " + reltest1 + " values(2, 'trond', 2, 1)"));
    QVERIFY_SQL( q, exec("insert into " + reltest1 + " values(3, 'vohi', 1, 2)"));
    QVERIFY_SQL( q, exec("insert into " + reltest1 + " values(4, 'boris', 2, 2)"));
    QVERIFY_SQL( q, exec("insert into " + reltest1 + " values(5, 'nat', NULL, NULL)"));
    QVERIFY_SQL( q, exec("insert into " + reltest1 + " values(6, 'ale', NULL, 2)"));

    QVERIFY_SQL( q, exec("create table " + reltest2 + " (tid int not null primary key, title varchar(20))"));
    QVERIFY_SQL( q, exec("insert into " + reltest2 + " values(1, 'herr')"));
    QVERIFY_SQL( q, exec("insert into " + reltest2 + " values(2, 'mister')"));

    QVERIFY_SQL( q, exec("create table " + reltest3 + " (id int not null primary key, name varchar(20), city_key int)"));
    QVERIFY_SQL( q, exec("insert into " + reltest3 + " values(1, 'Gustav', 1)"));
    QVERIFY_SQL( q, exec("insert into " + reltest3 + " values(2, 'Heidi', 2)"));

    QVERIFY_SQL( q, exec("create table " + reltest4 + " (id int not null primary key, name varchar(20))"));
    QVERIFY_SQL( q, exec("insert into " + reltest4 + " values(1, 'Oslo')"));
    QVERIFY_SQL( q, exec("insert into " + reltest4 + " values(2, 'Trondheim')"));

    QVERIFY_SQL( q, exec("create table " + reltest5 + " (title varchar(20) not null primary key, abbrev varchar(20))"));
    QVERIFY_SQL( q, exec("insert into " + reltest5 + " values('herr', 'Hr')"));
    QVERIFY_SQL( q, exec("insert into " + reltest5 + " values('mister', 'Mr')"));

    if (testWhiteSpaceNames(db.driverName())) {
        QString reltest6 = db.driver()->escapeIdentifier(qTableName("rel", __FILE__, db) + " test6", QSqlDriver::TableName);
        QVERIFY_SQL( q, exec("create table " + reltest6 + " (id int not null primary key, " + db.driver()->escapeIdentifier("city key", QSqlDriver::FieldName) +
                    " int, " + db.driver()->escapeIdentifier("extra field", QSqlDriver::FieldName) + " int)"));
        QVERIFY_SQL( q, exec("insert into " + reltest6 + " values(1, 1,9)"));
        QVERIFY_SQL( q, exec("insert into " + reltest6 + " values(2, 2,8)"));

        QString reltest7 = db.driver()->escapeIdentifier(qTableName("rel", __FILE__, db) + " test7", QSqlDriver::TableName);
        QVERIFY_SQL( q, exec("create table " + reltest7 + " (" + db.driver()->escapeIdentifier("city id", QSqlDriver::TableName) + " int not null primary key, " + db.driver()->escapeIdentifier("city name", QSqlDriver::FieldName) + " varchar(20))"));
        QVERIFY_SQL( q, exec("insert into " + reltest7 + " values(1, 'New York')"));
        QVERIFY_SQL( q, exec("insert into " + reltest7 + " values(2, 'Washington')"));
    }
}

void tst_QSqlRelationalTableModel::initTestCase()
{
    foreach (const QString &dbname, dbs.dbNames) {
        QSqlDatabase db=QSqlDatabase::database(dbname);
        QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);
        if (dbType == QSqlDriverPrivate::Interbase) {
            db.exec("SET DIALECT 3");
        } else if (dbType == QSqlDriverPrivate::MSSqlServer) {
            db.exec("SET ANSI_DEFAULTS ON");
            db.exec("SET IMPLICIT_TRANSACTIONS OFF");
        } else if (dbType == QSqlDriverPrivate::PostgreSQL) {
            db.exec("set client_min_messages='warning'");
        }
        recreateTestTables(db);
    }
}

void tst_QSqlRelationalTableModel::cleanupTestCase()
{
    foreach (const QString &dbName, dbs.dbNames) {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        CHECK_DATABASE( db );
        dropTestTables( QSqlDatabase::database(dbName) );
    }
    dbs.close();
}

void tst_QSqlRelationalTableModel::dropTestTables( QSqlDatabase db )
{
    QStringList tableNames;
    tableNames << reltest1
            << reltest2
            << reltest3
            << reltest4
            << reltest5
            << (qTableName("rel", __FILE__, db) + " test6")
            << (qTableName( "rel", __FILE__, db) + " test7")
            << qTableName("CASETEST1", db)
            << qTableName("casetest1", db);
    tst_Databases::safeDropTables( db, tableNames );

    db.exec("DROP SCHEMA " + qTableName("QTBUG_5373", __FILE__, db) + " CASCADE");
    db.exec("DROP SCHEMA " + qTableName("QTBUG_5373_s2", __FILE__, db) + " CASCADE");
}

void tst_QSqlRelationalTableModel::init()
{
}

void tst_QSqlRelationalTableModel::cleanup()
{
}

void tst_QSqlRelationalTableModel::data()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlRelationalTableModel model(0, db);

    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
    QVERIFY_SQL(model, select());

    QCOMPARE(model.columnCount(), 4);
    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));

    //try a non-existent index
    QVERIFY2(model.data(model.index(0,4)).isValid() == false,"Invalid index returned valid QVariant");

    // check row with null relation: they are reported only in LeftJoin mode
    QCOMPARE(model.rowCount(), 4);

    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(4, 0)).toInt(), 5);
    QCOMPARE(model.data(model.index(4, 1)).toString(), QString("nat"));
    QVERIFY2(model.data(model.index(4, 2)).isValid() == true, "NULL relation reported with invalid QVariant");

    //check data retrieval when relational key is a non-integer type
    //in this case a string
    QSqlRelationalTableModel model2(0,db);
    model2.setTable(reltest2);
    model2.setRelation(1, QSqlRelation(reltest5,"title","abbrev"));
    QVERIFY_SQL(model2, select());

    QCOMPARE(model2.data(model2.index(0, 1)).toString(), QString("Hr"));
    QCOMPARE(model2.data(model2.index(1, 1)).toString(), QString("Mr"));
}

void tst_QSqlRelationalTableModel::setData()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);

    // set the values using OnRowChange Strategy
    {
        QSqlRelationalTableModel model(0, db);

        model.setTable(reltest1);
        model.setSort(0, Qt::AscendingOrder);
        model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
        QVERIFY_SQL(model, select());

        QVERIFY(model.setData(model.index(0, 1), QString("harry2")));
        QVERIFY(model.setData(model.index(0, 2), 2));

        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry2"));
        QCOMPARE(model.data(model.index(0, 2)).toString(), QString("mister"));

        model.submit();

        QVERIFY(model.setData(model.index(3,1), QString("boris2")));
        QVERIFY(model.setData(model.index(3, 2), 1));

        QCOMPARE(model.data(model.index(3,1)).toString(), QString("boris2"));
        QCOMPARE(model.data(model.index(3, 2)).toString(), QString("herr"));

        model.submit();
    }
    { //verify values
        QSqlRelationalTableModel model(0, db);
        model.setTable(reltest1);
        model.setSort(0, Qt::AscendingOrder);
        QVERIFY_SQL(model, select());

        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry2"));
        QCOMPARE(model.data(model.index(0, 2)).toInt(), 2);
        QCOMPARE(model.data(model.index(3, 1)).toString(), QString("boris2"));
        QCOMPARE(model.data(model.index(3, 2)).toInt(), 1);

        model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
        QVERIFY_SQL(model, select());
        QCOMPARE(model.data(model.index(0, 2)).toString(), QString("mister"));
        QCOMPARE(model.data(model.index(3,2)).toString(), QString("herr"));

    }

    //set the values using OnFieldChange strategy
    {
        QSqlRelationalTableModel model(0, db);
        model.setTable(reltest1);
        model.setEditStrategy(QSqlTableModel::OnFieldChange);
        model.setSort(0, Qt::AscendingOrder);
        model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
        QVERIFY_SQL(model, select());

        QVERIFY(model.setData(model.index(1,1), QString("trond2")));
        QVERIFY(model.setData(model.index(2,2), 2));

        QCOMPARE(model.data(model.index(1,1)).toString(), QString("trond2"));
        QCOMPARE(model.data(model.index(2,2)).toString(), QString("mister"));
    }
    { //verify values
        QSqlRelationalTableModel model(0, db);
        model.setTable(reltest1);
        model.setSort(0, Qt::AscendingOrder);
        QVERIFY_SQL(model, select());

        QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond2"));
        QCOMPARE(model.data(model.index(2, 2)).toInt(), 2);

        model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
        QVERIFY_SQL(model, select());
        QCOMPARE(model.data(model.index(2, 2)).toString(), QString("mister"));
    }

    //set values using OnManualSubmit strategy
    {
        QSqlRelationalTableModel model(0, db);

        model.setTable(reltest1);
        model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));

        //sybase doesn't allow tables with the same alias used twice as col names
        //so don't set up an identical relation when using the tds driver
        if (dbType != QSqlDriverPrivate::Sybase)
            model.setRelation(3, QSqlRelation(reltest2, "tid", "title"));

        model.setEditStrategy(QSqlTableModel::OnManualSubmit);
        model.setSort(0, Qt::AscendingOrder);
        QVERIFY_SQL(model, select());

        QVERIFY(model.setData(model.index(2, 1), QString("vohi2")));
        QVERIFY(model.setData(model.index(3, 2), 1));
        QVERIFY(model.setData(model.index(0, 3), 1));

        QCOMPARE(model.data(model.index(2, 1)).toString(), QString("vohi2"));
        QCOMPARE(model.data(model.index(3, 2)).toString(), QString("herr"));
        if (dbType != QSqlDriverPrivate::Sybase)
            QCOMPARE(model.data(model.index(0, 3)).toString(), QString("herr"));
        else
            QCOMPARE(model.data(model.index(0, 3)).toInt(), 1);

        QVERIFY_SQL(model, submitAll());
    }
    { //verify values
        QSqlRelationalTableModel model(0, db);
        model.setTable(reltest1);
        model.setSort(0, Qt::AscendingOrder);
        QVERIFY_SQL(model, select());

        QCOMPARE(model.data(model.index(2, 1)).toString(), QString("vohi2"));
        QCOMPARE(model.data(model.index(3, 2)).toInt(), 1);
        QCOMPARE(model.data(model.index(0, 3)).toInt(), 1);

        model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
        if (dbType != QSqlDriverPrivate::Sybase)
            model.setRelation(3, QSqlRelation(reltest2, "tid", "title"));
        QVERIFY_SQL(model, select());
        QCOMPARE(model.data(model.index(3, 2)).toString(), QString("herr"));

        if (dbType != QSqlDriverPrivate::Sybase)
            QCOMPARE(model.data(model.index(0, 3)).toString(), QString("herr"));
        else
            QCOMPARE(model.data(model.index(0, 3)).toInt(), 1);
    }

    //check setting of data when the relational key is a non-integer type
    //in this case a string.
    {
        QSqlRelationalTableModel model(0, db);

        model.setTable(reltest2);
        model.setRelation(1, QSqlRelation(reltest5, "title", "abbrev"));
        model.setEditStrategy(QSqlTableModel::OnManualSubmit);
        QVERIFY_SQL(model, select());

        QCOMPARE(model.data(model.index(0,1)).toString(), QString("Hr"));
        QVERIFY(model.setData(model.index(0,1), QString("mister")));
        QCOMPARE(model.data(model.index(0,1)).toString(), QString("Mr"));
        QVERIFY_SQL(model, submitAll());

        QCOMPARE(model.data(model.index(0,1)).toString(), QString("Mr"));
    }

    // Redo same tests, with a LeftJoin
    {
        QSqlRelationalTableModel model(0, db);

        model.setTable(reltest2);
        model.setRelation(1, QSqlRelation(reltest5, "title", "abbrev"));
        model.setEditStrategy(QSqlTableModel::OnManualSubmit);
        model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
        QVERIFY_SQL(model, select());

        QCOMPARE(model.data(model.index(0,1)).toString(), QString("Mr"));
        QVERIFY(model.setData(model.index(0,1), QString("herr")));
        QCOMPARE(model.data(model.index(0,1)).toString(), QString("Hr"));
        QVERIFY_SQL(model, submitAll());

        QCOMPARE(model.data(model.index(0,1)).toString(), QString("Hr"));
    }

}

void tst_QSqlRelationalTableModel::multipleRelation()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    recreateTestTables(db);

    QSqlRelationalTableModel model(0, db);

    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
    model.setRelation(3, QSqlRelation(reltest4, "id", "name"));
    model.setSort(0, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(2, 0)).toInt(), 3);

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));
    QCOMPARE(model.data(model.index(0, 3)).toString(), QString("Trondheim"));

    // Redo same test in the LeftJoin mode
    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
    model.setRelation(3, QSqlRelation(reltest4, "id", "name"));
    model.setSort(0, Qt::AscendingOrder);
    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(2, 0)).toInt(), 3);

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));
    QCOMPARE(model.data(model.index(0, 3)).toString(), QString("Trondheim"));
}

void tst_QSqlRelationalTableModel::insertRecord()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlRelationalTableModel model(0, db);

    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
    model.setSort(0, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

    QSqlRecord rec;
    QSqlField f1("id", QVariant::Int);
    QSqlField f2("name", QVariant::String);
    QSqlField f3("title_key", QVariant::Int);
    QSqlField f4("another_title_key", QVariant::Int);

    f1.setValue(7);
    f2.setValue("test");
    f3.setValue(1);
    f4.setValue(2);

    f1.setGenerated(true);
    f2.setGenerated(true);
    f3.setGenerated(true);
    f4.setGenerated(true);

    rec.append(f1);
    rec.append(f2);
    rec.append(f3);
    rec.append(f4);

    QVERIFY_SQL(model, insertRecord(-1, rec));

    QCOMPARE(model.data(model.index(4, 0)).toInt(), 7);
    QCOMPARE(model.data(model.index(4, 1)).toString(), QString("test"));
    QCOMPARE(model.data(model.index(4, 2)).toString(), QString("herr"));

    // In LeftJoin mode, two additional rows are fetched
    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(6, 0)).toInt(), 7);
    QCOMPARE(model.data(model.index(6, 1)).toString(), QString("test"));
    QCOMPARE(model.data(model.index(6, 2)).toString(), QString("herr"));
}

void tst_QSqlRelationalTableModel::setRecord()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    recreateTestTables(db);

    QSqlRelationalTableModel model(0, db);

    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
    model.setSort(0, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

    QSqlRecord rec;
    QSqlField f1("id", QVariant::Int);
    QSqlField f2("name", QVariant::String);
    QSqlField f3("title_key", QVariant::Int);
    QSqlField f4("another_title_key", QVariant::Int);

    f1.setValue(7);
    f2.setValue("tester");
    f3.setValue(1);
    f4.setValue(2);

    f1.setGenerated(true);
    f2.setGenerated(true);
    f3.setGenerated(true);
    f4.setGenerated(true);

    rec.append(f1);
    rec.append(f2);
    rec.append(f3);
    rec.append(f4);

    QCOMPARE(model.data(model.index(1, 0)).toInt(), 2);
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("trond"));
    QCOMPARE(model.data(model.index(1, 2)).toString(), QString("mister"));

    QVERIFY_SQL(model, setRecord(1, rec));

    QCOMPARE(model.data(model.index(1, 0)).toInt(), 7);
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("tester"));
    QCOMPARE(model.data(model.index(1, 2)).toString(), QString("herr"));

    model.setSort(0, Qt::AscendingOrder);
    QVERIFY_SQL(model, submit());

    if (model.editStrategy() != QSqlTableModel::OnManualSubmit) {
        QCOMPARE(model.data(model.index(1, 0)).toInt(), 7);
        QCOMPARE(model.data(model.index(1, 1)).toString(), QString("tester"));
        QCOMPARE(model.data(model.index(1, 2)).toString(), QString("herr"));
        QVERIFY_SQL(model, select());
    }

    QCOMPARE(model.data(model.index(3, 0)).toInt(), 7);
    QCOMPARE(model.data(model.index(3, 1)).toString(), QString("tester"));
    QCOMPARE(model.data(model.index(3, 2)).toString(), QString("herr"));

}

void tst_QSqlRelationalTableModel::insertWithStrategies()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);

    QSqlRelationalTableModel model(0, db);

    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
    model.setSort(0, Qt::AscendingOrder);

    if (dbType != QSqlDriverPrivate::Sybase)
        model.setRelation(3, QSqlRelation(reltest2, "tid", "title"));
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(0,0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0,1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0,2)).toString(), QString("herr"));
    if (dbType != QSqlDriverPrivate::Sybase)
        QCOMPARE(model.data(model.index(0,3)).toString(), QString("mister"));
    else
        QCOMPARE(model.data(model.index(0,3)).toInt(), 2);

    model.insertRows(0, 1);
    model.setData(model.index(0, 0), 1011);
    model.setData(model.index(0, 1), "test");
    model.setData(model.index(0, 2), 2);
    model.setData(model.index(0, 3), 1);

    QCOMPARE(model.data(model.index(0,0)).toInt(), 1011);
    QCOMPARE(model.data(model.index(0,1)).toString(), QString("test"));
    QCOMPARE(model.data(model.index(0,2)).toString(), QString("mister"));
    if (dbType != QSqlDriverPrivate::Sybase)
        QCOMPARE(model.data(model.index(0,3)).toString(), QString("herr"));
    else
        QCOMPARE(model.data(model.index(0,3)).toInt(), 1);

    QCOMPARE(model.data(model.index(1,0)).toInt(), 1);
    QCOMPARE(model.data(model.index(1,1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(1,2)).toString(), QString("herr"));
    if (dbType != QSqlDriverPrivate::Sybase)
        QCOMPARE(model.data(model.index(1,3)).toString(), QString("mister"));
    else
        QCOMPARE(model.data(model.index(1,3)).toInt(), 2);



    QVERIFY_SQL(model, submitAll());

    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    // The changes were submitted, but there was no automatic select to resort
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(0,0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0,1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0,2)).toString(), QString("herr"));

    if (dbType != QSqlDriverPrivate::Sybase) {
        QCOMPARE(model.data(model.index(0,3)).toString(), QString("mister"));
        model.setData(model.index(0,3),1);
        QCOMPARE(model.data(model.index(0,3)).toString(), QString("herr"));
    } else {
        QCOMPARE(model.data(model.index(0,3)).toInt(), 2);
        model.setData(model.index(0,3),1);
        QCOMPARE(model.data(model.index(0,3)).toInt(), 1);
    }

    model.insertRows(0, 2);
    model.setData(model.index(0, 0), 1012);
    model.setData(model.index(0, 1), "george");
    model.setData(model.index(0, 2), 2);
    model.setData(model.index(0, 3), 2);

    model.setData(model.index(1, 0), 1013);
    model.setData(model.index(1, 1), "kramer");
    model.setData(model.index(1, 2), 2);
    model.setData(model.index(1, 3), 1);

    QCOMPARE(model.data(model.index(0,0)).toInt(),1012);
    QCOMPARE(model.data(model.index(0,1)).toString(), QString("george"));
    QCOMPARE(model.data(model.index(0,2)).toString(), QString("mister"));
    if (dbType != QSqlDriverPrivate::Sybase)
        QCOMPARE(model.data(model.index(0,3)).toString(), QString("mister"));
    else
        QCOMPARE(model.data(model.index(0,3)).toInt(), 2);


    QCOMPARE(model.data(model.index(1,0)).toInt(),1013);
    QCOMPARE(model.data(model.index(1,1)).toString(), QString("kramer"));
    QCOMPARE(model.data(model.index(1,2)).toString(), QString("mister"));
    if (dbType != QSqlDriverPrivate::Sybase)
        QCOMPARE(model.data(model.index(1,3)).toString(), QString("herr"));
    else
        QCOMPARE(model.data(model.index(1,3)).toInt(), 1);

    QCOMPARE(model.data(model.index(2,0)).toInt(), 1);
    QCOMPARE(model.data(model.index(2,1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(2,2)).toString(), QString("herr"));
    if (dbType != QSqlDriverPrivate::Sybase)
        QCOMPARE(model.data(model.index(2,3)).toString(), QString("herr"));
    else
        QCOMPARE(model.data(model.index(2,3)).toInt(), 1);

    QVERIFY_SQL(model, submitAll());
}

void tst_QSqlRelationalTableModel::removeColumn()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    recreateTestTables(db);

    QSqlRelationalTableModel model(0, db);

    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
    QVERIFY_SQL(model, select());

    QVERIFY_SQL(model, removeColumn(3));
    QVERIFY_SQL(model, select());

    QCOMPARE(model.columnCount(), 3);

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));
    QCOMPARE(model.data(model.index(0, 3)), QVariant());

    // try removing more than one column
    QVERIFY_SQL(model, removeColumns(1, 2));
    QCOMPARE(model.columnCount(), 1);
    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)), QVariant());

    // try in LeftJoin mode the same tests
    CHECK_DATABASE(db);
    recreateTestTables(db);

    QSqlRelationalTableModel lmodel(0, db);

    lmodel.setTable(reltest1);
    lmodel.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
    lmodel.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(lmodel, select());

    QVERIFY_SQL(lmodel, removeColumn(3));
    QVERIFY_SQL(lmodel, select());

    QCOMPARE(lmodel.columnCount(), 3);

    QCOMPARE(lmodel.data(lmodel.index(0, 0)).toInt(), 1);
    QCOMPARE(lmodel.data(lmodel.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(lmodel.data(lmodel.index(0, 2)).toString(), QString("herr"));
    QCOMPARE(lmodel.data(lmodel.index(0, 3)), QVariant());

    // try removing more than one column
    QVERIFY_SQL(lmodel, removeColumns(1, 2));
    QCOMPARE(lmodel.columnCount(), 1);
    QCOMPARE(lmodel.data(lmodel.index(0, 0)).toInt(), 1);
    QCOMPARE(lmodel.data(lmodel.index(0, 1)), QVariant());
}

void tst_QSqlRelationalTableModel::filter()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    recreateTestTables(db);

    QSqlRelationalTableModel model(0, db);

    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
    model.setFilter("title = 'herr'");

    QVERIFY_SQL(model, select());
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));
    QCOMPARE(model.data(model.index(1, 2)).toString(), QString("herr"));

    // Redo same filter test in LeftJoin mode
    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(model,select());

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));
    QCOMPARE(model.data(model.index(1, 2)).toString(), QString("herr"));
}

void tst_QSqlRelationalTableModel::sort()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);

    QSqlRelationalTableModel model(0, db);

    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
    if (dbType != QSqlDriverPrivate::Sybase)
        model.setRelation(3, QSqlRelation(reltest2, "tid", "title"));

    model.setSort(2, Qt::DescendingOrder);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.rowCount(), 4);
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("mister"));
    QCOMPARE(model.data(model.index(1, 2)).toString(), QString("mister"));
    QCOMPARE(model.data(model.index(2, 2)).toString(), QString("herr"));
    QCOMPARE(model.data(model.index(3, 2)).toString(), QString("herr"));


    model.setSort(3, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

     if (dbType != QSqlDriverPrivate::Sybase) {
        QCOMPARE(model.rowCount(), 4);
        QCOMPARE(model.data(model.index(0, 3)).toString(), QString("herr"));
        QCOMPARE(model.data(model.index(1, 3)).toString(), QString("mister"));
        QCOMPARE(model.data(model.index(2, 3)).toString(), QString("mister"));
        QCOMPARE(model.data(model.index(3, 3)).toString(), QString("mister"));
    } else {
        QCOMPARE(model.data(model.index(0, 3)).toInt(), 1);
        QCOMPARE(model.data(model.index(1, 3)).toInt(), 2);
        QCOMPARE(model.data(model.index(2, 3)).toInt(), 2);
        QCOMPARE(model.data(model.index(3, 3)).toInt(), 2);
    }

    // redo same test in LeftJoin mode
    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    model.setSort(2, Qt::DescendingOrder);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.rowCount(), 6);
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("mister"));
    QCOMPARE(model.data(model.index(1, 2)).toString(), QString("mister"));
    QCOMPARE(model.data(model.index(2, 2)).toString(), QString("herr"));
    QCOMPARE(model.data(model.index(3, 2)).toString(), QString("herr"));
    QCOMPARE(model.data(model.index(4, 2)).toString(), QString(""));
    QCOMPARE(model.data(model.index(5, 2)).toString(), QString(""));

    model.setSort(3, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

    if (dbType != QSqlDriverPrivate::Sybase) {
        QCOMPARE(model.rowCount(), 6);
        QCOMPARE(model.data(model.index(0, 3)).toString(), QString(""));
        QCOMPARE(model.data(model.index(1, 3)).toString(), QString("herr"));
        QCOMPARE(model.data(model.index(2, 3)).toString(), QString("mister"));
        QCOMPARE(model.data(model.index(3, 3)).toString(), QString("mister"));
        QCOMPARE(model.data(model.index(4, 3)).toString(), QString("mister"));
        QCOMPARE(model.data(model.index(5, 3)).toString(), QString("mister"));
    } else {
        QCOMPARE(model.data(model.index(0, 3)).toInt(), 1);
        QCOMPARE(model.data(model.index(1, 3)).toInt(), 2);
        QCOMPARE(model.data(model.index(2, 3)).toInt(), 2);
        QCOMPARE(model.data(model.index(3, 3)).toInt(), 2);
    }
}

static void testRevert(QSqlRelationalTableModel &model)
{
    /* revert single row */
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));
    QVERIFY(model.setData(model.index(0, 2), 2, Qt::EditRole));

    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("mister"));
    model.revertRow(0);
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));

    /* revert all */
    QVERIFY(model.setData(model.index(0, 2), 2, Qt::EditRole));

    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("mister"));
    model.revertAll();
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));

    // the following only works for OnManualSubmit
    if (model.editStrategy() != QSqlTableModel::OnManualSubmit)
        return;

    /* revert inserted rows */
    int initialRowCount = model.rowCount();
    QVERIFY(model.insertRows(4, 4));
    QVERIFY(model.rowCount() == (initialRowCount + 4));

    /* make sure the new rows are initialized to nothing */
    QVERIFY(model.data(model.index(4, 2)).toString().isEmpty());
    QVERIFY(model.data(model.index(5, 2)).toString().isEmpty());
    QVERIFY(model.data(model.index(6, 2)).toString().isEmpty());
    QVERIFY(model.data(model.index(7, 2)).toString().isEmpty());

    /* Set some values */
    QVERIFY(model.setData(model.index(4, 0), 42, Qt::EditRole));
    QVERIFY(model.setData(model.index(5, 0), 43, Qt::EditRole));
    QVERIFY(model.setData(model.index(6, 0), 44, Qt::EditRole));
    QVERIFY(model.setData(model.index(7, 0), 45, Qt::EditRole));

    QVERIFY(model.setData(model.index(4, 2), 2, Qt::EditRole));
    QVERIFY(model.setData(model.index(5, 2), 2, Qt::EditRole));
    QVERIFY(model.setData(model.index(6, 2), 1, Qt::EditRole));
    QVERIFY(model.setData(model.index(7, 2), 2, Qt::EditRole));

    /* Now revert the newly inserted rows */
    model.revertAll();
    QVERIFY(model.rowCount() == initialRowCount);

    /* Insert rows again */
    QVERIFY(model.insertRows(4, 4));

    /* make sure the new rows are initialized to nothing */
    QVERIFY(model.data(model.index(4, 2)).toString().isEmpty());
    QVERIFY(model.data(model.index(5, 2)).toString().isEmpty());
    QVERIFY(model.data(model.index(6, 2)).toString().isEmpty());
    QVERIFY(model.data(model.index(7, 2)).toString().isEmpty());
}

void tst_QSqlRelationalTableModel::revert()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlRelationalTableModel model(0, db);

    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
    model.setRelation(3, QSqlRelation(reltest4, "id", "name"));

    model.setSort(0, Qt::AscendingOrder);

    QVERIFY_SQL(model, select());
    QCOMPARE(model.data(model.index(0, 0)).toString(), QString("1"));

    testRevert(model);
    if (QTest::currentTestFailed())
        return;

    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(model, select());
    testRevert(model);

    /* and again with OnManualSubmit */
    model.setJoinMode(QSqlRelationalTableModel::InnerJoin);
    QVERIFY_SQL(model, select());
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    testRevert(model);

    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(model, select());
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    testRevert(model);
}

void tst_QSqlRelationalTableModel::clearDisplayValuesCache()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);

    QSqlRelationalTableModel model(0, db);

    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));

    if (dbType != QSqlDriverPrivate::Sybase)
        model.setRelation(3, QSqlRelation(reltest2, "tid", "title"));
    model.setSort(1, Qt::AscendingOrder);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);

    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(3, 0)).toInt(), 3);
    QCOMPARE(model.data(model.index(3, 1)).toString(), QString("vohi"));
    QCOMPARE(model.data(model.index(3, 2)).toString(), QString("herr"));
    if (dbType != QSqlDriverPrivate::Sybase)
        QCOMPARE(model.data(model.index(3, 3)).toString(), QString("mister"));
    else
        QCOMPARE(model.data(model.index(3, 3)).toInt(), 2 );

    model.insertRow(model.rowCount());
    QVERIFY(model.setData(model.index(4, 0), 7, Qt::EditRole));
    QVERIFY(model.setData(model.index(4, 1), "anders", Qt::EditRole));
    QVERIFY(model.setData(model.index(4, 2), 1, Qt::EditRole));
    QVERIFY(model.setData(model.index(4, 3), 1, Qt::EditRole));
    model.submitAll();

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 7);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("anders"));
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));
    if (dbType != QSqlDriverPrivate::Sybase)
        QCOMPARE(model.data(model.index(0, 3)).toString(), QString("herr"));
    else
        QCOMPARE(model.data(model.index(0, 3)).toInt(), 1);

    QCOMPARE(model.data(model.index(4, 0)).toInt(), 3);
    QCOMPARE(model.data(model.index(4, 1)).toString(), QString("vohi"));
    QCOMPARE(model.data(model.index(4, 2)).toString(), QString("herr"));
    if (dbType != QSqlDriverPrivate::Sybase)
        QCOMPARE(model.data(model.index(4, 3)).toString(), QString("mister"));
    else
        QCOMPARE(model.data(model.index(4, 3)).toInt(), 2);
}

// For task 140782 and 176374: If the main table and the related tables uses the same
// name for a column or display column then insertRecord() would return true though it
// actually failed.
void tst_QSqlRelationalTableModel::insertRecordDuplicateFieldNames()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);

    QSqlRelationalTableModel model(0, db);
    model.setTable(reltest3);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    model.setSort(0, Qt::AscendingOrder);

    // Duplication of "name", used in both reltest3 and reltest4.
    model.setRelation(2, QSqlRelation(reltest4, "id", "name"));
    QVERIFY_SQL(model, select());

    if (dbType == QSqlDriverPrivate::Interbase || dbType == QSqlDriverPrivate::Oracle || dbType == QSqlDriverPrivate::DB2) {
        QCOMPARE(model.record(1).value((reltest4+QLatin1String("_name_2")).toUpper()).toString(),
            QString("Trondheim"));
    } else {
        QCOMPARE(model.record(1).value((reltest4+QLatin1String("_name_2"))).toString(),
            QString("Trondheim"));
    }

    QSqlRecord rec = model.record();
    rec.setValue(0, 3);
    rec.setValue(1, "Berge");
    rec.setValue(2, 1); // Must insert the key value

    if (dbType == QSqlDriverPrivate::Interbase || dbType == QSqlDriverPrivate::Oracle || dbType == QSqlDriverPrivate::DB2) {
        QCOMPARE(rec.fieldName(0), QLatin1String("ID"));
        QCOMPARE(rec.fieldName(1), QLatin1String("NAME")); // This comes from main table
    } else {
        QCOMPARE(rec.fieldName(0), QLatin1String("id"));
        QCOMPARE(rec.fieldName(1), QLatin1String("name"));
    }

    // The duplicate field names is aliased because it's comes from the relation's display column.
    if (dbType == QSqlDriverPrivate::Interbase || dbType == QSqlDriverPrivate::Oracle || dbType == QSqlDriverPrivate::DB2)
        QCOMPARE(rec.fieldName(2), (reltest4+QLatin1String("_name_2")).toUpper());
    else
        QCOMPARE(rec.fieldName(2), reltest4+QLatin1String("_name_2"));

    QVERIFY(model.insertRecord(-1, rec));
    QCOMPARE(model.data(model.index(2, 2)).toString(), QString("Oslo"));
    QVERIFY(model.submitAll());
    QCOMPARE(model.data(model.index(2, 2)).toString(), QString("Oslo"));
}

void tst_QSqlRelationalTableModel::invalidData()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlRelationalTableModel model(0, db);
    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
    QVERIFY_SQL(model, select());

    //try set a non-existent relational key
    QVERIFY(model.setData(model.index(0, 2), 3) == false);
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));

    //try to set data in non valid index
    QVERIFY(model.setData(model.index(0,10),5) == false);

    //same test with LeftJoin mode
    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(model, select());

    //try set a non-existent relational key
    QVERIFY(model.setData(model.index(0, 2), 3) == false);
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));

    //try to set data in non valid index
    QVERIFY(model.setData(model.index(0,10),5) == false);
}

void tst_QSqlRelationalTableModel::relationModel()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlRelationalTableModel model(0, db);
    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
    QVERIFY_SQL(model, select());

    QVERIFY(model.relationModel(0) == NULL);
    QVERIFY(model.relationModel(1) == NULL);
    QVERIFY(model.relationModel(2) != NULL);
    QVERIFY(model.relationModel(3) == NULL);
    QVERIFY(model.relationModel(4) == NULL);

    model.setRelation(3, QSqlRelation(reltest4, "id", "name"));
    QVERIFY_SQL(model, select());

    QVERIFY(model.relationModel(0) == NULL);
    QVERIFY(model.relationModel(1) == NULL);
    QVERIFY(model.relationModel(2) != NULL);
    QVERIFY(model.relationModel(3) != NULL);
    QVERIFY(model.relationModel(4) == NULL);

    QSqlTableModel *rel_model = model.relationModel(2);
    QCOMPARE(rel_model->data(rel_model->index(0,1)).toString(), QString("herr"));

    //same test in JoinMode
    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(model, select());

    QVERIFY(model.relationModel(0) == NULL);
    QVERIFY(model.relationModel(1) == NULL);
    QVERIFY(model.relationModel(2) != NULL);
    QVERIFY(model.relationModel(3) != NULL);
    QVERIFY(model.relationModel(4) == NULL);

    QSqlTableModel *rel_model2 = model.relationModel(2);
    QCOMPARE(rel_model2->data(rel_model->index(0,1)).toString(), QString("herr"));
}

void tst_QSqlRelationalTableModel::casing()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);

    if (dbType == QSqlDriverPrivate::Interbase || dbType == QSqlDriverPrivate::SQLite || dbType == QSqlDriverPrivate::MSSqlServer)
        QSKIP("The casing test for this database is irrelevant since this database does not treat different cases as separate entities");

    QSqlQuery q(db);
    QVERIFY_SQL( q, exec("create table " + qTableName("CASETEST1", db).toUpper() +
                " (id int not null primary key, name varchar(20), title_key int, another_title_key int)"));

    if (!q.exec("create table " + qTableName("casetest1", db) +
                " (ident int not null primary key, name varchar(20), title_key int)"))
        QSKIP("The casing test for this database is irrelevant since this database does not treat different cases as separate entities");

    QVERIFY_SQL( q, exec("insert into " + qTableName("CASETEST1", db).toUpper() + " values(1, 'harry', 1, 2)"));
    QVERIFY_SQL( q, exec("insert into " + qTableName("CASETEST1", db).toUpper() + " values(2, 'trond', 2, 1)"));
    QVERIFY_SQL( q, exec("insert into " + qTableName("CASETEST1", db).toUpper() + " values(3, 'vohi', 1, 2)"));
    QVERIFY_SQL( q, exec("insert into " + qTableName("CASETEST1", db).toUpper() + " values(4, 'boris', 2, 2)"));
    QVERIFY_SQL( q, exec("insert into " + qTableName("casetest1", db) + " values(1, 'jerry', 1)"));
    QVERIFY_SQL( q, exec("insert into " + qTableName("casetest1", db) + " values(2, 'george', 2)"));
    QVERIFY_SQL( q, exec("insert into " + qTableName("casetest1", db) + " values(4, 'kramer', 2)"));

    if (dbType == QSqlDriverPrivate::Oracle) {
        //try an owner that doesn't exist
        QSqlRecord rec = db.driver()->record("doug." + qTableName("CASETEST1", db).toUpper());
        QCOMPARE( rec.count(), 0);

        //try an owner that does exist
        rec = db.driver()->record(db.userName() + "." + qTableName("CASETEST1", db).toUpper());
        QCOMPARE( rec.count(), 4);
    }
    QSqlRecord rec = db.driver()->record(qTableName("CASETEST1", db).toUpper());
    QCOMPARE( rec.count(), 4);

    rec = db.driver()->record(qTableName("casetest1", db));
    QCOMPARE( rec.count(), 3);

    QSqlTableModel upperCaseModel(0, db);
    upperCaseModel.setTable(qTableName("CASETEST1", db).toUpper());

    QCOMPARE(upperCaseModel.tableName(), qTableName("CASETEST1", db).toUpper());

    QVERIFY_SQL(upperCaseModel, select());

    QCOMPARE(upperCaseModel.rowCount(), 4);

    QSqlTableModel lowerCaseModel(0, db);
    lowerCaseModel.setTable(qTableName("casetest1", db));
    QCOMPARE(lowerCaseModel.tableName(), qTableName("casetest1", db));
    QVERIFY_SQL(lowerCaseModel, select());

    QCOMPARE(lowerCaseModel.rowCount(), 3);

    QSqlRelationalTableModel model(0, db);
    model.setTable(qTableName("CASETEST1", db).toUpper());
    model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));
}

void tst_QSqlRelationalTableModel::escapedRelations()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);

    recreateTestTables(db);

    QSqlRelationalTableModel model(0, db);
    model.setTable(reltest1);

    //try with relation table name quoted
    if (dbType == QSqlDriverPrivate::Interbase || dbType == QSqlDriverPrivate::Oracle || dbType == QSqlDriverPrivate::DB2) {
        model.setRelation(2, QSqlRelation(db.driver()->escapeIdentifier(reltest2.toUpper(),QSqlDriver::TableName),
                            "tid",
                            "title"));
    } else {
        model.setRelation(2, QSqlRelation(db.driver()->escapeIdentifier(reltest2,QSqlDriver::TableName),
                            "tid",
                            "title"));

    }
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));

    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));

    //try with index column quoted
    model.setJoinMode(QSqlRelationalTableModel::InnerJoin);
    if (dbType == QSqlDriverPrivate::Interbase || dbType == QSqlDriverPrivate::Oracle || dbType == QSqlDriverPrivate::DB2) {
        model.setRelation(2, QSqlRelation(reltest2,
                            db.driver()->escapeIdentifier("tid", QSqlDriver::FieldName).toUpper(),
                            "title"));
    } else {
        model.setRelation(2, QSqlRelation(reltest2,
                            db.driver()->escapeIdentifier("tid", QSqlDriver::FieldName),
                            "title"));
    }
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));

    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));

    //try with display column quoted
    model.setJoinMode(QSqlRelationalTableModel::InnerJoin);
    if (dbType == QSqlDriverPrivate::Interbase || dbType == QSqlDriverPrivate::Oracle || dbType == QSqlDriverPrivate::DB2) {

        model.setRelation(2, QSqlRelation(reltest2,
                            "tid",
                            db.driver()->escapeIdentifier("title", QSqlDriver::FieldName).toUpper()));
    } else {
        model.setRelation(2, QSqlRelation(reltest2,
                            "tid",
                            db.driver()->escapeIdentifier("title", QSqlDriver::FieldName)));
    }

    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));

    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));

    //try with tablename and index and display columns quoted in the relation
    model.setJoinMode(QSqlRelationalTableModel::InnerJoin);
    if (dbType == QSqlDriverPrivate::Interbase || dbType == QSqlDriverPrivate::Oracle || dbType == QSqlDriverPrivate::DB2) {
        model.setRelation(2, QSqlRelation(reltest2,
                            "tid",
                            db.driver()->escapeIdentifier("title", QSqlDriver::FieldName).toUpper()));
    } else {
        model.setRelation(2, QSqlRelation(reltest2,
                            "tid",
                            db.driver()->escapeIdentifier("title", QSqlDriver::FieldName)));
    }
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));

    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));
}

void tst_QSqlRelationalTableModel::escapedTableName()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    const QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);

    // set the values using OnRowChange Strategy with an escaped tablename
    {
        QSqlRelationalTableModel model(0, db);

        if (dbType == QSqlDriverPrivate::Interbase || dbType == QSqlDriverPrivate::Oracle || dbType == QSqlDriverPrivate::DB2) {
            model.setTable(db.driver()->escapeIdentifier(reltest1.toUpper(), QSqlDriver::TableName));
        } else {
            model.setTable(db.driver()->escapeIdentifier(reltest1, QSqlDriver::TableName));
        }
        model.setSort(0, Qt::AscendingOrder);
        model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
        QVERIFY_SQL(model, select());

        QVERIFY(model.setData(model.index(0, 1), QString("harry2")));
        QVERIFY(model.setData(model.index(0, 2), 2));

        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry2"));
        QCOMPARE(model.data(model.index(0, 2)).toString(), QString("mister"));

        model.submit();

        QVERIFY(model.setData(model.index(3,1), QString("boris2")));
        QVERIFY(model.setData(model.index(3, 2), 1));

        QCOMPARE(model.data(model.index(3,1)).toString(), QString("boris2"));
        QCOMPARE(model.data(model.index(3, 2)).toString(), QString("herr"));

        model.submit();
    }
    { //verify values
        QSqlRelationalTableModel model(0, db);
        model.setTable(reltest1);
        model.setSort(0, Qt::AscendingOrder);
        QVERIFY_SQL(model, select());

        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry2"));
        QCOMPARE(model.data(model.index(0, 2)).toInt(), 2);
        QCOMPARE(model.data(model.index(3, 1)).toString(), QString("boris2"));
        QCOMPARE(model.data(model.index(3, 2)).toInt(), 1);

        model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
        QVERIFY_SQL(model, select());
        QCOMPARE(model.data(model.index(0, 2)).toString(), QString("mister"));
        QCOMPARE(model.data(model.index(3,2)).toString(), QString("herr"));

    }

    //ok, now do same test with LeftJoin
    {
        QSqlRelationalTableModel model(0, db);

        if (dbType == QSqlDriverPrivate::Interbase || dbType == QSqlDriverPrivate::Oracle || dbType == QSqlDriverPrivate::DB2) {
            model.setTable(db.driver()->escapeIdentifier(reltest1.toUpper(), QSqlDriver::TableName));
        } else {
            model.setTable(db.driver()->escapeIdentifier(reltest1, QSqlDriver::TableName));
        }
        model.setSort(0, Qt::AscendingOrder);
        model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
        model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
        QVERIFY_SQL(model, select());

        QVERIFY(model.setData(model.index(0, 1), QString("harry2")));
        QVERIFY(model.setData(model.index(0, 2), 2));

        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry2"));
        QCOMPARE(model.data(model.index(0, 2)).toString(), QString("mister"));

        model.submit();

        QVERIFY(model.setData(model.index(3,1), QString("boris2")));
        QVERIFY(model.setData(model.index(3, 2), 1));

        QCOMPARE(model.data(model.index(3,1)).toString(), QString("boris2"));
        QCOMPARE(model.data(model.index(3, 2)).toString(), QString("herr"));

        model.submit();
    }
    { //verify values
        QSqlRelationalTableModel model(0, db);
        model.setTable(reltest1);
        model.setSort(0, Qt::AscendingOrder);
        model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
        QVERIFY_SQL(model, select());

        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry2"));
        QCOMPARE(model.data(model.index(0, 2)).toInt(), 2);
        QCOMPARE(model.data(model.index(3, 1)).toString(), QString("boris2"));
        QCOMPARE(model.data(model.index(3, 2)).toInt(), 1);

        model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
        QVERIFY_SQL(model, select());
        QCOMPARE(model.data(model.index(0, 2)).toString(), QString("mister"));
        QCOMPARE(model.data(model.index(3,2)).toString(), QString("herr"));

    }
}

void tst_QSqlRelationalTableModel::whiteSpaceInIdentifiers()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (!testWhiteSpaceNames(db.driverName()))
        QSKIP("White space test irrelevant for driver");
    QSqlRelationalTableModel model(0, db);
    model.setTable(db.driver()->escapeIdentifier(qTableName("rel", __FILE__, db) + " test6", QSqlDriver::TableName));
    model.setSort(0, Qt::DescendingOrder);
    model.setRelation(1, QSqlRelation(db.driver()->escapeIdentifier(qTableName("rel", __FILE__, db) + " test7", QSqlDriver::TableName),
                        db.driver()->escapeIdentifier("city id", QSqlDriver::FieldName),
                        db.driver()->escapeIdentifier("city name", QSqlDriver::FieldName)));
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(0,1)).toString(), QString("Washington"));
    QCOMPARE(model.data(model.index(1,1)).toString(), QString("New York"));

    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(0,1)).toString(), QString("Washington"));
    QCOMPARE(model.data(model.index(1,1)).toString(), QString("New York"));

    model.setJoinMode(QSqlRelationalTableModel::InnerJoin);
    QVERIFY_SQL(model, select());

    QSqlRecord rec;
    QSqlField f1("id", QVariant::Int);
    QSqlField f2(db.driver()->escapeIdentifier("city key", QSqlDriver::FieldName), QVariant::Int);
    QSqlField f3(db.driver()->escapeIdentifier("extra field", QSqlDriver::FieldName), QVariant::Int);

    f1.setValue(3);
    f2.setValue(2);
    f3.setValue(7);

    f1.setGenerated(true);
    f2.setGenerated(true);
    f3.setGenerated(true);

    rec.append(f1);
    rec.append(f2);
    rec.append(f3);

    QVERIFY_SQL(model, insertRecord(-1, rec));
    model.submitAll();
    if (model.editStrategy() != QSqlTableModel::OnManualSubmit)
        QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 3);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("Washington"));
    QCOMPARE(model.data(model.index(0, 2)).toInt(), 7);

    model.setEditStrategy(QSqlTableModel::OnManualSubmit);

    QSqlRecord recNew;
    QSqlField f1New("id", QVariant::Int);
    QSqlField f2New(db.driver()->escapeIdentifier("city key", QSqlDriver::FieldName), QVariant::Int);
    QSqlField f3New(db.driver()->escapeIdentifier("extra field", QSqlDriver::FieldName), QVariant::Int);

    f1New.setValue(4);
    f2New.setValue(1);
    f3New.setValue(6);

    f1New.setGenerated(true);
    f2New.setGenerated(true);
    f3New.setGenerated(true);

    recNew.append(f1New);
    recNew.append(f2New);
    recNew.append(f3New);

    QVERIFY_SQL(model, setRecord(0, recNew));

    QCOMPARE(model.data(model.index(0, 0)).toInt(), 4);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("New York"));
    QCOMPARE(model.data(model.index(0, 2)).toInt(), 6);

    QVERIFY_SQL(model, submitAll());
    QCOMPARE(model.data(model.index(0, 0)).toInt(), 4);
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("New York"));
    QCOMPARE(model.data(model.index(0, 2)).toInt(), 6);
}

void tst_QSqlRelationalTableModel::psqlSchemaTest()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    QSqlDriverPrivate::DBMSType dbType = tst_Databases::getDatabaseType(db);

    if (dbType != QSqlDriverPrivate::PostgreSQL)
        QSKIP("Postgresql specific test");

    QSqlRelationalTableModel model(0, db);
    QSqlQuery q(db);
    QVERIFY_SQL(q, exec("create schema " + qTableName("QTBUG_5373", __FILE__, db)));
    QVERIFY_SQL(q, exec("create schema " + qTableName("QTBUG_5373_s2", __FILE__, db)));
    QVERIFY_SQL(q, exec("create table " + qTableName("QTBUG_5373", __FILE__, db) + "." + qTableName("document", __FILE__, db) +
                        "(document_id int primary key, relatingid int, userid int)"));
    QVERIFY_SQL(q, exec("create table " + qTableName("QTBUG_5373_s2", __FILE__, db) + "." + qTableName("user", __FILE__, db) +
                        "(userid int primary key, username char(40))"));
    model.setTable(qTableName("QTBUG_5373", __FILE__, db) + "." + qTableName("document", __FILE__, db));
    model.setRelation(1, QSqlRelation(qTableName("QTBUG_5373_s2", __FILE__, db) + "." + qTableName("user", __FILE__, db), "userid", "username"));
    model.setRelation(2, QSqlRelation(qTableName("QTBUG_5373_s2", __FILE__, db) + "." + qTableName("user", __FILE__, db), "userid", "username"));
    QVERIFY_SQL(model, select());

    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(model, select());
}

void tst_QSqlRelationalTableModel::selectAfterUpdate()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QSqlRelationalTableModel model(0, db);
    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "tid", "title"));
    QVERIFY_SQL(model, select());
    QVERIFY(model.relationModel(2)->rowCount() == 2);
    {
        QSqlQuery q(db);
        QVERIFY_SQL(q, exec("insert into " + reltest2 + " values(3, 'mrs')"));
        model.relationModel(2)->select();
    }
    QVERIFY(model.relationModel(2)->rowCount() == 3);
    QVERIFY(model.setData(model.index(0,2), 3));
    QVERIFY(model.submitAll());
    QCOMPARE(model.data(model.index(0,2)), QVariant("mrs"));
}

/**
  This test case verifies bug fix for QTBUG-20038.
  */
void tst_QSqlRelationalTableModel::relationOnFirstColumn()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QString testTable1 = qTableName("QTBUG_20038_test1", __FILE__, db);
    QString testTable2 = qTableName("QTBUG_20038_test2", __FILE__, db);
    tst_Databases::safeDropTables(db, QStringList() << testTable1 << testTable2);

    //prepare test1 table
    QSqlQuery q(db);
    QVERIFY_SQL(q, exec("CREATE TABLE " + testTable1 + " (val1 INTEGER, id1 INTEGER PRIMARY KEY);"));
    QVERIFY_SQL(q, exec("DELETE FROM " + testTable1 + ";"));
    QVERIFY_SQL(q, exec("INSERT INTO " + testTable1 + " (id1, val1) VALUES(1, 10);"));
    QVERIFY_SQL(q, exec("INSERT INTO " + testTable1 + " (id1, val1) VALUES(2, 20);"));
    QVERIFY_SQL(q, exec("INSERT INTO " + testTable1 + " (id1, val1) VALUES(3, 30);"));

    //prepare test2 table
    QVERIFY_SQL(q, exec("CREATE TABLE " + testTable2 + " (id INTEGER PRIMARY KEY, name TEXT);"));
    QVERIFY_SQL(q, exec("DELETE FROM " + testTable2 + ";"));
    QVERIFY_SQL(q, exec("INSERT INTO " + testTable2 + " (id, name) VALUES (10, 'Hervanta');"));
    QVERIFY_SQL(q, exec("INSERT INTO " + testTable2 + " (id, name) VALUES (20, 'Keskusta');"));
    QVERIFY_SQL(q, exec("INSERT INTO " + testTable2 + " (id, name) VALUES (30, 'Annala');"));
    QVERIFY_SQL(q, exec("INSERT INTO " + testTable2 + " (id, name) VALUES (40, 'Tammela');"));
    QVERIFY_SQL(q, exec("INSERT INTO " + testTable2 + " (id, name) VALUES (50, 'Amuri');"));

    //set test model
    QSqlRelationalTableModel model(NULL, db);
    model.setTable(testTable1);
    model.setRelation(0, QSqlRelation(testTable2, "id", "name"));
    QVERIFY_SQL(model, select());

    //verify the data
    QCOMPARE(model.data(model.index(0, 0)), QVariant("Hervanta"));
    QCOMPARE(model.data(model.index(1, 0)), QVariant("Keskusta"));
    QCOMPARE(model.data(model.index(2, 0)), QVariant("Annala"));

    //modify the model data
    QVERIFY_SQL(model, setData(model.index(0, 0), 40));
    QVERIFY_SQL(model, submit());
    QVERIFY_SQL(model, setData(model.index(1, 0), 50));
    QVERIFY_SQL(model, submit());
    QVERIFY_SQL(model, setData(model.index(2, 0), 30));

    //verify the data after modificaiton
    QCOMPARE(model.data(model.index(0, 0)), QVariant("Tammela"));
    QCOMPARE(model.data(model.index(1, 0)), QVariant("Amuri"));
    QCOMPARE(model.data(model.index(2, 0)), QVariant("Annala"));

    tst_Databases::safeDropTables(db, QStringList() << testTable1 << testTable2);
}

QTEST_MAIN(tst_QSqlRelationalTableModel)
#include "tst_qsqlrelationaltablemodel.moc"
