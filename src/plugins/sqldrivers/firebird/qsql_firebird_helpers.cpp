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

/*! \internal
    10^n as a 64-bit integer (n >= 0). Firebird numeric scale magnitudes are
    small (NUMERIC/DECIMAL precision <= 38, INT64 <= 18), so this never overflows
    for the integer-backed types it is used with.
*/
qint64 pow10i(int n)
{
    qint64 r = 1;
    for (int i = 0; i < n; ++i)
        r *= 10;
    return r;
}

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
    case SQL_INT128:     return QMetaType::QString; // 128-bit; no native Qt integer type
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

/*! \internal
    Canonical decimal string for binding a value to an INT128/DECFLOAT parameter
    via Firebird's fromString(). Integer and string variants are already plain
    C-locale decimals; floating-point variants are reformatted only when
    QVariant::toByteArray() yields exponential notation (e.g. a large double),
    which IInt128::fromString does not accept.
*/
QByteArray numericInputString(const QVariant &val)
{
    QByteArray s = val.toByteArray();
    const int t = val.typeId();
    if ((t == QMetaType::Double || t == QMetaType::Float)
        && (s.contains('e') || s.contains('E'))) {
        s = QByteArray::number(val.toDouble(), 'f', 18);
        if (s.contains('.')) {
            while (s.endsWith('0'))
                s.chop(1);
            if (s.endsWith('.'))
                s.chop(1);
        }
    }
    return s;
}

/*! \internal
    Convert a plain decimal string (as produced by numericInputString) into the
    scaled integer Firebird stores for a NUMERIC/DECIMAL column: shift the
    decimal point by -scale digits (right for the usual negative scale), round
    half away from zero, and detect qint64 overflow. Works on the digit string,
    never through double — a double round-trip silently drops digits above 2^53
    and an out-of-range double-to-integer cast is undefined behaviour. The read
    path (applyScale) avoids double for the same reason. Exponent notation is
    rejected, matching IInt128::fromString on the INT128 path.
*/
bool decimalToScaledInt64(const QByteArray &decimal, int scale, qint64 *out)
{
    const QByteArray s = decimal.trimmed();
    qsizetype pos = 0;
    bool negative = false;
    if (pos < s.size() && (s[pos] == '+' || s[pos] == '-'))
        negative = (s[pos++] == '-');

    QByteArray intPart, fracPart;
    bool dot = false, anyDigit = false;
    for (; pos < s.size(); ++pos) {
        const char c = s[pos];
        if (c == '.') {
            if (dot)
                return false;
            dot = true;
        } else if (c >= '0' && c <= '9') {
            (dot ? fracPart : intPart).append(c);
            anyDigit = true;
        } else {
            return false;
        }
    }
    if (!anyDigit)
        return false;

    // Shift the decimal point right by -scale digits (left for positive scale).
    const int shift = -scale;
    if (shift >= 0) {
        while (fracPart.size() < shift)
            fracPart.append('0');
        intPart += fracPart.left(shift);
        fracPart.remove(0, shift);
    } else {
        const qsizetype k = -shift;
        while (intPart.size() < k)
            intPart.prepend('0');
        fracPart.prepend(intPart.right(k));
        intPart.chop(k);
    }

    // Round half away from zero on the first remaining fractional digit.
    const bool roundUp = !fracPart.isEmpty() && fracPart[0] >= '5';

    constexpr quint64 maxPos = quint64((std::numeric_limits<qint64>::max)());
    const quint64 limit = negative ? maxPos + 1 : maxPos;
    quint64 magnitude = 0;
    for (const char c : intPart) {
        const unsigned d = unsigned(c - '0');
        if (magnitude > (limit - d) / 10)
            return false; // overflow
        magnitude = magnitude * 10 + d;
    }
    if (roundUp) {
        if (magnitude == limit)
            return false; // overflow
        ++magnitude;
    }

    if (negative) {
        *out = (magnitude == maxPos + 1)
                   ? (std::numeric_limits<qint64>::min)()
                   : -static_cast<qint64>(magnitude);
    } else {
        *out = static_cast<qint64>(magnitude);
    }
    return true;
}

QString int128ToString(IUtil *util, ThrowStatusWrapper &st,
                       const FB_I128 *value, int scale)
{
    char buf[64];
    util->getInt128(&st)->toString(&st, value, scale, sizeof(buf), buf);
    return QString::fromLatin1(buf);
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
