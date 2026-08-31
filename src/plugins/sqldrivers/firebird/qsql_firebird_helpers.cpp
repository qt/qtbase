// Copyright (C) 2026 The Qt Company Ltd.
// Copyright (C) 2026 Andreas Bacher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#include "qsql_firebird_helpers_p.h"

#include <QtCore/qtimezone.h>

#include <firebird/impl/blr.h>   // BLR type constants for array element types

#include <algorithm>
#include <cstring>
#include <limits>
#include <iterator>

QT_BEGIN_NAMESPACE

// Firebird OO API types are used throughout; avoid full Firebird:: qualification.
using namespace Firebird;

Q_LOGGING_CATEGORY(lcFirebird, "qt.sql.firebird")

using namespace Qt::StringLiterals;

//  Helper: convert Firebird status to a QSqlError string

QString fbErrorString(IMaster *master, IStatus *status)
{
    IUtil *util = master->getUtilInterface();
    char buf[1024];
    util->formatStatus(buf, sizeof(buf), status);
    return QString::fromUtf8(buf);
}

/*! \internal
    Convert a Firebird status vector into a QSqlError. `type` is the context in
    which the error occurred, supplied by the caller (open() -> ConnectionError,
    prepare()/exec() -> StatementError, begin/commit/rollback ->
    TransactionError); QSqlError::ErrorType categorises errors by that context,
    not by the underlying Firebird cause. The SQLCODE is carried through as the
    native error code.
*/
QSqlError fbError(IMaster *master, IStatus *status, QSqlError::ErrorType type)
{
    const QString msg = fbErrorString(master, status);
    const ISC_LONG sqlcode = isc_sqlcode(status->getErrors());
    const QString code = (sqlcode != -1 && sqlcode != 0)
        ? QString::number(sqlcode) : QString();

    return QSqlError({}, msg, type, code);
}

QString fbErrorLog(const QSqlError &err)
{
    static const char *typeNames[] = {
        "NoError", "ConnectionError", "StatementError", "TransactionError", "UnknownError"
    };
    const int ti = std::clamp(static_cast<int>(err.type()), 0,
                              static_cast<int>(std::size(typeNames)) - 1);
    const QString code = err.nativeErrorCode();
    const QString text = err.databaseText().isEmpty() ? err.driverText() : err.databaseText();
    return QString::fromLatin1("[%1, %2] %3").arg(QLatin1StringView(typeNames[ti]), code, text);
}

/*! \internal
    Commit (or roll back) a transaction whose handle must not outlive a failure:
    on a throw the interface is released and the member cleared before the
    rethrow, on success commit/rollback already released it, so the member is
    cleared too. Taking the handle by reference makes "released implies cleared"
    structural — every finish site shares this discipline instead of repeating
    it. (The bare-COMMIT path in exec() intentionally does NOT use this: a
    failed user COMMIT keeps the still-valid handle so it can be retried.)
*/
void finishAndClear(Firebird::ThrowStatusWrapper &st,
                    Firebird::ITransaction *&tr, TxnEnd end)
{
    try {
        if (end == TxnEnd::Commit)
            tr->commit(&st);
        else
            tr->rollback(&st);
    } catch (...) {
        tr->release();
        tr = nullptr;
        throw;
    }
    tr = nullptr;
}

/*! \internal
    Start a transaction with the driver's default TPB: read-committed,
    record-version, read-write. wait selects isc_tpb_wait vs isc_tpb_nowait —
    the only axis on which the auto-started transaction (wait) and the explicit
    user transaction (nowait) differ.
*/
ITransaction *startDefaultTransaction(IAttachment *att, ThrowStatusWrapper &st,
                                      TxnWait wait)
{
    const unsigned char tpb[] = {
        isc_tpb_version3, isc_tpb_write, isc_tpb_read_committed, isc_tpb_rec_version,
        static_cast<unsigned char>(wait == TxnWait::Wait ? isc_tpb_wait : isc_tpb_nowait)
    };
    return att->startTransaction(&st, sizeof(tpb), tpb);
}

// Map BLR element type (from ISC_ARRAY_DESC) to QMetaType
QMetaType::Type blrTypeToQt(unsigned char blrType, bool hasScale)
{
    switch (blrType) {
    case blr_text:
    case blr_varying:
    case blr_cstring:      return QMetaType::QString;
    case blr_short:
    case blr_long:         return hasScale ? QMetaType::Double : QMetaType::Int;
    case blr_quad:
    case blr_int64:        return hasScale ? QMetaType::Double : QMetaType::LongLong;
    case blr_float:
    case blr_double:
    case blr_d_float:
    case blr_dec64:
    case blr_dec128:       return QMetaType::Double;
    case blr_int128:       return QMetaType::QString;
    case blr_sql_date:     return QMetaType::QDate;
    case blr_sql_time:     return QMetaType::QTime;
    case blr_sql_time_tz:  return QMetaType::QDateTime;
    case blr_timestamp:
    case blr_timestamp_tz: return QMetaType::QDateTime;
    case blr_bool:         return QMetaType::Bool;
    /* Note: no blr_blob case. blr_blob (261) exceeds unsigned char range and
       Firebird has no array-of-BLOB type, so the element dtype is never a blob. */
    default:               return QMetaType::QString;
    }
}

//  Numeric codecs

QMetaType::Type fbTypeToQt(int fbType, int scale)
{
    if (scale != 0) {
        /* Scaled NUMERIC/DECIMAL. INT128-backed values cannot be faithfully
           represented as Double, so report them as String. */
        if (fbBaseType(fbType) == SQL_INT128)
            return QMetaType::QString;
        return QMetaType::Double;
    }
    switch (fbBaseType(fbType)) {
    case SQL_SHORT:
    case SQL_LONG:       return QMetaType::Int;
    case SQL_INT64:      return QMetaType::LongLong;
    case SQL_INT128:     return QMetaType::QString; // 128-bit; QVariant has no such type — exact decimal string
    case SQL_FLOAT:      return QMetaType::Double;
    case SQL_DOUBLE:     return QMetaType::Double;
    case SQL_DEC16:
    case SQL_DEC34:      return QMetaType::QString; // DECFLOAT; no native Qt type
    case SQL_BOOLEAN:    return QMetaType::Bool;
    case SQL_TYPE_DATE:  return QMetaType::QDate;
    case SQL_TYPE_TIME:  return QMetaType::QTime;
    case SQL_TIMESTAMP:
    case SQL_TIMESTAMP_TZ:
    case SQL_TIME_TZ:    return QMetaType::QDateTime;
    case SQL_BLOB:       return QMetaType::QByteArray;
    case SQL_ARRAY:      return QMetaType::QVariantList;
    case SQL_TEXT:
    case SQL_VARYING:    return QMetaType::QString;
    default:
        qCWarning(lcFirebird, "fbTypeToQt: unknown Firebird type %d", fbType);
        return QMetaType::QString;
    }
}

//  INT128 codec — see the header for the design

#ifndef QSQL_HAS_INT128

// Firebird's IInt128 converts via a caller buffer; 64 chars covers INT128's 39 digits.
static QString int128SlotToString(const char *slot, int scale, IMaster *master, IStatus *status)
{
    ThrowStatusWrapper st(status);
    char buf[64];
    master->getUtilInterface()->getInt128(&st)->toString(
        &st, reinterpret_cast<const FB_I128 *>(slot), scale, sizeof(buf), buf);
    return QString::fromLatin1(buf);
}

#endif // QSQL_HAS_INT128

bool encodeInt128Slot(const QVariant &val, int scale, char *slot,
                      IMaster *master, IStatus *status, QString &errorText)
{
#ifdef QSQL_HAS_INT128
    Q_UNUSED(master);
    Q_UNUSED(status);
    qinternalint128 v = 0;
    if (!QtPrivate::sqlScaledFromVariant(val, scale, &v, &errorText))
        return false;
    /* FB_I128 is two 64-bit words, fb_data[0] low and fb_data[1] high, at
       8-byte alignment in the message buffer: copy the words, never dereference
       a 16-byte-aligned type in place. */
    static_assert(std::is_same_v<ISC_UINT64, quint64>,
                  "the FB_I128 words are written through quint64 pointers");
    FB_I128 raw;
    QtPrivate::sqlInt128ToWords(v, &raw.fb_data[0], &raw.fb_data[1]);
    std::memcpy(slot, &raw, sizeof(raw));
    return true;
#else
    /* No 128-bit integer type: let IInt128::fromString scale the decimal
       string. It would also accept exponent notation, so pre-validate with
       the exact codec's grammar (the same canonical text, so a Bool is "1" here
       too), and report its out-of-range exception with the exact codec's
       wording - "out of range" for an integral input, "cannot encode" for text.
       Both builds then accept the same values and fail with the same message. */
    const QByteArray strVal = QtPrivate::sqlNumericInputString(val);
    if (!QtPrivate::sqlIsPlainDecimal(strVal)) {
        errorText = QtPrivate::sqlCannotEncodeError(strVal, scale);
        return false;
    }
    try {
        ThrowStatusWrapper st(status);
        master->getUtilInterface()->getInt128(&st)->fromString(
            &st, scale, strVal.constData(), reinterpret_cast<FB_I128 *>(slot));
        return true;
    } catch (const FbException &) {
        status->init();
        errorText = QtPrivate::sqlIsIntegralVariant(val.typeId())
                    ? QtPrivate::sqlOutOfRangeError(val, scale)
                    : QtPrivate::sqlCannotEncodeError(strVal, scale);
        return false;
    }
#endif
}

QVariant decodeInt128Slot(const char *slot, int scale, QSql::NumericalPrecisionPolicy policy,
                          IMaster *master, IStatus *status, QString *overflowText)
{
#ifdef QSQL_HAS_INT128
    Q_UNUSED(master);
    Q_UNUSED(status);
    // Copy the FB_I128 words out of the slot (see encodeInt128Slot for the layout)
    FB_I128 raw;
    std::memcpy(&raw, slot, sizeof(raw));
    const qinternalint128 value = QtPrivate::sqlInt128FromWords(raw.fb_data[0], raw.fb_data[1]);
    bool overflow = false;
    const QVariant result = QtPrivate::sqlApplyScale(value, scale, policy, &overflow);
    // A double beats a wrapped or zero value; the caller decides when to say so.
    if (overflow && overflowText)
        *overflowText = QtPrivate::sqlScaledToString(value, scale);
    return result;
#else
    const QString str = int128SlotToString(slot, scale, master, status);
    switch (policy) {
    case QSql::HighPrecision:
        return str;
    case QSql::LowPrecisionDouble:
        return QVariant(str.toDouble());
    case QSql::LowPrecisionInt32:
    case QSql::LowPrecisionInt64: {
        /* Take the integer part straight from the decimal string (truncating
           toward zero) rather than via double, so 128-bit values keep full
           integer precision up to the target type's range. */
        const qsizetype dot = str.indexOf(u'.');
        const QStringView intPart = dot < 0 ? QStringView(str) : QStringView(str).first(dot);
        bool ok = false;
        if (policy == QSql::LowPrecisionInt32) {
            const int v = intPart.toInt(&ok);
            if (ok)
                return QVariant(v);
        } else {
            const qint64 v = intPart.toLongLong(&ok);
            if (ok)
                return QVariant(v);
        }
        // Same fallback as the native codec: a double beats a wrapped or zero value.
        if (overflowText)
            *overflowText = str;
        return QVariant(str.toDouble());
    }
    }
    return QVariant(str.toDouble());
#endif
}

// ---- Date/time encode helpers ----

ISC_DATE encodeQDate(IUtil *util, const QDate &d)
{
    /* An invalid date reaches here when a non-date value is bound to a DATE
       parameter — most often a date string in a format QVariant can't parse
       (e.g. "25.05.2020"; QVariant only parses ISO). encodeDate(0,0,0) would
       produce an out-of-range value the engine rejects ("value exceeds the
       range for valid dates"). The legacy QIBASE driver instead maps an invalid
       QDate to ISC_DATE 0 (the 1858-11-17 epoch) and stores it without error;
       match that so such statements succeed identically. */
    if (!d.isValid())
        return 0;
    return util->encodeDate(static_cast<unsigned>(d.year()),
                            static_cast<unsigned>(d.month()),
                            static_cast<unsigned>(d.day()));
}

ISC_TIME encodeQTime(IUtil *util, const QTime &t)
{
    if (!t.isValid())
        return 0; // see encodeQDate: invalid time -> midnight, matching QIBASE
    return util->encodeTime(static_cast<unsigned>(t.hour()),
                            static_cast<unsigned>(t.minute()),
                            static_cast<unsigned>(t.second()),
                            static_cast<unsigned>(t.msec()) * 10);
}

ISC_TIMESTAMP encodeQDateTime(IUtil *util, const QDateTime &dt)
{
    /* A plain TIMESTAMP carries no zone; Firebird just stores whatever wall-clock
       date/time it is given and returns the same values back verbatim. Treat the
       QDateTime as naive and pass its date()/time() straight through with no
       conversion or relabeling - this matches Firebird's own semantics and how
       other drivers; see decodeFirebirdTimestamp for the matching read side. */
    ISC_TIMESTAMP ts;
    ts.timestamp_date = encodeQDate(util, dt.date());
    ts.timestamp_time = encodeQTime(util, dt.time());
    return ts;
}

/*! \internal
    Firebird names fixed-offset zones by their bare displacement ("+05:30"),
    while Qt uses "UTC+05:30"-style ids for them. Map between the two so
    offset-tagged QDateTimes bind and read back cleanly.
*/
static QByteArray toFirebirdZoneId(const QTimeZone &zone)
{
    QByteArray id = zone.id();
    if (id.size() > 3 && id.startsWith("UTC") && (id.at(3) == '+' || id.at(3) == '-'))
        id.remove(0, 3);
    return id;
}

static QTimeZone fromFirebirdZoneId(const QByteArray &id)
{
    if (id.isEmpty())
        return QTimeZone::utc();
    if (id.at(0) == '+' || id.at(0) == '-')
        return QTimeZone("UTC" + id);
    return QTimeZone(id);
}

ISC_TIMESTAMP_TZ encodeQDateTimeTz(IStatus *iStatus, IUtil *util, const QDateTime &dt)
{
    ThrowStatusWrapper st(iStatus);
    ISC_TIMESTAMP_TZ tstz;
    // Invalid input -> epoch (see encodeQDate), so the engine doesn't reject it.
    const bool valid = dt.isValid();
    const QDate d = valid ? dt.date() : QDate(1858, 11, 17);
    const QTime t = valid ? dt.time() : QTime(0, 0, 0);
    const QByteArray ianaId =
            (valid && dt.timeZone().isValid()) ? toFirebirdZoneId(dt.timeZone()) : "UTC"_ba;
    util->encodeTimeStampTz(&st, &tstz,
                            static_cast<unsigned>(d.year()),
                            static_cast<unsigned>(d.month()),
                            static_cast<unsigned>(d.day()),
                            static_cast<unsigned>(t.hour()),
                            static_cast<unsigned>(t.minute()),
                            static_cast<unsigned>(t.second()),
                            static_cast<unsigned>(t.msec()) * 10,
                            ianaId.constData());
    return tstz;
}

ISC_TIME_TZ encodeQTimeTz(IStatus *iStatus, IUtil *util, const QDateTime &dt)
{
    ThrowStatusWrapper st(iStatus);
    ISC_TIME_TZ ttz;
    const bool valid = dt.isValid();
    const QTime t = valid ? dt.time() : QTime(0, 0, 0);
    const QByteArray ianaId =
            (valid && dt.timeZone().isValid()) ? toFirebirdZoneId(dt.timeZone()) : "UTC"_ba;
    util->encodeTimeTz(&st, &ttz,
                       static_cast<unsigned>(t.hour()),
                       static_cast<unsigned>(t.minute()),
                       static_cast<unsigned>(t.second()),
                       static_cast<unsigned>(t.msec()) * 10,
                       ianaId.constData());
    return ttz;
}

// ---- Date/time decode helpers ----

QDate decodeFirebirdDate(IUtil *util, ISC_DATE date)
{
    unsigned year = 0;
    unsigned month = 0;
    unsigned day = 0;
    util->decodeDate(date, &year, &month, &day);
    return QDate(static_cast<int>(year), static_cast<int>(month), static_cast<int>(day));
}

QTime decodeFirebirdTime(IUtil *util, ISC_TIME time)
{
    unsigned hour = 0;
    unsigned minute = 0;
    unsigned sec = 0;
    unsigned frac = 0;
    util->decodeTime(time, &hour, &minute, &sec, &frac);
    return QTime(static_cast<int>(hour), static_cast<int>(minute),
                 static_cast<int>(sec), static_cast<int>(frac / 10));
}

QDateTime decodeFirebirdTimestamp(IUtil *util, const ISC_TIMESTAMP &ts)
{
    /* A plain TIMESTAMP is naive: return its wall-clock date/time components
       as-is, with no zone conversion or relabeling. This mirrors encodeQDateTime
       (which stores the input's own date()/time() unmodified) and matches
       Firebird's own semantics plus common driver behavior */
    return QDateTime(decodeFirebirdDate(util, ts.timestamp_date),
                     decodeFirebirdTime(util, ts.timestamp_time));
}

QDateTime decodeFirebirdTimestampTz(IStatus *iStatus, IUtil *util,
                                    const ISC_TIMESTAMP_TZ &tstz)
{
    CheckStatusWrapper st(iStatus);
    unsigned year = 0;
    unsigned month = 0;
    unsigned day = 0;
    unsigned hour = 0;
    unsigned minute = 0;
    unsigned sec = 0;
    unsigned frac = 0;
    char tzName[64] = {};
    util->decodeTimeStampTz(&st, &tstz, &year, &month, &day, &hour, &minute, &sec, &frac,
                            sizeof(tzName), tzName);
    const QTimeZone tz = fromFirebirdZoneId(QByteArray(tzName));
    return QDateTime(QDate(static_cast<int>(year), static_cast<int>(month), static_cast<int>(day)),
                     QTime(static_cast<int>(hour), static_cast<int>(minute),
                           static_cast<int>(sec), static_cast<int>(frac / 10)),
                     tz);
}

QDateTime decodeFirebirdTimeTz(IStatus *iStatus, IUtil *util,
                               const ISC_TIME_TZ &ttz)
{
    CheckStatusWrapper st(iStatus);
    unsigned hour = 0;
    unsigned minute = 0;
    unsigned sec = 0;
    unsigned frac = 0;
    char tzName[64] = {};
    util->decodeTimeTz(&st, &ttz, &hour, &minute, &sec, &frac, sizeof(tzName), tzName);
    const QTimeZone tz = fromFirebirdZoneId(QByteArray(tzName));
    return QDateTime(QDate(1970, 1, 1),
                     QTime(static_cast<int>(hour), static_cast<int>(minute),
                           static_cast<int>(sec), static_cast<int>(frac / 10)),
                     tz);
}

/*! \internal
    Encode a UTF-8 string into a CHAR/VARCHAR message slot: VARYING gets a
    2-byte length prefix, TEXT is space-padded; data beyond the declared length
    is truncated. Shared by fillInputBuffer and lookupArrayDesc's bind loop so
    the Firebird text wire format lives in one place.
*/
void encodeTextValue(char *data, int fbType, qsizetype length, const QByteArray &bytes)
{
    if (fbBaseType(fbType) == SQL_VARYING) {
        // VARY header: 2-byte length then data
        const qsizetype len = std::min(length, bytes.size());
        *reinterpret_cast<unsigned short *>(data) = static_cast<unsigned short>(len);
        std::memcpy(data + 2, bytes.constData(), len);
    } else {
        const qsizetype len = std::min(bytes.size(), length);
        std::memcpy(data, bytes.constData(), len);
        if (len < length)
            std::memset(data + len, ' ', length - len);
    }
}

QT_END_NAMESPACE
