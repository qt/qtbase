// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsqlfield.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

class QSqlFieldPrivate : public QSharedData
{
public:
    QSqlFieldPrivate(const QString &name,
                     QMetaType type, const QString &tableName) :
        nm(name), table(tableName), def(QVariant()), type(type),
        req(QSqlField::Unknown), len(-1), prec(-1), tp(-1),
        ro(false), gen(true), autoval(false)
    {}

    bool operator==(const QSqlFieldPrivate& other) const
    {
        return (nm == other.nm
                && table == other.table
                && def == other.def
                && type == other.type
                && req == other.req
                && len == other.len
                && prec == other.prec
                && ro == other.ro
                && gen == other.gen
                && autoval == other.autoval);
    }

    QString nm;
    QString table;
    QVariant def;
    QMetaType type;
    QSqlField::RequiredStatus req;
    int len;
    int prec;
    int tp;
    bool ro: 1;
    bool gen: 1;
    bool autoval: 1;
};
QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QSqlFieldPrivate)


/*!
    \class QSqlField
    \brief The QSqlField class manipulates the fields in SQL database tables
    and views.

    \ingroup database
    \ingroup shared
    \inmodule QtSql

    QSqlField represents the characteristics of a single column in a
    database table or view, such as the data type and column name. A
    field also contains the value of the database column, which can be
    viewed or changed.

    Field data values are stored as QVariants. Using an incompatible
    type is not permitted. For example:

    \snippet sqldatabase/sqldatabase.cpp 2

    However, the field will attempt to cast certain data types to the
    field data type where possible:

    \snippet sqldatabase/sqldatabase.cpp 3

    QSqlField objects are rarely created explicitly in application
    code. They are usually accessed indirectly through \l{QSqlRecord}s
    that already contain a list of fields. For example:

    \snippet sqldatabase/sqldatabase.cpp 4
    \dots
    \snippet sqldatabase/sqldatabase.cpp 5
    \snippet sqldatabase/sqldatabase.cpp 6

    A QSqlField object can provide some meta-data about the field, for
    example, its name(), variant type(), length(), precision(),
    defaultValue(), typeID(), and its requiredStatus(),
    isGenerated() and isReadOnly(). The field's data can be
    checked to see if it isNull(), and its value() retrieved. When
    editing the data can be set with setValue() or set to NULL with
    clear().

    \sa QSqlRecord
*/

/*!
    \enum QSqlField::RequiredStatus

    Specifies whether the field is required or optional.

    \value Required  The field must be specified when inserting records.
    \value Optional  The fields doesn't have to be specified when inserting records.
    \value Unknown  The database driver couldn't determine whether the field is required or
                    optional.

    \sa requiredStatus()
*/

/*!
    \fn QSqlField::QSqlField(const QString &fieldName, QVariant::Type type, const QString &table)
    \deprecated [6.0] Use the constructor taking a QMetaType instead.
    \overload

    Constructs an empty field called \a fieldName of variant type \a
    type in \a table.

    \sa setRequiredStatus(), setLength(), setPrecision(), setDefaultValue(),
        setGenerated(), setReadOnly()
*/

/*!
    \fn void QSqlField::swap(QSqlField &other)
    \since 6.6

    Swaps this field with \a other. This function is very fast and
    never fails.
*/

/*!
    \since 6.0

    \overload
    Constructs an empty field called \a fieldName of type \a
    type in \a table.

    \sa setRequiredStatus(), setLength(), setPrecision(), setDefaultValue(),
        setGenerated(), setReadOnly()
*/
QSqlField::QSqlField(const QString &fieldName, QMetaType type, const QString &table)
    : val(QVariant(type, nullptr)),
      d(new QSqlFieldPrivate(fieldName, type, table))
{
}

/*!
    Constructs a copy of \a other.
*/

QSqlField::QSqlField(const QSqlField &other)
    = default;

/*!
    Sets the field equal to \a other.
*/

QSqlField& QSqlField::operator=(const QSqlField& other)
    = default;

/*! \fn bool QSqlField::operator!=(const QSqlField &other) const
    Returns \c true if the field is unequal to \a other; otherwise returns
    false.
*/

/*!
    Returns \c true if the field is equal to \a other; otherwise returns
    false.
*/
bool QSqlField::operator==(const QSqlField& other) const
{
    return ((d == other.d || *d == *other.d)
            && val == other.val);
}

/*!
    Destroys the object and frees any allocated resources.
*/

QSqlField::~QSqlField()
    = default;

/*!
    Sets the required status of this field to \a required.

    \sa requiredStatus(), setMetaType(), setLength(), setPrecision(),
        setDefaultValue(), setGenerated(), setReadOnly()
*/
void QSqlField::setRequiredStatus(RequiredStatus required)
{
    detach();
    d->req = required;
}

/*! \fn void QSqlField::setRequired(bool required)

    Sets the required status of this field to \l Required if \a
    required is true; otherwise sets it to \l Optional.

    \sa setRequiredStatus(), requiredStatus()
*/

/*!
    Sets the field's length to \a fieldLength. For strings this is the
    maximum number of characters the string can hold; the meaning
    varies for other types.

    \sa length(), setMetaType(), setRequiredStatus(), setPrecision(),
        setDefaultValue(), setGenerated(), setReadOnly()
*/
void QSqlField::setLength(int fieldLength)
{
    detach();
    d->len = fieldLength;
}

/*!
    Sets the field's \a precision. This only affects numeric fields.

    \sa precision(), setMetaType(), setRequiredStatus(), setLength(),
        setDefaultValue(), setGenerated(), setReadOnly()
*/
void QSqlField::setPrecision(int precision)
{
    detach();
    d->prec = precision;
}

/*!
    Sets the default value used for this field to \a value.

    \sa defaultValue(), value(), setMetaType(), setRequiredStatus(),
        setLength(), setPrecision(), setGenerated(), setReadOnly()
*/
void QSqlField::setDefaultValue(const QVariant &value)
{
    detach();
    d->def = value;
}

/*!
    \internal
*/
void QSqlField::setSqlType(int type)
{
    detach();
    d->tp = type;
}

/*!
    Sets the generated state. If \a gen is false, no SQL will
    be generated for this field; otherwise, Qt classes such as
    QSqlQueryModel and QSqlTableModel will generate SQL for this
    field.

    \sa isGenerated(), setMetaType(), setRequiredStatus(), setLength(),
        setPrecision(), setDefaultValue(), setReadOnly()
*/
void QSqlField::setGenerated(bool gen)
{
    detach();
    d->gen = gen;
}


/*!
    Sets the value of the field to \a value. If the field is read-only
    (isReadOnly() returns \c true), nothing happens.

    If the data type of \a value differs from the field's current
    data type, an attempt is made to cast it to the proper type. This
    preserves the data type of the field in the case of assignment,
    e.g. a QString to an integer data type.

    To set the value to NULL, use clear().

    \sa value(), isReadOnly(), defaultValue()
*/

void QSqlField::setValue(const QVariant& value)
{
    if (isReadOnly())
        return;
    val = value;
}

/*!
    Clears the value of the field and sets it to NULL.
    If the field is read-only, nothing happens.

    \sa setValue(), isReadOnly(), requiredStatus()
*/

void QSqlField::clear()
{
    if (isReadOnly())
        return;
    val = QVariant(d->type, nullptr);
}

/*!
    Sets the name of the field to \a name.

    \sa name()
*/

void QSqlField::setName(const QString& name)
{
    detach();
    d->nm = name;
}

/*!
    Sets the read only flag of the field's value to \a readOnly. A
    read-only field cannot have its value set with setValue() and
    cannot be cleared to NULL with clear().
*/
void QSqlField::setReadOnly(bool readOnly)
{
    detach();
    d->ro = readOnly;
}

/*!
    \fn QVariant QSqlField::value() const

    Returns the value of the field as a QVariant.

    Use isNull() to check if the field's value is NULL.
*/

/*!
    Returns the name of the field.

    \sa setName()
*/
QString QSqlField::name() const
{
    return d->nm;
}

/*!
    Returns the field's type as stored in the database.
    Note that the actual value might have a different type,
    Numerical values that are too large to store in a long
    int or double are usually stored as strings to prevent
    precision loss.

    \sa setMetaType()
*/
QMetaType QSqlField::metaType() const
{
    return d->type;
}

/*!
    Set's the field's variant type to \a type.

    \sa metaType(), setRequiredStatus(), setLength(), setPrecision(),
        setDefaultValue(), setGenerated(), setReadOnly()
*/
void QSqlField::setMetaType(QMetaType type)
{
    detach();
    d->type = type;
    if (!val.isValid())
        val = QVariant(type, nullptr);
}

/*!
    \fn QVariant::Type QSqlField::type() const
    \deprecated [6.0] Use metaType() instead.

    Returns the field's type as stored in the database.
    Note that the actual value might have a different type,
    Numerical values that are too large to store in a long
    int or double are usually stored as strings to prevent
    precision loss.

    \sa metaType()
*/

/*!
    \fn void QSqlField::setType(QVariant::Type type)
    \deprecated [6.0] Use setMetaType() instead.

    Sets the field's variant type to \a type.

    \sa setMetaType()
*/

/*!
    Returns \c true if the field's value is read-only; otherwise returns
    false.

    \sa setReadOnly(), metaType(), requiredStatus(), length(), precision(),
        defaultValue(), isGenerated()
*/
bool QSqlField::isReadOnly() const
{ return d->ro; }

/*!
    Returns \c true if the field's value is NULL; otherwise returns
    false.

    \sa value()
*/
bool QSqlField::isNull() const
{ return val.isNull(); }

/*! \internal
*/
void QSqlField::detach()
{
    d.detach();
}

/*!
    Returns \c true if this is a required field; otherwise returns \c false.
    An \c INSERT will fail if a required field does not have a value.

    \sa setRequiredStatus(), metaType(), length(), precision(), defaultValue(),
        isGenerated()
*/
QSqlField::RequiredStatus QSqlField::requiredStatus() const
{
    return d->req;
}

/*!
    Returns the field's length.

    If the returned value is negative, it means that the information
    is not available from the database.

    \sa setLength(), metaType(), requiredStatus(), precision(), defaultValue(),
        isGenerated()
*/
int QSqlField::length() const
{
    return d->len;
}

/*!
    Returns the field's precision; this is only meaningful for numeric
    types.

    If the returned value is negative, it means that the information
    is not available from the database.

    \sa setPrecision(), metaType(), requiredStatus(), length(), defaultValue(),
        isGenerated()
*/
int QSqlField::precision() const
{
    return d->prec;
}

/*!
    Returns the field's default value (which may be NULL).

    \sa setDefaultValue(), metaType(), requiredStatus(), length(), precision(),
        isGenerated()
*/
QVariant QSqlField::defaultValue() const
{
    return d->def;
}

/*!
    \internal

    Returns the type ID for the field.

    If the returned value is negative, it means that the information
    is not available from the database.
*/
int QSqlField::typeID() const
{
    return d->tp;
}

/*!
    Returns \c true if the field is generated; otherwise returns
    false.

    \sa setGenerated(), metaType(), requiredStatus(), length(), precision(),
        defaultValue()
*/
bool QSqlField::isGenerated() const
{
    return d->gen;
}

/*!
    Returns \c true if the field's variant type is valid; otherwise
    returns \c false.
*/
bool QSqlField::isValid() const
{
    return d->type.isValid();
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QSqlField &f)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QSqlField(" << f.name() << ", " << f.metaType().name();
    dbg << ", tableName: " << (f.tableName().isEmpty() ? QStringLiteral("(not specified)") : f.tableName());
    if (f.length() >= 0)
        dbg << ", length: " << f.length();
    if (f.precision() >= 0)
        dbg << ", precision: " << f.precision();
    if (f.requiredStatus() != QSqlField::Unknown)
        dbg << ", required: "
            << (f.requiredStatus() == QSqlField::Required ? "yes" : "no");
    dbg  << ", generated: " << (f.isGenerated() ? "yes" : "no");
    if (f.typeID() >= 0)
        dbg << ", typeID: " << f.typeID();
    if (!f.defaultValue().isNull())
        dbg << ", defaultValue: \"" << f.defaultValue() << '\"';
    dbg << ", autoValue: " << f.isAutoValue()
        << ", readOnly: " << f.isReadOnly() << ')';
    return dbg;
}
#endif

/*!
    Returns \c true if the value is auto-generated by the database,
    for example auto-increment primary key values.

    \note When using the ODBC driver, due to limitations in the ODBC API,
    the \c isAutoValue() field is only populated in a QSqlField resulting from a
    QSqlRecord obtained by executing a \c SELECT query. It is \c false in a QSqlField
    resulting from a QSqlRecord returned from QSqlDatabase::record() or
    QSqlDatabase::primaryIndex().

    \sa setAutoValue()
*/
bool QSqlField::isAutoValue() const
{
    return d->autoval;
}

/*!
    Marks the field as an auto-generated value if \a autoVal
    is true.

    \sa isAutoValue()
 */
void QSqlField::setAutoValue(bool autoVal)
{
    detach();
    d->autoval = autoVal;
}

/*!
    Sets the tableName of the field to \a table.

    \sa tableName()
*/
void QSqlField::setTableName(const QString &table)
{
    detach();
    d->table = table;
}

/*!
    Returns the tableName of the field.

    \note When using the QPSQL driver, due to limitations in the libpq library,
    the \c tableName() field is not populated in a QSqlField resulting
    from a QSqlRecord obtained by QSqlQuery::record() of a forward-only query.

    \sa setTableName()
*/
QString QSqlField::tableName() const
{
    return d->table;
}

QT_END_NAMESPACE
