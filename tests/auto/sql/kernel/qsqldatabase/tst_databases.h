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
/* possible connection parameters */

#ifndef TST_DATABASES_H
#define TST_DATABASES_H

#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDir>
#include <QScopedPointer>
#include <QVariant>
#include <QDebug>
#include <QSqlTableModel>
#include <QtSql/private/qsqldriver_p.h>
#include <QtTest/QtTest>

#define CHECK_DATABASE( db ) \
    if ( !db.isValid() ) { qFatal( "db is Invalid" ); }

#define QVERIFY_SQL(q, stmt) QVERIFY2((q).stmt, tst_Databases::printError((q).lastError(), db))
#define QFAIL_SQL(q, stmt) QVERIFY2(!(q).stmt, tst_Databases::printError((q).lastError(), db))

#define DBMS_SPECIFIC(db, driver) \
    if (!db.driverName().startsWith(driver)) { QSKIP(driver " specific test"); }

// ### use QSystem::hostName if it is integrated in qtest/main
static QString qGetHostName()
{
    static QString hostname;

    if (hostname.isEmpty()) {
        hostname = QSysInfo::machineHostName();
        hostname.replace(QLatin1Char( '.' ), QLatin1Char( '_' ));
        hostname.replace(QLatin1Char( '-' ), QLatin1Char( '_' ));
    }

    return hostname;
}

// to prevent nameclashes on our database server, each machine
// will use its own set of table names. Call this function to get
// "tablename_hostname"
inline QString fixupTableName(const QString &tableName, QSqlDatabase db)
{
    QString tbName = tableName;
    // On Oracle we are limited to 30 character tablenames
    QSqlDriverPrivate *d = static_cast<QSqlDriverPrivate *>(QObjectPrivate::get(db.driver()));
    if (d && d->dbmsType == QSqlDriver::Oracle)
        tbName.truncate(30);
    return tbName;
}

inline static QString qTableName(const QString &prefix, const char *sourceFileName,
                                 QSqlDatabase db, bool escape = true)
{
    const auto tableStr = fixupTableName(QString(QLatin1String("dbtst") + db.driverName() +
                                                 QString::number(qHash(QLatin1String(sourceFileName) +
                                                 "_" + qGetHostName().replace("-", "_")), 16) +
                                                 "_" + prefix), db);
    return escape ? db.driver()->escapeIdentifier(tableStr, QSqlDriver::TableName) : tableStr;
}

inline static QString qTableName(const QString& prefix, QSqlDatabase db)
{
    QString tableStr;
    if (db.driverName().toLower().contains("ODBC"))
        tableStr += QLatin1String("_odbc");
    return fixupTableName(QString(db.driver()->escapeIdentifier(prefix + tableStr + QLatin1Char('_') +
                          qGetHostName(), QSqlDriver::TableName)),db);
}

inline static bool testWhiteSpaceNames( const QString &name )
{
/*    return name.startsWith( "QPSQL" )
           || name.startsWith( "QODBC" )
           || name.startsWith( "QSQLITE" )
           || name.startsWith( "QMYSQL" );*/
    return name != QLatin1String("QSQLITE2");
}

inline static QString toHex( const QString& binary )
{
    QString str;
    static char const hexchars[] = "0123456789ABCDEF";

    for ( int i = 0; i < binary.size(); i++ ) {
        ushort code = binary.at(i).unicode();
        str += (QChar)(hexchars[ (code >> 12) & 0x0F ]);
        str += (QChar)(hexchars[ (code >> 8) & 0x0F ]);
        str += (QChar)(hexchars[ (code >> 4) & 0x0F ]);
        str += (QChar)(hexchars[ code & 0x0F ]);
    }

    return str;
}


class tst_Databases
{

public:
    tst_Databases(): counter( 0 )
    {
    }

    ~tst_Databases()
    {
        close();
    }

    // returns a testtable consisting of the names of all database connections if
    // driverPrefix is empty, otherwise only those that start with driverPrefix.
    int fillTestTable( const QString& driverPrefix = QString() ) const
    {
        QTest::addColumn<QString>( "dbName" );
        int count = 0;

        for ( int i = 0; i < dbNames.count(); ++i ) {
            QSqlDatabase db = QSqlDatabase::database( dbNames.at( i ) );

            if ( !db.isValid() )
                continue;

            if ( driverPrefix.isEmpty() || db.driverName().startsWith( driverPrefix ) ) {
                QTest::newRow( dbNames.at( i ).toLatin1() ) << dbNames.at( i );
                ++count;
            }
        }

        return count;
    }

    int fillTestTableWithStrategies( const QString& driverPrefix = QString() ) const
    {
        QTest::addColumn<QString>( "dbName" );
        QTest::addColumn<int>("submitpolicy_i");
        int count = 0;

        for ( int i = 0; i < dbNames.count(); ++i ) {
            QSqlDatabase db = QSqlDatabase::database( dbNames.at( i ) );

            if ( !db.isValid() )
                continue;

            if ( driverPrefix.isEmpty() || db.driverName().startsWith( driverPrefix ) ) {
                QTest::newRow( QString("%1 [field]").arg(dbNames.at( i )).toLatin1() ) << dbNames.at( i ) << (int)QSqlTableModel::OnFieldChange;
                QTest::newRow( QString("%1 [row]").arg(dbNames.at( i )).toLatin1() ) << dbNames.at( i ) << (int)QSqlTableModel::OnRowChange;
                QTest::newRow( QString("%1 [manual]").arg(dbNames.at( i )).toLatin1() ) << dbNames.at( i ) << (int)QSqlTableModel::OnManualSubmit;
                ++count;
            }
        }

        return count;
    }

    void addDb( const QString& driver, const QString& dbName,
                const QString& user = QString(), const QString& passwd = QString(),
                const QString& host = QString(), int port = -1, const QString params = QString() )
    {
        QSqlDatabase db;

        if ( !QSqlDatabase::drivers().contains( driver ) ) {
            qWarning() <<  "Driver" << driver << "is not installed";
            return;
        }

        // construct a stupid unique name
        QString cName = QString::number( counter++ ) + QLatin1Char('_') + driver + QLatin1Char('@');

        cName += host.isEmpty() ? dbName : host;

        if ( port > 0 )
            cName += QLatin1Char(':') + QString::number( port );

        db = QSqlDatabase::addDatabase( driver, cName );

        if ( !db.isValid() ) {
            qWarning( "Could not create database object" );
            return;
        }

        db.setDatabaseName( dbName );

        db.setUserName( user );
        db.setPassword( passwd );
        db.setHostName( host );
        db.setPort( port );
        db.setConnectOptions( params );
        dbNames.append( cName );
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
                QJsonArray::const_iterator it = entriesA.constBegin();
                while (it != entriesA.constEnd()) {
                    if ((*it).isObject()) {
                        const QJsonObject object = (*it).toObject();
                        addDb(object.value(QStringLiteral("driver")).toString(),
                              object.value(QStringLiteral("name")).toString(),
                              object.value(QStringLiteral("username")).toString(),
                              object.value(QStringLiteral("password")).toString(),
                              object.value(QStringLiteral("hostname")).toString(),
                              object.value(QStringLiteral("port")).toInt(),
                              object.value(QStringLiteral("parameters")).toString());
                        added = true;
                    }
                    ++it;
                }
            }
        }
        QTemporaryDir *sqLiteDir = dbDir();
        if (sqLiteDir) {
            addDb(QStringLiteral("QSQLITE"), QDir::toNativeSeparators(sqLiteDir->path() + QStringLiteral("/foo.db")));
            added = true;
        }
        return added;
    }

    // 'false' return indicates a system error, for example failure to create a temporary directory.
    bool open()
    {
        if (!addDbs())
            return false;

        QStringList::Iterator it = dbNames.begin();

        while ( it != dbNames.end() ) {
            QSqlDatabase db = QSqlDatabase::database(( *it ), false );
            qDebug() << "Opening:" << (*it);

            if ( db.isValid() && !db.isOpen() ) {
                if ( !db.open() ) {
                    qWarning( "tst_Databases: Unable to open %s on %s:\n%s", qPrintable( db.driverName() ), qPrintable( *it ), qPrintable( db.lastError().databaseText() ) );
                    // well... opening failed, so we just ignore the server, maybe it is not running
                    it = dbNames.erase( it );
                } else {
                    ++it;
                }
            }
        }
        return true;
    }

    void close()
    {
        for ( QStringList::Iterator it = dbNames.begin(); it != dbNames.end(); ++it ) {
            {
                QSqlDatabase db = QSqlDatabase::database(( *it ), false );

                if ( db.isValid() && db.isOpen() )
                    db.close();
            }

            QSqlDatabase::removeDatabase(( *it ) );
        }

        dbNames.clear();
    }

    // for debugging only: outputs the connection as string
    static QString dbToString( const QSqlDatabase db )
    {
        QString res = db.driverName() + QLatin1Char('@');

        if ( db.driverName().startsWith( "QODBC" ) || db.driverName().startsWith( "QOCI" ) ) {
            res += db.databaseName();
        } else {
            res += db.hostName();
        }

        if ( db.port() > 0 ) {
            res += QLatin1Char(':') + QString::number( db.port() );
        }

        return res;
    }

    // drop a table only if it exists to prevent warnings
    static void safeDropTables( QSqlDatabase db, const QStringList& tableNames )
    {
        bool wasDropped;
        QSqlQuery q( db );
        QStringList dbtables=db.tables();
        QSqlDriver::DbmsType dbType = getDatabaseType(db);
        foreach(const QString &tableName, tableNames)
        {
            wasDropped = true;
            QString table=tableName;
            if ( db.driver()->isIdentifierEscaped(table, QSqlDriver::TableName))
                table = db.driver()->stripDelimiters(table, QSqlDriver::TableName);

            if ( dbtables.contains( table, Qt::CaseInsensitive ) ) {
                foreach(const QString &table2, dbtables.filter(table, Qt::CaseInsensitive)) {
                    if(table2.compare(table.section('.', -1, -1), Qt::CaseInsensitive) == 0) {
                        table=db.driver()->escapeIdentifier(table2, QSqlDriver::TableName);
                        if (dbType == QSqlDriver::PostgreSQL)
                            wasDropped = q.exec( "drop table " + table + " cascade");
                        else
                            wasDropped = q.exec( "drop table " + table);
                        dbtables.removeAll(table2);
                    }
                }
            }
            if ( !wasDropped ) {
                qWarning() << dbToString(db) << "unable to drop table" << tableName << ':' << q.lastError();
//              qWarning() << "last query:" << q.lastQuery();
//              qWarning() << "dbtables:" << dbtables;
//              qWarning() << "db.tables():" << db.tables();
            }
        }
    }

    static void safeDropTable( QSqlDatabase db, const QString& tableName )
    {
        safeDropTables(db, QStringList() << tableName);
    }

    static void safeDropViews( QSqlDatabase db, const QStringList &viewNames )
    {
        if ( isMSAccess( db ) ) // Access is sooo stupid.
            safeDropTables( db, viewNames );

        bool wasDropped;
        QSqlQuery q( db );
        QStringList dbtables=db.tables(QSql::Views);

        foreach(QString viewName, viewNames)
        {
            wasDropped = true;
            QString view=viewName;
            if ( db.driver()->isIdentifierEscaped(view, QSqlDriver::TableName))
                view = db.driver()->stripDelimiters(view, QSqlDriver::TableName);

            if ( dbtables.contains( view, Qt::CaseInsensitive ) ) {
                foreach(const QString &view2, dbtables.filter(view, Qt::CaseInsensitive)) {
                    if(view2.compare(view.section('.', -1, -1), Qt::CaseInsensitive) == 0) {
                        view=db.driver()->escapeIdentifier(view2, QSqlDriver::TableName);
                        wasDropped = q.exec( "drop view " + view);
                        dbtables.removeAll(view);
                    }
                }
            }

            if ( !wasDropped )
                qWarning() << dbToString(db) << "unable to drop view" << viewName << ':' << q.lastError();
//                  << "\nlast query:" << q.lastQuery()
//                  << "\ndbtables:" << dbtables
//                  << "\ndb.tables(QSql::Views):" << db.tables(QSql::Views);
        }
    }

    static void safeDropView( QSqlDatabase db, const QString& tableName )
    {
        safeDropViews(db, QStringList() << tableName);
    }

    // returns the type name of the blob datatype for the database db.
    // blobSize is only used if the db doesn't have a generic blob type
    static QString blobTypeName( QSqlDatabase db, int blobSize = 10000 )
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

    static QString dateTimeTypeName(QSqlDatabase db)
    {
        const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
        if (dbType == QSqlDriver::PostgreSQL)
            return QLatin1String("timestamptz");
        if (dbType == QSqlDriver::Oracle && getOraVersion(db) >= 9)
            return QLatin1String("timestamp(0)");
        return QLatin1String("datetime");
    }

    static QString autoFieldName( QSqlDatabase db )
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

    static QByteArray printError( const QSqlError& err )
    {
        QString result;
        if (!err.nativeErrorCode().isEmpty())
            result += '(' + err.nativeErrorCode() + ") ";
        result += '\'';
        if(!err.driverText().isEmpty())
            result += err.driverText() + "' || '";
        result += err.databaseText() + QLatin1Char('\'');
        return result.toLocal8Bit();
    }

    static QByteArray printError( const QSqlError& err, const QSqlDatabase& db )
    {
        QString result(dbToString(db) + ": ");
        if (!err.nativeErrorCode().isEmpty())
            result += '(' + err.nativeErrorCode() + ") ";
        result += '\'';
        if(!err.driverText().isEmpty())
            result += err.driverText() + "' || '";
        result += err.databaseText() + QLatin1Char('\'');
        return result.toLocal8Bit();
    }

    static QSqlDriver::DbmsType getDatabaseType(QSqlDatabase db)
    {
        QSqlDriverPrivate *d = static_cast<QSqlDriverPrivate *>(QObjectPrivate::get(db.driver()));
        return d->dbmsType;
    }

    static bool isMSAccess( QSqlDatabase db )
    {
        return db.databaseName().contains( "Access Driver", Qt::CaseInsensitive );
    }

    // -1 on fail, else Oracle version
    static int getOraVersion( QSqlDatabase db )
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

    static QString getMySqlVersion( const QSqlDatabase &db )
    {
        QSqlQuery q(db);
        q.exec( "select version()" );
        if(q.next())
            return q.value( 0 ).toString();
        else
            return QString();
    }

    static QString getPSQLVersion( const QSqlDatabase &db )
    {
        QSqlQuery q(db);
        q.exec( "select version()" );
        if(q.next())
            return q.value( 0 ).toString();
        else
            return QString();
    }

    QStringList     dbNames;
    int      counter;

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

#endif

