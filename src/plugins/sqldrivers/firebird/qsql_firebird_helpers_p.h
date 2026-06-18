// Copyright (C) 2026 The Qt Company Ltd.
// Copyright (C) 2026 Andreas Bacher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#ifndef QSQL_FIREBIRD_HELPERS_P_H
#define QSQL_FIREBIRD_HELPERS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qbytearray.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qvariant.h>
#include <QtSql/qtsqlglobal.h>
#include <QtSql/qsqlerror.h>

#include <ibase.h>               // SQL type constants (SQL_TEXT, SQL_BLOB, …)
#include <firebird/Interface.h>

#include <cmath>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcFirebird)

/*! \internal
    RAII guard for Firebird OO-API handles

    The Firebird OO API splits cleanup across two families: reference-counted
    interfaces freed with release() (IStatement, IResultSet, IBlob, …) and
    IDisposable ones freed with dispose() (IXpbBuilder, IBatchCompletionState).
    FbGuard centralises the "free on every error path" bookkeeping so it is not
    repeated at every call site.

    FbGuard owns one handle and frees it on scope exit with the action supplied
    at construction, unless it has been dismissed. closeWith() performs the
    type-specific graceful shutdown (IResultSet::close, IStatement::free,
    ITransaction::commit, …) which itself consumes the interface reference on
    success — so on success the guard is dismissed (no extra free), and only on a
    failed close does the destructor release the handle. This mirrors exactly the
    "close on the happy path, release on the error path" idiom used throughout.
*/

template <typename T> static void fbRelease(T *h) { h->release(); }
template <typename T> static void fbDispose(T *h) { h->dispose(); }

template <typename T>
class FbGuard
{
public:
    using FreeFn = void (*)(T *);
    /*! \internal
        A discarded unnamed temporary would free the handle immediately while
        later code keeps using it — force the guard to be a named local.
    */
    Q_NODISCARD_CTOR FbGuard(T *handle, FreeFn freeFn) : m_handle(handle), m_free(freeFn) {}
    ~FbGuard() { if (m_handle) m_free(m_handle); }
    /*! \internal
        Intentionally scope-pinned: not copyable, and no move operations —
        ownership transfer goes through take() instead.
    */
    FbGuard(const FbGuard &) = delete;
    FbGuard &operator=(const FbGuard &) = delete;

    T *get() const { return m_handle; }
    T *operator->() const { return m_handle; }
    explicit operator bool() const { return m_handle != nullptr; }

    // Relinquish ownership; the destructor will no longer free the handle.
    T *take() { T *h = m_handle; m_handle = nullptr; return h; }
    void dismiss() { m_handle = nullptr; }

    /*! \internal
        Graceful close. On success the guard is dismissed (close already released
        the interface). If closeFn throws, the destructor still frees the handle;
        the exception is rethrown unless rethrow == false (for noexcept cleanup
        paths that must not propagate).
    */
    template <typename CloseFn>
    void closeWith(CloseFn closeFn, bool rethrow = true)
    {
        if (!m_handle)
            return;
        try {
            closeFn(m_handle);
        } catch (...) {
            m_free(m_handle);
            m_handle = nullptr;
            if (rethrow)
                throw;
            return;
        }
        m_handle = nullptr;
    }

private:
    T *m_handle;
    FreeFn m_free;
};

// Strip nullable bit from Firebird SQL type
inline int fbBaseType(int t) { return t & ~1; }

//  Status/error helpers

QString fbErrorString(Firebird::IMaster *master, Firebird::IStatus *status);
QSqlError fbError(Firebird::IMaster *master, Firebird::IStatus *status,
                  QSqlError::ErrorType type);
QString fbErrorLog(const QSqlError &err);

//  Transaction helpers

// How to finish a transaction
enum class TxnEnd : quint8 {
    Commit,
    Rollback,
};

// Lock-conflict behaviour of a transaction: isc_tpb_wait vs isc_tpb_nowait
enum class TxnWait : quint8 {
    Wait,
    NoWait,
};

void finishAndClear(Firebird::ThrowStatusWrapper &st,
                    Firebird::ITransaction *&tr, TxnEnd end);
Firebird::ITransaction *startDefaultTransaction(Firebird::IAttachment *att,
                                                Firebird::ThrowStatusWrapper &st,
                                                TxnWait wait);

// Map BLR element type (from ISC_ARRAY_DESC) to QMetaType
QMetaType::Type blrTypeToQt(unsigned char blrType, bool hasScale);

// Encode a UTF-8 string into a CHAR/VARCHAR message slot
void encodeTextValue(char *data, int fbType, qsizetype length, const QByteArray &bytes);

//  Numeric codecs

// 10^n as a 64-bit integer (n >= 0) — see qsql_firebird_helpers.cpp.
qint64 pow10i(int n);

/*! \internal
    Format a scaled integer as a high-precision decimal string.
    e.g. val=12345, scale=-2 → "123.45"; val=12, scale=2 → "1200"
*/
template<typename T>
static QString numberToHighPrecision(T val, int scale)
{
    if (scale == 0)
        return QString::number(static_cast<qint64>(val));
    if (scale > 0) {
        // actual = stored * 10^scale → an integer with `scale` trailing zeros
        return QString::number(static_cast<qint64>(val)) + QString(scale, u'0');
    }
    const bool negative = val < 0;
    QString number = QString::number(negative ? -static_cast<qint64>(val)
                                              : static_cast<qint64>(val));
    const int absScale = -scale;
    if (absScale >= number.size())
        number = QString(absScale - number.size() + 1, u'0') + number;
    const int sepPos = number.size() - absScale;
    number = number.left(sepPos) + u'.' + number.mid(sepPos);
    if (negative)
        number = u'-' + number;
    return number;
}


// Return a scaled integer value as the type dictated by numericalPrecisionPolicy.
template<typename T>
static QVariant applyScale(T val, int scale, QSql::NumericalPrecisionPolicy policy)
{
    if (scale == 0)
        return QVariant(static_cast<qint64>(val));
    switch (policy) {
    case QSql::LowPrecisionInt32:
    case QSql::LowPrecisionInt64: {
        /* actual = stored * 10^scale. Scale with integer arithmetic so large
           INT64-backed values keep full precision — a double multiply would
           drop bits above 2^53. */
        const qint64 iv = static_cast<qint64>(val);
        const qint64 scaled = scale < 0 ? iv / pow10i(-scale) : iv * pow10i(scale);
        return policy == QSql::LowPrecisionInt32 ? QVariant(static_cast<qint32>(scaled))
                                                 : QVariant(scaled);
    }
    case QSql::LowPrecisionDouble:
        return QVariant(static_cast<double>(val) * std::pow(10.0, scale));
    case QSql::HighPrecision:
        return QVariant(numberToHighPrecision(val, scale));
    }
    return QVariant(static_cast<double>(val) * std::pow(10.0, scale));
}

QMetaType::Type fbTypeToQt(int fbType, int scale);
QByteArray numericInputString(const QVariant &val);
bool decimalToScaledInt64(const QByteArray &decimal, int scale, qint64 *out);

/*! \internal
    ---- INT128 / DECFLOAT string conversion helpers ----

    Firebird's IInt128/IDecFloat interfaces convert to and from a canonical
    decimal string via a fixed-size caller buffer. These wrappers own the buffer
    (64 chars is ample for INT128's 39 digits and DECFLOAT's 34) so the value
    conversions read as one line at each call site.
*/


QString int128ToString(Firebird::IUtil *util, Firebird::ThrowStatusWrapper &st,
                       const FB_I128 *value, int scale);

/*! \internal
    IDecFloat16 and IDecFloat34 share identical toString()/fromString() shapes;
    templated on the interface so one helper serves both DECFLOAT widths.
*/
template <typename Iface, typename Raw>
static QString decFloatToString(Iface *iface, Firebird::ThrowStatusWrapper &st, const Raw *value)
{
    char buf[64];
    iface->toString(&st, value, sizeof(buf), buf);
    return QString::fromLatin1(buf);
}

template <typename Iface, typename Raw>
static void decFloatFromString(Iface *iface, Firebird::ThrowStatusWrapper &st,
                               const QByteArray &s, Raw *out)
{
    iface->fromString(&st, s.constData(), out);
}

//  Date/time codecs

ISC_DATE encodeQDate(Firebird::IUtil *util, const QDate &d);
ISC_TIME encodeQTime(Firebird::IUtil *util, const QTime &t);
ISC_TIMESTAMP encodeQDateTime(Firebird::IUtil *util, const QDateTime &dt);
ISC_TIMESTAMP_TZ encodeQDateTimeTz(Firebird::IStatus *iStatus, Firebird::IUtil *util,
                                   const QDateTime &dt);
ISC_TIME_TZ encodeQTimeTz(Firebird::IStatus *iStatus, Firebird::IUtil *util,
                          const QDateTime &dt);
QDate decodeFirebirdDate(Firebird::IUtil *util, ISC_DATE date);
QTime decodeFirebirdTime(Firebird::IUtil *util, ISC_TIME time);
QDateTime decodeFirebirdTimestamp(Firebird::IUtil *util, const ISC_TIMESTAMP &ts);
QDateTime decodeFirebirdTimestampTz(Firebird::IStatus *iStatus, Firebird::IUtil *util,
                                    const ISC_TIMESTAMP_TZ &tstz);
QDateTime decodeFirebirdTimeTz(Firebird::IStatus *iStatus, Firebird::IUtil *util,
                               const ISC_TIME_TZ &ttz);

QT_END_NAMESPACE

#endif // QSQL_FIREBIRD_HELPERS_P_H
