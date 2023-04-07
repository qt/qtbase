// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsqlresult.h"

#include "qlist.h"
#include "qsqldriver.h"
#include "qsqlerror.h"
#include "qsqlfield.h"
#include "qsqlrecord.h"
#include "qsqlresult_p.h"
#include "quuid.h"
#include "qvariant.h"
#include "qdatetime.h"
#include "private/qsqldriver_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QString QSqlResultPrivate::holderAt(int index) const
{
    return holders.size() > index ? holders.at(index).holderName : fieldSerial(index);
}

QString QSqlResultPrivate::fieldSerial(qsizetype i) const
{
    return QString(":%1"_L1).arg(i);
}

static bool qIsAlnum(QChar ch)
{
    uint u = uint(ch.unicode());
    // matches [a-zA-Z0-9_]
    return u - 'a' < 26 || u - 'A' < 26 || u - '0' < 10 || u == '_';
}

QString QSqlResultPrivate::positionalToNamedBinding(const QString &query) const
{
    const qsizetype n = query.size();

    QString result;
    result.reserve(n * 5 / 4);
    QChar closingQuote;
    qsizetype count = 0;
    bool ignoreBraces = (sqldriver->dbmsType() == QSqlDriver::PostgreSQL);

    for (qsizetype i = 0; i < n; ++i) {
        QChar ch = query.at(i);
        if (!closingQuote.isNull()) {
            if (ch == closingQuote) {
                if (closingQuote == u']'
                    && i + 1 < n && query.at(i + 1) == closingQuote) {
                    // consume the extra character. don't close.
                    ++i;
                    result += ch;
                } else {
                    closingQuote = QChar();
                }
            }
            result += ch;
        } else {
            if (ch == u'?') {
                result += fieldSerial(count++);
            } else {
                if (ch == u'\'' || ch == u'"' || ch == u'`')
                    closingQuote = ch;
                else if (!ignoreBraces && ch == u'[')
                    closingQuote = u']';
                result += ch;
            }
        }
    }
    result.squeeze();
    return result;
}

QString QSqlResultPrivate::namedToPositionalBinding(const QString &query)
{
    // In the Interbase case if it is an EXECUTE BLOCK then it is up to the
    // caller to make sure that it is not using named bindings for the wrong
    // parts of the query since Interbase uses them literally
    if (sqldriver->dbmsType() == QSqlDriver::Interbase &&
        query.trimmed().startsWith("EXECUTE BLOCK"_L1, Qt::CaseInsensitive))
        return query;

    const qsizetype n = query.size();

    QString result;
    result.reserve(n);
    QChar closingQuote;
    int count = 0;
    qsizetype i = 0;
    bool ignoreBraces = (sqldriver->dbmsType() == QSqlDriver::PostgreSQL);

    while (i < n) {
        QChar ch = query.at(i);
        if (!closingQuote.isNull()) {
            if (ch == closingQuote) {
                if (closingQuote == u']'
                        && i + 1 < n && query.at(i + 1) == closingQuote) {
                    // consume the extra character. don't close.
                    ++i;
                    result += ch;
                } else {
                    closingQuote = QChar();
                }
            }
            result += ch;
            ++i;
        } else {
            if (ch == u':'
                    && (i == 0 || query.at(i - 1) != u':')
                    && (i + 1 < n && qIsAlnum(query.at(i + 1)))) {
                int pos = i + 2;
                while (pos < n && qIsAlnum(query.at(pos)))
                    ++pos;
                QString holder(query.mid(i, pos - i));
                indexes[holder].append(count++);
                holders.append(QHolder(holder, i));
                result += u'?';
                i = pos;
            } else {
                if (ch == u'\'' || ch == u'"' || ch == u'`')
                    closingQuote = ch;
                else if (!ignoreBraces && ch == u'[')
                    closingQuote = u']';
                result += ch;
                ++i;
            }
        }
    }
    result.squeeze();
    values.resize(holders.size());
    return result;
}

/*!
    \class QSqlResult
    \brief The QSqlResult class provides an abstract interface for
    accessing data from specific SQL databases.

    \ingroup database
    \inmodule QtSql

    Normally, you would use QSqlQuery instead of QSqlResult, since
    QSqlQuery provides a generic wrapper for database-specific
    implementations of QSqlResult.

    If you are implementing your own SQL driver (by subclassing
    QSqlDriver), you will need to provide your own QSqlResult
    subclass that implements all the pure virtual functions and other
    virtual functions that you need.

    \sa QSqlDriver
*/

/*!
    \enum QSqlResult::BindingSyntax

    This enum type specifies the different syntaxes for specifying
    placeholders in prepared queries.

    \value PositionalBinding Use the ODBC-style positional syntax, with "?" as placeholders.
    \value NamedBinding Use the Oracle-style syntax with named placeholders (e.g., ":id")

    \sa bindingSyntax()
*/

/*!
    \enum QSqlResult::VirtualHookOperation
    \internal
*/

/*!
    Creates a QSqlResult using database driver \a db. The object is
    initialized to an inactive state.

    \sa isActive(), driver()
*/

QSqlResult::QSqlResult(const QSqlDriver *db)
{
    d_ptr = new QSqlResultPrivate(this, db);
    Q_D(QSqlResult);
    if (d->sqldriver)
        setNumericalPrecisionPolicy(d->sqldriver->numericalPrecisionPolicy());
}

/*!  \internal
*/
QSqlResult::QSqlResult(QSqlResultPrivate &dd)
    : d_ptr(&dd)
{
    Q_D(QSqlResult);
    if (d->sqldriver)
        setNumericalPrecisionPolicy(d->sqldriver->numericalPrecisionPolicy());
}

/*!
    Destroys the object and frees any allocated resources.
*/

QSqlResult::~QSqlResult()
{
    Q_D(QSqlResult);
    delete d;
}

/*!
    Sets the current query for the result to \a query. You must call
    reset() to execute the query on the database.

    \sa reset(), lastQuery()
*/

void QSqlResult::setQuery(const QString& query)
{
    Q_D(QSqlResult);
    d->sql = query;
}

/*!
    Returns the current SQL query text, or an empty string if there
    isn't one.

    \sa setQuery()
*/

QString QSqlResult::lastQuery() const
{
    Q_D(const QSqlResult);
    return d->sql;
}

/*!
    Returns the current (zero-based) row position of the result. May
    return the special values QSql::BeforeFirstRow or
    QSql::AfterLastRow.

    \sa setAt(), isValid()
*/
int QSqlResult::at() const
{
    Q_D(const QSqlResult);
    return d->idx;
}


/*!
    Returns \c true if the result is positioned on a valid record (that
    is, the result is not positioned before the first or after the
    last record); otherwise returns \c false.

    \sa at()
*/

bool QSqlResult::isValid() const
{
    Q_D(const QSqlResult);
    return d->idx != QSql::BeforeFirstRow && d->idx != QSql::AfterLastRow;
}

/*!
    \fn bool QSqlResult::isNull(int index)

    Returns \c true if the field at position \a index in the current row
    is null; otherwise returns \c false.
*/

/*!
    Returns \c true if the result has records to be retrieved; otherwise
    returns \c false.
*/

bool QSqlResult::isActive() const
{
    Q_D(const QSqlResult);
    return d->active;
}

/*!
    This function is provided for derived classes to set the
    internal (zero-based) row position to \a index.

    \sa at()
*/

void QSqlResult::setAt(int index)
{
    Q_D(QSqlResult);
    d->idx = index;
}


/*!
    This function is provided for derived classes to indicate whether
    or not the current statement is a SQL \c SELECT statement. The \a
    select parameter should be true if the statement is a \c SELECT
    statement; otherwise it should be false.

    \sa isSelect()
*/

void QSqlResult::setSelect(bool select)
{
    Q_D(QSqlResult);
    d->isSel = select;
}

/*!
    Returns \c true if the current result is from a \c SELECT statement;
    otherwise returns \c false.

    \sa setSelect()
*/

bool QSqlResult::isSelect() const
{
    Q_D(const QSqlResult);
    return d->isSel;
}

/*!
    Returns the driver associated with the result. This is the object
    that was passed to the constructor.
*/

const QSqlDriver *QSqlResult::driver() const
{
    Q_D(const QSqlResult);
    return d->sqldriver;
}


/*!
    This function is provided for derived classes to set the internal
    active state to \a active.

    \sa isActive()
*/

void QSqlResult::setActive(bool active)
{
    Q_D(QSqlResult);
    if (active)
        d->executedQuery = d->sql;

    d->active = active;
}

/*!
    This function is provided for derived classes to set the last
    error to \a error.

    \sa lastError()
*/

void QSqlResult::setLastError(const QSqlError &error)
{
    Q_D(QSqlResult);
    d->error = error;
}


/*!
    Returns the last error associated with the result.
*/

QSqlError QSqlResult::lastError() const
{
    Q_D(const QSqlResult);
    return d->error;
}

/*!
    \fn int QSqlResult::size()

    Returns the size of the \c SELECT result, or -1 if it cannot be
    determined or if the query is not a \c SELECT statement.

    \sa numRowsAffected()
*/

/*!
    \fn int QSqlResult::numRowsAffected()

    Returns the number of rows affected by the last query executed, or
    -1 if it cannot be determined or if the query is a \c SELECT
    statement.

    \sa size()
*/

/*!
    \fn QVariant QSqlResult::data(int index)

    Returns the data for field \a index in the current row as
    a QVariant. This function is only called if the result is in
    an active state and is positioned on a valid record and \a index is
    non-negative. Derived classes must reimplement this function and
    return the value of field \a index, or QVariant() if it cannot be
    determined.
*/

/*!
    \fn  bool QSqlResult::reset(const QString &query)

    Sets the result to use the SQL statement \a query for subsequent
    data retrieval.

    Derived classes must reimplement this function and apply the \a
    query to the database. This function is only called after the
    result is set to an inactive state and is positioned before the
    first record of the new result. Derived classes should return
    true if the query was successful and ready to be used, or false
    otherwise.

    \sa setQuery()
*/

/*!
    \fn bool QSqlResult::fetch(int index)

    Positions the result to an arbitrary (zero-based) row \a index.

    This function is only called if the result is in an active state.
    Derived classes must reimplement this function and position the
    result to the row \a index, and call setAt() with an appropriate
    value. Return true to indicate success, or false to signify
    failure.

    \sa isActive(), fetchFirst(), fetchLast(), fetchNext(), fetchPrevious()
*/

/*!
    \fn bool QSqlResult::fetchFirst()

    Positions the result to the first record (row 0) in the result.

    This function is only called if the result is in an active state.
    Derived classes must reimplement this function and position the
    result to the first record, and call setAt() with an appropriate
    value. Return true to indicate success, or false to signify
    failure.

    \sa fetch(), fetchLast()
*/

/*!
    \fn bool QSqlResult::fetchLast()

    Positions the result to the last record (last row) in the result.

    This function is only called if the result is in an active state.
    Derived classes must reimplement this function and position the
    result to the last record, and call setAt() with an appropriate
    value. Return true to indicate success, or false to signify
    failure.

    \sa fetch(), fetchFirst()
*/

/*!
    Positions the result to the next available record (row) in the
    result.

    This function is only called if the result is in an active
    state. The default implementation calls fetch() with the next
    index. Derived classes can reimplement this function and position
    the result to the next record in some other way, and call setAt()
    with an appropriate value. Return true to indicate success, or
    false to signify failure.

    \sa fetch(), fetchPrevious()
*/

bool QSqlResult::fetchNext()
{
    return fetch(at() + 1);
}

/*!
    Positions the result to the previous record (row) in the result.

    This function is only called if the result is in an active state.
    The default implementation calls fetch() with the previous index.
    Derived classes can reimplement this function and position the
    result to the next record in some other way, and call setAt()
    with an appropriate value. Return true to indicate success, or
    false to signify failure.
*/

bool QSqlResult::fetchPrevious()
{
    return fetch(at() - 1);
}

/*!
    Returns \c true if you can only scroll forward through the result
    set; otherwise returns \c false.

    \sa setForwardOnly()
*/
bool QSqlResult::isForwardOnly() const
{
    Q_D(const QSqlResult);
    return d->forwardOnly;
}

/*!
    Sets forward only mode to \a forward. If \a forward is true, only
    fetchNext() is allowed for navigating the results. Forward only
    mode needs much less memory since results do not have to be
    cached. By default, this feature is disabled.

    Setting forward only to false is a suggestion to the database engine,
    which has the final say on whether a result set is forward only or
    scrollable. isForwardOnly() will always return the correct status of
    the result set.

    \note Calling setForwardOnly after execution of the query will result
    in unexpected results at best, and crashes at worst.

    \note To make sure the forward-only query completed successfully,
    the application should check lastError() for an error not only after
    executing the query, but also after navigating the query results.

    \warning PostgreSQL: While navigating the query results in forward-only
    mode, do not execute any other SQL command on the same database
    connection. This will cause the query results to be lost.

    \sa isForwardOnly(), fetchNext(), QSqlQuery::setForwardOnly()
*/
void QSqlResult::setForwardOnly(bool forward)
{
    Q_D(QSqlResult);
    d->forwardOnly = forward;
}

/*!
    Prepares the given \a query, using the underlying database
    functionality where possible. Returns \c true if the query is
    prepared successfully; otherwise returns \c false.

    Note: This method should have been called "safePrepare()".

    \sa prepare()
*/
bool QSqlResult::savePrepare(const QString& query)
{
    Q_D(QSqlResult);
    if (!driver())
        return false;
    d->clear();
    d->sql = query;
    if (!driver()->hasFeature(QSqlDriver::PreparedQueries))
        return prepare(query);

    // parse the query to memorize parameter location
    d->executedQuery = d->namedToPositionalBinding(query);

    if (driver()->hasFeature(QSqlDriver::NamedPlaceholders))
        d->executedQuery = d->positionalToNamedBinding(query);

    return prepare(d->executedQuery);
}

/*!
    Prepares the given \a query for execution; the query will normally
    use placeholders so that it can be executed repeatedly. Returns
    true if the query is prepared successfully; otherwise returns \c false.

    \sa exec()
*/
bool QSqlResult::prepare(const QString& query)
{
    Q_D(QSqlResult);
    d->sql = query;
    if (d->holders.isEmpty()) {
        // parse the query to memorize parameter location
        d->namedToPositionalBinding(query);
    }
    return true; // fake prepares should always succeed
}

bool QSqlResultPrivate::isVariantNull(const QVariant &variant)
{
    if (variant.isNull())
        return true;

    switch (variant.typeId()) {
    case qMetaTypeId<QString>():
        return static_cast<const QString*>(variant.constData())->isNull();
    case qMetaTypeId<QByteArray>():
        return static_cast<const QByteArray*>(variant.constData())->isNull();
    case qMetaTypeId<QDateTime>():
        // We treat invalid date-time as null, since its ISODate would be empty.
        return !static_cast<const QDateTime*>(variant.constData())->isValid();
    case qMetaTypeId<QDate>():
        return static_cast<const QDate*>(variant.constData())->isNull();
    case qMetaTypeId<QTime>():
        // As for QDateTime, QTime can be invalid without being null.
        return !static_cast<const QTime*>(variant.constData())->isValid();
    case qMetaTypeId<QUuid>():
        return static_cast<const QUuid*>(variant.constData())->isNull();
    default:
        break;
    }

    return false;
}

/*!
    Executes the query, returning true if successful; otherwise returns
    false.

    \sa prepare()
*/
bool QSqlResult::exec()
{
    Q_D(QSqlResult);
    bool ret;
    // fake preparation - just replace the placeholders..
    QString query = lastQuery();
    if (d->binds == NamedBinding) {
        for (qsizetype i = d->holders.size() - 1; i >= 0; --i) {
            const QString &holder = d->holders.at(i).holderName;
            const QVariant val = d->values.value(d->indexes.value(holder).value(0,-1));
            QSqlField f(""_L1, val.metaType());
            if (QSqlResultPrivate::isVariantNull(val))
                f.setValue(QVariant());
            else
                f.setValue(val);
            query = query.replace(d->holders.at(i).holderPos,
                                   holder.size(), driver()->formatValue(f));
        }
    } else {
        qsizetype i = 0;
        for (const QVariant &var : std::as_const(d->values)) {
            i = query.indexOf(u'?', i);
            if (i == -1)
                continue;
            QSqlField f(""_L1, var.metaType());
            if (QSqlResultPrivate::isVariantNull(var))
                f.clear();
            else
                f.setValue(var);
            const QString val = driver()->formatValue(f);
            query = query.replace(i, 1, val);
            i += val.size();
        }
    }

    // have to retain the original query with placeholders
    QString orig = lastQuery();
    ret = reset(query);
    d->executedQuery = query;
    setQuery(orig);
    d->resetBindCount();
    return ret;
}

/*!
    Binds the value \a val of parameter type \a paramType to position \a index
    in the current record (row).

    \sa addBindValue()
*/
void QSqlResult::bindValue(int index, const QVariant& val, QSql::ParamType paramType)
{
    Q_D(QSqlResult);
    d->binds = PositionalBinding;
    QList<int> &indexes = d->indexes[d->fieldSerial(index)];
    if (!indexes.contains(index))
        indexes.append(index);
    if (d->values.size() <= index)
        d->values.resize(index + 1);
    d->values[index] = val;
    if (paramType != QSql::In || !d->types.isEmpty())
        d->types[index] = paramType;
}

/*!
    \overload

    Binds the value \a val of parameter type \a paramType to the \a
    placeholder name in the current record (row).

    \note Binding an undefined placeholder will result in undefined behavior.

    \sa QSqlQuery::bindValue()
*/
void QSqlResult::bindValue(const QString& placeholder, const QVariant& val,
                           QSql::ParamType paramType)
{
    Q_D(QSqlResult);
    d->binds = NamedBinding;
    // if the index has already been set when doing emulated named
    // bindings - don't reset it
    const QList<int> indexes = d->indexes.value(placeholder);
    for (int idx : indexes) {
        if (d->values.size() <= idx)
            d->values.resize(idx + 1);
        d->values[idx] = val;
        if (paramType != QSql::In || !d->types.isEmpty())
            d->types[idx] = paramType;
    }
}

/*!
    Binds the value \a val of parameter type \a paramType to the next
    available position in the current record (row).

    \sa bindValue()
*/
void QSqlResult::addBindValue(const QVariant& val, QSql::ParamType paramType)
{
    Q_D(QSqlResult);
    d->binds = PositionalBinding;
    bindValue(d->bindCount, val, paramType);
    ++d->bindCount;
}

/*!
    Returns the value bound at position \a index in the current record
    (row).

    \sa bindValue(), boundValues()
*/
QVariant QSqlResult::boundValue(int index) const
{
    Q_D(const QSqlResult);
    return d->values.value(index);
}

/*!
    \overload

    Returns the value bound by the given \a placeholder name in the
    current record (row).

    \sa bindValueType()
*/
QVariant QSqlResult::boundValue(const QString& placeholder) const
{
    Q_D(const QSqlResult);
    const QList<int> indexes = d->indexes.value(placeholder);
    return d->values.value(indexes.value(0,-1));
}

/*!
    Returns the parameter type for the value bound at position \a index.

    \sa boundValue()
*/
QSql::ParamType QSqlResult::bindValueType(int index) const
{
    Q_D(const QSqlResult);
    return d->types.value(index, QSql::In);
}

/*!
    \overload

    Returns the parameter type for the value bound with the given \a
    placeholder name.
*/
QSql::ParamType QSqlResult::bindValueType(const QString& placeholder) const
{
    Q_D(const QSqlResult);
    return d->types.value(d->indexes.value(placeholder).value(0,-1), QSql::In);
}

/*!
    Returns the number of bound values in the result.

    \sa boundValues()
*/
int QSqlResult::boundValueCount() const
{
    Q_D(const QSqlResult);
    return d->values.size();
}

/*!
    Returns a list of the result's bound values for the current
    record (row).

    \sa boundValueCount()
*/
QVariantList QSqlResult::boundValues(QT6_IMPL_NEW_OVERLOAD) const
{
    Q_D(const QSqlResult);
    return d->values;
}

/*!
    \overload

    Returns a mutable reference to the list of the result's bound values
    for the current record (row).

    \sa boundValueCount()
*/
QVariantList &QSqlResult::boundValues(QT6_IMPL_NEW_OVERLOAD)
{
    Q_D(QSqlResult);
    return d->values;
}


/*!
    Returns the binding syntax used by prepared queries.
*/
QSqlResult::BindingSyntax QSqlResult::bindingSyntax() const
{
    Q_D(const QSqlResult);
    return d->binds;
}

/*!
    Clears the entire result set and releases any associated
    resources.
*/
void QSqlResult::clear()
{
    Q_D(QSqlResult);
    d->clear();
}

/*!
    Returns the query that was actually executed. This may differ from
    the query that was passed, for example if bound values were used
    with a prepared query and the underlying database doesn't support
    prepared queries.

    \sa exec(), setQuery()
*/
QString QSqlResult::executedQuery() const
{
    Q_D(const QSqlResult);
    return d->executedQuery;
}

/*!
    Resets the number of bind parameters.
*/
void QSqlResult::resetBindCount()
{
    Q_D(QSqlResult);
    d->resetBindCount();
}

/*!
    Returns the names of all bound values.

    \sa boundValue(), boundValueName()
 */
QStringList QSqlResult::boundValueNames() const
{
    Q_D(const QSqlResult);
    QList<QString> ret;
    for (const QHolder &holder : std::as_const(d->holders))
        ret.push_back(holder.holderName);
    return ret;
}

/*!
    Returns the name of the bound value at position \a index in the
    current record (row).

    \sa boundValue(), boundValueNames()
*/
QString QSqlResult::boundValueName(int index) const
{
    Q_D(const QSqlResult);
    return d->holderAt(index);
}

/*!
    Returns \c true if at least one of the query's bound values is a \c
    QSql::Out or a QSql::InOut; otherwise returns \c false.

    \sa bindValueType()
*/
bool QSqlResult::hasOutValues() const
{
    Q_D(const QSqlResult);
    if (d->types.isEmpty())
        return false;
    QHash<int, QSql::ParamType>::ConstIterator it;
    for (it = d->types.constBegin(); it != d->types.constEnd(); ++it) {
        if (it.value() != QSql::In)
            return true;
    }
    return false;
}

/*!
    Returns the current record if the query is active; otherwise
    returns an empty QSqlRecord.

    The default implementation always returns an empty QSqlRecord.

    \sa isActive()
*/
QSqlRecord QSqlResult::record() const
{
    return QSqlRecord();
}

/*!
    Returns the object ID of the most recent inserted row if the
    database supports it.
    An invalid QVariant will be returned if the query did not
    insert any value or if the database does not report the id back.
    If more than one row was touched by the insert, the behavior is
    undefined.

    Note that for Oracle databases the row's ROWID will be returned,
    while for MySQL databases the row's auto-increment field will
    be returned.

    \sa QSqlDriver::hasFeature()
*/
QVariant QSqlResult::lastInsertId() const
{
    return QVariant();
}

/*! \internal
*/
void QSqlResult::virtual_hook(int, void *)
{
}

/*! \internal
    \since 4.2

    Executes a prepared query in batch mode if the driver supports it,
    otherwise emulates a batch execution using bindValue() and exec().
    QSqlDriver::hasFeature() can be used to find out whether a driver
    supports batch execution.

    Batch execution can be faster for large amounts of data since it
    reduces network roundtrips.

    For batch executions, bound values have to be provided as lists
    of variants (QVariantList).

    Each list must contain values of the same type. All lists must
    contain equal amount of values (rows).

    NULL values are passed in as typed QVariants, for example
    \c {QVariant(QMetaType::fromType<int>())} for an integer NULL value.

    Example:

    \snippet code/src_sql_kernel_qsqlresult.cpp 0

    Here, we insert two rows into a SQL table, with each row containing three values.

    \sa exec(), QSqlDriver::hasFeature()
*/
bool QSqlResult::execBatch(bool arrayBind)
{
    Q_UNUSED(arrayBind);
    Q_D(QSqlResult);

    const QList<QVariant> values = d->values;
    if (values.size() == 0)
        return false;
    const qsizetype batchCount = values.at(0).toList().size();
    const qsizetype valueCount = values.size();
    for (qsizetype i = 0; i < batchCount; ++i) {
        for (qsizetype j = 0; j < valueCount; ++j)
            bindValue(j, values.at(j).toList().at(i), QSql::In);
        if (!exec())
            return false;
    }
    return true;
}

/*! \internal
 */
void QSqlResult::detachFromResultSet()
{
}

/*! \internal
 */
void QSqlResult::setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy policy)
{
    Q_D(QSqlResult);
    d->precisionPolicy = policy;
}

/*! \internal
 */
QSql::NumericalPrecisionPolicy QSqlResult::numericalPrecisionPolicy() const
{
    Q_D(const QSqlResult);
    return d->precisionPolicy;
}

/*! \internal
*/
bool QSqlResult::nextResult()
{
    return false;
}

/*!
    Returns the low-level database handle for this result set
    wrapped in a QVariant or an invalid QVariant if there is no handle.

    \warning Use this with uttermost care and only if you know what you're doing.

    \warning The handle returned here can become a stale pointer if the result
    is modified (for example, if you clear it).

    \warning The handle can be NULL if the result was not executed yet.

    \warning PostgreSQL: in forward-only mode, the handle of QSqlResult can change
    after calling fetch(), fetchFirst(), fetchLast(), fetchNext(), fetchPrevious(),
    nextResult().

    The handle returned here is database-dependent, you should query the type
    name of the variant before accessing it.

    This example retrieves the handle for a sqlite result:

    \snippet code/src_sql_kernel_qsqlresult.cpp 1

    This snippet returns the handle for PostgreSQL or MySQL:

    \snippet code/src_sql_kernel_qsqlresult_snippet.cpp 2

    \sa QSqlDriver::handle()
*/
QVariant QSqlResult::handle() const
{
    return QVariant();
}

QT_END_NAMESPACE
