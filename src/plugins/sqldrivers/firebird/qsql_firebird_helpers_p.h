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

// Scaled-numeric / INT128 codec shared with QIBASE; see the INT128 slot adapters below.
#include <QtSql/private/qsqlnumeric_p.h>

#include <ibase.h>               // SQL type constants (SQL_TEXT, SQL_BLOB, …)
#include <firebird/Interface.h>

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
//
//  The policy-aware scaling, exact decimal parsing/formatting and the 128-bit
//  word helpers are the generic QtPrivate::sql* functions in QtSql's private
//  qsqlnumeric_p.h; only the Firebird-specific adapters live here.

QMetaType::Type fbTypeToQt(int fbType, int scale);

/*! \internal
    INT128 slot adapters

    The two adapters own the single native/fallback switch for a 16-byte INT128
    message slot (scale 0 for a plain INT128 parameter or array element), so the
    scalar paths and the array codec cannot diverge. With a 128-bit integer type
    (QSQL_HAS_INT128 from qsqlnumeric_p.h: qint128 on GCC/Clang, std::_Signed128
    on MSVC via Qt's private qinternalint128 alias) the slot is packed and
    unpacked word-wise and converted with the QtPrivate::sql* helpers; without
    one, Firebird's IInt128 interface converts to and from the same canonical
    decimal string. Behaviour is identical either way.

    Encoding reports a failure through errorText/false in both builds — never as
    FbException; callers wrap errorText with their own context. Decoding applies
    the numerical precision policy; when an integer part does not fit a
    LowPrecisionInt target it returns a double instead and, if overflowText is
    given, stores the value's decimal text there so the caller can warn at the
    rate it sees fit (QFirebirdResult::data() runs on every access of a cell).
*/
bool encodeInt128Slot(const QVariant &val, int scale, char *slot,
                      Firebird::IMaster *master, Firebird::IStatus *status,
                      QString &errorText);
QVariant decodeInt128Slot(const char *slot, int scale, QSql::NumericalPrecisionPolicy policy,
                          Firebird::IMaster *master, Firebird::IStatus *status,
                          QString *overflowText = nullptr);

/*! \internal
    IDecFloat16 and IDecFloat34 share identical toString()/fromString() shapes;
    templated on the interface so one helper serves both DECFLOAT widths. The
    64-char caller buffer is ample for DECFLOAT's 34 digits.
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
