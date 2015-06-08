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
/* possible connection parameters */

#ifndef TST_DATABASES_H
#define TST_DATABASES_H

#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QRegExp>
#include <QDir>
#include <QScopedPointer>
#include <QVariant>
#include <QDebug>
#include <QSqlTableModel>
#include <QtSql/private/qsqldriver_p.h>
#include <QtTest/QtTest>

#if defined(Q_OS_WIN)
#  include <qt_windows.h>
#  if defined(Q_OS_WINCE) || defined(Q_OS_WINRT)
#    include <winsock2.h>
#  endif
#else
#include <unistd.h>
#endif
#if defined(Q_OS_WINRT)
   static inline int qgethostname(char *name, int) { qstrcpy(name, "localhost"); return 9; }
#else
#  define qgethostname gethostname
#endif

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

    if ( !hostname.isEmpty() )
        return hostname;

    char hn[257];

    if ( qgethostname( hn, 255 ) == 0 ) {
        hn[256] = '\0';
        hostname = QString::fromLatin1( hn );
        hostname.replace( QLatin1Char( '.' ), QLatin1Char( '_' ) );
        hostname.replace( QLatin1Char( '-' ), QLatin1Char( '_' ) );
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

inline static QString qTableName(const QString& prefix, const char *sourceFileName, QSqlDatabase db)
{
    QString tableStr = QLatin1String("dbtst");
    if (db.driverName().toLower().contains("ODBC"))
        tableStr += QLatin1String("_odbc");
    return fixupTableName(QString(QLatin1String("dbtst") + db.driverName() +
                          QString::number(qHash(QLatin1String(sourceFileName) +
                          "_" + qGetHostName().replace( "-", "_" )), 16) + "_" + prefix), db);
}

inline static QString qTableName(const QString& prefix, QSqlDatabase db)
{
    QString tableStr;
    if (db.driverName().toLower().contains("ODBC"))
        tableStr += QLatin1String("_odbc");
    return fixupTableName(QString(db.driver()->escapeIdentifier(prefix + tableStr + "_" +
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
        QString cName = QString::number( counter++ ) + "_" + driver + "@";

        cName += host.isEmpty() ? dbName : host;

        if ( port > 0 )
            cName += ":" + QString::number( port );

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
        //addDb("QOCI", "localhost", "system", "penandy");
//         addDb( "QOCI8", "//horsehead.qt-project.org:1521/pony.troll.no", "scott", "tiger" ); // Oracle 9i on horsehead
//         addDb( "QOCI8", "//horsehead.qt-project.org:1521/ustest.troll.no", "scott", "tiger", "" ); // Oracle 9i on horsehead
//         addDb( "QOCI8", "//iceblink.qt-project.org:1521/ice.troll.no", "scott", "tiger", "" ); // Oracle 8 on iceblink (not currently working)
//         addDb( "QOCI", "//silence.qt-project.org:1521/testdb", "scott", "tiger" ); // Oracle 10g on silence
//         addDb( "QOCI", "//bq-oracle10g.qt-project.org:1521/XE", "scott", "tiger" ); // Oracle 10gexpress

//      This requires a local ODBC data source to be configured( pointing to a MySql database )
//         addDb( "QODBC", "mysqlodbc", "troll", "trond" );
//         addDb( "QODBC", "SqlServer", "troll", "trond" );
//         addDb( "QTDS7", "testdb", "troll", "trondk", "horsehead" );
//         addDb( "QODBC", "silencetestdb", "troll", "trond", "silence" );
//         addDb( "QODBC", "horseheadtestdb", "troll", "trondk", "horsehead" );

//         addDb( "QMYSQL3", "testdb", "troll", "trond", "horsehead.qt-project.org" );
//         addDb( "QMYSQL3", "testdb", "troll", "trond", "horsehead.qt-project.org", 3307 );
//         addDb( "QMYSQL3", "testdb", "troll", "trond", "horsehead.qt-project.org", 3308, "CLIENT_COMPRESS=1" ); // MySQL 4.1.1
//         addDb( "QMYSQL3", "testdb", "troll", "trond", "horsehead.qt-project.org", 3309, "CLIENT_COMPRESS=1" ); // MySQL 5.0.18 Linux
//         addDb( "QMYSQL3", "testdb", "troll", "trond", "silence.qt-project.org" ); // MySQL 5.1.36 Windows

//         addDb( "QMYSQL3", "testdb", "testuser", "Ee4Gabf6_", "bq-mysql41.qt-project.org" ); // MySQL 4.1.22-2.el4  linux
//         addDb( "QMYSQL3", "testdb", "testuser", "Ee4Gabf6_", "bq-mysql50.qt-project.org" ); // MySQL 5.0.45-7.el5 linux
//         addDb( "QMYSQL3", "testdb", "testuser", "Ee4Gabf6_", "bq-mysql51.qt-project.org" ); // MySQL 5.1.36-6.7.2.i586 linux

//         addDb( "QPSQL7", "testdb", "troll", "trond", "horsehead.qt-project.org" ); // V7.2 NOT SUPPORTED!
//         addDb( "QPSQL7", "testdb", "troll", "trond", "horsehead.qt-project.org", 5434 ); // V7.2 NOT SUPPORTED! Multi-byte
//         addDb( "QPSQL7", "testdb", "troll", "trond", "horsehead.qt-project.org", 5435 ); // V7.3
//         addDb( "QPSQL7", "testdb", "troll", "trond", "horsehead.qt-project.org", 5436 ); // V7.4
//         addDb( "QPSQL7", "testdb", "troll", "trond", "horsehead.qt-project.org", 5437 ); // V8.0.3
//         addDb( "QPSQL7", "testdb", "troll", "trond", "silence.qt-project.org" );         // V8.2.1, UTF-8

//         addDb( "QPSQL7", "testdb", "testuser", "Ee4Gabf6_", "bq-postgres74.qt-project.org" );         // Version 7.4.19-1.el4_6.1
//         addDb( "QPSQL7", "testdb", "testuser", "Ee4Gabf6_", "bq-pgsql81.qt-project.org" );         // Version 8.1.11-1.el5_1.1
//         addDb( "QPSQL7", "testdb", "testuser", "Ee4Gabf6_", "bq-pgsql84.qt-project.org" );         // Version 8.4.1-2.1.i586
//         addDb( "QPSQL7", "testdb", "testuser", "Ee4Gabf6_", "bq-pgsql90.qt-project.org" );         // Version 9.0.0


//         addDb( "QDB2", "testdb", "troll", "trond", "silence.qt-project.org" ); // DB2 v9.1 on silence
//         addDb( "QDB2", "testdb", "testuser", "Ee4Gabf6_", "bq-db2-972.qt-project.org" ); // DB2

//      yes - interbase really wants the physical path on the host machine.
//         addDb( "QIBASE", "/opt/interbase/qttest.gdb", "SYSDBA", "masterkey", "horsehead.qt-project.org" );
//         addDb( "QIBASE", "silence.troll.no:c:\\ibase\\testdb", "SYSDBA", "masterkey", "" ); // InterBase 7.5 on silence
//         addDb( "QIBASE", "silence.troll.no:c:\\ibase\\testdb_ascii", "SYSDBA", "masterkey", "" ); // InterBase 7.5 on silence
//         addDb( "QIBASE", "/opt/firebird/databases/testdb.fdb", "testuser", "Ee4Gabf6_", "firebird1-nokia.trolltech.com.au" ); // Firebird 1.5.5
//         addDb( "QIBASE", "/opt/firebird/databases/testdb.fdb", "testuser", "Ee4Gabf6_", "firebird2-nokia.trolltech.com.au" ); // Firebird 2.1.1

//         addDb( "QIBASE", "/opt/firebird/databases/testdb.fdb", "testuser", "Ee4Gabf6_", "bq-firebird1.qt-project.org" ); // Firebird 1.5.5
//         addDb( "QIBASE", "/opt/firebird/databases/testdb.fdb", "testuser", "Ee4Gabf6_", "bq-firebird2.qt-project.org" ); // Firebird 2.1.1

//      use in-memory database to prevent local files
//         addDb("QSQLITE", ":memory:");
        QTemporaryDir *sqLiteDir = dbDir();
        if (!sqLiteDir)
            return false;
        addDb( QStringLiteral("QSQLITE"), QDir::toNativeSeparators(sqLiteDir->path() + QStringLiteral("/foo.db")) );
//         addDb( "QSQLITE2", QDir::toNativeSeparators(dbDir.path() + "/foo2.db") );
//         addDb( "QODBC3", "DRIVER={SQL SERVER};SERVER=iceblink.qt-project.org\\ICEBLINK", "troll", "trond", "" );
//         addDb( "QODBC3", "DRIVER={SQL Native Client};SERVER=silence.qt-project.org\\SQLEXPRESS", "troll", "trond", "" );

//         addDb( "QODBC", "DRIVER={MySQL ODBC 5.1 Driver};SERVER=bq-mysql50.qt-project.org;DATABASE=testdb", "testuser", "Ee4Gabf6_", "" );
//         addDb( "QODBC", "DRIVER={MySQL ODBC 5.1 Driver};SERVER=bq-mysql51.qt-project.org;DATABASE=testdb", "testuser", "Ee4Gabf6_", "" );
//         addDb( "QODBC", "DRIVER={FreeTDS};SERVER=horsehead.qt-project.org;DATABASE=testdb;PORT=4101;UID=troll;PWD=trondk", "troll", "trondk", "" );
//         addDb( "QODBC", "DRIVER={FreeTDS};SERVER=silence.qt-project.org;DATABASE=testdb;PORT=2392;UID=troll;PWD=trond", "troll", "trond", "" );
//         addDb( "QODBC", "DRIVER={FreeTDS};SERVER=bq-winserv2003-x86-01.qt-project.org;DATABASE=testdb;PORT=1433;UID=testuser;PWD=Ee4Gabf6_;TDS_Version=8.0", "", "", "" );
//         addDb( "QODBC", "DRIVER={FreeTDS};SERVER=bq-winserv2008-x86-01.qt-project.org;DATABASE=testdb;PORT=1433;UID=testuser;PWD=Ee4Gabf6_;TDS_Version=8.0", "", "", "" );
//         addDb( "QTDS7", "testdb", "testuser", "Ee4Gabf6_", "bq-winserv2003" );
//         addDb( "QTDS7", "testdb", "testuser", "Ee4Gabf6_", "bq-winserv2008" );
//         addDb( "QODBC3", "DRIVER={SQL SERVER};SERVER=bq-winserv2003-x86-01.qt-project.org;DATABASE=testdb;PORT=1433", "testuser", "Ee4Gabf6_", "" );
//         addDb( "QODBC3", "DRIVER={SQL SERVER};SERVER=bq-winserv2008-x86-01.qt-project.org;DATABASE=testdb;PORT=1433", "testuser", "Ee4Gabf6_", "" );
//         addDb( "QODBC", "DRIVER={Microsoft Access Driver (*.mdb)};DBQ=c:\\dbs\\access\\testdb.mdb", "", "", "" );
//         addDb( "QODBC", "DRIVER={Postgresql};SERVER=bq-pgsql84.qt-project.org;DATABASE=testdb", "testuser", "Ee4Gabf6_", "" );
         return true;
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
        QString res = db.driverName() + "@";

        if ( db.driverName().startsWith( "QODBC" ) || db.driverName().startsWith( "QOCI" ) ) {
            res += db.databaseName();
        } else {
            res += db.hostName();
        }

        if ( db.port() > 0 ) {
            res += ":" + QString::number( db.port() );
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
        result += err.databaseText() + "'";
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
        result += err.databaseText() + "'";
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

        QRegExp vers( "([0-9]+)\\.[0-9\\.]+[0-9]" );

        if ( vers.indexIn( q.value( 0 ).toString() ) ) {
            bool ok;
            ver = vers.cap( 1 ).toInt( &ok );

            if ( !ok )
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

