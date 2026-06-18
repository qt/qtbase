// Copyright (C) 2026 The Qt Company Ltd.
// Copyright (C) 2026 Andreas Bacher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#include "qsql_firebird_p.h"

#include "qsql_firebird_driver_p.h"
#include "qsql_firebird_helpers_p.h"
#include "qsql_firebird_result_p.h"

#include <QtCore/qdatetime.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qvariant.h>
#include <QtSql/qsqlfield.h>
#include <QtSql/qsqlindex.h>
#include <QtSql/qsqlquery.h>
#include <QtSql/qsqlrecord.h>

#include <ibase.h>               // DPB/TPB constants, isc_event_block
#include <firebird/Interface.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <mutex>

/* The driver uses the Firebird 4.0 object-oriented C++ API. Configure-time
   detection (FindFirebird.cmake with Firebird_MINIMUM_API_VERSION) already
   disables the sql-firebird feature for older clients; this static_assert is a
   backstop for out-of-tree builds that bypass configure. */
static_assert(FB_API_VER >= 40,
              "The QFIREBIRD driver requires Firebird 4.0 or later client libraries.");

/* Error handling is built on the API's ThrowStatusWrapper/FbException model,
   so the driver cannot be compiled without exceptions. Configure-time detection
   already disables the sql-firebird feature in a -no-exceptions build; this is
   the backstop for out-of-tree builds that bypass configure. */
#ifdef QT_NO_EXCEPTIONS
#  error The QFIREBIRD driver requires C++ exception support
#endif

QT_BEGIN_NAMESPACE

// Firebird OO API types are used throughout; avoid full Firebird:: qualification.
using namespace Firebird;

using namespace Qt::StringLiterals;

/*! \internal
    Event notification support (OO API)

    Follows the pattern from Firebird's examples/interfaces/08.events.cpp:
    - The subscription IS the callback (IEventCallback), ref-counted
    - eventCallbackFunction (Firebird thread) memcpy's result + posts; it holds
      a self-reference for its duration and a mutex around the buffer write and
      the driver-pointer use (stop() clears the pointer under the same mutex,
      so a returned stop() guarantees no callback still touches the driver)
    - isc_event_counts and re-registration happen on the main thread; the read
      takes the same mutex (re-arm does not — see qHandleEventNotification)
    - Only release(), never cancel(), is used on IEvents
*/

class QFirebirdEventSubscription
    : public IEventCallbackImpl<QFirebirdEventSubscription, CheckStatusWrapper>
{
public:
    QFirebirdEventSubscription(QFirebirdDriver *drv, IAttachment *att,
                               const QString &eventName)
        : name(eventName), driver(drv), attachment(att)
    {
        bufferLength = static_cast<unsigned>(
            isc_event_block(&eventBuffer, &resultBuffer,
                            1, eventName.toUtf8().constData()));
    }

    // IRefCounted
    void addRef() override { ++refCount; }
    int  release() override {
        if (--refCount == 0) {
            delete this;
            return 0;
        }
        return 1;
    }

    /*! \internal
        IEventCallback — called on Firebird's internal event thread.
        Holds a self-reference for the duration so the owner thread's stop()/
        release() cannot delete the subscription (and free resultBuffer) while
        this callback is still running. The mutex serialises the resultBuffer
        write against qHandleEventNotification's read on the owner thread, and
        covers the driver-pointer use: stop() clears the pointer under the same
        mutex, so the driver cannot be destroyed between the read and the
        invokeMethod. The queued invokeMethod only posts an event — it neither
        blocks nor re-enters Firebird — so holding the mutex across it cannot
        deadlock with the re-arm lock-ordering constraint.
    */
    void eventCallbackFunction(unsigned length, const unsigned char *data) override
    {
        addRef();
        {
            std::lock_guard<std::mutex> lock(mutex);
            /* Clamp to the registered buffer size: fbclient should never
               deliver a longer block, but the length arrives off the wire. */
            length = std::min(length, bufferLength);
            if (length > 0)
                memcpy(resultBuffer, data, length);
            ++counter;
            if (driver) {
                QMetaObject::invokeMethod(driver, "qHandleEventNotification",
                                          Qt::QueuedConnection, Q_ARG(QString, name));
            }
        }
        release();
    }

    /*! \internal
        Disconnect from the driver so the callback no longer posts to it,
        and release the IEvents handle (no cancel — avoids Firebird internal
        lock contention that causes crashes on Windows). Clearing the pointer
        under the mutex waits out any in-flight callback that already loaded it,
        so after stop() returns the driver can be destroyed safely.
    */
    void stop()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            driver = nullptr;
        }
        if (events) {
            events->release();
            events = nullptr;
        }
    }

    QString name;
    QFirebirdDriver *driver = nullptr; // guarded by mutex (written on the owner thread,
                                       // read on the Firebird event thread)
    IAttachment *attachment = nullptr;
    IEvents *events = nullptr;
    ISC_UCHAR *eventBuffer  = nullptr;
    ISC_UCHAR *resultBuffer = nullptr;
    unsigned bufferLength = 0;
    bool first = true;
    std::atomic<int> counter = 0;
    std::mutex mutex; // serialises resultBuffer access with the FB event thread

private:
    ~QFirebirdEventSubscription()
    {
        if (events)
            events->release();
        if (eventBuffer)
            isc_free(reinterpret_cast<char *>(eventBuffer));
        if (resultBuffer)
            isc_free(reinterpret_cast<char *>(resultBuffer));
    }

    std::atomic<int> refCount = 0;
};

//  QFirebirdDriverPrivate

void QFirebirdDriverPrivate::setFbError(const QString &context, QSqlError::ErrorType type)
{
    Q_Q(QFirebirdDriver);
    auto err = fbError(master, iStatus, type);
    q->setLastError(QSqlError(context + u": " + err.databaseText(),
                              {}, err.type(), err.nativeErrorCode()));
    iStatus->init();
}

bool QFirebirdDriverPrivate::finishTransaction(TxnEnd end)
{
    Q_Q(QFirebirdDriver);
    if (!iTrans) {
        q->setLastError(QSqlError(u"No active transaction"_s, {},
                                  QSqlError::TransactionError));
        return false;
    }
    try {
        ThrowStatusWrapper st(iStatus);
        finishAndClear(st, iTrans, end);
    } catch (const FbException &e) {
        q->setLastError(fbError(master, e.getStatus(), QSqlError::TransactionError));
        iStatus->init();
        return false;
    }
    return true;
}

//  QFirebirdDriver

QFirebirdDriver::QFirebirdDriver(QObject *parent)
    : QSqlDriver(*new QFirebirdDriverPrivate, parent)
{
}

QFirebirdDriver::~QFirebirdDriver()
{
    close();
    /* Neutralise any results that outlive this driver: once the driver private
       below is freed, their drv_d_func() would dangle. After this, cleanup()
       on those results becomes a no-op (see QFirebirdResultPrivate::cleanup()). */
    Q_D(QFirebirdDriver);
    for (QFirebirdResultPrivate *r : std::as_const(d->activeResults))
        r->driverAlive = false;
    d->activeResults.clear();
}

bool QFirebirdDriver::hasFeature(DriverFeature feature) const
{
    Q_D(const QFirebirdDriver);
    switch (feature) {
    case Transactions:
    case Unicode:
    case BLOB:
    case PreparedQueries:
    case PositionalPlaceholders:
    case LowPrecisionNumbers:
    case EventNotifications:
    case BatchOperations:
    case FinishQuery: // QSqlQuery::finish() releases the cursor, keeps the prepare
    case CancelQuery: // cancelQuery() raises fb_cancel_raise from another thread
        return true;
    case QuerySize:
        /* Firebird cursors cannot report cardinality without fetching, so the
           count is only available when the opt-in client-side row cache
           (connect option FB_SCROLLABLE_CACHE=1) buffers the result set anyway
           — see QFirebirdResult::size(). */
        return d->cacheScrollableResults;
    case NamedPlaceholders:
    case LastInsertId:
    case SimpleLocking:
    case MultipleResultSets:
        return false;
    }
    return false;
}

/*! \internal
    Connection options recognized by open(), mapped to their DPB tag and value
    kind. lc_ctype and the SQL dialect are intentionally NOT exposed: the driver
    hardcodes UTF-8 and dialect 3 and decodes results on that basis. Integer
    kinds also cover the boolean DPB options (pass 0/1).
*/
namespace {
enum DpbValueKind : quint8 { DpbString, DpbInt };
struct DpbOption {
    QLatin1StringView key;
    unsigned char tag;
    DpbValueKind kind;
};
constexpr DpbOption dpbOptionTable[] = {
    { "ISC_DPB_SQL_ROLE_NAME"_L1,         isc_dpb_sql_role_name,         DpbString },
    { "ISC_DPB_SESSION_TIME_ZONE"_L1,     isc_dpb_session_time_zone,     DpbString },
    { "ISC_DPB_NUM_BUFFERS"_L1,           isc_dpb_num_buffers,           DpbInt },
    { "ISC_DPB_CONNECT_TIMEOUT"_L1,       isc_dpb_connect_timeout,       DpbInt },
    { "ISC_DPB_DUMMY_PACKET_INTERVAL"_L1, isc_dpb_dummy_packet_interval, DpbInt },
    { "ISC_DPB_NO_GARBAGE_COLLECT"_L1,    isc_dpb_no_garbage_collect,    DpbInt },
    { "ISC_DPB_NO_DB_TRIGGERS"_L1,        isc_dpb_no_db_triggers,        DpbInt },
};

/*! \internal
    Options that map to real DPB tags but which the driver sets itself and will
    not let the caller override: the connection charset is fixed to UTF-8 and the
    SQL dialect to 3, because results are decoded on that basis. Recognized here
    so they get an accurate message instead of being reported as "unknown".
*/
constexpr QLatin1StringView driverManagedKeys[] = {
    "ISC_DPB_LC_CTYPE"_L1,
    "ISC_DPB_SQL_DIALECT"_L1,
};
} // namespace

bool QFirebirdDriver::open(const QString &db,
                           const QString &user,
                           const QString &password,
                           const QString &host,
                           int port,
                           const QString &connOpts)
{
    Q_D(QFirebirdDriver);

    close();

    // Build the connection string: host/port:database (Firebird uses / between host and port)
    QString connStr;
    if (!host.isEmpty()) {
        connStr = host;
        if (port > 0)
            connStr += u'/' + QString::number(port);
        connStr += u':';
    }
    connStr += db;

    // Build DPB using IXpbBuilder
    IUtil *util = d->master->getUtilInterface();
    ThrowStatusWrapper st(d->iStatus);
    try {
        // Disposed by its guard on scope exit (normal return or the catch below).
        FbGuard<IXpbBuilder> dpb(util->getXpbBuilder(&st, IXpbBuilder::DPB, nullptr, 0),
                                 fbDispose<IXpbBuilder>);

        dpb->insertString(&st, isc_dpb_user_name,
                          user.toUtf8().constData());
        dpb->insertString(&st, isc_dpb_password,
                          password.toUtf8().constData());
        // UTF-8 connection charset
        dpb->insertString(&st, isc_dpb_lc_ctype, "UTF8");

        /* Apply recognized connection options (semicolon-separated key=value).
           Unknown or malformed options are warned about rather than silently
           ignored. */
        for (const auto &opt : QStringView(connOpts).split(u';', Qt::SkipEmptyParts)) {
            const auto kv = opt.trimmed();
            const auto eqIdx = kv.indexOf(u'=');
            if (eqIdx < 0) {
                qCWarning(lcFirebird) << "QFirebirdDriver: ignoring malformed connection option"
                                      << kv.toString();
                continue;
            }
            const QString key = kv.left(eqIdx).trimmed().toString().toUpper();
            const QString value = kv.mid(eqIdx + 1).trimmed().toString();

            /* Driver-level option (not a DPB tag): enable client-side row
               caching for scrollable results. Handled before the DPB lookup so
               it isn't reported as "unknown". */
            if (key == "FB_SCROLLABLE_CACHE"_L1) {
                d->cacheScrollableResults = (value != "0"_L1)
                        && (value.compare("false"_L1, Qt::CaseInsensitive) != 0);
                continue;
            }

            /* Driver-managed options: silently accept a value that matches what
               the driver already enforces (e.g. ISC_DPB_LC_CTYPE=UTF8); warn
               only when the caller asks for something the driver won't honor. */
            bool managed = false;
            for (const auto &k : driverManagedKeys) {
                if (key == k) {
                    managed = true;
                    break;
                }
            }
            if (managed) {
                const bool isLcCtype = (key == "ISC_DPB_LC_CTYPE"_L1);
                const bool matchesDefault = isLcCtype
                        ? (value.compare("UTF8"_L1, Qt::CaseInsensitive) == 0
                           || value.compare("UTF-8"_L1, Qt::CaseInsensitive) == 0)
                        : (value == "3"_L1);
                if (!matchesDefault) {
                    qCWarning(lcFirebird) << "QFirebirdDriver: connection option" << key
                                          << "is managed by the driver and cannot be overridden"
                                             " (charset is fixed to UTF-8, dialect to 3); ignoring"
                                          << value;
                }
                continue;
            }

            const DpbOption *match = nullptr;
            for (const auto &o : dpbOptionTable) {
                if (key == o.key) {
                    match = &o;
                    break;
                }
            }
            if (!match) {
                qCWarning(lcFirebird) << "QFirebirdDriver: ignoring unknown connection option" << key;
                continue;
            }
            if (match->kind == DpbString) {
                QByteArray s = value.toUtf8();
                if (match->tag == isc_dpb_sql_role_name)
                    s.truncate(255);
                dpb->insertString(&st, match->tag, s.constData());
            } else {
                bool ok = false;
                const int n = value.toInt(&ok);
                if (!ok) {
                    qCWarning(lcFirebird) << "QFirebirdDriver: non-integer value for connection option"
                                          << key << ':' << value;
                    continue;
                }
                dpb->insertInt(&st, match->tag, n);
            }
        }

        const unsigned char *dpbData = dpb->getBuffer(&st);
        unsigned dpbLen              = dpb->getBufferLength(&st);

        IAttachment *att = d->master->getDispatcher()->attachDatabase(
            &st, connStr.toUtf8().constData(), dpbLen, dpbData);
        /* Publish under the mutex so a concurrent cancelQuery() never sees a
           half-initialised attachment. */
        {
            const std::lock_guard<std::mutex> lock(d->attMutex);
            d->iAtt = att;
        }
    } catch (const FbException &e) {
        setLastError(fbError(d->master, e.getStatus(), QSqlError::ConnectionError));
        d->iStatus->init();
        setOpenError(true);
        return false;
    }

    setOpen(true);
    setOpenError(false);

    return true;
}

void QFirebirdDriver::close()
{
    Q_D(QFirebirdDriver);
    if (!isOpen())
        return;

    CheckStatusWrapper st(d->iStatus);
    // Cancel all event subscriptions before detaching
    for (auto *sub : std::as_const(d->eventSubscriptions)) {
        sub->stop();
        sub->release();
    }
    d->eventSubscriptions.clear();
    if (d->iTrans) {
        try {
            ThrowStatusWrapper tsw(d->iStatus);
            d->iTrans->rollback(&tsw);
        } catch (...) {
            d->iTrans->release();
        }
        d->iTrans = nullptr;
    }
    if (d->iAtt) {
        /* Withdraw the attachment from cancelQuery()'s reach before detaching:
           once the lock is dropped, a concurrent cancelQuery() sees nullptr,
           and one already inside cancelOperation() has finished (it holds the
           mutex for the duration of the call). */
        IAttachment *att;
        {
            const std::lock_guard<std::mutex> lock(d->attMutex);
            att = d->iAtt;
            d->iAtt = nullptr;
        }
        try {
            ThrowStatusWrapper tsw(d->iStatus);
            att->detach(&tsw);
        } catch (...) {
            att->release();
        }
    }
    d->iStatus->init();
    setOpen(false);
    setOpenError(false);
}

QSqlResult *QFirebirdDriver::createResult() const
{
    return new QFirebirdResult(this);
}

/*! \internal
    Cross-thread cancellation (QSqlDriver::CancelQuery). This runs on a
    DIFFERENT thread than the one executing the query, so it must not touch any
    driver state beyond the mutex-guarded attachment: no setLastError, and not
    iStatus (the executing thread owns it) — a private status object is used
    instead. fb_cancel_raise asks the engine to abort the current operation at
    its next cancellation point; the executing call then fails with
    isc_cancelled, which the normal FbException -> QSqlError path reports on the
    executing thread. The attachment itself stays open and usable.
*/
bool QFirebirdDriver::cancelQuery()
{
    Q_D(QFirebirdDriver);
    const std::lock_guard<std::mutex> lock(d->attMutex);
    if (!d->iAtt)
        return false;
    FbGuard<IStatus> status(d->master->getStatus(), fbDispose<IStatus>);
    try {
        ThrowStatusWrapper st(status.get());
        d->iAtt->cancelOperation(&st, fb_cancel_raise);
    } catch (const FbException &e) {
        const auto err = fbError(d->master, e.getStatus(), QSqlError::UnknownError);
        qCWarning(lcFirebird) << "cancelQuery:" << fbErrorLog(err);
        return false;
    }
    return true;
}

bool QFirebirdDriver::beginTransaction()
{
    Q_D(QFirebirdDriver);
    /* QSqlDatabase::transaction() forwards here after only a feature check, so
       guard against a closed (or failed-to-open) database: without it the
       start call below would dereference a null attachment. */
    if (!isOpen() || isOpenError()) {
        setLastError(QSqlError(u"Database not open"_s, {},
                               QSqlError::TransactionError));
        return false;
    }
    if (d->iTrans) {
        setLastError(QSqlError(u"Transaction already active"_s, {},
                               QSqlError::TransactionError));
        return false;
    }
    try {
        ThrowStatusWrapper st(d->iStatus);
        d->iTrans = startDefaultTransaction(d->iAtt, st, TxnWait::NoWait);
    } catch (const FbException &e) {
        setLastError(fbError(d->master, e.getStatus(), QSqlError::TransactionError));
        d->iStatus->init();
        return false;
    }
    return true;
}

bool QFirebirdDriver::commitTransaction()
{
    Q_D(QFirebirdDriver);
    return d->finishTransaction(TxnEnd::Commit);
}

bool QFirebirdDriver::rollbackTransaction()
{
    Q_D(QFirebirdDriver);
    return d->finishTransaction(TxnEnd::Rollback);
}

QVariant QFirebirdDriver::handle() const
{
    Q_D(const QFirebirdDriver);
    return QVariant::fromValue(d->iAtt);
}

QString QFirebirdDriver::escapeIdentifier(const QString &identifier,
                                          IdentifierType /*type*/) const
{
    QString res = identifier;
    if (!identifier.isEmpty() && !identifier.startsWith(u'"') && !identifier.endsWith(u'"')) {
        res.replace(u'"', u"\"\""_s);
        res.replace(u'.', u"\".\""_s);
        res = u'"' + res + u'"';
    }
    return res;
}

QString QFirebirdDriver::formatValue(const QSqlField &field, bool trimStrings) const
{
    switch (field.metaType().id()) {
    case QMetaType::QDateTime: {
        /* A plain TIMESTAMP is naive: emit the QDateTime's own wall-clock
           date/time components verbatim (no conversion), matching the
           bound-parameter path (encodeQDateTime). */
        const QDateTime dt = field.value().toDateTime();
        if (dt.isValid())
            return dt.toString(u"''yyyy-MM-dd HH:mm:ss.zzz''");
        return u"NULL"_s;
    }
    case QMetaType::QTime: {
        const QTime t = field.value().toTime();
        if (t.isValid())
            return t.toString(u"''HH:mm:ss.zzz''");
        return u"NULL"_s;
    }
    case QMetaType::QDate: {
        const QDate d = field.value().toDate();
        if (d.isValid())
            return d.toString(u"''yyyy-MM-dd''");
        return u"NULL"_s;
    }
    default:
        return QSqlDriver::formatValue(field, trimStrings);
    }
}

int QFirebirdDriver::maximumIdentifierLength(IdentifierType /*type*/) const
{
    // Firebird 4.0+ raised the maximum identifier length to 63 characters
    return 63;
}

//  Schema introspection helpers

/*! \internal
    setLastError only mutates the driver's error state, which is logically
    mutable from the const introspection methods — hence the const_cast.
*/
QSqlQuery QFirebirdDriver::execMetadataQuery(const QString &sql,
                                             const QVariantList &binds) const
{
    QSqlQuery q(createResult());
    q.prepare(sql);
    for (const QVariant &v : binds)
        q.addBindValue(v);
    if (!q.exec()) {
        qCWarning(lcFirebird) << "metadata query failed:"
                              << fbErrorLog(q.lastError()) << "SQL:" << sql;
        const_cast<QFirebirdDriver *>(this)->setLastError(q.lastError());
    }
    return q;
}

QStringList QFirebirdDriver::tables(QSql::TableType type) const
{
    QStringList result;
    if (!isOpen())
        return result;

    QString filter;
    if (type == QSql::SystemTables) {
        filter = u"WHERE RDB$SYSTEM_FLAG != 0"_s;
    } else if (type == (QSql::SystemTables | QSql::Views)) {
        filter = u"WHERE RDB$SYSTEM_FLAG != 0 OR RDB$VIEW_SOURCE IS NOT NULL"_s;
    } else {
        QStringList conditions;
        if (!(type & QSql::SystemTables))
            conditions << u"RDB$SYSTEM_FLAG = 0"_s;
        if (!(type & QSql::Views))
            conditions << u"RDB$VIEW_SOURCE IS NULL"_s;
        if (!(type & QSql::Tables))
            conditions << u"RDB$VIEW_SOURCE IS NOT NULL"_s;
        if (!conditions.isEmpty())
            filter = u"WHERE "_s + conditions.join(u" AND "_s);
    }

    QSqlQuery q = execMetadataQuery(
        u"SELECT TRIM(RDB$RELATION_NAME) FROM RDB$RELATIONS "_s
        + filter + u" ORDER BY RDB$RELATION_NAME"_s);
    while (q.next())
        result.append(q.value(0).toString());
    return result;
}

QSqlRecord QFirebirdDriver::record(const QString &tableName) const
{
    QSqlRecord rec;
    if (!isOpen())
        return rec;

    /* A quoted (delimited) identifier is case-sensitive: strip its delimiters and
       match RDB$RELATION_NAME verbatim. An unquoted name is passed through as-is
       (stripDelimiters() is a no-op for it); the caller is responsible for the
       upper-case folding Firebird applies to unquoted identifiers. This mirrors
       the QIBASE driver's behaviour. */
    const QString table = stripDelimiters(tableName, QSqlDriver::TableName);

    /* RDB$FIELDS.RDB$FIELD_TYPE stores the BLR type codes (blr_short, blr_long, …).
       Map them to Qt types. */
    const QString sql =
        u"SELECT TRIM(f.RDB$FIELD_NAME), fld.RDB$FIELD_TYPE, fld.RDB$FIELD_SUB_TYPE,"
        u" fld.RDB$FIELD_SCALE, fld.RDB$CHARACTER_LENGTH, f.RDB$NULL_FLAG,"
        u" fld.RDB$FIELD_PRECISION, fld.RDB$FIELD_LENGTH"
        u" FROM RDB$RELATION_FIELDS f"
        u" JOIN RDB$FIELDS fld ON fld.RDB$FIELD_NAME = f.RDB$FIELD_SOURCE"
        u" WHERE TRIM(f.RDB$RELATION_NAME) = ?"
        u" ORDER BY f.RDB$FIELD_POSITION"_s;

    QSqlQuery q = execMetadataQuery(sql, {table});

    while (q.next()) {
        QString  name      = q.value(0).toString().trimmed();
        int      rdbType   = q.value(1).toInt();
        int      subType   = q.value(2).toInt();
        int      scale     = q.value(3).toInt();        // negative for NUMERIC/DECIMAL
        int      charLen   = q.value(4).toInt();
        bool     notNull   = (q.value(5).toInt() == 1);
        int      precision = q.value(6).toInt();        // total digits for NUMERIC
        int      byteLen   = q.value(7).toInt();

        QMetaType::Type qtType = blrTypeToQt(static_cast<unsigned char>(rdbType), scale < 0);
        QSqlField fld(name, QMetaType(qtType));
        fld.setRequired(notNull);

        // Set length and precision for NUMERIC/DECIMAL; character length for text types
        if (scale < 0 && precision > 0) {
            fld.setLength(precision);
            fld.setPrecision(-scale);
        } else if (rdbType == blr_text || rdbType == blr_varying) {  // CHAR / VARCHAR
            fld.setLength(charLen > 0 ? charLen : byteLen);
        }

        Q_UNUSED(subType)
        rec.append(fld);
    }
    return rec;
}

QSqlIndex QFirebirdDriver::primaryIndex(const QString &tableName) const
{
    QSqlIndex index;
    if (!isOpen())
        return index;

    // See record(): strip a quoted identifier and match RDB$RELATION_NAME verbatim.
    const QString table = stripDelimiters(tableName, QSqlDriver::TableName);

    const QString sql =
        u"SELECT TRIM(s.RDB$FIELD_NAME)"
        u" FROM RDB$RELATION_CONSTRAINTS c"
        u" JOIN RDB$INDEX_SEGMENTS s ON s.RDB$INDEX_NAME = c.RDB$INDEX_NAME"
        u" WHERE c.RDB$CONSTRAINT_TYPE = 'PRIMARY KEY'"
        u"   AND TRIM(c.RDB$RELATION_NAME) = ?"
        u" ORDER BY s.RDB$FIELD_POSITION"_s;

    QSqlQuery q = execMetadataQuery(sql, {table});

    QSqlRecord rec = record(tableName);
    while (q.next()) {
        QString col = q.value(0).toString().trimmed();
        index.append(rec.field(col));
    }
    return index;
}

//  Event notification API

bool QFirebirdDriver::subscribeToNotification(const QString &name)
{
    Q_D(QFirebirdDriver);
    if (!isOpen()) {
        qCWarning(lcFirebird, "QFirebirdDriver::subscribeToNotification: database not open.");
        return false;
    }
    if (d->eventSubscriptions.contains(name)) {
        qCWarning(lcFirebird,
                  "QFirebirdDriver::subscribeToNotification: already subscribed to '%ls'.",
                  qUtf16Printable(name));
        return false;
    }

    auto *sub = new QFirebirdEventSubscription(this, d->iAtt, name);
    sub->addRef();  // our reference

    try {
        ThrowStatusWrapper csw(d->iStatus);
        sub->events = d->iAtt->queEvents(&csw, sub,
                                          sub->bufferLength, sub->eventBuffer);
    } catch (const FbException &e) {
        sub->release();
        auto err = fbError(d->master, e.getStatus(), QSqlError::ConnectionError);
        setLastError(QSqlError(
            tr("Could not subscribe to event notifications for %1.").arg(name),
            err.databaseText(), err.type(), err.nativeErrorCode()));
        return false;
    }

    d->eventSubscriptions.insert(name, sub);
    return true;
}

bool QFirebirdDriver::unsubscribeFromNotification(const QString &name)
{
    Q_D(QFirebirdDriver);
    if (!isOpen()) {
        qCWarning(lcFirebird, "QFirebirdDriver::unsubscribeFromNotification: database not open.");
        return false;
    }
    if (!d->eventSubscriptions.contains(name)) {
        qCWarning(lcFirebird,
                  "QFirebirdDriver::unsubscribeFromNotification: not subscribed to '%ls'.",
                  qUtf16Printable(name));
        return false;
    }

    QFirebirdEventSubscription *sub = d->eventSubscriptions.take(name);
    sub->stop();
    sub->release();
    return true;
}

QStringList QFirebirdDriver::subscribedToNotifications() const
{
    Q_D(const QFirebirdDriver);
    return QStringList(d->eventSubscriptions.keys());
}

void QFirebirdDriver::qHandleEventNotification(const QString &name)
{
    Q_D(QFirebirdDriver);

    QFirebirdEventSubscription *sub = d->eventSubscriptions.value(name, nullptr);
    if (!sub || sub->counter.load(std::memory_order_relaxed) == 0)
        return; // already unsubscribed or spurious call

    /* isc_event_counts writes one counter per event registered with isc_event_block.
       Each QFirebirdEventSubscription registers exactly one event name. */
    static constexpr int maxEventNamesPerSubscription = 1;
    ISC_ULONG counts[maxEventNamesPerSubscription] = {};
    {
        /* Serialise the resultBuffer read against the Firebird event thread's
           memcpy in eventCallbackFunction — the only cross-thread buffer access. */
        std::lock_guard<std::mutex> lock(sub->mutex);
        isc_event_counts(counts, static_cast<ISC_USHORT>(sub->bufferLength),
                         sub->eventBuffer, sub->resultBuffer);
        sub->counter.store(0, std::memory_order_relaxed);
    }

    /* Re-arm: queEvents is one-shot. Release the old IEvents handle and
       re-register using the updated eventBuffer as the new baseline. This runs
       OUTSIDE sub->mutex on purpose: queEvents takes Firebird's internal event
       lock, which the event thread also holds while calling back into us, so
       holding our mutex across queEvents could deadlock. No callback can write
       resultBuffer between the read above and this re-arm (queEvents is
       one-shot), so the buffer stays stable in the gap. */
    IEvents *old = sub->events;
    sub->events = nullptr;

    IStatus *s = d->master->getStatus();
    ThrowStatusWrapper csw(s);
    try {
        sub->events = sub->attachment->queEvents(&csw, sub,
                                                  sub->bufferLength, sub->eventBuffer);
    } catch (const FbException &e) {
        const auto err = fbError(d->master, e.getStatus(), QSqlError::ConnectionError);
        qCWarning(lcFirebird) << "qHandleEventNotification: re-arm failed for"
                              << name << ':' << fbErrorLog(err);
    } catch (...) {
        qCWarning(lcFirebird, "qHandleEventNotification: unexpected exception re-arming '%ls'",
                  qUtf16Printable(name));
    }
    if (old)
        old->release();
    s->dispose();

    /* Suppress the first callback (initial registration baseline). Update the
       subscription state BEFORE emitting: a directly-connected slot may
       unsubscribe, releasing the subscription, so sub must not be touched
       after the emit. */
    const bool suppress = sub->first;
    sub->first = false;
    if (counts[0] > 0 && !suppress)
        emit notification(name, QSqlDriver::UnknownSource, QVariant());
}

QT_END_NAMESPACE
