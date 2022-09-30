// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsqlrecord.h"

#include "qatomic.h"
#include "qdebug.h"
#include "qlist.h"
#include "qsqlfield.h"
#include "qstring.h"
#include "qstringlist.h"

QT_BEGIN_NAMESPACE

class QSqlRecordPrivate
{
public:
    QSqlRecordPrivate();
    QSqlRecordPrivate(const QSqlRecordPrivate &other);

    inline bool contains(int index) { return index >= 0 && index < fields.size(); }
    QString createField(int index, const QString &prefix) const;

    QList<QSqlField> fields;
    QAtomicInt ref;
};

QSqlRecordPrivate::QSqlRecordPrivate() : ref(1)
{
}

QSqlRecordPrivate::QSqlRecordPrivate(const QSqlRecordPrivate &other): fields(other.fields), ref(1)
{
}

/*! \internal
    Just for compat
*/
QString QSqlRecordPrivate::createField(int index, const QString &prefix) const
{
    QString f;
    if (!prefix.isEmpty())
        f = prefix + u'.';
    f += fields.at(index).name();
    return f;
}

/*!
    \class QSqlRecord
    \brief The QSqlRecord class encapsulates a database record.

    \ingroup database
    \ingroup shared
    \inmodule QtSql

    The QSqlRecord class encapsulates the functionality and
    characteristics of a database record (usually a row in a table or
    view within the database). QSqlRecord supports adding and
    removing fields as well as setting and retrieving field values.

    The values of a record's fields can be set by name or position
    with setValue(); if you want to set a field to null use
    setNull(). To find the position of a field by name use indexOf(),
    and to find the name of a field at a particular position use
    fieldName(). Use field() to retrieve a QSqlField object for a
    given field. Use contains() to see if the record contains a
    particular field name.

    When queries are generated to be executed on the database only
    those fields for which isGenerated() is true are included in the
    generated SQL.

    A record can have fields added with append() or insert(), replaced
    with replace(), and removed with remove(). All the fields can be
    removed with clear(). The number of fields is given by count();
    all their values can be cleared (to null) using clearValues().

    \sa QSqlField, QSqlQuery::record()
*/


/*!
    Constructs an empty record.

    \sa isEmpty(), append(), insert()
*/

QSqlRecord::QSqlRecord()
{
    d = new QSqlRecordPrivate();
}

/*!
    Constructs a copy of \a other.

    QSqlRecord is \l{implicitly shared}. This means you can make copies
    of a record in \l{constant time}.
*/

QSqlRecord::QSqlRecord(const QSqlRecord& other)
{
    d = other.d;
    d->ref.ref();
}

/*!
    Sets the record equal to \a other.

    QSqlRecord is \l{implicitly shared}. This means you can make copies
    of a record in \l{constant time}.
*/

QSqlRecord& QSqlRecord::operator=(const QSqlRecord& other)
{
    qAtomicAssign(d, other.d);
    return *this;
}

/*!
    Destroys the object and frees any allocated resources.
*/

QSqlRecord::~QSqlRecord()
{
    if (!d->ref.deref())
        delete d;
}

/*!
    \fn bool QSqlRecord::operator!=(const QSqlRecord &other) const

    Returns \c true if this object is not identical to \a other;
    otherwise returns \c false.

    \sa operator==()
*/

/*!
    Returns \c true if this object is identical to \a other (i.e., has
    the same fields in the same order); otherwise returns \c false.

    \sa operator!=()
*/
bool QSqlRecord::operator==(const QSqlRecord &other) const
{
    return d->fields == other.d->fields;
}

/*!
    Returns the value of the field located at position \a index in
    the record. If \a index is out of bounds, an invalid QVariant
    is returned.

    \sa fieldName(), isNull()
*/

QVariant QSqlRecord::value(int index) const
{
    return d->fields.value(index).value();
}

/*!
    \overload

    Returns the value of the field called \a name in the record. If
    field \a name does not exist an invalid variant is returned.

    \sa indexOf()
*/

QVariant QSqlRecord::value(const QString& name) const
{
    return value(indexOf(name));
}

/*!
    Returns the name of the field at position \a index. If the field
    does not exist, an empty string is returned.

    \sa indexOf()
*/

QString QSqlRecord::fieldName(int index) const
{
    return d->fields.value(index).name();
}

/*!
    Returns the position of the field called \a name within the
    record, or -1 if it cannot be found. Field names are not
    case-sensitive. If more than one field matches, the first one is
    returned.

    \sa fieldName()
*/

int QSqlRecord::indexOf(const QString& name) const
{
    QStringView tableName;
    QStringView fieldName(name);
    const qsizetype idx = name.indexOf(u'.');
    if (idx != -1) {
        tableName = fieldName.left(idx);
        fieldName = fieldName.mid(idx + 1);
    }
    const int cnt = count();
    for (int i = 0; i < cnt; ++i) {
        // Check the passed in name first in case it is an alias using a dot.
        // Then check if both the table and field match when there is a table name specified.
        const auto &currentField = d->fields.at(i);
        const auto &currentFieldName = currentField.name();
        if (currentFieldName.compare(name, Qt::CaseInsensitive) == 0
            || (idx != -1 && currentFieldName.compare(fieldName, Qt::CaseInsensitive) == 0
                && currentField.tableName().compare(tableName, Qt::CaseInsensitive) == 0)) {
            return i;
        }
    }
    return -1;
}

/*!
    Returns the field at position \a index. If the \a index
    is out of range, function returns
    a \l{default-constructed value}.
 */
QSqlField QSqlRecord::field(int index) const
{
    return d->fields.value(index);
}

/*! \overload
    Returns the field called \a name.
 */
QSqlField QSqlRecord::field(const QString &name) const
{
    return field(indexOf(name));
}


/*!
    Append a copy of field \a field to the end of the record.

    \sa insert(), replace(), remove()
*/

void QSqlRecord::append(const QSqlField& field)
{
    detach();
    d->fields.append(field);
}

/*!
    Inserts the field \a field at position \a pos in the record.

    \sa append(), replace(), remove()
 */
void QSqlRecord::insert(int pos, const QSqlField& field)
{
   detach();
   d->fields.insert(pos, field);
}

/*!
    Replaces the field at position \a pos with the given \a field. If
    \a pos is out of range, nothing happens.

    \sa append(), insert(), remove()
*/

void QSqlRecord::replace(int pos, const QSqlField& field)
{
    if (!d->contains(pos))
        return;

    detach();
    d->fields[pos] = field;
}

/*!
    Removes the field at position \a pos. If \a pos is out of range,
    nothing happens.

    \sa append(), insert(), replace()
*/

void QSqlRecord::remove(int pos)
{
    if (!d->contains(pos))
        return;

    detach();
    d->fields.remove(pos);
}

/*!
    Removes all the record's fields.

    \sa clearValues(), isEmpty()
*/

void QSqlRecord::clear()
{
    detach();
    d->fields.clear();
}

/*!
    Returns \c true if there are no fields in the record; otherwise
    returns \c false.

    \sa append(), insert(), clear()
*/

bool QSqlRecord::isEmpty() const
{
    return d->fields.isEmpty();
}


/*!
    Returns \c true if there is a field in the record called \a name;
    otherwise returns \c false.
*/

bool QSqlRecord::contains(const QString& name) const
{
    return indexOf(name) >= 0;
}

/*!
    Clears the value of all fields in the record and sets each field
    to null.

    \sa setValue()
*/

void QSqlRecord::clearValues()
{
    detach();
    int count = d->fields.size();
    for (int i = 0; i < count; ++i)
        d->fields[i].clear();
}

/*!
    Sets the generated flag for the field called \a name to \a
    generated. If the field does not exist, nothing happens. Only
    fields that have \a generated set to true are included in the SQL
    that is generated by QSqlQueryModel for example.

    \sa isGenerated()
*/

void QSqlRecord::setGenerated(const QString& name, bool generated)
{
    setGenerated(indexOf(name), generated);
}

/*!
    \overload

    Sets the generated flag for the field \a index to \a generated.

    \sa isGenerated()
*/

void QSqlRecord::setGenerated(int index, bool generated)
{
    if (!d->contains(index))
        return;
    detach();
    d->fields[index].setGenerated(generated);
}

/*!
    \overload

    Returns \c true if the field \a index is null or if there is no field at
    position \a index; otherwise returns \c false.
*/
bool QSqlRecord::isNull(int index) const
{
    return d->fields.value(index).isNull();
}

/*!
    Returns \c true if the field called \a name is null or if there is no
    field called \a name; otherwise returns \c false.

    \sa setNull()
*/
bool QSqlRecord::isNull(const QString& name) const
{
    return isNull(indexOf(name));
}

/*!
    Sets the value of field \a index to null. If the field does not exist,
    nothing happens.

    \sa setValue()
*/
void QSqlRecord::setNull(int index)
{
    if (!d->contains(index))
        return;
    detach();
    d->fields[index].clear();
}

/*!
    \overload

    Sets the value of the field called \a name to null. If the field
    does not exist, nothing happens.
*/
void QSqlRecord::setNull(const QString& name)
{
    setNull(indexOf(name));
}


/*!
    Returns \c true if the record has a field called \a name and this
    field is to be generated (the default); otherwise returns \c false.

    \sa setGenerated()
*/
bool QSqlRecord::isGenerated(const QString& name) const
{
    return isGenerated(indexOf(name));
}

/*! \overload

    Returns \c true if the record has a field at position \a index and this
    field is to be generated (the default); otherwise returns \c false.

    \sa setGenerated()
*/
bool QSqlRecord::isGenerated(int index) const
{
    return d->fields.value(index).isGenerated();
}

/*!
    Returns the number of fields in the record.

    \sa isEmpty()
*/

int QSqlRecord::count() const
{
    return d->fields.size();
}

/*!
    Sets the value of the field at position \a index to \a val. If the
    field does not exist, nothing happens.

    \sa setNull()
*/

void QSqlRecord::setValue(int index, const QVariant& val)
{
    if (!d->contains(index))
        return;
    detach();
    d->fields[index].setValue(val);
}


/*!
    \overload

    Sets the value of the field called \a name to \a val. If the field
    does not exist, nothing happens.
*/

void QSqlRecord::setValue(const QString& name, const QVariant& val)
{
    setValue(indexOf(name), val);
}


/*! \internal
*/
void QSqlRecord::detach()
{
    qAtomicDetach(d);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QSqlRecord &r)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    const int count = r.count();
    dbg << "QSqlRecord(" << count << ')';
    for (int i = 0; i < count; ++i) {
        dbg.nospace();
        dbg << '\n' << qSetFieldWidth(2) << Qt::right << i << Qt::left << qSetFieldWidth(0) << ':';
        dbg.space();
        dbg << r.field(i) << r.value(i).toString();
    }
    return dbg;
}
#endif

/*!
    \since 5.1
    Returns a record containing the fields represented in \a keyFields set to values
    that match by field name.
*/
QSqlRecord QSqlRecord::keyValues(const QSqlRecord &keyFields) const
{
    QSqlRecord retValues(keyFields);

    for (int i = retValues.count() - 1; i >= 0; --i)
        retValues.setValue(i, value(retValues.fieldName(i)));

    return retValues;
}

QT_END_NAMESPACE
