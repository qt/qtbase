// Copyright (C) 2026 The Qt Company Ltd.
// Copyright (C) 2026 Andreas Bacher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#include "qsql_firebird_result_p.h"

#include "qsql_firebird_array_p.h"
#include "qsql_firebird_helpers_p.h"

#include <QtCore/qvarlengtharray.h>
#include <QtSql/qsqlfield.h>

#include <algorithm>
#include <cstring>
#include <limits>

QT_BEGIN_NAMESPACE

// Firebird OO API types are used throughout; avoid full Firebird:: qualification.
using namespace Firebird;

using namespace Qt::StringLiterals;

//  Helpers used only by the result implementation

// Build columns metadata from IMessageMetadata
static QList<ColumnInfo> buildColumns(IMessageMetadata *meta, ThrowStatusWrapper &st)
{
    QList<ColumnInfo> cols;
    unsigned count = meta->getCount(&st);
    cols.reserve(count);
    for (unsigned i = 0; i < count; ++i) {
        ColumnInfo ci;
        ci.name       = QString::fromUtf8(meta->getAlias(&st, i));
        if (ci.name.isEmpty())
            ci.name   = QString::fromUtf8(meta->getField(&st, i));
        ci.relation   = QString::fromUtf8(meta->getRelation(&st, i));
        ci.field      = QString::fromUtf8(meta->getField(&st, i));
        ci.fbType     = static_cast<int>(meta->getType(&st, i));
        ci.fbSubType  = meta->getSubType(&st, i);
        ci.scale      = meta->getScale(&st, i);
        ci.length     = meta->getLength(&st, i);
        ci.offset     = meta->getOffset(&st, i);
        ci.nullOffset = meta->getNullOffset(&st, i);
        ci.nullable   = (meta->isNullable(&st, i) != FB_FALSE);
        ci.qtType     = fbTypeToQt(ci.fbType, ci.scale);
        cols.append(ci);
    }
    return cols;
}

/*! \internal
    Read a BLOB into QByteArray. Uses ThrowStatusWrapper so a read error throws
    FbException (caught by QFirebirdResult::data) instead of being swallowed and
    returning a silently truncated buffer.
*/
static QByteArray readBlob(IAttachment *att, ITransaction *tra, IStatus *st,
                           const ISC_QUAD &blobId)
{
    ThrowStatusWrapper tsw(st);
    FbGuard<IBlob> blob(att->openBlob(&tsw, tra, const_cast<ISC_QUAD *>(&blobId), 0, nullptr),
                        fbRelease<IBlob>);
    if (!blob)
        return {};

    QByteArray result;
    unsigned char seg[16384];
    result.reserve(sizeof(seg)); // avoid reallocations for the common small-BLOB case
    unsigned segLen = 0;
    int code;
    /* A getSegment or close failure throws; FbGuard releases the handle on the
       way out (and closeWith rethrows after releasing on a failed close). */
    do {
        code = blob->getSegment(&tsw, sizeof(seg), seg, &segLen);
        result.append(reinterpret_cast<const char *>(seg), segLen);
    } while (code == IStatus::RESULT_OK || code == IStatus::RESULT_SEGMENT);
    blob.closeWith([&](IBlob *b) { b->close(&tsw); });
    return result;
}

//  QFirebirdResultPrivate implementation

void QFirebirdResultPrivate::cleanup()
{
    /* Only touch the Firebird handles while the driver is alive AND its
       attachment is still open. If the connection was closed while this result
       was still around (QSqlDatabase::close() with a QSqlQuery in scope),
       detach() has already invalidated the statement/cursor/transaction; if the
       driver object itself was destroyed (driverAlive == false), the driver
       private is freed and must not be dereferenced at all. In either case just
       drop the now-dangling pointers. driverAlive is checked first so att()
       (which reads the driver private) is never called on a freed driver. */
    const bool attached = driverAlive && att() != nullptr;

    /* Close a handle gracefully while the attachment is alive (release it if the
       close throws); when detached, the engine has already invalidated it, so
       just drop the now-dangling pointer. closeFn performs the type's graceful
       shutdown (IResultSet::close, IStatement::free, ITransaction::commit). */
    auto closeHandle = [&](auto *&handle, auto closeFn) {
        if (!handle)
            return;
        if (attached) {
            try {
                ThrowStatusWrapper tsw(status());
                closeFn(handle, tsw);
            } catch (...) {
                handle->release();
            }
        }
        handle = nullptr;
    };

    closeHandle(cursor, [](IResultSet *c, ThrowStatusWrapper &st) { c->close(&st); });
    closeHandle(stmt, [](IStatement *s, ThrowStatusWrapper &st) { s->free(&st); });
    closeHandle(autoTrans, [](ITransaction *t, ThrowStatusWrapper &st) { t->commit(&st); });

    if (outMeta) {
        if (attached)
            outMeta->release();
        outMeta = nullptr;
    }
    if (inMeta) {
        if (attached)
            inMeta->release();
        inMeta = nullptr;
    }
    outBuffer.clear();
    inBuffer.clear();
    cols.clear();
    inCols.clear();
    arrayDescCache.clear();
    cachedRecord.clear();
    recordCached    = false;
    affectedRows    = -1;
    isSelect        = false;
    isProcExec      = false;
    procRowFetched  = false;
    txnOp           = TxnOp::None;
    useRowCache     = false;
    cacheComplete   = false;
    rowCache.clear();
}

void QFirebirdResultPrivate::closeCursor()
{
    if (!cursor)
        return;
    try {
        ThrowStatusWrapper tsw(status());
        cursor->close(&tsw);
    } catch (...) {
        cursor->release();
    }
    cursor = nullptr;
}

bool QFirebirdResultPrivate::ensureCachedRow(int index)
{
    if (index < 0)
        return false;
    while (!cacheComplete && rowCache.size() <= index) {
        ThrowStatusWrapper st(status());
        /* fetchNext writes into outBuffer; detaching it (COW) on the next
           iteration keeps the snapshot just appended to rowCache intact. */
        const int code = cursor->fetchNext(&st, outBuffer.data());
        if (code == IStatus::RESULT_OK)
            rowCache.append(outBuffer);
        else
            cacheComplete = true; // RESULT_NO_DATA (or non-OK): end of set
    }
    return index < rowCache.size();
}

QString QFirebirdResultPrivate::formatFbError(const QString &ctx)
{
    QString msg = ctx + u": " + fbErrorString(master(), status());
    status()->init();
    return msg;
}

ITransaction *QFirebirdResultPrivate::ensureTransaction()
{
    // User-managed transaction takes priority.
    if (drv_d_func()->iTrans)
        return drv_d_func()->iTrans;
    if (!autoTrans) {
        ThrowStatusWrapper st(status());
        autoTrans = startDefaultTransaction(att(), st, TxnWait::Wait);
    }
    return autoTrans;
}

//  QFirebirdResult

QFirebirdResult::QFirebirdResult(const QFirebirdDriver *driver)
    : QSqlResult(*new QFirebirdResultPrivate(this, driver))
{
    /* Register with the driver so it can neutralise this result if it is
       destroyed while the result is still alive (see ~QFirebirdDriver). */
    Q_D(QFirebirdResult);
    d->drv_d_func()->activeResults.append(d);
}

QFirebirdResult::~QFirebirdResult()
{
    Q_D(QFirebirdResult);
    d->cleanup();
}

void QFirebirdResult::cleanup()
{
    Q_D(QFirebirdResult);
    d->cleanup();
    setAt(QSql::BeforeFirstRow);
    setActive(false);
}

QVariant QFirebirdResult::handle() const
{
    Q_D(const QFirebirdResult);
    return QVariant::fromValue(d->stmt);
}

/*! \internal
    FinishQuery support: QSqlQuery::finish() routes here. Release the server
    cursor and the client-side row cache, but keep the prepared statement and
    its metadata caches so exec() can re-run the query without a new prepare().
    The auto transaction (if any) stays open on purpose: BLOB/array ids already
    handed to the application remain resolvable until the next exec()/cleanup().
*/
void QFirebirdResult::detachFromResultSet()
{
    Q_D(QFirebirdResult);
    d->closeCursor();
    d->rowCache.clear();
    d->cacheComplete = false;
}

/*! \internal
    Classify a bare transaction-control statement (COMMIT/ROLLBACK [WORK]). These
    appear in SQL scripts but cannot be executed as DSQL against the driver's
    managed transaction (Firebird rejects it with "invalid transaction handle");
    they are applied via the ITransaction API instead. COMMIT/ROLLBACK RETAIN
    are not treated specially.
*/
static TxnOp classifyTxnControl(const QString &query)
{
    QString s = query.trimmed();
    while (s.endsWith(u';'))
        s.chop(1);
    s = s.trimmed().toLower().simplified();
    if (s == "commit"_L1 || s == "commit work"_L1)
        return TxnOp::Commit;
    if (s == "rollback"_L1 || s == "rollback work"_L1)
        return TxnOp::Rollback;
    return TxnOp::None;
}

bool QFirebirdResult::prepare(const QString &query)
{
    Q_D(QFirebirdResult);
    cleanup();

    /* Bare COMMIT/ROLLBACK is handled in exec() via the transaction API; don't
       prepare it as a DSQL statement (the engine rejects executing it against a
       driver-managed transaction). */
    if (const TxnOp op = classifyTxnControl(query); op != TxnOp::None) {
        d->txnOp = op;
        setSelect(false);
        return true;
    }

    const QByteArray utf8 = query.toUtf8();
    try {
        ThrowStatusWrapper st(d->status());
        ITransaction *tr = d->ensureTransaction();
        d->stmt = d->att()->prepare(&st, tr, 0, utf8.constData(),
                                    SQL_DIALECT_V6,
                                    IStatement::PREPARE_PREFETCH_METADATA);
        unsigned flags = d->stmt->getFlags(&st);
        d->isSelect = (flags & IStatement::FLAG_HAS_CURSOR) != 0;
        setSelect(d->isSelect); // required: QSqlQuery::next() checks base-class isSelect()

        d->inMeta = d->stmt->getInputMetadata(&st);
        if (d->inMeta && d->inMeta->getCount(&st) == 0) {
            d->inMeta->release();
            d->inMeta = nullptr;
        }
        d->outMeta = d->stmt->getOutputMetadata(&st);
        if (d->outMeta) {
            if (d->outMeta->getCount(&st) == 0) {
                d->outMeta->release();
                d->outMeta = nullptr;
            } else {
                d->cols = buildColumns(d->outMeta, st);
                d->outBuffer.resize(static_cast<qsizetype>(d->outMeta->getAlignedLength(&st)));
            }
        }
        if (!d->isSelect && d->outMeta) {
            // EXECUTE PROCEDURE with RETURNS clause: expose as single-row result set
            d->isProcExec = true;
            setSelect(true);
        }
        if (d->inMeta) {
            d->inBuffer.resize(static_cast<qsizetype>(d->inMeta->getAlignedLength(&st)));
            d->inCols = buildColumns(d->inMeta, st);
        }
    } catch (const FbException &e) {
        const QSqlError err = fbError(d->master(), e.getStatus(), QSqlError::StatementError);
        qCInfo(lcFirebird) << "prepare:" << fbErrorLog(err);
        /* Release any partially-acquired statement/metadata so a failed prepare
           leaves the result non-executable (exec() guards on a null stmt) instead
           of running against a half-initialised statement. */
        d->cleanup();
        setSelect(false);
        setLastError(err);
        return false;
    }
    return true;
}

bool QFirebirdResultPrivate::buildInputMessage(QString &errorText)
{
    if (!inMeta)
        return true;
    inBuffer.fill(0);
    return fillInputBuffer(values, errorText);
}

bool QFirebirdResultPrivate::encodeScaledNumeric(char *data, int fbType, int scale,
                                                 const QVariant &val, QString &errorText)
{
    /* Numeric/decimal — store as INT64 scaled (or INT128 for very wide types).
       actual = stored * 10^scale, so stored = value / 10^scale. The scaled
       integer is derived exactly from the value's decimal-string form (see
       decimalToScaledInt64); out-of-range values are reported instead of
       silently wrapped. */
    if (fbBaseType(fbType) == SQL_INT128) {
        // Scale the string representation via IInt128::fromString with scale
        const QByteArray strVal = numericInputString(val);
        ThrowStatusWrapper innerSt(status());
        master()->getUtilInterface()->getInt128(&innerSt)->fromString(
            &innerSt, scale, strVal.constData(),
            reinterpret_cast<FB_I128 *>(data));
        return true;
    }

    const auto outOfRange = [&]() {
        errorText = u"Value '%1' out of range for NUMERIC/DECIMAL with scale %2"_s
                        .arg(val.toString(), QString::number(scale));
        return false;
    };

    qint64 iv = 0;
    const int t = val.typeId();
    if (scale < 0 && -scale < 19
        && (t == QMetaType::Int || t == QMetaType::UInt || t == QMetaType::LongLong)) {
        /* Fast path: an integral input scales with one checked multiply — no
           string round trip on the per-row bind path. */
        const qint64 v = val.toLongLong();
        const qint64 factor = pow10i(-scale);
        if (v == (std::numeric_limits<qint64>::min)()
            || qAbs(v) > (std::numeric_limits<qint64>::max)() / factor) {
            return outOfRange();
        }
        iv = v * factor;
    } else {
        const QByteArray strVal = numericInputString(val);
        if (!decimalToScaledInt64(strVal, scale, &iv)) {
            errorText = u"Cannot encode '%1' as NUMERIC/DECIMAL with scale %2"_s
                            .arg(QString::fromLatin1(strVal), QString::number(scale));
            return false;
        }
    }

    // One range check for the narrower targets, then a width-dispatched store.
    qint64 lo = (std::numeric_limits<qint64>::min)();
    qint64 hi = (std::numeric_limits<qint64>::max)();
    switch (fbBaseType(fbType)) {
    case SQL_SHORT:
        lo = (std::numeric_limits<qint16>::min)();
        hi = (std::numeric_limits<qint16>::max)();
        break;
    case SQL_LONG:
        lo = (std::numeric_limits<qint32>::min)();
        hi = (std::numeric_limits<qint32>::max)();
        break;
    default:
        break;
    }
    if (iv < lo || iv > hi)
        return outOfRange();

    switch (fbBaseType(fbType)) {
    case SQL_SHORT:
        *reinterpret_cast<qint16 *>(data) = static_cast<qint16>(iv);
        break;
    case SQL_LONG:
        *reinterpret_cast<qint32 *>(data) = static_cast<qint32>(iv);
        break;
    default:
        *reinterpret_cast<qint64 *>(data) = iv;
        break;
    }
    return true;
}

void QFirebirdResultPrivate::writeInlineBlob(ISC_QUAD &blobId, const QByteArray &blobData)
{
    /* Create the blob via IAttachment. On failure the handle is released and the
       exception propagates to exec()/execBatch, which report it — we never bind
       SQL NULL in place of the caller's data. */
    ThrowStatusWrapper bst(status());
    ITransaction *tr = ensureTransaction();
    FbGuard<IBlob> blob(att()->createBlob(&bst, tr, &blobId, 0, nullptr), fbRelease<IBlob>);
    const char *src = blobData.constData();
    qsizetype remaining = blobData.size();
    while (remaining > 0) {
        const unsigned segSize = static_cast<unsigned>(
            std::min(remaining, static_cast<qsizetype>(65535)));
        blob->putSegment(&bst, segSize, reinterpret_cast<const void *>(src));
        src += segSize;
        remaining -= segSize;
    }
    /* A putSegment or close failure throws; FbGuard frees the handle on the way
       out, so the caller's data is never silently replaced with SQL NULL. */
    blob.closeWith([&](IBlob *b) { b->close(&bst); });
}

void QFirebirdResultPrivate::adoptReplacementTransaction(ITransaction *executedTr,
                                                         ITransaction *newTr)
{
    if (!newTr || newTr == executedTr)
        return;
    /* Update whichever member actually held the executed transaction (it came
       from ensureTransaction(), so it is exactly one of these), releasing the
       old handle once and never touching the unrelated transaction — otherwise
       the stale, engine-invalidated handle would be left for a later
       double-release. */
    if (executedTr == drv_d_func()->iTrans) {
        drv_d_func()->iTrans->release();
        drv_d_func()->iTrans = newTr;
    } else if (executedTr == autoTrans) {
        autoTrans->release();
        autoTrans = newTr;
    }
}

bool QFirebirdResultPrivate::fillInputBuffer(const QList<QVariant> &vals,
                                             QString &errorText,
                                             BlobWriter blobWriter)
{
    if (!inMeta)
        return true;

    const qsizetype count = inCols.size();

    /* Reject under-binding instead of silently executing the missing
       parameters as zero-filled NOT NULL values (inBuffer is pre-zeroed, so an
       unwritten parameter would bind 0/''/epoch rather than fail). Matches the
       parameter-mismatch error other Qt SQL drivers report. */
    if (vals.size() < count) {
        errorText = u"Parameter count mismatch: statement expects %1, %2 bound"_s
                        .arg(count).arg(vals.size());
        return false;
    }

    for (qsizetype i = 0; i < count; ++i) {
        const ColumnInfo &ci = inCols.at(i);
        short *nullFlag = reinterpret_cast<short *>(inBuffer.data() + ci.nullOffset);
        const QVariant &val = vals[i];

        /* Decide whether the value is "null". In Qt 6, a QVariant holding a
           default-constructed value (e.g. QVariant(QString()), QVariant(QDate()))
           reports isNull()==false even though the contained value is null/invalid,
           so check the common types explicitly. */
        bool valueIsNull = val.isNull();
        if (!valueIsNull) {
            switch (val.metaType().id()) {
            case QMetaType::QString:    valueIsNull = val.toString().isNull();      break;
            case QMetaType::QByteArray: valueIsNull = val.toByteArray().isNull();   break;
            case QMetaType::QDateTime:  valueIsNull = !val.toDateTime().isValid();  break;
            case QMetaType::QDate:      valueIsNull = !val.toDate().isValid();      break;
            case QMetaType::QTime:      valueIsNull = !val.toTime().isValid();      break;
            default: break;
            }
        }

        /* Bind SQL NULL whenever the value is null, regardless of whether the
           target column is nullable. For a NOT NULL column this lets the engine
           raise its own constraint violation (surfaced as a QSqlError) instead
           of silently coercing the null to a default (0/''/epoch) and inserting
           a bogus row. The legacy QIBASE driver substituted the default here,
           which masked the constraint — see QTBUG-114683. */
        if (valueIsNull) {
            *nullFlag = -1; // SQL NULL
            continue;
        }
        *nullFlag = 0;

        char *data = inBuffer.data() + ci.offset;

        if (ci.scale != 0) {
            QString numError;
            if (!encodeScaledNumeric(data, ci.fbType, ci.scale, val, numError)) {
                errorText = u"Parameter %1: %2"_s.arg(QString::number(i + 1), numError);
                return false;
            }
            continue;
        }

        switch (fbBaseType(ci.fbType)) {
        case SQL_SHORT:
            *reinterpret_cast<qint16 *>(data) = static_cast<qint16>(val.toInt());
            break;
        case SQL_LONG:
            *reinterpret_cast<qint32 *>(data) = static_cast<qint32>(val.toInt());
            break;
        case SQL_INT64:
            *reinterpret_cast<qint64 *>(data) = val.toLongLong();
            break;
        case SQL_INT128: {
            // Accept string or numeric; use IInt128::fromString
            const QByteArray strVal = numericInputString(val);
            ThrowStatusWrapper innerSt(status());
            master()->getUtilInterface()->getInt128(&innerSt)->fromString(
                &innerSt, 0, strVal.constData(),
                reinterpret_cast<FB_I128 *>(data));
            break;
        }
        case SQL_FLOAT:
            *reinterpret_cast<float *>(data) = static_cast<float>(val.toDouble());
            break;
        case SQL_DOUBLE:
            *reinterpret_cast<double *>(data) = val.toDouble();
            break;
        case SQL_DEC16: {
            ThrowStatusWrapper innerSt(status());
            decFloatFromString(master()->getUtilInterface()->getDecFloat16(&innerSt), innerSt,
                               numericInputString(val), reinterpret_cast<FB_DEC16 *>(data));
            break;
        }
        case SQL_DEC34: {
            ThrowStatusWrapper innerSt(status());
            decFloatFromString(master()->getUtilInterface()->getDecFloat34(&innerSt), innerSt,
                               numericInputString(val), reinterpret_cast<FB_DEC34 *>(data));
            break;
        }
        case SQL_BOOLEAN:
            *reinterpret_cast<FB_BOOLEAN *>(data) =
                val.toBool() ? FB_TRUE : FB_FALSE;
            break;
        case SQL_TYPE_DATE:
            *reinterpret_cast<ISC_DATE *>(data) =
                encodeQDate(master()->getUtilInterface(), val.toDate());
            break;
        case SQL_TYPE_TIME:
            *reinterpret_cast<ISC_TIME *>(data) =
                encodeQTime(master()->getUtilInterface(), val.toTime());
            break;
        case SQL_TIMESTAMP:
            *reinterpret_cast<ISC_TIMESTAMP *>(data) =
                encodeQDateTime(master()->getUtilInterface(), val.toDateTime());
            break;
        case SQL_TIMESTAMP_TZ:
            *reinterpret_cast<ISC_TIMESTAMP_TZ *>(data) =
                encodeQDateTimeTz(status(), master()->getUtilInterface(), val.toDateTime());
            break;
        case SQL_TIME_TZ:
            *reinterpret_cast<ISC_TIME_TZ *>(data) =
                encodeQTimeTz(status(), master()->getUtilInterface(), val.toDateTime());
            break;
        case SQL_TEXT:
        case SQL_VARYING:
            encodeTextValue(data, ci.fbType, ci.length, val.toString().toUtf8());
            break;
        case SQL_BLOB: {
            const QByteArray blobData = val.toByteArray();
            ISC_QUAD &blobId = *reinterpret_cast<ISC_QUAD *>(data);
            if (blobWriter) {
                /* Batch mode: delegate blob creation to the caller (IBatch::addBlob).
                   A failure must abort the statement, not silently bind SQL NULL. */
                if (!blobWriter(blobId, blobData)) {
                    errorText = u"Failed to write BLOB for parameter %1"_s.arg(i + 1);
                    return false;
                }
            } else {
                writeInlineBlob(blobId, blobData);
            }
            break;
        }
        case SQL_ARRAY: {
            if (val.typeId() != QMetaType::QVariantList) {
                errorText = u"Cannot bind non-list value to ARRAY parameter %1"_s.arg(i + 1);
                return false;
            }
            ISC_QUAD &arrId = *reinterpret_cast<ISC_QUAD *>(data);
            ITransaction *tr = ensureTransaction();
            if (!writeArray(att(), tr, status(), master(), arrayDescCache,
                            &arrId, ci.relation, ci.field, val.toList())) {
                errorText = u"Failed to write ARRAY parameter %1"_s.arg(i + 1);
                return false;
            }
            break;
        }
        default:
            break;
        }
    }
    return true;
}

/*! \internal
    After an EXECUTE PROCEDURE, copy the procedure's output columns back into the
    bound QSql::Out / QSql::InOut parameters, mapped positionally in declaration
    order (output column 0 -> first OUT param, and so on). The single-row result
    set remains available via value(); this just additionally lets callers read
    the outputs through QSqlQuery::boundValue(). Bind the OUT parameters after
    the procedure's IN arguments.
*/
void QFirebirdResult::writeOutValues()
{
    Q_D(QFirebirdResult);
    if (!d->isProcExec)
        return;
    const int columnCount = d->cols.size();
    const int boundCount = boundValueCount();
    int outCol = 0;
    for (int p = 0; p < boundCount && outCol < columnCount; ++p) {
        const QSql::ParamType type = bindValueType(p);
        if (type.testFlag(QSql::Out)) {
            bindValue(p, data(outCol), type);
            ++outCol;
        }
    }
}

bool QFirebirdResult::exec()
{
    Q_D(QFirebirdResult);

    /* Bare COMMIT/ROLLBACK: finish the active transaction via the API. A user
       transaction (iTrans) takes priority; otherwise the auto-transaction. If
       none is open (e.g. the previous statement already auto-committed) this is
       a no-op success. The handle is reset so the next statement starts fresh. */
    if (d->txnOp != TxnOp::None) {
        ITransaction *&iTrans = d->drv_d_func()->iTrans;
        ITransaction *tr = iTrans ? iTrans : d->autoTrans;
        if (tr) {
            try {
                ThrowStatusWrapper st(d->status());
                if (d->txnOp == TxnOp::Commit)
                    tr->commit(&st);
                else
                    tr->rollback(&st);
            } catch (const FbException &e) {
                const QSqlError err = fbError(d->master(), e.getStatus(), QSqlError::TransactionError);
                qCInfo(lcFirebird) << "exec(txn):" << fbErrorLog(err);
                setLastError(err);
                return false;
            }
            if (tr == iTrans)
                iTrans = nullptr;
            else
                d->autoTrans = nullptr;
        }
        setActive(true);
        setAt(QSql::AfterLastRow);
        return true;
    }

    if (!d->stmt) {
        setLastError(QSqlError(u"Statement not prepared"_s, QString(), QSqlError::StatementError));
        return false;
    }

    /* Reject over-binding: more input values bound than the statement has
       parameters. OUT-only parameters of a stored procedure are returned in the
       output message and do not consume an input slot, so exclude them from the
       count. (Under-binding is rejected in fillInputBuffer.) */
    {
        const qsizetype paramCount = d->inMeta ? d->inCols.size() : 0;
        int inputCount = 0;
        for (int p = 0, bc = boundValueCount(); p < bc; ++p) {
            if (bindValueType(p).testFlag(QSql::In))
                ++inputCount;
        }
        if (inputCount > paramCount) {
            setLastError(QSqlError(
                u"Parameter count mismatch: statement expects %1 input parameter(s), %2 bound"_s
                    .arg(paramCount).arg(inputCount),
                QString(), QSqlError::StatementError));
            return false;
        }
    }

    // Close any open cursor from a previous execution
    d->closeCursor();
    d->affectedRows = -1;

    try {
        ThrowStatusWrapper st(d->status());
        ITransaction *tr = d->ensureTransaction();
        QString bindError;
        if (!d->buildInputMessage(bindError)) {
            setLastError(QSqlError(bindError, QString(), QSqlError::StatementError));
            return false;
        }

        if (d->isSelect) {
            d->cursor = d->stmt->openCursor(&st, tr,
                                            d->inMeta,
                                            d->inBuffer.isEmpty() ? nullptr : d->inBuffer.data(),
                                            d->outMeta,
                                            isForwardOnly() ? 0 : IStatement::CURSOR_TYPE_SCROLLABLE);
            // Opt-in client-side caching applies only to scrollable SELECTs.
            d->rowCache.clear();
            d->cacheComplete = false;
            d->useRowCache = d->drv_d_func()->cacheScrollableResults && !isForwardOnly();
            setAt(QSql::BeforeFirstRow);
            setActive(true);
        } else {
            d->isProcExec = false;
            d->procRowFetched = false;
            /* Zero the output buffer so stale null indicators from a previous
               execution (e.g. a prior call that returned NULL for a field) don't
               bleed through when Firebird only writes the value and not the flag. */
            if (!d->outBuffer.isEmpty())
                d->outBuffer.fill(0);
            ITransaction *newTr = d->stmt->execute(&st, tr,
                                                   d->inMeta,
                                                   d->inBuffer.isEmpty() ? nullptr : d->inBuffer.data(),
                                                   d->outMeta,
                                                   d->outBuffer.isEmpty() ? nullptr : d->outBuffer.data());
            /* execute() may return a transaction that replaces the one we ran
               under; adopt it, releasing the replaced handle exactly once. */
            d->adoptReplacementTransaction(tr, newTr);
            d->affectedRows = QFirebirdResultPrivate::AffectedPending;
            if (d->outMeta) {
                /* EXECUTE PROCEDURE output: expose the filled buffer as a single
                   row (read via value()) and also copy it back to any bound
                   QSql::Out/InOut parameters (read via boundValue()). */
                d->isProcExec = true;
                setAt(QSql::BeforeFirstRow);
                writeOutValues();
            } else {
                // Auto-commit non-SELECT statements that used an auto transaction
                if (d->autoTrans && !d->drv_d_func()->iTrans)
                    finishAndClear(st, d->autoTrans, TxnEnd::Commit);
                setAt(QSql::AfterLastRow);
            }
            setActive(true);
        }
    } catch (const FbException &e) {
        const QSqlError err = fbError(d->master(), e.getStatus(), QSqlError::StatementError);
        qCInfo(lcFirebird) << "exec:" << fbErrorLog(err);
        setLastError(err);
        return false;
    }
    return true;
}

bool QFirebirdResult::execBatch(bool arrayBind)
{
    Q_D(QFirebirdResult);

    if (!d->stmt || !d->inMeta) {
        // No prepared statement or no input parameters — fall back to default loop
        return QSqlResult::execBatch(arrayBind);
    }

    const QList<QVariant> &batchValues = d->values;
    if (batchValues.isEmpty()) {
        setLastError(QSqlError(u"No values bound for batch execution"_s,
                               QString(), QSqlError::StatementError));
        return false;
    }

    const qsizetype paramCount = batchValues.size();
    const QVariantList firstList = batchValues.at(0).toList();
    const qsizetype batchCount = firstList.size();
    if (batchCount == 0)
        return true;

    // Close any open cursor from a previous execution
    d->closeCursor();
    d->affectedRows = -1;

    IUtil *utl = d->master()->getUtilInterface();

    try {
        ThrowStatusWrapper st(d->status());
        ITransaction *tr = d->ensureTransaction();

        /* Build batch parameters block. The builder, batch and completion-state
           handles are freed by FbGuard on every path (including the FbException
           catch below), so no manual unwinding is needed. */
        FbGuard<IXpbBuilder> pb(utl->getXpbBuilder(&st, IXpbBuilder::BATCH, nullptr, 0),
                                fbDispose<IXpbBuilder>);
        pb->insertInt(&st, IBatch::TAG_RECORD_COUNTS, 1);

        // Check if any parameter is a BLOB — enable inline blob IDs
        bool hasBlobs = false;
        for (const ColumnInfo &ci : std::as_const(d->inCols)) {
            if (fbBaseType(ci.fbType) == SQL_BLOB) {
                hasBlobs = true;
                break;
            }
        }
        if (hasBlobs)
            pb->insertInt(&st, IBatch::TAG_BLOB_POLICY, IBatch::BLOB_ID_ENGINE);

        /* Create batch from the prepared statement (pb is disposed by its guard
           on scope exit; its buffer was already consumed by createBatch). */
        FbGuard<IBatch> batch(d->stmt->createBatch(&st, d->inMeta,
                                                   pb->getBufferLength(&st), pb->getBuffer(&st)),
                              fbRelease<IBatch>);

        /* Extract each parameter's value list once before the row loop —
           toList() per (row, param) would convert the same QVariant for every
           row of the batch. */
        QVarLengthArray<QVariantList, 16> paramLists;
        paramLists.reserve(paramCount);
        for (qsizetype p = 0; p < paramCount; ++p)
            paramLists.append(batchValues.at(p).toList());

        for (qsizetype row = 0; row < batchCount; ++row) {
            QList<QVariant> rowValues;
            rowValues.reserve(paramCount);
            for (qsizetype p = 0; p < paramCount; ++p) {
                const QVariantList &col = paramLists[p];
                rowValues.append(row < col.size() ? col.at(row) : QVariant());
            }

            d->inBuffer.fill(0);

            // BlobWriter lambda for batch mode: uses IBatch::addBlob
            auto blobWriter = [&](ISC_QUAD &blobId, const QByteArray &data) -> bool {
                try {
                    batch->addBlob(&st, static_cast<unsigned>(data.size()),
                                   data.constData(), &blobId, 0, nullptr);
                    return true;
                } catch (const FbException &e) {
                    const auto berr = fbError(d->master(), e.getStatus(), QSqlError::StatementError);
                    qCWarning(lcFirebird) << "batch addBlob:" << fbErrorLog(berr);
                    return false;
                }
            };

            QString bindError;
            if (!d->fillInputBuffer(rowValues, bindError,
                                    hasBlobs ? blobWriter : QFirebirdResultPrivate::BlobWriter{})) {
                setLastError(QSqlError(bindError, QString(), QSqlError::StatementError));
                return false; // batch guard releases the handle
            }

            batch->add(&st, 1, d->inBuffer.data());
        }

        FbGuard<IBatchCompletionState> cs(batch->execute(&st, tr),
                                          fbDispose<IBatchCompletionState>);

        unsigned total = cs->getSize(&st);
        int totalAffected = 0;
        bool hadError = false;
        for (unsigned p = 0; p < total; ++p) {
            int state = cs->getState(&st, p);
            if (state == IBatchCompletionState::EXECUTE_FAILED) {
                hadError = true;
            } else if (state != IBatchCompletionState::SUCCESS_NO_INFO) {
                totalAffected += state;
            } else {
                // SUCCESS_NO_INFO: count as 1 affected row
                totalAffected += 1;
            }
        }
        d->affectedRows = totalAffected;

        if (hadError) {
            unsigned errPos = cs->findError(&st, 0);
            if (errPos != IBatchCompletionState::NO_MORE_ERRORS) {
                IStatus *errStatus = d->master()->getStatus();
                try {
                    cs->getStatus(&st, errStatus, errPos);
                    setLastError(fbError(d->master(), errStatus, QSqlError::StatementError));
                } catch (...) {
                    setLastError(QSqlError(u"Batch execution failed at message %1"_s
                                               .arg(errPos),
                                           QString(), QSqlError::StatementError));
                }
                errStatus->dispose();
            } else {
                /* EXECUTE_FAILED was reported but findError returned no
                   position — should not happen, but never fail silently. */
                setLastError(QSqlError(u"Batch execution failed"_s,
                                       QString(), QSqlError::StatementError));
            }
        }

        /* Close the batch gracefully (its guard is dismissed on success, releases
           on a failed close); the completion state is disposed by its guard on
           scope exit. */
        batch.closeWith([&](IBatch *b) { b->close(&st); });

        // Auto-commit if using an auto-transaction
        if (d->autoTrans && !d->drv_d_func()->iTrans)
            finishAndClear(st, d->autoTrans, TxnEnd::Commit);

        setAt(QSql::AfterLastRow);
        setActive(true);
        return !hadError;

    } catch (const FbException &e) {
        const QSqlError err = fbError(d->master(), e.getStatus(), QSqlError::StatementError);
        qCInfo(lcFirebird) << "execBatch:" << fbErrorLog(err);
        setLastError(err);
        return false;
    }
}

bool QFirebirdResult::reset(const QString &query)
{
    if (!prepare(query))
        return false;
    return exec();
}

// ---------- Fetch helpers ----------

/*! \internal
    Largest non-negative row-position sentinel. Two uses, same value:
     - passed to ensureCachedRow() to force the cache to pull every remaining row;
     - the at() position set after fetchLast() on a server-side scrollable cursor,
       which exposes no row count so the true last index is unknown (QSqlQuery
       only needs a valid non-negative position; the row data comes from outBuffer).
*/
static constexpr int kMaxRowSentinel = 0x7FFFFFFE;

bool QFirebirdResult::fetchCached(int target)
{
    Q_D(QFirebirdResult);
    if (target < 0)
        return false;
    try {
        if (d->ensureCachedRow(target)) {
            /* Restore the snapshot so data()/isNull() read it unchanged. QByteArray
               assignment is a cheap COW share, not a copy. */
            d->outBuffer = d->rowCache.at(target);
            setAt(target);
            return true;
        }
    } catch (const FbException &e) {
        setLastError(fbError(d->master(), e.getStatus(), QSqlError::StatementError));
    }
    return false;
}

bool QFirebirdResult::fetchNext()
{
    Q_D(QFirebirdResult);
    if (d->isProcExec) {
        if (d->procRowFetched)
            return false;
        d->procRowFetched = true;
        setAt(0);
        return true;
    }
    if (!d->cursor)
        return false;
    if (d->useRowCache)
        return fetchCached(at() == QSql::BeforeFirstRow ? 0 : at() + 1);
    try {
        ThrowStatusWrapper st(d->status());
        int code = d->cursor->fetchNext(&st, d->outBuffer.data());
        if (code == IStatus::RESULT_OK) {
            setAt(at() == QSql::BeforeFirstRow ? 0 : at() + 1);
            return true;
        }
    } catch (const FbException &e) {
        setLastError(fbError(d->master(), e.getStatus(), QSqlError::StatementError));
    }
    return false;
}

bool QFirebirdResult::fetchFirst()
{
    Q_D(QFirebirdResult);
    if (d->isProcExec) {
        if (d->procRowFetched)
            return false;
        d->procRowFetched = true;
        setAt(0);
        return true;
    }
    if (!d->cursor)
        return false;
    if (d->useRowCache)
        return fetchCached(0);
    try {
        ThrowStatusWrapper st(d->status());
        int code;
        if (isForwardOnly())
            code = d->cursor->fetchNext(&st, d->outBuffer.data());
        else
            code = d->cursor->fetchFirst(&st, d->outBuffer.data());
        if (code == IStatus::RESULT_OK) {
            setAt(0);
            return true;
        }
    } catch (const FbException &e) {
        setLastError(fbError(d->master(), e.getStatus(), QSqlError::StatementError));
    }
    return false;
}

bool QFirebirdResult::fetchLast()
{
    Q_D(QFirebirdResult);
    if (d->isProcExec) {
        // EXECUTE PROCEDURE exposes a single output row at index 0.
        d->procRowFetched = true;
        setAt(0);
        return true;
    }
    if (!d->cursor)
        return false;
    if (d->useRowCache) {
        /* Pull the whole set, then position on the real last index. Unlike the
           server-cursor path below, cache mode therefore gives last() a genuine
           absolute at() (and lets size() report a row count). */
        try {
            d->ensureCachedRow(kMaxRowSentinel);
        } catch (const FbException &e) {
            setLastError(fbError(d->master(), e.getStatus(), QSqlError::StatementError));
            return false;
        }
        if (d->rowCache.isEmpty())
            return false;
        return fetchCached(int(d->rowCache.size()) - 1);
    }
    if (isForwardOnly()) {
        /* Forward-only cursors cannot jump to the end, so walk there with
           fetchNext(): the last successful fetch leaves the final row in the
           buffer with at() at its index. Mirrors QSqlCachedResult. */
        if (at() == QSql::AfterLastRow)
            return false;
        if (!fetchNext())
            return false; // empty result set (or already exhausted)
        while (fetchNext())
            ;
        return true;
    }
    try {
        ThrowStatusWrapper st(d->status());
        int code = d->cursor->fetchLast(&st, d->outBuffer.data());
        if (code == IStatus::RESULT_OK) {
            /* The last row's absolute index is unknown (see kMaxRowSentinel), so
               use the sentinel as the position: QSqlQuery::value() treats it as
               valid and the row data (read from outBuffer) is correct. at() is
               therefore NOT a meaningful absolute index after last(); relative
               navigation from here (previous()) still returns correct row data
               but at() stays sentinel-relative. Use forward iteration from
               first()/next() if a true row index is required. */
            setAt(kMaxRowSentinel);
            return true;
        }
    } catch (const FbException &e) {
        setLastError(fbError(d->master(), e.getStatus(), QSqlError::StatementError));
    }
    return false;
}

bool QFirebirdResult::fetchPrevious()
{
    Q_D(QFirebirdResult);
    if (d->isProcExec)
        return false; // single-row proc result: nothing precedes the one row
    if (!d->cursor)
        return false;
    if (d->useRowCache) {
        if (at() <= 0) {
            setAt(QSql::BeforeFirstRow);
            return false;
        }
        return fetchCached(at() - 1);
    }
    if (isForwardOnly())
        return false; // cannot move backwards on a forward-only cursor
    try {
        ThrowStatusWrapper st(d->status());
        int code = d->cursor->fetchPrior(&st, d->outBuffer.data());
        if (code == IStatus::RESULT_OK) {
            /* The cursor moves correctly and the row data is valid; at() is
               decremented from the previous position. After last() the previous
               position is the sentinel (see fetchLast), so at() stays
               sentinel-relative rather than a true absolute index. */
            setAt(at() > 0 ? at() - 1 : QSql::BeforeFirstRow);
            return true;
        }
    } catch (const FbException &e) {
        setLastError(fbError(d->master(), e.getStatus(), QSqlError::StatementError));
    }
    return false;
}

bool QFirebirdResult::fetch(int i)
{
    Q_D(QFirebirdResult);
    if (i < 0)
        return false;
    if (d->isProcExec) {
        /* EXECUTE PROCEDURE exposes a single output row at index 0, so only
           seek(0) is valid — this makes QSqlQuery::seek()/first() work on
           procedure results, not just next(). */
        if (i != 0)
            return false;
        d->procRowFetched = true;
        setAt(0);
        return true;
    }
    if (!d->cursor)
        return false;
    if (d->useRowCache)
        return fetchCached(i);
    if (isForwardOnly()) {
        /* Forward-only cursors have no random access, but absolute *forward*
           seeks can be emulated by iterating with fetchNext(). Backward seeks
           are rejected by QSqlQuery before reaching here. Mirrors the
           forward-only behaviour of QSqlCachedResult. */
        if (i < at())
            return false;
        while (at() < i) {
            if (!fetchNext())
                return false;
        }
        return true;
    }
    // IResultSet supports absolute positioning
    try {
        ThrowStatusWrapper st(d->status());
        int code = d->cursor->fetchAbsolute(&st, i + 1, d->outBuffer.data());
        if (code == IStatus::RESULT_OK) {
            setAt(i);
            return true;
        }
    } catch (const FbException &e) {
        setLastError(fbError(d->master(), e.getStatus(), QSqlError::StatementError));
    }
    return false;
}

// ---------- Data access ----------

bool QFirebirdResult::isNull(int field)
{
    Q_D(const QFirebirdResult);
    if (field < 0 || field >= d->cols.size())
        return true;
    const ColumnInfo &ci = d->cols.at(field);
    const short *nullFlag =
        reinterpret_cast<const short *>(d->outBuffer.constData() + ci.nullOffset);
    qCDebug(lcFirebird, "isNull(%d): nullOffset=%u nullFlag=%d", field, ci.nullOffset, int(*nullFlag));
    return *nullFlag != 0;
}

QVariant QFirebirdResult::data(int field)
{
    Q_D(const QFirebirdResult);

    if (field < 0 || field >= d->cols.size()) {
        qCDebug(lcFirebird, "data(%d): out of range, cols.size()=%d", field, int(d->cols.size()));
        return {};
    }

    const ColumnInfo &ci = d->cols.at(field);

    /* SQL NULL: return a valid but null QVariant of the correct type.
       Returning {} (invalid QVariant) would violate Qt SQL conventions and
       break code that checks QVariant::isValid() to detect present-but-null values. */
    if (isNull(field)) {
        qCDebug(lcFirebird, "data(%d): SQL NULL, fbType=%d qtType=%d", field, ci.fbType, int(ci.qtType));
        return QVariant(QMetaType(ci.qtType));
    }

    qCDebug(lcFirebird, "data(%d): fbType=%d qtType=%d", field, ci.fbType, int(ci.qtType));
    const char *ptr = d->outBuffer.constData() + ci.offset;

    /* The Firebird OO-API conversions below (IInt128/IDecFloat, BLOB and array
       reads) use ThrowStatusWrapper, so any conversion or fetch failure raises
       FbException. Catch it here and surface a QSqlError instead of returning a
       silently corrupt or truncated value. */
    try {
        // Scaled numerics — honour numericalPrecisionPolicy (any non-zero scale)
        if (ci.scale != 0) {
            const auto policy = numericalPrecisionPolicy();
            switch (fbBaseType(ci.fbType)) {
            case SQL_SHORT:   return applyScale(*reinterpret_cast<const qint16 *>(ptr), ci.scale, policy);
            case SQL_LONG:    return applyScale(*reinterpret_cast<const qint32 *>(ptr), ci.scale, policy);
            case SQL_INT128: {
                // Use Firebird's IInt128 interface to format, then apply policy
                ThrowStatusWrapper st(d->status());
                const QString str = int128ToString(d->master()->getUtilInterface(), st,
                                                    reinterpret_cast<const FB_I128 *>(ptr),
                                                    ci.scale);
                if (policy == QSql::HighPrecision)
                    return str;
                if (policy == QSql::LowPrecisionDouble)
                    return QVariant(str.toDouble());
                /* LowPrecisionInt32/Int64: take the integer part straight from
                   the decimal string (truncating toward zero) rather than via
                   double, so 128-bit values keep full integer precision up to
                   the target type's range — a double round-trip would drop bits
                   above 2^53. */
                const qsizetype dot = str.indexOf(u'.');
                const QStringView intPart =
                    dot < 0 ? QStringView(str) : QStringView(str).first(dot);
                if (policy == QSql::LowPrecisionInt32)
                    return QVariant(intPart.toInt());
                return QVariant(intPart.toLongLong());
            }
            default:          return applyScale(*reinterpret_cast<const qint64 *>(ptr), ci.scale, policy);
            }
        }

        switch (fbBaseType(ci.fbType)) {
        case SQL_SHORT:
            return static_cast<int>(*reinterpret_cast<const qint16 *>(ptr));
        case SQL_LONG:
            return static_cast<int>(*reinterpret_cast<const qint32 *>(ptr));
        case SQL_INT64:
            return *reinterpret_cast<const qint64 *>(ptr);
        case SQL_INT128: {
            // 128-bit integer: use Firebird's IInt128 to convert to decimal string
            ThrowStatusWrapper st(d->status());
            return int128ToString(d->master()->getUtilInterface(), st,
                                  reinterpret_cast<const FB_I128 *>(ptr), 0);
        }
        case SQL_FLOAT:
            return static_cast<double>(*reinterpret_cast<const float *>(ptr));
        case SQL_DOUBLE:
            return *reinterpret_cast<const double *>(ptr);
        case SQL_DEC16: {
            ThrowStatusWrapper st(d->status());
            return decFloatToString(d->master()->getUtilInterface()->getDecFloat16(&st), st,
                                    reinterpret_cast<const FB_DEC16 *>(ptr));
        }
        case SQL_DEC34: {
            ThrowStatusWrapper st(d->status());
            return decFloatToString(d->master()->getUtilInterface()->getDecFloat34(&st), st,
                                    reinterpret_cast<const FB_DEC34 *>(ptr));
        }
        case SQL_BOOLEAN:
            return *reinterpret_cast<const FB_BOOLEAN *>(ptr) != FB_FALSE;
        case SQL_TYPE_DATE:
            return decodeFirebirdDate(d->master()->getUtilInterface(),
                                      *reinterpret_cast<const ISC_DATE *>(ptr));
        case SQL_TYPE_TIME:
            return decodeFirebirdTime(d->master()->getUtilInterface(),
                                      *reinterpret_cast<const ISC_TIME *>(ptr));
        case SQL_TIMESTAMP:
            return decodeFirebirdTimestamp(d->master()->getUtilInterface(),
                                          *reinterpret_cast<const ISC_TIMESTAMP *>(ptr));
        case SQL_TIMESTAMP_TZ:
            return decodeFirebirdTimestampTz(d->status(), d->master()->getUtilInterface(),
                                             *reinterpret_cast<const ISC_TIMESTAMP_TZ *>(ptr));
        case SQL_TIME_TZ:
            return decodeFirebirdTimeTz(d->status(), d->master()->getUtilInterface(),
                                        *reinterpret_cast<const ISC_TIME_TZ *>(ptr));
        case SQL_BLOB: {
            const ISC_QUAD &blobId = *reinterpret_cast<const ISC_QUAD *>(ptr);
            QByteArray ba = readBlob(d->att(), d->activeTransaction(),
                                     d->status(), blobId);
            // TEXT blob (sub_type 1) → return as QString
            if (ci.fbSubType == 1)
                return QString::fromUtf8(ba);
            return ba;
        }
        case SQL_ARRAY: {
            const ISC_QUAD &arrayId = *reinterpret_cast<const ISC_QUAD *>(ptr);
            ITransaction *tra = d->activeTransaction();
            return fetchArray(d->att(), tra, d->status(), d->master(),
                              d->arrayDescCache, arrayId, ci.relation, ci.field);
        }
        case SQL_VARYING: {
            /* First 2 bytes are the length. Clamp it to the field's data
               capacity (byte length minus the 2-byte prefix) so a corrupt or
               oversized length indicator cannot read past the row buffer. */
            const unsigned short len = *reinterpret_cast<const unsigned short *>(ptr);
            const qsizetype maxLen =
                std::max(qsizetype(0), static_cast<qsizetype>(ci.length) - 2);
            return QString::fromUtf8(ptr + 2, std::min(static_cast<qsizetype>(len), maxLen));
        }
        case SQL_TEXT: {
            /* ci.length is the byte length cached from the output metadata in
               buildColumns(); reuse it instead of a per-row getLength() call. */
            QString s = QString::fromUtf8(ptr, static_cast<qsizetype>(ci.length));
            while (s.endsWith(u' '))
                s.chop(1);
            return s;
        }
        default:
            qCWarning(lcFirebird, "data: unhandled Firebird type %d for field %d", ci.fbType, field);
            return QString::fromUtf8(ptr);
        }
    } catch (const FbException &e) {
        setLastError(fbError(d->master(), e.getStatus(), QSqlError::StatementError));
        return {};
    }
}

QSqlRecord QFirebirdResult::record() const
{
    Q_D(const QFirebirdResult);
    if (d->recordCached)
        return d->cachedRecord;
    QSqlRecord rec;
    for (const ColumnInfo &ci : d->cols) {
        /* Pass the source relation as the field's table name so table-qualified
           lookups (QSqlRecord::indexOf("TABLE.COLUMN")) can disambiguate columns
           that share a name across joined tables — matching the QIBASE driver. */
        QSqlField f(ci.name, QMetaType(ci.qtType), ci.relation);
        const int base = fbBaseType(ci.fbType);
        if (base == SQL_TEXT || base == SQL_VARYING) {
            /* ci.length is the UTF-8 byte length (the connection charset is
               UTF-8); report the declared character length instead. Integer
               division also absorbs the 2-byte VARYING prefix. */
            f.setLength(static_cast<int>(ci.length) / 4);
        } else {
            f.setLength(static_cast<int>(ci.length));
        }
        /* Precision (fractional digits) only applies to scaled numeric types;
           leave it unset (-1) for everything else. */
        if (ci.scale != 0)
            f.setPrecision(qAbs(ci.scale));
        f.setRequiredStatus(ci.nullable ? QSqlField::Optional : QSqlField::Required);
        rec.append(f);
    }
    d->cachedRecord = rec;
    d->recordCached = true;
    return rec;
}

int QFirebirdResult::size()
{
    Q_D(QFirebirdResult);
    /* The row count is only available in cache mode (FB_SCROLLABLE_CACHE=1),
       where the result buffers its rows client-side anyway: pull the remaining
       rows into the cache and report its size. Firebird cursors cannot report
       cardinality without fetching, so every other path returns -1. */
    if (!d->useRowCache)
        return -1;
    if (!d->cacheComplete) {
        if (!d->cursor) // e.g. after finish() released the cursor
            return -1;
        try {
            d->ensureCachedRow((std::numeric_limits<int>::max)());
        } catch (const FbException &e) {
            const QSqlError err = fbError(d->master(), e.getStatus(), QSqlError::StatementError);
            qCInfo(lcFirebird) << "size:" << fbErrorLog(err);
            setLastError(err);
            return -1;
        }
    }
    return int(d->rowCache.size());
}

int QFirebirdResult::numRowsAffected()
{
    Q_D(QFirebirdResult);
    if (d->affectedRows == QFirebirdResultPrivate::AffectedPending) {
        d->affectedRows = -1;
        if (d->stmt) {
            try {
                ThrowStatusWrapper st(d->status());
                d->affectedRows = static_cast<int>(d->stmt->getAffectedRecords(&st));
            } catch (const FbException &e) {
                qCInfo(lcFirebird) << "numRowsAffected:"
                                   << fbErrorLog(fbError(d->master(), e.getStatus(),
                                                         QSqlError::StatementError));
                d->status()->init();
            }
        }
    }
    return d->affectedRows;
}

QT_END_NAMESPACE
