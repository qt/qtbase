// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsql_psql_p.h"

#include <qcoreapplication.h>
#include <qvariant.h>
#include <qdatetime.h>
#include <qloggingcategory.h>
#include <qregularexpression.h>
#include <qsqlerror.h>
#include <qsqlfield.h>
#include <qsqlindex.h>
#include <qsqlrecord.h>
#include <qsqlquery.h>
#include <qsocketnotifier.h>
#include <qstringlist.h>
#include <qlocale.h>
#include <QtSql/private/qsqlresult_p.h>
#include <QtSql/private/qsqldriver_p.h>
#include <QtCore/private/qlocale_tools_p.h>

#include <queue>

#include <libpq-fe.h>
#include <pg_config.h>

#include <cmath>

// workaround for postgres defining their OIDs in a private header file
#define QBOOLOID 16
#define QINT8OID 20
#define QINT2OID 21
#define QINT4OID 23
#define QNUMERICOID 1700
#define QFLOAT4OID 700
#define QFLOAT8OID 701
#define QABSTIMEOID 702
#define QRELTIMEOID 703
#define QDATEOID 1082
#define QTIMEOID 1083
#define QTIMETZOID 1266
#define QTIMESTAMPOID 1114
#define QTIMESTAMPTZOID 1184
#define QOIDOID 2278
#define QBYTEAOID 17
#define QREGPROCOID 24
#define QXIDOID 28
#define QCIDOID 29

#define QBITOID 1560
#define QVARBITOID 1562

#define VARHDRSZ 4

/* This is a compile time switch - if PQfreemem is declared, the compiler will use that one,
   otherwise it'll run in this template */
template <typename T>
inline void PQfreemem(T *t, int = 0) { free(t); }

Q_DECLARE_OPAQUE_POINTER(PGconn*)
Q_DECLARE_METATYPE(PGconn*)

Q_DECLARE_OPAQUE_POINTER(PGresult*)
Q_DECLARE_METATYPE(PGresult*)

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcPsql, "qt.sql.postgresql")

using namespace Qt::StringLiterals;

inline void qPQfreemem(void *buffer)
{
    PQfreemem(buffer);
}

/* Missing declaration of PGRES_SINGLE_TUPLE for PSQL below 9.2 */
#if !defined PG_VERSION_NUM || PG_VERSION_NUM-0 < 90200
static constexpr int PGRES_SINGLE_TUPLE = 9;
#endif

typedef int StatementId;
static constexpr StatementId InvalidStatementId = 0;

class QPSQLResultPrivate;

class QPSQLResult final : public QSqlResult
{
    Q_DECLARE_PRIVATE(QPSQLResult)

public:
    QPSQLResult(const QPSQLDriver *db);
    ~QPSQLResult();

    QVariant handle() const override;
    void virtual_hook(int id, void *data) override;

protected:
    void cleanup();
    bool fetch(int i) override;
    bool fetchFirst() override;
    bool fetchLast() override;
    bool fetchNext() override;
    bool nextResult() override;
    QVariant data(int i) override;
    bool isNull(int field) override;
    bool reset(const QString &query) override;
    int size() override;
    int numRowsAffected() override;
    QSqlRecord record() const override;
    QVariant lastInsertId() const override;
    bool prepare(const QString &query) override;
    bool exec() override;
};

class QPSQLDriverPrivate final : public QSqlDriverPrivate
{
    Q_DECLARE_PUBLIC(QPSQLDriver)
public:
    QPSQLDriverPrivate() : QSqlDriverPrivate(QSqlDriver::PostgreSQL) {}

    QStringList seid;
    PGconn *connection = nullptr;
    QSocketNotifier *sn = nullptr;
    QPSQLDriver::Protocol pro = QPSQLDriver::Version6;
    StatementId currentStmtId = InvalidStatementId;
    StatementId stmtCount = InvalidStatementId;
    mutable bool pendingNotifyCheck = false;
    bool hasBackslashEscape = false;

    void appendTables(QStringList &tl, QSqlQuery &t, QChar type);
    PGresult *exec(const char *stmt);
    PGresult *exec(const QString &stmt);
    StatementId sendQuery(const QString &stmt);
    bool setSingleRowMode() const;
    PGresult *getResult(StatementId stmtId) const;
    void finishQuery(StatementId stmtId);
    void discardResults() const;
    StatementId generateStatementId();
    void checkPendingNotifications() const;
    QPSQLDriver::Protocol getPSQLVersion();
    bool setEncodingUtf8();
    void setDatestyle();
    void setByteaOutput();
    void setUtcTimeZone();
    void detectBackslashEscape();
    mutable QHash<int, QString> oidToTable;
};

void QPSQLDriverPrivate::appendTables(QStringList &tl, QSqlQuery &t, QChar type)
{
    const QString query =
            QStringLiteral("SELECT pg_class.relname, pg_namespace.nspname FROM pg_class "
                           "LEFT JOIN pg_namespace ON (pg_class.relnamespace = pg_namespace.oid) "
                           "WHERE (pg_class.relkind = '") + type +
            QStringLiteral("') AND (pg_class.relname !~ '^Inv') "
                           "AND (pg_class.relname !~ '^pg_') "
                           "AND (pg_namespace.nspname != 'information_schema')");
    t.exec(query);
    while (t.next()) {
        QString schema = t.value(1).toString();
        if (schema.isEmpty() || schema == "public"_L1)
            tl.append(t.value(0).toString());
        else
            tl.append(t.value(0).toString().prepend(u'.').prepend(schema));
    }
}

PGresult *QPSQLDriverPrivate::exec(const char *stmt)
{
    // PQexec() silently discards any prior query results that the application didn't eat.
    PGresult *result = PQexec(connection, stmt);
    currentStmtId = result ? generateStatementId() : InvalidStatementId;
    checkPendingNotifications();
    return result;
}

PGresult *QPSQLDriverPrivate::exec(const QString &stmt)
{
    return exec(stmt.toUtf8().constData());
}

StatementId QPSQLDriverPrivate::sendQuery(const QString &stmt)
{
    // Discard any prior query results that the application didn't eat.
    // This is required for PQsendQuery()
    discardResults();
    const int result = PQsendQuery(connection, stmt.toUtf8().constData());
    currentStmtId = result ? generateStatementId() : InvalidStatementId;
    return currentStmtId;
}

bool QPSQLDriverPrivate::setSingleRowMode() const
{
    // Activates single-row mode for last sent query, see:
    // https://www.postgresql.org/docs/9.2/static/libpq-single-row-mode.html
    // This method should be called immediately after the sendQuery() call.
#if defined PG_VERSION_NUM && PG_VERSION_NUM-0 >= 90200
    return PQsetSingleRowMode(connection) > 0;
#else
    return false;
#endif
}

PGresult *QPSQLDriverPrivate::getResult(StatementId stmtId) const
{
    // Make sure the results of stmtId weren't discarded. This might
    // happen for forward-only queries if somebody executed another
    // SQL query on the same db connection.
    if (stmtId != currentStmtId) {
        // If you change the following warning, remember to update it
        // on sql-driver.html page too.
        qCWarning(lcPsql, "QPSQLDriver::getResult: Query results lost - "
                          "probably discarded on executing another SQL query.");
        return nullptr;
    }
    PGresult *result = PQgetResult(connection);
    checkPendingNotifications();
    return result;
}

void QPSQLDriverPrivate::finishQuery(StatementId stmtId)
{
    if (stmtId != InvalidStatementId && stmtId == currentStmtId) {
        discardResults();
        currentStmtId = InvalidStatementId;
    }
}

void QPSQLDriverPrivate::discardResults() const
{
    while (PGresult *result = PQgetResult(connection))
        PQclear(result);
}

StatementId QPSQLDriverPrivate::generateStatementId()
{
    StatementId stmtId = ++stmtCount;
    if (stmtId <= 0)
        stmtId = stmtCount = 1;
    return stmtId;
}

void QPSQLDriverPrivate::checkPendingNotifications() const
{
    Q_Q(const QPSQLDriver);
    if (seid.size() && !pendingNotifyCheck) {
        pendingNotifyCheck = true;
        QMetaObject::invokeMethod(const_cast<QPSQLDriver*>(q), &QPSQLDriver::_q_handleNotification, Qt::QueuedConnection);
    }
}

class QPSQLResultPrivate final : public QSqlResultPrivate
{
    Q_DECLARE_PUBLIC(QPSQLResult)
public:
    Q_DECLARE_SQLDRIVER_PRIVATE(QPSQLDriver)
    using QSqlResultPrivate::QSqlResultPrivate;

    QString fieldSerial(qsizetype i) const override { return QString("$%1"_L1).arg(i + 1); }
    void deallocatePreparedStmt();

    std::queue<PGresult*> nextResultSets;
    QString preparedStmtId;
    PGresult *result = nullptr;
    StatementId stmtId = InvalidStatementId;
    int currentSize = -1;
    bool canFetchMoreRows = false;
    bool preparedQueriesEnabled = false;

    bool processResults();
};

static QSqlError qMakeError(const QString &err, QSqlError::ErrorType type,
                            const QPSQLDriverPrivate *p, PGresult *result = nullptr)
{
    const char *s = PQerrorMessage(p->connection);
    QString msg = QString::fromUtf8(s);
    QString errorCode;
    if (result) {
        errorCode = QString::fromLatin1(PQresultErrorField(result, PG_DIAG_SQLSTATE));
        msg += QString::fromLatin1("(%1)").arg(errorCode);
    }
    return QSqlError("QPSQL: "_L1 + err, msg, type, errorCode);
}

bool QPSQLResultPrivate::processResults()
{
    Q_Q(QPSQLResult);
    if (!result) {
        q->setSelect(false);
        q->setActive(false);
        currentSize = -1;
        canFetchMoreRows = false;
        if (stmtId != drv_d_func()->currentStmtId) {
            q->setLastError(qMakeError(QCoreApplication::translate("QPSQLResult",
                            "Query results lost - probably discarded on executing "
                            "another SQL query."), QSqlError::StatementError, drv_d_func(), result));
        }
        return false;
    }
    int status = PQresultStatus(result);
    switch (status) {
    case PGRES_TUPLES_OK:
        q->setSelect(true);
        q->setActive(true);
        currentSize = q->isForwardOnly() ? -1 : PQntuples(result);
        canFetchMoreRows = false;
        return true;
    case PGRES_SINGLE_TUPLE:
        q->setSelect(true);
        q->setActive(true);
        currentSize = -1;
        canFetchMoreRows = true;
        return true;
    case PGRES_COMMAND_OK:
        q->setSelect(false);
        q->setActive(true);
        currentSize = -1;
        canFetchMoreRows = false;
        return true;
    default:
        break;
    }
    q->setSelect(false);
    q->setActive(false);
    currentSize = -1;
    canFetchMoreRows = false;
    q->setLastError(qMakeError(QCoreApplication::translate("QPSQLResult",
                    "Unable to create query"), QSqlError::StatementError, drv_d_func(), result));
    return false;
}

static QMetaType qDecodePSQLType(int t)
{
    int type = QMetaType::UnknownType;
    switch (t) {
    case QBOOLOID:
        type = QMetaType::Bool;
        break;
    case QINT8OID:
        type = QMetaType::LongLong;
        break;
    case QINT2OID:
    case QINT4OID:
    case QOIDOID:
    case QREGPROCOID:
    case QXIDOID:
    case QCIDOID:
        type = QMetaType::Int;
        break;
    case QNUMERICOID:
    case QFLOAT4OID:
    case QFLOAT8OID:
        type = QMetaType::Double;
        break;
    case QABSTIMEOID:
    case QRELTIMEOID:
    case QDATEOID:
        type = QMetaType::QDate;
        break;
    case QTIMEOID:
    case QTIMETZOID:
        type = QMetaType::QTime;
        break;
    case QTIMESTAMPOID:
    case QTIMESTAMPTZOID:
        type = QMetaType::QDateTime;
        break;
    case QBYTEAOID:
        type = QMetaType::QByteArray;
        break;
    default:
        type = QMetaType::QString;
        break;
    }
    return QMetaType(type);
}

void QPSQLResultPrivate::deallocatePreparedStmt()
{
    if (drv_d_func()) {
        const QString stmt = QStringLiteral("DEALLOCATE ") + preparedStmtId;
        PGresult *result = drv_d_func()->exec(stmt);

        if (PQresultStatus(result) != PGRES_COMMAND_OK) {
            const QString msg = QString::fromUtf8(PQerrorMessage(drv_d_func()->connection));
            qCWarning(lcPsql, "Unable to free statement: %ls.", qUtf16Printable(msg));
        }
        PQclear(result);
    }
    preparedStmtId.clear();
}

QPSQLResult::QPSQLResult(const QPSQLDriver *db)
    : QSqlResult(*new QPSQLResultPrivate(this, db))
{
    Q_D(QPSQLResult);
    d->preparedQueriesEnabled = db->hasFeature(QSqlDriver::PreparedQueries);
}

QPSQLResult::~QPSQLResult()
{
    Q_D(QPSQLResult);
    cleanup();

    if (d->preparedQueriesEnabled && !d->preparedStmtId.isNull())
        d->deallocatePreparedStmt();
}

QVariant QPSQLResult::handle() const
{
    Q_D(const QPSQLResult);
    return QVariant::fromValue(d->result);
}

void QPSQLResult::cleanup()
{
    Q_D(QPSQLResult);
    if (d->result)
        PQclear(d->result);
    d->result = nullptr;
    while (!d->nextResultSets.empty()) {
        PQclear(d->nextResultSets.front());
        d->nextResultSets.pop();
    }
    if (d->stmtId != InvalidStatementId) {
        if (d->drv_d_func())
            d->drv_d_func()->finishQuery(d->stmtId);
    }
    d->stmtId = InvalidStatementId;
    setAt(QSql::BeforeFirstRow);
    d->currentSize = -1;
    d->canFetchMoreRows = false;
    setActive(false);
}

bool QPSQLResult::fetch(int i)
{
    Q_D(const QPSQLResult);
    if (!isActive())
        return false;
    if (i < 0)
        return false;
    if (at() == i)
        return true;

    if (isForwardOnly()) {
        if (i < at())
            return false;
        bool ok = true;
        while (ok && i > at())
            ok = fetchNext();
        return ok;
    }

    if (i >= d->currentSize)
        return false;
    setAt(i);
    return true;
}

bool QPSQLResult::fetchFirst()
{
    Q_D(const QPSQLResult);
    if (!isActive())
        return false;
    if (at() == 0)
        return true;

    if (isForwardOnly()) {
        if (at() == QSql::BeforeFirstRow) {
            // First result has been already fetched by exec() or
            // nextResult(), just check it has at least one row.
            if (d->result && PQntuples(d->result) > 0) {
                setAt(0);
                return true;
            }
        }
        return false;
    }

    return fetch(0);
}

bool QPSQLResult::fetchLast()
{
    Q_D(const QPSQLResult);
    if (!isActive())
        return false;

    if (isForwardOnly()) {
        // Cannot seek to last row in forwardOnly mode, so we have to use brute force
        int i = at();
        if (i == QSql::AfterLastRow)
            return false;
        if (i == QSql::BeforeFirstRow)
            i = 0;
        while (fetchNext())
            ++i;
        setAt(i);
        return true;
    }

    return fetch(d->currentSize - 1);
}

bool QPSQLResult::fetchNext()
{
    Q_D(QPSQLResult);
    if (!isActive())
        return false;

    const int currentRow = at();  // Small optimalization
    if (currentRow == QSql::BeforeFirstRow)
        return fetchFirst();
    if (currentRow == QSql::AfterLastRow)
        return false;

    if (isForwardOnly()) {
        if (!d->canFetchMoreRows)
            return false;
        PQclear(d->result);
        d->result = d->drv_d_func()->getResult(d->stmtId);
        if (!d->result) {
            setLastError(qMakeError(QCoreApplication::translate("QPSQLResult",
                                    "Unable to get result"), QSqlError::StatementError, d->drv_d_func(), d->result));
            d->canFetchMoreRows = false;
            return false;
        }
        int status = PQresultStatus(d->result);
        switch (status) {
        case PGRES_SINGLE_TUPLE:
            // Fetched next row of current result set
            Q_ASSERT(PQntuples(d->result) == 1);
            Q_ASSERT(d->canFetchMoreRows);
            setAt(currentRow + 1);
            return true;
        case PGRES_TUPLES_OK:
            // In single-row mode PGRES_TUPLES_OK means end of current result set
            Q_ASSERT(PQntuples(d->result) == 0);
            d->canFetchMoreRows = false;
            return false;
        default:
            setLastError(qMakeError(QCoreApplication::translate("QPSQLResult",
                                    "Unable to get result"), QSqlError::StatementError, d->drv_d_func(), d->result));
            d->canFetchMoreRows = false;
            return false;
        }
    }

    if (currentRow + 1 >= d->currentSize)
        return false;
    setAt(currentRow + 1);
    return true;
}

bool QPSQLResult::nextResult()
{
    Q_D(QPSQLResult);
    if (!isActive())
        return false;

    setAt(QSql::BeforeFirstRow);

    if (isForwardOnly()) {
        if (d->canFetchMoreRows) {
            // Skip all rows from current result set
            while (d->result && PQresultStatus(d->result) == PGRES_SINGLE_TUPLE) {
                PQclear(d->result);
                d->result = d->drv_d_func()->getResult(d->stmtId);
            }
            d->canFetchMoreRows = false;
            // Check for unexpected errors
            if (d->result && PQresultStatus(d->result) == PGRES_FATAL_ERROR)
                return d->processResults();
        }
        // Fetch first result from next result set
        if (d->result)
            PQclear(d->result);
        d->result = d->drv_d_func()->getResult(d->stmtId);
        return d->processResults();
    }

    if (d->result)
        PQclear(d->result);
    d->result = nullptr;
    if (!d->nextResultSets.empty()) {
        d->result = d->nextResultSets.front();
        d->nextResultSets.pop();
    }
    return d->processResults();
}

QVariant QPSQLResult::data(int i)
{
    Q_D(const QPSQLResult);
    if (i >= PQnfields(d->result)) {
        qCWarning(lcPsql, "QPSQLResult::data: column %d out of range.", i);
        return QVariant();
    }
    const int currentRow = isForwardOnly() ? 0 : at();
    int ptype = PQftype(d->result, i);
    QMetaType type = qDecodePSQLType(ptype);
    if (PQgetisnull(d->result, currentRow, i))
        return QVariant(type, nullptr);
    const char *val = PQgetvalue(d->result, currentRow, i);
    switch (type.id()) {
    case QMetaType::Bool:
        return QVariant((bool)(val[0] == 't'));
    case QMetaType::QString:
        return QString::fromUtf8(val);
    case QMetaType::LongLong:
        if (val[0] == '-')
            return QByteArray::fromRawData(val, qstrlen(val)).toLongLong();
        else
            return QByteArray::fromRawData(val, qstrlen(val)).toULongLong();
    case QMetaType::Int:
        return atoi(val);
    case QMetaType::Double: {
        if (ptype == QNUMERICOID) {
            if (numericalPrecisionPolicy() == QSql::HighPrecision)
                return QString::fromLatin1(val);
        }
        bool ok;
        double dbl = qstrtod(val, nullptr, &ok);
        if (!ok) {
            if (qstricmp(val, "NaN") == 0)
                dbl = qQNaN();
            else if (qstricmp(val, "Infinity") == 0)
                dbl = qInf();
            else if (qstricmp(val, "-Infinity") == 0)
                dbl = -qInf();
            else
                return QVariant();
        }
        if (ptype == QNUMERICOID) {
            if (numericalPrecisionPolicy() == QSql::LowPrecisionInt64)
                return QVariant((qlonglong)dbl);
            else if (numericalPrecisionPolicy() == QSql::LowPrecisionInt32)
                return QVariant((int)dbl);
            else if (numericalPrecisionPolicy() == QSql::LowPrecisionDouble)
                return QVariant(dbl);
        }
        return dbl;
    }
#if QT_CONFIG(datestring)
    case QMetaType::QDate:
        return QVariant(QDate::fromString(QString::fromLatin1(val), Qt::ISODate));
    case QMetaType::QTime:
        return QVariant(QTime::fromString(QString::fromLatin1(val), Qt::ISODate));
    case QMetaType::QDateTime: {
        const QLatin1StringView tzString(val);
        const auto timeString(tzString.sliced(11));
        if (timeString.contains(u'-') || timeString.contains(u'+') || timeString.endsWith(u'Z'))
            return QDateTime::fromString(tzString, Qt::ISODate);
        const auto utc = tzString.toString() + u'Z';
        return QVariant(QDateTime::fromString(utc, Qt::ISODate));
    }
#else
    case QMetaType::QDate:
    case QMetaType::QTime:
    case QMetaType::QDateTime:
        return QVariant(QString::fromLatin1(val));
#endif
    case QMetaType::QByteArray: {
        size_t len;
        unsigned char *data = PQunescapeBytea(reinterpret_cast<const unsigned char *>(val), &len);
        QByteArray ba(reinterpret_cast<const char *>(data), len);
        qPQfreemem(data);
        return QVariant(ba);
    }
    default:
        qCWarning(lcPsql, "QPSQLResult::data: unhandled data type %d.", type.id());
    }
    return QVariant();
}

bool QPSQLResult::isNull(int field)
{
    Q_D(const QPSQLResult);
    const int currentRow = isForwardOnly() ? 0 : at();
    return PQgetisnull(d->result, currentRow, field);
}

bool QPSQLResult::reset(const QString &query)
{
    Q_D(QPSQLResult);
    cleanup();
    if (!driver())
        return false;
    if (!driver()->isOpen() || driver()->isOpenError())
        return false;

    d->stmtId = d->drv_d_func()->sendQuery(query);
    if (d->stmtId == InvalidStatementId) {
        setLastError(qMakeError(QCoreApplication::translate("QPSQLResult",
                                "Unable to send query"), QSqlError::StatementError, d->drv_d_func()));
        return false;
    }

    if (isForwardOnly())
        setForwardOnly(d->drv_d_func()->setSingleRowMode());

    d->result = d->drv_d_func()->getResult(d->stmtId);
    if (!isForwardOnly()) {
        // Fetch all result sets right away
        while (PGresult *nextResultSet = d->drv_d_func()->getResult(d->stmtId))
            d->nextResultSets.push(nextResultSet);
    }
    return d->processResults();
}

int QPSQLResult::size()
{
    Q_D(const QPSQLResult);
    return d->currentSize;
}

int QPSQLResult::numRowsAffected()
{
    Q_D(const QPSQLResult);
    const char *tuples = PQcmdTuples(d->result);
    return QByteArray::fromRawData(tuples, qstrlen(tuples)).toInt();
}

QVariant QPSQLResult::lastInsertId() const
{
    Q_D(const QPSQLResult);
    if (d->drv_d_func()->pro >= QPSQLDriver::Version8_1) {
        QSqlQuery qry(driver()->createResult());
        // Most recent sequence value obtained from nextval
        if (qry.exec(QStringLiteral("SELECT lastval();")) && qry.next())
            return qry.value(0);
    } else if (isActive()) {
        Oid id = PQoidValue(d->result);
        if (id != InvalidOid)
            return QVariant(id);
    }
    return QVariant();
}

QSqlRecord QPSQLResult::record() const
{
    Q_D(const QPSQLResult);
    QSqlRecord info;
    if (!isActive() || !isSelect())
        return info;

    int count = PQnfields(d->result);
    QSqlField f;
    for (int i = 0; i < count; ++i) {
        f.setName(QString::fromUtf8(PQfname(d->result, i)));
        const int tableOid = PQftable(d->result, i);
        // WARNING: We cannot execute any other SQL queries on
        // the same db connection while forward-only mode is active
        // (this would discard all results of forward-only query).
        // So we just skip this...
        if (tableOid != InvalidOid && !isForwardOnly()) {
            auto &tableName = d->drv_d_func()->oidToTable[tableOid];
            if (tableName.isEmpty()) {
                QSqlQuery qry(driver()->createResult());
                if (qry.exec(QStringLiteral("SELECT relname FROM pg_class WHERE pg_class.oid = %1")
                            .arg(tableOid)) && qry.next()) {
                    tableName = qry.value(0).toString();
                }
            }
            f.setTableName(tableName);
        } else {
            f.setTableName(QString());
        }
        int ptype = PQftype(d->result, i);
        f.setMetaType(qDecodePSQLType(ptype));
        f.setValue(QVariant(f.metaType())); // only set in setType() when it's invalid before
        int len = PQfsize(d->result, i);
        int precision = PQfmod(d->result, i);

        switch (ptype) {
        case QTIMESTAMPOID:
        case QTIMESTAMPTZOID:
            precision = 3;
            break;

        case QNUMERICOID:
            if (precision != -1) {
                len = (precision >> 16);
                precision = ((precision - VARHDRSZ) & 0xffff);
            }
            break;
        case QBITOID:
        case QVARBITOID:
            len = precision;
            precision = -1;
            break;
        default:
            if (len == -1 && precision >= VARHDRSZ) {
                len = precision - VARHDRSZ;
                precision = -1;
            }
        }

        f.setLength(len);
        f.setPrecision(precision);
        info.append(f);
    }
    return info;
}

void QPSQLResult::virtual_hook(int id, void *data)
{
    Q_ASSERT(data);
    QSqlResult::virtual_hook(id, data);
}

static QString qCreateParamString(const QList<QVariant> &boundValues, const QSqlDriver *driver)
{
    if (boundValues.isEmpty())
        return QString();

    QString params;
    QSqlField f;
    for (const QVariant &val : boundValues) {
        f.setMetaType(val.metaType());
        if (QSqlResultPrivate::isVariantNull(val))
            f.clear();
        else
            f.setValue(val);
        if (!params.isNull())
            params.append(", "_L1);
        params.append(driver->formatValue(f));
    }
    return params;
}

QString qMakePreparedStmtId()
{
    Q_CONSTINIT static QBasicAtomicInt qPreparedStmtCount = Q_BASIC_ATOMIC_INITIALIZER(0);
    QString id = QStringLiteral("qpsqlpstmt_") + QString::number(qPreparedStmtCount.fetchAndAddRelaxed(1) + 1, 16);
    return id;
}

bool QPSQLResult::prepare(const QString &query)
{
    Q_D(QPSQLResult);
    if (!d->preparedQueriesEnabled)
        return QSqlResult::prepare(query);

    cleanup();

    if (!d->preparedStmtId.isEmpty())
        d->deallocatePreparedStmt();

    const QString stmtId = qMakePreparedStmtId();
    const QString stmt = QStringLiteral("PREPARE %1 AS ").arg(stmtId).append(d->positionalToNamedBinding(query));

    PGresult *result = d->drv_d_func()->exec(stmt);

    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        setLastError(qMakeError(QCoreApplication::translate("QPSQLResult",
                                "Unable to prepare statement"), QSqlError::StatementError, d->drv_d_func(), result));
        PQclear(result);
        d->preparedStmtId.clear();
        return false;
    }

    PQclear(result);
    d->preparedStmtId = stmtId;
    return true;
}

bool QPSQLResult::exec()
{
    Q_D(QPSQLResult);
    if (!d->preparedQueriesEnabled)
        return QSqlResult::exec();

    cleanup();

    QString stmt;
    const QString params = qCreateParamString(boundValues(), driver());
    if (params.isEmpty())
        stmt = QStringLiteral("EXECUTE %1").arg(d->preparedStmtId);
    else
        stmt = QStringLiteral("EXECUTE %1 (%2)").arg(d->preparedStmtId, params);

    d->stmtId = d->drv_d_func()->sendQuery(stmt);
    if (d->stmtId == InvalidStatementId) {
        setLastError(qMakeError(QCoreApplication::translate("QPSQLResult",
                                "Unable to send query"), QSqlError::StatementError, d->drv_d_func()));
        return false;
    }

    if (isForwardOnly())
        setForwardOnly(d->drv_d_func()->setSingleRowMode());

    d->result = d->drv_d_func()->getResult(d->stmtId);
    if (!isForwardOnly()) {
        // Fetch all result sets right away
        while (PGresult *nextResultSet = d->drv_d_func()->getResult(d->stmtId))
            d->nextResultSets.push(nextResultSet);
    }
    return d->processResults();
}

///////////////////////////////////////////////////////////////////

bool QPSQLDriverPrivate::setEncodingUtf8()
{
    PGresult *result = exec("SET CLIENT_ENCODING TO 'UNICODE'");
    int status = PQresultStatus(result);
    PQclear(result);
    return status == PGRES_COMMAND_OK;
}

void QPSQLDriverPrivate::setDatestyle()
{
    PGresult *result = exec("SET DATESTYLE TO 'ISO'");
    int status =  PQresultStatus(result);
    if (status != PGRES_COMMAND_OK)
        qCWarning(lcPsql) << QString::fromUtf8(PQerrorMessage(connection));
    PQclear(result);
}

void QPSQLDriverPrivate::setByteaOutput()
{
    if (pro >= QPSQLDriver::Version9) {
        // Server version before QPSQLDriver::Version9 only supports escape mode for bytea type,
        // but bytea format is set to hex by default in PSQL 9 and above. So need to force the
        // server to use the old escape mode when connects to the new server.
        PGresult *result = exec("SET bytea_output TO escape");
        int status = PQresultStatus(result);
        if (status != PGRES_COMMAND_OK)
            qCWarning(lcPsql) << QString::fromUtf8(PQerrorMessage(connection));
        PQclear(result);
    }
}

void QPSQLDriverPrivate::setUtcTimeZone()
{
    PGresult *result = exec("SET TIME ZONE 'UTC'");
    int status = PQresultStatus(result);
    if (status != PGRES_COMMAND_OK)
        qCWarning(lcPsql) << QString::fromUtf8(PQerrorMessage(connection));
    PQclear(result);
}

void QPSQLDriverPrivate::detectBackslashEscape()
{
    // standard_conforming_strings option introduced in 8.2
    // http://www.postgresql.org/docs/8.2/static/runtime-config-compatible.html
    if (pro < QPSQLDriver::Version8_2) {
        hasBackslashEscape = true;
    } else {
        hasBackslashEscape = false;
        PGresult *result = exec(QStringLiteral("SELECT '\\\\' x"));
        int status = PQresultStatus(result);
        if (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK)
            if (QString::fromLatin1(PQgetvalue(result, 0, 0)) == "\\"_L1)
                hasBackslashEscape = true;
        PQclear(result);
    }
}

static QPSQLDriver::Protocol qMakePSQLVersion(int vMaj, int vMin)
{
    switch (vMaj) {
    case 6:
        return QPSQLDriver::Version6;
    case 7:
    {
        switch (vMin) {
        case 1:
            return QPSQLDriver::Version7_1;
        case 3:
            return QPSQLDriver::Version7_3;
        case 4:
            return QPSQLDriver::Version7_4;
        default:
            return QPSQLDriver::Version7;
        }
        break;
    }
    case 8:
    {
        switch (vMin) {
        case 1:
            return QPSQLDriver::Version8_1;
        case 2:
            return QPSQLDriver::Version8_2;
        case 3:
            return QPSQLDriver::Version8_3;
        case 4:
            return QPSQLDriver::Version8_4;
        default:
            return QPSQLDriver::Version8;
        }
        break;
    }
    case 9:
    {
        switch (vMin) {
        case 1:
            return QPSQLDriver::Version9_1;
        case 2:
            return QPSQLDriver::Version9_2;
        case 3:
            return QPSQLDriver::Version9_3;
        case 4:
            return QPSQLDriver::Version9_4;
        case 5:
            return QPSQLDriver::Version9_5;
        case 6:
            return QPSQLDriver::Version9_6;
        default:
            return QPSQLDriver::Version9;
        }
        break;
    }
    case 10:
        return QPSQLDriver::Version10;
    case 11:
        return QPSQLDriver::Version11;
    case 12:
        return QPSQLDriver::Version12;
    default:
        if (vMaj > 12)
            return QPSQLDriver::UnknownLaterVersion;
        break;
    }
    return QPSQLDriver::VersionUnknown;
}

static QPSQLDriver::Protocol qFindPSQLVersion(const QString &versionString)
{
    const QRegularExpression rx(QStringLiteral("(\\d+)(?:\\.(\\d+))?"));
    const QRegularExpressionMatch match = rx.match(versionString);
    if (match.hasMatch()) {
        // Beginning with PostgreSQL version 10, a major release is indicated by
        // increasing the first part of the version, e.g. 10 to 11.
        // Before version 10, a major release was indicated by increasing either
        // the first or second part of the version number, e.g. 9.5 to 9.6.
        int vMaj = match.capturedView(1).toInt();
        int vMin;
        if (vMaj >= 10) {
            vMin = 0;
        } else {
            if (match.capturedView(2).isEmpty())
                return QPSQLDriver::VersionUnknown;
            vMin = match.capturedView(2).toInt();
        }
        return qMakePSQLVersion(vMaj, vMin);
    }

    return QPSQLDriver::VersionUnknown;
}

QPSQLDriver::Protocol QPSQLDriverPrivate::getPSQLVersion()
{
    QPSQLDriver::Protocol serverVersion = QPSQLDriver::Version6;
    PGresult *result = exec("SELECT version()");
    int status = PQresultStatus(result);
    if (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK) {
        serverVersion = qFindPSQLVersion(
            QString::fromLatin1(PQgetvalue(result, 0, 0)));
    }
    PQclear(result);

    QPSQLDriver::Protocol clientVersion =
#if defined(PG_MAJORVERSION)
        qFindPSQLVersion(PG_MAJORVERSION ""_L1);
#elif defined(PG_VERSION)
        qFindPSQLVersion(PG_VERSION ""_L1);
#else
        QPSQLDriver::VersionUnknown;
#endif

    if (serverVersion == QPSQLDriver::VersionUnknown) {
        serverVersion = clientVersion;
        if (serverVersion != QPSQLDriver::VersionUnknown)
            qCWarning(lcPsql,  "The server version of this PostgreSQL is unknown, "
                               "falling back to the client version.");
    }

    // Keep the old behavior unchanged
    if (serverVersion == QPSQLDriver::VersionUnknown)
        serverVersion = QPSQLDriver::Version6;

    if (serverVersion < QPSQLDriver::Version7_3)
        qCWarning(lcPsql, "This version of PostgreSQL is not supported and may not work.");

    return serverVersion;
}

QPSQLDriver::QPSQLDriver(QObject *parent)
    : QSqlDriver(*new QPSQLDriverPrivate, parent)
{
}

QPSQLDriver::QPSQLDriver(PGconn *conn, QObject *parent)
    : QSqlDriver(*new QPSQLDriverPrivate, parent)
{
    Q_D(QPSQLDriver);
    d->connection = conn;
    if (conn) {
        d->pro = d->getPSQLVersion();
        d->detectBackslashEscape();
        setOpen(true);
        setOpenError(false);
    }
}

QPSQLDriver::~QPSQLDriver()
{
    Q_D(QPSQLDriver);
    PQfinish(d->connection);
}

QVariant QPSQLDriver::handle() const
{
    Q_D(const QPSQLDriver);
    return QVariant::fromValue(d->connection);
}

bool QPSQLDriver::hasFeature(DriverFeature f) const
{
    Q_D(const QPSQLDriver);
    switch (f) {
    case Transactions:
    case QuerySize:
    case LastInsertId:
    case LowPrecisionNumbers:
    case EventNotifications:
    case MultipleResultSets:
    case BLOB:
    case Unicode:
        return true;
    case PreparedQueries:
    case PositionalPlaceholders:
        return d->pro >= QPSQLDriver::Version8_2;
    case BatchOperations:
    case NamedPlaceholders:
    case SimpleLocking:
    case FinishQuery:
    case CancelQuery:
        return false;
    }
    return false;
}

/*
   Quote a string for inclusion into the connection string
   \ -> \\
   ' -> \'
   surround string by single quotes
 */
static QString qQuote(QString s)
{
    s.replace(u'\\', "\\\\"_L1);
    s.replace(u'\'', "\\'"_L1);
    s.append(u'\'').prepend(u'\'');
    return s;
}

bool QPSQLDriver::open(const QString &db,
                       const QString &user,
                       const QString &password,
                       const QString &host,
                       int port,
                       const QString &connOpts)
{
    Q_D(QPSQLDriver);
    close();
    QString connectString;
    if (!host.isEmpty())
        connectString.append("host="_L1).append(qQuote(host));
    if (!db.isEmpty())
        connectString.append(" dbname="_L1).append(qQuote(db));
    if (!user.isEmpty())
        connectString.append(" user="_L1).append(qQuote(user));
    if (!password.isEmpty())
        connectString.append(" password="_L1).append(qQuote(password));
    if (port != -1)
        connectString.append(" port="_L1).append(qQuote(QString::number(port)));

    // add any connect options - the server will handle error detection
    if (!connOpts.isEmpty()) {
        QString opt = connOpts;
        opt.replace(';'_L1, ' '_L1, Qt::CaseInsensitive);
        connectString.append(u' ').append(opt);
    }

    d->connection = PQconnectdb(std::move(connectString).toLocal8Bit().constData());
    if (PQstatus(d->connection) == CONNECTION_BAD) {
        setLastError(qMakeError(tr("Unable to connect"), QSqlError::ConnectionError, d));
        setOpenError(true);
        PQfinish(d->connection);
        d->connection = nullptr;
        return false;
    }

    d->pro = d->getPSQLVersion();
    d->detectBackslashEscape();
    if (!d->setEncodingUtf8()) {
        setLastError(qMakeError(tr("Unable to set client encoding to 'UNICODE'"), QSqlError::ConnectionError, d));
        setOpenError(true);
        PQfinish(d->connection);
        d->connection = nullptr;
        return false;
    }
    d->setDatestyle();
    d->setByteaOutput();
    d->setUtcTimeZone();

    setOpen(true);
    setOpenError(false);
    return true;
}

void QPSQLDriver::close()
{
    Q_D(QPSQLDriver);

    d->seid.clear();
    if (d->sn) {
        disconnect(d->sn, &QSocketNotifier::activated, this, &QPSQLDriver::_q_handleNotification);
        delete d->sn;
        d->sn = nullptr;
    }

    PQfinish(d->connection);
    d->connection = nullptr;
    setOpen(false);
    setOpenError(false);
}

QSqlResult *QPSQLDriver::createResult() const
{
    return new QPSQLResult(this);
}

bool QPSQLDriver::beginTransaction()
{
    Q_D(QPSQLDriver);
    if (!isOpen()) {
        qCWarning(lcPsql, "QPSQLDriver::beginTransaction: Database not open.");
        return false;
    }
    PGresult *res = d->exec("BEGIN");
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
        setLastError(qMakeError(tr("Could not begin transaction"),
                                QSqlError::TransactionError, d, res));
        PQclear(res);
        return false;
    }
    PQclear(res);
    return true;
}

bool QPSQLDriver::commitTransaction()
{
    Q_D(QPSQLDriver);
    if (!isOpen()) {
        qCWarning(lcPsql, "QPSQLDriver::commitTransaction: Database not open.");
        return false;
    }
    PGresult *res = d->exec("COMMIT");

    bool transaction_failed = false;

    // XXX
    // This hack is used to tell if the transaction has succeeded for the protocol versions of
    // PostgreSQL below. For 7.x and other protocol versions we are left in the dark.
    // This hack can disappear once there is an API to query this sort of information.
    if (d->pro >= QPSQLDriver::Version8) {
        transaction_failed = qstrcmp(PQcmdStatus(res), "ROLLBACK") == 0;
    }

    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK || transaction_failed) {
        setLastError(qMakeError(tr("Could not commit transaction"),
                                QSqlError::TransactionError, d, res));
        PQclear(res);
        return false;
    }
    PQclear(res);
    return true;
}

bool QPSQLDriver::rollbackTransaction()
{
    Q_D(QPSQLDriver);
    if (!isOpen()) {
        qCWarning(lcPsql, "QPSQLDriver::rollbackTransaction: Database not open.");
        return false;
    }
    PGresult *res = d->exec("ROLLBACK");
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
        setLastError(qMakeError(tr("Could not rollback transaction"),
                                QSqlError::TransactionError, d, res));
        PQclear(res);
        return false;
    }
    PQclear(res);
    return true;
}

QStringList QPSQLDriver::tables(QSql::TableType type) const
{
    Q_D(const QPSQLDriver);
    QStringList tl;
    if (!isOpen())
        return tl;
    QSqlQuery t(createResult());
    t.setForwardOnly(true);

    if (type & QSql::Tables)
        const_cast<QPSQLDriverPrivate*>(d)->appendTables(tl, t, u'r');
    if (type & QSql::Views)
        const_cast<QPSQLDriverPrivate*>(d)->appendTables(tl, t, u'v');
    if (type & QSql::SystemTables) {
        t.exec(QStringLiteral("SELECT relname FROM pg_class WHERE (relkind = 'r') "
                              "AND (relname LIKE 'pg_%') "));
        while (t.next())
            tl.append(t.value(0).toString());
    }

    return tl;
}

static void qSplitTableName(QString &tablename, QString &schema)
{
    qsizetype dot = tablename.indexOf(u'.');
    if (dot == -1)
        return;
    schema = tablename.left(dot);
    tablename = tablename.mid(dot + 1);
}

QSqlIndex QPSQLDriver::primaryIndex(const QString &tablename) const
{
    QSqlIndex idx(tablename);
    if (!isOpen())
        return idx;
    QSqlQuery i(createResult());

    QString tbl = tablename;
    QString schema;
    qSplitTableName(tbl, schema);
    schema = stripDelimiters(schema, QSqlDriver::TableName);
    tbl = stripDelimiters(tbl, QSqlDriver::TableName);

    QString stmt = QStringLiteral("SELECT pg_attribute.attname, pg_attribute.atttypid::int, "
                                  "pg_class.relname "
                                  "FROM pg_attribute, pg_class "
                                  "WHERE %1 pg_class.oid IN "
                                  "(SELECT indexrelid FROM pg_index WHERE indisprimary = true AND indrelid IN "
                                  "(SELECT oid FROM pg_class WHERE relname = '%2')) "
                                  "AND pg_attribute.attrelid = pg_class.oid "
                                  "AND pg_attribute.attisdropped = false "
                                  "ORDER BY pg_attribute.attnum");
    if (schema.isEmpty())
        stmt = stmt.arg(QStringLiteral("pg_table_is_visible(pg_class.oid) AND"));
    else
        stmt = stmt.arg(QStringLiteral("pg_class.relnamespace = (SELECT oid FROM "
                                       "pg_namespace WHERE pg_namespace.nspname = '%1') AND").arg(schema));

    i.exec(stmt.arg(tbl));
    while (i.isActive() && i.next()) {
        QSqlField f(i.value(0).toString(), qDecodePSQLType(i.value(1).toInt()), tablename);
        idx.append(f);
        idx.setName(i.value(2).toString());
    }
    return idx;
}

QSqlRecord QPSQLDriver::record(const QString &tablename) const
{
    QSqlRecord info;
    if (!isOpen())
        return info;

    QString tbl = tablename;
    QString schema;
    qSplitTableName(tbl, schema);
    schema = stripDelimiters(schema, QSqlDriver::TableName);
    tbl = stripDelimiters(tbl, QSqlDriver::TableName);

    const QString adsrc = protocol() < Version8
        ? QStringLiteral("pg_attrdef.adsrc")
        : QStringLiteral("pg_get_expr(pg_attrdef.adbin, pg_attrdef.adrelid)");
    const QString nspname = schema.isEmpty()
        ? QStringLiteral("pg_table_is_visible(pg_class.oid)")
        : QStringLiteral("pg_class.relnamespace = (SELECT oid FROM "
                         "pg_namespace WHERE pg_namespace.nspname = '%1')").arg(schema);
    const QString stmt =
        QStringLiteral("SELECT pg_attribute.attname, pg_attribute.atttypid::int, "
                       "pg_attribute.attnotnull, pg_attribute.attlen, pg_attribute.atttypmod, "
                       "%1 "
                       "FROM pg_class, pg_attribute "
                       "LEFT JOIN pg_attrdef ON (pg_attrdef.adrelid = "
                       "pg_attribute.attrelid AND pg_attrdef.adnum = pg_attribute.attnum) "
                       "WHERE %2 "
                       "AND pg_class.relname = '%3' "
                       "AND pg_attribute.attnum > 0 "
                       "AND pg_attribute.attrelid = pg_class.oid "
                       "AND pg_attribute.attisdropped = false "
                       "ORDER BY pg_attribute.attnum").arg(adsrc, nspname, tbl);

    QSqlQuery query(createResult());
    query.exec(stmt);
    while (query.next()) {
        int len = query.value(3).toInt();
        int precision = query.value(4).toInt();
        // swap length and precision if length == -1
        if (len == -1 && precision > -1) {
            len = precision - 4;
            precision = -1;
        }
        QString defVal = query.value(5).toString();
        if (!defVal.isEmpty() && defVal.at(0) == u'\'') {
            const qsizetype end = defVal.lastIndexOf(u'\'');
            if (end > 0)
                defVal = defVal.mid(1, end - 1);
        }
        QSqlField f(query.value(0).toString(), qDecodePSQLType(query.value(1).toInt()), tablename);
        f.setRequired(query.value(2).toBool());
        f.setLength(len);
        f.setPrecision(precision);
        f.setDefaultValue(defVal);
        info.append(f);
    }

    return info;
}

template <class FloatType>
inline void assignSpecialPsqlFloatValue(FloatType val, QString *target)
{
    if (qIsNaN(val))
        *target = QStringLiteral("'NaN'");
    else if (qIsInf(val))
        *target = (val < 0) ? QStringLiteral("'-Infinity'") : QStringLiteral("'Infinity'");
}

QString QPSQLDriver::formatValue(const QSqlField &field, bool trimStrings) const
{
    Q_D(const QPSQLDriver);
    const auto nullStr = [](){ return QStringLiteral("NULL"); };
    QString r;
    if (field.isNull()) {
        r = nullStr();
    } else {
        switch (field.metaType().id()) {
        case QMetaType::QDateTime: {
            const auto dt = field.value().toDateTime();
            if (dt.isValid()) {
                // we force the value to be considered with a timezone information, and we force it to be UTC
                // this is safe since postgresql stores only the UTC value and not the timezone offset (only used
                // while parsing), so we have correct behavior in both case of with timezone and without tz
                r = QStringLiteral("TIMESTAMP WITH TIME ZONE ") + u'\'' +
                        dt.toOffsetFromUtc(dt.offsetFromUtc()).toString(Qt::ISODateWithMs) + u'\'';
            } else {
                r = nullStr();
            }
            break;
        }
        case QMetaType::QTime: {
            const auto t = field.value().toTime();
            if (t.isValid())
                r = u'\'' + QLocale::c().toString(t, u"hh:mm:ss.zzz") + u'\'';
            else
                r = nullStr();
            break;
        }
        case QMetaType::QString:
            r = QSqlDriver::formatValue(field, trimStrings);
            if (d->hasBackslashEscape)
                r.replace(u'\\', "\\\\"_L1);
            break;
        case QMetaType::Bool:
            if (field.value().toBool())
                r = QStringLiteral("TRUE");
            else
                r = QStringLiteral("FALSE");
            break;
        case QMetaType::QByteArray: {
            QByteArray ba(field.value().toByteArray());
            size_t len;
#if defined PG_VERSION_NUM && PG_VERSION_NUM-0 >= 80200
            unsigned char *data = PQescapeByteaConn(d->connection, (const unsigned char*)ba.constData(), ba.size(), &len);
#else
            unsigned char *data = PQescapeBytea((const unsigned char*)ba.constData(), ba.size(), &len);
#endif
            r += u'\'';
            r += QLatin1StringView((const char*)data);
            r += u'\'';
            qPQfreemem(data);
            break;
        }
        case QMetaType::Float:
            assignSpecialPsqlFloatValue(field.value().toFloat(), &r);
            if (r.isEmpty())
                r = QSqlDriver::formatValue(field, trimStrings);
            break;
        case QMetaType::Double:
            assignSpecialPsqlFloatValue(field.value().toDouble(), &r);
            if (r.isEmpty())
                r = QSqlDriver::formatValue(field, trimStrings);
            break;
        case QMetaType::QUuid:
            r = u'\'' + field.value().toString() + u'\'';
            break;
        default:
            r = QSqlDriver::formatValue(field, trimStrings);
            break;
        }
    }
    return r;
}

QString QPSQLDriver::escapeIdentifier(const QString &identifier, IdentifierType) const
{
    QString res = identifier;
    if (!identifier.isEmpty() && !identifier.startsWith(u'"') && !identifier.endsWith(u'"') ) {
        res.replace(u'"', "\"\""_L1);
        res.replace(u'.', "\".\""_L1);
        res = u'"' + res + u'"';
    }
    return res;
}

bool QPSQLDriver::isOpen() const
{
    Q_D(const QPSQLDriver);
    return PQstatus(d->connection) == CONNECTION_OK;
}

QPSQLDriver::Protocol QPSQLDriver::protocol() const
{
    Q_D(const QPSQLDriver);
    return d->pro;
}

bool QPSQLDriver::subscribeToNotification(const QString &name)
{
    Q_D(QPSQLDriver);
    if (!isOpen()) {
        qCWarning(lcPsql, "QPSQLDriver::subscribeToNotification: Database not open.");
        return false;
    }

    const bool alreadyContained = d->seid.contains(name);
    int socket = PQsocket(d->connection);
    if (socket) {
        // Add the name to the list of subscriptions here so that QSQLDriverPrivate::exec knows
        // to check for notifications immediately after executing the LISTEN. If it has already
        // been subscribed then LISTEN Will do nothing. But we do the call anyway in case the
        // connection was lost and this is a re-subscription.
        if (!alreadyContained)
            d->seid << name;
        QString query = QStringLiteral("LISTEN ") + escapeIdentifier(name, QSqlDriver::TableName);
        PGresult *result = d->exec(query);
        if (PQresultStatus(result) != PGRES_COMMAND_OK) {
            if (!alreadyContained)
                d->seid.removeLast();
            setLastError(qMakeError(tr("Unable to subscribe"), QSqlError::StatementError, d, result));
            PQclear(result);
            return false;
        }
        PQclear(result);

        if (!d->sn) {
            d->sn = new QSocketNotifier(socket, QSocketNotifier::Read, this);
            connect(d->sn, &QSocketNotifier::activated, this, &QPSQLDriver::_q_handleNotification);
        }
    } else {
        qCWarning(lcPsql, "QPSQLDriver::subscribeToNotificationImplementation: "
                          "PQsocket didn't return a valid socket to listen on.");
        return false;
    }

    return true;
}

bool QPSQLDriver::unsubscribeFromNotification(const QString &name)
{
    Q_D(QPSQLDriver);
    if (!isOpen()) {
        qCWarning(lcPsql, "QPSQLDriver::unsubscribeFromNotification: Database not open.");
        return false;
    }

    if (!d->seid.contains(name)) {
        qCWarning(lcPsql, "QPSQLDriver::unsubscribeFromNotification: not subscribed to '%ls'.",
                  qUtf16Printable(name));
        return false;
    }

    QString query = QStringLiteral("UNLISTEN ") + escapeIdentifier(name, QSqlDriver::TableName);
    PGresult *result = d->exec(query);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        setLastError(qMakeError(tr("Unable to unsubscribe"), QSqlError::StatementError, d, result));
        PQclear(result);
        return false;
    }
    PQclear(result);

    d->seid.removeAll(name);

    if (d->seid.isEmpty()) {
        disconnect(d->sn, &QSocketNotifier::activated, this, &QPSQLDriver::_q_handleNotification);
        delete d->sn;
        d->sn = nullptr;
    }

    return true;
}

QStringList QPSQLDriver::subscribedToNotifications() const
{
    Q_D(const QPSQLDriver);
    return d->seid;
}

void QPSQLDriver::_q_handleNotification()
{
    Q_D(QPSQLDriver);
    d->pendingNotifyCheck = false;
    PQconsumeInput(d->connection);

    PGnotify *notify = nullptr;
    while ((notify = PQnotifies(d->connection)) != nullptr) {
        QString name(QLatin1StringView(notify->relname));
        if (d->seid.contains(name)) {
            QString payload;
#if defined PG_VERSION_NUM && PG_VERSION_NUM-0 >= 70400
            if (notify->extra)
                payload = QString::fromUtf8(notify->extra);
#endif
            QSqlDriver::NotificationSource source = (notify->be_pid == PQbackendPID(d->connection)) ? QSqlDriver::SelfSource : QSqlDriver::OtherSource;
            emit notification(name, source, payload);
        }
        else
            qCWarning(lcPsql, "QPSQLDriver: received notification for '%ls' which isn't subscribed to.",
                      qUtf16Printable(name));

        qPQfreemem(notify);
    }
}

QT_END_NAMESPACE

#include "moc_qsql_psql_p.cpp"
