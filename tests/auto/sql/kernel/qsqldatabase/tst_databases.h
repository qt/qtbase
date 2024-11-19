// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
/* possible connection parameters */

#ifndef TST_DATABASES_H
#define TST_DATABASES_H

#include <QtTest/qtest.h>

#include <QtSql/qsqldatabase.h>
#include <QtSql/qsqldriver.h>
#include <QtSql/qsqlerror.h>
#include <QtSql/qsqlquery.h>
#include <QtSql/qsqlrecord.h>
#include <QtSql/qsqltablemodel.h>

#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qtemporarydir.h>
#include <QtCore/qversionnumber.h>

using namespace Qt::StringLiterals;

#define CHECK_DATABASE( db ) \
    if ( !db.isValid() ) { qFatal( "db is Invalid" ); }

#define QVERIFY_SQL(q, stmt) QVERIFY2((q).stmt, tst_Databases::printError((q).lastError(), db))
#define QFAIL_SQL(q, stmt) QVERIFY2(!(q).stmt, tst_Databases::printError((q).lastError(), db))

#define DBMS_SPECIFIC(db, driver) \
    if (!db.driverName().startsWith(driver)) { QSKIP(driver " specific test"); }

// to prevent nameclashes on our database server, each machine
// will use its own set of table names. Call this function to get
// "tablename_hostname"
inline static QString qTableName(const QString &prefix, const char *sourceFileName,
                                 QSqlDatabase db, bool escape = true)
{
    const auto hash = qHash(QLatin1String(sourceFileName) + '_' +
                            QSysInfo::machineHostName().replace('-', '_'));
    auto tableStr = QLatin1String("dbtst") + db.driverName() + '_' + prefix +
                    QString::number(hash, 16);
    // Oracle & Interbase/Firebird have a limit on the tablename length
    QSqlDriver *drv = db.driver();
    if (drv)
        tableStr.truncate(drv->maximumIdentifierLength(QSqlDriver::TableName));
    return escape ? db.driver()->escapeIdentifier(tableStr, QSqlDriver::TableName) : tableStr;
}

class tst_Databases
{
public:
    ~tst_Databases()
    {
        close();
    }

    // returns a testtable consisting of the names of all database connections if
    // driverPrefix is empty, otherwise only those that start with driverPrefix.
    int fillTestTable(const QString &driverPrefix = QString()) const
    {
        QTest::addColumn<QString>("dbName");
        int count = 0;

        for (const auto &dbName : std::as_const(dbNames)) {
            QSqlDatabase db = QSqlDatabase::database(dbName);
            if (!db.isValid())
                continue;
            if (driverPrefix.isEmpty() || db.driverName().startsWith(driverPrefix)) {
                QTest::newRow(dbName.toLatin1()) << dbName;
                ++count;
            }
        }
        return count;
    }

    int fillTestTableWithStrategies(const QString &driverPrefix = QString()) const
    {
        QTest::addColumn<QString>("dbName");
        QTest::addColumn<QSqlTableModel::EditStrategy>("submitpolicy");
        int count = 0;

        for (const auto &dbName : std::as_const(dbNames)) {
            QSqlDatabase db = QSqlDatabase::database(dbName);
            if (!db.isValid())
                continue;

            if ( driverPrefix.isEmpty() || db.driverName().startsWith( driverPrefix ) ) {
                QTest::newRow(QString("%1 [field]").arg(dbName).toLatin1() ) << dbName << QSqlTableModel::OnFieldChange;
                QTest::newRow(QString("%1 [row]").arg(dbName).toLatin1() ) << dbName << QSqlTableModel::OnRowChange;
                QTest::newRow(QString("%1 [manual]").arg(dbName).toLatin1() ) << dbName << QSqlTableModel::OnManualSubmit;
                ++count;
            }
        }
        return count;
    }

    void addDb(const QString &driver, const QString &dbName,
               const QString &user = QString(), const QString &passwd = QString(),
               const QString &host = QString(), int port = -1, const QString &params = QString())
    {
        if (!QSqlDatabase::drivers().contains(driver)) {
            qWarning() <<  "Driver" << driver << "is not installed";
            return;
        }

        // construct a stupid unique name
        QString cName = QString::number(counter++) + QLatin1Char('_') + driver + QLatin1Char('@');

        cName += host.isEmpty() ? dbName : host;

        if (port > 0)
            cName += QLatin1Char(':') + QString::number(port);

        QString opts = params;
        if (driver == "QSQLITE") {
            // Since the database for sqlite is generated at runtime it's always
            // available, but we use QTempDir so it's always in a different
            // location. Thus, let's ignore the path completely.
            cName = "SQLite";
            qInfo("SQLite will use the database located at %ls", qUtf16Printable(dbName));
            opts += QStringLiteral(";QSQLITE_ENABLE_NON_ASCII_CASE_FOLDING");
        }

        auto db = QSqlDatabase::addDatabase(driver, cName);
        if (!db.isValid()) {
            qWarning( "Could not create database object" );
            return;
        }

        db.setDatabaseName(dbName);
        db.setUserName(user);
        db.setPassword(passwd);
        db.setHostName(host);
        db.setPort(port);
        db.setConnectOptions(opts);
        dbNames.append(cName);
    }

    bool addDbs()
    {
        // Test databases can be defined in a file using the following format:
        //
        // {
        //  "entries": [
        //      {
        //          "driver": "QPSQL",
        //          "name": "testdb",
        //          "username": "postgres",
        //          "password": "password",
        //          "hostname": "localhost",
        //          "port": 5432,
        //          "parameters": "extraoptions"
        //      },
        //      {
        //          ....
        //      }
        //  ]
        // }

        bool added = false;
        const QString databasesFile(qgetenv("QT_TEST_DATABASES_FILE"));
        QFile f(databasesFile.isEmpty() ? "testdbs.json" : databasesFile);
        if (f.exists() && f.open(QIODevice::ReadOnly)) {
            const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            f.close();
            const QJsonValue entriesV = doc.object().value(QLatin1String("entries"));
            if (!entriesV.isArray()) {
                qWarning() << "No entries in " + f.fileName();
            } else {
                const QJsonArray entriesA = entriesV.toArray();
                for (const auto &elem : entriesA) {
                    if (elem.isObject()) {
                        const QJsonObject object = elem.toObject();
                        addDb(object.value(QStringLiteral("driver")).toString(),
                              object.value(QStringLiteral("name")).toString(),
                              object.value(QStringLiteral("username")).toString(),
                              object.value(QStringLiteral("password")).toString(),
                              object.value(QStringLiteral("hostname")).toString(),
                              object.value(QStringLiteral("port")).toInt(),
                              object.value(QStringLiteral("parameters")).toString());
                        added = true;
                    }
                }
            }
        }
        QTemporaryDir *sqLiteDir = dbDir();
        if (sqLiteDir) {
            addDb(QStringLiteral("QSQLITE"), QDir::toNativeSeparators(sqLiteDir->path() + QStringLiteral("/sqlite.db")));
            added = true;
        }
        return added;
    }

    // 'false' return indicates a system error, for example failure to create a temporary directory.
    bool open()
    {
        if (!addDbs())
            return false;

        auto it = dbNames.begin();
        while (it != dbNames.end()) {
            const auto &dbName = *it;
            QSqlDatabase db = QSqlDatabase::database(dbName, false );
            qDebug() << "Opening:" << dbName;

            if (db.isValid() && !db.isOpen()) {
                if (!db.open()) {
                    qWarning("tst_Databases: Unable to open %s on %s:\n%s", qPrintable(db.driverName()),
                             qPrintable(dbName), qPrintable(db.lastError().databaseText()));
                    // well... opening failed, so we just ignore the server, maybe it is not running
                    it = dbNames.erase(it);
                } else {
                    ++it;
                }
            }
        }
        return true;
    }

    void close()
    {
        for (const auto &dbName : std::as_const(dbNames)) {
            {
                QSqlDatabase db = QSqlDatabase::database(dbName, false);
                if (db.isValid() && db.isOpen())
                    db.close();
            }
            QSqlDatabase::removeDatabase(dbName);
        }
        dbNames.clear();
    }

    // for debugging only: outputs the connection as string
    static QString dbToString(const QSqlDatabase &db)
    {
        QString res = db.driverName() + QLatin1Char('@');

        if (db.driverName().startsWith("QODBC") || db.driverName().startsWith("QOCI"))
            res += db.databaseName();
        else
            res += db.hostName();

        if (db.port() > 0)
            res += QLatin1Char(':') + QString::number(db.port());

        return res;
    }

    // drop a table only if it exists to prevent warnings
    static void safeDropTables(const QSqlDatabase &db, const QStringList &tableNames)
    {
        QSqlQuery q(db);
        QStringList dbtables = db.tables();
        QSqlDriver::DbmsType dbType = getDatabaseType(db);
        for (const QString &tableName : tableNames)
        {
            bool wasDropped = true;
            QString table = tableName;
            if (db.driver()->isIdentifierEscaped(table, QSqlDriver::TableName))
                table = db.driver()->stripDelimiters(table, QSqlDriver::TableName);

            if (dbtables.contains(table, Qt::CaseInsensitive)) {
                for (const QString &table2 : dbtables.filter(table, Qt::CaseInsensitive)) {
                    if (table2.compare(table.section('.', -1, -1), Qt::CaseInsensitive) == 0) {
                        table = db.driver()->escapeIdentifier(table2, QSqlDriver::TableName);
                        if (dbType == QSqlDriver::PostgreSQL || dbType == QSqlDriver::MimerSQL)
                            wasDropped = q.exec( "drop table " + table + " cascade");
                        else
                            wasDropped = q.exec( "drop table " + table);
                        dbtables.removeAll(table2);
                    }
                }
            }
            if (!wasDropped) {
                qWarning() << dbToString(db) << "unable to drop table" << tableName << ':' << q.lastError();
//              qWarning() << "last query:" << q.lastQuery();
//              qWarning() << "dbtables:" << dbtables;
//              qWarning() << "db.tables():" << db.tables();
            }
        }
    }

    static void safeDropViews(const QSqlDatabase &db, const QStringList &viewNames)
    {
        if (isMSAccess(db)) // Access is sooo stupid.
            safeDropTables(db, viewNames);

        QSqlQuery q(db);
        QStringList dbtables = db.tables(QSql::Views);
        for (const QString &viewName : viewNames)
        {
            bool wasDropped = true;
            QString view = viewName;
            if (db.driver()->isIdentifierEscaped(view, QSqlDriver::TableName))
                view = db.driver()->stripDelimiters(view, QSqlDriver::TableName);

            if (dbtables.contains(view, Qt::CaseInsensitive))  {
                for (const QString &view2 : dbtables.filter(view, Qt::CaseInsensitive)) {
                    if (view2.compare(view.section('.', -1, -1), Qt::CaseInsensitive) == 0) {
                        view = db.driver()->escapeIdentifier(view2, QSqlDriver::TableName);
                        wasDropped = q.exec("drop view " + view);
                        dbtables.removeAll(view);
                    }
                }
            }

            if (!wasDropped)
                qWarning() << dbToString(db) << "unable to drop view" << viewName << ':' << q.lastError();
//                  << "\nlast query:" << q.lastQuery()
//                  << "\ndbtables:" << dbtables
//                  << "\ndb.tables(QSql::Views):" << db.tables(QSql::Views);
        }
    }

    // returns the type name of the blob datatype for the database db.
    // blobSize is only used if the db doesn't have a generic blob type
    static QString blobTypeName(const QSqlDatabase &db, int blobSize = 10000)
    {
        const QSqlDriver::DbmsType dbType = getDatabaseType(db);
        if (dbType == QSqlDriver::MySqlServer)
            return "longblob";

        if (dbType == QSqlDriver::PostgreSQL)
            return "bytea";

        if (dbType == QSqlDriver::Sybase
                || dbType == QSqlDriver::MSSqlServer
                || isMSAccess( db ) )
            return "image";

        if (dbType == QSqlDriver::DB2)
            return QString( "blob(%1)" ).arg( blobSize );

        if (dbType == QSqlDriver::Interbase)
            return QString( "blob sub_type 0 segment size 4096" );

        if (dbType == QSqlDriver::Oracle
                || dbType == QSqlDriver::SQLite)
            return "blob";

        qDebug() <<  "tst_Databases::blobTypeName: Don't know the blob type for" << dbToString( db );

        return "blob";
    }

    static QString dateTimeTypeName(const QSqlDatabase &db)
    {
        const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
        if (dbType == QSqlDriver::PostgreSQL)
            return QLatin1String("timestamptz");
        if (dbType == QSqlDriver::Oracle && getOraVersion(db) >= 9)
            return QLatin1String("timestamp(0)");
        if (dbType == QSqlDriver::Interbase || dbType == QSqlDriver::MimerSQL)
            return QLatin1String("timestamp");
        if (dbType == QSqlDriver::MSSqlServer)
            return QLatin1String("Datetime2");
        return QLatin1String("datetime");
    }

    static QString timeTypeName(const QSqlDatabase &db)
    {
        const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
        if (dbType == QSqlDriver::Oracle && getOraVersion(db) >= 9)
            return QLatin1String("timestamp(0)");
        return QLatin1String("time");
    }

    static QString dateTypeName(const QSqlDatabase &db)
    {
        const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
        if (dbType == QSqlDriver::Oracle && getOraVersion(db) >= 9)
            return QLatin1String("timestamp(0)");
        return QLatin1String("date");
    }

    static QString autoFieldName(const QSqlDatabase &db)
    {
        const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
        if (dbType == QSqlDriver::MySqlServer)
            return "AUTO_INCREMENT";
        if (dbType == QSqlDriver::Sybase || dbType == QSqlDriver::MSSqlServer)
            return "IDENTITY";
/*        if (dbType == QSqlDriver::PostgreSQL)
            return "SERIAL";*/
//        if (dbType == QSqlDriver::DB2)
//            return "GENERATED BY DEFAULT AS IDENTITY";

        return QString();
    }

    static QByteArray printError(const QSqlError &err)
    {
        QString result;
        if (!err.nativeErrorCode().isEmpty())
            result += u'(' + err.nativeErrorCode() + ") ";
        result += u'\'';
        if (!err.driverText().isEmpty())
            result += err.driverText() + "' || '";
        result += err.databaseText() + u'\'';
        return result.toLocal8Bit();
    }

    static QByteArray printError(const QSqlError &err, const QSqlDatabase &db)
    {
        return dbToString(db).toLocal8Bit() + ": " + printError(err);
    }

    static QSqlDriver::DbmsType getDatabaseType(const QSqlDatabase &db)
    {
        return db.driver()->dbmsType();
    }

    static bool isMSAccess(const QSqlDatabase &db)
    {
        return db.databaseName().contains( "Access Driver", Qt::CaseInsensitive );
    }

    // -1 on fail, else Oracle version
    static int getOraVersion(const QSqlDatabase &db)
    {
        int ver = -1;
        QSqlQuery q( "SELECT banner FROM v$version", db );
        q.next();

        QRegularExpression vers("([0-9]+)\\.[0-9\\.]+[0-9]");
        QRegularExpressionMatch match = vers.match(q.value(0).toString());
        if (match.hasMatch()) {
            bool ok;
            ver = match.captured(1).toInt(&ok);

            if (!ok)
                ver = -1;
        }

        return ver;
    }

    static QVersionNumber getIbaseEngineVersion(const QSqlDatabase &db)
    {
        QSqlQuery q(db);
        q.exec("SELECT rdb$get_context('SYSTEM', 'ENGINE_VERSION') as version from rdb$database;"_L1);
        q.next();
        auto record = q.record();
        auto version = QVersionNumber::fromString(record.value(0).toString());
        return version;
    }

    QStringList     dbNames;
    int      counter = 0;

private:
    QTemporaryDir *dbDir()
    {
        if (m_dbDir.isNull()) {
            m_dbDir.reset(new QTemporaryDir);
            if (!m_dbDir->isValid()) {
                qWarning() << Q_FUNC_INFO << "Unable to create a temporary directory: " << QDir::toNativeSeparators(m_dbDir->path());
                m_dbDir.reset();
            }
        }
        return m_dbDir.data();
    }

    QScopedPointer<QTemporaryDir> m_dbDir;
};

class TableScope
{
public:
    TableScope(const QSqlDatabase &db, const QString &fullTableName)
        : m_db(db)
        , m_tableName(fullTableName)
    {
        tst_Databases::safeDropTables(m_db, {m_tableName});
    }
    TableScope(const QSqlDatabase &db, const char *tableName, const char *file, bool escape = true)
        : TableScope(db, qTableName(tableName, file, db, escape))
    {
    }

    ~TableScope()
    {
        tst_Databases::safeDropTables(m_db, {m_tableName});
    }

    QString tableName() const
    {
        return m_tableName;
    }
private:
    QSqlDatabase m_db;
    QString m_tableName;
};

class ProcScope
{
public:
    ProcScope(const QSqlDatabase &db, const char *procName, const char *file)
        : m_db(db),
          m_procName(qTableName(procName, file, db))
    {
        cleanup();
    }
    ~ProcScope()
    {
        cleanup();
    }
    QString name() const
    {
        return m_procName;
    }
protected:
    void cleanup()
    {
        QSqlQuery q(m_db);
        if (m_db.driverName() == "QIBASE")
            q.exec("DROP PROCEDURE " + m_procName);
        else
            q.exec("DROP PROCEDURE IF EXISTS " + m_procName);
    }
private:
    QSqlDatabase m_db;
    const QString m_procName;
};

#endif

