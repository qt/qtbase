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


#include "../qsqldatabase/tst_databases.h"

#include <QtCore>
#include <QtSql>
#include "qdebug.h"

#ifdef Q_OS_LINUX
#include <sched.h>
#endif

const QString qtest(qTableName("qtest", __FILE__, QSqlDatabase()));
// set this define if Oracle is built with threading support
//#define QOCI_THREADED

class tst_QSqlThread : public QObject
{
    Q_OBJECT

public:
    tst_QSqlThread();
    virtual ~tst_QSqlThread();


    void dropTestTables();
    void createTestTables();
    void recreateTestTables();
    void repopulateTestTables();

    void generic_data(const QString &engine=QString());
    tst_Databases dbs;

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

protected slots:
    void threadFinished() { ++threadFinishedCount; }

private slots:
    void simpleThreading_data() { generic_data(); }
    void simpleThreading();
    void readWriteThreading_data() { generic_data(); }
    void readWriteThreading();
    void readFromSingleConnection_data() { generic_data(); }
    void readFromSingleConnection();
    void readWriteFromSingleConnection_data() { generic_data(); }
    void readWriteFromSingleConnection();
    void preparedReadWriteFromSingleConnection_data() { generic_data(); }
    void preparedReadWriteFromSingleConnection();
    void transactionsFromSingleConnection_data() { generic_data(); }
    void transactionsFromSingleConnection();

private:
    int threadFinishedCount;
};

static QAtomicInt counter;

class QtTestSqlThread : public QThread
{
    Q_OBJECT
public:
    QtTestSqlThread(const QSqlDatabase &aDb, QObject *parent = 0)
        : QThread(parent), sourceDb(aDb) {}

    void runHelper(const QString &dbName)
    {
        QSqlDatabase db = QSqlDatabase::cloneDatabase(sourceDb, dbName);
        QVERIFY_SQL(db, open());

        int sum = 0;
        QSqlQuery q("select id from " + qtest, db);
        QVERIFY_SQL(q, isActive());
        while (q.next())
            sum += q.value(0).toInt();
        QCOMPARE(sum, 6);
        q.clear();
    }

    void run()
    {
        QString dbName = QString("QThreadDb%1").arg((size_t)currentThreadId());
        runHelper(dbName);

        QSqlDatabase::database(dbName).close();
        QSqlDatabase::removeDatabase(dbName);
    }

private:
    QSqlDatabase sourceDb;
};

enum { ProdConIterations = 10 };

class SqlProducer: public QThread
{
    Q_OBJECT
public:
    SqlProducer(const QSqlDatabase &aDb, QObject *parent = 0)
        : QThread(parent), sourceDb(aDb) {}

    void runHelper(const QString &dbName)
    {
        QSqlDatabase db = QSqlDatabase::cloneDatabase(sourceDb, dbName);
        QVERIFY_SQL(db, open());
        QSqlQuery q(db);
        QVERIFY_SQL(q, prepare("insert into " + qtest + " values (?, ?, ?)"));
        int id = 10;
        for (int i = 0; i < ProdConIterations; ++i) {
            q.bindValue(0, ++id);
            q.bindValue(1, "threaddy");
            q.bindValue(2, 10);
            QVERIFY_SQL(q, exec());
#ifdef Q_OS_LINUX
            sched_yield();
#endif
        }
    }

    void run()
    {
        QString dbName = QString("Producer%1").arg((size_t)currentThreadId());
        runHelper(dbName);
        QSqlDatabase::database(dbName).close();
        QSqlDatabase::removeDatabase(dbName);
    }
private:
    QSqlDatabase sourceDb;
};

class SqlConsumer: public QThread
{
    Q_OBJECT

public:
    SqlConsumer(const QSqlDatabase &aDb, QObject *parent = 0)
        : QThread(parent), sourceDb(aDb) {}

    void runHelper(const QString &dbName)
    {
        QSqlDatabase db = QSqlDatabase::cloneDatabase(sourceDb, dbName);
        QVERIFY_SQL(db, open());
        QSqlQuery q1(db), q2(db);
        QVERIFY_SQL(q2, prepare("delete from " + qtest + " where id = :id"));

        for (int i = 0; i < ProdConIterations; ++i) {
            QVERIFY_SQL(q1, exec("select max(id) from " + qtest));
            q1.first();
            q2.bindValue("id", q1.value(0));
            q1.clear();
            QVERIFY_SQL(q2, exec());
#ifdef Q_OS_LINUX
            sched_yield();
#endif
        }
    }

    void run()
    {
        QString dbName = QString("Consumer%1").arg((size_t)currentThreadId());
        runHelper(dbName);
        QSqlDatabase::database(dbName).close();
        QSqlDatabase::removeDatabase(dbName);
    }

private:
    QSqlDatabase sourceDb;
};

class SqlThread: public QThread
{
    Q_OBJECT

public:
    enum Mode { SimpleReading, PreparedReading, SimpleWriting, PreparedWriting };

    SqlThread(Mode m, const QSqlDatabase &db, QObject *parent = 0)
        : QThread(parent), sourceDb(db), mode(m) {}

    void run()
    {
        QSqlDatabase &db = sourceDb;
        switch (mode) {
        case SimpleReading: {
            // Executes a Query for reading, iterates over the first 4 results
            QSqlQuery q(sourceDb);
            for (int j = 0; j < ProdConIterations; ++j) {
                QVERIFY_SQL(q, exec("select id,name from " + qtest + " order by id"));
                for (int i = 1; i < 4; ++i) {
                    QVERIFY_SQL(q, next());
                    QCOMPARE(q.value(0).toInt(), i);
                }
            }
            break; }
        case SimpleWriting: {
            // Executes a query for writing (appends a new row)
            QSqlQuery q(sourceDb);
            for (int j = 0; j < ProdConIterations; ++j) {
                QVERIFY_SQL(q, exec(QString("insert into " + qtest
                                + " (id, name) values(%1, '%2')")
                                      .arg(counter.fetchAndAddRelaxed(1)).arg("Robert")));
            }
            break; }
        case PreparedReading: {
            // Prepares a query for reading and iterates over the results
            QSqlQuery q(sourceDb);
            QVERIFY_SQL(q, prepare("select id, name from " + qtest + " where id = ?"));
            for (int j = 0; j < ProdConIterations; ++j) {
                q.addBindValue(j % 3 + 1);
                QVERIFY_SQL(q, exec());
                QVERIFY_SQL(q, next());
                QCOMPARE(q.value(0).toInt(), j % 3 + 1);
            }
            break; }
        case PreparedWriting: {
            QSqlQuery q(sourceDb);
            QVERIFY_SQL(q, prepare("insert into " + qtest + " (id, name) "
                                     "values(?, ?)"));
            for (int i = 0; i < ProdConIterations; ++i) {
                q.addBindValue(counter.fetchAndAddRelaxed(1));
                q.addBindValue("Robert");
                QVERIFY_SQL(q, exec());
            }
            break; }
        }
    }

private:
    QSqlDatabase sourceDb;
    Mode mode;
};


tst_QSqlThread::tst_QSqlThread()
    : threadFinishedCount(0)
{
}

tst_QSqlThread::~tst_QSqlThread()
{
}

void tst_QSqlThread::generic_data(const QString& engine)
{
    if ( dbs.fillTestTable(engine) == 0 ) {
        if(engine.isEmpty())
           QSKIP( "No database drivers are available in this Qt configuration");
        else
           QSKIP( (QString("No database drivers of type %1 are available in this Qt configuration").arg(engine)).toLocal8Bit());
    }
}

void tst_QSqlThread::dropTestTables()
{
    for (int i = 0; i < dbs.dbNames.count(); ++i) {
        QSqlDatabase db = QSqlDatabase::database(dbs.dbNames.at(i));
        QSqlQuery q(db);

        tst_Databases::safeDropTables(db, QStringList() << qtest << qTableName("qtest2", __FILE__, db) << qTableName("emptytable", __FILE__, db));
    }
}

void tst_QSqlThread::createTestTables()
{
    for (int i = 0; i < dbs.dbNames.count(); ++i) {
        QSqlDatabase db = QSqlDatabase::database(dbs.dbNames.at(i));
        QSqlQuery q(db);

        QVERIFY_SQL(q, exec("create table " + qtest
                       + "(id int NOT NULL primary key, name varchar(20), title int)"));

        QVERIFY_SQL(q, exec("create table " + qTableName("qtest2", __FILE__, db)
                       + "(id int NOT NULL primary key, title varchar(20))"));

        QVERIFY_SQL(q, exec("create table " + qTableName("emptytable", __FILE__, db)
                       + "(id int NOT NULL primary key)"));
    }
}

void tst_QSqlThread::repopulateTestTables()
{
    for (int i = 0; i < dbs.dbNames.count(); ++i) {
        QSqlDatabase db = QSqlDatabase::database(dbs.dbNames.at(i));
        QSqlQuery q(db);

        QVERIFY_SQL(q, exec("delete from " + qtest));
        QVERIFY_SQL(q, exec("insert into " + qtest + " values(1, 'harry', 1)"));
        QVERIFY_SQL(q, exec("insert into " + qtest + " values(2, 'trond', 2)"));
        QVERIFY_SQL(q, exec("insert into " + qtest + " values(3, 'vohi', 3)"));

        QVERIFY_SQL(q, exec("delete from " + qTableName("qtest2", __FILE__, db)));
        QVERIFY_SQL(q, exec("insert into " + qTableName("qtest2", __FILE__, db) + " values(1, 'herr')"));
        QVERIFY_SQL(q, exec("insert into " + qTableName("qtest2", __FILE__, db) + " values(2, 'mister')"));
    }
}

void tst_QSqlThread::recreateTestTables()
{
    dropTestTables();
    createTestTables();
    repopulateTestTables();
}

void tst_QSqlThread::initTestCase()
{
    QVERIFY(dbs.open());
    recreateTestTables();
}

void tst_QSqlThread::cleanupTestCase()
{
    dropTestTables();
    dbs.close();
}

void tst_QSqlThread::init()
{
    threadFinishedCount = 0;
    counter.store(4);
}

void tst_QSqlThread::cleanup()
{
//     repopulateTestTables();
}

// This test creates two threads that clone their db connection and read
// from it
void tst_QSqlThread::simpleThreading()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (db.databaseName() == ":memory:")
        QSKIP("does not work with in-memory databases");

    QtTestSqlThread t1(db);
    QtTestSqlThread t2(db);

    connect(&t1, SIGNAL(finished()), this, SLOT(threadFinished()), Qt::QueuedConnection);
    connect(&t2, SIGNAL(finished()), this, SLOT(threadFinished()), Qt::QueuedConnection);

    t1.start();
    t2.start();

    while (threadFinishedCount < 2)
        QTest::qWait(100);
}

// This test creates two threads that clone their db connection and read
// or write
void tst_QSqlThread::readWriteThreading()
{
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (db.databaseName() == ":memory:")
        QSKIP("does not work with in-memory databases");
    else if (tst_Databases::isMSAccess(db))
        QSKIP("does not work with MS Access databases");

    SqlProducer producer(db);
    SqlConsumer consumer(db);

    connect(&producer, SIGNAL(finished()), this, SLOT(threadFinished()), Qt::QueuedConnection);
    connect(&consumer, SIGNAL(finished()), this, SLOT(threadFinished()), Qt::QueuedConnection);

    producer.start();
    consumer.start();

    while (threadFinishedCount < 2)
        QTest::qWait(100);
}

// run with n threads in parallel. Change this constant to hammer the poor DB server even more
static const int maxThreadCount = 4;

void tst_QSqlThread::readFromSingleConnection()
{
#ifdef QOCI_THREADED
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (db.databaseName() == ":memory:")
        QSKIP("does not work with in-memory databases");

    QObject cleanupHelper; // make sure the threads die when we exit the scope
    for (int i = 0; i < maxThreadCount; ++i) {
        SqlThread *reader = new SqlThread(SqlThread::SimpleReading, db, &cleanupHelper);
        connect(reader, SIGNAL(finished()), this, SLOT(threadFinished()), Qt::QueuedConnection);
        reader->start();
    }

    while (threadFinishedCount < maxThreadCount)
        QTest::qWait(100);
#endif
}

void tst_QSqlThread::readWriteFromSingleConnection()
{
#ifdef QOCI_THREADED
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (db.databaseName() == ":memory:")
        QSKIP("does not work with in-memory databases");

    QObject cleanupHelper;
    for (int i = 0; i < maxThreadCount; ++i) {
        SqlThread *reader = new SqlThread(SqlThread::SimpleReading, db, &cleanupHelper);
        connect(reader, SIGNAL(finished()), this, SLOT(threadFinished()), Qt::QueuedConnection);
        reader->start();

        SqlThread *writer = new SqlThread(SqlThread::SimpleWriting, db, &cleanupHelper);
        connect(writer, SIGNAL(finished()), this, SLOT(threadFinished()), Qt::QueuedConnection);
        writer->start();
    }

    while (threadFinishedCount < maxThreadCount * 2)
        QTest::qWait(100);
#endif
}

void tst_QSqlThread::preparedReadWriteFromSingleConnection()
{
#ifdef QOCI_THREADED
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (db.databaseName() == ":memory:")
        QSKIP("does not work with in-memory databases");

    QObject cleanupHelper;
    for (int i = 0; i < maxThreadCount; ++i) {
        SqlThread *reader = new SqlThread(SqlThread::PreparedReading, db, &cleanupHelper);
        connect(reader, SIGNAL(finished()), this, SLOT(threadFinished()), Qt::QueuedConnection);
        reader->start();

        SqlThread *writer = new SqlThread(SqlThread::PreparedWriting, db, &cleanupHelper);
        connect(writer, SIGNAL(finished()), this, SLOT(threadFinished()), Qt::QueuedConnection);
        writer->start();
    }

    while (threadFinishedCount < maxThreadCount * 2)
        QTest::qWait(100);
#endif
}

void tst_QSqlThread::transactionsFromSingleConnection()
{
#ifdef QOCI_THREADED
    QFETCH(QString, dbName);
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);

    if (db.databaseName() == ":memory:")
        QSKIP("does not work with in-memory databases");

    // start and commit a transaction
    QVERIFY_SQL(db, db.transaction());
    preparedReadWriteFromSingleConnection(); // read and write from multiple threads
    if (QTest::currentTestFailed())
        return;
    QVERIFY_SQL(db, db.commit());

    // reset test environment
    threadFinishedCount = 0;

    // start and roll back a transaction
    QVERIFY_SQL(db, db.transaction());
    preparedReadWriteFromSingleConnection(); // read and write from multiple threads
    if (QTest::currentTestFailed())
        return;
    QVERIFY_SQL(db, db.rollback());
#endif
}

QTEST_MAIN(tst_QSqlThread)
#include "tst_qsqlthread.moc"
