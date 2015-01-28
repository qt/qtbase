/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <qcoreapplication.h>
#include <qsqldatabase.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <qsqlrecord.h>
#include <qsql.h>
#include <qsqlresult.h>
#include <qsqldriver.h>
#include <qdebug.h>
#include <private/qsqlnulldriver_p.h>

#include "../qsqldatabase/tst_databases.h"

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
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    QCoreApplication app(argc, argv, false);
    tst_Databases dbs;
    QVERIFY(dbs.open());

    foreach (const QString& dbName, dbs.dbNames) {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        QVERIFY_SQL(db, isValid());

        QStringList tables = db.tables();
        QString tableName;

        if (tables.contains("qtest_basictest.txt"))
            tableName = "qtest_basictest.txt";
        else if (tables.contains("qtest_basictest"))
            tableName = "qtest_basictest";
        else if (tables.contains("QTEST_BASICTEST"))
            tableName = "QTEST_BASICTEST";
        else {
            QVERIFY(1);
            continue;
        }

        qDebug("Testing: %s", qPrintable(tst_Databases::dbToString(db)));

        QSqlRecord rInf = db.record(tableName);
        QCOMPARE(rInf.count(), 2);
        QCOMPARE(rInf.fieldName(0).toLower(), QString("id"));
        QCOMPARE(rInf.fieldName(1).toLower(), QString("name"));
    }

    dbs.close();
    QVERIFY(1); // make sure the test doesn't fail if no database drivers are there
}

// make sure that the static stuff will be deleted
// when using multiple QCoreApplication objects
void tst_QSql::open()
{
    int i;
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    int count = -1;
    for (i = 0; i < 10; ++i) {
        QCoreApplication app(argc, argv, false);
        tst_Databases dbs;

        QVERIFY(dbs.open());
        if (count == -1)
            // first iteration: see how many dbs are open
            count = (int) dbs.dbNames.count();
        else
            // next iterations: make sure all are opened again
            QCOMPARE(count, (int)dbs.dbNames.count());
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
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    QCoreApplication app(argc, argv, false);
    tst_Databases dbs;

    QVERIFY(dbs.open());
    foreach (const QString& dbName, dbs.dbNames) {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        QVERIFY(db.isValid());
        if (tst_Databases::isMSAccess(db))
            continue;

        QSqlDatabase ndb = QSqlDatabase::addDatabase(db.driverName(), "tst_QSql::concurrentAccess");
        ndb.setDatabaseName(db.databaseName());
        ndb.setHostName(db.hostName());
        ndb.setPort(db.port());
        ndb.setUserName(db.userName());
        ndb.setPassword(db.password());
        QVERIFY_SQL(ndb, open());

        QCOMPARE(db.tables(), ndb.tables());
        ndb.close();
    }
    // no database servers installed - don't fail
    QVERIFY(1);
    dbs.close();
}

void tst_QSql::openErrorRecovery()
{
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    QCoreApplication app(argc, argv, false);
    tst_Databases dbs;

    QVERIFY(dbs.addDbs());
    if (dbs.dbNames.isEmpty())
        QSKIP("No database drivers installed");
    foreach (const QString& dbName, dbs.dbNames) {
        QSqlDatabase db = QSqlDatabase::database(dbName, false);
        CHECK_DATABASE(db);

        QString userName = db.userName();
        QString password = db.password();

        // force an open error
        if (db.open("dummy130977", "doesnt_exist")) {
            qDebug("Promiscuous database server without access control - test skipped for %s",
                   qPrintable(tst_Databases::dbToString(db)));
            QVERIFY(1);
            continue;
        }

        QFAIL_SQL(db, isOpen());
        QVERIFY_SQL(db, isOpenError());

        // now open it
        if (!db.open(userName, password)) {
            qDebug() << "Could not open Database " << tst_Databases::dbToString(db) <<
                        ". Assuming DB is down, skipping... (Error: " <<
                        tst_Databases::printError(db.lastError()) << ")";
            continue;
        }
        QVERIFY_SQL(db, open(userName, password));
        QVERIFY_SQL(db, isOpen());
        QFAIL_SQL(db, isOpenError());
        db.close();
        QFAIL_SQL(db, isOpen());

        // force another open error
        QFAIL_SQL(db, open("dummy130977", "doesnt_exist"));
        QFAIL_SQL(db, isOpen());
        QVERIFY_SQL(db, isOpenError());
    }
}

void tst_QSql::registerSqlDriver()
{
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    QCoreApplication app(argc, argv, false);

    QSqlDatabase::registerSqlDriver("QSQLTESTDRIVER", new QSqlDriverCreator<QSqlNullDriver>);
    QVERIFY(QSqlDatabase::drivers().contains("QSQLTESTDRIVER"));

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLTESTDRIVER");
    QVERIFY(db.isValid());

    QCOMPARE(db.tables(), QStringList());
}

QTEST_APPLESS_MAIN(tst_QSql)
#include "tst_qsql.moc"
