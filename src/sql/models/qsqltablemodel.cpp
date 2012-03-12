/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtSql module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsqltablemodel.h"

#include "qsqldriver.h"
#include "qsqlerror.h"
#include "qsqlfield.h"
#include "qsqlindex.h"
#include "qsqlquery.h"
#include "qsqlrecord.h"
#include "qsqlresult.h"

#include "qsqltablemodel_p.h"

#include <qdebug.h>

QT_BEGIN_NAMESPACE

/*! \internal
    Populates our record with values.
*/
QSqlRecord QSqlTableModelPrivate::record(const QVector<QVariant> &values) const
{
    QSqlRecord r = rec;
    for (int i = 0; i < r.count() && i < values.count(); ++i)
        r.setValue(i, values.at(i));
    return r;
}

int QSqlTableModelPrivate::nameToIndex(const QString &name) const
{
    return rec.indexOf(strippedFieldName(name));
}

QString QSqlTableModelPrivate::strippedFieldName(const QString &name) const
{
    QString fieldname = name;
    if (db.driver()->isIdentifierEscaped(fieldname, QSqlDriver::FieldName))
        fieldname = db.driver()->stripDelimiters(fieldname, QSqlDriver::FieldName);
    return fieldname;
}

int QSqlTableModelPrivate::insertCount(int maxRow) const
{
    int cnt = 0;
    CacheMap::ConstIterator i = cache.constBegin();
    const CacheMap::ConstIterator e = cache.constEnd();
    for (;
         i != e && (maxRow < 0 || i.key() <= maxRow);
         ++i) {
        if (i.value().insert())
            ++cnt;
    }

    return cnt;
}

void QSqlTableModelPrivate::initRecordAndPrimaryIndex()
{
    rec = db.record(tableName);
    primaryIndex = db.primaryIndex(tableName);
}

void QSqlTableModelPrivate::clear()
{
    sortColumn = -1;
    sortOrder = Qt::AscendingOrder;
    tableName.clear();
    editQuery.clear();
    cache.clear();
    primaryIndex.clear();
    rec.clear();
    filter.clear();
}

void QSqlTableModelPrivate::clearCache()
{
    cache.clear();
}

void QSqlTableModelPrivate::revertCachedRow(int row)
{
    Q_Q(QSqlTableModel);
    ModifiedRow r = cache.value(row);

    switch (r.op()) {
    case QSqlTableModelPrivate::None:
        Q_ASSERT_X(false, "QSqlTableModelPrivate::revertCachedRow()", "Invalid entry in cache map");
        return;
    case QSqlTableModelPrivate::Update:
    case QSqlTableModelPrivate::Delete:
        if (!r.submitted()) {
            cache[row].revert();
            emit q->dataChanged(q->createIndex(row, 0),
                                q->createIndex(row, q->columnCount() - 1));
        }
        break;
    case QSqlTableModelPrivate::Insert: {
            QMap<int, QSqlTableModelPrivate::ModifiedRow>::Iterator it = cache.find(row);
            if (it == cache.end())
                return;
            q->beginRemoveRows(QModelIndex(), row, row);
            it = cache.erase(it);
            while (it != cache.end()) {
                int oldKey = it.key();
                const QSqlTableModelPrivate::ModifiedRow oldValue = it.value();
                cache.erase(it);
                it = cache.insert(oldKey - 1, oldValue);
                ++it;
            }
            q->endRemoveRows();
        break; }
    }
}

bool QSqlTableModelPrivate::exec(const QString &stmt, bool prepStatement,
                                 const QSqlRecord &rec, const QSqlRecord &whereValues)
{
    if (stmt.isEmpty())
        return false;

    // lazy initialization of editQuery
    if (editQuery.driver() != db.driver())
        editQuery = QSqlQuery(db);

    // workaround for In-Process databases - remove all read locks
    // from the table to make sure the editQuery succeeds
    if (db.driver()->hasFeature(QSqlDriver::SimpleLocking))
        const_cast<QSqlResult *>(query.result())->detachFromResultSet();

    if (prepStatement) {
        if (editQuery.lastQuery() != stmt) {
            if (!editQuery.prepare(stmt)) {
                error = editQuery.lastError();
                return false;
            }
        }
        int i;
        for (i = 0; i < rec.count(); ++i) {
            if (rec.isGenerated(i))
                editQuery.addBindValue(rec.value(i));
        }
        for (i = 0; i < whereValues.count(); ++i) {
            if (whereValues.isGenerated(i) && !whereValues.isNull(i))
                editQuery.addBindValue(whereValues.value(i));
        }

        if (!editQuery.exec()) {
            error = editQuery.lastError();
            return false;
        }
    } else {
        if (!editQuery.exec(stmt)) {
            error = editQuery.lastError();
            return false;
        }
    }
    return true;
}

QSqlRecord QSqlTableModelPrivate::primaryValues(int row) const
{
    return cache.value(row).primaryValues(primaryIndex.isEmpty() ? rec : primaryIndex);
}

/*!
    \class QSqlTableModel
    \brief The QSqlTableModel class provides an editable data model
    for a single database table.

    \ingroup database
    \inmodule QtSql

    QSqlTableModel is a high-level interface for reading and writing
    database records from a single table. It is build on top of the
    lower-level QSqlQuery and can be used to provide data to view
    classes such as QTableView. For example:

    \snippet doc/src/snippets/sqldatabase/sqldatabase.cpp 24

    We set the SQL table's name and the edit strategy, then we set up
    the labels displayed in the view header. The edit strategy
    dictates when the changes done by the user in the view are
    actually applied to the database. The possible values are \l
    OnFieldChange, \l OnRowChange, and \l OnManualSubmit.

    QSqlTableModel can also be used to access a database
    programmatically, without binding it to a view:

    \snippet doc/src/snippets/sqldatabase/sqldatabase.cpp 21

    The code snippet above extracts the \c salary field from record 4 in
    the result set of the query \c{SELECT * from employee}.

    It is possible to set filters using setFilter(), or modify the
    sort order using setSort(). At the end, you must call select() to
    populate the model with data.

    The \l{sql/tablemodel} example illustrates how to use
    QSqlTableModel as the data source for a QTableView.

    QSqlTableModel provides no direct support for foreign keys. Use
    the QSqlRelationalTableModel and QSqlRelationalDelegate if you
    want to resolve foreign keys.

    \sa QSqlRelationalTableModel, QSqlQuery, {Model/View Programming},
        {Table Model Example}, {Cached Table Example}
*/

/*!
    \fn QSqlTableModel::beforeDelete(int row)

    This signal is emitted by deleteRowFromTable() before the \a row
    is deleted from the currently active database table.
*/

/*!
    \fn void QSqlTableModel::primeInsert(int row, QSqlRecord &record)

    This signal is emitted by insertRows(), when an insertion is
    initiated in the given \a row of the currently active database
    table. The \a record parameter can be written to (since it is a
    reference), for example to populate some fields with default
    values and set the generated flags of the fields. Do not try to
    edit the record via other means such as setData() or setRecord()
    while handling this signal.
*/

/*!
    \fn QSqlTableModel::beforeInsert(QSqlRecord &record)

    This signal is emitted by insertRowIntoTable() before a new row is
    inserted into the currently active database table. The values that
    are about to be inserted are stored in \a record and can be
    modified before they will be inserted.
*/

/*!
    \fn QSqlTableModel::beforeUpdate(int row, QSqlRecord &record)

    This signal is emitted by updateRowInTable() before the \a row is
    updated in the currently active database table with the values
    from \a record.

    Note that only values that are marked as generated will be updated.
    The generated flag can be set with \l QSqlRecord::setGenerated()
    and checked with \l QSqlRecord::isGenerated().

    \sa QSqlRecord::isGenerated()
*/

/*!
    Creates an empty QSqlTableModel and sets the parent to \a parent
    and the database connection to \a db. If \a db is not valid, the
    default database connection will be used.

    The default edit strategy is \l OnRowChange.
*/
QSqlTableModel::QSqlTableModel(QObject *parent, QSqlDatabase db)
    : QSqlQueryModel(*new QSqlTableModelPrivate, parent)
{
    Q_D(QSqlTableModel);
    d->db = db.isValid() ? db : QSqlDatabase::database();
}

/*!  \internal
*/
QSqlTableModel::QSqlTableModel(QSqlTableModelPrivate &dd, QObject *parent, QSqlDatabase db)
    : QSqlQueryModel(dd, parent)
{
    Q_D(QSqlTableModel);
    d->db = db.isValid() ? db : QSqlDatabase::database();
}

/*!
    Destroys the object and frees any allocated resources.
*/
QSqlTableModel::~QSqlTableModel()
{
}

/*!
    Sets the database table on which the model operates to \a
    tableName. Does not select data from the table, but fetches its
    field information.

    To populate the model with the table's data, call select().

    Error information can be retrieved with \l lastError().

    \sa select(), setFilter(), lastError()
*/
void QSqlTableModel::setTable(const QString &tableName)
{
    Q_D(QSqlTableModel);
    clear();
    d->tableName = tableName;
    d->initRecordAndPrimaryIndex();
    d->initColOffsets(d->rec.count());

    if (d->rec.count() == 0)
        d->error = QSqlError(QLatin1String("Unable to find table ") + d->tableName, QString(),
                             QSqlError::StatementError);
}

/*!
    Returns the name of the currently selected table.
*/
QString QSqlTableModel::tableName() const
{
    Q_D(const QSqlTableModel);
    return d->tableName;
}

/*!
    Populates the model with data from the table that was set via setTable(), using the
    specified filter and sort condition, and returns true if successful; otherwise
    returns false.

    \note Calling select() will revert any unsubmitted changes and remove any inserted columns.

    \sa setTable(), setFilter(), selectStatement()
*/
bool QSqlTableModel::select()
{
    Q_D(QSqlTableModel);
    QString query = selectStatement();
    if (query.isEmpty())
        return false;

    QSqlTableModelPrivate::CacheMap::Iterator it = d->cache.end();
    while (it != d->cache.constBegin()) {
        --it;
        // rows must be accounted for
        if (it.value().insert()) {
            beginRemoveRows(QModelIndex(), it.key(), it.key());
            it = d->cache.erase(it);
            endRemoveRows();
        } else {
            it = d->cache.erase(it);
        }
    }

    QSqlQuery qu(query, d->db);
    setQuery(qu);

    if (!qu.isActive() || lastError().isValid()) {
        // something went wrong - revert to non-select state
        d->initRecordAndPrimaryIndex();
        return false;
    }
    return true;
}

/*!
    \since 5.0

    Refreshes \a row in the model with values from the database table row matching
    on primary key values. Without a primary key, all column values must match. If
    no matching row is found, the model will show an empty row.

    Returns true if successful; otherwise returns false.

    \sa select()
*/
bool QSqlTableModel::selectRow(int row)
{
    Q_D(QSqlTableModel);

    if (row < 0 || row >= rowCount())
        return false;

    const int table_sort_col = d->sortColumn;
    d->sortColumn = -1;
    const QString table_filter = d->filter;
    d->filter = d->db.driver()->sqlStatement(QSqlDriver::WhereStatement,
                                              d->tableName,
                                              d->primaryValues(row),
                                              false);
    if (d->filter.startsWith(QLatin1String("WHERE "), Qt::CaseInsensitive))
        d->filter.remove(0, 6);
    const QString stmt = selectStatement();
    d->sortColumn = table_sort_col;
    d->filter = table_filter;

    QSqlQuery q(d->db);
    q.setForwardOnly(true);
    if (!q.exec(stmt))
        return false;

    bool exists = q.next();
    d->cache[row].refresh(exists, q.record());

    emit headerDataChanged(Qt::Vertical, row, row);
    emit dataChanged(createIndex(row, 0), createIndex(row, columnCount() - 1));

    return true;
}

/*!
    \reimp
*/
QVariant QSqlTableModel::data(const QModelIndex &index, int role) const
{
    Q_D(const QSqlTableModel);
    if (!index.isValid() || (role != Qt::DisplayRole && role != Qt::EditRole))
        return QVariant();

    const QSqlTableModelPrivate::ModifiedRow mrow = d->cache.value(index.row());
    if (mrow.op() != QSqlTableModelPrivate::None)
        return mrow.rec().value(index.column());

    return QSqlQueryModel::data(index, role);
}

/*!
    \reimp
*/
QVariant QSqlTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_D(const QSqlTableModel);
    if (orientation == Qt::Vertical && role == Qt::DisplayRole) {
        const QSqlTableModelPrivate::Op op = d->cache.value(section).op();
        if (op == QSqlTableModelPrivate::Insert)
            return QLatin1String("*");
        else if (op == QSqlTableModelPrivate::Delete)
            return QLatin1String("!");
    }
    return QSqlQueryModel::headerData(section, orientation, role);
}

/*!
    Returns true if the value at the index \a index is dirty, otherwise false.
    Dirty values are values that were modified in the model
    but not yet written into the database.

    If \a index is invalid or points to a non-existing row, false is returned.
*/
bool QSqlTableModel::isDirty(const QModelIndex &index) const
{
    Q_D(const QSqlTableModel);
    if (!index.isValid())
        return false;

    const QSqlTableModelPrivate::ModifiedRow row = d->cache.value(index.row());
    if (row.submitted())
        return false;

    return row.op() == QSqlTableModelPrivate::Insert
           || row.op() == QSqlTableModelPrivate::Delete
           || (row.op() == QSqlTableModelPrivate::Update
               && row.rec().isGenerated(index.column()));
}

/*!
    Sets the data for the item \a index for the role \a role to \a
    value. Depending on the edit strategy, the value might be applied
    to the database at once or cached in the model.

    Returns true if the value could be set or false on error, for
    example if \a index is out of bounds.

    \sa editStrategy(), data(), submit(), submitAll(), revertRow()
*/
bool QSqlTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_D(QSqlTableModel);
    if (d->busyInsertingRows)
        return false;

    if (role != Qt::EditRole)
        return QSqlQueryModel::setData(index, value, role);

    if (!index.isValid() || index.column() >= d->rec.count() || index.row() >= rowCount())
        return false;

    if (d->cache.value(index.row()).op() == QSqlTableModelPrivate::Delete)
        return false;

    if (d->strategy == OnFieldChange && d->cache.value(index.row()).op() != QSqlTableModelPrivate::Insert) {
        revertAll();
    } else if (d->strategy == OnRowChange && !d->cache.isEmpty() && !d->cache.contains(index.row())) {
        submit();
        revertAll();
    }

    QSqlTableModelPrivate::ModifiedRow &row = d->cache[index.row()];

    if (row.op() == QSqlTableModelPrivate::None)
        row = QSqlTableModelPrivate::ModifiedRow(QSqlTableModelPrivate::Update,
                                                 record(index.row()));

    row.setValue(index.column(), value);
    emit dataChanged(index, index);

    if (d->strategy == OnFieldChange && row.op() != QSqlTableModelPrivate::Insert)
        return submit();

    return true;
}

/*!
    This function simply calls QSqlQueryModel::setQuery(\a query).
    You should normally not call it on a QSqlTableModel. Instead, use
    setTable(), setSort(), setFilter(), etc., to set up the query.

    \sa selectStatement()
*/
void QSqlTableModel::setQuery(const QSqlQuery &query)
{
    QSqlQueryModel::setQuery(query);
}

/*!
    Updates the given \a row in the currently active database table
    with the specified \a values. Returns true if successful; otherwise
    returns false.

    This is a low-level method that operates directly on the database
    and should not be called directly. Use setData() to update values.
    The model will decide depending on its edit strategy when to modify
    the database.

    Note that only values that have the generated-flag set are updated.
    The generated-flag can be set with QSqlRecord::setGenerated() and
    tested with QSqlRecord::isGenerated().

    \sa QSqlRecord::isGenerated(), setData()
*/
bool QSqlTableModel::updateRowInTable(int row, const QSqlRecord &values)
{
    Q_D(QSqlTableModel);
    QSqlRecord rec(values);
    emit beforeUpdate(row, rec);

    const QSqlRecord whereValues = d->primaryValues(row);
    bool prepStatement = d->db.driver()->hasFeature(QSqlDriver::PreparedQueries);
    QString stmt = d->db.driver()->sqlStatement(QSqlDriver::UpdateStatement, d->tableName,
                                                rec, prepStatement);
    QString where = d->db.driver()->sqlStatement(QSqlDriver::WhereStatement, d->tableName,
                                                 whereValues, prepStatement);

    if (stmt.isEmpty() || where.isEmpty() || row < 0 || row >= rowCount()) {
        d->error = QSqlError(QLatin1String("No Fields to update"), QString(),
                                 QSqlError::StatementError);
        return false;
    }
    stmt.append(QLatin1Char(' ')).append(where);

    return d->exec(stmt, prepStatement, rec, whereValues);
}


/*!
    Inserts the values \a values into the currently active database table.

    This is a low-level method that operates directly on the database
    and should not be called directly. Use insertRow() and setData()
    to insert values. The model will decide depending on its edit strategy
    when to modify the database.

    Returns true if the values could be inserted, otherwise false.
    Error information can be retrieved with \l lastError().

    \sa lastError(), insertRow(), insertRows()
*/
bool QSqlTableModel::insertRowIntoTable(const QSqlRecord &values)
{
    Q_D(QSqlTableModel);
    QSqlRecord rec = values;
    emit beforeInsert(rec);

    bool prepStatement = d->db.driver()->hasFeature(QSqlDriver::PreparedQueries);
    QString stmt = d->db.driver()->sqlStatement(QSqlDriver::InsertStatement, d->tableName,
                                                rec, prepStatement);

    if (stmt.isEmpty()) {
        d->error = QSqlError(QLatin1String("No Fields to update"), QString(),
                                 QSqlError::StatementError);
        return false;
    }

    return d->exec(stmt, prepStatement, rec, QSqlRecord() /* no where values */);
}

/*!
    Deletes the given \a row from the currently active database table.

    This is a low-level method that operates directly on the database
    and should not be called directly. Use removeRow() or removeRows()
    to delete values. The model will decide depending on its edit strategy
    when to modify the database.

    Returns true if the row was deleted; otherwise returns false.

    \sa removeRow(), removeRows()
*/
bool QSqlTableModel::deleteRowFromTable(int row)
{
    Q_D(QSqlTableModel);
    emit beforeDelete(row);

    const QSqlRecord whereValues = d->primaryValues(row);
    bool prepStatement = d->db.driver()->hasFeature(QSqlDriver::PreparedQueries);
    QString stmt = d->db.driver()->sqlStatement(QSqlDriver::DeleteStatement,
                                                d->tableName,
                                                QSqlRecord(),
                                                prepStatement);
    QString where = d->db.driver()->sqlStatement(QSqlDriver::WhereStatement,
                                                 d->tableName,
                                                 whereValues,
                                                 prepStatement);

    if (stmt.isEmpty() || where.isEmpty()) {
        d->error = QSqlError(QLatin1String("Unable to delete row"), QString(),
                             QSqlError::StatementError);
        return false;
    }
    stmt.append(QLatin1Char(' ')).append(where);

    return d->exec(stmt, prepStatement, QSqlRecord() /* no new values */, whereValues);
}

/*!
    Submits all pending changes and returns true on success.
    Returns false on error, detailed error information can be
    obtained with lastError().

    In OnManualSubmit, on success the model will be repopulated.
    Any views presenting it will lose their selections.

    Note: In OnManualSubmit mode, already submitted changes won't
    be cleared from the cache when submitAll() fails. This allows
    transactions to be rolled back and resubmitted without
    losing data.

    \sa revertAll(), lastError()
*/
bool QSqlTableModel::submitAll()
{
    Q_D(QSqlTableModel);

    bool success = true;

    for (QSqlTableModelPrivate::CacheMap::Iterator it = d->cache.begin();
         it != d->cache.constEnd(); ++it) {
        if (it.value().submitted())
            continue;

        switch (it.value().op()) {
        case QSqlTableModelPrivate::Insert:
            success = insertRowIntoTable(it.value().rec());
            break;
        case QSqlTableModelPrivate::Update:
            success = updateRowInTable(it.key(), it.value().rec());
            break;
        case QSqlTableModelPrivate::Delete:
            success = deleteRowFromTable(it.key());
            break;
        case QSqlTableModelPrivate::None:
            Q_ASSERT_X(false, "QSqlTableModel::submitAll()", "Invalid cache operation");
            break;
        }

        if (success) {
            it.value().setSubmitted();
            if (d->strategy != OnManualSubmit)
                success = selectRow(it.key());
        }

        if (!success)
            break;
    }

    if (success) {
        if (d->strategy == OnManualSubmit)
            success = select();
    }

    return success;
}

/*!
    This reimplemented slot is called by the item delegates when the
    user stopped editing the current row.

    Submits the currently edited row if the model's strategy is set
    to OnRowChange or OnFieldChange. Does nothing for the OnManualSubmit
    strategy.

    Use submitAll() to submit all pending changes for the
    OnManualSubmit strategy.

    Returns true on success; otherwise returns false. Use lastError()
    to query detailed error information.

    Does not automatically repopulate the model. Submitted rows are
    refreshed from the database on success.

    \sa revert(), revertRow(), submitAll(), revertAll(), lastError()
*/
bool QSqlTableModel::submit()
{
    Q_D(QSqlTableModel);
    if (d->strategy == OnRowChange || d->strategy == OnFieldChange)
        return submitAll();
    return true;
}

/*!
    This reimplemented slot is called by the item delegates when the
    user canceled editing the current row.

    Reverts the changes if the model's strategy is set to
    OnRowChange. Does nothing for the other edit strategies.

    Use revertAll() to revert all pending changes for the
    OnManualSubmit strategy or revertRow() to revert a specific row.

    \sa submit(), submitAll(), revertRow(), revertAll()
*/
void QSqlTableModel::revert()
{
    Q_D(QSqlTableModel);
    if (d->strategy == OnRowChange)
        revertAll();
}

/*!
    \enum QSqlTableModel::EditStrategy

    This enum type describes which strategy to choose when editing values in the database.

    \value OnFieldChange  All changes to the model will be applied immediately to the database.
    \value OnRowChange  Changes to a row will be applied when the user selects a different row.
    \value OnManualSubmit  All changes will be cached in the model until either submitAll()
                           or revertAll() is called.

    Note: To prevent inserting only partly initialized rows into the database,
    \c OnFieldChange will behave like \c OnRowChange for newly inserted rows.

    \sa setEditStrategy()
*/


/*!
    Sets the strategy for editing values in the database to \a
    strategy.

    This will revert any pending changes.

    \sa editStrategy(), revertAll()
*/
void QSqlTableModel::setEditStrategy(EditStrategy strategy)
{
    Q_D(QSqlTableModel);
    revertAll();
    d->strategy = strategy;
}

/*!
    Returns the current edit strategy.

    \sa setEditStrategy()
*/
QSqlTableModel::EditStrategy QSqlTableModel::editStrategy() const
{
    Q_D(const QSqlTableModel);
    return d->strategy;
}

/*!
    Reverts all pending changes.

    \sa revert(), revertRow(), submitAll()
*/
void QSqlTableModel::revertAll()
{
    Q_D(QSqlTableModel);

    const QList<int> rows(d->cache.keys());
    for (int i = rows.size() - 1; i >= 0; --i) {
        revertRow(rows.value(i));
    }
}

/*!
    Reverts all changes for the specified \a row.

    \sa revert(), revertAll(), submit(), submitAll()
*/
void QSqlTableModel::revertRow(int row)
{
    if (row < 0)
        return;

    Q_D(QSqlTableModel);
    d->revertCachedRow(row);
}

/*!
    Returns the primary key for the current table, or an empty
    QSqlIndex if the table is not set or has no primary key.

    \sa setTable(), setPrimaryKey(), QSqlDatabase::primaryIndex()
*/
QSqlIndex QSqlTableModel::primaryKey() const
{
    Q_D(const QSqlTableModel);
    return d->primaryIndex;
}

/*!
    Protected method that allows subclasses to set the primary key to
    \a key.

    Normally, the primary index is set automatically whenever you
    call setTable().

    \sa primaryKey(), QSqlDatabase::primaryIndex()
*/
void QSqlTableModel::setPrimaryKey(const QSqlIndex &key)
{
    Q_D(QSqlTableModel);
    d->primaryIndex = key;
}

/*!
    Returns a pointer to the used QSqlDatabase or 0 if no database was set.
*/
QSqlDatabase QSqlTableModel::database() const
{
    Q_D(const QSqlTableModel);
     return d->db;
}

/*!
    Sorts the data by \a column with the sort order \a order.
    This will immediately select data, use setSort()
    to set a sort order without populating the model with data.

    \sa setSort(), select(), orderByClause()
*/
void QSqlTableModel::sort(int column, Qt::SortOrder order)
{
    setSort(column, order);
    select();
}

/*!
    Sets the sort order for \a column to \a order. This does not
    affect the current data, to refresh the data using the new
    sort order, call select().

    \sa select(), orderByClause()
*/
void QSqlTableModel::setSort(int column, Qt::SortOrder order)
{
    Q_D(QSqlTableModel);
    d->sortColumn = column;
    d->sortOrder = order;
}

/*!
    Returns an SQL \c{ORDER BY} clause based on the currently set
    sort order.

    \sa setSort(), selectStatement()
*/
QString QSqlTableModel::orderByClause() const
{
    Q_D(const QSqlTableModel);
    QString s;
    QSqlField f = d->rec.field(d->sortColumn);
    if (!f.isValid())
        return s;
        
    QString table = d->tableName;
    //we can safely escape the field because it would have been obtained from the database
    //and have the correct case
    QString field = d->db.driver()->escapeIdentifier(f.name(), QSqlDriver::FieldName);
    s.append(QLatin1String("ORDER BY ")).append(table).append(QLatin1Char('.')).append(field);
    s += d->sortOrder == Qt::AscendingOrder ? QLatin1String(" ASC") : QLatin1String(" DESC");

    return s;
}

/*!
    Returns the index of the field \a fieldName, or -1 if no corresponding field
    exists in the model.
*/
int QSqlTableModel::fieldIndex(const QString &fieldName) const
{
    Q_D(const QSqlTableModel);
    return d->rec.indexOf(fieldName);
}

/*!
    Returns the SQL \c SELECT statement used internally to populate
    the model. The statement includes the filter and the \c{ORDER BY}
    clause.

    \sa filter(), orderByClause()
*/
QString QSqlTableModel::selectStatement() const
{
    Q_D(const QSqlTableModel);
    QString query;
    if (d->tableName.isEmpty()) {
        d->error = QSqlError(QLatin1String("No table name given"), QString(),
                             QSqlError::StatementError);
        return query;
    }
    if (d->rec.isEmpty()) {
        d->error = QSqlError(QLatin1String("Unable to find table ") + d->tableName, QString(),
                             QSqlError::StatementError);
        return query;
    }

    query = d->db.driver()->sqlStatement(QSqlDriver::SelectStatement,
                                         d->tableName,
                                         d->rec,
                                         false);
    if (query.isEmpty()) {
        d->error = QSqlError(QLatin1String("Unable to select fields from table ") + d->tableName,
                             QString(), QSqlError::StatementError);
        return query;
    }
    if (!d->filter.isEmpty())
        query.append(QLatin1String(" WHERE ")).append(d->filter);
    QString orderBy(orderByClause());
    if (!orderBy.isEmpty())
        query.append(QLatin1Char(' ')).append(orderBy);

    return query;
}

/*!
    Removes \a count columns from the \a parent model, starting at
    index \a column.

    Returns if the columns were successfully removed; otherwise
    returns false.

    \sa removeRows()
*/
bool QSqlTableModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    Q_D(QSqlTableModel);
    if (parent.isValid() || column < 0 || column + count > d->rec.count())
        return false;
    for (int i = 0; i < count; ++i)
        d->rec.remove(column);
    if (d->query.isActive())
        return select();
    return true;
}

/*!
    Removes \a count rows starting at \a row. Since this model
    does not support hierarchical structures, \a parent must be
    an invalid model index.

    When the edit strategy is OnManualSubmit, deletion of rows from
    the database is delayed until submitAll() is called; otherwise,
    deletions are immediate.

    Inserted but not yet submitted rows in the range to be removed
    are immediately removed from the model.

    Before a row is deleted from the database, the beforeDelete()
    signal is emitted.

    If row < 0 or row + count > rowCount(), no action is taken and
    false is returned. Returns true if all rows could be removed;
    otherwise returns false. Detailed database error information
    can be retrieved using lastError().

    \sa removeColumns(), insertRows()
*/
bool QSqlTableModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_D(QSqlTableModel);
    if (parent.isValid() || row < 0 || count <= 0)
        return false;
    else if (row + count > rowCount())
        return false;
    else if (!count)
        return true;

    // Iterate backwards so we don't have to worry about removed rows causing
    // higher cache entries to shift downwards.
    for (int idx = row + count - 1; idx >= row; --idx) {
        QSqlTableModelPrivate::ModifiedRow& mrow = d->cache[idx];
        if (mrow.op() == QSqlTableModelPrivate::Insert) {
            revertRow(idx);
        } else {
            if (mrow.op() == QSqlTableModelPrivate::None)
                mrow = QSqlTableModelPrivate::ModifiedRow(QSqlTableModelPrivate::Delete, record(idx));
            else
                mrow.setOp(QSqlTableModelPrivate::Delete);
            if (d->strategy == OnManualSubmit)
                emit headerDataChanged(Qt::Vertical, idx, idx);
        }
    }

    if (d->strategy != OnManualSubmit)
        return submit();

    return true;
}

/*!
    Inserts \a count empty rows at position \a row. Note that \a
    parent must be invalid, since this model does not support
    parent-child relations.

    Only one row at a time can be inserted when using the
    OnFieldChange or OnRowChange update strategies.

    The primeInsert() signal will be emitted for each new row.
    Connect to it if you want to initialize the new row with default
    values.

    Returns false if the parameters are out of bounds; otherwise
    returns true.

    Does not submit rows, regardless of edit strategy, not even OnFieldChange.

    \sa primeInsert(), insertRecord()
*/
bool QSqlTableModel::insertRows(int row, int count, const QModelIndex &parent)
{
    Q_D(QSqlTableModel);
    if (row < 0 || count <= 0 || row > rowCount() || parent.isValid())
        return false;

    if (d->strategy != OnManualSubmit && count != 1)
        return false;

    d->busyInsertingRows = true;
    beginInsertRows(parent, row, row + count - 1);

    if (d->strategy != OnManualSubmit)
        d->cache.empty();

    if (!d->cache.isEmpty()) {
        QMap<int, QSqlTableModelPrivate::ModifiedRow>::Iterator it = d->cache.end();
        while (it != d->cache.begin() && (--it).key() >= row) {
            int oldKey = it.key();
            const QSqlTableModelPrivate::ModifiedRow oldValue = it.value();
            d->cache.erase(it);
            it = d->cache.insert(oldKey + count, oldValue);
        }
    }

    for (int i = 0; i < count; ++i) {
        d->cache[row + i] = QSqlTableModelPrivate::ModifiedRow(QSqlTableModelPrivate::Insert,
                                                               d->rec);
        emit primeInsert(row + i, d->cache[row + i].recRef());
    }

    endInsertRows();
    d->busyInsertingRows = false;
    return true;
}

/*!
    Inserts the \a record at position \a row. If \a row is negative,
    the record will be appended to the end. Calls insertRows() and
    setRecord() internally.

    Only fields where the generated flag is true will be included.

    Returns true if the record could be inserted, otherwise false.

    Changes are submitted immediately for OnFieldChange and
    OnRowChange. Note the contrast with setRecord() in respect to
    OnRowChange.

    \sa insertRows(), removeRows(), setRecord()
*/
bool QSqlTableModel::insertRecord(int row, const QSqlRecord &record)
{
    Q_D(QSqlTableModel);
    if (row < 0)
        row = rowCount();
    if (!insertRow(row, QModelIndex()))
        return false;
    if (!setRecord(row, record)) {
        if (d->strategy == OnManualSubmit)
            revertRow(row);
        return false;
    }
    if (d->strategy == OnFieldChange || d->strategy == OnRowChange)
        return submit();
    return true;
}

/*! \reimp
*/
int QSqlTableModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const QSqlTableModel);

    if (parent.isValid())
        return 0;

    return QSqlQueryModel::rowCount() + d->insertCount();
}

/*!
    Returns the index of the value in the database result set for the
    given \a item in the model.

    The return value is identical to \a item if no columns or rows
    have been inserted, removed, or moved around.

    Returns an invalid model index if \a item is out of bounds or if
    \a item does not point to a value in the result set.

    \sa QSqlQueryModel::indexInQuery()
*/
QModelIndex QSqlTableModel::indexInQuery(const QModelIndex &item) const
{
    Q_D(const QSqlTableModel);
    if (d->cache.value(item.row()).insert())
        return QModelIndex();

    const int rowOffset = d->insertCount(item.row());
    return QSqlQueryModel::indexInQuery(createIndex(item.row() - rowOffset, item.column(), item.internalPointer()));
}

/*!
    Returns the currently set filter.

    \sa setFilter(), select()
*/
QString QSqlTableModel::filter() const
{
    Q_D(const QSqlTableModel);
    return d->filter;
}

/*!
    Sets the current filter to \a filter.

    The filter is a SQL \c WHERE clause without the keyword \c WHERE
    (for example, \c{name='Josephine')}.

    If the model is already populated with data from a database,
    the model re-selects it with the new filter. Otherwise, the filter
    will be applied the next time select() is called.

    \sa filter(), select(), selectStatement(), orderByClause()
*/
void QSqlTableModel::setFilter(const QString &filter)
{
    Q_D(QSqlTableModel);
    d->filter = filter;
    if (d->query.isActive())
        select();
}

/*! \reimp
*/
void QSqlTableModel::clear()
{
    Q_D(QSqlTableModel);
    d->clear();
    QSqlQueryModel::clear();
}

/*! \reimp
*/
Qt::ItemFlags QSqlTableModel::flags(const QModelIndex &index) const
{
    Q_D(const QSqlTableModel);
    if (index.internalPointer() || index.column() < 0 || index.column() >= d->rec.count()
        || index.row() < 0)
        return 0;
    if (d->rec.field(index.column()).isReadOnly())
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (d->cache.value(index.row()).op() == QSqlTableModelPrivate::Delete)
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

/*!
    Sets the values at the specified \a row to the values of \a
    record for fields where generated flag is true.

    Returns true if all the values could be set; otherwise returns
    false.

    The edit strategy affects automatic submitting.
    With OnFieldChange, setRecord() commits its changed row.
    With OnRowChange, setRecord() does not commit its changed row,
    but making a change to another row causes previous changes to
    be submitted.

    \sa record(), editStrategy()
*/
bool QSqlTableModel::setRecord(int row, const QSqlRecord &values)
{
    Q_D(QSqlTableModel);
    Q_ASSERT_X(row >= 0, "QSqlTableModel::setRecord()", "Cannot set a record to a row less than 0");
    if (d->busyInsertingRows)
        return false;

    if (row >= rowCount())
        return false;

    if (d->cache.value(row).op() == QSqlTableModelPrivate::Delete)
        return false;

    if (d->strategy == OnFieldChange && d->cache.value(row).op() != QSqlTableModelPrivate::Insert)
        revertAll();
    else if (d->strategy == OnRowChange && !d->cache.isEmpty() && !d->cache.contains(row))
        submit();

    // Check field names and remember mapping
    typedef QMap<int, int> Map;
    Map map;
    for (int i = 0; i < values.count(); ++i) {
        if (values.isGenerated(i)) {
            int idx = d->nameToIndex(values.fieldName(i));
            if (idx == -1)
                return false;
            map[i] = idx;
        }
    }

    QSqlTableModelPrivate::ModifiedRow &mrow = d->cache[row];
    if (mrow.op() == QSqlTableModelPrivate::None)
        mrow = QSqlTableModelPrivate::ModifiedRow(QSqlTableModelPrivate::Update,
                                                  record(row));

    Map::const_iterator i = map.constBegin();
    const Map::const_iterator e = map.constEnd();
    for ( ; i != e; ++i)
         mrow.setValue(i.value(), values.value(i.key()));

    if (columnCount())
        emit dataChanged(createIndex(row, 0), createIndex(row, columnCount() - 1));

    if (d->strategy == OnFieldChange)
        return submit();

    return true;
}

QT_END_NAMESPACE
