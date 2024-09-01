// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
#include <QtSql/QtSql>

#include "../../kernel/qsqldatabase/tst_databases.h"

class tst_QSqlRelationalTableModel : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

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
    void setRelation();
    void setMultipleRelations();

private:
    void fixupTableNamesForDb(const QSqlDatabase &db);
    void recreateTestTables(const QSqlDatabase &db);
    void dropTestTables(const QSqlDatabase &db);
    static QString escapeTableName(const QSqlDatabase &db, const QString &name)
    {
        QString _name = name;
        const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
        if (dbType == QSqlDriver::Oracle ||
            dbType == QSqlDriver::DB2)
            _name = name.toUpper();
        return db.driver()->escapeIdentifier(_name, QSqlDriver::TableName);
    }
    static QString escapeFieldName(const QSqlDatabase &db, const QString &name)
    {
        QString _name = name;
        const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
        if (dbType == QSqlDriver::Interbase ||
            dbType == QSqlDriver::Oracle ||
            dbType == QSqlDriver::DB2)
            _name = name.toUpper();
        return db.driver()->escapeIdentifier(_name, QSqlDriver::FieldName);
    }
    QString reltest1;
    QString reltest2;
    QString reltest3;
    QString reltest4;
    QString reltest5;
    tst_Databases dbs;
};

void tst_QSqlRelationalTableModel::fixupTableNamesForDb(const QSqlDatabase &db)
{
    reltest1 = qTableName("reltest1", __FILE__, db);
    reltest2 = qTableName("reltest2", __FILE__, db);
    reltest3 = qTableName("reltest3", __FILE__, db);
    reltest4 = qTableName("reltest4", __FILE__, db);
    reltest5 = qTableName("reltest5", __FILE__, db);
}

void tst_QSqlRelationalTableModel::initTestCase_data()
{
    QVERIFY(dbs.open());
    if (dbs.fillTestTable() == 0)
        QSKIP("No database drivers are available in this Qt configuration");
}

void tst_QSqlRelationalTableModel::recreateTestTables(const QSqlDatabase &db)
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

    QVERIFY_SQL( q, exec("create table " + reltest2 + " (id int not null primary key, title varchar(20))"));
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

    const auto reltest6 = qTableName("rel test6", __FILE__, db);
    const auto cityKeyStr = db.driver()->escapeIdentifier("city key", QSqlDriver::FieldName);
    const auto extraFieldStr = db.driver()->escapeIdentifier("extra field", QSqlDriver::FieldName);
    QVERIFY_SQL( q, exec("create table " + reltest6 + " (id int not null primary key, " + cityKeyStr +
                " int, " + extraFieldStr + " int)"));
    QVERIFY_SQL( q, exec("insert into " + reltest6 + " values(1, 1,9)"));
    QVERIFY_SQL( q, exec("insert into " + reltest6 + " values(2, 2,8)"));

    const auto reltest7 = qTableName("rel test7", __FILE__, db);
    const auto cityIdStr = db.driver()->escapeIdentifier("city id", QSqlDriver::TableName);
    const auto cityNameStr = db.driver()->escapeIdentifier("city name", QSqlDriver::FieldName);
    QVERIFY_SQL( q, exec("create table " + reltest7 + " (" + cityIdStr + " int not null primary key, " +
                         cityNameStr + " varchar(20))"));
    QVERIFY_SQL( q, exec("insert into " + reltest7 + " values(1, 'New York')"));
    QVERIFY_SQL( q, exec("insert into " + reltest7 + " values(2, 'Washington')"));
}

void tst_QSqlRelationalTableModel::initTestCase()
{
    for (const QString &dbName : std::as_const(dbs.dbNames)) {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        QSqlQuery q(db);
        QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
        if (dbType == QSqlDriver::Interbase) {
            q.exec("SET DIALECT 3");
        } else if (dbType == QSqlDriver::MSSqlServer) {
            q.exec("SET ANSI_DEFAULTS ON");
            q.exec("SET IMPLICIT_TRANSACTIONS OFF");
        } else if (dbType == QSqlDriver::PostgreSQL) {
            q.exec("set client_min_messages='warning'");
        }
        recreateTestTables(db);
    }
}

void tst_QSqlRelationalTableModel::cleanupTestCase()
{
    for (const QString &dbName : std::as_const(dbs.dbNames)) {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        CHECK_DATABASE(db);
        dropTestTables(db);
    }
    dbs.close();
}

void tst_QSqlRelationalTableModel::dropTestTables(const QSqlDatabase &db)
{
    fixupTableNamesForDb(db);
    QStringList tableNames{reltest1, reltest2, reltest3, reltest4, reltest5,
                           qTableName("rel test6", __FILE__, db),
                           qTableName("rel test7", __FILE__, db),
                           qTableName("CASETEST1", __FILE__, db),
                           qTableName("casetest1", __FILE__, db)};
    tst_Databases::safeDropTables( db, tableNames );

    QSqlQuery q(db);
    q.exec("DROP SCHEMA " + qTableName("QTBUG_5373", __FILE__, db) + " CASCADE");
    q.exec("DROP SCHEMA " + qTableName("QTBUG_5373_s2", __FILE__, db) + " CASCADE");
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
    fixupTableNamesForDb(db);

    QSqlRelationalTableModel model(0, db);

    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
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
    fixupTableNamesForDb(db);
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);

    // set the values using OnRowChange Strategy
    {
        QSqlRelationalTableModel model(0, db);

        model.setTable(reltest1);
        model.setSort(0, Qt::AscendingOrder);
        model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
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

        model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
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
        model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
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

        model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
        QVERIFY_SQL(model, select());
        QCOMPARE(model.data(model.index(2, 2)).toString(), QString("mister"));
    }

    //set values using OnManualSubmit strategy
    {
        QSqlRelationalTableModel model(0, db);

        model.setTable(reltest1);
        model.setRelation(2, QSqlRelation(reltest2, "id", "title"));

        //sybase doesn't allow tables with the same alias used twice as col names
        //so don't set up an identical relation when using the tds driver
        if (dbType != QSqlDriver::Sybase)
            model.setRelation(3, QSqlRelation(reltest2, "id", "title"));

        model.setEditStrategy(QSqlTableModel::OnManualSubmit);
        model.setSort(0, Qt::AscendingOrder);
        QVERIFY_SQL(model, select());

        QVERIFY(model.setData(model.index(2, 1), QString("vohi2")));
        QVERIFY(model.setData(model.index(3, 2), 1));
        QVERIFY(model.setData(model.index(0, 3), 1));

        QCOMPARE(model.data(model.index(2, 1)).toString(), QString("vohi2"));
        QCOMPARE(model.data(model.index(3, 2)).toString(), QString("herr"));
        if (dbType != QSqlDriver::Sybase)
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

        model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
        if (dbType != QSqlDriver::Sybase)
            model.setRelation(3, QSqlRelation(reltest2, "id", "title"));
        QVERIFY_SQL(model, select());
        QCOMPARE(model.data(model.index(3, 2)).toString(), QString("herr"));

        if (dbType != QSqlDriver::Sybase)
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
        model.setSort(0, Qt::AscendingOrder);
        QVERIFY_SQL(model, select());

        QCOMPARE(model.data(model.index(0,1)).toString(), QString("Mr"));
        QVERIFY(model.setData(model.index(0,1), QString("herr")));
        QCOMPARE(model.data(model.index(0,1)).toString(), QString("Hr"));
        QVERIFY_SQL(model, submitAll());

        QCOMPARE(model.data(model.index(0,1)).toString(), QString("Hr"));
    }

    // verify that clearing a foreign key works
    {
        QSqlRelationalTableModel model(0, db);

        model.setTable(reltest1);
        model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
        model.setSort(0, Qt::AscendingOrder);
        QVERIFY_SQL(model, select());

        QVERIFY(model.setData(model.index(0, 1), QString("harry2")));
        QVERIFY(model.setData(model.index(0, 2), 2));

        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry2"));
        QCOMPARE(model.data(model.index(0, 2)).toString(), QString("mister"));

        QVERIFY(model.setData(model.index(0, 2), QVariant())); // clear foreign key

        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("harry2"));
        QCOMPARE(model.data(model.index(0, 2)).toString(), QString()); // check that foreign value is not visible
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
    model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
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
    model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
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
    fixupTableNamesForDb(db);

    QSqlRelationalTableModel model(0, db);

    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
    model.setSort(0, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

    constexpr auto fkTitleKey = 4711;
    constexpr auto fkTitleVal = "new title";
    {
        auto relModel = model.relationModel(2);
        // make sure populateDictionary() is called
        relModel->select();

        QSqlRecord rec;
        QSqlField f1("id", QMetaType(QMetaType::Int));
        QSqlField f2("title", QMetaType(QMetaType::QString));

        f1.setValue(fkTitleKey);
        f2.setValue(fkTitleVal);

        f1.setGenerated(true);
        f2.setGenerated(true);

        rec.append(f1);
        rec.append(f2);

        QVERIFY(relModel->insertRecord(-1, rec));
    }

    QSqlRecord rec;
    QSqlField f1("id", QMetaType(QMetaType::Int));
    QSqlField f2("name", QMetaType(QMetaType::QString));
    QSqlField f3("title_key", QMetaType(QMetaType::Int));
    QSqlField f4("another_title_key", QMetaType(QMetaType::Int));

    f1.setValue(7);
    f2.setValue("test");
    f3.setValue(fkTitleKey);
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
    QCOMPARE(model.data(model.index(4, 2)).toString(), QString(fkTitleVal));

    // In LeftJoin mode, two additional rows are fetched
    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(6, 0)).toInt(), 7);
    QCOMPARE(model.data(model.index(6, 1)).toString(), QString("test"));
    QCOMPARE(model.data(model.index(6, 2)).toString(), QString(fkTitleVal));
}

void tst_QSqlRelationalTableModel::setRecord()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    recreateTestTables(db);

    QSqlRelationalTableModel model(0, db);

    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
    model.setSort(0, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

    QSqlRecord rec;
    QSqlField f1("id", QMetaType(QMetaType::Int));
    QSqlField f2("name", QMetaType(QMetaType::QString));
    QSqlField f3("title_key", QMetaType(QMetaType::Int));
    QSqlField f4("another_title_key", QMetaType(QMetaType::Int));

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
    fixupTableNamesForDb(db);
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);

    QSqlRelationalTableModel model(0, db);

    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
    model.setSort(0, Qt::AscendingOrder);

    if (dbType != QSqlDriver::Sybase)
        model.setRelation(3, QSqlRelation(reltest2, "id", "title"));
    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(0,0)).toInt(), 1);
    QCOMPARE(model.data(model.index(0,1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(0,2)).toString(), QString("herr"));
    if (dbType != QSqlDriver::Sybase)
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
    if (dbType != QSqlDriver::Sybase)
        QCOMPARE(model.data(model.index(0,3)).toString(), QString("herr"));
    else
        QCOMPARE(model.data(model.index(0,3)).toInt(), 1);

    QCOMPARE(model.data(model.index(1,0)).toInt(), 1);
    QCOMPARE(model.data(model.index(1,1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(1,2)).toString(), QString("herr"));
    if (dbType != QSqlDriver::Sybase)
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

    if (dbType != QSqlDriver::Sybase) {
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
    if (dbType != QSqlDriver::Sybase)
        QCOMPARE(model.data(model.index(0,3)).toString(), QString("mister"));
    else
        QCOMPARE(model.data(model.index(0,3)).toInt(), 2);


    QCOMPARE(model.data(model.index(1,0)).toInt(),1013);
    QCOMPARE(model.data(model.index(1,1)).toString(), QString("kramer"));
    QCOMPARE(model.data(model.index(1,2)).toString(), QString("mister"));
    if (dbType != QSqlDriver::Sybase)
        QCOMPARE(model.data(model.index(1,3)).toString(), QString("herr"));
    else
        QCOMPARE(model.data(model.index(1,3)).toInt(), 1);

    QCOMPARE(model.data(model.index(2,0)).toInt(), 1);
    QCOMPARE(model.data(model.index(2,1)).toString(), QString("harry"));
    QCOMPARE(model.data(model.index(2,2)).toString(), QString("herr"));
    if (dbType != QSqlDriver::Sybase)
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

    for (const auto mode : {QSqlRelationalTableModel::InnerJoin, QSqlRelationalTableModel::LeftJoin}) {
        recreateTestTables(db);

        QSqlRelationalTableModel model(0, db);

        model.setTable(reltest1);
        model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
        model.setJoinMode(mode);
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
    }
}

void tst_QSqlRelationalTableModel::filter()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    recreateTestTables(db);

    QSqlRelationalTableModel model(0, db);

    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
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
    fixupTableNamesForDb(db);
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);

    QSqlRelationalTableModel model(0, db);

    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
    if (dbType != QSqlDriver::Sybase)
        model.setRelation(3, QSqlRelation(reltest2, "id", "title"));

    model.setSort(2, Qt::DescendingOrder);
    QVERIFY_SQL(model, select());

    QCOMPARE(model.rowCount(), 4);
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("mister"));
    QCOMPARE(model.data(model.index(1, 2)).toString(), QString("mister"));
    QCOMPARE(model.data(model.index(2, 2)).toString(), QString("herr"));
    QCOMPARE(model.data(model.index(3, 2)).toString(), QString("herr"));


    model.setSort(3, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

     if (dbType != QSqlDriver::Sybase) {
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

    QStringList stringsInDatabaseOrder;
    // PostgreSQL puts the null ones (from the table with the original value) first in descending order
    // which translate to empty strings in the related table
    if (dbType == QSqlDriver::PostgreSQL || dbType == QSqlDriver::MimerSQL)
        stringsInDatabaseOrder << "" << "" << "mister" << "mister" << "herr" << "herr";
    else
        stringsInDatabaseOrder << "mister" << "mister" << "herr" << "herr" << "" << "";
    for (int i = 0; i < 6; ++i)
        QCOMPARE(model.data(model.index(i, 2)).toString(), stringsInDatabaseOrder.at(i));

    model.setSort(3, Qt::AscendingOrder);
    QVERIFY_SQL(model, select());

    // PostgreSQL puts the null ones (from the table with the original value) first in descending order
    // which translate to empty strings in the related table
    stringsInDatabaseOrder.clear();
    if (dbType == QSqlDriver::PostgreSQL || dbType == QSqlDriver::MimerSQL)
        stringsInDatabaseOrder << "herr" << "mister" << "mister" << "mister" << "mister" << "";
    else if (dbType != QSqlDriver::Sybase)
        stringsInDatabaseOrder << "" << "herr" << "mister" << "mister" << "mister" << "mister";

    if (dbType != QSqlDriver::Sybase) {
        QCOMPARE(model.rowCount(), 6);
        for (int i = 0; i < 6; ++i)
            QCOMPARE(model.data(model.index(i, 3)).toString(), stringsInDatabaseOrder.at(i));
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
    QCOMPARE(model.rowCount(), initialRowCount);

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
    fixupTableNamesForDb(db);

    QSqlRelationalTableModel model(0, db);

    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
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
    fixupTableNamesForDb(db);
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);

    QSqlRelationalTableModel model(0, db);

    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "id", "title"));

    if (dbType != QSqlDriver::Sybase)
        model.setRelation(3, QSqlRelation(reltest2, "id", "title"));
    model.setSort(1, Qt::AscendingOrder);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);

    QVERIFY_SQL(model, select());

    QCOMPARE(model.data(model.index(3, 0)).toInt(), 3);
    QCOMPARE(model.data(model.index(3, 1)).toString(), QString("vohi"));
    QCOMPARE(model.data(model.index(3, 2)).toString(), QString("herr"));
    if (dbType != QSqlDriver::Sybase)
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
    if (dbType != QSqlDriver::Sybase)
        QCOMPARE(model.data(model.index(0, 3)).toString(), QString("herr"));
    else
        QCOMPARE(model.data(model.index(0, 3)).toInt(), 1);

    QCOMPARE(model.data(model.index(4, 0)).toInt(), 3);
    QCOMPARE(model.data(model.index(4, 1)).toString(), QString("vohi"));
    QCOMPARE(model.data(model.index(4, 2)).toString(), QString("herr"));
    if (dbType != QSqlDriver::Sybase)
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
    fixupTableNamesForDb(db);
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);

    QSqlRelationalTableModel model(0, db);
    model.setTable(reltest3);
    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    model.setSort(0, Qt::AscendingOrder);

    // Duplication of "name", used in both reltest3 and reltest4.
    model.setRelation(2, QSqlRelation(reltest4, "id", "name"));
    QVERIFY_SQL(model, select());

    QString reltest4Unescaped = qTableName("reltest4", __FILE__, db, false);
    QString fieldName = reltest4Unescaped + QLatin1String("_name_2");
    if (dbType == QSqlDriver::Oracle || dbType == QSqlDriver::DB2)
        fieldName = fieldName.toUpper();
    fieldName.truncate(db.driver()->maximumIdentifierLength(QSqlDriver::TableName));
    QCOMPARE(model.record(1).value(fieldName).toString(), QLatin1String("Trondheim"));

    QSqlRecord rec = model.record();
    rec.setValue(0, 3);
    rec.setValue(1, "Berge");
    rec.setValue(2, 1); // Must insert the key value

    if (dbType == QSqlDriver::Interbase || dbType == QSqlDriver::Oracle || dbType == QSqlDriver::DB2) {
        QCOMPARE(rec.fieldName(0), QLatin1String("ID"));
        QCOMPARE(rec.fieldName(1), QLatin1String("NAME")); // This comes from main table
    } else {
        QCOMPARE(rec.fieldName(0), QLatin1String("id"));
        QCOMPARE(rec.fieldName(1), QLatin1String("name"));
    }

    // The duplicate field names is aliased because it's comes from the relation's display column.
    QCOMPARE(rec.fieldName(2), fieldName);

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
    fixupTableNamesForDb(db);

    QSqlRelationalTableModel model(0, db);
    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
    QVERIFY_SQL(model, select());

    //try set a non-existent relational key
    QVERIFY(model.setData(model.index(0, 2), 3) == false);
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));

    //try to set data in non valid index
    QVERIFY(!model.setData(model.index(0,10),5));

    //same test with LeftJoin mode
    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(model, select());

    //try set a non-existent relational key
    QVERIFY(model.setData(model.index(0, 2), 3) == false);
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("herr"));

    //try to set data in non valid index
    QVERIFY(!model.setData(model.index(0,10),5));
}

void tst_QSqlRelationalTableModel::relationModel()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    fixupTableNamesForDb(db);

    QSqlRelationalTableModel model(0, db);
    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
    QVERIFY_SQL(model, select());

    QVERIFY(!model.relationModel(0));
    QVERIFY(!model.relationModel(1));
    QVERIFY(model.relationModel(2) != NULL);
    QVERIFY(!model.relationModel(3));
    QVERIFY(!model.relationModel(4));

    model.setRelation(3, QSqlRelation(reltest4, "id", "name"));
    QVERIFY_SQL(model, select());

    QVERIFY(!model.relationModel(0));
    QVERIFY(!model.relationModel(1));
    QVERIFY(model.relationModel(2) != NULL);
    QVERIFY(model.relationModel(3) != NULL);
    QVERIFY(!model.relationModel(4));

    QSqlTableModel *rel_model = model.relationModel(2);
    QCOMPARE(rel_model->data(rel_model->index(0,1)).toString(), QString("herr"));

    //same test in JoinMode
    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(model, select());

    QVERIFY(!model.relationModel(0));
    QVERIFY(!model.relationModel(1));
    QVERIFY(model.relationModel(2) != NULL);
    QVERIFY(model.relationModel(3) != NULL);
    QVERIFY(!model.relationModel(4));

    QSqlTableModel *rel_model2 = model.relationModel(2);
    QCOMPARE(rel_model2->data(rel_model->index(0,1)).toString(), QString("herr"));
}

void tst_QSqlRelationalTableModel::casing()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    fixupTableNamesForDb(db);
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);

    if (dbType == QSqlDriver::SQLite || dbType == QSqlDriver::MSSqlServer)
        QSKIP("The casing test for this database is irrelevant since this database does not treat different cases as separate entities");

    QSqlQuery q(db);
    const QString caseTestUpper = qTableName("CASETEST1", __FILE__, db).toUpper();
    const QString caseTestLower = qTableName("casetest1", __FILE__, db);
    tst_Databases::safeDropTables(db, {caseTestUpper, caseTestLower});
    QVERIFY_SQL( q, exec("create table " + caseTestUpper +
                " (id int not null primary key, name varchar(20), title_key int, another_title_key int)"));

    if (!q.exec("create table " + caseTestLower +
                " (ident int not null primary key, name varchar(20), title_key int)"))
        QSKIP("The casing test for this database is irrelevant since this database does not treat different cases as separate entities");

    QVERIFY_SQL( q, exec("insert into " + caseTestUpper + " values(1, 'harry', 1, 2)"));
    QVERIFY_SQL( q, exec("insert into " + caseTestUpper + " values(2, 'trond', 2, 1)"));
    QVERIFY_SQL( q, exec("insert into " + caseTestUpper + " values(3, 'vohi', 1, 2)"));
    QVERIFY_SQL( q, exec("insert into " + caseTestUpper + " values(4, 'boris', 2, 2)"));
    QVERIFY_SQL( q, exec("insert into " + caseTestLower + " values(1, 'jerry', 1)"));
    QVERIFY_SQL( q, exec("insert into " + caseTestLower + " values(2, 'george', 2)"));
    QVERIFY_SQL( q, exec("insert into " + caseTestLower + " values(4, 'kramer', 2)"));

    if (dbType == QSqlDriver::Oracle) {
        //try an owner that doesn't exist
        QSqlRecord rec = db.driver()->record("doug." + caseTestUpper);
        QCOMPARE( rec.count(), 0);

        //try an owner that does exist
        rec = db.driver()->record(db.userName() + QLatin1Char('.') + caseTestUpper);
        QCOMPARE( rec.count(), 4);
    }
    QSqlRecord rec = db.driver()->record(caseTestUpper);
    QCOMPARE( rec.count(), 4);

    rec = db.driver()->record(caseTestLower);
    QCOMPARE( rec.count(), 3);

    QSqlTableModel upperCaseModel(0, db);
    upperCaseModel.setTable(caseTestUpper);

    QCOMPARE(upperCaseModel.tableName(), caseTestUpper);

    QVERIFY_SQL(upperCaseModel, select());

    QCOMPARE(upperCaseModel.rowCount(), 4);

    QSqlTableModel lowerCaseModel(0, db);
    lowerCaseModel.setTable(caseTestLower);
    QCOMPARE(lowerCaseModel.tableName(), caseTestLower);
    QVERIFY_SQL(lowerCaseModel, select());

    QCOMPARE(lowerCaseModel.rowCount(), 3);

    QSqlRelationalTableModel model(0, db);
    model.setTable(caseTestUpper);
    model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
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
    fixupTableNamesForDb(db);

    recreateTestTables(db);

    QSqlRelationalTableModel model(0, db);
    model.setTable(reltest1);

    //try with relation table name quoted
    model.setRelation(2, QSqlRelation(escapeTableName(db, reltest2), "id", "title"));
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
    model.setRelation(2, QSqlRelation(reltest2, escapeFieldName(db, "id"), "title"));
    model.setJoinMode(QSqlRelationalTableModel::InnerJoin);
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
    model.setRelation(2, QSqlRelation(reltest2, "id", escapeFieldName(db, "title")));
    model.setJoinMode(QSqlRelationalTableModel::InnerJoin);
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
    model.setRelation(2, QSqlRelation(escapeTableName(db, reltest2),
                                      escapeFieldName(db, "id"),
                                      escapeFieldName(db, "title")));
    model.setJoinMode(QSqlRelationalTableModel::InnerJoin);
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
    fixupTableNamesForDb(db);

    // set the values using OnRowChange Strategy with an escaped tablename
    {
        QSqlRelationalTableModel model(0, db);

        model.setTable(escapeTableName(db, reltest1));
        model.setSort(0, Qt::AscendingOrder);
        model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
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

        model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
        QVERIFY_SQL(model, select());
        QCOMPARE(model.data(model.index(0, 2)).toString(), QString("mister"));
        QCOMPARE(model.data(model.index(3,2)).toString(), QString("herr"));

    }

    //ok, now do same test with LeftJoin
    {
        QSqlRelationalTableModel model(0, db);

        model.setTable(escapeTableName(db, reltest1));
        model.setSort(0, Qt::AscendingOrder);
        model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
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

        model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
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
    fixupTableNamesForDb(db);

    QSqlRelationalTableModel model(0, db);
    model.setTable(qTableName("rel test6", __FILE__, db));
    model.setSort(0, Qt::DescendingOrder);
    model.setRelation(1, QSqlRelation(qTableName("rel test7", __FILE__, db),
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
    QSqlField f1("id", QMetaType(QMetaType::Int));
    QSqlField f2(db.driver()->escapeIdentifier("city key", QSqlDriver::FieldName), QMetaType(QMetaType::Int));
    QSqlField f3(db.driver()->escapeIdentifier("extra field", QSqlDriver::FieldName), QMetaType(QMetaType::Int));

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
    QSqlField f1New("id", QMetaType(QMetaType::Int));
    QSqlField f2New(db.driver()->escapeIdentifier("city key", QSqlDriver::FieldName), QMetaType(QMetaType::Int));
    QSqlField f3New(db.driver()->escapeIdentifier("extra field", QSqlDriver::FieldName), QMetaType(QMetaType::Int));

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
    fixupTableNamesForDb(db);
    QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);

    if (dbType != QSqlDriver::PostgreSQL)
        QSKIP("Postgresql specific test");

    QSqlRelationalTableModel model(0, db);
    QSqlQuery q(db);
    QVERIFY_SQL(q, exec("create schema " + qTableName("QTBUG_5373", __FILE__, db)));
    QVERIFY_SQL(q, exec("create schema " + qTableName("QTBUG_5373_s2", __FILE__, db)));
    QVERIFY_SQL(q, exec("create table " + qTableName("QTBUG_5373", __FILE__, db) + QLatin1Char('.') + qTableName("document", __FILE__, db) +
                        "(document_id int primary key, relatingid int, userid int)"));
    QVERIFY_SQL(q, exec("create table " + qTableName("QTBUG_5373_s2", __FILE__, db) + QLatin1Char('.') + qTableName("user", __FILE__, db) +
                        "(userid int primary key, username char(40))"));
    model.setTable(qTableName("QTBUG_5373", __FILE__, db) + QLatin1Char('.') + qTableName("document", __FILE__, db));
    model.setRelation(1, QSqlRelation(qTableName("QTBUG_5373_s2", __FILE__, db) + QLatin1Char('.') + qTableName("user", __FILE__, db), "userid", "username"));
    model.setRelation(2, QSqlRelation(qTableName("QTBUG_5373_s2", __FILE__, db) + QLatin1Char('.') + qTableName("user", __FILE__, db), "userid", "username"));
    QVERIFY_SQL(model, select());

    model.setJoinMode(QSqlRelationalTableModel::LeftJoin);
    QVERIFY_SQL(model, select());
}

void tst_QSqlRelationalTableModel::selectAfterUpdate()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    fixupTableNamesForDb(db);

    QSqlRelationalTableModel model(0, db);
    model.setTable(reltest1);
    model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
    QVERIFY_SQL(model, select());
    QCOMPARE(model.relationModel(2)->rowCount(), 2);
    {
        QSqlQuery q(db);
        QVERIFY_SQL(q, exec("insert into " + reltest2 + " values(3, 'mrs')"));
        model.relationModel(2)->select();
    }
    QCOMPARE(model.relationModel(2)->rowCount(), 3);
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
    fixupTableNamesForDb(db);

    QString testTable1 = qTableName("QTBUG_20038_test1", __FILE__, db);
    QString testTable2 = qTableName("QTBUG_20038_test2", __FILE__, db);
    tst_Databases::safeDropTables(db, QStringList() << testTable1 << testTable2);

    //prepare test1 table
    QSqlQuery q(db);
    QVERIFY_SQL(q, exec("CREATE TABLE " + testTable1 + " (val1 INTEGER, id1 INTEGER PRIMARY KEY);"));
    QVERIFY_SQL(q, exec("DELETE FROM " + testTable1 + QLatin1Char(';')));
    QVERIFY_SQL(q, exec("INSERT INTO " + testTable1 + " (id1, val1) VALUES(1, 10);"));
    QVERIFY_SQL(q, exec("INSERT INTO " + testTable1 + " (id1, val1) VALUES(2, 20);"));
    QVERIFY_SQL(q, exec("INSERT INTO " + testTable1 + " (id1, val1) VALUES(3, 30);"));

    //prepare test2 table
    if (tst_Databases::getDatabaseType(db) == QSqlDriver::MimerSQL) {
        QVERIFY_SQL(q,
                    exec("CREATE TABLE " + testTable2
                         + " (id INTEGER PRIMARY KEY, name NVARCHAR(100));"));
    } else {
        QVERIFY_SQL(q,
                    exec("CREATE TABLE " + testTable2 + " (id INTEGER PRIMARY KEY, name VARCHAR(100));"));
    }
    QVERIFY_SQL(q, exec("DELETE FROM " + testTable2 + QLatin1Char(';')));
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

void tst_QSqlRelationalTableModel::setRelation()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    recreateTestTables(db);

    QSqlRelationalTableModel model(0, db);
    model.setTable(reltest1);
    QVERIFY_SQL(model, select());
    QCOMPARE(model.data(model.index(0, 2)), QVariant(1));

    model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
    QVERIFY_SQL(model, select());
    QCOMPARE(model.data(model.index(0, 2)), QVariant("herr"));

    // Check that setting an invalid QSqlRelation() clears the relation
    model.setRelation(2, QSqlRelation());
    QVERIFY_SQL(model, select());
    QCOMPARE(model.data(model.index(0, 2)), QVariant(1));
}

void tst_QSqlRelationalTableModel::setMultipleRelations()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    recreateTestTables(db);

    QSqlRelationalTableModel model(0, db);
    model.setTable(reltest1);
    QVERIFY_SQL(model, select());
    model.setRelation(2, QSqlRelation(reltest2, "id", "title"));
    model.data(model.index(0, 2));  // initialize model for QSqlRelation above
    // id must be big enough that the internal QList needs to be reallocated
    model.setRelation(100, QSqlRelation(reltest2, "id", "title"));
    QSqlTableModel *relationModel = model.relationModel(2);
    QVERIFY(relationModel);
    QVERIFY(relationModel->select());
}

QTEST_MAIN(tst_QSqlRelationalTableModel)
#include "tst_qsqlrelationaltablemodel.moc"
