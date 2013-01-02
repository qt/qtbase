/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "../qsqldatabase/tst_databases.h"

class tst_QSqlDriver : public QObject
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
    void record();
    void primaryIndex();
};


void tst_QSqlDriver::initTestCase_data()
{
    dbs.open();
    if (dbs.fillTestTable() == 0)
        QSKIP("No database drivers are available in this Qt configuration");
}

void tst_QSqlDriver::recreateTestTables(QSqlDatabase db)
{
    QSqlQuery q(db);
    const QString relTEST1(qTableName("relTEST1", __FILE__));

    if(tst_Databases::isPostgreSQL(db))
        QVERIFY_SQL( q, exec("set client_min_messages='warning'"));

    tst_Databases::safeDropTable( db, relTEST1 );

    QVERIFY_SQL( q, exec("create table " + relTEST1 +
            " (id int not null primary key, name varchar(20), title_key int, another_title_key int)"));
    QVERIFY_SQL( q, exec("insert into " + relTEST1 + " values(1, 'harry', 1, 2)"));
    QVERIFY_SQL( q, exec("insert into " + relTEST1 + " values(2, 'trond', 2, 1)"));
    QVERIFY_SQL( q, exec("insert into " + relTEST1 + " values(3, 'vohi', 1, 2)"));
    QVERIFY_SQL( q, exec("insert into " + relTEST1 + " values(4, 'boris', 2, 2)"));
}

void tst_QSqlDriver::initTestCase()
{
    foreach (const QString &dbname, dbs.dbNames)
        recreateTestTables(QSqlDatabase::database(dbname));
}

void tst_QSqlDriver::cleanupTestCase()
{
    foreach (const QString &dbName, dbs.dbNames) {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        tst_Databases::safeDropTable( db, qTableName( "relTEST1", __FILE__ ) );
    }
    dbs.close();
}

void tst_QSqlDriver::init()
{
}

void tst_QSqlDriver::cleanup()
{
}

void tst_QSqlDriver::record()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QString tablename(qTableName("relTEST1", __FILE__));
    QStringList fields;
    fields << "id" << "name" << "title_key" << "another_title_key";

    //check we can get records using an unquoted mixed case table name
    QSqlRecord rec = db.driver()->record(tablename);
    QCOMPARE(rec.count(), 4);

    // QTBUG-1363: QSqlField::length() always return -1 when using QODBC3 driver and QSqlDatabase::record()
    if (db.driverName().startsWith("QODBC") && tst_Databases::isSqlServer(db))
        QCOMPARE(rec.field(1).length(), 20);

    if (db.driverName().startsWith("QIBASE")|| db.driverName().startsWith("QOCI") || db.driverName().startsWith("QDB2"))
        for(int i = 0; i < fields.count(); ++i)
            fields[i] = fields[i].toUpper();

    for (int i = 0; i < fields.count(); ++i)
        QCOMPARE(rec.fieldName(i), fields[i]);

    if (db.driverName().startsWith("QIBASE") || db.driverName().startsWith("QOCI") || db.driverName().startsWith("QDB2"))
        tablename = tablename.toUpper();
    else if (db.driverName().startsWith("QPSQL"))
        tablename = tablename.toLower();

    if(!db.driverName().startsWith("QODBC") && !db.databaseName().contains("PostgreSql")) {
        //check we can get records using a properly quoted table name
        rec = db.driver()->record(db.driver()->escapeIdentifier(tablename,QSqlDriver::TableName));
        QCOMPARE(rec.count(), 4);
    }

    for (int i = 0; i < fields.count(); ++i)
        QCOMPARE(rec.fieldName(i), fields[i]);

    if( db.driverName().startsWith("QIBASE") || db.driverName().startsWith("QOCI") || db.driverName().startsWith("QDB2"))
        tablename = tablename.toLower();
    else if (db.driverName().startsWith("QPSQL"))
        tablename = tablename.toUpper();

    //check that we can't get records using incorrect tablename casing that's been quoted
    rec = db.driver()->record(db.driver()->escapeIdentifier(tablename,QSqlDriver::TableName));
    if (tst_Databases::isMySQL(db)
      || db.driverName().startsWith("QSQLITE")
      || db.driverName().startsWith("QTDS")
      || tst_Databases::isSqlServer(db)
      || tst_Databases::isMSAccess(db))
        QCOMPARE(rec.count(), 4); //mysql, sqlite and tds will match
    else
        QCOMPARE(rec.count(), 0);

}

void tst_QSqlDriver::primaryIndex()
{
    QFETCH_GLOBAL(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    QString tablename(qTableName("relTEST1", __FILE__));
    //check that we can get primary index using unquoted mixed case table name
    QSqlIndex index = db.driver()->primaryIndex(tablename);
    QCOMPARE(index.count(), 1);

    if( db.driverName().startsWith("QIBASE") || db.driverName().startsWith("QOCI") || db.driverName().startsWith("QDB2"))
        QCOMPARE(index.fieldName(0), QString::fromLatin1("ID"));
    else
        QCOMPARE(index.fieldName(0), QString::fromLatin1("id"));


    //check that we can get the primary index using a quoted tablename
    if( db.driverName().startsWith("QIBASE") || db.driverName().startsWith("QOCI") || db.driverName().startsWith("QDB2"))
        tablename = tablename.toUpper();
    else if (db.driverName().startsWith("QPSQL"))
        tablename = tablename.toLower();

    if(!db.driverName().startsWith("QODBC") && !db.databaseName().contains("PostgreSql")) {
        index = db.driver()->primaryIndex(db.driver()->escapeIdentifier(tablename, QSqlDriver::TableName));
        QCOMPARE(index.count(), 1);
    }
    if( db.driverName().startsWith("QIBASE") || db.driverName().startsWith("QOCI") || db.driverName().startsWith("QDB2"))
        QCOMPARE(index.fieldName(0), QString::fromLatin1("ID"));
    else
        QCOMPARE(index.fieldName(0), QString::fromLatin1("id"));



    //check that we can not get the primary index using a quoted but incorrect table name casing
    if( db.driverName().startsWith("QIBASE") || db.driverName().startsWith("QOCI") || db.driverName().startsWith("QDB2"))
        tablename = tablename.toLower();
    else if (db.driverName().startsWith("QPSQL"))
        tablename = tablename.toUpper();

    index = db.driver()->primaryIndex(db.driver()->escapeIdentifier(tablename, QSqlDriver::TableName));
    if (tst_Databases::isMySQL(db)
      || db.driverName().startsWith("QSQLITE")
      || db.driverName().startsWith("QTDS")
      || tst_Databases::isSqlServer(db)
      || tst_Databases::isMSAccess(db))
        QCOMPARE(index.count(), 1); //mysql will always find the table name regardless of casing
    else
        QCOMPARE(index.count(), 0);
}

QTEST_MAIN(tst_QSqlDriver)
#include "tst_qsqldriver.moc"
