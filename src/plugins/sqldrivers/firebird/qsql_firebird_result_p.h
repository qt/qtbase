// Copyright (C) 2026 The Qt Company Ltd.
// Copyright (C) 2026 Andreas Bacher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#ifndef QSQL_FIREBIRD_RESULT_P_H
#define QSQL_FIREBIRD_RESULT_P_H

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

#include "qsql_firebird_array_p.h"
#include "qsql_firebird_driver_p.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtSql/qsqlrecord.h>
#include <QtSql/private/qsqlresult_p.h>

#include <ibase.h>               // ISC_QUAD, ISC_ARRAY_DESC
#include <firebird/Interface.h>

#include <functional>

QT_BEGIN_NAMESPACE

/*! \internal
    Transaction-control statement kind (bare COMMIT/ROLLBACK), handled via the
    ITransaction API rather than executed as DSQL.
*/
enum class TxnOp : quint8 {
    None,
    Commit,
    Rollback,
};

struct ColumnInfo {
    QString name;
    QString relation;   // table name (for array lookup)
    QString field;      // column name in table (for array lookup)
    int fbType = 0;
    int fbSubType = 0;
    int scale = 0;
    unsigned length = 0;  // byte length from metadata
    unsigned offset = 0;
    unsigned nullOffset = 0;
    bool nullable = true;
    QMetaType::Type qtType = QMetaType::UnknownType;
};

/*! \internal
    Defined ahead of QFirebirdResultPrivate: Q_DECLARE_PUBLIC below static_casts
    QSqlResult* to QFirebirdResult*, which needs the complete type here.
*/
class QFirebirdResult : public QSqlResult
{
    Q_DECLARE_PRIVATE(QFirebirdResult)
    friend class QFirebirdDriver;
    friend class QFirebirdDriverPrivate;

public:
    explicit QFirebirdResult(const QFirebirdDriver *driver);
    ~QFirebirdResult() override;

    QVariant handle() const override;

protected:
    bool prepare(const QString &query) override;
    bool exec() override;
    bool execBatch(bool arrayBind = false) override;

    QVariant data(int field) override;
    bool isNull(int field) override;
    bool reset(const QString &query) override;
    int size() override;
    int numRowsAffected() override;
    void detachFromResultSet() override;

    bool fetch(int i) override;
    bool fetchFirst() override;
    bool fetchLast() override;
    bool fetchNext() override;
    bool fetchPrevious() override;

    QSqlRecord record() const override;

private:
    void cleanup();
    /*! \internal
        After EXECUTE PROCEDURE, copy output columns back into bound QSql::Out
        parameters so they can be read via QSqlQuery::boundValue().
    */
    void writeOutValues();
    /*! \internal
        Cache-mode (FB_SCROLLABLE_CACHE) fetch: serve absolute row `target` from
        the client-side row cache, populating it from the server as needed.
    */
    bool fetchCached(int target);
};

class QFirebirdResultPrivate : public QSqlResultPrivate
{
    Q_DECLARE_PUBLIC(QFirebirdResult)
    Q_DECLARE_SQLDRIVER_PRIVATE(QFirebirdDriver)
public:
    using QSqlResultPrivate::QSqlResultPrivate;

    ~QFirebirdResultPrivate()
    {
        cleanup();
        /* Deregister from the driver, unless the driver has already been
           destroyed (which sets driverAlive = false and clears its list). */
        if (driverAlive)
            drv_d_func()->activeResults.removeOne(this);
    }

    void cleanup();

    /*! \internal
        Close the open cursor (if any), releasing the handle if the close throws.
        Assumes the attachment is alive — only exec()/execBatch() call it, and only
        while executing. cleanup() handles the detached case inline.
    */
    void closeCursor();

    /*! \internal
        Set to false by ~QFirebirdDriver if the driver object is destroyed while
        this result is still alive. Once false, cleanup() must not dereference the
        (now-freed) driver private.
    */
    bool driverAlive = true;

    /*! \internal
        Returns the driver's master/status/attachment/transaction without copying
        them. These are const accessors that hand back mutable Firebird interface
        pointers by design: the Firebird OO API has no const-qualified interfaces,
        and result fetching (e.g. data() const reading a BLOB) is inherently a
        mutating server operation, so logical const cannot propagate here.
    */
    Firebird::IMaster  *master()  const { return drv_d_func()->master; }
    Firebird::IStatus  *status()  const { return drv_d_func()->iStatus; }
    Firebird::IAttachment *att()  const { return drv_d_func()->iAtt; }
    Firebird::ITransaction *activeTransaction() const
    { return drv_d_func()->iTrans ? drv_d_func()->iTrans : autoTrans; }

    // Ensure a transaction is active; auto-starts one if needed.
    Firebird::ITransaction *ensureTransaction();

    // Build input message from bound values and fill inBuffer.
    bool buildInputMessage(QString &errorText);

    /*! \internal
        Core helper: fill inBuffer from a flat list of values (one per parameter,
        metadata taken from the inCols cache). blobWriter, when set, is called
        for SQL_BLOB parameters instead of using att->createBlob().
        Signature: bool(ISC_QUAD &blobId, const QByteArray &data).
        Returns false on error.
    */
    using BlobWriter = std::function<bool(ISC_QUAD &, const QByteArray &)>;
    bool fillInputBuffer(const QList<QVariant> &vals,
                         QString &errorText, BlobWriter blobWriter = {});

    /*! \internal
        Per-value encoders, factored out of fillInputBuffer so it stays focused
        on parameter iteration and NULL handling. Both may throw FbException,
        which exec()/execBatch translate into a QSqlError.
        encodeScaledNumeric returns false (with errorText set) for values that
        cannot be represented exactly in the column's range.
    */
    bool encodeScaledNumeric(char *data, int fbType, int scale, const QVariant &val,
                             QString &errorText);
    void writeInlineBlob(ISC_QUAD &blobId, const QByteArray &blobData);

    /*! \internal
        Adopt a transaction handle that IStatement::execute() returned in place of
        the one it was given, releasing the replaced handle exactly once.
    */
    void adoptReplacementTransaction(Firebird::ITransaction *executedTr,
                                     Firebird::ITransaction *newTr);

    Firebird::IStatement *stmt = nullptr;
    Firebird::IResultSet *cursor = nullptr;
    Firebird::ITransaction *autoTrans = nullptr;  // auto-started transaction (non-user)

    Firebird::IMessageMetadata *outMeta = nullptr;
    Firebird::IMessageMetadata *inMeta = nullptr;

    QList<ColumnInfo> cols;    // output columns, cached at prepare
    QList<ColumnInfo> inCols;  // input parameters, cached at prepare (metadata
                               // is immutable after prepare; avoids re-reading
                               // it per parameter on every exec/batch row)
    QByteArray outBuffer;  // row buffer for output
    QByteArray inBuffer;   // parameter message buffer

    /*! \internal
        Array descriptors resolved by cachedArrayDesc, keyed by relation/field.
        Valid while the statement lives; cleared in cleanup(). mutable: data()
        reads through a const private pointer and only populates the cache.
    */
    mutable ArrayDescCache arrayDescCache;

    /*! \internal
        getAffectedRecords() costs a statement-info round trip, so exec() only
        marks the count as pending and numRowsAffected() resolves it on demand
        (QIBASE defers the same way). -1 = unknown/none.
    */
    static constexpr int AffectedPending = -2;
    int affectedRows = -1;
    bool isSelect = false;
    bool isProcExec = false; // EXECUTE PROCEDURE with output params
    bool procRowFetched = false; // the single proc output row has been consumed

    TxnOp txnOp = TxnOp::None; // see TxnOp

    /*! \internal
        ---- Opt-in client-side row cache (FB_SCROLLABLE_CACHE) ----
        Enabled in exec() for scrollable SELECTs when the driver flag is set.
        rowCache holds a raw snapshot of outBuffer per absolute row index; on a
        cached fetch the snapshot is copied back into outBuffer so data() works
        unchanged (BLOB/array columns cache their blob id, valid for the txn).
    */
    bool useRowCache = false;
    bool cacheComplete = false;     // true once the cursor has been read to EOF
    QList<QByteArray> rowCache;

    /*! \internal
        Pull rows from the server cursor (sequential fetchNext, which benefits
        from fbclient read-ahead) into rowCache until it holds at least index+1
        rows or the cursor is exhausted. Returns true if rowCache[index] exists.
        May throw FbException (callers wrap in try/catch as elsewhere).
    */
    bool ensureCachedRow(int index);

    /*! \internal
        QSqlRecord is derived solely from the (immutable-after-prepare) column
        metadata, so build it once and hand back copies — see record().
    */
    mutable QSqlRecord cachedRecord;
    mutable bool recordCached = false;

    QString formatFbError(const QString &ctx);
};

QT_END_NAMESPACE

#endif // QSQL_FIREBIRD_RESULT_P_H
